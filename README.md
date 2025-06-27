```cpp
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <conio.h>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

struct Pixel {
    int r, g, b;
    bool active;

    Pixel() : r(255), g(255), b(255), active(false) {}
    Pixel(int r, int g, int b) : r(r), g(g), b(b), active(true) {}
};

class Bitmap {
private:
    std::vector<std::vector<Pixel>> data;
    int width, height;

public:
    Bitmap(int w, int h) : width(w), height(h) {
        data.resize(h, std::vector<Pixel>(w));
    }

    void setPixel(int x, int y, int r, int g, int b) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            data[y][x] = Pixel(r, g, b);
        }
    }

    void setPixel(int x, int y, const Pixel& p) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            data[y][x] = p;
        }
    }

    Pixel getPixel(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return data[y][x];
        }
        return Pixel();
    }

    void clear() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                data[y][x] = Pixel();
            }
        }
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

std::string fgColor(int r, int g, int b) {
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string bgColor(int r, int g, int b) {
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string resetColor() {
    return "\033[0m";
}

void renderBitmap(const Bitmap& bitmap) {
    int width = bitmap.getWidth();
    int height = bitmap.getHeight();
    
    int adjustedHeight = (height + 1) / 2 * 2;
    
    // Move cursor to top-left corner
    std::cout << "\033[H";
    
    for (int y = 0; y < adjustedHeight; y += 2) {
        for (int x = 0; x < width; x++) {
            Pixel upperPixel = bitmap.getPixel(x, y);
            Pixel lowerPixel = (y + 1 < height) ? bitmap.getPixel(x, y + 1) : Pixel();
            
            if (!upperPixel.active && !lowerPixel.active) {
                std::cout << " ";
            } else if (upperPixel.active && !lowerPixel.active) {
                std::cout << fgColor(upperPixel.r, upperPixel.g, upperPixel.b) << "▀" << resetColor();
            } else if (!upperPixel.active && lowerPixel.active) {
                std::cout << fgColor(lowerPixel.r, lowerPixel.g, lowerPixel.b) << "▄" << resetColor();
            } else {
                // Both pixels are active, use bgColor for the lower pixel and fgColor for the upper pixel
                std::cout << fgColor(upperPixel.r, upperPixel.g, upperPixel.b) 
                          << bgColor(lowerPixel.r, lowerPixel.g, lowerPixel.b) 
                          << "▀" << resetColor();
            }
        }
        std::cout << std::endl;
    }
}

// Tetromino shapes
const std::vector<std::vector<std::vector<int>>> TETROMINOS = {
    // I-piece (cyan)
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    // J-piece (blue)
    {
        {2, 0, 0},
        {2, 2, 2},
        {0, 0, 0}
    },
    // L-piece (orange)
    {
        {0, 0, 3},
        {3, 3, 3},
        {0, 0, 0}
    },
    // O-piece (yellow)
    {
        {4, 4},
        {4, 4}
    },
    // S-piece (green)
    {
        {0, 5, 5},
        {5, 5, 0},
        {0, 0, 0}
    },
    // T-piece (purple)
    {
        {0, 6, 0},
        {6, 6, 6},
        {0, 0, 0}
    },
    // Z-piece (red)
    {
        {7, 7, 0},
        {0, 7, 7},
        {0, 0, 0}
    }
};

// Colors for each tetromino (RGB)
const std::vector<std::vector<int>> COLORS = {
    {0, 0, 0},       // Empty (black)
    {0, 240, 240},   // I-piece (cyan)
    {0, 0, 240},     // J-piece (blue)
    {240, 160, 0},   // L-piece (orange)
    {240, 240, 0},   // O-piece (yellow)
    {0, 240, 0},     // S-piece (green)
    {160, 0, 240},   // T-piece (purple)
    {240, 0, 0}      // Z-piece (red)
};

class TetrisGame {
private:
    Bitmap bitmap;
    int width, height;
    int boardWidth, boardHeight;
    int score;
    int level;
    int linesCleared;
    bool gameOver;
    
    std::vector<std::vector<int>> board;
    std::vector<std::vector<int>> currentPiece;
    int currentPieceX, currentPieceY;
    int currentPieceType;
    
    std::mt19937 rng;
    
    int getDropDelay() const {
        // Speed increases with level
        return std::max(100, 800 - level * 50);
    }
    
    void spawnNewPiece() {
        std::uniform_int_distribution<int> dist(0, TETROMINOS.size() - 1);
        currentPieceType = dist(rng);
        currentPiece = TETROMINOS[currentPieceType];
        
        // Start position (centered at top)
        currentPieceX = (boardWidth - currentPiece[0].size()) / 2;
        currentPieceY = 0;
        
        // Check if the new piece can be placed
        if (!isValidPosition()) {
            gameOver = true;
        }
    }
    
    bool isValidPosition() const {
        for (size_t y = 0; y < currentPiece.size(); y++) {
            for (size_t x = 0; x < currentPiece[y].size(); x++) {
                if (currentPiece[y][x] != 0) {
                    int boardX = currentPieceX + x;
                    int boardY = currentPieceY + y;
                    
                    // Check if out of bounds or collides with existing blocks
                    if (boardX < 0 || boardX >= boardWidth || 
                        boardY < 0 || boardY >= boardHeight ||
                        board[boardY][boardX] != 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    void placePiece() {
        for (size_t y = 0; y < currentPiece.size(); y++) {
            for (size_t x = 0; x < currentPiece[y].size(); x++) {
                if (currentPiece[y][x] != 0) {
                    int boardX = currentPieceX + x;
                    int boardY = currentPieceY + y;
                    board[boardY][boardX] = currentPiece[y][x];
                }
            }
        }
        
        // Check for completed lines
        checkLines();
        
        // Spawn a new piece
        spawnNewPiece();
    }
    
    void checkLines() {
        int linesCompleted = 0;
        
        for (int y = boardHeight - 1; y >= 0; y--) {
            bool lineComplete = true;
            
            for (int x = 0; x < boardWidth; x++) {
                if (board[y][x] == 0) {
                    lineComplete = false;
                    break;
                }
            }
            
            if (lineComplete) {
                // Remove the line and shift everything down
                for (int moveY = y; moveY > 0; moveY--) {
                    for (int x = 0; x < boardWidth; x++) {
                        board[moveY][x] = board[moveY - 1][x];
                    }
                }
                
                // Clear the top line
                for (int x = 0; x < boardWidth; x++) {
                    board[0][x] = 0;
                }
                
                // Since we've shifted everything down, we need to check this row again
                y++;
                linesCompleted++;
            }
        }
        
        // Update score and level
        if (linesCompleted > 0) {
            // Classic Tetris scoring: 40, 100, 300, or 1200 points for 1, 2, 3, or 4 lines
            int points[] = {0, 40, 100, 300, 1200};
            score += points[linesCompleted] * (level + 1);
            
            linesCleared += linesCompleted;
            level = linesCleared / 10;
        }
    }
    
    void rotatePiece() {
        // Save the current piece
        auto originalPiece = currentPiece;
        
        // Get dimensions
        int n = currentPiece.size();
        
        // Create a new rotated piece
        std::vector<std::vector<int>> rotated(n, std::vector<int>(n, 0));
        
        // Rotate 90 degrees clockwise
        for (int y = 0; y < n; y++) {
            for (int x = 0; x < n; x++) {
                rotated[x][n - 1 - y] = currentPiece[y][x];
            }
        }
        
        // Apply rotation
        currentPiece = rotated;
        
        // If the rotation is invalid, revert back
        if (!isValidPosition()) {
            currentPiece = originalPiece;
        }
    }
    
public:
    TetrisGame(int w, int h) : bitmap(w, h), width(w), height(h), 
                              boardWidth(10), boardHeight(20),
                              score(0), level(0), linesCleared(0), gameOver(false) {
        // Initialize random number generator
        std::random_device rd;
        rng = std::mt19937(rd());
        
        // Initialize the board
        board.resize(boardHeight, std::vector<int>(boardWidth, 0));
        
        // Spawn the first piece
        spawnNewPiece();
    }
    
    void update(bool &dropTimer) {
        if (gameOver) return;
        
        if (dropTimer) {
            // Move piece down
            currentPieceY++;
            
            // Check if the piece can be placed in the new position
            if (!isValidPosition()) {
                // Move back up and place the piece
                currentPieceY--;
                placePiece();
            }
            
            dropTimer = false;
        }
    }
    
    void moveLeft() {
        if (gameOver) return;
        
        currentPieceX--;
        if (!isValidPosition()) {
            currentPieceX++;
        }
    }
    
    void moveRight() {
        if (gameOver) return;
        
        currentPieceX++;
        if (!isValidPosition()) {
            currentPieceX--;
        }
    }
    
    void rotate() {
        if (gameOver) return;
        
        rotatePiece();
    }
    
    void hardDrop() {
        if (gameOver) return;
        
        // Move down until collision
        while (isValidPosition()) {
            currentPieceY++;
        }
        
        // Move back up one step and place
        currentPieceY--;
        placePiece();
    }
    
    void render() {
        bitmap.clear();
        
        // Calculate the offset to center the board
        int offsetX = (width - boardWidth * 2) / 2;
        int offsetY = (height - boardHeight) / 2;
        
        // Draw border
        for (int x = -1; x <= boardWidth; x++) {
            bitmap.setPixel(offsetX + x, offsetY - 1, 150, 150, 150);
            bitmap.setPixel(offsetX + x, offsetY + boardHeight, 150, 150, 150);
        }
        for (int y = -1; y <= boardHeight; y++) {
            bitmap.setPixel(offsetX - 1, offsetY + y, 150, 150, 150);
            bitmap.setPixel(offsetX + boardWidth, offsetY + y, 150, 150, 150);
        }
        
        // Draw the board
        for (int y = 0; y < boardHeight; y++) {
            for (int x = 0; x < boardWidth; x++) {
                if (board[y][x] != 0) {
                    int colorIndex = board[y][x];
                    auto& color = COLORS[colorIndex];
                    
                    bitmap.setPixel(offsetX + x, offsetY + y, color[0], color[1], color[2]);
                }
            }
        }
        
        // Draw ghost piece (preview of where the piece will land)
        int ghostY = currentPieceY;
        while (true) {
            ghostY++;
            
            // Check if the ghost piece would be in a valid position
            bool valid = true;
            for (size_t y = 0; y < currentPiece.size() && valid; y++) {
                for (size_t x = 0; x < currentPiece[y].size() && valid; x++) {
                    if (currentPiece[y][x] != 0) {
                        int boardX = currentPieceX + x;
                        int boardY = ghostY + y;
                        
                        if (boardX < 0 || boardX >= boardWidth || 
                            boardY < 0 || boardY >= boardHeight ||
                            board[boardY][boardX] != 0) {
                            valid = false;
                        }
                    }
                }
            }
            
            if (!valid) {
                ghostY--;
                break;
            }
        }
        
        // Draw the ghost piece if it's different from the current position
        if (ghostY > currentPieceY) {
            for (size_t y = 0; y < currentPiece.size(); y++) {
                for (size_t x = 0; x < currentPiece[y].size(); x++) {
                    if (currentPiece[y][x] != 0) {
                        int boardX = currentPieceX + x;
                        int boardY = ghostY + y;
                        
                        // Draw ghost piece in a dimmer color
                        int colorIndex = currentPiece[y][x];
                        auto& color = COLORS[colorIndex];
                        
                        bitmap.setPixel(offsetX + boardX, offsetY + boardY, color[0]/3, color[1]/3, color[2]/3);
                    }
                }
            }
        }

        // Draw the current piece
        for (size_t y = 0; y < currentPiece.size(); y++) {
            for (size_t x = 0; x < currentPiece[y].size(); x++) {
                if (currentPiece[y][x] != 0) {
                    int colorIndex = currentPiece[y][x];
                    auto& color = COLORS[colorIndex];
                    
                    int boardX = currentPieceX + x;
                    int boardY = currentPieceY + y;
                    
                    bitmap.setPixel(offsetX + boardX, offsetY + boardY, color[0], color[1], color[2]);
                }
            }
        }
        
        // Draw score and level info
        std::string scoreText = "Score: " + std::to_string(score);
        std::string levelText = "Level: " + std::to_string(level);
        std::string linesText = "Lines: " + std::to_string(linesCleared);
        
        // Render the bitmap
        renderBitmap(bitmap);
        
        // Display game info
        std::cout << scoreText << "  " << levelText << "  " << linesText << std::endl;
        
        if (gameOver) {
            std::cout << "Game Over! Press 'R' to restart or 'Q' to quit." << std::endl;
        } else {
            std::cout << "Controls: ←→↓ to move, ↑ to rotate, Space for hard drop" << std::endl;
        }
    }
    
    bool isGameOver() const {
        return gameOver;
    }
    
    void restart() {
        // Clear the board
        for (auto& row : board) {
            std::fill(row.begin(), row.end(), 0);
        }
        
        // Reset game state
        score = 0;
        level = 0;
        linesCleared = 0;
        gameOver = false;
        
        // Spawn a new piece
        spawnNewPiece();
    }
    
    int getDropDelay() {
        return std::max(100, 800 - level * 50);
    }
};

int main() {
    // Windows UTF-8 support
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    
    // Hide cursor
    std::cout << "\033[?25l";
    
    // Clear screen
    std::cout << "\033[2J";
    
    // Create game with 30x30 bitmap
    TetrisGame game(30, 30);
    
    // Game loop
    bool running = true;
    auto lastDropTime = std::chrono::steady_clock::now();
    bool dropTimer = false;
    
    while (running) {
        // Process input
        if (_kbhit()) {
            int key = _getch();
            
            // Handle arrow keys (which return two characters)
            if (key == 224 || key == 0) {
                key = _getch();
                switch (key) {
                    case 72: // Up arrow
                        game.rotate();
                        break;
                    case 75: // Left arrow
                        game.moveLeft();
                        break;
                    case 77: // Right arrow
                        game.moveRight();
                        break;
                    case 80: // Down arrow
                        dropTimer = true;
                        break;
                }
            } else {
                switch (key) {
                    case 'w': case 'W':
                        game.rotate();
                        break;
                    case 'a': case 'A':
                        game.moveLeft();
                        break;
                    case 'd': case 'D':
                        game.moveRight();
                        break;
                    case 's': case 'S':
                        dropTimer = true;
                        break;
                    case ' ': // Space
                        game.hardDrop();
                        break;
                    case 'r': case 'R':
                        if (game.isGameOver()) game.restart();
                        break;
                    case 'q': case 'Q': case 27: // ESC
                        running = false;
                        break;
                }
            }
        }
        
        // Check if it's time to drop the piece
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastDropTime).count();
            
        if (elapsed >= game.getDropDelay()) {
            dropTimer = true;
            lastDropTime = currentTime;
        }
        
        // Update game state
        game.update(dropTimer);
        
        // Render
        game.render();
        
        // Control game speed
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Show cursor again
    std::cout << "\033[?25h";
    
    return 0;
}
```
