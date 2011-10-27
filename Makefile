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
CHROME_PATH=~/Desktop/nacl.app/Contents/MacOS/Chromium
#CHROME_PATH=./Chromium.app/Contents/MacOS/Chromium
CHROME=${CHROME_PATH} --enable-nacl

CC32=${NACLSDK_BIN_DIR}/nacl-gcc
CC64=${NACLSDK_BIN_DIR}/nacl64-gcc
STRIP32=${NACLSDK_BIN_DIR}/nacl-strip
STRIP64=${NACLSDK_BIN_DIR}/nacl64-strip


RODATA_START=0x4800000

CFLAGS=-O2 -Wall -Werror -DRODATA_START=${RODATA_START}
LFLAGS=${CFLAGS} -Wl,--section-start,.rodata=${RODATA_START}
LIBS=-lsrpc -lpthread


CFLAGS_X86_32=-DARCH_X86_32 ${CFLAGS}
CFLAGS_X86_64=-DARCH_X86_64 ${CFLAGS}
LFLAGS_X86_32=${LFLAGS}
LFLAGS_X86_64=${LFLAGS}


all: forth_x86-32.nexe forth_x86-64.nexe naclforth_web

forth_x86-32.nexe: forth_x86-32.o
	${CC32} -o $@ $< ${LFLAGS_X86_32} ${LIBS}
	${STRIP32} $@

forth_x86-64.nexe: forth_x86-64.o
	${CC64} -o $@ $< ${LFLAGS_X86_64} ${LIBS}
	${STRIP64} $@

forth_x86-32.o: forth.c
	${CC32} -o $@ -c $< ${CFLAGS_X86_32}

forth_x86-64.o: forth.c
	${CC64} -o $@ -c $< ${CFLAGS_X86_64}

naclforth_web: web/static/forth_x86-32.nexe web/static/forth_x86-64.nexe web/static/forth.boot web/forth.html

web/static/forth_x86-32.nexe: forth_x86-32.nexe
	cp $< $@

web/static/forth_x86-64.nexe: forth_x86-64.nexe
	cp $< $@

web/static/forth.boot: forth.boot
	cp $< $@

web/forth.html: forth.html
	cp $< $@

deploy: naclforth_web
	appcfg.py update web

sel_ldr: forth_x86-32.nexe
	${SEL_LDR} -f $<

sel_universal: forth_x86-32.nexe
	${SEL_UNIVERSAL} -f $<

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

grab:
	curl 'https://naclforth.appspot.com/read?owner=0&section=0&index=0&count=4' -o forth.boot


start: httpd chrome

stop: httpdstop chromestop

clean:
	rm -f *.o *.nexe \
      web/static/*.nexe \
      web/static/forth.boot \
      web/forth.html
