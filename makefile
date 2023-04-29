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

## Formatter
STYLE_BLUEPRINT=webkit
FORMATTER=clang-format -i -style=$(STYLE_BLUEPRINT)

build: $(SRC)
	$(CC) $(SRC) -o mumble $(DEBUG) 

format: $(SRC)
	$(FORMATTER) $(SRC)

