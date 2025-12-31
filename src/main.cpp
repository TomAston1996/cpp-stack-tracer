#include <iostream>

#include "instrumentor_macros.h"

static void foo()
{
    // Automatically profiles the entire function
    ST_PROFILE_FUNCTION();

    volatile int x = 0;
    for (int i = 0; i < 2'000'000; ++i)
        x += i;
}

static void bar()
{
    ST_PROFILE_FUNCTION();

    foo();

    {
        // Profile a nested scope inside bar
        ST_PROFILE_SCOPE("bar/inner");
        volatile int y = 0;
        for (int i = 0; i < 1'000'000; ++i)
            y += i;
    }
}

int main()
{
    // Start a profiling session (opens results.json)
    ST_PROFILE_BEGIN_SESSION("Example Session", "results.json");

    {
        // Optional top-level scope
        ST_PROFILE_SCOPE("main");

        foo();
        bar();
        foo();
    }

    // End the session (closes JSON properly)
    ST_PROFILE_END_SESSION();

    return 0;
}