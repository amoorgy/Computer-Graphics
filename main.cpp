#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Game states
enum GameState {
    START_MENU,
    CHARACTER_SELECT,
    PLAYING,
    GAME_OVER,
    GAME_WIN
};

// Character types
enum CharacterType {
    WITCH,
    FOOTBALLER,
    BUSINESSMAN
};

// Menu states
enum MenuSelection {
    MENU_START,
    MENU_CHARACTER,
    MENU_EXIT
};

enum CharacterSelection {
    CHAR_WITCH,
    CHAR_FOOTBALLER,
    CHAR_BUSINESSMAN,
    CHAR_BACK
};

// Game variables
GameState gameState = START_MENU;
int score = 0;
int playerLives = 3;
float gameTime = 0.0f;

// Menu variables
MenuSelection currentMenuSelection = MENU_START;
CharacterSelection currentCharacterSelection = CHAR_WITCH;
CharacterType selectedCharacter = WITCH;
float menuAnimTime = 0.0f;
float logoGlowTime = 0.0f;

// Win/Lose screen button states
enum WinLoseButton {
    BUTTON_RESTART,
    BUTTON_EXIT
};
WinLoseButton currentWinLoseButton = BUTTON_RESTART;

// Player properties
struct Player {
    float x, y;
    float velocityX, velocityY;
    float width, height;
    bool onGround;
    bool hasKey;
    int powerUpType; // 0 = none, 1 = shield, 2 = double jump
    float powerUpTimer;
    bool canDoubleJump;
    bool hasDoubleJumped;
} player;

// Movement input state for smooth acceleration
bool leftPressed = false;
bool rightPressed = false;

// Platform structure
struct Platform {
    float x, y, width, height;
    bool active;
};

// Falling rock structure
struct Rock {
    float x, y;
    bool active;
};

// Collectable structure
struct Collectable {
    float x, y;
    bool collected;
    float animTime;
    int index; // For determining odd/even behavior
};

// Power-up structure
struct PowerUp {
    float x, y;
    int type; // 1 = shield, 2 = double jump
    bool active;
    float lifeTime;
    float animTime;
};

// Game objects
std::vector<Platform> platforms;
std::vector<Rock> rocks;
std::vector<Collectable> collectables;
std::vector<PowerUp> powerUps;
float lavaHeight = 50.0f;
float lavaSpeed = 0.5f;

// Falling character structure for win screen
struct FallingCharacter {
    float x, y;
    CharacterType type;
    float rotationSpeed;
    float rotation;
    float fallSpeed;
    float scale;
    bool active;
};

std::vector<FallingCharacter> fallingCharacters;
float characterSpawnTimer = 0.0f;
bool keySpawned = false;
float keyX, keyY;
float keyAnimTime = 0.0f;
bool keyCollected = false;
float doorAnimTime = 0.0f;
float doorUnlockAnimTime = 0.0f;
float doorEnterAnimTime = 0.0f;
bool doorIsUnlocking = false;
bool doorIsEntering = false;
float rockSpawnTimer = 0.0f;
float powerUpSpawnTimer = 0.0f;

// Jump flip state
float playerAirTime = 0.0f;
float playerFlipAngle = 0.0f;

// Door suction animation state
bool playerBeingSucked = false;
float suctionAnimTime = 0.0f;
float suctionStartX, suctionStartY;
float doorCenterX, doorCenterY;

// Terrain generation patterns
enum TerrainPattern {
    MIDDLE_FOCUSED,
    LEFT_FOCUSED, 
    RIGHT_FOCUSED
};

// Initialize game
void initGame() {
    srand(time(NULL));
    
    // Initialize player with better stats
    player.x = WIDTH / 2.0f;
    player.y = 110.0f; // Start slightly higher
    player.velocityX = 0.0f;
    player.velocityY = 0.0f;
    player.width = 30.0f;
    player.height = 40.0f;
    player.onGround = false;
    player.hasKey = false;
    player.powerUpType = 0;
    player.powerUpTimer = 0.0f;
    player.canDoubleJump = false;
    player.hasDoubleJumped = false;
    
    // Create platforms with different sizes
    platforms.clear();
    
    // Ground platform
    platforms.push_back({0, 80, WIDTH, 20, true});
    
    // Choose random terrain pattern
    TerrainPattern pattern = (TerrainPattern)(rand() % 3);
    
    // Level platforms with challenging, varied generation
    float platformY = 135; // Start slightly higher
    const int baseWidths[4] = {50, 100, 150, 200};
    for (int i = 0; i < 18; i++) { // Slightly more platforms
        // Pick one of the 4 main lengths with variation
        int w = baseWidths[rand() % 4];
        int jitter = (rand() % 21) - 10; // -10..+10 for more variation
        float platformWidth = std::max(40, w + jitter);
        
        // Generate platform position using edge-to-edge offset rule (±50)
        float platformX;
        if (i == 0) {
            platformX = WIDTH / 2 - platformWidth / 2; // Center first platform
        } else {
            const Platform& prev = platforms.back();
            float prevLeft = prev.x;
            float prevRight = prev.x + prev.width;
            bool attachRight = rand() % 2; // Randomly branch left/right
            float minLeft, maxLeft;
            if (attachRight) {
                // New left edge within ±50 of previous right edge
                minLeft = prevRight - 50.0f;
                maxLeft = prevRight + 50.0f - platformWidth;
            } else {
                // New right edge within ±50 of previous left edge
                minLeft = (prevLeft - 50.0f) - platformWidth;
                maxLeft = (prevLeft + 50.0f) - platformWidth;
            }
            // Clamp to screen bounds
            minLeft = std::max(0.0f, minLeft);
            maxLeft = std::min((float)(WIDTH - platformWidth), maxLeft);
            
            if (minLeft <= maxLeft) {
                if (maxLeft - minLeft < 1.0f) platformX = minLeft;
                else platformX = minLeft + (rand() % (int)(maxLeft - minLeft + 1));
            } else {
                // Fallback: near previous center within ±50
                float prevCenter = prev.x + prev.width / 2.0f;
                float fallbackMin = std::max(0.0f, prevCenter - 50.0f - platformWidth / 2.0f);
                float fallbackMax = std::min((float)(WIDTH - platformWidth), prevCenter + 50.0f - platformWidth / 2.0f);
                platformX = (fallbackMin + fallbackMax) * 0.5f;
            }
        }
        
        platforms.push_back({platformX, platformY, platformWidth, 15, true});
        // Vertical spacing tuned for reachability
        platformY += 45 + rand() % 30; // 45..74
    }
    
    // Create collectables (at least 5) - Place them near platforms
    collectables.clear();
    for (int i = 0; i < 8; i++) {
        if (i < platforms.size() - 1) {
            // Place collectables near platforms for easier collection
            float platX = platforms[i + 1].x + platforms[i + 1].width / 2;
            float platY = platforms[i + 1].y + platforms[i + 1].height + 20;
            float x = platX + (rand() % 60 - 30); // Small offset from platform center
            float y = platY + (rand() % 30);
            collectables.push_back({x, y, false, 0.0f, i});
        } else {
            // Backup placement for extra collectables
            float x = 100 + rand() % (WIDTH - 200);
            float y = 250 + i * 60 + rand() % 30;
            collectables.push_back({x, y, false, 0.0f, i});
        }
    }
    
    rocks.clear();
    powerUps.clear();
    
    // Reset door animation states
    doorAnimTime = 0.0f;
    doorUnlockAnimTime = 0.0f;
    doorEnterAnimTime = 0.0f;
    doorIsUnlocking = false;
    doorIsEntering = false;
    
    // Reset suction animation
    playerBeingSucked = false;
    suctionAnimTime = 0.0f;
}

// Check collision between two rectangles
bool checkCollision(float x1, float y1, float w1, float h1,
                   float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 &&
            y1 < y2 + h2 && y1 + h1 > y2);
}

// Draw text
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        text++;
    }
}

// Draw shadowed text with custom color
void drawShadowedText(float x, float y, const char* text, float r, float g, float b) {
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(x + 1, y - 1, text);
    glColor3f(r, g, b);
    drawText(x, y, text);
}

// Forward declarations
void drawBrickPanel(float x, float y, float width, float height, float r, float g, float b);
int measureTextWidth(const char* text);
void drawTextCentered(float cx, float y, const char* text);
void drawShadowedTextCentered(float cx, float y, const char* text, float r, float g, float b);
void drawLayeredBackground();
void initFallingCharacters();

