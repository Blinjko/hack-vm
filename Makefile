CC=gcc

hack-vm: src/main.c src/parser.c src/stack_arena.c src/assembly_gen.c include/bool.h include/assembly_gen.h include/command.h include/parser.h include/stack_arena.h
	gcc src/main.c src/parser.c src/stack_arena.c src/assembly_gen.c -Wall -pedantic -o Hack-VM 
