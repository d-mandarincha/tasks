#ifndef NUMBER_H
#define NUMBER_H

class Number {
private:
    double value;

public:
    // Конструкторы
    Number();
    Number(double val);

    // Методы операций
    Number operator+(const Number& other) const;
    Number operator-(const Number& other) const;
    Number operator*(const Number& other) const;
    Number operator/(const Number& other) const;

    // Геттер
    double getValue() const;

    // Сеттер
    void setValue(double val);
};

// Глобальные переменные
extern const Number ZERO;
extern const Number ONE;

// Функция для создания числа
Number createNumber(double value);

#endif // NUMBER_H