// Draw pixel art Icy Tower logo with glow animation
void drawIcyTowerLogo(float centerX, float centerY) {
    float pixelSize = 8.0f; // Bigger pixels
    logoGlowTime += 0.016f; // Increment glow timer
    
    // Create glow wave that moves from left to right every 3 seconds
    float glowWave = fmod(logoGlowTime, 3.0f) / 3.0f; // 0 to 1 over 3 seconds
    
    // ICY - pixel art pattern (iced blue style) - bigger and more tower-like
    float icyStartX = centerX - 280;
    float icyStartY = centerY + 30;
    
    // I letter pixels - taller and more prominent
    int iPixels[][2] = {
        {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0},
        {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}, {4,8}, {4,9}, {4,10},
        {1,11}, {2,11}, {3,11}, {4,11}, {5,11}, {6,11}, {7,11}
    };
    
    // C letter pixels - bigger and more detailed
    int cPixels[][2] = {
        {11,2}, {12,2}, {13,2}, {14,2}, {15,2}, {16,2},
        {10,3}, {17,3},
        {9,4}, {18,4},
        {9,5}, 
        {9,6},
        {9,7}, {18,7},
        {10,8}, {17,8},
        {11,9}, {12,9}, {13,9}, {14,9}, {15,9}, {16,9}
    };
    
    // Y letter pixels - bigger and more tower-like
    int yPixels[][2] = {
        {21,0}, {27,0},
        {22,1}, {26,1},
        {23,2}, {25,2},
        {24,3}, {24,4}, {24,5}, {24,6}, {24,7}, {24,8}, {24,9}, {24,10}, {24,11}
    };
    
    // Draw ICY with ice-blue colors and glow effect
    for (int i = 0; i < 19; i++) { // Updated count
        float pixelX = icyStartX + iPixels[i][0] * pixelSize;
        float pixelY = icyStartY - iPixels[i][1] * pixelSize;
        float glowIntensity = 1.0f;
        
        // Calculate glow based on horizontal position
        float normalizedX = (float)iPixels[i][0] / 28.0f; // Normalize across full logo width
        if (glowWave > normalizedX - 0.1f && glowWave < normalizedX + 0.1f) {
            glowIntensity = 1.5f + 0.5f * sin((glowWave - normalizedX) * 50.0f);
        }
        
        // Ice blue colors with glow
        glColor3f(0.4f * glowIntensity, 0.8f * glowIntensity, 1.0f * glowIntensity);
        glBegin(GL_QUADS);
        glVertex2f(pixelX, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY + pixelSize);
        glVertex2f(pixelX, pixelY + pixelSize);
        glEnd();
        
        // Ice crystal effect on some pixels
        if (i % 3 == 0) {
            glColor3f(0.8f * glowIntensity, 0.9f * glowIntensity, 1.0f * glowIntensity);
            glBegin(GL_LINES);
            glVertex2f(pixelX + 1, pixelY + 1);
            glVertex2f(pixelX + pixelSize - 1, pixelY + pixelSize - 1);
            glVertex2f(pixelX + pixelSize - 1, pixelY + 1);
            glVertex2f(pixelX + 1, pixelY + pixelSize - 1);
            glEnd();
        }
    }
    
    // Draw C
    for (int i = 0; i < 18; i++) { // Updated count
        float pixelX = icyStartX + cPixels[i][0] * pixelSize;
        float pixelY = icyStartY - cPixels[i][1] * pixelSize;
        float glowIntensity = 1.0f;
        
        float normalizedX = (float)cPixels[i][0] / 28.0f;
        if (glowWave > normalizedX - 0.1f && glowWave < normalizedX + 0.1f) {
            glowIntensity = 1.5f + 0.5f * sin((glowWave - normalizedX) * 50.0f);
        }
        
        glColor3f(0.4f * glowIntensity, 0.8f * glowIntensity, 1.0f * glowIntensity);
        glBegin(GL_QUADS);
        glVertex2f(pixelX, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY + pixelSize);
        glVertex2f(pixelX, pixelY + pixelSize);
        glEnd();
        
        if (i % 2 == 0) {
            glColor3f(0.8f * glowIntensity, 0.9f * glowIntensity, 1.0f * glowIntensity);
            glBegin(GL_LINES);
            glVertex2f(pixelX + 1, pixelY + 1);
            glVertex2f(pixelX + pixelSize - 1, pixelY + pixelSize - 1);
            glEnd();
        }
    }
    
    // Draw Y
    for (int i = 0; i < 15; i++) { // Updated count
        float pixelX = icyStartX + yPixels[i][0] * pixelSize;
        float pixelY = icyStartY - yPixels[i][1] * pixelSize;
        float glowIntensity = 1.0f;
        
        float normalizedX = (float)yPixels[i][0] / 28.0f;
        if (glowWave > normalizedX - 0.1f && glowWave < normalizedX + 0.1f) {
            glowIntensity = 1.5f + 0.5f * sin((glowWave - normalizedX) * 50.0f);
        }
        
        glColor3f(0.4f * glowIntensity, 0.8f * glowIntensity, 1.0f * glowIntensity);
        glBegin(GL_QUADS);
        glVertex2f(pixelX, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY);
        glVertex2f(pixelX + pixelSize, pixelY + pixelSize);
        glVertex2f(pixelX, pixelY + pixelSize);
        glEnd();
        
        if (i % 3 == 1) {
            glColor3f(0.8f * glowIntensity, 0.9f * glowIntensity, 1.0f * glowIntensity);
            glBegin(GL_LINES);
            glVertex2f(pixelX + 1, pixelY + 1);
            glVertex2f(pixelX + pixelSize - 1, pixelY + 1);
            glEnd();
        }
    }
    
    // TOWER - brick style pixels (bigger and more prominent)
    float towerStartX = centerX - 200;
    float towerStartY = centerY - 50;
    
    // T letter pixels - bigger
    int tPixels[][2] = {
        {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0},
        {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7}, {3,8}, {3,9}
    };
    
    // O letter pixels - bigger
    int oPixels[][2] = {
        {9,1}, {10,1}, {11,1}, {12,1}, {13,1}, {14,1},
        {8,2}, {15,2},
        {8,3}, {15,3}, 
        {8,4}, {15,4},
        {8,5}, {15,5},
        {8,6}, {15,6},
        {8,7}, {15,7},
        {9,8}, {10,8}, {11,8}, {12,8}, {13,8}, {14,8}
    };
    
    // W letter pixels - bigger
    int wPixels[][2] = {
        {18,0}, {24,0},
        {18,1}, {24,1},
        {18,2}, {24,2},
        {18,3}, {24,3},
        {18,4}, {21,4}, {24,4},
        {18,5}, {20,5}, {22,5}, {24,5},
        {18,6}, {19,6}, {23,6}, {24,6},
        {18,7}, {24,7},
        {18,8}, {24,8},
        {18,9}, {24,9}
    };
    
    // E letter pixels - bigger
    int ePixels[][2] = {
        {27,0}, {28,0}, {29,0}, {30,0}, {31,0}, {32,0},
        {27,1}, {27,2}, {27,3}, {27,4}, 
        {27,5}, {28,5}, {29,5}, {30,5},
        {27,6}, {27,7}, {27,8},
        {27,9}, {28,9}, {29,9}, {30,9}, {31,9}, {32,9}
    };
    
    // R letter pixels - bigger
    int rPixels[][2] = {
        {35,0}, {36,0}, {37,0}, {38,0}, {39,0},
        {35,1}, {40,1},
        {35,2}, {40,2},
        {35,3}, {40,3},
        {35,4}, {36,4}, {37,4}, {38,4},
        {35,5}, {38,5},
        {35,6}, {39,6},
        {35,7}, {40,7},
        {35,8}, {40,8},
        {35,9}, {40,9}
    };
    
    // Draw TOWER with brick colors
    auto drawBrickPixel = [&](float px, float py) {
        // Main brick color
        glColor3f(0.6f, 0.3f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2f(px, py);
        glVertex2f(px + pixelSize, py);
        glVertex2f(px + pixelSize, py + pixelSize);
        glVertex2f(px, py + pixelSize);
        glEnd();
        
        // Mortar lines
        glColor3f(0.4f, 0.2f, 0.1f);
        glBegin(GL_LINES);
        glVertex2f(px, py); glVertex2f(px + pixelSize, py);
        glVertex2f(px, py); glVertex2f(px, py + pixelSize);
        glEnd();
        
        // Highlight
        glColor3f(0.8f, 0.5f, 0.3f);
        glBegin(GL_LINES);
        glVertex2f(px + 1, py + 1);
        glVertex2f(px + pixelSize - 1, py + 1);
        glVertex2f(px + 1, py + 1);
        glVertex2f(px + 1, py + pixelSize - 1);
        glEnd();
    };
    
    // Draw each letter with updated counts
    for (int i = 0; i < 16; i++) { // T letter
        drawBrickPixel(towerStartX + tPixels[i][0] * pixelSize, towerStartY - tPixels[i][1] * pixelSize);
    }
    for (int i = 0; i < 22; i++) { // O letter
        drawBrickPixel(towerStartX + oPixels[i][0] * pixelSize, towerStartY - oPixels[i][1] * pixelSize);
    }
    for (int i = 0; i < 20; i++) { // W letter
        drawBrickPixel(towerStartX + wPixels[i][0] * pixelSize, towerStartY - wPixels[i][1] * pixelSize);
    }
    for (int i = 0; i < 17; i++) { // E letter
        drawBrickPixel(towerStartX + ePixels[i][0] * pixelSize, towerStartY - ePixels[i][1] * pixelSize);
    }
    for (int i = 0; i < 20; i++) { // R letter
        drawBrickPixel(towerStartX + rPixels[i][0] * pixelSize, towerStartY - rPixels[i][1] * pixelSize);
    }
}

// Brick-style UI panel with optional soft shadow
void drawBrickPanelWithShadow(float x, float y, float width, float height, float r, float g, float b, float shadowAlpha = 0.25f) {
    // Soft shadow
    glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);
    glBegin(GL_QUADS);
    glVertex2f(x + 2, y - 2);
    glVertex2f(x + width + 2, y - 2);
    glVertex2f(x + width + 2, y + height - 2);
    glVertex2f(x + 2, y + height - 2);
    glEnd();
    
    // Panel itself
    drawBrickPanel(x, y, width, height, r, g, b);
}

// Draw brick-style UI panel
void drawBrickPanel(float x, float y, float width, float height, float r, float g, float b) {
    // Background
    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    
    // Brick pattern on UI
    int smallBrickW = 15;
    int smallBrickH = 5;
    
    for (float by = y; by < y + height; by += smallBrickH) {
        for (float bx = x; bx < x + width; bx += smallBrickW) {
            int offsetX = ((int)by / smallBrickH) % 2 == 0 ? 0 : smallBrickW / 2;
            float brickX = bx + offsetX;
            
            if (brickX >= x + width) continue;
            float actualWidth = std::min((float)smallBrickW, x + width - brickX);
            
            // Slightly lighter brick color for UI
            glColor3f(r * 0.9f, g * 0.9f, b * 0.9f);
            glBegin(GL_QUADS);
            glVertex2f(brickX, by);
            glVertex2f(brickX + actualWidth - 1, by);
            glVertex2f(brickX + actualWidth - 1, by + smallBrickH - 1);
            glVertex2f(brickX, by + smallBrickH - 1);
            glEnd();
        }
    }
    
    // Border
    glColor3f(r * 1.2f, g * 1.2f, b * 1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Draw witch character (4+ primitives: dress, hat, hands, broomstick)
void drawWitch(float x, float y, bool inMenu = false) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    if (inMenu) glScalef(2.0f, 2.0f, 1.0f); // Bigger in menu
    
    // Shield effect if active (only in game)
    if (!inMenu && player.powerUpType == 1) {
        glColor3f(0.5f, 0.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(15 + 25 * cos(angle), 20 + 25 * sin(angle));
        }
        glEnd();
    }
    
    // Dress (trapezoid using triangles)
    glColor3f(0.2f, 0.0f, 0.4f); // Dark purple
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 5);  // Top center
    glVertex2f(5, 25);  // Bottom left
    glVertex2f(25, 25); // Bottom right
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 5);  // Top center
    glVertex2f(10, 5);  // Top left
    glVertex2f(5, 25);  // Bottom left
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 5);  // Top center
    glVertex2f(25, 25); // Bottom right
    glVertex2f(20, 5);  // Top right
    glEnd();
    
    // Witch hat (triangle)
    glColor3f(0.1f, 0.0f, 0.2f);
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 45); // Top
    glVertex2f(8, 25);  // Left
    glVertex2f(22, 25); // Right
    glEnd();
    
    // Hat brim (rectangle)
    glBegin(GL_QUADS);
    glVertex2f(6, 25);
    glVertex2f(24, 25);
    glVertex2f(24, 28);
    glVertex2f(6, 28);
    glEnd();
    
    // Hands (circles)
    glColor3f(0.8f, 0.6f, 0.4f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(-2 + 3 * cos(angle), 15 + 3 * sin(angle));
    }
    glEnd();
    
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(32 + 3 * cos(angle), 15 + 3 * sin(angle));
    }
    glEnd();
    
    // Broomstick (rectangle)
    glColor3f(0.6f, 0.3f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(30, 12);
    glVertex2f(45, 10);
    glVertex2f(45, 14);
    glVertex2f(30, 16);
    glEnd();
    
    // Broom bristles (triangles)
    glColor3f(0.8f, 0.7f, 0.3f);
    for (int i = 0; i < 3; i++) {
        glBegin(GL_TRIANGLES);
        glVertex2f(45, 8 + i * 3);
        glVertex2f(52, 6 + i * 4);
        glVertex2f(45, 10 + i * 3);
        glEnd();
    }
    
    glPopMatrix();
}

// Draw footballer character (4+ primitives: jersey, shorts, boots, ball)
void drawFootballer(float x, float y, bool inMenu = false) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    if (inMenu) glScalef(2.0f, 2.0f, 1.0f);
    
    // Shield effect if active (only in game)
    if (!inMenu && player.powerUpType == 1) {
        glColor3f(0.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(15 + 25 * cos(angle), 20 + 25 * sin(angle));
        }
        glEnd();
    }
    
    // Jersey (rectangle)
    glColor3f(0.0f, 0.8f, 0.0f); // Green jersey
    glBegin(GL_QUADS);
    glVertex2f(8, 15);
    glVertex2f(22, 15);
    glVertex2f(22, 28);
    glVertex2f(8, 28);
    glEnd();
    
    // Jersey number (rectangle)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(12, 20);
    glVertex2f(18, 20);
    glVertex2f(18, 25);
    glVertex2f(12, 25);
    glEnd();
    
    // Shorts (rectangle)
    glColor3f(0.0f, 0.0f, 0.8f); // Blue shorts
    glBegin(GL_QUADS);
    glVertex2f(9, 8);
    glVertex2f(21, 8);
    glVertex2f(21, 15);
    glVertex2f(9, 15);
    glEnd();
    
    // Head (circle)
    glColor3f(1.0f, 0.8f, 0.6f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 16; i++) {
        float angle = 2.0f * M_PI * i / 16;
        glVertex2f(15 + 6 * cos(angle), 34 + 6 * sin(angle));
    }
    glEnd();
    
    // Arms (rectangles)
    glColor3f(1.0f, 0.8f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(4, 20);
    glVertex2f(8, 20);
    glVertex2f(8, 26);
    glVertex2f(4, 26);
    glEnd();
    
    glBegin(GL_QUADS);
    glVertex2f(22, 20);
    glVertex2f(26, 20);
    glVertex2f(26, 26);
    glVertex2f(22, 26);
    glEnd();
    
    // Football boots (rectangles)
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(8, 0);
    glVertex2f(14, 0);
    glVertex2f(14, 8);
    glVertex2f(8, 8);
    glEnd();
    
    glBegin(GL_QUADS);
    glVertex2f(16, 0);
    glVertex2f(22, 0);
    glVertex2f(22, 8);
    glVertex2f(16, 8);
    glEnd();
    
    // Football (circle)
    if (inMenu) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 12; i++) {
            float angle = 2.0f * M_PI * i / 12;
            glVertex2f(35 + 6 * cos(angle), 15 + 6 * sin(angle));
        }
        glEnd();
        
        // Football pattern (lines)
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex2f(32, 15); glVertex2f(38, 15);
        glVertex2f(35, 12); glVertex2f(35, 18);
        glEnd();
    }
    
    glPopMatrix();
}

