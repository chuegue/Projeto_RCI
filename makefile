PROJECT_NAME = cot #target file name

CC = gcc #compiler
CFLAGS = -g#-Wall -O3 -pedantic

OBJECTS = Main.o #Utilities.o Variantes.o Stack.o Dijkstra.o MinHeap.o List.o Queue.o

# FILES = $(shell ls ../Testes/*.in1)

# V6 = $(shell ls ../Testes/enunciado6*.in1)

# SALADAS = $(shell ls ../Testes/salada*.in1)

# SIZEABLE = $(shell ls ../Testes/sizeable*.in1)

# FINAL = $(shell ls ../Testes/*.in)

VALG = valgrind #--leak-check=full

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(PROJECT_NAME)
	rm -f *.o

Main.o: Main.c

# saladas:
# 	for F in ${SALADAS}; do ./roap -s $${F} ; done;

# saladasv:
# 	for F in ${SALADAS}; do ${VALG} ./roap -s $${F} ; echo "\n\n" ; done;

# v6:
# 	for F in ${V6}; do ./roap -s $${F} ; done;

# v6v:
# 	for F in ${V6}; do ${VALG} ./roap -s $${F} ; echo "\n\n" ; done;

# sizeable:
# 	for F in ${SIZEABLE}; do ./roap -s $${F} ; done;

# sizeablev:
# 	for F in ${SIZEABLE}; do ${VALG} ./roap -s $${F} ; echo "\n\n" ; done;

# geral:
# 	for F in ${FILES}; do ./roap -s $${F} ; done;

# geralv:
# 	for F in ${FILES}; do ${VALG} ./roap -s $${F} ; echo "\n\n" ; done;

# final:
# 	for F in ${FINAL}; do ./roap $${F} ; echo "\n\n" ; done;

# finalv:
# 	for F in ${FINAL}; do ${VALG} ./roap $${F} ; echo "\n\n" ; done;

# finalt:
# 	for F in ${FINAL}; do time ./roap $${F} ; echo "\n\n" ; done;

# clean:
# 	rm -f *.o