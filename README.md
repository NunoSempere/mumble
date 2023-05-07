# Mumble: A lisp in C

## About

This is a Lisp written in C. It follows the outline in this [Build Your Own Lisp](https://buildyourownlisp.com/chapter11_variables) book, though it then adds some small tweaks and improvements and quality of life improvements:

- A makefile
- Configurable verbosity levels
- Different and perhaps slightly more elegant printing functions
- A slightly different approach to evaluating functions
- Capturing Ctrl+D
- Float instead of ints

Conversely, it doesn't have:
- Function currying
- strings
- Variable arguments
- ...

Overall this might be mostly of interest as a pointer to the book that this is originally based on. And to readers of that same book, I'd be curious to see what you ended up with.

## Installation and usage

### Dependencies

This depends on [editline](https://github.com/troglobit/editline), which can be installed on Debian/Ubuntu with:

```
git clone https://github.com/troglobit/editline
./autogen.sh
./configure
make all
sudo make install
ldconfig
```

Readers might also find it interesting to compile it with [tiny c compiler](https://bellard.org/tcc/) rather than gcc, as it is significantly faster.

### Compilation

```
git clone https://github.com/NunoSempere/mumble
make
# sudo make install # 
```

### Usage

Simply call the `./mumble` binary:

```
./mumble
```

## Example usage

```
mumble> (1 2 3)
mumble> { 1 2 3 }
mumble> head (1 2 3)
mumble> { head (1 2 3) }
mumble> tail { 1 2 3 }
mumble> list ( 1 2 3 )
mumble> eval { head {1 2 3} } 
mumble> (eval { head {+ tail head } } ) 1 2 3 
mumble> len {1 2 3}
mumble> join { {1 2} {3 4} }
mumble> def { {x} { 100 } }
mumble> x
mumble> def { { a b c } { 1 2 3} }
mumble> * a b c
mumble> - a b c
mumble> / a b c
mumble> VERBOSITY=0
mumble> VERBOSITY=1
mumble> VERBOSITY=2
```


## Gotchas

This doesn't currently run on Windows. But it easily could, with [preprocessor statements from the book].

## Usage and licensing

I don't expect this project to be such that people might want to use it. If you want a

But for the eventuality, this code is licensed under the MIT license; see the license.txt file.
