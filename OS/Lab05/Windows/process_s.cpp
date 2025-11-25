#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string line;
    long long sum = 0; 

    if (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        int num;
        long long lineSum = 0;

        while (iss >> num) {
            lineSum += num;
        }
        
        sum += lineSum;
        std::cout << lineSum << std::endl;
    }
    return 0;
}