// Draw businessman character (4+ primitives: suit jacket, tie, briefcase, dress shoes)
void drawBusinessman(float x, float y, bool inMenu = false) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    if (inMenu) glScalef(2.0f, 2.0f, 1.0f);
    
    // Shield effect if active (only in game)
    if (!inMenu && player.powerUpType == 1) {
        glColor3f(0.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(15 + 25 * cos(angle), 20 + 25 * sin(angle));
        }
        glEnd();
    }
    
    // Suit jacket (rectangle)
    glColor3f(0.2f, 0.2f, 0.2f); // Dark gray suit
    glBegin(GL_QUADS);
    glVertex2f(7, 10);
    glVertex2f(23, 10);
    glVertex2f(23, 28);
    glVertex2f(7, 28);
    glEnd();
    
    // Shirt (rectangle)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(11, 15);
    glVertex2f(19, 15);
    glVertex2f(19, 28);
    glVertex2f(11, 28);
    glEnd();
    
    // Tie (triangle)
    glColor3f(0.8f, 0.0f, 0.0f); // Red tie
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 28);
    glVertex2f(13, 18);
    glVertex2f(17, 18);
    glEnd();
    
    // Suit pants (rectangle)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(9, 2);
    glVertex2f(21, 2);
    glVertex2f(21, 10);
    glVertex2f(9, 10);
    glEnd();
    
    // Head (circle)
    glColor3f(1.0f, 0.8f, 0.6f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 16; i++) {
        float angle = 2.0f * M_PI * i / 16;
        glVertex2f(15 + 6 * cos(angle), 34 + 6 * sin(angle));
    }
    glEnd();
    
    // Arms (rectangles)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(3, 18);
    glVertex2f(7, 18);
    glVertex2f(7, 26);
    glVertex2f(3, 26);
    glEnd();
    
    glBegin(GL_QUADS);
    glVertex2f(23, 18);
    glVertex2f(27, 18);
    glVertex2f(27, 26);
    glVertex2f(23, 26);
    glEnd();
    
    // Dress shoes (rectangles)
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(8, 0);
    glVertex2f(14, 0);
    glVertex2f(14, 4);
    glVertex2f(8, 4);
    glEnd();
    
    glBegin(GL_QUADS);
    glVertex2f(16, 0);
    glVertex2f(22, 0);
    glVertex2f(22, 4);
    glVertex2f(16, 4);
    glEnd();
    
    // Briefcase (rectangle)
    if (inMenu) {
        glColor3f(0.4f, 0.2f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(30, 12);
        glVertex2f(42, 12);
        glVertex2f(42, 20);
        glVertex2f(30, 20);
        glEnd();
        
        // Briefcase handle (rectangle)
        glColor3f(0.2f, 0.1f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(34, 20);
        glVertex2f(38, 20);
        glVertex2f(38, 22);
        glVertex2f(34, 22);
        glEnd();
    }
    
    glPopMatrix();
}

// Draw player based on selected character (with jump flip rotation)
void drawPlayer() {
    float pivotX = player.x + player.width / 2.0f;
    float pivotY = player.y + player.height / 2.0f;
    glPushMatrix();
    glTranslatef(pivotX, pivotY, 0);
    glRotatef(playerFlipAngle, 0, 0, 1); // Negative angles = clockwise
    glTranslatef(-pivotX, -pivotY, 0);
    
    switch (selectedCharacter) {
        case WITCH:
            drawWitch(player.x, player.y, false);
            break;
        case FOOTBALLER:
            drawFootballer(player.x, player.y, false);
            break;
        case BUSINESSMAN:
            drawBusinessman(player.x, player.y, false);
            break;
    }
    glPopMatrix();
}

// Draw platforms (3+ primitives: rectangle base, triangle decoration, line borders)
void drawPlatforms() {
    for (const auto& platform : platforms) {
        if (!platform.active || platform.y < lavaHeight) continue;
        
        glPushMatrix();
        glTranslatef(platform.x, platform.y, 0);
        
        // Platform base (rectangle)
        glColor3f(0.4f, 0.8f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(platform.width, 0);
        glVertex2f(platform.width, platform.height);
        glVertex2f(0, platform.height);
        glEnd();
        
        // Decorative triangles on top
        glColor3f(0.2f, 0.6f, 0.1f);
        for (float i = 10; i < platform.width - 10; i += 20) {
            glBegin(GL_TRIANGLES);
            glVertex2f(i, platform.height);
            glVertex2f(i + 5, platform.height + 5);
            glVertex2f(i + 10, platform.height);
            glEnd();
        }
        
        // Border lines
        glColor3f(0.1f, 0.4f, 0.05f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0);
        glVertex2f(platform.width, 0);
        glVertex2f(platform.width, platform.height);
        glVertex2f(0, platform.height);
        glEnd();
        
        glPopMatrix();
    }
}

// Draw lava (2+ primitives: rectangle base, wavy triangles on top)
void drawLava() {
    // Lava base (rectangle)
    glColor3f(1.0f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, lavaHeight);
    glVertex2f(0, lavaHeight);
    glEnd();
    
    // Wavy flame effect on top (triangles)
    glColor3f(1.0f, 0.8f, 0.0f);
    float waveOffset = sin(gameTime * 5) * 5;
    for (float i = 0; i < WIDTH; i += 20) {
        float height = 10 + sin((i + gameTime * 100) * 0.1f) * 5;
        glBegin(GL_TRIANGLES);
        glVertex2f(i, lavaHeight);
        glVertex2f(i + 10, lavaHeight + height + waveOffset);
        glVertex2f(i + 20, lavaHeight);
        glEnd();
    }
}

// Draw rocks (2+ primitives: hexagon body, triangle spike)
void drawRocks() {
    for (const auto& rock : rocks) {
        if (!rock.active) continue;
        
        glPushMatrix();
        glTranslatef(rock.x, rock.y, 0);
        
        // Rock body (hexagon)
        glColor3f(0.6f, 0.4f, 0.2f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 6; i++) {
            float angle = i * M_PI / 3;
            glVertex2f(10 * cos(angle), 10 * sin(angle));
        }
        glEnd();
        
        // Dangerous spike (triangle)
        glColor3f(0.8f, 0.2f, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, 12);
        glVertex2f(-5, 5);
        glVertex2f(5, 5);
        glEnd();
        
        glPopMatrix();
    }
}

// Draw collectables with 3D-like Y-axis rotation illusion (3+ primitives: circle, line loop, triangle fan, quad)
void drawCollectables() {
    for (const auto& collectable : collectables) {
        if (collectable.collected) continue;

        glPushMatrix();

        // Horizontal movement for odd-numbered coins (±20 pixels max)
        float horizontalOffset = 0.0f;
        if (collectable.index % 2 == 1) {
            horizontalOffset = sinf(collectable.animTime * 2.0f) * 20.0f;
        }

        glTranslatef(collectable.x + horizontalOffset, collectable.y, 0);

        // Y-axis flip illusion using X-scale squash and overall size modulation
        float t = (sinf(collectable.animTime * 4.0f) + 1.0f) * 0.5f; // 0..1
        float xScale = 0.25f + 0.75f * t; // Thin at edge, full when face-on
        float overall = 0.8f + 0.4f * t;  // Larger when face-on
        glScalef(overall * xScale, overall, 1.0f);

        // Base coin (ellipse due to X scaling) - PRIMITIVE 1: Polygon
        glColor3f(1.0f, 0.82f, 0.1f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 24; i++) {
            float angle = 2.0f * M_PI * i / 24;
            glVertex2f(10.0f * cosf(angle), 10.0f * sinf(angle));
        }
        glEnd();

        // Rim ring - PRIMITIVE 2: Line loop
        glColor3f(1.0f, 0.9f, 0.3f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 24; i++) {
            float angle = 2.0f * M_PI * i / 24;
            glVertex2f(9.0f * cosf(angle), 9.0f * sinf(angle));
        }
        glEnd();

        // Radial highlight - PRIMITIVE 3: Triangle fan gradient
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(1.0f, 0.98f, 0.6f); // center bright
        glVertex2f(0.0f, 0.0f); // centered for Y-axis rotation
        glColor3f(1.0f, 0.85f, 0.2f); // outer gold
        for (int i = 0; i <= 24; i++) {
            float angle = 2.0f * M_PI * i / 24;
            glVertex2f(10.0f * cosf(angle), 10.0f * sinf(angle));
        }
        glEnd();

        // Specular streak across face - PRIMITIVE 4: Quad
        glColor4f(1.0f, 1.0f, 1.0f, 0.35f);
        glBegin(GL_QUADS);
        glVertex2f(-7.0f, 3.0f);
        glVertex2f(7.0f, 3.0f);
        glVertex2f(7.0f, 1.0f);
        glVertex2f(-7.0f, 1.0f);
        glEnd();

        // Edge darkening when thin (simulates depth)
        float edgeAlpha = 1.0f - t; // stronger when thinner
        glColor4f(0.6f, 0.4f, 0.1f, 0.4f * edgeAlpha);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 24; i++) {
            float angle = 2.0f * M_PI * i / 24;
            glVertex2f(10.5f * cosf(angle), 10.5f * sinf(angle));
        }
        glEnd();

        glPopMatrix();
    }
}

// Draw key (4+ primitives: rectangle shaft, circle head, triangle teeth, line handle)
void drawKey() {
    if (!keySpawned || keyCollected) return;
    
    glPushMatrix();
    glTranslatef(keyX, keyY, 0);
    glRotatef(sin(keyAnimTime * 3) * 10, 0, 0, 1);
    float scale = 1.0f + 0.1f * sin(keyAnimTime * 4);
    glScalef(scale, scale, 1);
    
    // Key shaft (rectangle)
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(-15, -2);
    glVertex2f(5, -2);
    glVertex2f(5, 2);
    glVertex2f(-15, 2);
    glEnd();
    
    // Key head (circle)
    glColor3f(1.0f, 0.9f, 0.2f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(-15 + 6 * cos(angle), 6 * sin(angle));
    }
    glEnd();
    
    // Key teeth (triangles)
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(5, -2);
    glVertex2f(10, -2);
    glVertex2f(10, 0);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(5, 2);
    glVertex2f(8, 2);
    glVertex2f(8, 0);
    glEnd();
    
    // Handle decoration (line)
    glColor3f(0.8f, 0.6f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(-15, -4);
    glVertex2f(-15, 4);
    glEnd();
    
    glPopMatrix();
}

// Draw epic animated door
void drawDoor() {
    float doorX = WIDTH / 2 - 40;
    float doorY = HEIGHT - 120;
    
    glPushMatrix();
    glTranslatef(doorX, doorY, 0);
    
    if (keyCollected || doorIsUnlocking) {
        // Unlocked/Unlocking door with animations
        float unlockProgress = doorIsUnlocking ? std::min(1.0f, doorUnlockAnimTime / 2.0f) : 1.0f;
        float enterProgress = doorIsEntering ? std::min(1.0f, doorEnterAnimTime / 1.5f) : 0.0f;
        
        // Magical portal frame (hexagon)
        glColor3f(0.2f + unlockProgress * 0.6f, 0.8f, 0.2f + unlockProgress * 0.6f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 6; i++) {
            float angle = M_PI / 2 + i * M_PI / 3;
            glVertex2f(40 + 45 * cos(angle), 60 + 50 * sin(angle));
        }
        glEnd();
        
        // Inner portal (darker hexagon)
        glColor3f(0.1f, 0.3f, 0.1f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 6; i++) {
            float angle = M_PI / 2 + i * M_PI / 3;
            glVertex2f(40 + 38 * cos(angle), 60 + 43 * sin(angle));
        }
        glEnd();
        
        // Swirling energy vortex
        for (int layer = 0; layer < 3; layer++) {
            float layerOffset = layer * 0.5f;
            float rotation = doorAnimTime * 2.0f + layerOffset;
            float radius = 35 - layer * 8;
            float alpha = 0.3f - layer * 0.08f;
            
            glColor4f(0.2f + unlockProgress * 0.5f, 1.0f, 0.2f + unlockProgress * 0.5f, alpha * unlockProgress);
            
            for (int i = 0; i < 8; i++) {
                float angle1 = rotation + i * M_PI / 4;
                float angle2 = rotation + (i + 0.5f) * M_PI / 4;
                
                glBegin(GL_TRIANGLES);
                glVertex2f(40, 60);
                glVertex2f(40 + radius * cos(angle1), 60 + radius * sin(angle1));
                glVertex2f(40 + radius * cos(angle2), 60 + radius * sin(angle2));
                glEnd();
            }
        }
        
        // Pulsing outer glow rings
        float pulseSize = sin(doorAnimTime * 3.0f) * 5 + 50;
        float pulseAlpha = (sin(doorAnimTime * 3.0f) * 0.3f + 0.5f) * unlockProgress;
        
        glColor4f(0.0f, 1.0f, 0.0f, pulseAlpha);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(40 + pulseSize * cos(angle), 60 + pulseSize * sin(angle));
        }
        glEnd();
        
        glColor4f(0.0f, 1.0f, 0.5f, pulseAlpha * 0.6f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(40 + (pulseSize + 5) * cos(angle), 60 + (pulseSize + 5) * sin(angle));
        }
        glEnd();
        
        // Entrance animation - player being sucked in
        if (doorIsEntering) {
            // Bright flash effect
            glColor4f(1.0f, 1.0f, 1.0f, (1.0f - enterProgress) * 0.7f);
            glBegin(GL_POLYGON);
            for (int i = 0; i < 12; i++) {
                float angle = 2.0f * M_PI * i / 12;
                float flashRadius = 60 * (1.0f - enterProgress);
                glVertex2f(40 + flashRadius * cos(angle), 60 + flashRadius * sin(angle));
            }
            glEnd();
            
            // Spiraling particles being sucked in
            for (int i = 0; i < 12; i++) {
                float particleAngle = doorEnterAnimTime * 5.0f + i * M_PI / 6;
                float particleRadius = 70 * (1.0f - enterProgress);
                
                glColor4f(1.0f, 1.0f, 0.0f, 1.0f - enterProgress);
                glBegin(GL_POLYGON);
                for (int j = 0; j < 6; j++) {
                    float angle = 2.0f * M_PI * j / 6;
                    glVertex2f(40 + particleRadius * cos(particleAngle) + 4 * cos(angle),
                              60 + particleRadius * sin(particleAngle) + 4 * sin(angle));
                }
                glEnd();
            }
        }
        
        // Unlock animation - expanding energy waves
        if (doorIsUnlocking && doorUnlockAnimTime < 2.0f) {
            for (int wave = 0; wave < 3; wave++) {
                float waveTime = doorUnlockAnimTime - wave * 0.3f;
                if (waveTime > 0) {
                    float waveRadius = waveTime * 50;
                    float waveAlpha = std::max(0.0f, 1.0f - waveTime / 2.0f);
                    
                    glColor4f(1.0f, 1.0f, 0.0f, waveAlpha * 0.6f);
                    glBegin(GL_LINE_LOOP);
                    for (int i = 0; i < 24; i++) {
                        float angle = 2.0f * M_PI * i / 24;
                        glVertex2f(40 + waveRadius * cos(angle), 60 + waveRadius * sin(angle));
                    }
                    glEnd();
                }
            }
        }
        
    } else {
        // Locked door - ancient magical sealed door
        // Stone archway frame (trapezoid)
        glColor3f(0.4f, 0.4f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(5, 0);
        glVertex2f(75, 0);
        glVertex2f(70, 110);
        glVertex2f(10, 110);
        glEnd();
        
        // Arch top (semi-circle)
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 10; i++) {
            float angle = M_PI * i / 10;
            glVertex2f(40 + 30 * cos(angle), 110 + 30 * sin(angle));
        }
        glEnd();
        
        // Inner door surface (darker)
        glColor3f(0.2f, 0.15f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(15, 5);
        glVertex2f(65, 5);
        glVertex2f(62, 105);
        glVertex2f(18, 105);
        glEnd();
        
        // Door panels (rectangles)
        glColor3f(0.25f, 0.2f, 0.35f);
        glBegin(GL_QUADS);
        glVertex2f(20, 10);
        glVertex2f(35, 10);
        glVertex2f(35, 50);
        glVertex2f(20, 50);
        glEnd();
        
        glBegin(GL_QUADS);
        glVertex2f(45, 10);
        glVertex2f(60, 10);
        glVertex2f(60, 50);
        glVertex2f(45, 50);
        glEnd();
        
        glBegin(GL_QUADS);
        glVertex2f(20, 60);
        glVertex2f(35, 60);
        glVertex2f(35, 100);
        glVertex2f(20, 100);
        glEnd();
        
        glBegin(GL_QUADS);
        glVertex2f(45, 60);
        glVertex2f(60, 60);
        glVertex2f(60, 100);
        glVertex2f(45, 100);
        glEnd();
        
        // Mystical seal/lock in center (circle with runes)
        glColor3f(0.6f, 0.3f, 0.8f); // Purple glow
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float angle = 2.0f * M_PI * i / 16;
            glVertex2f(40 + 15 * cos(angle), 55 + 15 * sin(angle));
        }
        glEnd();
        
        // Inner seal
        glColor3f(0.4f, 0.2f, 0.6f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float angle = 2.0f * M_PI * i / 16;
            glVertex2f(40 + 10 * cos(angle), 55 + 10 * sin(angle));
        }
        glEnd();
        
        // Keyhole (star shape)
        glColor3f(0.1f, 0.0f, 0.2f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 8; i++) {
            float angle = 2.0f * M_PI * i / 8;
            float radius = (i % 2 == 0) ? 6.0f : 3.0f;
            glVertex2f(40 + radius * cos(angle), 55 + radius * sin(angle));
        }
        glEnd();
        
        // Pulsing magical chains/runes around door
        float runeGlow = sin(doorAnimTime * 2.0f) * 0.3f + 0.5f;
        glColor4f(0.8f, 0.3f, 1.0f, runeGlow);
        
        // Rune symbols (simple geometric shapes)
        for (int i = 0; i < 4; i++) {
            float runeX = (i % 2 == 0) ? 10 : 70;
            float runeY = 30 + (i / 2) * 50;
            
            glBegin(GL_LINE_LOOP);
            for (int j = 0; j < 3; j++) {
                float angle = 2.0f * M_PI * j / 3 + doorAnimTime;
                glVertex2f(runeX + 5 * cos(angle), runeY + 5 * sin(angle));
            }
            glEnd();
        }
    }
    
    glPopMatrix();
}

// Draw power-ups
void drawPowerUps() {
    for (const auto& powerUp : powerUps) {
        if (!powerUp.active) continue;
        
        glPushMatrix();
        glTranslatef(powerUp.x, powerUp.y, 0);
        float bob = sin(powerUp.animTime * 3) * 3;
        glTranslatef(0, bob, 0);
        glRotatef(powerUp.animTime * 50, 0, 0, 1);
        
        if (powerUp.type == 1) { // Shield power-up
            // Shield base (hexagon)
            glColor3f(0.0f, 0.8f, 1.0f);
            glBegin(GL_POLYGON);
            for (int i = 0; i < 6; i++) {
                float angle = i * M_PI / 3;
                glVertex2f(10 * cos(angle), 10 * sin(angle));
            }
            glEnd();
            
            // Shield cross (lines)
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
            glVertex2f(-8, 0); glVertex2f(8, 0);
            glVertex2f(0, -8); glVertex2f(0, 8);
            glEnd();
            
            // Outer glow (triangle)
            glColor3f(0.5f, 0.9f, 1.0f);
            for (int i = 0; i < 6; i++) {
                float angle = i * M_PI / 3;
                glBegin(GL_TRIANGLES);
                glVertex2f(0, 0);
                glVertex2f(12 * cos(angle), 12 * sin(angle));
                glVertex2f(12 * cos(angle + M_PI/3), 12 * sin(angle + M_PI/3));
                glEnd();
            }
        } else if (powerUp.type == 2) { // Double jump power-up
            // Wing base (triangles)
            glColor3f(1.0f, 0.8f, 0.2f);
            glBegin(GL_TRIANGLES);
            glVertex2f(-15, -5);
            glVertex2f(-5, 5);
            glVertex2f(-15, 10);
            glEnd();
            
            glBegin(GL_TRIANGLES);
            glVertex2f(15, -5);
            glVertex2f(5, 5);
            glVertex2f(15, 10);
            glEnd();
            
            // Center orb (circle)
            glColor3f(1.0f, 1.0f, 0.0f);
            glBegin(GL_POLYGON);
            for (int i = 0; i < 12; i++) {
                float angle = 2.0f * M_PI * i / 12;
                glVertex2f(6 * cos(angle), 6 * sin(angle));
            }
            glEnd();
            
            // Speed lines (lines)
            glColor3f(1.0f, 0.9f, 0.7f);
            glBegin(GL_LINES);
            for (int i = 0; i < 4; i++) {
                float angle = i * M_PI / 2;
                glVertex2f(8 * cos(angle), 8 * sin(angle));
                glVertex2f(15 * cos(angle), 15 * sin(angle));
            }
            glEnd();
        }
        
        glPopMatrix();
    }
}

// Simple icons for HUD
void drawHeartIcon(float x, float y, float s) {
    glColor3f(0.9f, 0.1f, 0.2f);
    // Left lobe
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float a = 2.0f * M_PI * i / 12;
        glVertex2f(x - 3*s + 3*s * cos(a), y + 2*s + 3*s * sin(a));
    }
    glEnd();
    // Right lobe
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float a = 2.0f * M_PI * i / 12;
        glVertex2f(x + 3*s + 3*s * cos(a), y + 2*s + 3*s * sin(a));
    }
    glEnd();
    // Bottom triangle
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 6*s, y + 2*s);
    glVertex2f(x + 6*s, y + 2*s);
    glVertex2f(x, y - 6*s);
    glEnd();
}

