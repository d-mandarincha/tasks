#ifndef VECTOR_H
#define VECTOR_H

#include "Number.h"  // ПРОСТО ПОДКЛЮЧАЕМ, БЕЗ ПРЕДВАРИТЕЛЬНЫХ ОБЪЯВЛЕНИЙ

class Vector {
private:
    Number x;  // Прямое использование Number
    Number y;
    
public:
    Vector();
    Vector(double x_val, double y_val);
    
    double getX() const;
    double getY() const;
    void setX(double x_val);
    void setY(double y_val);
    
    double getRadius() const;
    double getAngle() const;
    
    Vector operator+(const Vector& other) const;
    
    void print() const;
};

#endif
