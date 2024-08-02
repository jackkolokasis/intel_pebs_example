## Examples for Using Intel PEBS

This repo contains some examples of using Intel PEBS.
Intel PEBS is a powerful performance analysis tool that provides
precise sampling data to help developers understand and optimize their
applications. This repository demonstrates a basic example of how to
set up and use PEBS to collect and analyze performance data.

## Requirements
- Linux-based operating system
- Intel processor supporting PEBS
- GCC (GNU Compiler Collection)
- `perf` tool (Linux profiling with performance counters)

## Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/jackkolokasis/intel_pebs_example.git
    cd intel_pebs_example
    ```

2. Ensure you have the required tools installed:
    ```sh
    sudo apt-get update
    sudo apt-get install gcc linux-tools-common linux-tools-generic linux-tools-$(uname -r)
    ```

3. Build the example:
    ```sh
    make
    ```

## Usage

Run the example program:
```sh
make run
```