void drawCoinIcon(float x, float y, float s) {
    glColor3f(1.0f, 0.85f, 0.1f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 16; i++) {
        float a = 2.0f * M_PI * i / 16;
        glVertex2f(x + 5*s * cos(a), y + 5*s * sin(a));
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 0.9f);
    glBegin(GL_LINES);
    glVertex2f(x - 3*s, y);
    glVertex2f(x + 3*s, y);
    glEnd();
}

void drawKeyIcon(float x, float y, float s) {
    glColor3f(1.0f, 0.9f, 0.2f);
    // Head
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float a = 2.0f * M_PI * i / 12;
        glVertex2f(x - 6*s + 4*s * cos(a), y + 4*s * sin(a));
    }
    glEnd();
    // Shaft
    glBegin(GL_QUADS);
    glVertex2f(x - 2*s, y - 1*s);
    glVertex2f(x + 8*s, y - 1*s);
    glVertex2f(x + 8*s, y + 1*s);
    glVertex2f(x - 2*s, y + 1*s);
    glEnd();
    // Teeth
    glBegin(GL_TRIANGLES);
    glVertex2f(x + 8*s, y - 1*s);
    glVertex2f(x + 11*s, y - 1*s);
    glVertex2f(x + 11*s, y + 1*s);
    glEnd();
}

