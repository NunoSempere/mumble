# Mumble: A lisp in C

## About

This is a Lisp written in C. It follows the outline in this [Build Your Own Lisp](https://buildyourownlisp.com) book, though it then adds some small tweaks and improvements and quality of life improvements:

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
mumble> head {1 2 3}
mumble> { head {1 2 3) }
mumble> tail { 1 2 3 }
mumble> list ( 1 2 3 )
mumble> eval { head {1 2 3} } 
mumble> (eval { head {+ tail head } } ) 1 2 3 
mumble> len {1 2 3}
mumble> join { {1 2} {3 4} }
mumble> def {x} { 100 }
mumble> def {y} 100
mumble> (x y)
mumble> VERBOSITY=2
mumble> def {sq} (@ {x} {* x x})
mumble> sq 44
mumble> VERBOSITY=1
mumble> def {sqsum} (@ {x y} {(+ (sq x) (sq y))})
mumble> sqsum 2 3
mumble> VERBOSITY=0
mumble> def {init} (@ {xs} { list((head xs)) } )
mumble> def {kopf} (@ {xx} { head (list xx) } )
mumble> init {1 2}
mumble> kopf (1 2)
mumble> ifelse 1 2 3
mumble> if 1 2 3
mumble> ifelse 0 1 2 
mumble> if 0 1 2 
mumble> if {1 2 3} (1) (1)
mumble> def {positive} (@ {x} {> x 0})
mumble> def {fibtest} (@ {x} {if (> x 0) {+ x 1} 0 })
mumble> fibtest 2
mumble> def {fibonacci} (@ {x} {if (> x 1) { + (fibonacci (- x 2)) ( fibonacci ( - x 1 ) ) } 1} )
mumble> fibonacci 4
mumble> def {!} (@ {x} { if ( > 1 x) 1 { * x (! (- x 1)) } })
mumble> ! 100
mumble> def {++} (@ {x} { if ( > 1 x) 0 { + x (++ (- x 1)) } })
mumble> ++ 10
mumble>  def {++2} (@ {x} { / (* x (+ x 1)) 2 })
mumble> ++2 10

```

## To do

- [x] Define functions
- [x] Define if, = and >
- [x] Build fibonacci function

## Gotchas

This doesn't currently run on Windows. But it easily could, with the preprocessor staments from parsing.c [here](https://buildyourownlisp.com/chapter6_parsing).
