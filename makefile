# make
# make build
# (sudo) make install
# make format
# make clean
# make uninstall

## C compiler
CC=gcc # much faster compilation than gcc
COMPILER_FLAGS=#-Wall -Wextra -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined # -g3 
# exclude: -fsanitize-trap, because I'm using an old version of gcc and couldn't bother getting a new one.
## ^ from <https://nullprogram.com/blog/2023/04/29/>
## <https://news.ycombinator.com/item?id=35758898>

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
	$(CC) $(COMPILER_FLAGS)  $(INCS) $(SRC) $(MPC) -o mumble $(LIBS) $(DEBUG)

format: $(SRC)
	$(FORMATTER) $(SRC)

debug: 
	gcc -I/usr/include/editline ./src/mumble.c ./src/mpc/mpc.c -o mumble -lm -ledit -g
	# valgrind --tool=memcheck --leak-check=yes  --show-leak-kinds=all ./mumble
	valgrind --tool=memcheck --leak-check=yes ./mumble
