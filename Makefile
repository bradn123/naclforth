NACL_SDK_ROOT?=~/clients/naclports/src
NACL_SDK_X86_BIN_DIR=${NACL_SDK_ROOT}/toolchain/linux_x86_newlib/bin
NACL_SDK_ARM_BIN_DIR=${NACL_SDK_ROOT}/toolchain/linux_arm_newlib/bin

CC32=${NACL_SDK_X86_BIN_DIR}/i686-nacl-gcc
CC64=${NACL_SDK_X86_BIN_DIR}/x86_64-nacl-gcc
CCARM=${NACL_SDK_ARM_BIN_DIR}/arm-nacl-gcc
STRIP32=${NACL_SDK_X86_BIN_DIR}/i686-nacl-strip
STRIP64=${NACL_SDK_X86_BIN_DIR}/x86_64-nacl-strip
STRIPARM=${NACL_SDK_ARM_BIN_DIR}/arm-nacl-strip


CFLAGS=-O3 -Wall -Werror -I${NACL_SDK_ROOT}/include
LDFLAGS=$(CFLAGS)
LIBS=-lppapi -lnacl_dyncode -lpthread


CFLAGS_X86_32=-DARCH_X86_32 $(CFLAGS)
CFLAGS_X86_64=-DARCH_X86_64 $(CFLAGS)
CFLAGS_ARM=-DARCH_ARM $(CFLAGS)
LDFLAGS_X86_32=$(LDFLAGS)
LDFLAGS_X86_64=$(LDFLAGS)
LDFLAGS_ARM=$(LDFLAGS)


OUT=out
OBJ=$(OUT)/obj

OUT_APPENGINE=$(OUT)/appengine
OUT_APPENGINE_STATIC=$(OUT_APPENGINE)/static
OUT_APPENGINE_TEMPLATES=$(OUT_APPENGINE)/templates

OUT_APPSTORE=$(OUT)/appstore
OUT_APPSTORE_PACKAGE=$(OUT_APPSTORE)/package

OUT_HOST=$(OUT)/host


NACLFORTH32=$(OUT_APPSTORE_PACKAGE)/naclforth_x86-32.nexe
NACLFORTH64=$(OUT_APPSTORE_PACKAGE)/naclforth_x86-64.nexe
NACLFORTHARM=$(OUT_APPSTORE_PACKAGE)/naclforth_arm.nexe


all: naclforth_appengine naclforth_appstore $(OUT_HOST)/naclforth


$(OUT_HOST)/naclforth: naclforth.c | $(OUT_HOST)
	gcc $< -o $@ -Wall -Werror -g -m32


$(NACLFORTH32): $(OBJ)/naclforth_x86-32.o | $(OUT_APPSTORE_PACKAGE)
	${CC32} -o $@ $< ${LDFLAGS_X86_32} ${LIBS}
	${STRIP32} $@

$(NACLFORTH64): $(OBJ)/naclforth_x86-64.o | $(OUT_APPSTORE_PACKAGE)
	${CC64} -o $@ $< ${LDFLAGS_X86_64} ${LIBS}
	${STRIP64} $@

$(NACLFORTHARM): $(OBJ)/naclforth_arm.o | $(OUT_APPSTORE_PACKAGE)
	${CCARM} -o $@ $< ${LDFLAGS_ARM} ${LIBS}
	${STRIPARM} $@

$(OBJ)/naclforth_x86-32.o: naclforth.c | $(OBJ)
	${CC32} -o $@ -c $< ${CFLAGS_X86_32}

$(OBJ)/naclforth_x86-64.o: naclforth.c | $(OBJ)
	${CC64} -o $@ -c $< ${CFLAGS_X86_64}

$(OBJ)/naclforth_arm.o: naclforth.c | $(OBJ)
	${CCARM} -o $@ -c $< ${CFLAGS_ARM}

$(OUT_APPENGINE)/%: web/%
	mkdir -p $(@D)
	cp $< $@


naclforth_appengine: \
    $(OUT_APPENGINE_STATIC)/favicon.ico \
    $(OUT_APPENGINE_STATIC)/favicon.png \
    $(OUT_APPENGINE_STATIC)/googleffb2e0a7be075da2.html \
    $(OUT_APPENGINE_TEMPLATES)/getapp.html \
    $(OUT_APPENGINE_TEMPLATES)/close.html \
    $(OUT_APPENGINE_TEMPLATES)/status.html \
    $(OUT_APPENGINE)/naclforth.py \
    $(OUT_APPENGINE)/app.yaml \
    $(OUT_APPENGINE)/index.yaml


$(OUT_APPENGINE) $(OUT_APPENGINE_STATIC) $(OUT_APPENGINE_TEMPLATES) \
   $(OUT_APPSTORE) $(OUT_APPSTORE_PACKAGE) \
   $(OUT) $(OUT_HOST) $(OBJ):
	mkdir -p $@

naclforth_appstore: $(OUT_APPSTORE)/naclforth.zip

APPSTORE_PACKAGE_FILES = \
    $(NACLFORTH32) \
    $(NACLFORTH64) \
    $(NACLFORTHARM) \
    $(OUT_APPSTORE_PACKAGE)/manifest.json \
    $(OUT_APPSTORE_PACKAGE)/naclforth_16.png \
    $(OUT_APPSTORE_PACKAGE)/naclforth_128.png \
    $(OUT_APPSTORE_PACKAGE)/naclforth.nmf \
    $(OUT_APPSTORE_PACKAGE)/naclforth.js \
    $(OUT_APPSTORE_PACKAGE)/naclforth.html

$(OUT_APPSTORE)/naclforth.zip: $(APPSTORE_PACKAGE_FILES)
	-rm -f $@
	cd $(OUT_APPSTORE_PACKAGE) && zip -r ../naclforth.zip .

$(OUT_APPSTORE_PACKAGE)/%: appstore/%
	mkdir -p $(@D)
	cp $< $@

deploy: naclforth_appengine
	appcfg.py update $(OUT_APPENGINE)

local: naclforth_appengine
	dev_appserver.py -d $(OUT_APPENGINE)

getboot:
	curl 'https://naclforth.appspot.com/_read?owner=0&filename=%2fpublic%2f_boot' -o - | build/reduce80.py >boot.fs

clean:
	rm -rf $(OUT) `find ./ -name "*~"`

.PHONY: clean naclforth_appengine naclforth_appstore all deploy local getboot
