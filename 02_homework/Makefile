all: solution

solution: main.c
	$(CC) $^ -o $@ -lcurl -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f solution core

.PHONY: all clean
