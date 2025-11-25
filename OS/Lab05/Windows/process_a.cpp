#include <iostream>
#include <sstream>
#include <string>

const int N = 26; 

int main() {
    std::string line;

    if (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        int num;
        bool first = true;

        while (iss >> num) {
            int result = num + N;  

            if (!first) std::cout << " ";
            std::cout << result;
            first = false;
        }

        if (!first) std::cout << std::endl;
    }

    return 0;
}