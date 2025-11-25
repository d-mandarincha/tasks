#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string line;

    if (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        int num;
        bool first = true;

        while (iss >> num) {
            int result = num * 7;

            if (!first) std::cout << " ";
            std::cout << result;
            first = false;
        }

        if (!first) std::cout << std::endl;
    }

    return 0;
}