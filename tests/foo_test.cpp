#include <gtest/gtest.h>

#include <foo.h>

TEST(Smoke, Works)
{
    EXPECT_EQ(1 + 1, 2);
}

TEST(TestFoo, ReturnsTheSumOfBothIntegers)
{
    int expected{2};
    int result{Foo::placeholderFunc(1, 1)};

    EXPECT_EQ(expected, result);
}