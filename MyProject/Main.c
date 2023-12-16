#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include<stdbool.h>
#include<SDL_ttf.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#ifdef __EMSCRIPTEN__

#include<emscripten.h>

#endif

#define GRID_SIZE 4
#define CELL_SIZE1 120
#define CELL_SIZE 40

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600


// Button dimensions
const int BUTTON_WIDTH = 200;
const int BUTTON_HEIGHT = 100;

#define BLOCK_SIZE 20
#define NUM_BLOCKS_X (SCREEN_WIDTH / BLOCK_SIZE)
#define NUM_BLOCKS_Y (SCREEN_HEIGHT / BLOCK_SIZE)

#define MAZE_WIDTH 20
#define MAZE_HEIGHT 15
#define GOAL_X 11
#define GOAL_Y 5
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PLAYER_SIZE 50
#define COIN_SIZE 20
#define OBSTACLE_WIDTH_MIN 100
#define OBSTACLE_WIDTH_MAX 200
#define OBSTACLE_HEIGHT_MIN 100
#define OBSTACLE_HEIGHT_MAX 300
#define PLAYER_VELOCITY 7

#define GROUND_HEIGHT 50
#define DINO_WIDTH 100
#define DINO_HEIGHT 100
#define OBSTACLE_WIDTH 80
#define OBSTACLE_HEIGHT 70
#define GRAVITY 0.6
#define JUMP_FORCE 17
#define GRID_ROWS 8
#define GRID_COLS 12
#define JEWEL_SIZE 64
#define SCORE_INCREMENT 10

#define SPACESHIP_SIZE 60
#define PROJECTILE_SIZE 30
#define ASTEROID_SIZE 100
#define SPACESHIP_SPEED 5
#define PROJECTILE_SPEED 10
#define ASTEROID_SPEED 3

#define IMAGE_WIDTH 100
#define IMAGE_HEIGHT 100
#define MAX_CLICK_TIME 1000 // Maximum time in milliseconds to click the image

typedef struct {
    float x, y;
    float velocityY;
    SDL_Texture* texture;
} Dino;

typedef struct {
    float x, y;
    float velocityX;
    SDL_Texture* texture;
} Obstacle;

typedef struct {
    int x;
    int y;
    int color;
} Jewel;

typedef struct {
    int x;
    int y;
    SDL_Texture* texture;
} Image;

typedef struct {
    int x;
    int y;
    int shape[4][4];
} Tetromino;

typedef struct {
    SDL_Rect rect;
    int velocity;
} GameObject;

typedef struct {
    GameObject player;
    GameObject bullets[10];
    GameObject asteroids[10];
} GameState;


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Rect playerRect;
SDL_Rect coinRect;
SDL_Rect obstacleRect;
bool gameover4 = false;
int score4 = 0;

void initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow("Coin Dash", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Failed to create SDL renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    playerRect = (SDL_Rect){ 50, WINDOW_HEIGHT / 2 - PLAYER_SIZE / 2, PLAYER_SIZE, PLAYER_SIZE };
    coinRect = (SDL_Rect){ WINDOW_WIDTH - COIN_SIZE, WINDOW_HEIGHT / 2 - COIN_SIZE / 2, COIN_SIZE, COIN_SIZE };
    obstacleRect = (SDL_Rect){ WINDOW_WIDTH, 0, 0, 0 };
}

void generate_coin() {
    int randomY = rand() % (WINDOW_HEIGHT - COIN_SIZE);
    coinRect.y = randomY;
    coinRect.x = WINDOW_WIDTH;
}


bool initializeSDL77(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Sugar Crush Saga", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }

    return true;
}

SDL_Texture* loadJewelImage(SDL_Renderer* renderer, const char* imagePath) {
    SDL_Surface* surface = IMG_Load(imagePath);
    if (surface == NULL) {
        printf("Failed to load image %s! SDL_image Error: %s\n", imagePath, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Unable to create texture from %s! SDL Error: %s\n", imagePath, SDL_GetError());
    }

    SDL_FreeSurface(surface);

    return texture;
}

void generateJewels(Jewel grid[GRID_ROWS][GRID_COLS]) {
    int colorCount[4] = { 0 };

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int color = rand() % 4;
            grid[row][col].color = color;
            colorCount[color]++;
        }
    }

    for (int color = 0; color < 4; color++) {
        while (colorCount[color] >= 3) {
            for (int row = 0; row < GRID_ROWS; row++) {
                for (int col = 0; col < GRID_COLS; col++) {
                    if (grid[row][col].color == color) {
                        int newColor;
                        do {
                            newColor = rand() % 4;
                        } while (newColor == color);
                        grid[row][col].color = newColor;
                        colorCount[color]--;
                        colorCount[newColor]++;
                    }
                }
            }
        }
    }
}

void renderJewels(SDL_Renderer* renderer, SDL_Texture* jewelTextures[], Jewel grid[GRID_ROWS][GRID_COLS]) {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int color = grid[row][col].color;
            int x = col * JEWEL_SIZE;
            int y = row * JEWEL_SIZE;

            SDL_Rect destRect = { x, y, JEWEL_SIZE, JEWEL_SIZE };
            SDL_RenderCopy(renderer, jewelTextures[color], NULL, &destRect);
        }
    }
}

bool isValidPosition77(int row, int col) {
    return (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS);
}

void swapJewels(Jewel grid[GRID_ROWS][GRID_COLS], int row1, int col1, int row2, int col2) {
    Jewel temp = grid[row1][col1];
    grid[row1][col1] = grid[row2][col2];
    grid[row2][col2] = temp;
}

