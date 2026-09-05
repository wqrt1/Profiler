#include <chrono>

void expensive_work()
{
    volatile long long x = 0;

    for (long long i = 0; i < 1'000'000'000; ++i)
    {
        x += i;
    }
}
void expensive_work2()
{
    volatile long long x = 0;

    for (long long i = 0; i < 1'000'000'000; ++i)
    {
        x += i;
    }
}
void expensive_work3()
{
    volatile long long x = 0;

    for (long long i = 0; i < 1'000'000'000; ++i)
    {
        x += i;
    }
}
void expensive_work4()
{
    volatile long long x = 0;

    for (long long i = 0; i < 10'000'000'000; ++i)
    {
        x += i;
    }

    expensive_work();
    expensive_work2();
    expensive_work3();
}

int main()
{
    expensive_work();
    expensive_work2();
    expensive_work3();
    expensive_work4();
}