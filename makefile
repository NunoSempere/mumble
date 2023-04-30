# make
# make build
# (sudo) make install
# make format
# make clean
# make uninstall

## C compiler
CC=tcc # much faster compilation than gcc

## Debugging options
DEBUG=-g#-g

## Main file
SRC=./src/mumble.c
MPC=./src/mpc/mpc.c

## Dependencies
DEPS_PC=libedit
# ^ libm, which doesn't have files which pkg-config can find, grr.
DEBUG= #'-g'

INCS=`pkg-config --cflags ${DEPS_PC}`
LIBS_PC=`pkg-config --libs ${DEPS_PC}`
LIBS_DIRECT=-lm
LIBS=$(LIBS_DIRECT) $(LIBS_PC)
# $(CC) $(DEBUG) $(INCS) $(PLUGS) $(SRC) -o rose $(LIBS) $(ADBLOCK)

## Formatter
STYLE_BLUEPRINT=webkit
FORMATTER=clang-format -i -style=$(STYLE_BLUEPRINT)

build: $(SRC)
	$(CC)  -Wall  $(INCS) $(SRC) $(MPC) -o mumble $(LIBS)

format: $(SRC)
	$(FORMATTER) $(SRC)