// Draw HUD
void drawHUD() {
    // Main HUD panel with brick texture and shadow
    drawBrickPanelWithShadow(5, HEIGHT - 50, WIDTH - 10, 45, 0.4f, 0.4f, 0.6f);
    
    // Health section with brick panel
    drawBrickPanelWithShadow(15, HEIGHT - 35, 200, 20, 0.3f, 0.5f, 0.3f);
    
    drawShadowedText(20, HEIGHT - 20, "Health:", 1.0f, 1.0f, 1.0f);
    
    // Heart icon
    drawHeartIcon(65, HEIGHT - 22, 1.0f);
    
    // Health bar (2+ primitives: rectangle background, rectangle health)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(75, HEIGHT - 30);
    glVertex2f(175, HEIGHT - 30);
    glVertex2f(175, HEIGHT - 15);
    glVertex2f(75, HEIGHT - 15);
    glEnd();
    
    // Health bar fill with color coding
    float healthRatio = (float)playerLives / 3.0f;
    if (healthRatio > 0.6f) glColor3f(0.2f, 0.8f, 0.2f); // Green
    else if (healthRatio > 0.3f) glColor3f(0.8f, 0.8f, 0.2f); // Yellow
    else glColor3f(0.8f, 0.2f, 0.2f); // Red
    
    float healthWidth = 100.0f * healthRatio;
    glBegin(GL_QUADS);
    glVertex2f(75, HEIGHT - 30);
    glVertex2f(75 + healthWidth, HEIGHT - 30);
    glVertex2f(75 + healthWidth, HEIGHT - 15);
    glVertex2f(75, HEIGHT - 15);
    glEnd();
    
    // Lives text
    std::stringstream livesText;
    livesText << playerLives << "/3";
    drawShadowedText(185, HEIGHT - 25, livesText.str().c_str(), 1.0f, 1.0f, 1.0f);
    
    // Lava danger section with brick panel
    drawBrickPanelWithShadow(230, HEIGHT - 35, 200, 20, 0.5f, 0.3f, 0.3f);
    
    drawShadowedText(235, HEIGHT - 20, "Lava Danger:", 1.0f, 1.0f, 1.0f);
    
    // Lava danger indicator (2+ primitives: rectangle background, rectangle danger)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(325, HEIGHT - 30);
    glVertex2f(425, HEIGHT - 30);
    glVertex2f(425, HEIGHT - 15);
    glVertex2f(325, HEIGHT - 15);
    glEnd();
    
    float dangerLevel = std::min(1.0f, lavaHeight / (HEIGHT * 0.7f));
    glColor3f(1.0f, 1.0f - dangerLevel, 0.0f);
    float dangerWidth = 100.0f * dangerLevel;
    glBegin(GL_QUADS);
    glVertex2f(325, HEIGHT - 30);
    glVertex2f(325 + dangerWidth, HEIGHT - 30);
    glVertex2f(325 + dangerWidth, HEIGHT - 15);
    glVertex2f(325, HEIGHT - 15);
    glEnd();
    
    // Score and stats panel with brick texture
    drawBrickPanelWithShadow(WIDTH - 180, HEIGHT - 35, 170, 20, 0.6f, 0.5f, 0.3f);
    
    // Score display with coin icon
    std::stringstream ss;
    ss << "Score: " << score;
    drawShadowedText(WIDTH - 175, HEIGHT - 20, ss.str().c_str(), 1.0f, 1.0f, 1.0f);
    drawCoinIcon(WIDTH - 40, HEIGHT - 23, 1.0f);
    
    // Collectables counter
    int collected = 0;
    for (const auto& c : collectables) {
        if (c.collected) collected++;
    }
    std::stringstream collectText;
    collectText << "Coins: " << collected << "/" << collectables.size();
    drawShadowedText(WIDTH - 100, HEIGHT - 20, collectText.str().c_str(), 1.0f, 1.0f, 1.0f);
    
    // Key status with brick panel
    if (keySpawned || keyCollected || collected > 0) {
        drawBrickPanelWithShadow(WIDTH / 2 - 100, HEIGHT - 80, 200, 25, 0.5f, 0.5f, 0.2f);
        
        if (keySpawned && !keyCollected) {
            drawShadowedText(WIDTH / 2 - 60, HEIGHT - 65, "KEY AVAILABLE!", 1.0f, 1.0f, 0.0f);
            drawKeyIcon(WIDTH / 2 + 60, HEIGHT - 65, 0.8f);
        } else if (keyCollected) {
            drawShadowedText(WIDTH / 2 - 50, HEIGHT - 65, "KEY FOUND!", 0.0f, 1.0f, 0.0f);
            drawKeyIcon(WIDTH / 2 + 60, HEIGHT - 65, 0.8f);
        } else {
            std::stringstream keyText;
            keyText << "Collect " << (5 - collected) << " more coins for key";
            if (collected < 5) drawShadowedText(WIDTH / 2 - 80, HEIGHT - 65, keyText.str().c_str(), 0.8f, 0.8f, 0.8f);
        }
    }
    
    // Power-up indicator with brick panel
    if (player.powerUpType > 0) {
        drawBrickPanelWithShadow(WIDTH / 2 - 80, HEIGHT - 110, 160, 35, 0.2f, 0.4f, 0.6f);
        
        std::string powerUpText = (player.powerUpType == 1) ? "SHIELD ACTIVE" : "DOUBLE JUMP ACTIVE";
        drawShadowedText(WIDTH / 2 - 50, HEIGHT - 90, powerUpText.c_str(), 0.0f, 1.0f, 0.0f);
        
        // Timer bar for power-up
        float timerRatio = player.powerUpTimer / 12.0f;
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2f(WIDTH / 2 - 70, HEIGHT - 85);
        glVertex2f(WIDTH / 2 + 70, HEIGHT - 85);
        glVertex2f(WIDTH / 2 + 70, HEIGHT - 80);
        glVertex2f(WIDTH / 2 - 70, HEIGHT - 80);
        glEnd();
        
        glColor3f(0.0f, 0.8f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(WIDTH / 2 - 70, HEIGHT - 85);
        glVertex2f(WIDTH / 2 - 70 + 140 * timerRatio, HEIGHT - 85);
        glVertex2f(WIDTH / 2 - 70 + 140 * timerRatio, HEIGHT - 80);
        glVertex2f(WIDTH / 2 - 70, HEIGHT - 80);
        glEnd();
    }
    
    // Controls reminder with brick panel
    drawBrickPanelWithShadow(5, 5, WIDTH - 10, 25, 0.35f, 0.35f, 0.45f);
    drawShadowedText(15, 20, "Controls: WASD/Arrows to move, Space/W/Up to jump", 0.9f, 0.9f, 0.9f);
}

// Draw game over screen (stylized)
void drawGameOver() {
    float t = menuAnimTime;
    
    // Use the Vice City background but with darker overlay
    drawLayeredBackground();
    
    // Dark red overlay for game over effect
    glColor4f(0.3f, 0.0f, 0.0f, 0.4f + 0.2f * sin(t * 2.0f));
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();

    // Big GAME OVER logo with dramatic effect
    float logoScale = 0.8f + 0.1f * sin(t * 2.0f);
    float jitterX = sin(t * 8.0f) * 3.0f;
    float jitterY = cos(t * 6.0f) * 2.0f;
    
    glPushMatrix();
    glTranslatef(WIDTH / 2.0f + jitterX, HEIGHT / 2 + 150 + jitterY, 0);
    glScalef(logoScale, logoScale, 1.0f);
    
    // GAME text
    glColor3f(1.0f, 0.2f + 0.3f * sin(t * 3.0f), 0.0f);
    glBegin(GL_QUADS);
    // G
    glVertex2f(-120, 40); glVertex2f(-80, 40); glVertex2f(-80, 30); glVertex2f(-120, 30);
    glVertex2f(-120, 30); glVertex2f(-110, 30); glVertex2f(-110, -20); glVertex2f(-120, -20);
    glVertex2f(-120, -20); glVertex2f(-80, -20); glVertex2f(-80, -30); glVertex2f(-120, -30);
    glVertex2f(-90, 0); glVertex2f(-80, 0); glVertex2f(-80, -20); glVertex2f(-90, -20);
    glVertex2f(-100, -10); glVertex2f(-80, -10); glVertex2f(-80, -20); glVertex2f(-100, -20);
    
    // A
    glVertex2f(-70, -30); glVertex2f(-60, -30); glVertex2f(-45, 40); glVertex2f(-55, 40);
    glVertex2f(-45, 40); glVertex2f(-35, 40); glVertex2f(-20, -30); glVertex2f(-30, -30);
    glVertex2f(-55, 10); glVertex2f(-35, 10); glVertex2f(-35, 0); glVertex2f(-55, 0);
    
    // M
    glVertex2f(-10, 40); glVertex2f(0, 40); glVertex2f(0, -30); glVertex2f(-10, -30);
    glVertex2f(20, 40); glVertex2f(30, 40); glVertex2f(30, -30); glVertex2f(20, -30);
    glVertex2f(0, 30); glVertex2f(10, 40); glVertex2f(20, 30); glVertex2f(10, 20);
    
    // E
    glVertex2f(40, 40); glVertex2f(80, 40); glVertex2f(80, 30); glVertex2f(40, 30);
    glVertex2f(40, 30); glVertex2f(50, 30); glVertex2f(50, 10); glVertex2f(40, 10);
    glVertex2f(40, 10); glVertex2f(70, 10); glVertex2f(70, 0); glVertex2f(40, 0);
    glVertex2f(40, 0); glVertex2f(50, 0); glVertex2f(50, -20); glVertex2f(40, -20);
    glVertex2f(40, -20); glVertex2f(80, -20); glVertex2f(80, -30); glVertex2f(40, -30);
    glEnd();
    
    glPopMatrix();
    
    // OVER text
    glPushMatrix();
    glTranslatef(WIDTH / 2.0f + jitterX, HEIGHT / 2 + 80 + jitterY, 0);
    glScalef(logoScale * 0.8f, logoScale * 0.8f, 1.0f);
    
    glColor3f(0.8f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    // O
    glVertex2f(-80, 30); glVertex2f(-40, 30); glVertex2f(-40, 20); glVertex2f(-80, 20);
    glVertex2f(-80, 20); glVertex2f(-70, 20); glVertex2f(-70, -20); glVertex2f(-80, -20);
    glVertex2f(-50, 20); glVertex2f(-40, 20); glVertex2f(-40, -20); glVertex2f(-50, -20);
    glVertex2f(-80, -20); glVertex2f(-40, -20); glVertex2f(-40, -30); glVertex2f(-80, -30);
    
    // V
    glVertex2f(-30, 30); glVertex2f(-20, 30); glVertex2f(-5, -30); glVertex2f(-15, -30);
    glVertex2f(5, 30); glVertex2f(15, 30); glVertex2f(0, -30); glVertex2f(-10, -30);
    
    // E
    glVertex2f(25, 30); glVertex2f(65, 30); glVertex2f(65, 20); glVertex2f(25, 20);
    glVertex2f(25, 20); glVertex2f(35, 20); glVertex2f(35, 5); glVertex2f(25, 5);
    glVertex2f(25, 5); glVertex2f(55, 5); glVertex2f(55, -5); glVertex2f(25, -5);
    glVertex2f(25, -5); glVertex2f(35, -5); glVertex2f(35, -20); glVertex2f(25, -20);
    glVertex2f(25, -20); glVertex2f(65, -20); glVertex2f(65, -30); glVertex2f(25, -30);
    
    // R
    glVertex2f(75, 30); glVertex2f(115, 30); glVertex2f(115, 20); glVertex2f(75, 20);
    glVertex2f(75, 20); glVertex2f(85, 20); glVertex2f(85, 5); glVertex2f(75, 5);
    glVertex2f(75, 5); glVertex2f(105, 5); glVertex2f(105, -5); glVertex2f(75, -5);
    glVertex2f(95, 5); glVertex2f(105, 5); glVertex2f(115, -30); glVertex2f(105, -30);
    glVertex2f(75, -5); glVertex2f(85, -5); glVertex2f(85, -30); glVertex2f(75, -30);
    glVertex2f(105, 20); glVertex2f(115, 20); glVertex2f(115, 5); glVertex2f(105, 5);
    glEnd();
    
    glPopMatrix();

    // Interactive buttons (same as win screen)
    float buttonY = HEIGHT / 2 - 50;
    float buttonSpacing = 80;
    
    auto drawGameOverButton = [&](const char* label, bool selected, float y) {
        float buttonWidth = 200;
        float buttonHeight = 50;
        float buttonX = WIDTH / 2 - buttonWidth / 2;
        
        if (selected) {
            // Selected button - red glowing effect for game over
            drawBrickPanelWithShadow(buttonX, y, buttonWidth, buttonHeight, 0.8f, 0.2f, 0.2f, 0.5f);
            glColor4f(1.0f, 0.0f, 0.0f, 0.3f + 0.2f * sin(t * 5.0f));
            glBegin(GL_QUADS);
            glVertex2f(buttonX - 5, y - 5);
            glVertex2f(buttonX + buttonWidth + 5, y - 5);
            glVertex2f(buttonX + buttonWidth + 5, y + buttonHeight + 5);
            glVertex2f(buttonX - 5, y + buttonHeight + 5);
            glEnd();
            drawShadowedTextCentered(WIDTH / 2.0f, y + 30, label, 1.0f, 1.0f, 0.0f);
        } else {
            drawBrickPanelWithShadow(buttonX, y, buttonWidth, buttonHeight, 0.3f, 0.3f, 0.3f);
            drawShadowedTextCentered(WIDTH / 2.0f, y + 30, label, 0.7f, 0.7f, 0.7f);
        }
    };
    
    drawGameOverButton("TRY AGAIN", currentWinLoseButton == BUTTON_RESTART, buttonY);
    drawGameOverButton("EXIT GAME", currentWinLoseButton == BUTTON_EXIT, buttonY - buttonSpacing);
    
    // Stats in smaller panel at the bottom
    std::stringstream ss;
    ss << "Score: " << score << " | ";
    
    int collected = 0;
    for (const auto& c : collectables) if (c.collected) collected++;
    ss << "Coins: " << collected << "/" << collectables.size() << " | ";
    ss << "Time: " << (int)gameTime << "s";
    
    float statsWidth = measureTextWidth(ss.str().c_str()) + 40;
    drawBrickPanelWithShadow(WIDTH / 2 - statsWidth/2, 50, statsWidth, 32, 0.3f, 0.2f, 0.2f, 0.4f);
    drawShadowedTextCentered(WIDTH / 2.0f, 70, ss.str().c_str(), 0.9f, 0.7f, 0.7f);
}

// Draw game win screen (stylized)
void drawGameWin() {
    float t = menuAnimTime;
    
    // Use the same Vice City background as other screens
    drawLayeredBackground();
    
    // Update and draw falling characters
    characterSpawnTimer += 0.016f;
    if (characterSpawnTimer > 0.3f && fallingCharacters.size() < 15) {
        FallingCharacter newChar;
        newChar.x = rand() % WIDTH;
        newChar.y = HEIGHT + 50;
        newChar.type = (CharacterType)(rand() % 3); // Random character type
        newChar.rotationSpeed = (rand() % 60) + 30; // 30-90 degrees per second
        newChar.rotation = 0;
        newChar.fallSpeed = (rand() % 100) + 150; // 150-250 pixels per second
        newChar.scale = 0.5f + (rand() % 50) / 100.0f; // 0.5 to 1.0 scale
        newChar.active = true;
        fallingCharacters.push_back(newChar);
        characterSpawnTimer = 0.0f;
    }
    
    // Update falling characters
    for (auto& character : fallingCharacters) {
        if (!character.active) continue;
        
        character.y -= character.fallSpeed * 0.016f;
        character.rotation += character.rotationSpeed * 0.016f;
        
        if (character.y < -100) {
            character.active = false;
        }
        
        // Draw the falling character
        glPushMatrix();
        glTranslatef(character.x, character.y, 0);
        glRotatef(character.rotation, 0, 0, 1);
        glScalef(character.scale, character.scale, 1.0f);
        
        // Add transparency
        glEnable(GL_BLEND);
        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
        
        switch (character.type) {
            case WITCH:
                drawWitch(0, 0, true);
                break;
            case FOOTBALLER:
                drawFootballer(0, 0, true);
                break;
            case BUSINESSMAN:
                drawBusinessman(0, 0, true);
                break;
        }
        
        glPopMatrix();
    }
    
    // Remove inactive characters
    fallingCharacters.erase(
        std::remove_if(fallingCharacters.begin(), fallingCharacters.end(),
                      [](const FallingCharacter& c) { return !c.active; }),
        fallingCharacters.end());
    
    // Victory celebration background effect
    glColor4f(1.0f, 1.0f, 0.0f, 0.1f + 0.1f * sin(t * 3.0f));
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();

    // Celebration sparkles around the screen edges
    for (int i = 0; i < 20; i++) {
        float angle = i * (2.0f * M_PI / 20.0f) + t * 2.0f;
        float radius = 100 + 50 * sin(t * 3.0f + i);
        float sparkleX = WIDTH / 2 + radius * cos(angle);
        float sparkleY = HEIGHT / 2 + radius * sin(angle);
        
        float sparkleSize = 3 + 2 * sin(t * 5.0f + i);
        glColor4f(1.0f, 1.0f, 0.0f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(sparkleX - sparkleSize, sparkleY - sparkleSize);
        glVertex2f(sparkleX + sparkleSize, sparkleY - sparkleSize);
        glVertex2f(sparkleX + sparkleSize, sparkleY + sparkleSize);
        glVertex2f(sparkleX - sparkleSize, sparkleY + sparkleSize);
        glEnd();
    }

    // Big YOU WIN logo - simpler and cleaner
    float logoScale = 1.0f + 0.1f * sin(t * 2.0f);
    
    // YOU text
    glPushMatrix();
    glTranslatef(WIDTH / 2.0f, HEIGHT / 2 + 150, 0);
    glScalef(logoScale, logoScale, 1.0f);
    
    glColor3f(1.0f, 0.8f + 0.2f * sin(t * 3.0f), 0.0f);
    
    // Draw "YOU" using text-like rectangles
    // Y
    glBegin(GL_QUADS);
    glVertex2f(-80, 40); glVertex2f(-70, 40); glVertex2f(-55, 10); glVertex2f(-65, 10);
    glVertex2f(-45, 40); glVertex2f(-35, 40); glVertex2f(-50, 10); glVertex2f(-60, 10);
    glVertex2f(-62, 10); glVertex2f(-53, 10); glVertex2f(-53, -40); glVertex2f(-62, -40);
    
    // O
    glVertex2f(-25, 40); glVertex2f(5, 40); glVertex2f(5, 30); glVertex2f(-25, 30);
    glVertex2f(-25, 30); glVertex2f(-15, 30); glVertex2f(-15, -30); glVertex2f(-25, -30);
    glVertex2f(-5, 30); glVertex2f(5, 30); glVertex2f(5, -30); glVertex2f(-5, -30);
    glVertex2f(-25, -30); glVertex2f(5, -30); glVertex2f(5, -40); glVertex2f(-25, -40);
    
    // U
    glVertex2f(15, 40); glVertex2f(25, 40); glVertex2f(25, -30); glVertex2f(15, -30);
    glVertex2f(45, 40); glVertex2f(55, 40); glVertex2f(55, -30); glVertex2f(45, -30);
    glVertex2f(15, -30); glVertex2f(55, -30); glVertex2f(55, -40); glVertex2f(15, -40);
    glEnd();
    
    glPopMatrix();
    
    // WIN text
    glPushMatrix();
    glTranslatef(WIDTH / 2.0f, HEIGHT / 2 + 60, 0);
    glScalef(logoScale, logoScale, 1.0f);
    
    glColor3f(0.0f, 1.0f, 0.5f + 0.5f * sin(t * 4.0f));
    
    // Draw "WIN"
    glBegin(GL_QUADS);
    // W
    glVertex2f(-90, 40); glVertex2f(-80, 40); glVertex2f(-80, -40); glVertex2f(-90, -40);
    glVertex2f(-55, 40); glVertex2f(-45, 40); glVertex2f(-45, -40); glVertex2f(-55, -40);
    glVertex2f(-80, -20); glVertex2f(-72, -20); glVertex2f(-65, -40); glVertex2f(-73, -40);
    glVertex2f(-72, -20); glVertex2f(-62, -20); glVertex2f(-55, -40); glVertex2f(-63, -40);
    
    // I
    glVertex2f(-30, 40); glVertex2f(0, 40); glVertex2f(0, 30); glVertex2f(-30, 30);
    glVertex2f(-20, 30); glVertex2f(-10, 30); glVertex2f(-10, -30); glVertex2f(-20, -30);
    glVertex2f(-30, -30); glVertex2f(0, -30); glVertex2f(0, -40); glVertex2f(-30, -40);
    
    // N
    glVertex2f(15, 40); glVertex2f(25, 40); glVertex2f(25, -40); glVertex2f(15, -40);
    glVertex2f(55, 40); glVertex2f(65, 40); glVertex2f(65, -40); glVertex2f(55, -40);
    glVertex2f(25, 30); glVertex2f(35, 40); glVertex2f(45, 30); glVertex2f(35, 20);
    glVertex2f(25, 10); glVertex2f(35, 20); glVertex2f(45, 10); glVertex2f(35, 0);
    glVertex2f(25, -10); glVertex2f(35, 0); glVertex2f(45, -10); glVertex2f(35, -20);
    glEnd();
    
    glPopMatrix();

    // Interactive buttons
    float buttonY = HEIGHT / 2 - 50;
    float buttonSpacing = 80;
    
    auto drawWinButton = [&](const char* label, bool selected, float y) {
        float buttonWidth = 200;
        float buttonHeight = 50;
        float buttonX = WIDTH / 2 - buttonWidth / 2;
        
        if (selected) {
            // Selected button - glowing effect
            drawBrickPanelWithShadow(buttonX, y, buttonWidth, buttonHeight, 0.2f, 0.8f, 0.2f, 0.5f);
            glColor4f(0.0f, 1.0f, 0.0f, 0.3f + 0.2f * sin(t * 5.0f));
            glBegin(GL_QUADS);
            glVertex2f(buttonX - 5, y - 5);
            glVertex2f(buttonX + buttonWidth + 5, y - 5);
            glVertex2f(buttonX + buttonWidth + 5, y + buttonHeight + 5);
            glVertex2f(buttonX - 5, y + buttonHeight + 5);
            glEnd();
            drawShadowedTextCentered(WIDTH / 2.0f, y + 30, label, 1.0f, 1.0f, 0.0f);
        } else {
            drawBrickPanelWithShadow(buttonX, y, buttonWidth, buttonHeight, 0.5f, 0.5f, 0.5f);
            drawShadowedTextCentered(WIDTH / 2.0f, y + 30, label, 0.85f, 0.85f, 0.85f);
        }
    };
    
    drawWinButton("PLAY AGAIN", currentWinLoseButton == BUTTON_RESTART, buttonY);
    drawWinButton("EXIT GAME", currentWinLoseButton == BUTTON_EXIT, buttonY - buttonSpacing);
    
    // Stats in smaller panels at the bottom
    std::stringstream ss;
    ss << "Score: " << score << " | ";
    
    int collected = 0;
    for (const auto& c : collectables) if (c.collected) collected++;
    ss << "Coins: " << collected << "/" << collectables.size() << " | ";
    ss << "Time: " << (int)gameTime << "s";
    
    float statsWidth = measureTextWidth(ss.str().c_str()) + 40;
    drawBrickPanelWithShadow(WIDTH / 2 - statsWidth/2, 50, statsWidth, 32, 0.3f, 0.3f, 0.4f, 0.3f);
    drawShadowedTextCentered(WIDTH / 2.0f, 70, ss.str().c_str(), 0.9f, 0.9f, 0.9f);
}

// Update game logic
void update(float deltaTime) {
    if (gameState != PLAYING) return;
    
    gameTime += deltaTime;
    
    // Update lava - slightly faster for more challenge
    lavaSpeed = 0.35f + gameTime * 0.008f; // Slightly faster increase
    lavaHeight += lavaSpeed * deltaTime * 9; // Slightly faster rising
    
    // Remove platforms touched by lava
    for (auto& platform : platforms) {
        if (platform.y <= lavaHeight) {
            platform.active = false;
        }
    }
    
    // Skip normal physics if player is being sucked into door
    if (!playerBeingSucked) {
        // Update player physics - balanced for challenge
        player.velocityY -= 750.0f * deltaTime; // Slightly more gravity

        // Smooth continuous movement with acceleration
        float acceleration = 1200.0f; // Acceleration rate
        float maxSpeed = 280.0f; // Maximum horizontal speed
        float deceleration = player.onGround ? 0.80f : 0.92f; // Different deceleration on ground vs air
        
        // Apply movement based on key states
        if (leftPressed) {
            player.velocityX -= acceleration * deltaTime;
            if (player.velocityX < -maxSpeed) player.velocityX = -maxSpeed;
        } else if (rightPressed) {
            player.velocityX += acceleration * deltaTime;
            if (player.velocityX > maxSpeed) player.velocityX = maxSpeed;
        } else {
            // Decelerate when no keys pressed
            player.velocityX *= deceleration;
            // Stop tiny velocities to avoid jitter
            if (fabs(player.velocityX) < 5.0f) player.velocityX = 0.0f;
        }

        player.x += player.velocityX * deltaTime;
        player.y += player.velocityY * deltaTime;
    }
    
    // Skip boundary checking and collisions during suction
    if (!playerBeingSucked) {
        // Boundary checking
        if (player.x < 0) player.x = 0;
        if (player.x + player.width > WIDTH) player.x = WIDTH - player.width;
        
        // Platform collision
        player.onGround = false;
        for (const auto& platform : platforms) {
            if (!platform.active) continue;
            
            if (checkCollision(player.x, player.y, player.width, player.height,
                             platform.x, platform.y, platform.width, platform.height)) {
                if (player.velocityY <= 0 && player.y > platform.y) {
                    player.y = platform.y + platform.height;
                    player.velocityY = 0;
                    player.onGround = true;
                    player.hasDoubleJumped = false;
                }
            }
        }
    }

    // Update jump flip timing
    if (player.onGround) {
        playerAirTime = 0.0f;
        playerFlipAngle = 0.0f;
    } else {
        playerAirTime += deltaTime;
        float flipDuration = 0.7f; // seconds per full flip
        float progress = std::min(1.0f, playerAirTime / flipDuration);
        playerFlipAngle = -360.0f * progress; // clockwise
    }
    
    // Lava collision
    if (player.y <= lavaHeight) {
        gameState = GAME_OVER;
        return;
    }
    
    // Update power-up timer
    if (player.powerUpType > 0) {
        player.powerUpTimer -= deltaTime;
        if (player.powerUpTimer <= 0) {
            player.powerUpType = 0;
            player.canDoubleJump = false;
        }
    }
    
    // Spawn rocks - more frequently at random intervals
    rockSpawnTimer -= deltaTime;
    if (rockSpawnTimer <= 0) {
        rocks.push_back({(float)(rand() % (WIDTH - 20)), (float)HEIGHT, true});
        rockSpawnTimer = 0.8f + (rand() % 80) / 100.0f; // 0.8s - 1.6s (more frequent)
    }
    
    // Update rocks
    for (auto& rock : rocks) {
        if (!rock.active) continue;
        
        rock.y -= 200.0f * deltaTime;
        
        // Rock collision with player
        if (checkCollision(player.x, player.y, player.width, player.height,
                         rock.x - 10, rock.y - 10, 20, 20)) {
            if (player.powerUpType != 1) { // No shield
                playerLives--;
                if (playerLives <= 0) {
                    gameState = GAME_OVER;
                    return;
                }
            }
            rock.active = false;
        }
        
        if (rock.y < -20) {
            rock.active = false;
        }
    }
    
    // Remove inactive rocks
    rocks.erase(std::remove_if(rocks.begin(), rocks.end(),
                              [](const Rock& r) { return !r.active; }), rocks.end());
    
    // Update collectables animations
    for (auto& collectable : collectables) {
        collectable.animTime += deltaTime;
    }
    
    // Collectable collision
    for (auto& collectable : collectables) {
        if (collectable.collected) continue;
        
        if (checkCollision(player.x, player.y, player.width, player.height,
                         collectable.x - 10, collectable.y - 10, 20, 20)) {
            collectable.collected = true;
            score += 100;
        }
    }
    
    // Check if key should spawn
    if (!keySpawned) {
        int collectedCount = 0;
        for (const auto& collectable : collectables) {
            if (collectable.collected) collectedCount++;
        }
        
        if (collectedCount >= 5) {
            keySpawned = true;
            keyX = 100 + rand() % (WIDTH - 200);
            keyY = 300 + rand() % 200;
        }
    }
    
    // Update key animation
    if (keySpawned) {
        keyAnimTime += deltaTime;
    }
    
    // Key collision
    if (keySpawned && !keyCollected) {
        if (checkCollision(player.x, player.y, player.width, player.height,
                         keyX - 15, keyY - 10, 30, 20)) {
            keyCollected = true;
            player.hasKey = true;
            score += 500;
            // Start door unlock animation
            doorIsUnlocking = true;
            doorUnlockAnimTime = 0.0f;
        }
    }
    
    // Update door animations
    doorAnimTime += deltaTime; // Always update for constant effects
    if (doorIsUnlocking) {
        doorUnlockAnimTime += deltaTime;
        if (doorUnlockAnimTime >= 2.0f) {
            doorIsUnlocking = false; // Unlock animation complete
        }
    }
    if (doorIsEntering) {
        doorEnterAnimTime += deltaTime;
    }
    
    // Spawn power-ups - more frequently
    powerUpSpawnTimer -= deltaTime;
    if (powerUpSpawnTimer <= 0 && powerUps.size() < 2) {
        int type = 1 + rand() % 2;
        float x = 50 + rand() % (WIDTH - 100);
        float y = lavaHeight + 100 + rand() % 200;
        powerUps.push_back({x, y, type, true, 15.0f, 0.0f}); // Last longer
        powerUpSpawnTimer = 10.0f + rand() % 8; // Spawn more often
    }
    
    // Update power-ups
    for (auto& powerUp : powerUps) {
        if (!powerUp.active) continue;
        
        powerUp.animTime += deltaTime;
        powerUp.lifeTime -= deltaTime;
        
        if (powerUp.lifeTime <= 0) {
            powerUp.active = false;
            continue;
        }
        
        // Power-up collision
        if (checkCollision(player.x, player.y, player.width, player.height,
                         powerUp.x - 15, powerUp.y - 15, 30, 30)) {
            player.powerUpType = powerUp.type;
            player.powerUpTimer = 12.0f; // Last longer when activated
            if (powerUp.type == 2) {
                player.canDoubleJump = true;
            }
            powerUp.active = false;
            score += 200;
        }
    }
    
    // Remove inactive power-ups
    powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
                                 [](const PowerUp& p) { return !p.active; }), powerUps.end());
    
    // Win condition - player entering the door
    if (keyCollected && !doorIsEntering && !playerBeingSucked) {
        float doorX = WIDTH / 2 - 40;
        float doorY = HEIGHT - 120;
        if (checkCollision(player.x, player.y, player.width, player.height,
                         doorX, doorY, 80, 120)) {
            // Start suction animation
            playerBeingSucked = true;
            suctionAnimTime = 0.0f;
            suctionStartX = player.x;
            suctionStartY = player.y;
            doorCenterX = doorX + 40; // Center of door
            doorCenterY = doorY + 60;
        }
    }
    
    // Update suction animation
    if (playerBeingSucked) {
        suctionAnimTime += deltaTime;
        
        // Animation duration: 2 seconds
        float suctionDuration = 2.0f;
        float progress = std::min(1.0f, suctionAnimTime / suctionDuration);
        
        if (progress < 1.0f) {
            // Freeze everything - don't update normal physics
            // Move player towards door center with easing
            float easeProgress = 1.0f - (1.0f - progress) * (1.0f - progress); // Ease in
            
            player.x = suctionStartX + (doorCenterX - suctionStartX) * easeProgress;
            player.y = suctionStartY + (doorCenterY - suctionStartY) * easeProgress;
            
            // Fast clockwise rotation
            playerFlipAngle = -progress * 720.0f * 3.0f; // Multiple fast spins
        } else {
            // Animation complete - trigger win
            gameState = GAME_WIN;
            initFallingCharacters(); // Initialize falling characters for win screen
        }
        
        // Don't update other game elements during suction
        return;
    }
    
    // Complete win after entrance animation finishes
    if (doorIsEntering && doorEnterAnimTime >= 1.5f) {
        gameState = GAME_WIN;
        initFallingCharacters(); // Initialize falling characters for win screen
    }
}

// Keyboard input
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            if (gameState == START_MENU) {
                exit(0);
            } else {
                gameState = START_MENU;
            }
            break;
        case 13: // ENTER
            if (gameState == START_MENU) {
                switch (currentMenuSelection) {
                    case MENU_START:
                        gameState = PLAYING;
                        score = 0;
                        playerLives = 3;
                        gameTime = 0;
                        lavaHeight = 50;
                        keySpawned = false;
                        keyCollected = false;
                        initGame();
                        break;
                    case MENU_CHARACTER:
                        gameState = CHARACTER_SELECT;
                        break;
                    case MENU_EXIT:
                        exit(0);
                        break;
                }
            } else if (gameState == CHARACTER_SELECT) {
                switch (currentCharacterSelection) {
                    case CHAR_WITCH:
                        selectedCharacter = WITCH;
                        gameState = START_MENU;
                        break;
                    case CHAR_FOOTBALLER:
                        selectedCharacter = FOOTBALLER;
                        gameState = START_MENU;
                        break;
                    case CHAR_BUSINESSMAN:
                        selectedCharacter = BUSINESSMAN;
                        gameState = START_MENU;
                        break;
                    case CHAR_BACK:
                        gameState = START_MENU;
                        break;
                }
            } else if (gameState == GAME_OVER || gameState == GAME_WIN) {
                switch (currentWinLoseButton) {
                    case BUTTON_RESTART:
                        gameState = PLAYING;
                        score = 0;
                        playerLives = 3;
                        gameTime = 0;
                        lavaHeight = 50;
                        keySpawned = false;
                        keyCollected = false;
                        currentWinLoseButton = BUTTON_RESTART; // Reset button selection
                        if (gameState == GAME_WIN) {
                            initFallingCharacters(); // Clear falling characters
                        }
                        initGame();
                        break;
                    case BUTTON_EXIT:
                        exit(0);
                        break;
                }
            }
            break;
        case 'r':
        case 'R':
            if (gameState == GAME_OVER || gameState == GAME_WIN) {
                gameState = START_MENU;
            }
            break;
        case 'a':
        case 'A':
            if (gameState == PLAYING && !playerBeingSucked) {
                leftPressed = true;
            }
            break;
        case 'd':
        case 'D':
            if (gameState == PLAYING && !playerBeingSucked) {
                rightPressed = true;
            }
            break;
        case 'w':
        case 'W':
        case ' ':
            if (gameState == PLAYING && !playerBeingSucked) {
                if (player.onGround) {
                    player.velocityY = 400; // Slightly lower jump
                    player.onGround = false;
                    // Start flip
                    playerAirTime = 0.0f;
                    playerFlipAngle = 0.0f;
                } else if (player.canDoubleJump && !player.hasDoubleJumped) {
                    player.velocityY = 310; // Slightly lower double jump
                    player.hasDoubleJumped = true;
                    // Restart flip on double jump
                    playerAirTime = 0.0f;
                    playerFlipAngle = 0.0f;
                }
            }
            break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    if (gameState != PLAYING) return;

    switch (key) {
        case 'a':
        case 'A':
            leftPressed = false;
            break;
        case 'd':
        case 'D':
            rightPressed = false;
            break;
    }
}

