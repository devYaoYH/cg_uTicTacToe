// trigger optimisation from source file
//#define OPTIMIZE //Trigger cg compiler optimizations

#ifdef OPTIMIZE
#pragma GCC optimize("O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops") //Optimization flags

#pragma GCC optimize("Ofast")

#pragma GCC option("arch=native","tune=native","no-zero-upper") //Enable AVX
#pragma GCC target("avx")  //Enable AVX
#pragma GCC target "bmi2"
#include <x86intrin.h> //AVX/SSE Extensions
#endif

//INCLUDES
#include<bits/stdc++.h>
#include<random>
#include<iterator>
#include<memory>
#include<chrono>
using namespace std;

//DEFINES
#define MYSELF 0
#define ENEMY 1
#define TIME_LIMIT_US 45000

/* RANDOM SELECTION */
template <typename RandomGenerator = std::default_random_engine>
struct random_selector
{
	//On most platforms, you probably want to use std::random_device("/dev/urandom")()
	random_selector(RandomGenerator g = RandomGenerator(std::random_device()()))
		: gen(g) {}

	template <typename Iter>
	Iter select(Iter start, Iter end) {
		std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
		std::advance(start, dis(gen));
		return start;
	}

	//convenience function
	template <typename Iter>
	Iter operator()(Iter start, Iter end) {
		return select(start, end);
	}

	//convenience function that works on anything with a sensible begin() and end(), and returns with a ref to the value type
	template <typename Container>
	auto operator()(const Container& c) -> decltype(*begin(c))& {
		return *select(begin(c), end(c));
	}

private:
	RandomGenerator gen;
};

random_selector<> choice{};

/* HEADER FILES */
//TGrid.h

const int play_EMPTY = -1;
const int play_ERROR = -2;
const int play_DRAW = -3;

class TGrid{
    public:
        TGrid();
        virtual ~TGrid();

        //Report to game referee who won
        virtual int winner() = 0;
        //Simulate a move on the board
        virtual int play(pair<int, int> move, int player) = 0;
        //Get a list of playable locations
        virtual void getValidLocations(vector<pair<int, int> >& output_list) = 0;
    protected:
        static int winning_lines[8][3][2];
};

//SmallGrid.h
class SmallGrid: public TGrid{
    public:
        SmallGrid();
        SmallGrid(const SmallGrid& other);
        SmallGrid& operator=(SmallGrid& other);
        virtual ~SmallGrid();

        int getPos(int r, int c);

        virtual int winner();
        virtual int play(pair<int, int> move, int player);
        virtual void getValidLocations(vector<pair<int, int> >& output_list);
    private:
        int grid[3][3];
};

//LargeGrid.h
class LargeGrid: public TGrid{
    public:
        LargeGrid();
        LargeGrid(const LargeGrid& other);
        LargeGrid& operator=(LargeGrid& other);
        virtual ~LargeGrid();
        
        //Prints board to stderr
        void display();
        
        virtual int winner();
        virtual int play(pair<int, int> move, int player);
        virtual void getValidLocations(vector<pair<int, int> >& output_list);
    private:
        //Previously made move -> determines next grid
        pair<int, int> prev_move;
        pair<int, int> last_played;
        //Grid of Tic Tac Toe objects
        shared_ptr<SmallGrid> grid[3][3];
};

//MyReferee.h
class Referee {
    public:
        Referee();
        Referee(const Referee& other);
        Referee& operator=(Referee& other);
        int move(int idx, pair<int, int> cur_move);
        LargeGrid* getBoard();
        virtual ~Referee();
        virtual int* run();
    private:
        int turn_count;
        int player_wins[2];
        int player_rank[2];
        pair<int, int> last_move;
        LargeGrid* board;
        bool turn();
        bool run_agent(int idx);
        void print_result();
};

/* IMPLEMENTATIONS */
//TGrid.cpp
int TGrid::winning_lines[8][3][2] = {
    {{0, 0}, {0, 1}, {0, 2}},
    {{1, 0}, {1, 1}, {1, 2}},
    {{2, 0}, {2, 1}, {2, 2}},
    {{0, 0}, {1, 0}, {2, 0}},
    {{0, 1}, {1, 1}, {2, 1}},
    {{0, 2}, {1, 2}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}},
    {{0, 2}, {1, 1}, {2, 0}}
};

