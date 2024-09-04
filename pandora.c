#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 600
#define CELL_SIZE 10
#define GRID_WIDTH (WIDTH / CELL_SIZE)
#define GRID_HEIGHT (HEIGHT / CELL_SIZE)
#define MAX_START_ALIVE_CELLS 100
#define MUTATION_RATE 0.001
#define INITIAL_POPULATION_RATIO 0.1
#define INITIAL_ENERGY 50
#define REPRODUCTION_THRESHOLD 150
#define REPRODUCTION_ENERGY 75
#define ENERGY_CONSUMPTION 1
#define WARRIOR_THRESHOLD 200
#define WARRIOR_CHANCE 0.00001
#define WARRIOR_DAMAGE 25
#define WARRIOR_RANGE 2
#define ENERGY_GAIN_FROM_ATTACK 15  // New constant
#define REGULAR_DAMAGE 15
#define MUTATED_DAMAGE 120
#define REGULAR_ATTACK_RANGE 1
#define MUTATED_ATTACK_RANGE 2
#define REGULAR_ENERGY_GAIN 10
#define MUTATED_ENERGY_GAIN 12

#define SIM_SPEED 50
#define MAX_STAGNANT_CYCLES 5  // Add this if it's not already defined

typedef enum {
    STATE_DEAD,
    STATE_ALIVE,
    STATE_MUTATED,
    STATE_WARRIOR,
    STATE_MAX
} CellState;

typedef struct {
    CellState state;
    int age;
    int energy;
    bool has_moved;
    int attack_cooldown;
    float dx;  // Direction and momentum on x-axis
    float dy;  // Direction and momentum on y-axis
    int stagnant_cycles;  // New field to track cycles without movement
} Cell;

typedef struct {
    int alive_count;
    int dead_count;
    int mutated_count;
    int warrior_count;
    int cycle_count;
} Statistics;

void initialize_grid(Cell grid[GRID_HEIGHT][GRID_WIDTH]) {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (rand() < INITIAL_POPULATION_RATIO * RAND_MAX) {
                grid[y][x].state = STATE_ALIVE;
                grid[y][x].energy = INITIAL_ENERGY;
            } else {
                grid[y][x].state = STATE_DEAD;
                grid[y][x].energy = 0;
            }
            grid[y][x].age = 0;
            grid[y][x].has_moved = false;
            grid[y][x].attack_cooldown = 0;
            grid[y][x].dx = ((float)rand() / RAND_MAX) * 2 - 1;
            grid[y][x].dy = ((float)rand() / RAND_MAX) * 2 - 1;
            grid[y][x].stagnant_cycles = 0;
        }
    }
}

CellState get_next_state(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y) {
    int neighbors = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                if (grid[ny][nx].state != STATE_DEAD) neighbors++;
            }
        }
    }

    if (grid[y][x].state == STATE_ALIVE || grid[y][x].state == STATE_MUTATED || grid[y][x].state == STATE_WARRIOR) {
        grid[y][x].energy -= ENERGY_CONSUMPTION;
        if (grid[y][x].energy <= 0) return STATE_DEAD;
        if (neighbors < 2 || neighbors > 3) {
            grid[y][x].energy -= ENERGY_CONSUMPTION * 2;
            if (grid[y][x].energy <= 0) return STATE_DEAD;
        }
        if (grid[y][x].energy > WARRIOR_THRESHOLD && rand() < WARRIOR_CHANCE * RAND_MAX) {
            return STATE_WARRIOR;
        }
        if (rand() < MUTATION_RATE * RAND_MAX) return STATE_MUTATED;
        return grid[y][x].state;
    } else {
        if (neighbors == 3) {
            return (rand() < MUTATION_RATE * RAND_MAX) ? STATE_MUTATED : STATE_ALIVE;
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
            Cell temp = grid[ny][nx];
            grid[ny][nx] = grid[y][x];
            grid[y][x] = temp;
            grid[ny][nx].has_moved = true; // Mark as moved
        }
    }
}

void warrior_action(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y) {
    if (grid[y][x].attack_cooldown > 0) {
        grid[y][x].attack_cooldown--;
        return;
    }

    for (int dy = -WARRIOR_RANGE; dy <= WARRIOR_RANGE; dy++) {
        for (int dx = -WARRIOR_RANGE; dx <= WARRIOR_RANGE; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                if (grid[ny][nx].state != STATE_DEAD) {
                    // Attack any non-dead cell, including other warriors
                    grid[ny][nx].energy -= WARRIOR_DAMAGE;
                    grid[y][x].energy += ENERGY_GAIN_FROM_ATTACK;
                    grid[y][x].attack_cooldown = 5; // Set cooldown
                    
                    // If the attacked cell's energy drops to 0 or below, it dies
                    if (grid[ny][nx].energy <= 0) {
                        grid[ny][nx].state = STATE_DEAD;
                        grid[ny][nx].energy = 0;
                    }
                    
                    return; // Exit after one attack
                }
            }
        }
    }
}

