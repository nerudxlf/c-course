all: mydaemon

mydaemon: main.c
	$(CC) $^ -o $@ -lcurl -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f mydaemon core

.PHONY: all clean