bool checkMatches(Jewel grid[GRID_ROWS][GRID_COLS]) {
    bool matchesFound = false;

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS - 2; col++) {
            int color = grid[row][col].color;
            if (color == grid[row][col + 1].color && color == grid[row][col + 2].color) {
                matchesFound = true;
                grid[row][col].color = -1;
                grid[row][col + 1].color = -1;
                grid[row][col + 2].color = -1;
            }
        }
    }

    for (int row = 0; row < GRID_ROWS - 2; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int color = grid[row][col].color;
            if (color == grid[row + 1][col].color && color == grid[row + 2][col].color) {
                matchesFound = true;
                grid[row][col].color = -1;
                grid[row + 1][col].color = -1;
                grid[row + 2][col].color = -1;
            }
        }
    }

    return matchesFound;
}

void removeAndShiftJewels(Jewel grid[GRID_ROWS][GRID_COLS], int* score) {
    for (int col = 0; col < GRID_COLS; col++) {
        int shift = 0;
        for (int row = GRID_ROWS - 1; row >= 0; row--) {
            if (grid[row][col].color != -1) {
                if (shift > 0) {
                    swapJewels(grid, row, col, row + shift, col);
                }
            }
            else {
                shift++;
            }
        }
        for (int row = 0; row < shift; row++) {
            int color = rand() % 4;
            grid[row][col].color = color;
        }
        *score += shift * SCORE_INCREMENT;
    }
}

void playCrush() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    if (!initializeSDL77(&window, &renderer)) {
        return 1;
    }

    srand(time(NULL));

    SDL_Texture* jewelTextures[4];
    jewelTextures[0] = loadJewelImage(renderer, "candy1.png");
    jewelTextures[1] = loadJewelImage(renderer, "candy2.png");
    jewelTextures[2] = loadJewelImage(renderer, "candy3.png");
    jewelTextures[3] = loadJewelImage(renderer, "candy4.png");

    Jewel grid[GRID_ROWS][GRID_COLS];

    generateJewels(grid);

    bool quit = false;
    SDL_Event event;
    int score = 0;
    int selectedRow = -1;
    int selectedCol = -1;

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                selectedRow = mouseY / JEWEL_SIZE;
                selectedCol = mouseX / JEWEL_SIZE;
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                int targetRow = mouseY / JEWEL_SIZE;
                int targetCol = mouseX / JEWEL_SIZE;

                if (isValidPosition77(selectedRow, selectedCol) && isValidPosition77(targetRow, targetCol)) {
                    if ((abs(targetRow - selectedRow) == 1 && targetCol == selectedCol) || (abs(targetCol - selectedCol) == 1 && targetRow == selectedRow)) {
                        swapJewels(grid, selectedRow, selectedCol, targetRow, targetCol);

                        if (checkMatches(grid)) {
                            removeAndShiftJewels(grid, &score);
                        }
                        else {
                            swapJewels(grid, selectedRow, selectedCol, targetRow, targetCol);
                        }
                    }
                }

                selectedRow = -1;
                selectedCol = -1;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (checkMatches(grid)) {
            removeAndShiftJewels(grid, &score);
        }

        renderJewels(renderer, jewelTextures, grid);

        SDL_RenderPresent(renderer);
    }

    for (int i = 0; i < 4; i++) {
        SDL_DestroyTexture(jewelTextures[i]);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

}

void generate_obstacle() {
    int randomWidth = rand() % (OBSTACLE_WIDTH_MAX - OBSTACLE_WIDTH_MIN + 1) + OBSTACLE_WIDTH_MIN;
    int randomHeight = rand() % (OBSTACLE_HEIGHT_MAX - OBSTACLE_HEIGHT_MIN + 1) + OBSTACLE_HEIGHT_MIN;
    int randomY = rand() % (WINDOW_HEIGHT - randomHeight);
    obstacleRect.x = WINDOW_WIDTH;
    obstacleRect.y = randomY;
    obstacleRect.w = randomWidth;
    obstacleRect.h = randomHeight;
}

bool check_collision(SDL_Rect rect1, SDL_Rect rect2) {
    if (rect1.x + rect1.w < rect2.x ||
        rect1.x > rect2.x + rect2.w ||
        rect1.y + rect1.h < rect2.y ||
        rect1.y > rect2.y + rect2.h) {
        return false;
    }

    return true;
}

void handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            gameover4 = true;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_UP:
                playerRect.y -= PLAYER_VELOCITY;
                break;
            case SDLK_DOWN:
                playerRect.y += PLAYER_VELOCITY;
                break;
            case SDLK_SPACE:
                // Jump or perform action
                // Implement your game logic here
                break;
            }
        }
    }
}

void update_game() {
    // Move the coin and obstacle to the left
    coinRect.x -= 5;
    obstacleRect.x -= 5;

    // Check collision with the player
    if (check_collision(playerRect, coinRect)) {
        // Increment score and generate a new coin
        generate_coin();
    }

    if (check_collision(playerRect, obstacleRect)) {
        // Game over if there is a collision with the obstacle
        gameover4 = true;
    }

    // Check if the obstacle has passed the screen
    if (obstacleRect.x + obstacleRect.w < 0) {
        generate_obstacle();
        score4++;
    }
    if (coinRect.x + coinRect.w < 0) {
        generate_coin();
    }
}

void render_game() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render the player
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &playerRect);

    // Render the coin
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderFillRect(renderer, &coinRect);

    // Render the obstacle
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &obstacleRect);

    SDL_RenderPresent(renderer);
}


int playerX = 0;
int playerY = 0;
int gameover1 = 0;

bool init1();
void begin();
bool loop();
void display_board();
void kill_resources();

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* surface;
SDL_Texture* texture;

int window_side = (GRID_SIZE * CELL_SIZE1) + 1;
TTF_Font* font;
int INIT_DISPLAY = true;

