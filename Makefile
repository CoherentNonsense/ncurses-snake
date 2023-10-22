all:
	g++ src/main.cpp -o snake -O2 -lncurses

clean:
	rm snake
