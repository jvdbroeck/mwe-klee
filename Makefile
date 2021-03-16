CC=wllvm
CFLAGS=-O1 -Xclang -disable-llvm-passes -g -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0

KLEE_FLAGS=-allow-external-sym-calls -libc=uclibc -posix-runtime --switch-type=internal --simplify-sym-indices -max-memory=4000 --run-in=/tmp/sandbox -watchdog -only-output-states-covering-new
KLEE_TIMEOUT=--max-time=120
KLEE_SEARCH=-search=dfs 

build:
	$(CC) $(CFLAGS) -c hashtable.c
	$(CC) $(CFLAGS) -c predicate.c
	$(CC) $(CFLAGS) -c main.c
	$(CC) $(CFLAGS) -o program main.o predicate.o hashtable.o -L/usr/local/lib -lkleeRuntest
	extract-bc program

klee:
	klee $(KLEE_SEARCH) $(KLEE_FLAGS) $(KLEE_TIMEOUT) ./program.bc

clean:
	rm -f *.bc *.o program
