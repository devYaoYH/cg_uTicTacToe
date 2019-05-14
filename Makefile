mcts: mcts.o
	g++ -o $@ $^

mcts.o: mcts.cpp
	g++ -c $^

.PHONY: clean
clean:
	rm -f mcts.o