SDL_Color grid_background = { 30, 100, 119, 255 };
SDL_Color grid_line_color = { 15, 50, 60, 255 };
SDL_Color grid_cursor_ghost_color = { 15, 50, 60, 255 };
SDL_Color grid_cursor_color = { 255, 255, 255, 255 };

int game[GRID_SIZE][GRID_SIZE];


#ifdef __EMSCRIPTEN__

void mainloop() {
    loop();
}

#endif


bool init1() {
    /* Initialize resources */

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialize SDL: %s", SDL_GetError());
        return 0;
    }

    if (SDL_CreateWindowAndRenderer(window_side, window_side, 0, &window, &renderer) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Create window and renderer: %s", SDL_GetError());
        return 0;
    }

    SDL_SetWindowTitle(window, "2048");

    TTF_Init();

    return true;
}

void display_board() {
    if (font == NULL) {
        printf("Failed to load font!\n");
    }

    // find Max Index
    int _max_i = 0, _max_j = 0, _max = game[0][0], i, j;

    for (i = 0; i < GRID_SIZE; ++i) {
        for (j = 0; j < GRID_SIZE; ++j) {
            if (_max < game[i][j]) {
                _max = game[i][j];
                _max_i = i;
                _max_j = j;
            }
        }
    }

    for (i = 0; i < GRID_SIZE; ++i) {
        for (j = 0; j < GRID_SIZE; ++j) {
            if (!game[i][j]) continue;

            char* snum = calloc(6, sizeof(char));
            sprintf(snum, "%d", game[i][j]);

            SDL_Color Font_Color;

            if (i == _max_i && j == _max_j) {
                SDL_Rect grid_cursor_ghost = {
                    j * CELL_SIZE1,
                    i * CELL_SIZE1,
                    CELL_SIZE1,
                    CELL_SIZE1
                };
                Font_Color.r = 0;
                Font_Color.g = 255;
                Font_Color.b = 255;
                SDL_SetRenderDrawColor(renderer, grid_cursor_ghost_color.r,
                    grid_cursor_ghost_color.g,
                    grid_cursor_ghost_color.b,
                    grid_cursor_ghost_color.a);
                SDL_RenderFillRect(renderer, &grid_cursor_ghost);
            }
            else {
                Font_Color.r = 255;
                Font_Color.g = 255;
                Font_Color.b = 255;
            }

            surface = TTF_RenderText_Blended(font, snum, Font_Color);
            texture = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_Rect Message_rect = {
                .x = j * CELL_SIZE1 + 36,
                .y = i * CELL_SIZE1 + 30,
                .w = 42 + game[i][j] / 16,
                .h = 72,
            };

            SDL_RenderCopy(renderer, texture, NULL, &Message_rect);

            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
            free(snum);
        }
    }
}

int random_cord() {
    return rand() % GRID_SIZE;
}

// https://xkcd.com/221/
int random_value() {
    return 2;
}

void begin() {
    memset(game, 0, sizeof(int) * GRID_SIZE * GRID_SIZE);

    game[random_cord()][random_cord()] = random_value();
    game[random_cord()][random_cord()] = random_value();
}

void add_value() {
    int i, j, c = 15;
    while (c) {
        i = random_cord();
        j = random_cord();

        if (!game[i][j]) {
            game[i][j] = random_value();
            return;
        }
        c--;
    }

    // lol
    begin();
}
int score123 = 0;
void playDino() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event event;
    SDL_Texture* dinoTexture = NULL;
    SDL_Texture* obstacleTexture = NULL;
    Dino dino;
    Obstacle obstacle;
    int isRunning = 1;

    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Create a window
    window = SDL_CreateWindow("Dashing Dino Dash", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures
    SDL_Surface* dinoSurface = IMG_Load("dino.png");
    SDL_Surface* obstacleSurface = IMG_Load("tree.png");
    dinoTexture = SDL_CreateTextureFromSurface(renderer, dinoSurface);
    obstacleTexture = SDL_CreateTextureFromSurface(renderer, obstacleSurface);
    SDL_FreeSurface(dinoSurface);
    SDL_FreeSurface(obstacleSurface);

    // Set initial position and velocity of the dino
    dino.x = SCREEN_WIDTH / 4 - DINO_WIDTH / 2;
    dino.y = SCREEN_HEIGHT - GROUND_HEIGHT - DINO_HEIGHT + 10;
    dino.velocityY = 0;

    // Set initial position and velocity of the obstacle
    obstacle.x = SCREEN_WIDTH;
    obstacle.y = SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT;
    obstacle.velocityX = -7;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = 0;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && dino.y >= SCREEN_HEIGHT - GROUND_HEIGHT - DINO_HEIGHT) {
                // Jump when spacebar is pressed
                dino.velocityY = -JUMP_FORCE;
                score123++;
            }
        }

        // Update dino position
        dino.y += dino.velocityY;
        dino.velocityY += GRAVITY;

        // Check collision with ground
        if (dino.y >= SCREEN_HEIGHT - GROUND_HEIGHT - DINO_HEIGHT) {
            dino.y = SCREEN_HEIGHT - GROUND_HEIGHT - DINO_HEIGHT;
            dino.velocityY = 0;
           
        }

        // Update obstacle position
        obstacle.x += obstacle.velocityX;

        // Reset obstacle position if it goes off-screen
        if (obstacle.x + OBSTACLE_WIDTH < 0) {
            obstacle.x = SCREEN_WIDTH;
        }

        // Check collision between dino and obstacle
        if (dino.x + DINO_WIDTH > obstacle.x && dino.x < obstacle.x + OBSTACLE_WIDTH &&
            dino.y + DINO_HEIGHT > obstacle.y && dino.y < obstacle.y + OBSTACLE_HEIGHT) {
            // Game over
            printf("Score: %d\n", score123);
            printf("Game Over!\n");
            isRunning = 0;
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw dino
        SDL_Rect dinoRect = { dino.x, dino.y, DINO_WIDTH, DINO_HEIGHT };
        SDL_RenderCopy(renderer, dinoTexture, NULL, &dinoRect);

        // Draw obstacle
        SDL_Rect obstacleRect = { obstacle.x, obstacle.y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT };
        SDL_RenderCopy(renderer, obstacleTexture, NULL, &obstacleRect);

        // Update the screen
        SDL_RenderPresent(renderer);

        // Delay to control the game's frame rate
        SDL_Delay(10);
    }

    // Clean up resources
    SDL_DestroyTexture(dinoTexture);
    SDL_DestroyTexture(obstacleTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void fall() {
    int i, j, k, temp;

    // migrate zeros to backward
    for (i = 0; i < GRID_SIZE; ++i) {
        j = GRID_SIZE - 1;
        k = GRID_SIZE - 2;
        while (k > -1) {
            if (game[j][i] == 0 && game[k][i] != 0) {
                temp = game[k][i];
                game[k][i] = game[j][i];
                game[j][i] = temp;
                j--;
            }
            else if (game[j][i]) {
                j--;
            }
            k--;
        }
    }

    // resolve
    for (i = GRID_SIZE - 2; i >= 0; --i) {
        for (j = 0; j < GRID_SIZE; ++j) {
            if (game[i][j] == game[i + 1][j]) {
                game[i + 1][j] = game[i][j] * 2;
                game[i][j] = 0;
            }
            else if (game[i + 1][j] == 0) {
                game[i + 1][j] = game[i][j];
                game[i][j] = 0;
            }
            else {
                game[i][j] = game[i][j];
            }
        }
    }
}

void upside_down() {
    int temp;
    for (int i = 0; i < GRID_SIZE / 2; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            temp = game[i][j];
            game[i][j] = game[GRID_SIZE - i - 1][j];
            game[GRID_SIZE - i - 1][j] = temp;
        }
    }
}

