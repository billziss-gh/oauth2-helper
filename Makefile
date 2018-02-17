# nmake magic; see https://stackoverflow.com/a/30906085/568557 \
!if 1 #\
!include Makefile.win #\
!else

ifeq ($(OS),Windows_NT)
	Platform=$(shell uname -o)
else
	Platform=$(shell uname -s)
endif

ifeq ($(Platform),Cygwin)
CFLAGS=
endif
ifeq ($(Platform),Darwin)
CFLAGS=-framework CoreFoundation -framework CoreServices
endif

oauth2-helper: oauth2-helper.c
	gcc -o $@ $^ $(CFLAGS)
	strip $@

# nmake magic; see https://stackoverflow.com/a/30906085/568557 \
!endif