TGrid::TGrid() {}

TGrid::~TGrid() {}

//SmallGrid.cpp
SmallGrid::SmallGrid(){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = -1;
        }
    }
}

SmallGrid::SmallGrid(const SmallGrid& other){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = other.grid[r][c];
        }
    }
}

SmallGrid& SmallGrid::operator=(SmallGrid& other){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = other.grid[r][c];
        }
    }
    return *this;
}

int SmallGrid::getPos(int r, int c){
    return grid[r][c];
}

int SmallGrid::winner(){
    for (int i=0;i<8;++i){
        //Check each line
        int winner = grid[winning_lines[i][0][0]][winning_lines[i][0][1]];
        if (winner != 0 && winner != 1) continue;
        int line_winner = winner;
        for (int j=1;j<3;++j){
            int next_winner = grid[winning_lines[i][j][0]][winning_lines[i][j][1]];
            if (next_winner != winner){
                line_winner = -1;
                break;
            }
        }
        if (line_winner != -1){
            //We have a winner!!!
            return line_winner;
        }
    }
    bool has_moves = false;
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            if (grid[r][c] == -1){
                has_moves = true;
                break;
            }
        }
    }
    if (!has_moves) return play_DRAW;
    return play_EMPTY;
}

int SmallGrid::play(pair<int, int> move, int player){
    if (grid[move.first][move.second] == -1){
        grid[move.first][move.second] = player;
    }
    else return play_ERROR;
    return winner();
}

void SmallGrid::getValidLocations(vector<pair<int, int> >& output_list){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            if (grid[r][c] == -1){
                output_list.push_back(pair<int, int>(r, c));
            }
        }
    }
    return;
}

SmallGrid::~SmallGrid() {}

//LargeGrid.cpp
LargeGrid::LargeGrid(): prev_move(pair<int, int>(-1, -1)), last_played(pair<int, int>(-1, -1)){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = make_shared<SmallGrid>();
        }
    }
}

LargeGrid::LargeGrid(const LargeGrid& other): prev_move(other.prev_move), last_played(other.last_played){
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = make_shared<SmallGrid>(*(other.grid[r][c]));
        }
    }
}

LargeGrid& LargeGrid::operator=(LargeGrid& other){
    prev_move.first = other.prev_move.first;
    prev_move.second = other.prev_move.second;
    last_played.first = other.last_played.first;
    last_played.second = other.last_played.second;
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            grid[r][c] = make_shared<SmallGrid>(*(other.grid[r][c]));
        }
    }
    return *this;
}

void LargeGrid::display(){
    int tmp_grid[9][9];
    for (int r=0;r<3;++r){
        for (int c=0;c<3;++c){
            //For each grid load pieces
            int gr = r*3, gc = c*3;
            for (int a=0;a<3;++a){
                for (int b=0;b<3;++b){
                    tmp_grid[gr+a][gc+b] = grid[r][c]->getPos(a, b);
                }
            }
        }
    }
    for (int r=0;r<9;++r){
        if (r%3 == 0) cerr << "-------------" << endl;
        for (int c=0;c<9;++c){
            if (c%3 == 0) cerr << '|';
            if (last_played.first == r && last_played.second == c){
                if (tmp_grid[r][c] == 0) cerr << 'O';
                else if (tmp_grid[r][c] == 1) cerr << 'X';
                else cerr << ' ';
            }
            else{
                if (tmp_grid[r][c] == 0) cerr << 'o';
                else if (tmp_grid[r][c] == 1) cerr << 'x';
                else cerr << ' ';
            }
        }
        cerr << '|' << endl;
    }
    cerr << "-------------" << endl;
}

