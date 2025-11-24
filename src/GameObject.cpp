#include "GameObject.h"

GameObject::GameObject(std::vector<GameObject*>& objects, int x, int y, int dx, int dy, int size, ObjectType type)
: objects(objects), x(x), y(y), dx(dx), dy(dy), size(size)
{
    this->destroyed = false;
    this->type = type;
    objects.push_back(this);
}

void GameObject::update(){
    this->x = x + dx;
    this->y = y + dy;
}

void GameObject::destroy(){
    destroyed = true;
}

int GameObject::getX() {
    return x;
}

int GameObject::getY() {
    return y;
}

int GameObject::getDx() {
    return dx;
}

int GameObject::getDy() {
    return dy;
}

int GameObject::getSize() {
    return size;
}

ObjectType GameObject::getType() {
    return type;
}

bool GameObject::isDestroyed(){
    return destroyed;
}

void GameObject::setX(int x) {
    this->x = x;
}

void GameObject::setY(int y) {
    this->y = y;
}

void GameObject::setDx(int dx) {
    this->dx = dx;
}

void GameObject::setDy(int dy) {
    this->dy = dy;
}

void GameObject::setSize(int size) {
    this->size = size;
}