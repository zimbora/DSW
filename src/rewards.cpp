// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The PIVX developers
// Copyright (c) 2021-2024 The DECENOMY Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fs.h"
#include "logging.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "rewards.h"
#include "sqlite3/sqlite3.h"
#include "timedata.h"
#include "utilmoneystr.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <unordered_map>

std::unordered_map<int, CAmount> mDynamicRewards;

sqlite3* db = nullptr;
sqlite3_stmt* insertStmt = nullptr;
sqlite3_stmt* deleteStmt = nullptr;

bool CRewards::Init(bool fReindex)
{
    std::ostringstream oss;
    auto ok = true;

    try
    {
        const auto& filename = (GetDataDir() / "chainstate" / "rewards.db").c_str();

        // Delete the database file if it exists when reindexing
        if(fReindex) 
        {
            if (auto file = std::fopen(filename, "r")) {
                std::fclose(file);
                // File exists, delete it
                if (std::remove(filename) == 0) {
                    oss << __func__ << " Deleted existing database file: " << filename << std::endl;
                } else {
                    oss << __func__ << " Failed to delete existing database file: " << filename << std::endl;
                    ok = false;
                }
            }
        }

        if(ok) { // Create and/or open the database
            oss << __func__ << " Opening database: " << filename << std::endl;
            auto rc = sqlite3_open(filename, &db);

            if (rc) 
            {
                oss << __func__ << " Can't open database: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
        }

        if(ok) { // database is open and working
            // Create the rewards table if not exists
            const auto create_table_query = "CREATE TABLE IF NOT EXISTS rewards (height INT PRIMARY KEY, amount INTEGER)";
            auto rc = sqlite3_exec(db, create_table_query, NULL, NULL, NULL);

            if (rc != SQLITE_OK) {
                oss << __func__ << " SQL error: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
        }

        if(ok) { // Create insert statement
            const std::string insertSql = "INSERT INTO rewards (height, amount) VALUES (?, ?)";
            auto rc = sqlite3_prepare_v2(db, insertSql.c_str(), insertSql.length(), &insertStmt, nullptr);
            if (rc != SQLITE_OK) {
                oss << __func__ << " SQL error: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
        }

        if(ok) { // Create delete statement
            const std::string deleteSql = "DELETE FROM rewards WHERE height = ?";
            auto rc = sqlite3_prepare_v2(db, deleteSql.c_str(), deleteSql.length(), &deleteStmt, nullptr);
            if (rc != SQLITE_OK) {
                oss << __func__ << " SQL error: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
        }

        if(ok) { // Loads the database into the in-memory map
            const char* sql = "SELECT height, amount FROM rewards";
            auto rc = sqlite3_exec(db, sql, [](void* data, int argc, char** argv, char** /* azColName */) -> int {
                int height = std::stoi(argv[0]);
                CAmount amount = std::stol(argv[1]);
                mDynamicRewards[height] = amount;
                return 0;
            }, nullptr, nullptr);

            if (rc != SQLITE_OK) {
                oss << __func__ <<  " SQL error: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
        }

        if(ok && mDynamicRewards.size() > 0) { // Printing the map
            oss << __func__ <<  " Dynamic Rewards:" << std::endl;
            for (const auto& pair : mDynamicRewards) {
                oss << __func__ <<  " Height: " << pair.first << ", Amount: " << FormatMoney(pair.second) << std::endl;
            }
        }
    }
    catch(const std::exception& e)
    {
        oss << __func__ << " An exception was thrown: " << e.what() << std::endl;
        ok = false;
    }

    if (!oss.str().empty()) LogPrintf("%s: %s", __func__, oss.str());
    
    return ok;
}

void CRewards::Shutdown()
{
    if(insertStmt != nullptr) sqlite3_finalize(insertStmt);
    if(deleteStmt != nullptr) sqlite3_finalize(deleteStmt);
    if(db != nullptr) sqlite3_close(db);

    return;
}

int CRewards::GetDynamicRewardsEpoch(int nHeight)
{
    auto& consensus = Params().GetConsensus();
    const auto nRewardAdjustmentInterval = consensus.nRewardAdjustmentInterval;
    return nHeight / nRewardAdjustmentInterval;
}

int CRewards::GetDynamicRewardsEpochHeight(int nHeight)
{
    auto& consensus = Params().GetConsensus();
    const auto nRewardAdjustmentInterval = consensus.nRewardAdjustmentInterval;
    return GetDynamicRewardsEpoch(nHeight) * nRewardAdjustmentInterval;
}

bool CRewards::IsDynamicRewardsEpochHeight(int nHeight)
{
    return GetDynamicRewardsEpochHeight(nHeight) == nHeight;
}

bool CRewards::ConnectBlock(CBlockIndex* pindex, CAmount nSubsidy, CCoinsViewCache& coins)
{
    auto& consensus = Params().GetConsensus();
    const auto nHeight = pindex->nHeight;
    std::ostringstream oss;
    auto ok = true;

    if (consensus.NetworkUpgradeActive(nHeight, Consensus::UPGRADE_DYNAMIC_REWARDS))
    {
        CAmount nNewSubsidy = 0;

        if (
            masternodeSync.IsSynced() &&
            IsDynamicRewardsEpochHeight(nHeight)  
        ) {
            auto nBlocksPerDay = DAY_IN_SECONDS / consensus.nTargetSpacing;
            auto nBlocksPerWeek = WEEK_IN_SECONDS / consensus.nTargetSpacing;
            auto nBlocksPerMonth = MONTH_IN_SECONDS / consensus.nTargetSpacing;

            // get total money supply
            const auto nMoneySupply = pindex->nMoneySupply.get();
            oss << __func__ << " nMoneySupply: " << FormatMoney(nMoneySupply) << std::endl;

            // get the current masternode collateral, and the next week collateral
            auto nCollateralAmount = CMasternode::GetMasternodeNodeCollateral(nHeight);
            auto nNextWeekCollateralAmount = CMasternode::GetMasternodeNodeCollateral(nHeight + nBlocksPerWeek);

            // calculate the current circulating supply
            CAmount nCirculatingSupply = 0;
            std::unique_ptr<CCoinsViewCursor> pcursor(coins.Cursor());

            while (pcursor->Valid()) {
                COutPoint key;
                Coin coin;
                if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
                    // ----------- burn address scanning -----------
                    CTxDestination source;
                    if (ExtractDestination(coin.out.scriptPubKey, source)) {
                        const std::string addr = EncodeDestination(source);
                        if (consensus.mBurnAddresses.find(addr) != consensus.mBurnAddresses.end() &&
                            consensus.mBurnAddresses.at(addr) < nHeight
                        ) {
                            pcursor->Next(); // Skip
                            continue;
                        }
                    }

                    // ----------- masternode collaterals scanning ----------- 
                    if(
                        coin.out.nValue == nCollateralAmount || 
                        coin.out.nValue == nNextWeekCollateralAmount
                    ) {
                        pcursor->Next(); // Skip
                        continue;
                    }

                    // ----------- UTXOs age related scanning -----------
                    auto nBlocksDiff = static_cast<int64_t>(nHeight - coin.nHeight);
                    const auto nMultiplier = 100000000L;

                    // y = mx + b 
                    // 3 months old or less => 100%
                    // 12 months old or greater => 0%
                    auto nSupplyWeightRatio = 
                        std::min(
                            std::max(
                                (100L * nMultiplier - (((100L * nMultiplier)/(9L * nBlocksPerMonth)) * (nBlocksDiff - 3L * nBlocksPerMonth))) / nMultiplier, 
                            0L), 
                        100L);

                    nCirculatingSupply += coin.out.nValue * nSupplyWeightRatio / 100L;
                }

                pcursor->Next();
            }
            oss << __func__ << " nCirculatingSupply: " << FormatMoney(nCirculatingSupply) << std::endl;

            // calculate target emissions
            const auto nRewardAdjustmentInterval = consensus.nRewardAdjustmentInterval;
            oss << __func__ << " nRewardAdjustmentInterval: " << nRewardAdjustmentInterval << std::endl;
            const auto nTotalEmissionRate = sporkManager.GetSporkValue(SPORK_116_TOT_SPLY_TRGT_EMISSION);
            oss << __func__ << " nTotalEmissionRate: " << nTotalEmissionRate << std::endl;
            const auto nCirculatingEmissionRate = sporkManager.GetSporkValue(SPORK_117_CIRC_SPLY_TRGT_EMISSION);
            oss << __func__ << " nCirculatingEmissionRate: " << nCirculatingEmissionRate << std::endl;
            const auto nActualEmission = nSubsidy * nRewardAdjustmentInterval;
            oss << __func__ << " nActualEmission: " << FormatMoney(nActualEmission) << std::endl;
            const auto nSupplyTargetEmission = ((nMoneySupply / (365L * nBlocksPerDay)) / 1000000) * nTotalEmissionRate * nRewardAdjustmentInterval;
            oss << __func__ << " nSupplyTargetEmission: " << FormatMoney(nSupplyTargetEmission) << std::endl;
            const auto nCirculatingTargetEmission = ((nCirculatingSupply / (365L * nBlocksPerDay)) / 1000000) * nCirculatingEmissionRate * nRewardAdjustmentInterval;
            oss << __func__ << " nCirculatingTargetEmission: " << FormatMoney(nCirculatingTargetEmission) << std::endl;

            // calculate required delta values
            const auto nDelta = (nActualEmission - std::max(nSupplyTargetEmission, nCirculatingTargetEmission)) / nRewardAdjustmentInterval;
            oss << __func__ << " nDelta: " << FormatMoney(nDelta) << std::endl;

            const auto nRatio = (nDelta * 100) / nSubsidy; // percentage of the difference on emissions and the current reward 
            oss << __func__ << " nRatio: " << nRatio << std::endl;
            
            // y = mx + b
            // 0% ratio => 10%
            // 100% ration => 0% 
            const auto nDampedDelta = nDelta * ((-nRatio / 10) + 10) / 100;
            oss << __func__ << " nDampedDelta: " << FormatMoney(nDampedDelta) << std::endl; 

            // adjust the reward for this epoch
            nNewSubsidy = nSubsidy - nDampedDelta;

            oss << __func__ << " Adjustment at height " << nHeight << ": " << FormatMoney(nSubsidy) << " => " << FormatMoney(nNewSubsidy) << std::endl;
        }

        if ( // if the wallet is syncing get the reward value from the first block of the epoch
            !masternodeSync.IsSynced() &&
            IsDynamicRewardsEpochHeight(nHeight - 1)  
        ) {
            nNewSubsidy = nSubsidy;
        }

        if(ok && nNewSubsidy > 0) { // store it
            auto nEpochHeight = GetDynamicRewardsEpochHeight(nHeight);
            mDynamicRewards[nEpochHeight] = nNewSubsidy; // on the in-memory map

            sqlite3_bind_int(insertStmt, 1, nEpochHeight); // on the file database
            sqlite3_bind_int64(insertStmt, 2, nNewSubsidy);
            auto rc = sqlite3_step(insertStmt);
            if (rc != SQLITE_DONE) {
                oss << __func__ <<  " SQL error: " << sqlite3_errmsg(db) << std::endl;
                ok = false;
            }
            sqlite3_reset(insertStmt);
        }
    }

    if (!oss.str().empty()) LogPrintf("%s: %s", __func__, oss.str());

    return ok;
}

bool CRewards::DisconnectBlock(CBlockIndex* pindex)
{
    auto& consensus = Params().GetConsensus();
    const auto nHeight = pindex->nHeight;
    std::ostringstream oss;
    auto ok = false;
    
    try
    {
        if (consensus.NetworkUpgradeActive(nHeight, Consensus::UPGRADE_DYNAMIC_REWARDS) &&
            IsDynamicRewardsEpochHeight(nHeight)
        ) {
            auto it = mDynamicRewards.find(nHeight);
            if (it != mDynamicRewards.end()) {
                // delete it
                mDynamicRewards.erase(it); // on the in-memory map

                sqlite3_bind_int(deleteStmt, 1, nHeight); // on the file database
                auto rc = sqlite3_step(deleteStmt);
                if (rc != SQLITE_DONE) {
                    oss << __func__ <<  " SQL error: " << sqlite3_errmsg(db) << std::endl;
                    ok = false;
                }
                sqlite3_reset(deleteStmt);
            }
        }
    } 
    catch(const std::exception& e)
    {
        oss << __func__ << " An exception was thrown: " << e.what() << std::endl;
        ok = false;
    }

    if (!oss.str().empty()) LogPrintf("%s: %s", __func__, oss.str());
    
    return ok;
}

CAmount CRewards::GetBlockValue(int nHeight)
{
    auto& consensus = Params().GetConsensus();

    CAmount nSubsidy;

    // ---- Static reward table ----
    if (nHeight > 1) nSubsidy = 800 * COIN;
    else
    if (nHeight > 0) nSubsidy = 600000000 * COIN;
    // ---- Static reward table ----

    if (masternodeSync.IsSynced() &&
        consensus.NetworkUpgradeActive(nHeight, Consensus::UPGRADE_DYNAMIC_REWARDS)
    ) {
        // if this is the block where calculations are made on ConnectBlock
        // return the reward value from the previous block
        if(IsDynamicRewardsEpochHeight(nHeight)) 
            return GetBlockValue(nHeight - 1);

        // find and return the dynamic reward
        const auto nEpochHeight = GetDynamicRewardsEpochHeight(nHeight);
        auto it = mDynamicRewards.find(nEpochHeight);
        if (it != mDynamicRewards.end()) {
            return std::min(nSubsidy, it->second);
        }
    }

    // fallback non-dynamic reward return
    return nSubsidy;
}