#include <iostream>
#include <chrono>
#include <cstdlib>

auto start = std::chrono::steady_clock::now();
    
uint64_t factorial(uint64_t num)
{
    if(num < 2) {return 1;}
    return num * factorial(num--);
}

void spin(uint64_t num)
{
    // std::cout << "spin happened\n";
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    if(diff > 5) {std::exit(0);}

    uint64_t fact = factorial(num);
    spin(num++);
}

int main() 
{
    spin(1);
    return 0;
}