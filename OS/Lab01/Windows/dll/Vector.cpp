#include "Vector.h"
#include <cmath>  

Vector::Vector() : x(0), y(0) {}

Vector::Vector(const Number& x_val, const Number& y_val)
    : x(x_val), y(y_val) {
}

Vector::Vector(double x_val, double y_val)
    : x(x_val), y(y_val) {
}

Number Vector::getX() const { return x; }
Number Vector::getY() const { return y; }

void Vector::setX(const Number& x_val) { x = x_val; }
void Vector::setY(const Number& y_val) { y = y_val; }

Number Vector::getRadius() const {
    Number x_sqr = x * x;
    Number y_sqr = y * y;
    Number sum = x_sqr + y_sqr;
    return createNumber(sqrt(sum.getValue()));
}

Number Vector::getAngle() const {
    if (x.getValue() == 0.0 && y.getValue() == 0.0) {
        return createNumber(0.0);  
    }
    return createNumber(atan2(y.getValue(), x.getValue()));
}

Vector Vector::add(const Vector& other) const {
    Number new_x = x + other.x;
    Number new_y = y + other.y;
    return Vector(new_x, new_y);
}

Vector Vector::operator+(const Vector& other) const {
    return add(other);
}

void Vector::print() const {
    std::cout << "Vector(" << x.getValue()
        << ", " << y.getValue() << ")";
}

void Vector::printPolar() const {
    std::cout << "Vector(r=" << getRadius().getValue()
        << ", θ=" << getAngle().getValue() << " rad)";
}

const Vector ZERO_VECTOR(0.0, 0.0);
const Vector ONE_ONE_VECTOR(1.0, 1.0);

extern "C" {
    VECTOR_API Vector* createVector(double x, double y) {
        return new Vector(x, y);
    }

    VECTOR_API void deleteVector(Vector* vec) {
        delete vec;
    }

    VECTOR_API double vectorGetX(Vector* vec) {
        return vec->getX().getValue();
    }

    VECTOR_API double vectorGetY(Vector* vec) {
        return vec->getY().getValue();
    }
}