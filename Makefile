COMMOTALKIE_REPO=git@github.com:westial/commotalkie.git
LIBRARY_DIR=lib
SRC_DIR=src
TMP_DIR=/tmp
WORKING_DIR=$(shell pwd)

.DEFAULT_GOAL := install

install:
	@echo "> Downloading CommoTalkie"
	cd "$(TMP_DIR)"
	git clone "git@github.com:westial/commotalkie.git" "$(TMP_DIR)/commotalkie"
	@echo "> Preparing files"
	cd "$(TMP_DIR)/commotalkie/arduino" && $(MAKE)
	cd "$(TMP_DIR)/commotalkie/arduino/prebuild"
	mv "$(TMP_DIR)/commotalkie/arduino/prebuild/messageconfig.h" "$(WORKING_DIR)/$(SRC_DIR)"
	mv "$(TMP_DIR)/commotalkie/arduino/prebuild/CommoTalkie" "$(WORKING_DIR)/$(LIBRARY_DIR)"
	@echo "> Finishing CommoTalkie install"
	rm -Rf "$(TMP_DIR)/commotalkie"

clean:
	@echo "> Cleaning CommoTalkie"
	rm -Rf "$(TMP_DIR)/commotalkie"
	rm -Rf "$(WORKING_DIR)/$(SRC_DIR)/messageconfig.h"
	rm -Rf "$(WORKING_DIR)/$(LIBRARY_DIR)/CommoTalkie"
