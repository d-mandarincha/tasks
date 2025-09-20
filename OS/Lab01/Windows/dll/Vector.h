#ifndef VECTOR_H
#define VECTOR_H

#ifdef VECTORLIBRARY_EXPORTS
#define VECTOR_API __declspec(dllexport)
#else
#define VECTOR_API __declspec(dllimport)
#endif

#include "Number.h"
#include <iostream>

class VECTOR_API Vector {
private:
    Number x;  
    Number y;

public:
    Vector();
    Vector(const Number& x_val, const Number& y_val);
    Vector(double x_val, double y_val);

    Number getX() const;
    Number getY() const;
    void setX(const Number& x_val);
    void setY(const Number& y_val);

    Number getRadius() const;      
    Number getAngle() const;       

    Vector add(const Vector& other) const;
    Vector operator+(const Vector& other) const;

    void print() const;
    void printPolar() const;
};

extern VECTOR_API const Vector ZERO_VECTOR;
extern VECTOR_API const Vector ONE_ONE_VECTOR;

extern "C" VECTOR_API Vector* createVector(double x, double y);
extern "C" VECTOR_API void deleteVector(Vector* vec);
extern "C" VECTOR_API double vectorGetX(Vector* vec);
extern "C" VECTOR_API double vectorGetY(Vector* vec);

#endif // VECTOR_H