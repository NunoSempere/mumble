# make
# make build
# (sudo) make install
# make format
# make clean
# make uninstall

## C compiler
CC=tcc # much faster compilation than gcc

## Main file
SRC=lisp.c

## Formatter
STYLE_BLUEPRINT=webkit
FORMATTER=clang-format -i -style=$(STYLE_BLUEPRINT)

build: $(SRC)
	$(cc) $(src) -o lisp

format: $(SRC)
	$(FORMATTER) $(SRC)

