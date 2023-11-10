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
                        export DOCKER_IMAGE_HASH="dca176c9663a7ba4c1f0e710986f5a25e672842963d95b960191e2d9f7185ebe"
                        bin/make-base-vm --suite bionic --arch amd64 --docker --docker-image-digest $DOCKER_IMAGE_HASH
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
                    if [ -d "${workspace_dir}/${BINARIES_PATH}/${BRANCH}/windows" ]; then
                        rm -r ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/windows
                    fi
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/windows
                    cd gitian-builder
                    mv build/out/${BASE_NAME}* build/out/${ZIP_NAME}* ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/windows
                    cp result/dsw-win* ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/windows
                '''
            }
        }



    }
}
