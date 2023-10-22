#include <ncurses.h>

#include <cstdlib>
#include <vector>
#include <cstdlib>
#include <ctime>


const char SNAKE_BODY = '#';
const int BOARD_SIZE = 20;



// ========================================================================== //
// == Utils ================================================================= //
// ========================================================================== //
struct Vec2 {
    int x;
    int y;
};


// ========================================================================== //
// == Snake ================================================================= //
// ========================================================================== //
enum class Facing { North, East, South, West };

struct Snake {
    Facing facing;
    std::vector<Vec2> segments;
};


// ========================================================================== //
// == Game ================================================================== //
// ========================================================================== //
enum class GameState {
    Menu,
    Game,
    Dead,
};

typedef struct Game {
    GameState state;
    int width;
    int height;
    unsigned int score;
    bool is_running;
    Snake snake;
    Vec2 food;
} Game;

// Systems
void game_spawn_food(Game& game) {
    const int spawn_radius = std::min(30, std::min(game.width, game.height));
    const int spawn_empty_width = (game.width - spawn_radius) / 2;
    const int spawn_empty_height = (game.height - spawn_radius) / 2;
    game.food = {
	.x = spawn_empty_width + (rand() % spawn_radius),
	.y = spawn_empty_height + (rand() % spawn_radius),
    };

    // don't spawn on snake
    if (mvinch(game.food.y, game.food.x * 2) == SNAKE_BODY) {
	game_spawn_food(game);
    }
}

void game_reset(Game& game) {
    game.score = 0;
    game.snake.facing = Facing::East;
    game.snake.segments.clear();
    game.snake.segments.push_back({ 10, 10 });

    game_spawn_food(game);
}

void get_input(Game& game) {
    int input = getch();
    if (input == '\n') {
	game.state = GameState::Game;
	game_reset(game);
    } else if (input == 'q') {
	game.is_running = false;
    }
}

void menu_loop(Game& game) {
    get_input(game);

    clear();

    move(game.height / 2, game.width - 6);
    printw("S N A K E");
    move(game.height / 2 + 2, game.width - 12);
    printw("Press enter to start...");

    refresh();
}

void dead_loop(Game& game) {
    get_input(game);

    clear();

    move(game.height / 2, game.width - 6);
    printw("Game Over");

    refresh();
}

Game game_init() {
    Game game;

    // setup ncurses
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, true);
    curs_set(0);
    start_color();
    halfdelay(1);
    init_color(COLOR_GREEN, 0, 1000, 400);
    init_color(COLOR_RED, 1000, 200, 0);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    // screen size
    getmaxyx(stdscr, game.height, game.width);
    game.width /= 2;

    // setup game stuff
    game.state = GameState::Menu;
    game.is_running = true;

    game_reset(game);

    return game;
}

void game_deinit(const Game& game) {
    endwin();
}

void game_input(Game& game) {
    int input = getch();

    Snake& snake = game.snake;
    switch (input) {
	case KEY_UP:    if (snake.facing != Facing::South) snake.facing = Facing::North; break;
	case KEY_RIGHT: if (snake.facing != Facing::West)  snake.facing = Facing::East;  break;
	case KEY_DOWN:  if (snake.facing != Facing::North) snake.facing = Facing::South; break;
	case KEY_LEFT:  if (snake.facing != Facing::East)  snake.facing = Facing::West;  break;
    }
}

void game_update(Game& game) {
    Snake& snake = game.snake;

    // move the body up one
    for (size_t i = snake.segments.size() - 1; i > 0; i -= 1) {
	Vec2& segment = snake.segments.at(i);
	const Vec2& parent = snake.segments.at(i - 1);
	segment = parent;
    }

    // move head
    Vec2& head = snake.segments[0];
    switch (snake.facing) {
	case Facing::North: head.y -= 1; break;
	case Facing::East:  head.x += 1; break;
	case Facing::South: head.y += 1; break;
	case Facing::West:  head.x -= 1; break;
    }

    // if hit food
    if (head.x == game.food.x && head.y == game.food.y) {
	game.score += 3;
	snake.segments.push_back(snake.segments.back());
	snake.segments.push_back(snake.segments.back());
	snake.segments.push_back(snake.segments.back());
	game_spawn_food(game);
    }

    // if hit body
    char at_head = mvinch(head.y, head.x * 2);
    if (at_head == SNAKE_BODY) {
	game.state = GameState::Dead;
    }

    // wrap around board
    if (head.x < 0) {
	head.x = game.width - 1;
    } else if (head.x >= game.width) {
	head.x = 0;
    }

    if (head.y < 0) {
	head.y = game.height - 1;
    } else if (head.y >= game.height) {
	head.y = 0;
    }
}

void game_draw(const Game& game) {
    clear();

    attron(A_BOLD);

    // draw snake
    const Snake& snake = game.snake;
    attron(COLOR_PAIR(1));
    for (Vec2 segment : snake.segments) {
	move(segment.y, segment.x * 2);
	addch(SNAKE_BODY);	
    }
    move(snake.segments[0].y, snake.segments[0].x * 2);
    switch (snake.facing) {
	case Facing::North: addch('^'); break;
	case Facing::East:  addch('>'); break;
	case Facing::South: addch('v'); break;
	case Facing::West:  addch('<'); break;
    }
    attroff(COLOR_PAIR(1));

    // draw food
    attron(COLOR_PAIR(3));
    move(game.food.y, game.food.x * 2);
    addch('@');
    attroff(COLOR_PAIR(3));

    attroff(A_BOLD);

    // draw score
    attron(COLOR_PAIR(2));
    move (1, game.width - 1);
    printw("\\- %.3d -/", game.score);
    attroff(COLOR_PAIR(2));

    refresh();
}


// ========================================================================== //
// == Entry ================================================================= //
// ========================================================================== //
int main() {
    std::srand(std::time(nullptr));

    Game game = game_init();

    while (game.is_running) {
	switch (game.state) {
	    case GameState::Menu:
		menu_loop(game);
		break;
	    case GameState::Game:
		game_input(game);
		game_update(game);
		game_draw(game);
		break;
	    case GameState::Dead:
		dead_loop(game);
		break;
	}
    }

    game_deinit(game);

    return EXIT_SUCCESS;
}