void rotate_right() {
    int temp;
    int(*g2)[GRID_SIZE] = calloc(GRID_SIZE, GRID_SIZE * sizeof(int));

    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            g2[GRID_SIZE - 1 - j][i] = game[i][j];
        }
    }

    memcpy(game, g2, sizeof(int) * GRID_SIZE * GRID_SIZE);
    free(g2);
}

void rotate_left() {
    int(*g2)[GRID_SIZE] = calloc(GRID_SIZE, GRID_SIZE * sizeof(int));

    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            g2[i][j] = game[GRID_SIZE - 1 - j][i];
        }
    }

    memcpy(game, g2, sizeof(int) * GRID_SIZE * GRID_SIZE);
    free(g2);
}


bool loop() {
    SDL_Event event;

    if (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_w:
            case SDLK_UP:
                upside_down();
                fall();
                upside_down();
                break;
            case SDLK_s:
            case SDLK_DOWN:
                fall();
                break;
            case SDLK_a:
            case SDLK_LEFT:
                rotate_right();
                fall();
                rotate_left();
                break;
            case SDLK_d:
            case SDLK_RIGHT:
                rotate_left();
                fall();
                rotate_right();
                break;
            case SDLK_n:
                begin();
                break;
            }
            add_value();
            break;
        case SDL_QUIT:
            return false;
            break;
        }
    }

    SDL_SetRenderDrawColor(renderer, grid_background.r, grid_background.g, grid_background.b, grid_background.a);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, grid_line_color.r, grid_line_color.g, grid_line_color.b, grid_line_color.a);

    // better than before - still not satisfied.
    for (int x = 0; x < window_side; x += CELL_SIZE1) {
        for (int i = 0; i < 4; i++)
            SDL_RenderDrawLine(renderer, x + i, 0, x + i, window_side);
    }

    for (int y = 0; y < window_side; y += CELL_SIZE1) {
        for (int i = 0; i < 4; i++)
            SDL_RenderDrawLine(renderer, 0, y + i, window_side, y + i);
    }

    display_board();
    SDL_RenderPresent(renderer);

    return true;
}

void kill_resources() {
    /* Free resources and Quit */

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

void handleInput1(SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
        case SDLK_UP:
            if (playerY > 0 && maze[playerY - 1][playerX] == 0)
                playerY--;
            break;
        case SDLK_DOWN:
            if (playerY < MAZE_HEIGHT - 1 && maze[playerY + 1][playerX] == 0)
                playerY++;
            break;
        case SDLK_LEFT:
            if (playerX > 0 && maze[playerY][playerX - 1] == 0)
                playerX--;
            break;
        case SDLK_RIGHT:
            if (playerX < MAZE_WIDTH - 1 && maze[playerY][playerX + 1] == 0)
                playerX++;
            break;
        }
    }
}

void checkGameCompletion() {
    if (playerX == GOAL_X && playerY == GOAL_Y) {
        gameover1 = 1;
        printf("Congratulations! You reached the goal!\n");
    }
}

void renderMaze(SDL_Renderer* renderer) {
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (maze[y][x] == 1) {
                SDL_Rect wallRect = { x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &wallRect);
            }
        }
    }
}

void renderPlayer(SDL_Renderer* renderer) {
    SDL_Rect playerRect = { playerX * CELL_SIZE, playerY * CELL_SIZE, CELL_SIZE, CELL_SIZE };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &playerRect);
}

void renderGoal(SDL_Renderer* renderer) {
    SDL_Rect goalRect = { GOAL_X * CELL_SIZE, GOAL_Y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &goalRect);
}


TTF_Font* font = NULL;