void cell_attack(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y, int damage, int range, int energy_gain) {
    if (grid[y][x].attack_cooldown > 0) {
        grid[y][x].attack_cooldown--;
        return;
    }

    for (int dy = -range; dy <= range; dy++) {
        for (int dx = -range; dx <= range; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                if (grid[ny][nx].state != STATE_DEAD && grid[ny][nx].state != grid[y][x].state) {
                    grid[ny][nx].energy -= damage;
                    grid[y][x].energy += energy_gain;
                    grid[y][x].attack_cooldown = 3; // Shorter cooldown than warriors
                    
                    if (grid[ny][nx].energy <= 0) {
                        grid[ny][nx].state = STATE_DEAD;
                        grid[ny][nx].energy = 0;
                    }
                    
                    return; // Exit after one attack
                }
            }
        }
    }
}

void advanced_move(Cell grid[GRID_HEIGHT][GRID_WIDTH], int x, int y) {
    if (grid[y][x].has_moved || grid[y][x].state == STATE_DEAD) return;

    float speed = (float)grid[y][x].energy / 50.0f;  // Increased base speed
    if (speed > 2.0f) speed = 2.0f;  // Increased max speed

    // Adjust direction based on surroundings
    float attract_x = 0, attract_y = 0;
    float repel_x = 0, repel_y = 0;
    int neighbors = 0;

    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                if (grid[ny][nx].state == STATE_DEAD) {
                    // Attraction to dead cells (food)
                    attract_x += dx / (float)(dx*dx + dy*dy);
                    attract_y += dy / (float)(dx*dx + dy*dy);
                } else if (grid[ny][nx].state != STATE_DEAD) {
                    // Repulsion from other living cells
                    repel_x -= dx / (float)(dx*dx + dy*dy);
                    repel_y -= dy / (float)(dx*dx + dy*dy);
                    neighbors++;
                }
            }
        }
    }

    // Update direction based on attraction, repulsion, and previous momentum
    grid[y][x].dx = grid[y][x].dx * 0.7f + (attract_x - repel_x) * 0.3f;
    grid[y][x].dy = grid[y][x].dy * 0.7f + (attract_y - repel_y) * 0.3f;

    // Add random movement
    grid[y][x].dx += ((float)rand() / RAND_MAX) * 0.4f - 0.2f;
    grid[y][x].dy += ((float)rand() / RAND_MAX) * 0.4f - 0.2f;

    // Normalize direction
    float length = sqrt(grid[y][x].dx * grid[y][x].dx + grid[y][x].dy * grid[y][x].dy);
    if (length > 0) {
        grid[y][x].dx /= length;
        grid[y][x].dy /= length;
    }

    // Calculate new position
    int new_x = x + (int)(grid[y][x].dx * speed);
    int new_y = y + (int)(grid[y][x].dy * speed);

    // Ensure new position is within grid bounds
    new_x = (new_x < 0) ? 0 : (new_x >= GRID_WIDTH ? GRID_WIDTH - 1 : new_x);
    new_y = (new_y < 0) ? 0 : (new_y >= GRID_HEIGHT ? GRID_HEIGHT - 1 : new_y);

    // Move if the new cell is dead or with a high probability
    if (grid[new_y][new_x].state == STATE_DEAD || (rand() < 0.7 * RAND_MAX)) {
        Cell temp = grid[new_y][new_x];
        grid[new_y][new_x] = grid[y][x];
        grid[y][x] = temp;
        grid[new_y][new_x].has_moved = true;
        grid[new_y][new_x].stagnant_cycles = 0;  // Reset stagnant cycles counter
    } else {
        grid[y][x].stagnant_cycles++;  // Increment stagnant cycles if the cell didn't move
    }
}

