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

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Game states
enum GameState {
    PLAYING,
    GAME_OVER,
    GAME_WIN
};

// Game variables
GameState gameState = PLAYING;
int score = 0;
int playerLives = 3;
float gameTime = 0.0f;

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
bool keySpawned = false;
float keyX, keyY;
float keyAnimTime = 0.0f;
bool keyCollected = false;
float doorAnimTime = 0.0f;
float rockSpawnTimer = 0.0f;
float powerUpSpawnTimer = 0.0f;

// Initialize game
void initGame() {
    srand(time(NULL));
    
    // Initialize player
    player.x = WIDTH / 2.0f;
    player.y = 100.0f;
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
    
    // Level platforms with different sizes (small, medium, large)
    float platformY = 150;
    for (int i = 0; i < 15; i++) {
        float platformWidth;
        if (i % 3 == 0) platformWidth = 80;  // Small
        else if (i % 3 == 1) platformWidth = 120; // Medium
        else platformWidth = 160; // Large
        
        float platformX = rand() % (int)(WIDTH - platformWidth);
        platforms.push_back({platformX, platformY, platformWidth, 15, true});
        platformY += 60 + rand() % 40;
    }
    
    // Create collectables (at least 5)
    collectables.clear();
    for (int i = 0; i < 8; i++) {
        float x = 50 + rand() % (WIDTH - 100);
        float y = 200 + i * 80 + rand() % 40;
        collectables.push_back({x, y, false, 0.0f});
    }
    
    rocks.clear();
    powerUps.clear();
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

// Draw player (4+ primitives: rectangle body, triangle head, circle hands, rectangle feet)
void drawPlayer() {
    glPushMatrix();
    glTranslatef(player.x, player.y, 0);
    
    // Shield effect if active
    if (player.powerUpType == 1) {
        glColor3f(0.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * M_PI * i / 20;
            glVertex2f(15 + 25 * cos(angle), 20 + 25 * sin(angle));
        }
        glEnd();
    }
    
    // Body (rectangle)
    glColor3f(0.2f, 0.6f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(5, 5);
    glVertex2f(25, 5);
    glVertex2f(25, 30);
    glVertex2f(5, 30);
    glEnd();
    
    // Head (triangle)
    glColor3f(1.0f, 0.8f, 0.6f);
    glBegin(GL_TRIANGLES);
    glVertex2f(15, 40);
    glVertex2f(10, 30);
    glVertex2f(20, 30);
    glEnd();
    
    // Hands (circles)
    glColor3f(1.0f, 0.8f, 0.6f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(0 + 4 * cos(angle), 20 + 4 * sin(angle));
    }
    glEnd();
    
    glBegin(GL_POLYGON);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * M_PI * i / 12;
        glVertex2f(30 + 4 * cos(angle), 20 + 4 * sin(angle));
    }
    glEnd();
    
    // Feet (rectangles)
    glColor3f(0.4f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(2, 0);
    glVertex2f(12, 0);
    glVertex2f(12, 5);
    glVertex2f(2, 5);
    glEnd();
    
    glBegin(GL_QUADS);
    glVertex2f(18, 0);
    glVertex2f(28, 0);
    glVertex2f(28, 5);
    glVertex2f(18, 5);
    glEnd();
    
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

// Draw collectables (3+ primitives: circle base, triangle sparkle, line rays)
void drawCollectables() {
    for (const auto& collectable : collectables) {
        if (collectable.collected) continue;
        
        glPushMatrix();
        glTranslatef(collectable.x, collectable.y, 0);
        glRotatef(collectable.animTime * 100, 0, 0, 1);
        
        // Coin base (circle)
        glColor3f(1.0f, 0.8f, 0.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float angle = 2.0f * M_PI * i / 16;
            float scale = 1.0f + 0.1f * sin(collectable.animTime * 5);
            glVertex2f(8 * scale * cos(angle), 8 * scale * sin(angle));
        }
        glEnd();
        
        // Sparkle (triangle)
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, 6);
        glVertex2f(-3, 0);
        glVertex2f(3, 0);
        glEnd();
        
        // Shine rays (lines)
        glColor3f(1.0f, 1.0f, 0.8f);
        glBegin(GL_LINES);
        for (int i = 0; i < 4; i++) {
            float angle = i * M_PI / 2 + collectable.animTime;
            glVertex2f(0, 0);
            glVertex2f(12 * cos(angle), 12 * sin(angle));
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

// Draw doors
void drawDoor() {
    float doorX = WIDTH / 2 - 30;
    float doorY = HEIGHT - 100;
    
    glPushMatrix();
    glTranslatef(doorX, doorY, 0);
    
    if (keyCollected) {
        // Unlocked door (2+ primitives: rectangle frame, rectangle opening)
        glColor3f(0.4f, 0.8f, 0.2f);
        
        // Door frame
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(60, 0);
        glVertex2f(60, 80);
        glVertex2f(0, 80);
        glEnd();
        
        // Opening (darker rectangle)
        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(5, 5);
        glVertex2f(55, 5);
        glVertex2f(55, 75);
        glVertex2f(5, 75);
        glEnd();
        
        // Glowing effect
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-2, -2);
        glVertex2f(62, -2);
        glVertex2f(62, 82);
        glVertex2f(-2, 82);
        glEnd();
    } else {
        // Locked door (3+ primitives: rectangle frame, rectangle door, circle lock)
        glColor3f(0.6f, 0.4f, 0.2f);
        
        // Door frame
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(60, 0);
        glVertex2f(60, 80);
        glVertex2f(0, 80);
        glEnd();
        
        // Door surface
        glColor3f(0.8f, 0.6f, 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(5, 5);
        glVertex2f(55, 5);
        glVertex2f(55, 75);
        glVertex2f(5, 75);
        glEnd();
        
        // Lock (circle)
        glColor3f(0.9f, 0.8f, 0.1f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 12; i++) {
            float angle = 2.0f * M_PI * i / 12;
            glVertex2f(30 + 8 * cos(angle), 40 + 8 * sin(angle));
        }
        glEnd();
        
        // Keyhole
        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_TRIANGLES);
        glVertex2f(30, 45);
        glVertex2f(28, 35);
        glVertex2f(32, 35);
        glEnd();
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

// Draw HUD
void drawHUD() {
    // Health bar (2+ primitives: rectangle background, rectangle health)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(10, HEIGHT - 40);
    glVertex2f(110, HEIGHT - 40);
    glVertex2f(110, HEIGHT - 20);
    glVertex2f(10, HEIGHT - 20);
    glEnd();
    
    glColor3f(1.0f, 0.2f, 0.2f);
    float healthWidth = 100.0f * playerLives / 3.0f;
    glBegin(GL_QUADS);
    glVertex2f(10, HEIGHT - 40);
    glVertex2f(10 + healthWidth, HEIGHT - 40);
    glVertex2f(10 + healthWidth, HEIGHT - 20);
    glVertex2f(10, HEIGHT - 20);
    glEnd();
    
    // Lava danger indicator (2+ primitives: rectangle background, rectangle danger)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(130, HEIGHT - 40);
    glVertex2f(230, HEIGHT - 40);
    glVertex2f(230, HEIGHT - 20);
    glVertex2f(130, HEIGHT - 20);
    glEnd();
    
    float dangerLevel = (lavaHeight / (HEIGHT * 0.8f));
    glColor3f(1.0f, dangerLevel, 0.0f);
    float dangerWidth = 100.0f * dangerLevel;
    glBegin(GL_QUADS);
    glVertex2f(130, HEIGHT - 40);
    glVertex2f(130 + dangerWidth, HEIGHT - 40);
    glVertex2f(130 + dangerWidth, HEIGHT - 20);
    glVertex2f(130, HEIGHT - 20);
    glEnd();
    
    // Score display
    glColor3f(1.0f, 1.0f, 1.0f);
    std::stringstream ss;
    ss << "Score: " << score;
    drawText(WIDTH - 150, HEIGHT - 30, ss.str().c_str());
    
    // Power-up indicator
    if (player.powerUpType > 0) {
        glColor3f(0.0f, 1.0f, 0.0f);
        std::string powerUpText = (player.powerUpType == 1) ? "SHIELD" : "DOUBLE JUMP";
        drawText(WIDTH / 2 - 40, HEIGHT - 30, powerUpText.c_str());
    }
}

// Draw game over screen
void drawGameOver() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();
    
    glColor3f(1.0f, 0.0f, 0.0f);
    drawText(WIDTH / 2 - 60, HEIGHT / 2, "GAME OVER");
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::stringstream ss;
    ss << "Final Score: " << score;
    drawText(WIDTH / 2 - 70, HEIGHT / 2 - 30, ss.str().c_str());
    
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 60, "Press R to restart or ESC to exit");
}

// Draw game win screen
void drawGameWin() {
    glColor3f(0.0f, 0.2f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();
    
    glColor3f(0.0f, 1.0f, 0.0f);
    drawText(WIDTH / 2 - 50, HEIGHT / 2, "YOU WIN!");
    
    glColor3f(1.0f, 1.0f, 1.0f);
    std::stringstream ss;
    ss << "Final Score: " << score;
    drawText(WIDTH / 2 - 70, HEIGHT / 2 - 30, ss.str().c_str());
    
    drawText(WIDTH / 2 - 100, HEIGHT / 2 - 60, "Press R to restart or ESC to exit");
}

// Update game logic
void update(float deltaTime) {
    if (gameState != PLAYING) return;
    
    gameTime += deltaTime;
    
    // Update lava
    lavaSpeed = 0.5f + gameTime * 0.01f; // Gradually increase speed
    lavaHeight += lavaSpeed * deltaTime * 10;
    
    // Remove platforms touched by lava
    for (auto& platform : platforms) {
        if (platform.y <= lavaHeight) {
            platform.active = false;
        }
    }
    
    // Update player physics
    player.velocityY -= 800.0f * deltaTime; // Gravity
    player.x += player.velocityX * deltaTime;
    player.y += player.velocityY * deltaTime;
    
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
    
    // Spawn rocks
    rockSpawnTimer -= deltaTime;
    if (rockSpawnTimer <= 0) {
        rocks.push_back({(float)(rand() % (WIDTH - 20)), (float)HEIGHT, true});
        rockSpawnTimer = 2.0f + (rand() % 3);
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
        }
    }
    
    // Spawn power-ups
    powerUpSpawnTimer -= deltaTime;
    if (powerUpSpawnTimer <= 0 && powerUps.size() < 2) {
        int type = 1 + rand() % 2;
        float x = 50 + rand() % (WIDTH - 100);
        float y = lavaHeight + 100 + rand() % 200;
        powerUps.push_back({x, y, type, true, 10.0f, 0.0f});
        powerUpSpawnTimer = 15.0f + rand() % 10;
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
            player.powerUpTimer = 8.0f;
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
    
    // Win condition
    if (keyCollected) {
        float doorX = WIDTH / 2 - 30;
        float doorY = HEIGHT - 100;
        if (checkCollision(player.x, player.y, player.width, player.height,
                         doorX, doorY, 60, 80)) {
            gameState = GAME_WIN;
        }
    }
}

// Keyboard input
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case 'r':
        case 'R':
            if (gameState != PLAYING) {
                gameState = PLAYING;
                score = 0;
                playerLives = 3;
                gameTime = 0;
                lavaHeight = 50;
                keySpawned = false;
                keyCollected = false;
                initGame();
            }
            break;
        case 'w':
        case 'W':
        case ' ':
            if (gameState == PLAYING) {
                if (player.onGround) {
                    player.velocityY = 400;
                    player.onGround = false;
                } else if (player.canDoubleJump && !player.hasDoubleJumped) {
                    player.velocityY = 300;
                    player.hasDoubleJumped = true;
                }
            }
            break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    // Handle key releases if needed
}

void specialKey(int key, int x, int y) {
    if (gameState != PLAYING) return;
    
    switch (key) {
        case GLUT_KEY_LEFT:
            player.velocityX = -200;
            break;
        case GLUT_KEY_RIGHT:
            player.velocityX = 200;
            break;
        case GLUT_KEY_UP:
            if (player.onGround) {
                player.velocityY = 400;
                player.onGround = false;
            } else if (player.canDoubleJump && !player.hasDoubleJumped) {
                player.velocityY = 300;
                player.hasDoubleJumped = true;
            }
            break;
    }
}

void specialKeyUp(int key, int x, int y) {
    if (gameState != PLAYING) return;
    
    switch (key) {
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
            player.velocityX = 0;
            break;
    }
}

// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    if (gameState == PLAYING) {
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
        drawGameOver();
    } else if (gameState == GAME_WIN) {
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
    
    update(deltaTime);
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Icy Tower - Computer Graphics Assignment");
    
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f); // Sky blue background
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