void specialKey(int key, int x, int y) {
    if (gameState == START_MENU) {
        switch (key) {
            case GLUT_KEY_UP:
                currentMenuSelection = (MenuSelection)((currentMenuSelection - 1 + 3) % 3);
                break;
            case GLUT_KEY_DOWN:
                currentMenuSelection = (MenuSelection)((currentMenuSelection + 1) % 3);
                break;
        }
    } else if (gameState == CHARACTER_SELECT) {
        switch (key) {
            case GLUT_KEY_LEFT:
                currentCharacterSelection = (CharacterSelection)((currentCharacterSelection - 1 + 4) % 4);
                break;
            case GLUT_KEY_RIGHT:
                currentCharacterSelection = (CharacterSelection)((currentCharacterSelection + 1) % 4);
                break;
        }
    } else if (gameState == GAME_OVER || gameState == GAME_WIN) {
        switch (key) {
            case GLUT_KEY_UP:
                currentWinLoseButton = (WinLoseButton)((currentWinLoseButton - 1 + 2) % 2);
                break;
            case GLUT_KEY_DOWN:
                currentWinLoseButton = (WinLoseButton)((currentWinLoseButton + 1) % 2);
                break;
        }
    } else if (gameState == PLAYING && !playerBeingSucked) {
        switch (key) {
            case GLUT_KEY_LEFT:
                leftPressed = true;
                break;
            case GLUT_KEY_RIGHT:
                rightPressed = true;
                break;
            case GLUT_KEY_UP:
                if (player.onGround) {
                    player.velocityY = 400; // Slightly lower jump
                    player.onGround = false;
                    // Start flip
                    playerAirTime = 0.0f;
                    playerFlipAngle = 0.0f;
                } else if (player.canDoubleJump && !player.hasDoubleJumped) {
                    player.velocityY = 310; // Slightly lower double jump
                    player.hasDoubleJumped = true;
                    // Restart flip on double jump
                    playerAirTime = 0.0f;
                    playerFlipAngle = 0.0f;
                }
                break;
        }
    }
}

