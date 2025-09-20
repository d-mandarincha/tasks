#include "Number.h"

Number::Number() : value(0.0) {}

Number::Number(double val) : value(val) {}

Number Number::operator+(const Number& other) const {
    return Number(this->value + other.value);
}

Number Number::operator-(const Number& other) const {
    return Number(this->value - other.value);
}

Number Number::operator*(const Number& other) const {
    return Number(this->value * other.value);
}

Number Number::operator/(const Number& other) const {
    if (other.value == 0.0) {
        throw "Division by zero!";
    }
    return Number(this->value / other.value);
}

double Number::getValue() const {
    return value;
}

void Number::setValue(double val) {
    value = val;
}

const Number ZERO(0.0);
const Number ONE(1.0);

Number createNumber(double value) {
    return Number(value);
}