int grid[NUM_BLOCKS_X][NUM_BLOCKS_Y] = { 0 };
int score = 0;
int gameover2 = 0;

void checkGameOver() {
    for (int x = 0; x < NUM_BLOCKS_X; x++) {
        if (grid[x][0] != 0) {
            gameover2 = 1;
            printf("Score: %d\n", score);
            printf("Game Over!\n");
            break;
        }
    }
}


Tetromino tetrominoes[7] = {
    { 0, 0, {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}} },  // I
    { 0, 0, {{0,0,0,0},{1,1,0,0},{1,1,0,0},{0,0,0,0}} },  // O
    { 0, 0, {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}} },  // L
    { 0, 0, {{0,0,0,0},{0,1,1,1},{0,1,0,0},{0,0,0,0}} },  // J
    { 0, 0, {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}} },  // T
    { 0, 0, {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}} },  // S
    { 0, 0, {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}} }   // Z
};


void drawBlock(int x, int y, SDL_Color color) {
    SDL_Rect rect = { x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &rect);

}

void drawGrid() {
    for (int x = 0; x < NUM_BLOCKS_X; x++) {
        for (int y = 0; y < NUM_BLOCKS_Y; y++) {
            if (grid[x][y]) {
                SDL_Color color = { 255, 255, 255 };
                drawBlock(x, y, color);
            }
        }
    }
}

void drawTetromino(Tetromino tetromino, SDL_Color color) {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (tetromino.shape[x][y]) {
                drawBlock(tetromino.x + x, tetromino.y + y, color);
            }
        }
    }
}

int isValidPosition(Tetromino tetromino) {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (tetromino.shape[x][y]) {
                int newX = tetromino.x + x;
                int newY = tetromino.y + y;
                if (newX < 0 || newX >= NUM_BLOCKS_X || newY >= NUM_BLOCKS_Y || grid[newX][newY])
                    return 0;
            }
        }
    }
    return 1;
}

void placeTetromino(Tetromino tetromino) {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (tetromino.shape[x][y]) {
                int newX = tetromino.x + x;
                int newY = tetromino.y + y;
                grid[newX][newY] = 1;
            }
        }
    }
}

void removeCompletedLines() {
    int numCompletedLines = 0;
    for (int y = NUM_BLOCKS_Y - 1; y >= 0; y--) {
        int lineComplete = 1;
        for (int x = 0; x < NUM_BLOCKS_X; x++) {
            if (!grid[x][y]) {
                lineComplete = 0;
                break;
            }
        }
        if (lineComplete) {
            numCompletedLines++;
            for (int i = y; i > 0; i--) {
                for (int x = 0; x < NUM_BLOCKS_X; x++) {
                    grid[x][i] = grid[x][i - 1];
                }
            }
        }
    }
    int lineScore = numCompletedLines * 10;
    score += lineScore;


    // Score calculation and other logic for the game can be added here
}

Tetromino getRandomTetromino() {
    int index = rand() % 7;
    return tetrominoes[index];
}

void rotateTetromino(Tetromino* tetromino) {
    int temp[4][4];
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            temp[x][y] = tetromino->shape[x][y];
        }
    }
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            tetromino->shape[x][y] = temp[y][3 - x];
        }
    }
}

void handleEvent(SDL_Event event, Tetromino* tetromino) {
    switch (event.type) {
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_UP:
            rotateTetromino(tetromino);
            if (!isValidPosition(*tetromino))
                rotateTetromino(tetromino);  // Revert if the rotation is invalid
            break;
        case SDLK_LEFT:
            tetromino->x--;
            if (!isValidPosition(*tetromino))
                tetromino->x++;  // Revert if the move is invalid
            break;
        case SDLK_RIGHT:
            tetromino->x++;
            if (!isValidPosition(*tetromino))
                tetromino->x--;  // Revert if the move is invalid
            break;
        case SDLK_DOWN:
            tetromino->y++;
            if (!isValidPosition(*tetromino))
                tetromino->y--;  // Revert if the move is invalid
            break;
        }
        break;
    case SDL_QUIT:
        exit(0);
        break;
    }
}



SDL_Rect spaceship;
SDL_Rect projectiles[10];
SDL_Rect asteroids[5];
int score10 = 0;

void initializeGame10() {
    spaceship.x = SCREEN_WIDTH / 2 - SPACESHIP_SIZE / 2;
    spaceship.y = SCREEN_HEIGHT - SPACESHIP_SIZE - 20;
    spaceship.w = SPACESHIP_SIZE;
    spaceship.h = SPACESHIP_SIZE;

    for (int i = 0; i < 10; i++) {
        projectiles[i].x = -PROJECTILE_SIZE;
        projectiles[i].y = -PROJECTILE_SIZE;
        projectiles[i].w = PROJECTILE_SIZE;
        projectiles[i].h = PROJECTILE_SIZE;
    }

    for (int i = 0; i < 5; i++) {
        asteroids[i].x = rand() % (SCREEN_WIDTH - ASTEROID_SIZE);
        asteroids[i].y = -(rand() % (SCREEN_HEIGHT / 2));
        asteroids[i].w = ASTEROID_SIZE;
        asteroids[i].h = ASTEROID_SIZE;
    }
}

