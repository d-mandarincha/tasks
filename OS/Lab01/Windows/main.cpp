#include <iostream>
#include <iomanip>

#include "Number.h"  
#include "Vector.h"  

#pragma comment(lib, "VectorLibrary.lib")

void printSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

void printSubSeparator() {
    std::cout << std::string(40, '-') << std::endl;
}

void printHeader(const std::string& title) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  " << title << std::endl;
    printSeparator();
}

int main() {
    std::cout << std::fixed << std::setprecision(3);

    std::cout << "=== ТЕСТИРОВАНИЕ БИБЛИОТЕК Number и Vector ===\n";
    std::cout << "Number: статическая библиотека\n";
    std::cout << "Vector: динамическая библиотека (DLL)\n\n";

    printHeader("ТЕСТ 1: Статическая библиотека Number");

    std::cout << "1. Создание чисел:\n";
    Number num1 = createNumber(15.5);
    Number num2 = createNumber(4.5);
    std::cout << "   num1 = createNumber(15.5) = " << num1.getValue() << std::endl;
    std::cout << "   num2 = createNumber(4.5) = " << num2.getValue() << std::endl;

    std::cout << "\n2. Арифметические операции:\n";
    std::cout << "   num1 + num2 = " << (num1 + num2).getValue() << std::endl;
    std::cout << "   num1 - num2 = " << (num1 - num2).getValue() << std::endl;
    std::cout << "   num1 * num2 = " << (num1 * num2).getValue() << std::endl;
    std::cout << "   num1 / num2 = " << (num1 / num2).getValue() << std::endl;

    std::cout << "\n3. Глобальные константы:\n";
    std::cout << "   ZERO = " << ZERO.getValue() << std::endl;
    std::cout << "   ONE = " << ONE.getValue() << std::endl;

    std::cout << "\n4. Сложные вычисления:\n";
    Number result = (num1 * ONE) + (num2 * ZERO);
    std::cout << "   (num1 * ONE) + (num2 * ZERO) = " << result.getValue() << std::endl;

    std::cout << "\n5. Проверка деления на ноль:\n";
    try {
        Number bad = num1 / Number(0.0);
        std::cout << "   Ошибка: исключение не сработало!\n";
    }
    catch (const char* error) {
        std::cout << "   Поймано исключение: " << error << std::endl;
    }

    printHeader("ТЕСТ 2: Динамическая библиотека Vector");

    std::cout << "1. Создание векторов:\n";
    Vector v1(3.0, 4.0);      
    Vector v2(-2.0, 5.0);     
    Vector v3(1.0, 1.0);      

    std::cout << "   v1 = "; v1.print(); std::cout << std::endl;
    std::cout << "   v2 = "; v2.print(); std::cout << std::endl;
    std::cout << "   v3 = "; v3.print(); std::cout << std::endl;

    std::cout << "\n2. Декартовы координаты:\n";
    std::cout << "   v1.x = " << v1.getX().getValue() << ", v1.y = " << v1.getY().getValue() << std::endl;
    std::cout << "   v2.x = " << v2.getX().getValue() << ", v2.y = " << v2.getY().getValue() << std::endl;

    std::cout << "\n3. Полярные координаты:\n";
    std::cout << "   v1: "; v1.print(); std::cout << " -> "; v1.printPolar(); std::cout << std::endl;
    std::cout << "   v2: "; v2.print(); std::cout << " -> "; v2.printPolar(); std::cout << std::endl;

    std::cout << "   v1 радиус = " << v1.getRadius().getValue() << std::endl;
    std::cout << "   v1 угол = " << v1.getAngle().getValue() << " рад ("
        << v1.getAngle().getValue() * 180 / 3.14159 << " град)" << std::endl;

    std::cout << "\n4. Сложение векторов:\n";
    Vector v4 = v1 + v2;
    std::cout << "   v1 + v2 = "; v4.print(); std::cout << std::endl;

    std::cout << "\n5. Метод add():\n";
    Vector v5 = v1.add(v2);
    std::cout << "   v1.add(v2) = "; v5.print(); std::cout << std::endl;

    std::cout << "\n6. Изменение координат:\n";
    std::cout << "   До: "; v3.print(); std::cout << std::endl;
    v3.setX(10.0);
    v3.setY(20.0);
    std::cout << "   После setX(10), setY(20): "; v3.print(); std::cout << std::endl;

    return 0;
}