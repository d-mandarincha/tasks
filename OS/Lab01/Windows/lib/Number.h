#ifndef NUMBER_H
#define NUMBER_H

class Number {
private:
    double value;

public:
    Number();
    Number(double val);

    Number operator+(const Number& other) const;
    Number operator-(const Number& other) const;
    Number operator*(const Number& other) const;
    Number operator/(const Number& other) const;

    double getValue() const;

    void setValue(double val);
};

extern const Number ZERO;
extern const Number ONE;

Number createNumber(double value);

#endif // NUMBER_H