void handleInput10(SDL_Event e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_LEFT:
            spaceship.x -= SPACESHIP_SPEED;
            break;
        case SDLK_RIGHT:
            spaceship.x += SPACESHIP_SPEED;
            break;
        case SDLK_SPACE:
            for (int i = 0; i < 10; i++) {
                if (projectiles[i].y <= 0 - PROJECTILE_SIZE) {
                    projectiles[i].x = spaceship.x + SPACESHIP_SIZE / 2 - PROJECTILE_SIZE / 2;
                    projectiles[i].y = spaceship.y;
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
}

void updateGame10() {
    for (int i = 0; i < 10; i++) {
        if (projectiles[i].y > 0 - PROJECTILE_SIZE) {
            projectiles[i].y -= PROJECTILE_SPEED;

            for (int j = 0; j < 5; j++) {
                if (SDL_HasIntersection(&projectiles[i], &asteroids[j])) {
                    projectiles[i].y = -PROJECTILE_SIZE;
                    asteroids[j].x = rand() % (SCREEN_WIDTH - ASTEROID_SIZE);
                    asteroids[j].y = -(rand() % (SCREEN_HEIGHT / 2));
                    score10++;
                    printf("Score: %d\n", score10);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        asteroids[i].y += ASTEROID_SPEED;

        if (asteroids[i].y >= SCREEN_HEIGHT) {
            asteroids[i].x = rand() % (SCREEN_WIDTH - ASTEROID_SIZE);
            asteroids[i].y = -(rand() % (SCREEN_HEIGHT / 2));
        }

        if (SDL_HasIntersection(&spaceship, &asteroids[i])) {
            printf("Game Over!\n");
            SDL_Delay(2000);
            exit(0);
        }
    }
}

void renderGame10() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &spaceship);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int i = 0; i < 10; i++) {
        SDL_RenderFillRect(renderer, &projectiles[i]);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 5; i++) {
        SDL_RenderFillRect(renderer, &asteroids[i]);
    }

    SDL_RenderPresent(renderer);
}



// Function to handle Tetris game
void playTetris()

{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // Create a window and renderer
    window = SDL_CreateWindow("Tetra Twist", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load the font
    font = TTF_OpenFont("arial.ttf", 28);

    // Game variables
    Tetromino currentTetromino = getRandomTetromino();
    Uint32 lastMoveTime = SDL_GetTicks();

    // Game loop
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event, &currentTetromino);
        }

        // Move tetromino down every 500ms
        if (SDL_GetTicks() - lastMoveTime > 500) {
            currentTetromino.y++;
            if (!isValidPosition(currentTetromino)) {
                currentTetromino.y--;  // Revert if the move is invalid
                placeTetromino(currentTetromino);
                removeCompletedLines();
                currentTetromino = getRandomTetromino();
                if (!isValidPosition(currentTetromino)) {
                    // Game over logic can be added here
                }
            }
            lastMoveTime = SDL_GetTicks();
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        removeCompletedLines();

        // Draw the grid
        drawGrid();

        // Draw the current tetromino
        SDL_Color color = { 255, 255, 255 };
        drawTetromino(currentTetromino, color);
        checkGameOver();
        if (gameover2)
            break;
        removeCompletedLines();

        // Render to the screen
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    // TODO: Implement Tetris game logic
}
void play2048() {

    begin();
    TTF_Init();
    font = TTF_OpenFont("arial.ttf", 108);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainloop, 0, 1);
#else
    while (loop(game)) {
        SDL_Delay(100);
    }
#endif

    kill_resources();

}
void playCar() {
    srand(time(NULL));
    initialize();

    while (!gameover4) {
        handle_input();
        update_game();
        render_game();
        SDL_Delay(16);  // Cap the frame rate at approximately 60 FPS
    }

    printf("Game over! Score: %d\n", score4);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
const int PLAYER_WIDTH = 90;
const int PLAYER_HEIGHT = 90;
const int PLAYER_VELOCITY22 = 5;

const int BULLET_WIDTH = 60;
const int BULLET_HEIGHT = 60;
const int BULLET_VELOCITY = 10;

const int ASTEROID_WIDTH = 90;
const int ASTEROID_HEIGHT = 90;
const int ASTEROID_VELOCITY = 5;

SDL_Texture* playerTexture = NULL;
SDL_Texture* bulletTexture = NULL;
SDL_Texture* asteroidTexture = NULL;


void initializeGame(GameState* gameState) {
    // Initialize player
    gameState->player.rect.x = (WINDOW_WIDTH - PLAYER_WIDTH) / 2;
    gameState->player.rect.y = WINDOW_HEIGHT - PLAYER_HEIGHT;
    gameState->player.rect.w = PLAYER_WIDTH;
    gameState->player.rect.h = PLAYER_HEIGHT;
    gameState->player.velocity = PLAYER_VELOCITY22;

    // Initialize bullets
    for (int i = 0; i < 10; i++) {
        gameState->bullets[i].rect.x = 0;
        gameState->bullets[i].rect.y = 0;
        gameState->bullets[i].rect.w = BULLET_WIDTH;
        gameState->bullets[i].rect.h = BULLET_HEIGHT;
        gameState->bullets[i].velocity = BULLET_VELOCITY;
    }

    // Initialize asteroids
    for (int i = 0; i < 10; i++) {
        gameState->asteroids[i].rect.x = rand() % (WINDOW_WIDTH - ASTEROID_WIDTH);
        gameState->asteroids[i].rect.y = -(rand() % WINDOW_HEIGHT);
        gameState->asteroids[i].rect.w = ASTEROID_WIDTH;
        gameState->asteroids[i].rect.h = ASTEROID_HEIGHT;
        gameState->asteroids[i].velocity = ASTEROID_VELOCITY;
    }
}

void handleInput88(GameState* gameState) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE) {
                // Shoot a bullet
                for (int i = 0; i < 10; i++) {
                    if (gameState->bullets[i].rect.y <= 0) {
                        gameState->bullets[i].rect.x = gameState->player.rect.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
                        gameState->bullets[i].rect.y = gameState->player.rect.y;
                        break;
                    }
                }
            }
            break;
        }
    }

    const Uint8* currentKeyState = SDL_GetKeyboardState(NULL);
    if (currentKeyState[SDL_SCANCODE_LEFT] && gameState->player.rect.x > 0) {
        gameState->player.rect.x -= gameState->player.velocity;
    }
    if (currentKeyState[SDL_SCANCODE_RIGHT] && gameState->player.rect.x + PLAYER_WIDTH < WINDOW_WIDTH) {
        gameState->player.rect.x += gameState->player.velocity;
    }
}
int score222 = 0;
void update(GameState* gameState) {
    // Update bullets
    for (int i = 0; i < 10; i++) {
        if (gameState->bullets[i].rect.y > 0) {
            gameState->bullets[i].rect.y -= gameState->bullets[i].velocity;
        }
    }

    // Update asteroids
    for (int i = 0; i < 10; i++) {
        if (gameState->asteroids[i].rect.y < WINDOW_HEIGHT) {
            gameState->asteroids[i].rect.y += gameState->asteroids[i].velocity;
        }
        else {
            gameState->asteroids[i].rect.x = rand() % (WINDOW_WIDTH - ASTEROID_WIDTH);
            gameState->asteroids[i].rect.y = -(rand() % WINDOW_HEIGHT);
        }

        // Check collision with bullets
        for (int j = 0; j < 10; j++) {
            if (SDL_HasIntersection(&gameState->asteroids[i].rect, &gameState->bullets[j].rect)) {
                gameState->bullets[j].rect.x = 0;
                gameState->bullets[j].rect.y = 0;
                gameState->asteroids[i].rect.x = rand() % (WINDOW_WIDTH - ASTEROID_WIDTH);
                gameState->asteroids[i].rect.y = -(rand() % WINDOW_HEIGHT);
                score222++;
            }
        }

        // Check collision with player
        if (SDL_HasIntersection(&gameState->asteroids[i].rect, &gameState->player.rect)) {
            printf("Score: %d\n", score222);
            printf("Game Over\n");
            exit(0);
        }
    }
}

