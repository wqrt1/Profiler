#include <chrono>

void expensive_work()
{
    volatile long long x = 0;

    for (long long i = 0; i < 1'000'000'000; ++i)
    {
        x += i;
    }
}

int main()
{
    expensive_work();
}