pipeline {

    agent any

    environment {
        NAME = '__Decenomy__'
        BASE_NAME = '__decenomy__'
        ZIP_NAME = '__DSW__'
        BINARIES_PATH = 'dsw-binaries'
        GIT_REPO = 'https://github.com/zimbora/dsw.git'
        BRANCH = 'develop'
        USE_DOCKER = '1'
        PROJECT = "dsw"
    }

    stages {

        stage("gitian") {

            steps {
                echo 'preparing gitian ...'
                    sh '''#!/bin/bash
                        repo_dir=$(pwd)
                        echo ${env.WORKSPACE}
                        cd ..
                        mkdir -p ${BINARIES_PATH}/${BRANCH}
                        if [ -d "gitian-builder" ]; then
                            cd gitian-builder
                            git pull origin master
                            cd ..
                        else
                            git clone https://github.com/devrandom/gitian-builder.git
                        fi
                        cd gitian-builder
                        mkdir -p inputs
                        cd inputs
                        if [ -d "dsw" ]; then
                            cd dsw
                            git pull origin develop
                            cd ..
                        else
                            git clone ${GIT_REPO}
                        fi
                        if [ ! -f "MacOSX10.11.sdk.tar.xz" ]; then
                            wget -c https://github.com/decenomy/depends/raw/main/SDKs/MacOSX10.11.sdk.tar.xz
                        fi
                        cd ..
                        bin/make-base-vm --suite bionic --arch amd64 --docker --docker-image-digest 152dc042452c496007f07ca9127571cb9c29697f42acbfad72324b2bb2e43c98
                        git checkout bin/make-base-vm
                    '''

            }
        }

        stage("depends") {

            steps {
                echo 'building depends ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    make -C ${repo_dir}/depends download SOURCES_PATH=${workspace_dir}/gitian-builder/cache/common
                '''
            }
        }
        /*
        stage("build_x86_64-pc-linux-gnu") {

            steps {
                echo 'building linux64 ...'
                sh '''#!/bin/bash
                    cd ..
                    workspace_dir=$(pwd)
                    cd gitian-builder
                    ./bin/gbuild -j 2 -m 6000 --commit ${PROJECT}=${BRANCH} --url ${PROJECT}=${GIT_REPO} ${workspace_dir}/gitian-builder/inputs/${PROJECT}/contrib/gitian-descriptors/gitian-linux64.yml
                '''
            }
        }

        stage("deploy_x86_64-pc-linux-gnu") {

            steps {
                echo 'deploy arm64 ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/linux64
                    cd gitian-builder
                    mv build/out/${BASE_NAME}*-linux64.tar.gz ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/linux64
                '''
            }
        }

        stage("build_arch64-linux-gnu") {

            steps {
                echo 'building arm64 ...'
                sh '''#!/bin/bash
                    cd ..
                    workspace_dir=$(pwd)
                    cd gitian-builder
                    ./bin/gbuild -j 2 -m 6000 --commit ${PROJECT}=${BRANCH} --url ${PROJECT}=${GIT_REPO} ${workspace_dir}/gitian-builder/inputs/${PROJECT}/contrib/gitian-descriptors/gitian-arm64.yml
                '''
            }
        }

        stage("deploy_arch64-linux-gnu") {

            steps {
                echo 'deploy linux ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/arm64
                    cd gitian-builder
                    mv build/out/${BASE_NAME}*-arm64.tar.gz ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/arm64
                '''
            }
        }
        */
        stage("build_x86_64-w64-mingw32") {

            steps {
                echo 'building windows ...'
                sh '''#!/bin/bash
                    cd ..
                    workspace_dir=$(pwd)
                    cd gitian-builder
                    ./bin/gbuild -j 2 -m 6000 --commit ${PROJECT}=${BRANCH} --url ${PROJECT}=${GIT_REPO} ${workspace_dir}/gitian-builder/inputs/${PROJECT}/contrib/gitian-descriptors/gitian-win.yml
                '''
            }
        }

        stage("deploy_x86_64-w64-mingw32") {

            steps {
                echo 'deploy windows ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/windows
                    cd gitian-builder
                    mv build/out/${BASE_NAME}* build/out/${ZIP_NAME}* ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/windows
                '''
            }
        }

        stage("build_x86_64-apple-darwin14") {

            steps {
                echo 'building macos ...'
                sh '''#!/bin/bash
                    cd ..
                    workspace_dir=$(pwd)
                    cd gitian-builder
                    ./bin/gbuild -j 2 -m 6000 --commit ${PROJECT}=${BRANCH} --url ${PROJECT}=${GIT_REPO} ${workspace_dir}/gitian-builder/inputs/${PROJECT}/contrib/gitian-descriptors/gitian-osx.yml
                '''
            }
        }

        stage("deploy_x86_64-apple-darwin14") {

            steps {
                echo 'deploy windows ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/macosx
                    cd gitian-builder
                    mv build/out/${BASE_NAME}* build/out/${ZIP_NAME}* ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/macosx
                '''
            }
        }



    }
}