int LargeGrid::winner(){
    bool has_moves = false;
    //cerr << "-----" << endl;
    for (int r=0;r<3;++r){
        //cerr << '|';
        for (int c=0;c<3;++c){
            int cell_winner = grid[r][c]->winner();
            //if (cell_winner == 0) cerr << 'o';
            //else if (cell_winner == 1) cerr << 'x';
            //else cerr << ' ';
            if (cell_winner == play_EMPTY){
                has_moves = true;
            }
        }
        //cerr << '|' << endl;
    }
    //cerr << "-----" << endl;
    for (int i=0;i<8;++i){
        //Check each line
        int winner = grid[winning_lines[i][0][0]][winning_lines[i][0][1]]->winner();
        if (winner != 0 && winner != 1) continue;
        int line_winner = winner;
        for (int j=1;j<3;++j){
            int next_winner = grid[winning_lines[i][j][0]][winning_lines[i][j][1]]->winner();
            if (next_winner != winner){
                line_winner = -1;
                break;
            }
        }
        if (line_winner != -1){
            //We have a winner!!!
            return line_winner;
        }
    }
    if (!has_moves) return play_DRAW;
    return play_EMPTY;
}

int LargeGrid::play(pair<int, int> move, int player){
    int gr = move.first/3, gc = move.second/3;
    //Check that player has played in a valid grid
    if (prev_move.first != -1 && grid[prev_move.first][prev_move.second]->winner() == play_EMPTY && (gr != prev_move.first || gc != prev_move.second)) return play_ERROR;
    int cr = move.first - gr*3, cc = move.second - gc*3;
    int play_result = grid[gr][gc]->play(pair<int, int>(cr, cc), player);
    if (play_result != play_ERROR){
        prev_move.first = cr;
        prev_move.second = cc;
        last_played.first = move.first;
        last_played.second = move.second;
    }
    return play_result;
}

void LargeGrid::getValidLocations(vector<pair<int, int> >& output_list){
    if (prev_move.first == -1 || grid[prev_move.first][prev_move.second]->winner() != play_EMPTY){
        //Can move anywhere
        for (int r=0;r<3;++r){
            for (int c=0;c<3;++c){
                if (grid[r][c]->winner() != play_EMPTY) continue;
                vector<pair<int, int> > tmp;
                grid[r][c]->getValidLocations(tmp);
                for (pair<int, int> cell: tmp){
                    output_list.push_back(pair<int, int>(r*3 + cell.first, c*3 + cell.second));
                }
            }
        }
    }
    else{
        //Restricted to previous grid ONLY
        vector<pair<int, int> > tmp;
        grid[prev_move.first][prev_move.second]->getValidLocations(tmp);
        for (pair<int, int> c: tmp){
            output_list.push_back(pair<int, int>(prev_move.first*3 + c.first, prev_move.second*3 + c.second));
        }
    }
    return;
}

LargeGrid::~LargeGrid() {}

//MyReferee.cpp
Referee::Referee(): turn_count(0){
    for (int i=0;i<2;++i){
        player_wins[i] = 0;
        player_rank[i] = 0;
    }
    last_move = pair<int, int>(-1, -1);
    board = new LargeGrid();
}

Referee::Referee(const Referee& other): turn_count(other.turn_count), last_move(other.last_move), board(nullptr){
    for (int i=0;i<2;++i){
        player_wins[i] = other.player_wins[i];
        player_rank[i] = other.player_rank[i];
    }
    board = new LargeGrid(*(other.board));
}

Referee& Referee::operator=(Referee& other){
    turn_count = other.turn_count;
    player_wins[0] = other.player_wins[0];
    player_wins[1] = other.player_wins[1];
    player_rank[0] = other.player_rank[0];
    player_rank[1] = other.player_rank[1];
    last_move.first = other.last_move.first;
    last_move.second = other.last_move.second;
    delete board;
    board = new LargeGrid(*(other.board));
    return *this;
}

bool Referee::run_agent(int idx){
    //Query the board with last move to get list of valid locations
    vector<pair<int, int> > next_locations;
    board->getValidLocations(next_locations);
    //Play random available move
    last_move = choice(next_locations);
    //Check outcome of this move
    int outcome = board->play(last_move, idx);
    if (outcome == idx){
        //Add to win boards
        player_wins[idx]++;
    }
    else if (outcome == play_ERROR){
        //Player made an invalid move
        cerr << "Agent " << idx << " Made an INVALID move: " << last_move.first << "," << last_move.second << endl;
        return false;
    }
    //Only case where valid execution happens
    return true;
}

