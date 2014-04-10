#include <iostream>
#include <string>
#include <iomanip>      // setw
#include <queue>        // priority_queue
#include <list>
#include <limits>       // numeric_limits
#include <chrono>       // high_resolution_clock
#include <random>       // default_random_engine, uniform_int_distribution

constexpr int BOARD_MIN_SIZE = 3;
constexpr int BOARD_MAX_SIZE = 21;

#if defined( COLORS )
	#if defined( _WIN32 )
		#include <windows.h>
	#endif
#else
	#pragma message "Compiling without colors"
#endif

enum class color {
#if defined( COLORS )
	#if defined( _WIN32 )
		// 0 black, 1 blue, 2 green, 7 white, 9 light blue,
		red   = 0x0C,
		blue  = 0x09,
		white = 0x07,
	#else
		red   = 31,
		blue  = 34,
		white = 0,
	#endif
// no colors
#else
		red   = 91,
		blue  = 94,
		white = 37,
#endif // ~COLORS
};

std::ostream& operator << (std::ostream& os, color c) {
#if defined( COLORS )
	#if defined( _WIN32 )
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, static_cast<DWORD>(c));
	#else
		os << "\033[0;" << static_cast<int>(c) << "m";
	#endif
#endif // ~COLORS
	return os;
}

enum class node : char {
	red   = 'X',
	blue  = 'O',
	empty = '.',
};

std::ostream& operator << (std::ostream& os, node n) {
	switch (n) {
		case node::red:  std::cout << color::red;  break;
		case node::blue: std::cout << color::blue; break;
		default:         std::cout << color::white;
	}
	os << static_cast<char>(n) << color::white;
	return os;
}

template <class T>
class Graph {
public:
	Graph(int size, T value=0)
		: size(size)
		, nodes(std::vector<T>(size * size, value))
	{
	}

	Graph(const Graph& copy)
		: size(copy.size), nodes(copy.nodes)
	{
	}

	constexpr int number_of_vertices()         { return size; }
	constexpr T   get_node_value(int x, int y) { return nodes[x * size + y]; }
	
	Graph& set_node_value(int x, int y, T v) {
		nodes[x * size + y] = v;
		return *this;
	}

	// returns a list of neighbours filtered by color
	std::list<int> neighbors(int n, T color) const {
		std::list<int> list;

		// neighbour offsets
		int offset[6][2] = {
			{ 0, -1}, { 0,  1},
			{ 1, -1}, { 1,  0},
			{-1,  0}, {-1,  1},
		};
		for (int i = 0; i < 6; ++i) {
			int x = n / size + offset[i][0];
			int y = n % size + offset[i][1];
			if (is_valid_coordinate(x, y)) {
				if (get_node_value(x, y) == color) {
					list.push_back(x * size + y);
				}
			}
		}
		return list;
	}

	bool is_connected(int source, int destination, T color) const {
		// if source node is not player's color, skip searching
		if (get_node_value(source / size, source % size) != color) {
			return false;
		}
		std::vector<bool> visited(std::vector<bool>(size * size, false));
		std::priority_queue<int, std::vector<int>, std::less<int>> queue;

		queue.push(source);

		while (!queue.empty()) {
			auto u = queue.top();
			queue.pop();

			if (u == destination) {
				return true;
			}
			visited[u] = true;

			for (auto v : neighbors(u, color)) {
				if (!visited[v]) {
					queue.push(v);
				}
			}
		}
		return false;
	}

	constexpr bool is_valid_coordinate(int x, int y) {
		return x >= 0 && x < size && y >= 0 && y < size;
	}

	bool has_any(T value) const {
		for (const auto& node : nodes) {
			if (node == value) {
				return true;
			}
		}
		return false;
	}

private:
	int              size;
	std::vector<T>   nodes;
};

class HexBoard;

struct Player {
	Player(const std::string& name, node value)
		: name(name), value(value)
	{
	}

