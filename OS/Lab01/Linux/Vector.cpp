#include "Vector.h"
#include <cmath>
#include <iostream>

Vector::Vector() : x(0), y(0) {}

Vector::Vector(double x_val, double y_val) : x(x_val), y(y_val) {}

double Vector::getX() const { 
    return x.getValue(); 
}

double Vector::getY() const { 
    return y.getValue(); 
}

void Vector::setX(double x_val) { 
    x.setValue(x_val); 
}

void Vector::setY(double y_val) { 
    y.setValue(y_val); 
}

double Vector::getRadius() const {
    double x_val = getX();
    double y_val = getY();
    return sqrt(x_val * x_val + y_val * y_val);
}

double Vector::getAngle() const {
    double x_val = getX();
    double y_val = getY();
    if (x_val == 0.0 && y_val == 0.0) return 0.0;
    return atan2(y_val, x_val);
}

Vector Vector::operator+(const Vector& other) const {
    return Vector(getX() + other.getX(), getY() + other.getY());
}

void Vector::print() const {
    std::cout << "Vector(" << getX() << ", " << getY() << ")";
}
