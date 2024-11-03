# Nom du programme
PROGRAM = programme

# Fichiers sources
SRCS = main.c skin.c menu.c

# Compilateur et options
CC = gcc
CFLAGS = -Wall -g

# Cible par défaut : compiler et lier
all: $(PROGRAM)

# Règle de compilation et linkage
$(PROGRAM): $(SRCS)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(SRCS)

# Cible pour exécuter le programme
run: all
	./$(PROGRAM)

# Cible pour nettoyer les fichiers objets
clean:
	rm -f $(PROGRAM)