	Player(const Player& copy)
		: name(copy.name), value(copy.value)
	{
	}

	virtual ~Player() {}
	virtual void do_move(const HexBoard& game, int& x, int& y) = 0;
	virtual void do_draw(const HexBoard& game) = 0;

	const std::string   name;
	const node          value;
};

class HexBoard {
public:
	HexBoard(int size)
		: _graph(size, node::empty)
		, _active_player(true) // blue player is always 1
		, _players()
	{
	}

	HexBoard(const HexBoard& copy)
		: _graph(copy._graph)
		, _active_player(copy._active_player)
	{
		_players[0] = copy._players[0];
		_players[1] = copy._players[1];
	}

	~HexBoard() {
		//delete _players[1];
		//delete _players[0];
	}

	void add_players(Player* p1, Player* p2) {
		_players[0] = p1;
		_players[1] = p2;
	}

	inline int size() const { return _graph.number_of_vertices(); }

	void draw() const {
		for (int i = 0; i < size(); ++i) {
			std::cout << "   " << static_cast<char>(i + 'a');
		}
		std::cout << "\n";

		// i is vertical, j is horizontal
		for (int n = 0, i = 0; n < size() * 2; ++n, ++i) {
			std::cout << std::string(n++, ' ');
			std::cout << color::white << std::setw(2) << i + 1 << " ";

			for (int j = 0; j < size(); ++j) {
				std::cout << _graph.get_node_value(i, j);
				if (j < size() - 1) {
					// very first and very last line
					if (n == 1 || n == size() * 2 - 1) {
						std::cout << color::red;
					}
					std::cout << " - " << color::white;
				}
			}
			std::cout << " " << color::white << i + 1 << "\n   ";
			std::cout << std::string(n, ' ');

			// stop drawing 2nd part of last line
			// n  -   -   -   -   n        <---- 1st part of line
			//   \ / \ / \ / \ / \         <---- 2nd part of line
			//n+1 - - - -   -   -   n+1    <---- 1st part of line
			if (2 * size() - n == 1) {
				break;
			}
			std::cout << color::blue << "\\" << color::white << " / ";

			for (int j = 1; j < size() - 1; ++j) {
				std::cout << "\\ / ";
			}
			std::cout << color::blue << "\\\n";
		}
		// move cursor 1 back
		std::cout << "\b";

		for (int i = 0; i < size(); ++i) {
			std::cout << static_cast<char>(i + 'a') << "   ";
		}
		std::cout << "\n\n";
	}

	bool check_red(bool print=false) const {
		// checking each pair from top to bottom:
		// a1 -> an, a1 -> bn, a1 -> cn...
		// b1 -> an, b1 -> bn, b1 -> cn...
		// n1 -> an, n1 -> bn, n1 -> nn
		// where n is size of board (and translated to letter as first coordinate)
		for (int i = 0; i < size(); ++i) {
			for (int j = 0; j < size(); ++j) {
				if (_graph.is_connected(i, size() * size() - size() + j, _players[0]->value)) {
					if (print) {
						std::cout << _players[0]->name << " WINS!\n";
					}
					return true;
				}
			}
		}
		return false;
	}

	bool check_blue(bool print=false) const {
		// checking each pair from left to right
		// a1 -> n1, a1 -> n2, a1 -> nn...
		// a2 -> n1, a2 -> n2, a2 -> nn...
		// an -> n1, an -> n2, an -> nn...
		for (int i = 0; i < size(); ++i) {
			for (int j = 0; j < size(); ++j) {
				if (_graph.is_connected(i * size(), (j + 1) * size() - 1, _players[1]->value)) {
					if (print) {
						std::cout << _players[1]->name << " WINS!\n";
					}
					return true;
				}
			}
		}
		return false;
	}
	
	bool is_over() const { return check_red(true) || check_blue(true); }

	bool is_over(node player) const {
		if (player == node::red) {
			return check_red();
		} else if (player == node::blue) {
			return check_blue();
		} else {
			return false;
		}
	}

