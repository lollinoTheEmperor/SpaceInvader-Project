#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <vector>

enum class ObjectType {
    Player,
    Enemy,
    Projectile
};

class GameObject {

public:
    GameObject(std::vector<GameObject*>& objects, int x, int y, int dx, int dy, int size, ObjectType type);

    void spawn(int x, int y);
    void update();
    void destroy();

    int getX();
    int getY();
    int getDx();
    int getDy();
    int getSize();
    ObjectType getType();
    bool isDestroyed();
    void setX(int x);
    void setY(int y);
    void setDx(int dx);
    void setDy(int dy);
    void setSize(int size);

private:
    int x;
    int y;
    int dx;
    int dy;
    int size;
    ObjectType type;
    std::vector<GameObject*>& objects;
    bool destroyed;
};

#endif