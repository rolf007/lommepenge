all: lommepenge

lommepenge: lommepenge.cpp Makefile
	g++ -std=c++11 lommepenge.cpp -o $@

clean:
	rm lommepenge