void render88(GameState* gameState) {
    SDL_RenderClear(renderer);

    // Render player
    SDL_RenderCopy(renderer, playerTexture, NULL, &gameState->player.rect);

    // Render bullets
    for (int i = 0; i < 10; i++) {
        SDL_RenderCopy(renderer, bulletTexture, NULL, &gameState->bullets[i].rect);
    }

    // Render asteroids
    for (int i = 0; i < 10; i++) {
        SDL_RenderCopy(renderer, asteroidTexture, NULL, &gameState->asteroids[i].rect);
    }

    SDL_RenderPresent(renderer);
}

void playHorizontal() {
    srand(SDL_GetTicks());

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Bird Hunter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface* playerSurface = IMG_Load("man2.png");
    SDL_Surface* bulletSurface = IMG_Load("arr.png");
    SDL_Surface* asteroidSurface = IMG_Load("birdie.png");

    playerTexture = SDL_CreateTextureFromSurface(renderer, playerSurface);
    bulletTexture = SDL_CreateTextureFromSurface(renderer, bulletSurface);
    asteroidTexture = SDL_CreateTextureFromSurface(renderer, asteroidSurface);

    SDL_FreeSurface(playerSurface);
    SDL_FreeSurface(bulletSurface);
    SDL_FreeSurface(asteroidSurface);

    GameState gameState;
    initializeGame(&gameState);

    while (1) {
        handleInput88(&gameState);
        update(&gameState);
        render88(&gameState);

        SDL_Delay(10);
    }

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(asteroidTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();



}
void playImage() {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event event;
    Image image;
    int score = 0;
    Uint32 startTicks = 0;
    Uint32 clickTicks = 0;
    int gameOver = 0;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    window = SDL_CreateWindow("Quick Click", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize random number generator
    srand(time(NULL));

    // Load image
    SDL_Surface* imageSurface = IMG_Load("skull.png");
    if (imageSurface == NULL) {
        printf("Failed to load image! SDL_Error: %s\n", IMG_GetError());
        return 1;
    }

    SDL_Texture* imageTexture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    SDL_FreeSurface(imageSurface);

    // Initialize image
    image.x = rand() % (SCREEN_WIDTH - IMAGE_WIDTH);
    image.y = rand() % (SCREEN_HEIGHT - IMAGE_HEIGHT);
    image.texture = imageTexture;

    // Game loop
    int running = 1;
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && !gameOver) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                // Check if the image was clicked
                if (mouseX >= image.x && mouseX <= image.x + IMAGE_WIDTH &&
                    mouseY >= image.y && mouseY <= image.y + IMAGE_HEIGHT) {
                    score++;
                    image.x = rand() % (SCREEN_WIDTH - IMAGE_WIDTH);
                    image.y = rand() % (SCREEN_HEIGHT - IMAGE_HEIGHT);
                    startTicks = SDL_GetTicks();
                }
            }
        }

        // Clear the renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Check if the image click time has exceeded the maximum click time
        if (!gameOver && startTicks > 0 && SDL_GetTicks() - startTicks > MAX_CLICK_TIME) {
            printf("Game over! Score: %d\n", score);
            gameOver = 1;
            SDL_Quit();
        }

        // Render image
        if (!gameOver) {
            SDL_Rect imageRect = { image.x, image.y, IMAGE_WIDTH, IMAGE_HEIGHT };
            SDL_RenderCopy(renderer, image.texture, NULL, &imageRect);
        }

        // Update the renderer
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyTexture(imageTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;

}

// Function to handle Maze game

