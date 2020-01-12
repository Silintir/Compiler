all: flex bison
	clang++ -std=c++11 -Wno-deprecated-register calculator.tab.c lex.yy.c operators.cpp -lfl -o calculator

flex: 
	flex calculator.l

bison:
	bison -d calculator.y

clean:
	rm calculator.tab.h calculator.tab.c lex.yy.c calculator