	void do_move() {
		while (true) {
			std::cout << _players[_active_player]->name << " - enter move (like a1): ";

			int x, y;

			_players[_active_player]->do_move(*this, x, y);

			if (is_valid_move(x, y)) {
				_graph.set_node_value(x, y, _players[_active_player]->value);
				_active_player = !_active_player;    // switch players
				break;
			}
		}
	}

	void do_draw() const { _players[_active_player]->do_draw(*this); }

	bool is_valid_move(int x, int y) const {
		// out of range
		if (x < 0 || x > size() || y < 0 || y > size()) {
			return false;
		// occupied cell
		} else if (_graph.get_node_value(x, y) != node::empty) {
			return false;
		}
		return true;
	}

	bool has_any() const { return _graph.has_any(node::empty); }
	Player* next_player() { return _players[_active_player = !_active_player]; }
	void set_node_value(int x, int y, node value) { _graph.set_node_value(x, y, value); }

private:
	Graph<node>   _graph;
	bool          _active_player;
	Player*       _players[2];
};


template <class T>
T ranged_rand(T min, T max)  {
		static auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		static std::default_random_engine  gen(now);
		std::uniform_int_distribution<T>   dist(min, max);

		return dist(gen);
}

struct HumanPlayer : public Player {
	HumanPlayer(const std::string& name, node value)
		: Player(name, value)
	{
	}

	void do_move(const HexBoard& game, int& x, int& y) {
		char c;

		std::cin >> c >> x;

		x -= 1;
		y = static_cast<int>(c - 'a');
	}

	void do_draw(const HexBoard& game) { game.draw(); }
};

struct ComputerPlayer : public Player {
	ComputerPlayer(const std::string& name, node value)
		: Player(name, value)
	{
	}

	void do_move(const HexBoard& game, int& x, int& y) {
		make_move(game, x, y);
		std::cout << static_cast<char>(y + 'a') << x + 1 << "\n";
	}

	void do_draw(const HexBoard& game) { return; }

protected:
	virtual void make_move(const HexBoard& game, int& x, int& y) {}
};

struct RandomComputerPlayer : public ComputerPlayer {
	RandomComputerPlayer(const std::string& name, node value)
		: ComputerPlayer(name, value)
	{
	}

protected:
	void make_move(const HexBoard& game, int& x, int& y) {
		do {
			x = ranged_rand(0, game.size() - 1);
			y = ranged_rand(0, game.size() - 1);
		} while (!game.is_valid_move(x, y));
	}
};

struct MonteCarloComputerPlayer : public ComputerPlayer {
	MonteCarloComputerPlayer(const std::string& name, node value)
		: ComputerPlayer(name, value)
	{
	}

protected:
	void make_move(const HexBoard& game, int& x, int& y) {
		auto size = game.size();
		std::vector<double> probabilities(size * size, 0);

		for (int i = 0; i < size; ++i) {
			for (int j = 0; j < size; ++j) {
				if (game.is_valid_move(i, j)) {
					probabilities[i * size + j] = do_monte_carlo(game, i, j);
				}
			}
		}

		double win_probability = 0;
		for (std::size_t i = 0; i < probabilities.size(); ++i) {
			if (probabilities[i] > win_probability) {
				win_probability = probabilities[i];
				x = i / size;
				y = i % size;
			}
		}
	}

private:
	const int TRIALS = 1000;

	double do_monte_carlo(const HexBoard& game, int x, int y) {
		int wins = 0;
		for (int i = 0; i < TRIALS; ++i) {
			HexBoard copy(game);
			copy.set_node_value(x, y, value);
			fill_board(copy);
			if (copy.is_over(value)) {
				++wins;
			}
		}
		return static_cast<double>(wins) / TRIALS;
	}

	void fill_board(HexBoard& game) {
		int x, y;
		while (game.has_any()) {
			do {
				x = ranged_rand(0, game.size() - 1);
				y = ranged_rand(0, game.size() - 1);
			} while (!game.is_valid_move(x, y));
			game.set_node_value(x, y, game.next_player()->value);
		}
	}
};

