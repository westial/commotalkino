COMMOTALKIE_VERSION=0.1.1
COMMOTALKIE_REPO=git@github.com:westial/commotalkie.git
LIBRARY_DIR=lib
SRC_DIR=src
TMP_DIR=/tmp
WORKING_DIR=$(shell pwd)

.DEFAULT_GOAL := deploy

deploy:
	@echo "> Downloading CommoTalkie"
	cd "$(TMP_DIR)"
	git clone --depth 1 -b "$(COMMOTALKIE_VERSION)" "git@github.com:westial/commotalkie.git" "$(TMP_DIR)/commotalkie"
	@echo "> Preparing files"
	cd "$(TMP_DIR)/commotalkie/arduino" && $(MAKE)
	cd "$(TMP_DIR)/commotalkie/arduino/prebuild"
	mv "$(TMP_DIR)/commotalkie/arduino/prebuild/CommoTalkie" "$(WORKING_DIR)/$(LIBRARY_DIR)"
	@echo "> Finishing CommoTalkie install"
	rm -Rf "$(TMP_DIR)/commotalkie"

clean:
	@echo "> Cleaning CommoTalkie"
	rm -Rf "$(TMP_DIR)/commotalkie"
	rm -Rf "$(WORKING_DIR)/$(LIBRARY_DIR)/CommoTalkie"

update:
	$(MAKE) clean
	$(MAKE) deploy
