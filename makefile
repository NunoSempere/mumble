# make
# make build
# (sudo) make install
# make format
# make clean
# make uninstall

## C compiler
CC=tcc # much faster compilation than gcc

## Main file
SRC=./src/lisp.c

## Formatter
STYLE_BLUEPRINT=webkit
FORMATTER=clang-format -i -style=$(STYLE_BLUEPRINT)

build: $(SRC)
	$(CC) $(SRC) -o lisp

format: $(SRC)
	$(FORMATTER) $(SRC)

