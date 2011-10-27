NACL_DIR?=~/clients/nacl1/native_client
NACL_OUT_DIR?=${NACL_DIR}/scons-out/opt-mac-x86-32
NACL_STAGING_DIR=${NACL_OUT_DIR}/staging

NACLSDK_DIR?=${NACL_DIR}/toolchain/mac_x86
#NACLSDK_DIR?=~/native_client_sdk_0_1_507_1/toolchain/mac_x86
#NACLSDK_DIR?=~/native_client_sdk_0_1_603_0/toolchain/mac_x86
NACLSDK_BIN_DIR=${NACLSDK_DIR}/bin

SEL_LDR=${NACL_STAGING_DIR}/sel_ldr
SEL_UNIVERSAL=${NACL_STAGING_DIR}/sel_universal
HTTPD=./httpd.py

#CHROME_PATH=/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome
#CHROME_PATH=./Google\ Chrome.app/Contents/MacOS/Google\ Chrome
CHROME_PATH=./Chromium.app/Contents/MacOS/Chromium
CHROME=${CHROME_PATH} --enable-nacl

CC=${NACLSDK_BIN_DIR}/nacl-gcc
STRIP=${NACLSDK_BIN_DIR}/nacl-strip


RODATA_START=0x4000000

CFLAGS=-O2 -Wall -Werror -DRODATA_START=${RODATA_START}
LFLAGS=${CFLAGS} -Wl,--section-start,.rodata=${RODATA_START}
LIBS=-lsrpc -lpthread


all: forth.nexe naclforth_web

forth.nexe: forth.o
	${CC} -o $@ $< ${LFLAGS} ${LIBS}
	${STRIP} $@

forth.o: forth.c
	${CC} -o $@ -c $< ${CFLAGS}

naclforth_web: web/static/forth.nexe web/static/forth.boot web/forth.html

web/static/forth.nexe: forth.nexe
	cp $< $@

web/static/forth.boot: forth.boot
	cp $< $@

web/forth.html: forth.html
	cp $< $@

deploy: naclforth_web
	appcfg.py update web

sel_ldr: forth.nexe
	${SEL_LDR} -f forth.nexe

sel_universal: forth.nexe
	${SEL_UNIVERSAL} -f forth.nexe

chrome:
	bash -c '${CHROME} --incognito http://localhost:5103/forth.html & echo $$! >chrome.pid'

chromeweb:
	bash -c '${CHROME} --incognito http://naclforth.appspot.com/ & echo $$! >chrome.pid'

chromestop:
	kill `cat chrome.pid` ; rm chrome.pid || true

httpd: httpdstop
	bash -c '${HTTPD} & echo $$! >httpd.pid'

httpdstop:
	kill `cat httpd.pid` ; rm httpd.pid || true

start: httpd chrome

stop: httpdstop chromestop

clean:
	rm -f forth.o forth.nexe \
      web/static/forth.nexe \
      web/static/forth.boot \
      web/forth.html