void playMaze()
{
    SDL_Init(SDL_INIT_VIDEO);

    // Create a window and renderer
    SDL_Window* window = SDL_CreateWindow("Maze Mastermind", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Game loop
    while (!gameover1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                gameover2 = 1;
            handleInput1(&event);
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update the game state
        checkGameCompletion();

        // Render the maze elements
        renderMaze(renderer);
        renderPlayer(renderer);
        renderGoal(renderer);

        // Render to the screen
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    // TODO: Implement Maze game logic
}


SDL_Texture* backgroundImage = NULL;
SDL_Texture* button1Image = NULL;
SDL_Texture* button2Image = NULL;
SDL_Texture* button3Image = NULL;
SDL_Texture* button4Image = NULL;
SDL_Texture* button5Image = NULL;
SDL_Texture* button6Image = NULL;
SDL_Texture* button7Image = NULL;
SDL_Texture* button8Image = NULL;

SDL_Rect button1Rect = { 150, 300, 100, 70 };
SDL_Rect button2Rect = { 490, 300, 100, 70 };
SDL_Rect button3Rect = { 150, 400, 100, 70 };
SDL_Rect button4Rect = { 490, 400, 100, 70 };
SDL_Rect button5Rect = { 150, 200, 100, 70 };
SDL_Rect button6Rect = { 490, 200, 100, 70 };
SDL_Rect button7Rect = { 150, 100, 100, 70 };
SDL_Rect button8Rect = { 490, 100, 100, 70 };

int gameover = 0;

int initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Number Merge Mania", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    return 1;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (surface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_FreeSurface(surface);
    return texture;
}

int loadMedia() {
    backgroundImage = loadTexture("city_background.png");
    if (backgroundImage == NULL)
        return 0;

    button1Image = loadTexture("button.png");
    if (button1Image == NULL)
        return 0;

    button2Image = loadTexture("button.png");
    if (button2Image == NULL)
        return 0;
    button3Image = loadTexture("button.png");
    if (button3Image == NULL)
        return 0;
    button4Image = loadTexture("button.png");
    if (button4Image == NULL)
        return 0;
    button5Image = loadTexture("button.png");
    if (button5Image == NULL)
        return 0;
    button6Image = loadTexture("button.png");
    if (button6Image == NULL)
        return 0;
    button7Image = loadTexture("button.png");
    if (button7Image == NULL)
        return 0;
    button8Image = loadTexture("button.png");
    if (button8Image == NULL)
        return 0;

    return 1;
}

void handleInput(SDL_Event* event) {
    if (event->type == SDL_QUIT) {
        gameover = 1;
    }
    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseX >= button1Rect.x && mouseX < button1Rect.x + button1Rect.w &&
            mouseY >= button1Rect.y && mouseY < button1Rect.y + button1Rect.h) {
            playTetris();
        }
        else if (mouseX >= button2Rect.x && mouseX < button2Rect.x + button2Rect.w &&
            mouseY >= button2Rect.y && mouseY < button2Rect.y + button2Rect.h) {
            playMaze();
        }
        else if (mouseX >= button3Rect.x && mouseX < button3Rect.x + button3Rect.w &&
            mouseY >= button3Rect.y && mouseY < button3Rect.y + button3Rect.h) {
            play2048();
        }
        else if (mouseX >= button4Rect.x && mouseX < button4Rect.x + button4Rect.w &&
            mouseY >= button4Rect.y && mouseY < button4Rect.y + button4Rect.h) {
            playCar();
        }
        else if (mouseX >= button5Rect.x && mouseX < button5Rect.x + button5Rect.w &&
            mouseY >= button5Rect.y && mouseY < button5Rect.y + button5Rect.h) {
            playHorizontal();
        }
        else if (mouseX >= button6Rect.x && mouseX < button6Rect.x + button6Rect.w &&
            mouseY >= button6Rect.y && mouseY < button6Rect.y + button6Rect.h) {
            playImage();
        }
        else if (mouseX >= button7Rect.x && mouseX < button7Rect.x + button7Rect.w &&
            mouseY >= button7Rect.y && mouseY < button7Rect.y + button7Rect.h) {
            playDino();
        }
        else if (mouseX >= button8Rect.x && mouseX < button8Rect.x + button8Rect.w &&
            mouseY >= button8Rect.y && mouseY < button8Rect.y + button8Rect.h) {
            playCrush();
        }
    }
}

void render() {
    SDL_RenderCopy(renderer, backgroundImage, NULL, NULL);
    SDL_RenderCopy(renderer, button1Image, NULL, &button1Rect);
    SDL_RenderCopy(renderer, button2Image, NULL, &button2Rect);
    SDL_RenderCopy(renderer, button3Image, NULL, &button3Rect);
    SDL_RenderCopy(renderer, button4Image, NULL, &button4Rect);
    SDL_RenderCopy(renderer, button5Image, NULL, &button5Rect);
    SDL_RenderCopy(renderer, button6Image, NULL, &button6Rect);
    SDL_RenderCopy(renderer, button7Image, NULL, &button7Rect);
    SDL_RenderCopy(renderer, button8Image, NULL, &button8Rect);
    SDL_RenderPresent(renderer);


}

void closeSDL() {
    SDL_DestroyTexture(backgroundImage);
    SDL_DestroyTexture(button1Image);
    SDL_DestroyTexture(button2Image);
    SDL_DestroyTexture(button3Image);
    SDL_DestroyTexture(button4Image);
    SDL_DestroyTexture(button5Image);
    SDL_DestroyTexture(button6Image);
    SDL_DestroyTexture(button7Image);
    SDL_DestroyTexture(button8Image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!initializeSDL()) {
        printf("Failed to initialize SDL.\n");
        return 1;
    }

    if (!loadMedia()) {
        printf("Failed to load media.\n");
        return 1;
    }

    while (!gameover) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleInput(&event);
        }

        render();
    }

    closeSDL();

    return 0;
}