bool Referee::turn(){
    turn_count++;
    //Run agents sequentially
    for (int j=0;j<2;++j){
        //Always start with ENEMY move first
        int i = (1 + j)%2;
        if (!run_agent(i)){
            //Agent reports error (crashes)
            player_wins[i] = -1;
        }
        board->display();
        int play_result = board->winner();
        if (play_result == i){
            //We have a winner!!
            if (i == 0){
                player_rank[0] = 1;
                player_rank[1] = 2;
            }
            else{
                player_rank[0] = 2;
                player_rank[1] = 1;
            }
            return false;
        }
        else if (play_result == play_DRAW){
            //Draw game
            //Check how many small grids each player won
            if (player_wins[0] > player_wins[1]){
                //Player 0 wins!
                player_rank[0] = 1;
                player_rank[1] = 2;
            }
            else if (player_wins[1] > player_wins[0]){
                //Player 1 wins!
                player_rank[0] = 2;
                player_rank[1] = 1;
            }
            else{
                //DRAW GAME
                player_rank[0] = 1;
                player_rank[1] = 1;
            }
            return false;
        }
    }
    if (player_wins[0] == -1 && player_wins[1] == -1){
        //SAD, both players crashed
        //DRAW
        player_rank[0] = -1;
        player_rank[1] = -1;
        return false;
    }
    else if (player_wins[0] == -1){
        //Player 1 wins
        player_rank[0] = -1;
        player_rank[1] = 1;
        return false;
    }
    else if (player_wins[1] == -1){
        //Player 0 wins
        player_rank[0] = 1;
        player_rank[1] = -1;
        return false;
    }
    return true;
}

void Referee::print_result(){
    cerr << "#####" << endl;
    cerr << player_rank[0] << " " << player_rank[1] << endl;
}

int* Referee::run(){
    //Write initial move to agent
    do{
        cerr << "=====" << endl << turn_count << endl;
    } while(turn());
    board->display();
    print_result();
    return player_rank;
}

LargeGrid* Referee::getBoard(){
    return board;
}

int Referee::move(int idx, pair<int, int> cur_move){
    int outcome = board->play(cur_move, idx);
    if (outcome == idx) player_wins[idx]++;
    return board->winner();
}

Referee::~Referee(){
    delete board;
}

/*
 * DECISION MAKING ALGO
 */
class MCTS{
    public:
        MCTS() {}
        ~MCTS() {}
        pair<int, int> play(Referee& cur_state, int time_limit_us);
    private:
        map<string, set<pair<int, int> > > visited_states;
};

pair<int, int> MCTS::play(Referee& cur_state, int time_limit_us){
    //Make a deep copy of our current state
    Referee init_state = Referee(cur_state);
    
    //Pick a random move
    vector<pair<int, int> > valid_moves;
    cur_state.getBoard()->getValidLocations(valid_moves);
    pair<int, int> next_move = choice(valid_moves);
    return next_move;
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main()
{
    /* Stack Variables */
    Referee game = Referee();
    MCTS decide = MCTS();
    
    // game loop
    while (1) {
        vector<pair<int, int> > valid_actions;
        int opponentRow;
        int opponentCol;
        cin >> opponentRow >> opponentCol; cin.ignore();
        
        // Record opponent's move
        if (opponentRow != -1){
            game.move(ENEMY, pair<int, int>(opponentRow, opponentCol));
        }

        int validActionCount;
        cin >> validActionCount; cin.ignore();
        for (int i = 0; i < validActionCount; i++) {
            int row;
            int col;
            cin >> row >> col; cin.ignore();
            valid_actions.push_back(pair<int, int>(row, col));
        }

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        
        int curR, curC;
        pair<int, int> next_move = decide.play(game, TIME_LIMIT_US);
        cerr << next_move.first << " " << next_move.second << endl;
        
        cout << next_move.first << " " << next_move.second << endl;
        game.move(MYSELF, next_move);
    }

    return 0;
}
