# 1st-simulator

## Build

	$ make

## Test

	$ make test/fib

## Usage

	$ ./sim ganbaru.zoi in.bin [options]

### Options

- `-d`  
Debug mode

- `-show-last`  
Show last CPU status

- `-show-max`  
Show register max values

- `-show-ulines`  
Show unreached lines

- `-show-ulabels`  
Show unreached labels

- `-silent`
- `-verbose`

### Commands in debug mode

- `next [count]`
- `continue`
- `print [arg]`
- `quit`
