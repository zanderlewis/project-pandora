#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 800
#define HEIGHT 600
#define CELL_SIZE 10
#define GRID_WIDTH (WIDTH / CELL_SIZE)
#define GRID_HEIGHT (HEIGHT / CELL_SIZE)

// Define possible cell states with associated colors
typedef enum {
    STATE_DEAD,
    STATE_ALIVE,
    STATE_MUTATED,
    STATE_MAX
} CellState;

typedef struct {
    CellState state;
    int age; // Track cell age for dynamic patterns
    bool has_moved; // Flag to track if the cell has moved this cycle
} Cell;

typedef struct {
    int alive_count;
    int dead_count;
    int mutated_count;
    int cycle_count;
} Statistics;

void initialize_grid(Cell grid[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].state = (rand() % 4 == 0) ? STATE_ALIVE : STATE_DEAD;
            grid[y][x].age = 0;
            grid[y][x].has_moved = false;
        }
    }
}

CellState get_next_state(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y) {
    int alive_neighbors = 0;
    int mutated_neighbors = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue; // Skip the cell itself
            int ny = y + dy;
            int nx = x + dx;
            if (ny >= 0 && ny < GRID_HEIGHT && nx >= 0 && nx < GRID_WIDTH) {
                if (grid[ny][nx].state == STATE_ALIVE) {
                    alive_neighbors++;
                } else if (grid[ny][nx].state == STATE_MUTATED) {
                    mutated_neighbors++;
                }
            }
        }
    }

    if (grid[y][x].state == STATE_ALIVE) {
        if (alive_neighbors < 2 || alive_neighbors > 3) {
            return STATE_DEAD;
        } else if (mutated_neighbors > 1) {
            return STATE_MUTATED;
        }
        return STATE_ALIVE;
    } else if (grid[y][x].state == STATE_MUTATED) {
        if (alive_neighbors == 3) {
            return STATE_ALIVE;
        }
        return STATE_MUTATED;
    } else {
        if (alive_neighbors == 3) {
            return STATE_ALIVE;
        }
        return STATE_DEAD;
    }
}

void attempt_move(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y) {
    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // Up, Down, Left, Right
    int random_dir = rand() % 4;

    int nx = x + directions[random_dir][0];
    int ny = y + directions[random_dir][1];

    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
        if (grid[ny][nx].state == STATE_DEAD) {
            // Swap the cells
            grid[ny][nx] = grid[y][x];
            grid[y][x].state = STATE_DEAD;
            grid[ny][nx].has_moved = true; // Mark as moved
        }
    }
}

void update_grid(Cell grid[GRID_HEIGHT][GRID_WIDTH], Statistics *stats) {
    Cell new_grid[GRID_HEIGHT][GRID_WIDTH];
    stats->alive_count = 0;
    stats->dead_count = 0;
    stats->mutated_count = 0;

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            new_grid[y][x].state = get_next_state(grid, x, y);
            new_grid[y][x].age = (new_grid[y][x].state == grid[y][x].state) ? grid[y][x].age + 1 : 0;
            new_grid[y][x].has_moved = false; // Reset movement flag

            // Move the cell if it hasn't moved yet
            if (grid[y][x].state != STATE_DEAD && !grid[y][x].has_moved) {
                attempt_move(grid, x, y);
            }

            // Update statistics
            if (new_grid[y][x].state == STATE_ALIVE) {
                stats->alive_count++;
            } else if (new_grid[y][x].state == STATE_DEAD) {
                stats->dead_count++;
            } else if (new_grid[y][x].state == STATE_MUTATED) {
                stats->mutated_count++;
            }
        }
    }

    // Copy the new grid back into the original grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = new_grid[y][x];
        }
    }

    stats->cycle_count++;
}

void draw_grid(SDL_Renderer *renderer, Cell grid[GRID_HEIGHT][GRID_WIDTH]) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            switch (grid[y][x].state) {
                case STATE_DEAD:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    break;
                case STATE_ALIVE:
                    // Apply a pulsating effect based on cell age
                    SDL_SetRenderDrawColor(renderer, 255, 255 - (grid[y][x].age % 255), 255 - (grid[y][x].age % 255), 255);
                    break;
                case STATE_MUTATED:
                    // Apply a dynamic color transition based on cell age
                    SDL_SetRenderDrawColor(renderer, 255, grid[y][x].age % 255, grid[y][x].age % 255, 255);
                    break;
                default:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    break;
            }
            SDL_Rect cell_rect = { x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
            SDL_RenderFillRect(renderer, &cell_rect);
        }
    }

    SDL_RenderPresent(renderer);
}

void draw_stats(SDL_Renderer *renderer, TTF_Font *font, Statistics stats) {
    char text[128];
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect text_rect;

    snprintf(text, sizeof(text), "Alive: %d | Dead: %d | Mutated: %d | Cycle: %d", stats.alive_count, stats.dead_count, stats.mutated_count, stats.cycle_count);

    surface = TTF_RenderText_Solid(font, text, white);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    text_rect.x = 10;
    text_rect.y = HEIGHT - 30;
    text_rect.w = surface->w;
    text_rect.h = surface->h;

    SDL_RenderCopy(renderer, texture, NULL, &text_rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL2: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL2_ttf: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Project Pandora",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("fonts/roboto.ttf", 16);
    if (!font) {
        fprintf(stderr, "Could not load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Cell grid[GRID_HEIGHT][GRID_WIDTH];
    Statistics stats = {0, 0, 0, 0};

    initialize_grid(grid);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        update_grid(grid, &stats);
        draw_grid(renderer, grid);
        draw_stats(renderer, font, stats);

        SDL_Delay(100); // Adjust simulation speed
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();

    return 0;
}