void specialKeyUp(int key, int x, int y) {
    if (gameState != PLAYING) return;

    switch (key) {
        case GLUT_KEY_LEFT:
            leftPressed = false;
            break;
        case GLUT_KEY_RIGHT:
            rightPressed = false;
            break;
    }
}

// Particle structure for floating elements
struct BackgroundParticle {
    float x, y;
    float size;
    float speed;
    float alpha;
};

std::vector<BackgroundParticle> bgParticles;
float bgAnimTime = 0.0f;

// Initialize background particles
void initBackgroundParticles() {
    bgParticles.clear();
    for (int i = 0; i < 50; i++) {
        BackgroundParticle p;
        p.x = rand() % WIDTH;
        p.y = rand() % HEIGHT;
        p.size = 2 + rand() % 4;
        p.speed = 5 + rand() % 15;
        p.alpha = 0.3f + (rand() % 50) / 100.0f;
        bgParticles.push_back(p);
    }
}

// Initialize falling characters for win screen
void initFallingCharacters() {
    fallingCharacters.clear();
    characterSpawnTimer = 0.0f;
}

// Draw epic animated parallax background
void drawLayeredBackground() {
    // Initialize particles if empty
    if (bgParticles.empty()) {
        initBackgroundParticles();
    }
    
    // Vice City style sunset gradient with color cycling
    float sunsetCycle = sin(bgAnimTime * 0.2f) * 0.3f + 0.7f; // Slower, more subtle cycling
    
    // Vintage sunset gradient - top to bottom
    glBegin(GL_QUADS);
    // Sky top - deep purple/magenta
    glColor3f(0.3f * sunsetCycle, 0.1f * sunsetCycle, 0.5f * sunsetCycle);
    glVertex2f(0, HEIGHT);
    glVertex2f(WIDTH, HEIGHT);
    
    // Upper middle - pink/orange blend
    glColor3f(0.8f * sunsetCycle, 0.3f * sunsetCycle, 0.6f * sunsetCycle);
    glVertex2f(WIDTH, HEIGHT * 0.75f);
    glVertex2f(0, HEIGHT * 0.75f);
    glEnd();
    
    glBegin(GL_QUADS);
    // Mid horizon - bright orange/yellow
    glColor3f(0.8f * sunsetCycle, 0.3f * sunsetCycle, 0.6f * sunsetCycle);
    glVertex2f(0, HEIGHT * 0.75f);
    glVertex2f(WIDTH, HEIGHT * 0.75f);
    
    glColor3f(1.0f * sunsetCycle, 0.5f * sunsetCycle, 0.2f * sunsetCycle);
    glVertex2f(WIDTH, HEIGHT * 0.5f);
    glVertex2f(0, HEIGHT * 0.5f);
    glEnd();
    
    glBegin(GL_QUADS);
    // Lower horizon - deep orange to dark
    glColor3f(1.0f * sunsetCycle, 0.5f * sunsetCycle, 0.2f * sunsetCycle);
    glVertex2f(0, HEIGHT * 0.5f);
    glVertex2f(WIDTH, HEIGHT * 0.5f);
    
    glColor3f(0.4f * sunsetCycle, 0.2f * sunsetCycle, 0.4f * sunsetCycle);
    glVertex2f(WIDTH, HEIGHT * 0.25f);
    glVertex2f(0, HEIGHT * 0.25f);
    glEnd();
    
    glBegin(GL_QUADS);
    // Bottom - dark purple/black
    glColor3f(0.4f * sunsetCycle, 0.2f * sunsetCycle, 0.4f * sunsetCycle);
    glVertex2f(0, HEIGHT * 0.25f);
    glVertex2f(WIDTH, HEIGHT * 0.25f);
    
    glColor3f(0.1f, 0.05f, 0.15f);
    glVertex2f(WIDTH, 0);
    glVertex2f(0, 0);
    glEnd();
    
    // Distant skyscrapers layer (slowest parallax)
    float backBuildingOffset = fmod(bgAnimTime * 5.0f, WIDTH + 400);
    glColor4f(0.1f, 0.05f, 0.2f, 0.6f); // Dark silhouette
    
    // Draw distant skyscraper silhouettes
    for (int i = 0; i < 8; i++) {
        float buildingX = backBuildingOffset + i * 80 - 200;
        if (buildingX > WIDTH) buildingX -= WIDTH + 640;
        
        float buildingHeight = 100 + (i * 23) % 80; // Varied heights
        float buildingWidth = 25 + (i * 7) % 15;
        
        // Main building rectangle
        glBegin(GL_QUADS);
        glVertex2f(buildingX, HEIGHT * 0.35f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.35f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.35f + buildingHeight);
        glVertex2f(buildingX, HEIGHT * 0.35f + buildingHeight);
        glEnd();
        
        // Window lights (some buildings have lights on)
        if (i % 2 == 0) {
            glColor4f(1.0f, 0.9f, 0.6f, 0.8f);
            for (int w = 0; w < 3; w++) {
                for (int h = 0; h < (int)(buildingHeight / 15); h++) {
                    if ((w + h + i) % 3 == 0) { // Random pattern
                        float winX = buildingX + 3 + w * 7;
                        float winY = HEIGHT * 0.35f + 5 + h * 15;
                        glBegin(GL_QUADS);
                        glVertex2f(winX, winY);
                        glVertex2f(winX + 4, winY);
                        glVertex2f(winX + 4, winY + 8);
                        glVertex2f(winX, winY + 8);
                        glEnd();
                    }
                }
            }
            glColor4f(0.1f, 0.05f, 0.2f, 0.6f); // Reset color
        }
    }
    
    // Middle skyscrapers layer (medium parallax)
    float midBuildingOffset = fmod(bgAnimTime * 10.0f, WIDTH + 300);
    glColor4f(0.15f, 0.08f, 0.25f, 0.7f);
    
    for (int i = 0; i < 6; i++) {
        float buildingX = midBuildingOffset + i * 120 - 200;
        if (buildingX > WIDTH) buildingX -= WIDTH + 720;
        
        float buildingHeight = 120 + (i * 31) % 100;
        float buildingWidth = 35 + (i * 11) % 20;
        
        // Main building
        glBegin(GL_QUADS);
        glVertex2f(buildingX, HEIGHT * 0.3f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.3f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.3f + buildingHeight);
        glVertex2f(buildingX, HEIGHT * 0.3f + buildingHeight);
        glEnd();
        
        // Antenna/spires on some buildings
        if (i % 3 == 1) {
            glBegin(GL_LINES);
            glVertex2f(buildingX + buildingWidth/2, HEIGHT * 0.3f + buildingHeight);
            glVertex2f(buildingX + buildingWidth/2, HEIGHT * 0.3f + buildingHeight + 20);
            glEnd();
        }
        
        // More detailed windows
        glColor4f(1.0f, 0.8f, 0.4f, 0.9f);
        for (int w = 0; w < (int)(buildingWidth / 8); w++) {
            for (int h = 0; h < (int)(buildingHeight / 12); h++) {
                if ((w + h + i * 2) % 4 != 0) {
                    float winX = buildingX + 2 + w * 8;
                    float winY = HEIGHT * 0.3f + 3 + h * 12;
                    glBegin(GL_QUADS);
                    glVertex2f(winX, winY);
                    glVertex2f(winX + 5, winY);
                    glVertex2f(winX + 5, winY + 6);
                    glVertex2f(winX, winY + 6);
                    glEnd();
                }
            }
        }
        glColor4f(0.15f, 0.08f, 0.25f, 0.7f);
    }
    
    // Foreground skyscrapers (fastest parallax)
    float frontBuildingOffset = fmod(bgAnimTime * 20.0f, WIDTH + 250);
    glColor4f(0.08f, 0.04f, 0.15f, 0.8f);
    
    for (int i = 0; i < 4; i++) {
        float buildingX = frontBuildingOffset + i * 200 - 200;
        if (buildingX > WIDTH) buildingX -= WIDTH + 800;
        
        float buildingHeight = 150 + (i * 43) % 120;
        float buildingWidth = 50 + (i * 13) % 30;
        
        // Main building silhouette
        glBegin(GL_QUADS);
        glVertex2f(buildingX, HEIGHT * 0.25f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.25f);
        glVertex2f(buildingX + buildingWidth, HEIGHT * 0.25f + buildingHeight);
        glVertex2f(buildingX, HEIGHT * 0.25f + buildingHeight);
        glEnd();
        
        // Building details - stepped tops
        if (i % 2 == 0) {
            glBegin(GL_QUADS);
            glVertex2f(buildingX + 10, HEIGHT * 0.25f + buildingHeight);
            glVertex2f(buildingX + buildingWidth - 10, HEIGHT * 0.25f + buildingHeight);
            glVertex2f(buildingX + buildingWidth - 10, HEIGHT * 0.25f + buildingHeight + 15);
            glVertex2f(buildingX + 10, HEIGHT * 0.25f + buildingHeight + 15);
            glEnd();
        }
        
        // Bright windows creating city atmosphere
        glColor4f(1.0f, 0.9f, 0.7f, 1.0f);
        for (int w = 0; w < (int)(buildingWidth / 10); w++) {
            for (int h = 0; h < (int)(buildingHeight / 15); h++) {
                if ((w * 3 + h + i) % 5 != 0) {
                    float winX = buildingX + 3 + w * 10;
                    float winY = HEIGHT * 0.25f + 5 + h * 15;
                    glBegin(GL_QUADS);
                    glVertex2f(winX, winY);
                    glVertex2f(winX + 6, winY);
                    glVertex2f(winX + 6, winY + 8);
                    glVertex2f(winX, winY + 8);
                    glEnd();
                }
            }
        }
        glColor4f(0.08f, 0.04f, 0.15f, 0.8f);
    }
    
    // Atmospheric particles (modified for Vice City vibe)
    for (auto& particle : bgParticles) {
        // Update particle position with slower drift
        particle.y += particle.speed * 0.008f; // Slower floating for atmospheric effect
        if (particle.y > HEIGHT + 20) {
            particle.y = -20;
            particle.x = rand() % WIDTH;
        }
        
        // Different particles: some are city lights, others are atmospheric dust
        float pulse = sin(bgAnimTime * 1.5f + particle.x * 0.01f) * 0.4f + 0.6f;
        
        if ((int)particle.x % 3 == 0) {
            // City light reflections - pink/magenta tint
            glColor4f(1.0f * pulse, 0.4f * pulse, 0.8f * pulse, particle.alpha * 0.6f);
        } else {
            // Warm atmospheric particles - orange/yellow
            glColor4f(1.0f * pulse, 0.8f * pulse, 0.3f * pulse, particle.alpha * 0.4f);
        }
        
        // Draw as small glowing points
        glBegin(GL_POLYGON);
        for (int i = 0; i < 6; i++) {
            float angle = 2.0f * M_PI * i / 6;
            glVertex2f(particle.x + particle.size * cos(angle), 
                      particle.y + particle.size * sin(angle));
        }
        glEnd();
        
        // Add subtle glow
        glColor4f(1.0f, 0.6f, 0.4f, particle.alpha * pulse * 0.2f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 6; i++) {
            float angle = 2.0f * M_PI * i / 6;
            glVertex2f(particle.x + (particle.size + 1) * cos(angle), 
                      particle.y + (particle.size + 1) * sin(angle));
        }
        glEnd();
    }
    
    // Add subtle grid lines in the distance for retro-futuristic effect
    glColor4f(0.3f * sunsetCycle, 0.1f * sunsetCycle, 0.4f * sunsetCycle, 0.15f);
    float gridOffset = fmod(bgAnimTime * 30.0f, 50.0f);
    
    // Horizontal grid lines
    for (int i = -2; i < HEIGHT / 25; i++) {
        float lineY = i * 25 + gridOffset;
        if (lineY > HEIGHT * 0.5f) {
            glBegin(GL_LINES);
            glVertex2f(0, lineY);
            glVertex2f(WIDTH, lineY);
            glEnd();
        }
    }
    
    // Semi-transparent overlay for depth blur effect
    glColor4f(0.05f, 0.05f, 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();
}

// Measure text width using GLUT bitmap widths
int measureTextWidth(const char* text) {
    int w = 0;
    while (*text) { w += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *text++); }
    return w;
}

