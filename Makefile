NACL_SDK_ROOT?=~/clients/naclports/src
NACL_SDK_BIN_DIR=${NACL_SDK_ROOT}/toolchain/mac_x86_newlib/bin

CC32=${NACL_SDK_BIN_DIR}/i686-nacl-gcc
CC64=${NACL_SDK_BIN_DIR}/x86_64-nacl-gcc
STRIP32=${NACL_SDK_BIN_DIR}/i686-nacl-strip
STRIP64=${NACL_SDK_BIN_DIR}/x86_64-nacl-strip


CFLAGS=-O3 -Wall -Werror
LDFLAGS=$(CFLAGS) -nostdlib -nodefaultlibs
LIBS=-lppapi -lnacl_dyncode -lpthread


CFLAGS_X86_32=-DARCH_X86_32 $(CFLAGS)
CFLAGS_X86_64=-DARCH_X86_64 $(CFLAGS)
LDFLAGS_X86_32=$(LDFLAGS)
LDFLAGS_X86_64=$(LDFLAGS)


OUT=out
DEPLOY=$(OUT)/deploy_appengine
DEPLOY_STATIC=$(DEPLOY)/static
DEPLOY_TEMPLATES=$(DEPLOY)/templates
OUT_HOSTED=$(OUT)/deploy_hosted
OBJ=$(OUT)/obj


NACLFORTH32=$(DEPLOY_STATIC)/naclforth_x86-32.nexe
NACLFORTH64=$(DEPLOY_STATIC)/naclforth_x86-64.nexe


all: naclforth_web naclforth_hosted $(OUT)/naclforth


$(OUT)/naclforth: naclforth.c
	gcc $< -o $@ -Wall -Werror -g -m32


$(NACLFORTH32): $(OBJ)/naclforth_x86-32.o | $(DEPLOY_STATIC)
	${CC32} -o $@ $< ${LDFLAGS_X86_32} ${LIBS}
	${STRIP32} $@

$(NACLFORTH64): $(OBJ)/naclforth_x86-64.o | $(DEPLOY_STATIC)
	${CC64} -o $@ $< ${LDFLAGS_X86_64} ${LIBS}
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
               $(DEPLOY_STATIC)/favicon.ico \
               $(DEPLOY_STATIC)/favicon.png \
               $(DEPLOY_STATIC)/naclforth.nmf \
               $(DEPLOY)/naclforth.py \
               $(DEPLOY)/app.yaml \
               $(DEPLOY)/index.yaml \
               $(DEPLOY_TEMPLATES)/getchrome.html \
               $(DEPLOY_TEMPLATES)/getapp.html \
               $(DEPLOY_TEMPLATES)/naclforth.html

$(DEPLOY_TEMPLATES)/%.html: web/templates/%.html
	cp $< $@

$(OUT) $(DEPLOY) $(OBJ) $(DEPLOY_STATIC) $(OUT_HOSTED):
	mkdir -p $@

naclforth_hosted: $(OUT_HOSTED)/naclforth.zip

$(OUT_HOSTED)/naclforth.zip: $(OUT_HOSTED)
	-rm -f hosted_app/*~
	-rm -f $@
	zip -r $@ hosted_app

deploy: naclforth_web
	appcfg.py update $(DEPLOY)

local: naclforth_web
	dev_appserver.py -d $(DEPLOY)

clean:
	rm -rf $(OUT) `find ./ -name "*~"`