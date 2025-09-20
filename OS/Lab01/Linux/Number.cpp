#include "Number.h"

// Реализация конструкторов
Number::Number() : value(0.0) {}

Number::Number(double val) : value(val) {}

// Реализация операций
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
        // Обработка деления на ноль
        throw "Division by zero!";
    }
    return Number(this->value / other.value);
}

// Геттер и сеттер
double Number::getValue() const {
    return value;
}

void Number::setValue(double val) {
    value = val;
}

// Определение глобальных переменных
const Number ZERO(0.0);
const Number ONE(1.0);

// Функция создания числа
Number createNumber(double value) {
    return Number(value);
}