void drawTextCentered(float cx, float y, const char* text) {
    int w = measureTextWidth(text);
    drawText(cx - w / 2.0f, y, text);
}

void drawShadowedTextCentered(float cx, float y, const char* text, float r, float g, float b) {
    int w = measureTextWidth(text);
    drawShadowedText(cx - w / 2.0f, y, text, r, g, b);
}

// Draw start menu
void drawStartMenu() {
    float centerX = WIDTH / 2.0f;
    
    // Draw pixel art logo instead of text title
    drawIcyTowerLogo(centerX, HEIGHT - 100);
    
    // Menu options with selection highlighting (responsive widths)
    float menuY = HEIGHT / 2 + 70;
    float spacing = 70;
    
    auto drawMenuItem = [&](const char* label, bool selected, float y) {
        float panelW = std::max<float>(240.0f, measureTextWidth(label) + 80.0f);
        float px = centerX - panelW / 2.0f;
        if (selected) {
            drawBrickPanelWithShadow(px, y - 15, panelW, 46, 0.2f, 0.8f, 0.2f);
            drawShadowedTextCentered(centerX, y + 6, label, 1.0f, 1.0f, 0.0f);
        } else {
            drawBrickPanelWithShadow(px, y - 15, panelW, 46, 0.5f, 0.5f, 0.5f);
            drawShadowedTextCentered(centerX, y + 6, label, 0.85f, 0.85f, 0.85f);
        }
    };
    
    drawMenuItem("START GAME", currentMenuSelection == MENU_START, menuY);
    drawMenuItem("SELECT CHARACTER", currentMenuSelection == MENU_CHARACTER, menuY - spacing);
    drawMenuItem("EXIT", currentMenuSelection == MENU_EXIT, menuY - 2 * spacing);
    
    // Instructions (responsive width)
    const char* hint = "Use UP/DOWN arrows to navigate, ENTER to select";
    float hintW = std::min<float>(WIDTH - 100, std::max<float>(420.0f, measureTextWidth(hint) + 60.0f));
    drawBrickPanelWithShadow(centerX - hintW / 2, 50, hintW, 40, 0.4f, 0.4f, 0.6f);
    drawShadowedTextCentered(centerX, 75, hint, 0.9f, 0.9f, 0.9f);
}

// Draw character selection menu
void drawCharacterSelect() {
    float centerX = WIDTH / 2.0f;
    
    // Title panel with shadow (responsive)
    const char* title = "CHOOSE CHARACTER";
    float titlePanelW = std::min<float>(WIDTH - 40, std::max<float>(300.0f, measureTextWidth(title) + 120.0f));
    drawBrickPanelWithShadow(centerX - titlePanelW / 2, HEIGHT - 110, titlePanelW, 48, 0.8f, 0.6f, 0.2f);
    drawShadowedTextCentered(centerX, HEIGHT - 85, title, 1.0f, 1.0f, 1.0f);
    
    // Character displays (responsive spacing and larger cards)
    float cardW = 140.0f, cardH = 180.0f;
    float charY = HEIGHT / 2.0f;
    float charSpacing = std::max(220.0f, WIDTH * 0.28f);
    float startX = centerX - charSpacing;
    
    auto drawCharacterCard = [&](float cardCenterX, const char* label, bool selected, void(*drawFn)(float,float,bool)) {
        float px = cardCenterX - cardW / 2.0f;
        if (selected) {
            drawBrickPanelWithShadow(px, charY - cardH / 2.0f, cardW, cardH, 0.4f, 0.6f, 0.9f);
            drawShadowedTextCentered(cardCenterX, charY - cardH / 2.0f - 20, label, 1.0f, 1.0f, 0.0f);
        } else {
            drawBrickPanelWithShadow(px, charY - cardH / 2.0f, cardW, cardH, 0.3f, 0.3f, 0.3f);
            drawShadowedTextCentered(cardCenterX, charY - cardH / 2.0f - 20, label, 0.8f, 0.8f, 0.8f);
        }
        drawFn(cardCenterX - 30, charY - 60, true);
    };
    
    drawCharacterCard(startX,       "WITCH",       currentCharacterSelection == CHAR_WITCH,       drawWitch);
    drawCharacterCard(startX + charSpacing, "FOOTBALLER", currentCharacterSelection == CHAR_FOOTBALLER, drawFootballer);
    drawCharacterCard(startX + 2 * charSpacing, "BUSINESSMAN", currentCharacterSelection == CHAR_BUSINESSMAN, drawBusinessman);
    
    // Back option (responsive)
    float backPanelW = std::max<float>(120.0f, measureTextWidth("BACK") + 40.0f);
    float backX = centerX - backPanelW / 2.0f;
    if (currentCharacterSelection == CHAR_BACK) {
        drawBrickPanelWithShadow(backX, 100, backPanelW, 34, 0.8f, 0.2f, 0.2f);
        drawShadowedTextCentered(centerX, 120, "BACK", 1.0f, 1.0f, 0.0f);
    } else {
        drawBrickPanelWithShadow(backX, 100, backPanelW, 34, 0.5f, 0.5f, 0.5f);
        drawShadowedTextCentered(centerX, 120, "BACK", 0.8f, 0.8f, 0.8f);
    }
    
    // Instructions (responsive)
    const char* hint = "Use LEFT/RIGHT arrows to select, ENTER to confirm";
    float hintW = std::min<float>(WIDTH - 100, std::max<float>(420.0f, measureTextWidth(hint) + 60.0f));
    drawBrickPanelWithShadow(centerX - hintW / 2, 20, hintW, 34, 0.4f, 0.4f, 0.6f);
    drawShadowedTextCentered(centerX, 40, hint, 0.9f, 0.9f, 0.9f);
}


// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    if (gameState == START_MENU) {
        drawLayeredBackground();
        drawStartMenu();
    } else if (gameState == CHARACTER_SELECT) {
        drawLayeredBackground();
        drawCharacterSelect();
    } else if (gameState == PLAYING) {
        drawLayeredBackground();
        drawLava();
        drawPlatforms();
        drawCollectables();
        drawKey();
        drawPowerUps();
        drawRocks();
        drawPlayer();
        drawDoor();
        drawHUD();
    } else if (gameState == GAME_OVER) {
        drawLayeredBackground();
        drawGameOver();
    } else if (gameState == GAME_WIN) {
        drawLayeredBackground();
        drawGameWin();
    }
    
    glutSwapBuffers();
}

// Timer function for consistent updates
void timer(int value) {
    static int lastTime = glutGet(GLUT_ELAPSED_TIME);
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    
    // Always update background animation
    bgAnimTime += deltaTime;
    
    if (gameState == PLAYING) {
        update(deltaTime);
    } else {
        // Update menu animations
        menuAnimTime += deltaTime;
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// Reshape function
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // RGBA for alpha blending
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Icy Tower - Computer Graphics Assignment");
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    initGame();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKey);
    glutSpecialUpFunc(specialKeyUp);
    glutTimerFunc(0, timer, 0);
    
    glutMainLoop();
    return 0;
}
