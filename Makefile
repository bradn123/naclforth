NACL_SDK_ROOT?=~/clients/naclports/src
NACL_SDK_BIN_DIR=${NACL_SDK_ROOT}/toolchain/mac_x86_newlib/bin

CC32=${NACL_SDK_BIN_DIR}/i686-nacl-gcc
CC64=${NACL_SDK_BIN_DIR}/x86_64-nacl-gcc
STRIP32=${NACL_SDK_BIN_DIR}/i686-nacl-strip
STRIP64=${NACL_SDK_BIN_DIR}/x86_64-nacl-strip


RODATA_START=0x4800000


CFLAGS=-O2 -Wall -Werror -DRODATA_START=${RODATA_START}
LFLAGS=${CFLAGS} -Wl,--section-start,.rodata=${RODATA_START}
LIBS=-lppapi -lnacl_dyncode -lpthread


CFLAGS_X86_32=-DARCH_X86_32 ${CFLAGS}
CFLAGS_X86_64=-DARCH_X86_64 ${CFLAGS}
LFLAGS_X86_32=${LFLAGS}
LFLAGS_X86_64=${LFLAGS}


OUT=out
DEPLOY=$(OUT)/deploy
DEPLOY_STATIC=$(DEPLOY)/static
OBJ=$(OUT)/obj


NACLFORTH32=$(DEPLOY_STATIC)/naclforth_x86-32.nexe
NACLFORTH64=$(DEPLOY_STATIC)/naclforth_x86-64.nexe


all: naclforth_web $(OUT)/naclforth


$(OUT)/naclforth: naclforth.c
	gcc $< -o $@ -Wall -Werror -g -m32


$(NACLFORTH32): $(OBJ)/naclforth_x86-32.o $(DEPLOY_STATIC)
	${CC32} -o $@ $< ${LFLAGS_X86_32} ${LIBS}
	${STRIP32} $@

$(NACLFORTH64): $(OBJ)/naclforth_x86-64.o $(DEPLOY_STATIC)
	${CC64} -o $@ $< ${LFLAGS_X86_64} ${LIBS}
	${STRIP64} $@

$(OBJ)/naclforth_x86-32.o: naclforth.c $(OBJ)
	${CC32} -o $@ -c $< ${CFLAGS_X86_32}

$(OBJ)/naclforth_x86-64.o: naclforth.c $(OBJ)
	${CC64} -o $@ -c $< ${CFLAGS_X86_64}

$(DEPLOY)/%: web/%
	mkdir -p $(@D)
	cp $< $@

naclforth_web: $(NACLFORTH32) \
               $(NACLFORTH64) \
               $(DEPLOY_STATIC)/nacl4th.ico \
               $(DEPLOY_STATIC)/nacl4th.png \
               $(DEPLOY)/naclforth.py \
               $(DEPLOY)/app.yaml \
               $(DEPLOY)/index.yaml \
               $(DEPLOY)/naclforth.html

$(DEPLOY)/naclforth.html: naclforth.html
	cp $< $@

$(OUT) $(DEPLOY) $(OBJ) $(DEPLOY_STATIC):
	mkdir -p $@

deploy: naclforth_web
	appcfg.py update out/deploy

local: naclforth_web
	dev_appserver.py out/deploy

clean:
	rm -rf $(OUT) `find ./ -name "*~"`