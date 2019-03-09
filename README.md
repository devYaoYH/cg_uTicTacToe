# Ultimate Tic Tac Toe

Tic Tac Toe played on a 3x3 grid of 3x3 grids where each move on the individual 3x3 grids determine which grid to play on for the next player.

## MCTS with UCB1 exploration

See this [medium post](https://medium.com/@quasimik/monte-carlo-tree-search-applied-to-letterpress-34f41c86e238) with a rundown of the algorithm implemented here.

Added early exit conditions to make sure one wins on the next move. Otherwise, general implementation of the above described MCTS/UCB1 algorithm