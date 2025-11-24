#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include "GameObject.h"
 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D
#define POINT_X_ENEMY_HIT 10

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Button pins
int pinButtonUp = 13;
int pinButtonDown = 12;
 
// Player settings
int playerSize = 8;
int playerX = SCREEN_WIDTH - playerSize - 1;
int playerY = SCREEN_HEIGHT/2;
int playerSpeed = 2;

// Enemy spawn settings
int enemiesAccumulator = 0;
int enemySpawnTresholdMin = 40;
int enemySpawnTresholdMax = 90;
int enemySizeMin = 2;
int enemySizeMax = 5;

// Difficulty scaling
const unsigned long difficultyUpdateInterval = 5000UL;  // update every 5s
const int enemySpawnTresholdStep = 1;                   // decrease spawn delay
unsigned long lastDifficultyUpdate  = 0;
const int minSpawnDelayLimit = 8;   
const int maxSpawnDelayLimit  = 20;

// Projectile settings
int projectileAccumulator = 0;
int projectileSpawnTreshold = 20;
int projectileSize = 0;
int projectileSpeed = 2;

// All active game objects
std::vector<GameObject*> objects;

// Game state
bool isGameOver = false;
int gameScore = 0;

// Draw circular object (enemy or projectile)
void drawObjectCircle(GameObject* o){
  oled.fillCircle(o->getX(), o->getY(), o->getSize(), SSD1306_WHITE);
}

// Draw triangular player ship
void drawObjectTriangle(GameObject* o){
    int cx = o->getX();
    int cy = o->getY();
    int s  = o->getSize();

    // Half width of an equilateral triangle
    float halfW = (float)s * 0.57735f;   // sqrt(3)/3

    // Apex (points toward negative-X)
    int x0 = cx - s / 2;
    int y0 = cy;
    // Base top
    int x1 = cx + s / 2;
    int y1 = cy - halfW;
    // Base bottom
    int x2 = cx + s / 2;
    int y2 = cy + halfW;


    oled.fillTriangle(x0, y0, x1, y1, x2, y2, SSD1306_WHITE);
}

// Handle button presses and move player
void handleInput(){
  if (digitalRead(pinButtonUp) == LOW && playerY - playerSize/2 > 0)
    playerY -= playerSpeed;
 
  if (digitalRead(pinButtonDown) == LOW && playerY + playerSize/2 < SCREEN_HEIGHT)
    playerY += playerSpeed;
}

// Spawn enemies based on accumulator and random threshold
void handleEnemySpawn(){
  int enemySpawnTreshold = random(enemySpawnTresholdMin, enemySpawnTresholdMax);

  if(enemiesAccumulator < enemySpawnTreshold){
    enemiesAccumulator++;
  } else {
    int enemySize = random(enemySizeMin, enemySizeMax);
    int enemySpawnY = random(enemySize, SCREEN_HEIGHT - enemySize);

    new GameObject(objects, -4, enemySpawnY, 1, 0, enemySize, ObjectType::Enemy);
    enemiesAccumulator = 0;
  }
}

// Spawn projectiles at fixed interval
void handleProjectileSpawn() {
  if(projectileAccumulator < projectileSpawnTreshold){
    projectileAccumulator++;
  } else {
    new GameObject(objects, playerX, playerY, -projectileSpeed, 0, projectileSize, ObjectType::Projectile);
    projectileAccumulator = 0;
  }
}

// Remove enemies exiting right border
void destroyEnemyOnBorder(GameObject* o){
  if(o->getX() > SCREEN_WIDTH + o->getSize())
    o->destroy();
}

// Remove projectiles exiting left border
void destroyProjectileOnBorder(GameObject* o){
  if(o->getX() < o->getSize())
    o->destroy();
}

// Projectile and enemy collision handling
void projectileCollision() {
    for (size_t i = 0; i < objects.size(); ++i) {
        GameObject* p = objects[i];
        if (p->getType() != ObjectType::Projectile) continue;

        for (size_t j = 0; j < objects.size(); ++j) {
            if (i == j) continue;

            GameObject* e = objects[j];
            if (e->getType() != ObjectType::Enemy) continue;

            int dx = p->getX() - e->getX();
            int dy = p->getY() - e->getY();
            
            // adding a padding to the hitbox of 1 pixel.
            int r  = p->getSize() + e->getSize() + 2; 

            // Circle–circle collision (using squared distance)
            // Use 'long' to avoid possible overflow in square calculation.
            if ((long)dx*dx + (long)dy*dy <= (long)r*r) { 

                // Large enemies shrink instead of dying
                if (e->getSize() > enemySizeMax/2) {
                  e->setSize(max(e->getSize()/2, enemySizeMin));
                  gameScore += POINT_X_ENEMY_HIT;

                } else {
                  // Small enemies are destroyed
                  p->destroy();
                  e->destroy();
                  gameScore += POINT_X_ENEMY_HIT;
                }            
                break;
            }
        }
    }
}

