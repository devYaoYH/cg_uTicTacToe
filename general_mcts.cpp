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

#include<bits/stdc++.h>
using namespace std;
typedef pair<int, int> ii;
typedef vector<ii> vii;

int board[4][4];

int adj[8][2] = {{0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}, {-1, -1}, {-1, 0}, {-1, 1}};
int lines[8][3][2] = {
    {{0, 0}, {0, 1}, {0, 2}},   //Horizontals
    {{1, 0}, {1, 1}, {1, 2}},
    {{2, 0}, {2, 1}, {2, 2}},
    {{0, 0}, {1, 0}, {2, 0}},   //Verticals
    {{0, 1}, {1, 1}, {2, 1}},
    {{0, 2}, {1, 2}, {2, 2}},
    {{0, 0}, {1, 1}, {2, 2}},   //Cross
    {{0, 2}, {1, 1}, {2, 0}}
};

/* HEADER FILES */
//TGrid.h
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
    cerr << "-----" << endl;
    for (int r=0;r<3;++r){
        cerr << '|';
        for (int c=0;c<3;++c){
            int cell_winner = grid[r][c]->winner();
            if (cell_winner == 0) cerr << 'o';
            else if (cell_winner == 1) cerr << 'x';
            else cerr << ' ';
            if (cell_winner == play_EMPTY){
                has_moves = true;
            }
        }
        cerr << '|' << endl;
    }
    cerr << "-----" << endl;
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
MyReferee::MyReferee(): turn_count(0){
    hive = game;
    for (int i=0;i<NUM_AGENTS;++i){
        player_wins[i] = 0;
        player_rank[i] = 0;
    }
    last_move = pair<int, int>(-1, -1);
    board = new LargeGrid();
}

bool MyReferee::run_agent(int idx){
    //Query the board with last move to get list of valid locations
    vector<pair<int, int> > next_locations;
    board->getValidLocations(next_locations);
    //Format it into a string to write to agent
    string to_child = to_string(last_move.first) + " " + to_string(last_move.second) + "\n" + to_string(next_locations.size()) + "\n";
    for (pair<int, int> p: next_locations){
        to_child += to_string(p.first) + " " + to_string(p.second) + "\n";
    }
    //Interact with agent to get response
    string response;
    ERR_CODES agent_status = hive->invoke_agent(idx, to_child, response);
    if (agent_status == SUCCESS){
        stringstream ss(response);
        int a, b;
        if (ss >> a && ss >> b){
            last_move.first = a;
            last_move.second = b;
            //Check outcome of this move
            int outcome = board->play(last_move, idx);
            if (outcome == idx){
                //Add to win boards
                player_wins[idx]++;
            }
            else if (outcome == play_ERROR){
                //Player made an invalid move
                cerr << "Agent " << idx << " Made an INVALID move: " << response << endl;
                return false;
            }

            //Only case where valid execution happens
            return true;
        }
        else{
            //Error badly formatted response
            cerr << "Agent " << idx << " Responded with bad string: " << response << endl;
        }
    }
    else{
        cerr << "Agent " << idx << " Responded with ERROR CODE: " << ERR_STRINGS[agent_status] << endl;
    }
    return false;
}

bool MyReferee::turn(){
    turn_count++;
    //Run agents sequentially
    for (int i=0;i<2;++i){
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

void MyReferee::print_result(){
    cerr << "#####" << endl;
    cerr << player_rank[0] << " " << player_rank[1] << endl;
}

int* MyReferee::run(){
    //Write initial move to agent
    do{
        cerr << "=====" << endl << turn_count << endl;
    } while(turn());
    board->display();
    print_result();
    return player_rank;
}

MyReferee::~MyReferee(){
    delete board;
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main()
{

    // game loop
    while (1) {
        vii valid_actions;
        int opponentRow;
        int opponentCol;
        cin >> opponentRow >> opponentCol; cin.ignore();
        
        // Record opponent's move
        board[opponentRow][opponentCol] = -1;
        
        int validActionCount;
        cin >> validActionCount; cin.ignore();
        for (int i = 0; i < validActionCount; i++) {
            int row;
            int col;
            cin >> row >> col; cin.ignore();
            valid_actions.push_back(ii(row, col));
        }

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        
        int curR, curC;
        ii insta_move = scanLines();
        cerr<<insta_move.first<<" "<<insta_move.second<<endl;
        if (insta_move.first != -1){
            curR = insta_move.first;
            curC = insta_move.second;
        }
        else{
            curR = valid_actions[0].first;
            curC = valid_actions[0].second;
        }
        
        cout<<curR<<" "<<curC<<endl;
        board[curR][curC] = 1;
    }
}
