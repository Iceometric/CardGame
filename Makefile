all:
	mkdir target
	gcc src/main.c -o target/run -Wall -Wpedantic
	./target/run