// Check collisions between player and enemies
void playerCollision() {
    GameObject* player = nullptr;

    for (GameObject* o : objects)
        if (o->getType() == ObjectType::Player) { player = o; break; }

    if (!player) return;

    for (GameObject* o : objects) {
        if (o->getType() != ObjectType::Enemy) continue;

        int dx = player->getX() - o->getX();
        int dy = player->getY() - o->getY();
        int r  = player->getSize() + o->getSize();

        if (dx*dx + dy*dy <= r*r) {
            // Player hit → clear objects and trigger game over
            Serial.println(F("Player hit - restarting"));
            for (GameObject* g : objects) g->destroy();
            isGameOver = true;
            return;
        }
    }
}

// Delete objects flagged for destruction
void handleObjectDestruction(){
  for (int i = (int)objects.size() - 1; i >= 0; --i){
    if(objects[i]->isDestroyed()){
      delete objects[i];
      objects.erase(objects.begin() + i);
    }
  }
}

// Display game over screen for 5 seconds (non-blocking -> live updates of millis)
void showGameOver()
{
  static unsigned long gameOverStart = 0;
  int waitTime = 5;

  if (gameOverStart == 0)
    gameOverStart = millis();

  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(10, 8);
  oled.print("Game Over");

  oled.setTextSize(1);
  oled.setCursor(10, 34);
  oled.print("Score: ");
  oled.print(gameScore);

  unsigned long elapsedSec = (millis() - gameOverStart) / 1000UL;
  int remaining = max(0, waitTime - (int)elapsedSec);

  oled.setCursor(10, 50);
  oled.print("Restart in ");
  oled.print(remaining);

  oled.display();

  // Wait period not finished
  if (millis() - gameOverStart < waitTime * 1000UL)
    return;

  // Reset state for new game
  gameOverStart = 0;
  isGameOver = false;
  playerX = SCREEN_WIDTH - playerSize - 1;
  playerY = SCREEN_HEIGHT / 2;
  gameScore = 0;

  enemySpawnTresholdMin = 40;
  enemySpawnTresholdMax = 90;
  lastDifficultyUpdate = millis();
  
  objects.clear();
  new GameObject(objects, playerX, playerY, 0, 0, playerSize, ObjectType::Player);
}

// Gradually reduce spawn delays -> increase difficulty
void updateDifficulty() {
  unsigned long now = millis();
  if (now - lastDifficultyUpdate < difficultyUpdateInterval) return;

  lastDifficultyUpdate = now;

  // Decrease spawn delays but respect limits
  if (enemySpawnTresholdMin > minSpawnDelayLimit)
    enemySpawnTresholdMin = max(minSpawnDelayLimit, enemySpawnTresholdMin - enemySpawnTresholdStep);

  if (enemySpawnTresholdMax > maxSpawnDelayLimit)
    enemySpawnTresholdMax = max(maxSpawnDelayLimit, enemySpawnTresholdMax - enemySpawnTresholdStep);
}

void setup() {
  Serial.begin(9600);
 
  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonDown, INPUT_PULLUP);
 
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    Serial.println(F("SSD1306 allocation failed"));

  // Create player object
  new GameObject(objects, playerX, playerY, 0, 0, playerSize, ObjectType::Player);
}
 
void loop() {
  oled.clearDisplay();
  // Remove objects marked for destruction
  handleObjectDestruction();

  // Check if game over
  if (isGameOver) {
    // -> Display game over + points
    showGameOver();
  } else {
    // -> Normal game loop

    updateDifficulty();

    // Update + draw all objects
    for(int i = 0; i < objects.size(); i++){
      switch(objects[i]->getType()){
        case ObjectType::Player:
          drawObjectTriangle(objects[i]);
          objects[i]->setX(playerX);
          objects[i]->setY(playerY);
          playerCollision();
          break;

        case ObjectType::Enemy:
          drawObjectCircle(objects[i]);
          destroyEnemyOnBorder(objects[i]);
          break;

        case ObjectType::Projectile:
          drawObjectCircle(objects[i]);
          destroyProjectileOnBorder(objects[i]);
          projectileCollision();
          break;
      }

      objects[i]->update();
    }

    handleInput();
    handleEnemySpawn();
    handleProjectileSpawn();
  }

  oled.display();
  delay(10);
}