void intro() {
	std::string title = 
	"     .__                   \n"\
	"     |  |__   ____ ___  ___\n"\
	"     |  |  \\_/ __ \\\\  \\/  /\n"\
	"     |   Y  \\  ___/ >    < \n"\
	"     |___|  /\\___  >__/\\_ \\\n"\
	"          \\/     \\/      \\/\n";
	
	std::cout << color::red << title << color::white << "\n\n";
}

// if you want colors, add -DCOLORS parameter
// g++ -Wall -Werror -std=c++11 -o hex hex.cpp
int main() {

	intro();

	int size;
	do {
		std::cout << "Enter board size (between " << BOARD_MIN_SIZE << " and " << BOARD_MAX_SIZE << "): ";
		std::cin >> size;
	} while (size < BOARD_MIN_SIZE || size > BOARD_MAX_SIZE);

	HexBoard game(size);

	int game_mode;
	do {
		std::cout << "Choose mode:\n";
		std::cout << "1 - player vs player\n";
		std::cout << "2 - player vs computer (random)\n";
		std::cout << "3 - player vs computer (monte carlo)\n";
		std::cout << "4 - computer (random) vs computer (random)\n";
		std::cout << "5 - computer (random) vs computer (monte carlo)\n";
		std::cout << "6 - computer (monte carlo) vs computer (monte carlo)\n";
		std::cout << "> ";

		std::cin >> game_mode;

	} while (game_mode < 1 || game_mode > 6);

	Player* players[2];
	switch (game_mode) {
		case 1:
			std::cout << "blue goes first\n";
			players[0] = new HumanPlayer("red", node::red);
			players[1] = new HumanPlayer("blue", node::blue);
			break;
		case 2:
			do {
				std::cout << "Choose your color (red or blue, blue goes first): ";

				std::string color;
				std::cin >> color;

				if (color == "red") {
					players[0] = new HumanPlayer("red", node::red);
					players[1] = new RandomComputerPlayer("blue-random", node::blue);
					break;
				} else if (color == "blue") {
					players[0] = new RandomComputerPlayer("red-random", node::red);
					players[1] = new HumanPlayer("blue", node::blue);
					break;
				}
			} while (true);
			break;
		case 3:
			do {
				std::cout << "Choose your color (red or blue, blue goes first): ";

				std::string color;
				std::cin >> color;

				if (color == "red") {
					players[0] = new HumanPlayer("red", node::red);
					players[1] = new MonteCarloComputerPlayer("blue-MC", node::blue);
					break;
				} else if (color == "blue") {
					players[0] = new MonteCarloComputerPlayer("red-MC", node::red);
					players[1] = new HumanPlayer("blue", node::blue);
					break;
				}
			} while (true);
			break;
		case 4:
			players[0] = new RandomComputerPlayer("red-random", node::red);
			players[1] = new RandomComputerPlayer("blue-random", node::blue);
			break;
		case 5:
			if (ranged_rand(0, 1)) {
				players[0] = new RandomComputerPlayer("red-random", node::red);
				players[1] = new MonteCarloComputerPlayer("blue-MC", node::blue);
			} else {
				players[0] = new MonteCarloComputerPlayer("red-MC", node::red);
				players[1] = new RandomComputerPlayer("blue-random", node::blue);
			}
			break;
		case 6:
			players[0] = new MonteCarloComputerPlayer("red-MC", node::red);
			players[1] = new MonteCarloComputerPlayer("blue-MC", node::blue);
			break;
	}

	game.add_players(players[0], players[1]);

	while (true) {
		if (game.is_over()) {
			game.draw();
			break;
		}
		if (game_mode >= 4 && game_mode <= 6) {
			game.draw();
		} else {
			game.do_draw();
		}
		game.do_move();
	}

	return 0;
}

