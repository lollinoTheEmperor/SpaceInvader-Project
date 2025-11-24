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
 
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
int pinButtonUp = 13;
int pinButtonDown = 12;
 
// Player Settings
int playerSize = 8;
int playerX = SCREEN_WIDTH - playerSize - 1;
int playerY = SCREEN_HEIGHT/2;
int playerSpeed = 2;

// Enemies Settings
int enemiesAccumulator = 0;
int enemySpawnTresholdMin = 40;
int enemySpawnTresholdMax = 90;
int enemySizeMin = 2;
int enemySizeMax = 5;

// Projectile Settings
int projectileAccumulator = 0;
int projectileSpawnTreshold = 20;
int projectileSize = 0;
int projectileSpeed = 2;

std::vector<GameObject*> objects;

bool isGameOver = false;

void drawObjectCircle(GameObject* o){
  oled.fillCircle(o->getX(), o->getY(), o->getSize(), SSD1306_WHITE);
}

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

void handleInput(){
  if (digitalRead(pinButtonUp) == LOW && playerY - playerSize/2 > 0) {
    playerY -= playerSpeed;
  }
 
  if (digitalRead(pinButtonDown) == LOW && playerY + playerSize/2 < SCREEN_HEIGHT) {
    playerY += playerSpeed;
  }
}

void handleEnemySpanwn(){
  int enemySpawnTreshold = random(enemySpawnTresholdMin, enemySpawnTresholdMax);

  if(enemiesAccumulator < enemySpawnTreshold){
    enemiesAccumulator++;
  } else {

    int enemySize = random(enemySizeMin, enemySizeMax);
    int enemySpawnY = random(0 + enemySize, SCREEN_HEIGHT - enemySize);

    GameObject* enemy = new GameObject(objects, -4, enemySpawnY, 1, 0, enemySize, ObjectType::Enemy);
    enemiesAccumulator = 0;
  }
}

void handleProjectileSpawn() {
  if(projectileAccumulator < projectileSpawnTreshold){
    projectileAccumulator++;
  } else {
    GameObject* projectile = new GameObject(objects, playerX, playerY, -projectileSpeed, 0, projectileSize, ObjectType::Projectile);
    projectileAccumulator = 0;
  }
}

void destroyEnemyOnBorder(GameObject* o){
  if(o->getX() > SCREEN_WIDTH + o->getSize()){
    o->destroy();
  }
}

void destroyProjectileOnBorder(GameObject* o){
    if(o->getX() < o->getSize()){
    o->destroy();
  }
}

void projectileCollision() {
    // For each projectile, check every enemy for circle-circle overlap
    for (size_t i = 0; i < objects.size(); ++i) {
        GameObject* p = objects[i];
        if (p->getType() != ObjectType::Projectile) {
          continue;
        }

        for (size_t j = 0; j < objects.size(); ++j) {
            if (i == j) {
              continue;
            }

            GameObject* e = objects[j];
            if (e->getType() != ObjectType::Enemy) {
              continue;
            }

            int dx = p->getX() - e->getX();
            int dy = p->getY() - e->getY();
            int r  = p->getSize() + e->getSize();

            if (dx*dx + dy*dy <= r*r) {
                p->destroy();
                e->destroy();
                break;
            }
        }
    }
}

void playerCollision() {
    // Find player object. If it collides with any enemy, mark objects destroyed.
    GameObject* player = nullptr;
    for (GameObject* o : objects) {
        if (o->getType() == ObjectType::Player) { player = o; break; }
    }
    if (!player) {
      return;
    };

    for (GameObject* o : objects) {
        if (o->getType() != ObjectType::Enemy) {
          continue;
        }
        int dx = player->getX() - o->getX();
        int dy = player->getY() - o->getY();
        int r  = player->getSize() + o->getSize();
        if (dx*dx + dy*dy <= r*r) {
            Serial.println(F("Player hit - restarting"));
            // mark all objects destroyed so handleObjectDestruction will delete them
            for (GameObject* g : objects) {
              g->destroy();
            }
            
            isGameOver = true;

            return;
        }
    }
}

void handleObjectDestruction(){
  // iterate backwards to safely erase elements
  for (int i = (int)objects.size() - 1; i >= 0; --i){
    if(objects[i]->isDestroyed()){
      delete objects[i];
      objects.erase(objects.begin() + i);
    }
  }
}

void setup() {
  Serial.begin(9600);
 
  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonDown, INPUT_PULLUP);
 
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  GameObject* player = new GameObject(objects, playerX, playerY, 0, 0, playerSize, ObjectType::Player);
}
 
void loop() {
  oled.clearDisplay();
  handleObjectDestruction();

  if (isGameOver) {
    oled.setTextSize(2);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(20, SCREEN_HEIGHT / 2 - 10);
    oled.println("Game Over");
    oled.display();

    // non-blocking Display "Game Over" for 5 seconds
    static unsigned long gameOverStart = 0;
    if (gameOverStart == 0) {
      gameOverStart = millis();
    }
    if (millis() - gameOverStart < 5000UL) {
      return;
    }
    gameOverStart = 0;
    
    // Reset game state
    isGameOver = false;
    playerX = SCREEN_WIDTH - playerSize - 1;
    playerY = SCREEN_HEIGHT / 2;
    objects.clear();
    GameObject* player = new GameObject(objects, playerX, playerY, 0, 0, playerSize, ObjectType::Player);
    return;

  } else {
    for(int i = 0; i<objects.size(); i++){
      switch(objects[i]->getType()){
        case ObjectType::Player : {
          drawObjectTriangle(objects[i]);

          objects[i]->setX(playerX);
          objects[i]->setY(playerY);

          playerCollision();
          break;
        }
        case ObjectType::Enemy : {
          drawObjectCircle(objects[i]);
          destroyEnemyOnBorder(objects[i]);
          break;
        }
        case ObjectType::Projectile : {
          drawObjectCircle(objects[i]);
          destroyProjectileOnBorder(objects[i]);
          projectileCollision();
          break;
        }
      }

      objects[i]->update();
    }

    handleInput();
    handleEnemySpanwn();
    handleProjectileSpawn();
  }
  oled.display();
  delay(10);
}
