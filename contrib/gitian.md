# GITIAN

## Building with gitian on local machine

	export NAME=zimbora
	export BRANCH=develop
	export cores=2
	export ram=4000

	curl -L -O https://raw.githubusercontent.com/${NAME}/dsw/${BRANCH}/contrib/gitian-build.py

	chmod +x gitian-build.py
	./gitian-build.py --docker --setup

	## build all
	./gitian-build.py --docker -b -c --detach-sign $NAME $BRANCH -j $cores -m $ram

	## build for linux
	./gitian-build.py --docker -b -c -o l --detach-sign $NAME $BRANCH -j $cores -m $ram

	## build for windows
	./gitian-build.py --docker -b -c -o w --detach-sign $NAME $BRANCH -j $cores -m $ram

	## build for macos
	./gitian-build.py --docker -b -c -o m --detach-sign $NAME $BRANCH -j $cores -m $ram

###

	Validate your compilation

	cd gitian.sigs
	success=python verify_sigs.py
	if [ $success ]: then; echo "your files are valid" else echo "your build cannot be verified" fi

## Building with GITHUB only for validators

	Do a repository fork
	Copy github workflow gitian-builder
	Asks developer for a PAT_TOKEN
	Define it in your secrets
	PAT_TOKEN=$secret
	Run workflow

	In the end a new pull request will be made in your name for gitian.sigs repository.
	This will help us to validate our work.
	Thank you for contribution
