CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
MODULES = src/log.c src/ordenacao.c src/relatorio.c src/indice.c

.PHONY: all test clean
all: build/analisador
build:
	mkdir -p build
build/analisador: src/main.c $(MODULES) $(wildcard include/*.h) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c $(MODULES) -o $@
build/testes: tests/testes.c $(MODULES) $(wildcard include/*.h) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/testes.c $(MODULES) -o $@
test: build/testes
	./build/testes
clean:
	rm -rf build
