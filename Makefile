#
#   C-- Compiler Front End
#   Copyright (C) 2019 NSKernel. All rights reserved.
#
#   A lab of Compilers at Nanjing University
#
#   Makefile
#

include config

CC = gcc
LD = ld
FLEX = flex
YACC = bison

CCFLAGS = -I./
LDFLAGS = -lc

CFILES = $(shell find ./ -name "*.c")
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")

LCFILE = $(addprefix gen/, $(addsuffix .yy.c,  $(notdir $(basename $(LFILE)))))
YCFILE = $(addprefix gen/, $(addsuffix .tab.c,  $(notdir $(basename $(YFILE))))) 
YHFILE = $(addprefix gen/, $(addsuffix .tab.h,  $(notdir $(basename $(YFILE)))))

OBJS = $(addprefix build/, $(addsuffix .o,  $(notdir $(basename $(CFILES)))))
YCGENOBJS = $(addprefix build/, $(addsuffix .yy.o,  $(notdir $(basename $(LFILE)))))
LCGENOBJS = $(addprefix build/, $(addsuffix .tab.o,  $(notdir $(basename $(YFILE)))))

VERSIONSTR = "\"$(VERSION)\""
SUBVERSIONSTR = "\"$(SUBVERSION)\""
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -ggdb3 -O0
	BUILDSTR = "\"$(BUILD)-debug\""
	LDFLAGS += -DDEBUG -ggdb3 -O0
else
	CFLAGS += -O2
	BUILDSTR = "\"$(BUILD)\""
	LDFLAGS += -O0
endif

UNAME = $(shell uname)
ifeq ($(UNAME), Darwin)
	CC = clang
	LDFLAGS += -ll -mmacosx-version-min=$(MACOSX_MIN_VER)
	CCFLAGS += -mmacosx-version-min=$(MACOSX_MIN_VER)
endif
ifeq ($(UNAME), Linux)
	LDFLAGS += -lfl
	CCFLAGS += -lfl
endif

build/cmmc: $(OBJS) $(YCGENOBJS) $(LCGENOBJS)
	@echo "  LD      " build/cmmc
	@$(CC) $(LDFLAGS) -o $@ $(OBJS)
	@echo $@" is ready"

build/%.o: %.c
	@mkdir -p build
	@echo "  CC      " $@
	@$(CC) -o $@ $< $(CCFLAGS) -c -D VERSION=$(VERSIONSTR) -D SUBVERSION=$(SUBVERSIONSTR) -D BUILD=$(BUILDSTR)

build/%.yy.o: gen/%.yy.c forcegen
	@mkdir -p build
	@echo "  CC [G]  " $@
	@$(CC) -o $@ $< $(CCFLAGS) -c

build/%.tab.o: gen/%.tab.c forcegen
	@mkdir -p build
	@echo "  CC [G]  " $@
	@$(CC) -o $@ $< $(CCFLAGS) -c

forcegen: $(YCFILE) $(LCFILE)

gen/%.tab.c: %.y
	@mkdir -p gen
	@echo "  YACC    " $@
	@$(YACC) -o $@ -d -v $<

gen/%.yy.c: %.l
	@mkdir -p gen
	@echo "  FLEX    " $@
	@$(FLEX) -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	@echo "  REMOVED  build"
	@rm -rf build
	@echo "  REMOVED  gen"
	@rm -rf gen
