#include <iostream>
#include "Number.h"
#include "Vector.h"

int main() {
    std::cout << "=== Тест библиотек ===\n\n";
    
    // Тест Number
    std::cout << "1. Тест Number:\n";
    Number a(10), b(3);
    std::cout << "   10 + 3 = " << (a + b).getValue() << "\n";
    std::cout << "   10 * 3 = " << (a * b).getValue() << "\n\n";
    
    // Тест Vector
    std::cout << "2. Тест Vector:\n";
    Vector v1(3, 4);
    Vector v2(1, 2);
    
    std::cout << "   v1 = ";
    v1.print();
    std::cout << "\n";
    
    std::cout << "   v2 = ";
    v2.print();
    std::cout << "\n";
    
    Vector v3 = v1 + v2;
    std::cout << "   v1 + v2 = ";
    v3.print();
    std::cout << "\n";
    
    std::cout << "   Радиус v1 = " << v1.getRadius() << "\n";
    std::cout << "   Угол v1 = " << v1.getAngle() << " рад\n";
    
    return 0;
}
