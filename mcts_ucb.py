import sys
import math
import time
import random

##################
# TIMER FUNCTION #
##################
def debug_time(msg, init, now):
    print("{} {}ms".format(msg, int(round((now-init)*1000*1000))/1000.0), file=sys.stderr)

###################
# BEHAVIOR CONSTS #
###################
C_RATE = 1.1
SEARCH_DEPTH = 9
INSTANT_WIN = False
DEFAULT_SAFE_SEARCH = False

#########
# Const #
#########
SELF = 2        # Play by Self      ||      Grid won by Self
ENEMY = -2      # Play by Enemy     ||      Grid won by Enemy
EMPTY = 0       # No Play           ||      Grid Undecided
TIED = -1       # -                 ||      Grid Tied (No Winner)
WIN_SELF = 6
WIN_ENEMY = -6

RANGE3 = (0, 1, 2)
RANGE9 = (0, 1, 2, 3, 4, 5, 6, 7, 8)

T_GRADIENT = [0 for i in range(50)]
for i in range(1, 50):
    T_GRADIENT[i] = 1/(((i+1)//2)**0.5)

# Adjacent Cell translation tuples
ADJ = ((0, -1), (0, 1), (1, -1), (1, 0), (1, 1), (-1, -1), (-1, 0), (-1, 1))

# Winning Lines of Play
LINES = (
    ((0, 0), (0, 1), (0, 2)),   # Verticals
    ((1, 0), (1, 1), (1, 2)),
    ((2, 0), (2, 1), (2, 2)),
    ((0, 0), (1, 0), (2, 0)),   # Horizontals
    ((0, 1), (1, 1), (2, 1)),
    ((0, 2), (1, 2), (2, 2)),
    ((0, 0), (1, 1), (2, 2)),   # Cross
    ((0, 2), (1, 1), (2, 0))
)

CELL_LINES = (
    (
        (((0, 0), (0, 1), (0, 2)), ((0, 0), (1, 0), (2, 0)), ((0, 0), (1, 1), (2, 2))),
        (((0, 0), (0, 1), (0, 2)), ((0, 1), (1, 1), (2, 1))),
        (((0, 0), (0, 1), (0, 2)), ((0, 2), (1, 2), (2, 2)), ((0, 2), (1, 1), (2, 0)))
    ),
    (
        (((1, 0), (1, 1), (1, 2)), ((0, 0), (1, 0), (2, 0))),
        (((1, 0), (1, 1), (1, 2)), ((0, 1), (1, 1), (2, 1)), ((0, 0), (1, 1), (2, 2)), ((0, 2), (1, 1), (2, 0))),
        (((1, 0), (1, 1), (1, 2)), ((0, 2), (1, 2), (2, 2)))
    ),
    (
        (((2, 0), (2, 1), (2, 2)), ((0, 0), (1, 0), (2, 0)), ((0, 2), (1, 1), (2, 0))),
        (((2, 0), (2, 1), (2, 2)), ((0, 1), (1, 1), (2, 1))),
        (((2, 0), (2, 1), (2, 2)), ((0, 2), (1, 2), (2, 2)), ((0, 0), (1, 1), (2, 2)))
    )
)

# Generate grid to cell mapping
BOARD_TEMPLATE = [[[[0, 0, 0] for cy in RANGE3] for gy in RANGE3] for gx in RANGE3]
grid_to_idx = [[[[0, 0, 0] for cy in RANGE3] for gy in RANGE3] for gx in RANGE3]
for gx in RANGE3:
    for gy in RANGE3:
        for cx in RANGE3:
            for cy in RANGE3:
                grid_to_idx[gx][gy][cx][cy] = (gy*3+cy, gx*3+cx)

# Generate cell to grid mapping
GRID_TEMPLATE = [[0 for gy in RANGE9] for gx in RANGE9]
idx_to_grid = [[0 for gy in RANGE9] for gx in RANGE9]
for gx in RANGE9:
    for gy in RANGE9:
        nx = int(gx//3)
        ny = int(gy//3)
        idx_to_grid[gx][gy] = (nx, ny, gx-nx*3, gy-ny*3)

# State Tuple
# (board, player, next_grid, won_grids)
# 0    -> [3][3][3][3] Board
# 1 -> -2/2 Player (ENEMY, SELF)
# 2 -> (x, y) next grid
# 3 -> [3][3] Grid of grid outcomes
BOARD = 0
PLAYER = 1
NEXT_GRID = 2
WON_GRIDS = 3
STATE_TEMPLATE = [[[[[0, 0, 0] for cy in RANGE3] for gy in RANGE3] for gx in RANGE3], ENEMY, (-1, -1), [[0, 0, 0] for gy in RANGE3]]

# Fast inline copy of state
# sim_state = [[[[cx[:] for cx in gy] for gy in gx] for gx in state[0]], state[1], state[2], [cy[:] for cy in state[3]]]

# Move Tuple
# (PLAYER, idx_coord)

class Board(object):
    def __init__(self):
        self.cur_state = [[[[[0, 0, 0] for cy in RANGE3] for gy in RANGE3] for gx in RANGE3], ENEMY, (-1, -1), [[0, 0, 0] for gy in RANGE3]]

    # Updates based on previous enemy move
    def play(self, move, state=None):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        # Update stuff
        gx, gy, cx, cy = move[1]
        board[gx][gy][cx][cy] = move[0]
        # Update grid outcome of this play
        # state[WON_GRIDS][gx][gy] = self.verify_grid((gx, gy), state=state)
        state[WON_GRIDS][gx][gy] = self.fast_verify_grid((gx, gy, cx, cy), state=state)
        # Update next grid to play in resulting from this play
        state[NEXT_GRID] = (cx, cy)
        # Update next player to play this state
        state[PLAYER] = -1*state[PLAYER]
        return state[WON_GRIDS][gx][gy]

    # Scores faster single grid of 3x3 cells => Early exit for monte carlo
    def fast_verify_grid(self, move, state=None, debug=False):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        gx, gy, cx, cy = move
        for line in CELL_LINES[cx][cy]:
            l_score = sum([board[gx][gy][i[0]][i[1]] for i in line])
            if (debug):
                print(l_score, file=sys.stderr)
            if (l_score == WIN_ENEMY):
                return ENEMY
            elif (l_score == WIN_SELF):
                return SELF
        # Check if grid is fully occupied
        for cx in RANGE3:
            for cy in RANGE3:
                if (board[gx][gy][cx][cy] == EMPTY):
                    return EMPTY
        # No empty cells ==> Grid is TIED
        return TIED

    # Scores single grid of 3x3 cells => Early exit for monte carlo
    def verify_grid(self, grid, state=None, debug=False):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        for line in LINES:
            l_score = sum([board[grid[0]][grid[1]][i[0]][i[1]] for i in line])
            if (debug):
                print(l_score, file=sys.stderr)
            if (l_score == WIN_ENEMY):
                return ENEMY
            elif (l_score == WIN_SELF):
                return SELF
        # Check if grid is fully occupied
        for cx in RANGE3:
            for cy in RANGE3:
                if (board[grid[0]][grid[1]][cx][cy] == EMPTY):
                    return EMPTY
        # No empty cells ==> Grid is TIED
        return TIED

    # Scores single grid of 3x3 cells => Early exit for monte carlo
    def verify_grid_avoid(self, grid, state=None, debug=False):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        for line in LINES:
            cur_placement = [board[grid[0]][grid[1]][i[0]][i[1]] for i in line]
            if (EMPTY in cur_placement and sum(cur_placement) == -4):
                return True
        return False

    # Returns a list of possible moves
    def next_moves(self, state=None, safe=DEFAULT_SAFE_SEARCH):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        # Check if proposed grid is fully occupied (won)
        n_grid = state[NEXT_GRID]
        if (n_grid[0] == -1):    # Starting...
            g_state = TIED
        else:
            g_state = state[WON_GRIDS][n_grid[0]][n_grid[1]]

        # Avoid dangerous grids
        avoid_grid = set()
        if (safe):
            for gx in RANGE3:
                for gy in RANGE3:
                    if (self.verify_grid_avoid((gx, gy), state=state)):
                        avoid_grid.add((gx, gy))

        open_cells = []
        if (g_state == EMPTY):    # Undecided, great, restrict play to this grid only
            for cx in RANGE3:
                for cy in RANGE3:
                    if (board[n_grid[0]][n_grid[1]][cx][cy] == EMPTY):
                        open_cells.append((state[PLAYER], (n_grid[0], n_grid[1], cx, cy)))
        else:
            for gx in RANGE3:
                for gy in RANGE3:
                    if (state[WON_GRIDS][gx][gy] != EMPTY):
                        continue
                    for cx in RANGE3:
                        for cy in RANGE3:
                            if (board[gx][gy][cx][cy] == EMPTY):
                                open_cells.append((state[PLAYER], (gx, gy, cx, cy)))

        if (safe):
            # Keep a backup
            orig_cells = open_cells[:]
            
            # Check for cells to avoid
            for i, m in enumerate(open_cells):
                if (m[0] != SELF):
                    continue
                gx, gy, cx, cy = m[1]
                if ((cx, cy) in avoid_grid):
                    open_cells[i] = None
                if (len(avoid_grid) > 0):
                    if (state[WON_GRIDS][cx][cy] != EMPTY):
                        open_cells[i] = None

            open_cells = [c for c in open_cells if c is not None]

            if (len(open_cells) < 1):
                return orig_cells
            else:
                return open_cells
        else:
            return open_cells

    # Project cells in which placement will win
    def projectWin(self, player, state=None, grid=None):
        if (state is None):
            state = self.cur_state
        if (grid is None):      # Global
            winning_grids = []
            for gx in RANGE3:
                for gy in RANGE3:
                    if (state[WON_GRIDS][gx][gy] != EMPTY):
                        continue
                    for line in CELL_LINES[gx][gy]:
                        l_score = sum([state[WON_GRIDS][i[0]][i[1]] for i in line])
                        l_score += player
                        if (l_score == player*3):
                            winning_grids.append((gx, gy))
            return winning_grids
        else:                   # On this grid
            winning_cells = []
            for cx in RANGE3:
                for cy in RANGE3:
                    if (state[BOARD][grid[0]][grid[1]][cx][cy] != EMPTY):
                        continue
                    for line in CELL_LINES[cx][cy]:
                        l_score = sum([state[BOARD][grid[0]][grid[1]][i[0]][i[1]] for i in line])
                        l_score += player
                        if (l_score == player*3):
                            winning_cells.append((grid[0], grid[1], cx, cy))
            return winning_cells

    # Global GAME winner
    def winner(self, state=None):
        if (state is None):
            state = self.cur_state
        board = state[BOARD]
        for line in LINES:
            l_score = sum([state[WON_GRIDS][i[0]][i[1]] for i in line])
            if (l_score == WIN_ENEMY):
                return ENEMY
            elif (l_score == WIN_SELF):
                return SELF
        return EMPTY

class MonteCarlo(object):
    def __init__(self, game):
        self.game = game
        self.C = C_RATE
        self.plays = dict()
        self.wins = dict()
        self.max_depth = 0

        # DP-ish memoization
        self.scanned_legals = dict()

    def next_play(self, t_limit, state):
        # Track time used and escape
        esc_time = time.time()+t_limit

        # Reset turn simulation variables        
        self.max_depth = 0
        self.scanned_legals = dict()

        # Get a sensing of current legal moves
        state_tuple = (tuple(tuple(tuple(tuple(cx[:]) for cx in gy) for gy in gx) for gx in state[0]), state[1], state[2], tuple(tuple(cy[:]) for cy in state[3]))
        cur_legal = self.game.next_moves(state=state)
        self.scanned_legals[state_tuple] = cur_legal    # Save some computation...

        # Predicted legal moves
        # predicted_valid_actions = [grid_to_idx[m[1][0]][m[1][1]][m[1][2]][m[1][3]] for m in cur_legal]
        # predicted_valid_actions = sorted(predicted_valid_actions)
        # print("Predicted: ", predicted_valid_actions, file=sys.stderr)

        # Bail Out (ERROR)
        if (len(cur_legal) < 1):
            print("ERROR! No legal moves computed...")
            return None

        if (INSTANT_WIN):
            # Whack the most obvious winning/blocking moves
            gx, gy = state[NEXT_GRID]
            free_move = False
            predicted_move = None
            # Check if we are free to move anywhere
            if (state[WON_GRIDS][gx][gy] != EMPTY):
                free_move = True
            # Identify which grid we can win in
            grids = self.game.projectWin(SELF, state=state)
            if (len(grids) > 0):
                if (free_move):
                    for g in grids:
                        cells = self.game.projectWin(SELF, state=state, grid=g)
                        if (len(cells) > 0):
                            predicted_move = (SELF, cells[0])
                            break
                else:
                    if ((gx, gy) in grids):
                        cells = self.game.projectWin(SELF, state=state, grid=(gx, gy))
                        if (len(cells) > 0):
                            predicted_move = (SELF, cells[0])

            # Identify which grid enemy will win in
            if (predicted_move is None):
                grids = self.game.projectWin(ENEMY, state=state)
                if (len(grids) > 0):
                    if (free_move):
                        for g in grids:
                            cells = self.game.projectWin(ENEMY, state=state, grid=g)
                            if (len(cells) > 0):
                                predicted_move = (SELF, cells[0])
                                break
                    else:
                        if ((gx, gy) in grids):
                            cells = self.game.projectWin(ENEMY, state=state, grid=(gx, gy))
                            if (len(cells) > 0):
                                predicted_move = (SELF, cells[0])

            if (predicted_move is not None):
                print("Predicting Instant Win Scenario...", file=sys.stderr)
                return predicted_move

        # Vary search depth based on completed grids
        undecided_grids = 0
        enemy_grids = 0
        self_grids = 0
        for gx in RANGE3:
            for gy in RANGE3:
                if (state[WON_GRIDS][gx][gy] == EMPTY):
                    undecided_grids += 1
                elif (state[WON_GRIDS][gx][gy] == ENEMY):
                    enemy_grids += 1
                elif (state[WON_GRIDS][gx][gy] == SELF):
                    self_grids += 1

        # Decide whether to do deep eval
        is_end_game = False
        if (enemy_grids >= 2):
            grids = self.game.projectWin(ENEMY, state=state)
            if (len(grids) > 0):
                is_end_game = True
        if (self_grids >= 2 and not is_end_game):
            grids = self.game.projectWin(SELF, state=state)
            if (len(grids) > 0):
                is_end_game = True

        # Rune MCTS simulation
        games = 0
        while(time.time() < esc_time):
            self.run_simulation(state, max_moves=SEARCH_DEPTH+int(undecided_grids//2), end_game=is_end_game)
            games += 1

        print("Games Run: {} | Max Depth: {}".format(games, self.max_depth), file=sys.stderr)

        # Pick highest win-rate move
        percent_wins, move = max(
            (self.wins.get((m, state_tuple), 0) /
             self.plays.get((m, state_tuple), 1), m)
            for m in cur_legal
        )

        # for x in sorted([(100*self.wins.get((m, state_tuple), 0) / 
        #     self.plays.get((m, state_tuple), 1),
        #     self.wins.get((m, state_tuple), 0),
        #     self.plays.get((m, state_tuple), 0), m) for m in cur_legal]):
        #     print("{3}: {0:.2f}% ({1}/{2})".format(*x), file=sys.stderr)

        return move

    def run_simulation(self, state, max_moves=SEARCH_DEPTH, end_game=False):
        # Tracking
        t_gradient = T_GRADIENT
        plays, wins, scanned_legals = self.plays, self.wins, self.scanned_legals
        visited_states = set()

        winner = EMPTY
        self_score = 0
        enemy_score = 0

        # Fast Copy all relevant data
        sim_state = [[[[cx[:] for cx in gy] for gy in gx] for gx in state[0]], state[1], state[2], [cy[:] for cy in state[3]]]
        
        # Run Simulation
        expand = True
        # t = 1
        # while(True):
        for t in range(1, max_moves+1):
            # Generate sim_state tuple for use in keys
            sim_state_tuple = (tuple(tuple(tuple(tuple(cx[:]) for cx in gy) for gy in gx) for gx in sim_state[0]), sim_state[1], sim_state[2], tuple(tuple(cy[:]) for cy in sim_state[3]))
            # Get legal moves moving forward
            # Memoization, check if we have already been here before
            if (scanned_legals.get(sim_state_tuple)):
                legal_moves = scanned_legals[sim_state_tuple]
            else:
                legal_moves = self.game.next_moves(state=sim_state)
                scanned_legals[sim_state_tuple] = legal_moves

            # if (len(legal_moves) < 1):
            #     legal_moves = self.game.next_moves(state=sim_state, safe=False)
            #     scanned_legals[sim_state_tuple] = legal_moves

            # Escape early if stuck
            if (len(legal_moves) < 1):
                break

            # Select next node
            if (all(plays.get((m, sim_state_tuple)) for m in legal_moves)):
                # UCB1 Ranking
                log_tot = sum(plays[(m, sim_state_tuple)] for m in legal_moves)
                if (log_tot <= 0):
                    print(legal_moves, file=sys.stderr)
                    print(log_tot)
                log_total = math.log(log_tot)
                value, move = max(
                    ((wins[(m, sim_state_tuple)] / plays[(m, sim_state_tuple)]) +
                        self.C * math.sqrt(log_total / plays[(m, sim_state_tuple)]), m)
                    for m in legal_moves
                )
            else:
                # If there's a winning move, pick it!!
                predicted_move = None
                free_move = False
                gx, gy = sim_state[NEXT_GRID]
                if (sim_state[WON_GRIDS][gx][gy] != EMPTY):
                    free_move = True
                if (end_game):
                    grids = self.game.projectWin(sim_state[PLAYER], state=sim_state)
                    if (len(grids) > 0):
                        if (free_move):
                            for g in grids:
                                cells = self.game.projectWin(sim_state[PLAYER], state=sim_state, grid=g)
                                if (len(cells) > 0):
                                    predicted_move = (sim_state[PLAYER], cells[0])
                                    break
                        else:
                            if ((gx, gy) in grids):
                                cells = self.game.projectWin(sim_state[PLAYER], state=sim_state, grid=(gx, gy))
                                if (len(cells) > 0):
                                    predicted_move = (sim_state[PLAYER], cells[0])
                else:
                    if (free_move):
                        for ggx in RANGE3:
                            for ggy in RANGE3:
                                if (sim_state[WON_GRIDS][ggx][ggy] != EMPTY or predicted_move is not None):
                                    continue
                                cells = self.game.projectWin(sim_state[PLAYER], state=sim_state, grid=(ggx, ggy))
                                for c in cells:
                                    tmp_state = ((sim_state[PLAYER], c), sim_state_tuple)
                                    if (tmp_state not in visited_states and tmp_state not in plays):
                                        predicted_move = tmp_state[0]
                                        break
                    else:
                        cells = self.game.projectWin(sim_state[PLAYER], state=sim_state, grid=(gx, gy))
                        for c in cells:
                            tmp_state = ((sim_state[PLAYER], c), sim_state_tuple)
                            if (tmp_state not in visited_states and tmp_state not in plays):
                                predicted_move = tmp_state[0]
                                break

                if (predicted_move is not None):
                    move = predicted_move
                    # if (t==1):
                    #     print("PREDICTED: {}".format(move), file=sys.stderr)
                else:
                    # Pick Randomly (a move not yet recorded)
                    unexplored_moves = [m for m in legal_moves if (m, sim_state_tuple) not in visited_states]
                    move = random.choice(unexplored_moves)

            # Record expansion
            if (expand and (move, sim_state_tuple) not in plays):
                expand = False
                plays[(move, sim_state_tuple)] = 0
                wins[(move, sim_state_tuple)] = 0
                if (t > self.max_depth):
                    self.max_depth = t

            # Record visitation
            visited_states.add((move, sim_state_tuple))

            # Evaluate the result of a move
            move_result = self.game.play(move, state=sim_state)
            if (move_result == SELF):
                self_score += t_gradient[t]
                enemy_score -= t_gradient[t]
            elif (move_result == ENEMY):
                enemy_score += t_gradient[t]
                self_score -= t_gradient[t]
            elif (move_result == TIED):
                self_score -= t_gradient[t+20]
                
            if (sim_state[PLAYER] == ENEMY and sim_state[WON_GRIDS][sim_state[NEXT_GRID][0]][sim_state[NEXT_GRID][1]] != EMPTY):
                self_score -= t_gradient[t+20]

            # Break if we encounter grid win (Early Escape)
            # if (move_result == SELF or move_result == ENEMY):
            #     winner = move_result
            #     break

            # End of game break
            winner = self.game.winner(state=sim_state)
            if (winner == SELF or winner == ENEMY):
                break
        
            # t += 1
            # if (self_score != 0 and max_moves < t):
            #     break

        # Back-track and update scores
        for m, v_state in visited_states:
            if ((m, v_state) not in plays):
                continue
            plays[(m, v_state)] += 1
            if (v_state[PLAYER] == winner):
                wins[(m, v_state)] += 100
            if (v_state[PLAYER] == SELF and winner == ENEMY):
                wins[(m, v_state)] -= 100
            wins[(m, v_state)] += self_score if v_state[PLAYER] == SELF else enemy_score
            # wins[(m, v_state)] += (1.5**self_score)-1 if v_state[PLAYER] == SELF else (1.5**enemy_score)-1

game = Board()
engine = MonteCarlo(game)

while True:
    opponent_row, opponent_col = [int(i) for i in input().split()]
    # From opp_row/opp_col we deduce which board we're playing on
    # Update global game state
    if (opponent_row < 0 or opponent_row >= 9 or opponent_col < 0 or opponent_col >= 9):
        print("Invalid Move||Starting...", file=sys.stderr)
        game.cur_state[PLAYER] = SELF
    else:
        o_move = idx_to_grid[opponent_col][opponent_row]
        o_move_result = game.play((ENEMY, o_move))
        # Debug predicted next_grid
        # print(game.cur_state[NEXT_GRID], file=sys.stderr)
        # print(game.cur_state[WON_GRIDS], file=sys.stderr)
        # print(game.cur_state[PLAYER], file=sys.stderr)
        # game.verify_grid((0, 0), debug=True)

    valid_action_count = int(input())
    valid_actions = []
    for i in range(valid_action_count):
        row, col = [int(j) for j in input().split()]
        valid_actions.append((row, col))

    t_round = time.time()

    # Good time to debug and check if our predicted moves matches those given to us
    # predicted_valid_actions = [grid_to_idx[m[0]][m[1]][m[2]][m[3]] for m in self.game.next_moves()]
    # predicted_valid_actions = sorted(predicted_valid_actions)
    # print("Predicted: ", predicted_valid_actions, file=sys.stderr)
    valid_actions = sorted(valid_actions)
    print("Actual: ", valid_actions, file=sys.stderr)

    # Give it 40ms of time
    next_move = engine.next_play(0.048, game.cur_state)
    n_move = next_move[1]
    ngx, ngy, ncx, ncy = n_move
    n_move_idx = grid_to_idx[ngx][ngy][ncx][ncy]

    # Update our model
    n_move_result = game.play(next_move)

    print("{} {}".format(n_move_idx[0], n_move_idx[1]))
    debug_time("Round Time:", t_round, time.time())