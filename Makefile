all:
	if ! test -d target; \
		then mkdir target; \
	fi
	gcc src/main.c -o target/run -Wall -Wpedantic -lSDL2main -lSDL2
	./target/run