void update_grid(Cell grid[GRID_HEIGHT][GRID_WIDTH], Statistics *stats) {
    Cell new_grid[GRID_HEIGHT][GRID_WIDTH];
    stats->alive_count = 0;
    stats->dead_count = 0;
    stats->mutated_count = 0;
    stats->warrior_count = 0;

    // Reset has_moved flag for all cells
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].has_moved = false;
        }
    }

    // First, attempt movement for all cells (SURVIVE)
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].state != STATE_DEAD) {
                advanced_move(grid, x, y);
                
                // Kill cells that haven't moved in MAX_STAGNANT_CYCLES
                if (grid[y][x].stagnant_cycles >= MAX_STAGNANT_CYCLES) {
                    grid[y][x].state = STATE_DEAD;
                    grid[y][x].energy = 0;
                    grid[y][x].stagnant_cycles = 0;
                }
            }
        }
    }

    // Then, handle reproduction and fighting
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].state != STATE_DEAD) {
                // REPRODUCE (if energy is sufficient)
                if (grid[y][x].energy > REPRODUCTION_THRESHOLD) {
                    bool reproduced = false;
                    for (int dy = -1; dy <= 1 && !reproduced; dy++) {
                        for (int dx = -1; dx <= 1 && !reproduced; dx++) {
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && grid[ny][nx].state == STATE_DEAD) {
                                grid[y][x].energy -= REPRODUCTION_ENERGY;
                                grid[ny][nx].state = (grid[y][x].state == STATE_WARRIOR && rand() >= WARRIOR_CHANCE * RAND_MAX) ? STATE_ALIVE : grid[y][x].state;
                                grid[ny][nx].energy = REPRODUCTION_ENERGY;
                                grid[ny][nx].age = 0;
                                grid[ny][nx].stagnant_cycles = 0;
                                reproduced = true;
                            }
                        }
                    }
                }
                // FIGHT (if didn't reproduce)
                if (grid[y][x].energy <= REPRODUCTION_THRESHOLD) {
                    switch (grid[y][x].state) {
                        case STATE_WARRIOR:
                            warrior_action(grid, x, y);
                            break;
                        case STATE_ALIVE:
                            cell_attack(grid, x, y, REGULAR_DAMAGE, REGULAR_ATTACK_RANGE, REGULAR_ENERGY_GAIN);
                            break;
                        case STATE_MUTATED:
                            cell_attack(grid, x, y, MUTATED_DAMAGE, MUTATED_ATTACK_RANGE, MUTATED_ENERGY_GAIN);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    // Update cell states and handle spontaneous warrior creation
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            new_grid[y][x] = grid[y][x];
            new_grid[y][x].state = get_next_state(grid, x, y);
            new_grid[y][x].has_moved = false; // Reset movement flag for next cycle

            if (new_grid[y][x].state != STATE_DEAD) {
                new_grid[y][x].age++;
                
                // Use WARRIOR_CHANCE for spontaneous warrior creation
                if (new_grid[y][x].state != STATE_WARRIOR && rand() < WARRIOR_CHANCE * RAND_MAX) {
                    new_grid[y][x].state = STATE_WARRIOR;
                    new_grid[y][x].energy = WARRIOR_THRESHOLD;
                }
            } else {
                new_grid[y][x].age = 0;
                new_grid[y][x].energy = 0;
                new_grid[y][x].stagnant_cycles = 0;  // Reset stagnant cycles for dead cells
            }

            // Update statistics
            switch (new_grid[y][x].state) {
                case STATE_ALIVE: stats->alive_count++; break;
                case STATE_DEAD: stats->dead_count++; break;
                case STATE_MUTATED: stats->mutated_count++; break;
                case STATE_WARRIOR: stats->warrior_count++; break;
                default: break;
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
            int intensity = grid[y][x].energy * 2;
            switch (grid[y][x].state) {
                case STATE_DEAD:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    break;
                case STATE_ALIVE:
                    SDL_SetRenderDrawColor(renderer, 0, intensity, 0, 255);
                    break;
                case STATE_MUTATED:
                    SDL_SetRenderDrawColor(renderer, intensity, 0, 0, 255);
                    break;
                case STATE_WARRIOR:
                    SDL_SetRenderDrawColor(renderer, 0, 0, intensity, 255);
                    break;
                case STATE_MAX:
                default:
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    break;
            }
            SDL_Rect rect;
            rect.x = x * CELL_SIZE;
            rect.y = y * CELL_SIZE;
            rect.w = CELL_SIZE;
            rect.h = CELL_SIZE;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL2: %s\n", SDL_GetError());
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

    Cell grid[GRID_HEIGHT][GRID_WIDTH];
    Statistics stats = {0, 0, 0, 0, 0};

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
        
        // Print stats on a single line in the terminal and move cursor to next line
        printf("\rAlive: %d | Dead: %d | Mutated: %d | Warriors: %d | Cycle: %d", 
               stats.alive_count, stats.dead_count, stats.mutated_count, stats.warrior_count, stats.cycle_count);
        fflush(stdout); // Ensure the output is displayed immediately

        SDL_Delay(SIM_SPEED); // Adjust simulation speed
    }

    printf("\n"); // Print a newline at the end to move to the next line in the terminal

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
