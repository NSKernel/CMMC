#
#   C-- Compiler Front End
#   Copyright (C) 2019 NSKernel. All rights reserved.
#
#   A lab of Compilers at Nanjing University
#
#   Makefile
#

CC = gcc
FLEX = flex
BISON = bison

CCFLAGS = -std=c99 -I./
LDFLAGS = -ly -lc

CFILES = $(shell find ./ -name "*.c")
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")

LCFILE = $(addprefix gen/, $(addsuffix .yy.c,  $(notdir $(basename $(LFILE)))))
YCFILE = $(addprefix gen/, $(addsuffix .tab.c,  $(notdir $(basename $(YFILE))))) 
YHFILE = $(addprefix gen/, $(addsuffix .tab.h,  $(notdir $(basename $(YFILE)))))

OBJS = $(addprefix build/, $(addsuffix .o,  $(notdir $(basename $(CFILES)))))
YCGENOBJS = $(addprefix build/, $(addsuffix .yy.o,  $(notdir $(basename $(LFILE)))))
LCGENOBJS = $(addprefix build/, $(addsuffix .tab.o,  $(notdir $(basename $(YFILE)))))

UNAME = $(shell uname)
ifeq ($(UNAME), Darwin)
	LDFLAGS += -ll -lSystem -macosx_version_min 10.14
endif
ifeq ($(UNAME), Linux)
	LDFLAGS += -lfl
endif

build/parser: $(OBJS) $(YCGENOBJS) $(LCGENOBJS)
	@echo "  LD    " $(OBJS)
	@$(LD) -o $@ $(OBJS) $(LDFLAGS)
	@echo "  build/parser is ready"

build/%.o: %.c
	@mkdir -p build
	@echo "  CC    " $<
	@$(CC) -o $@ $< $(CCFLAGS) -c

build/%.yy.o: gen/%.yy.c forcegen
	@mkdir -p build
	@echo "  CC    " $<
	@$(CC) -o $@ $< $(CCFLAGS) -c

build/%.tab.o: gen/%.tab.c forcegen
	@mkdir -p build
	@echo "  CC    " $<
	@$(CC) -o $@ $< $(CCFLAGS) -c

forcegen: $(YCFILE) $(LCFILE)

gen/%.tab.c: %.y
	@mkdir -p gen
	@echo "  BISON " $<
	@$(BISON) -o $@ -d -v $<

gen/%.yy.c: %.l
	@mkdir -p gen
	@echo "  FLEX  " $<
	@$(FLEX) -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	@echo "  REMOVED  build"
	@rm -rf build
	@echo "  REMOVED  gen"
	@rm -rf gen
