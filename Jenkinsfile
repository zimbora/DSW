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
                        git clone https://github.com/devrandom/gitian-builder.git
                        cd gitian-builder
                        mkdir inputs
                        cd inputs
                        git clone ${GIT_REPO}
                        wget -c https://github.com/decenomy/depends/raw/main/SDKs/MacOSX10.11.sdk.tar.xz
                        cd ..
                        bin/make-base-vm --suite bionic --arch amd64 --docker
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

        stage("build_x86_64-pc-linux-gnu") {

            steps {
                echo 'building linux ...'
                sh '''#!/bin/bash
                    cd ..
                    workspace_dir=$(pwd)
                    cd gitian-builder
                    ./bin/gbuild -j 2 -m 6000 --commit dsw=develop --url dsw=https://github.com/zimbora/dsw ${workspace_dir}/gitian-builder/inputs/dsw/contrib/gitian-descriptors/gitian-linux2.yml
                '''
            }
        }

        stage("deploy_x86_64-pc-linux-gnu") {

            steps {
                echo 'deploy linux ...'
                sh '''#!/bin/bash
                    repo_dir=$(pwd)
                    cd ..
                    workspace_dir=$(pwd)
                    mkdir -p ${BINARIES_PATH}/${BRANCH}/linux
                    cd gitian-builder
                    mv build/out/__decenomy__*-linux64.tar.gz ${workspace_dir}/${BINARIES_PATH}/${BRANCH}/linux
                '''
            }
        }

    }
}
