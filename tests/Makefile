CC=gcc

all: string-parsing assembly-gen

string-parsing: string-parsing.c ../include/parser.h ../include/command.h ../include/stack_arena.h ../src/parser.c ../src/stack_arena.c
	$(CC) -g string-parsing.c ../src/parser.c ../src/stack_arena.c -o String-parsing

assembly-gen: assembly-gen.c ../include/parser.h ../include/command.h ../include/stack_arena.h ../include/assembly_gen.h ../src/parser.c ../src/assembly_gen.c ../src/stack_arena.c
	$(CC) -g assembly-gen.c ../src/parser.c ../src/stack_arena.c ../src/assembly_gen.c -o Assembly-gen 
