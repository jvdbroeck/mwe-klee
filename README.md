## Minimal Working Example (MWE) to evaluate flexible protections in KLEE

This repository contains an MWE to assess the strength of flexible opaque predicates in KLEE. The MWE uses code from the Python project (https://www.python.org/ftp/python/3.7.4/Python-3.7.4.tar.xz). Particularly, Python's internal hash table implementation was extracted and an opaque predicate is encoded thereon.

### Contents of this repository
* `Makefile`: GNU Makefile to build and run the example. Available targets: `build`, `klee` and `clean`.
* `hashtable.c` and `hashtable.h`: Minimally adapted code of Python's internal hash table implementation.
* `predicate.c` and `predicate.h`: Minimal implementation for a flexible opaque predicate using Python's hash table implementation. The predicate is `TRUE` when an entry with a specific key (`g_key` in `predicate.c`) is present in a hash table instance (`g_instance` in `predicate.c`).
* `main.c`: The main program, which instantiates a predicate, manipulates its value and prints some output, which depends on the concrete value of the predicate.
* `fabs.patch`: Cherry-picked commit from KLEE's master branch to support the LLVM fabs.f64 intrinsic.

### Running the example
To be run in a KLEE docker container, tested on the Docker container artefact from https://srg.doc.ic.ac.uk/projects/klee-segmem/artifact.html. The source code of KLEE was patched to support the llvm.fabs intrinsic (`fabs.patch`).

1. To build the application and produce a bitcode (.bc) file: `make build`
2. To run the application natively: `./program`
3. To execute KLEE on the application: `make klee`

Goal: Use KLEE to deduce valid states of the hash table instance that cause the predicate to evaluate to `TRUE` or `FALSE`.
