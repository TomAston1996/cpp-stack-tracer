#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

#include "instrumentor.h"

static std::string readFile(const std::filesystem::path &p)
{
    std::ifstream in(p, std::ios::in);
    std::string s((std::istreambuf_iterator<char>(in)),
                  std::istreambuf_iterator<char>());
    return s;
}

// Extract number of trace events by counting occurrences of `"ph":"X"`
// (each event uses ph:"X" in your writer)
static int countEvents(const std::string &json)
{
    int count = 0;
    std::string needle = "\"ph\":\"X\"";
    size_t pos = 0;
    while ((pos = json.find(needle, pos)) != std::string::npos)
    {
        ++count;
        pos += needle.size();
    }
    return count;
}

// A small fixture that gives each test its own output file and cleans up.
class InstrumentorTest : public ::testing::Test
{
  protected:
    std::filesystem::path outPath;

    void SetUp() override
    {
        outPath = std::filesystem::temp_directory_path() /
                  "instrumentor_test_trace.json";
        // Ensure clean slate
        std::error_code ec;
        std::filesystem::remove(outPath, ec);

        // Ensure singleton isn't left in an active session from previous tests
        Instrumentor::get().endSession();
    }

    void TearDown() override
    {
        // End session if still open (tests should end it, but this is
        // defensive)
        Instrumentor::get().endSession();

        std::error_code ec;
        std::filesystem::remove(outPath, ec);
    }
};

TEST_F(InstrumentorTest, BeginEndSession_WritesJsonEnvelope)
{
    // Arrange & Act
    Instrumentor::get().beginSession("TestSession", outPath.string());
    Instrumentor::get().endSession();

    // Assert
    ASSERT_TRUE(std::filesystem::exists(outPath));

    const std::string json = readFile(outPath);
    EXPECT_NE(json.find("{\"otherData\": {},\"traceEvents\":["),
              std::string::npos);
    EXPECT_TRUE(!json.empty());
    EXPECT_EQ(json.rfind("]}"), json.size() - 2) << "File should end with ]}";
    EXPECT_EQ(countEvents(json), 0);
}

TEST_F(InstrumentorTest, Timer_RaiiWritesOneEvent)
{
    // Arrange & Act
    Instrumentor::get().beginSession("RAIITest", outPath.string());
    {
        InstrumentationTimer t("ScopeA");
        // Do nothing; we only care that it writes on destruction
    }
    Instrumentor::get().endSession();

    // Assert
    const std::string json = readFile(outPath);
    EXPECT_EQ(countEvents(json), 1);
    EXPECT_NE(json.find("\"name\":\"ScopeA\""), std::string::npos);
    EXPECT_NE(json.find("\"dur\":"), std::string::npos);
    EXPECT_NE(json.find("\"ts\":"), std::string::npos);
    EXPECT_NE(json.find("\"tid\":"), std::string::npos);
}

TEST_F(InstrumentorTest, Timer_StopIsIdempotent_DoesNotWriteTwice)
{
    // Arrange & Act
    Instrumentor::get().beginSession("StopIdempotent", outPath.string());
    {
        InstrumentationTimer t("ScopeStopOnce");
        t.stop();
        t.stop(); // should have no effect
    }
    Instrumentor::get().endSession();

    // Assert
    const std::string json = readFile(outPath);
    EXPECT_EQ(countEvents(json), 1);
}

TEST_F(InstrumentorTest, NestedTimers_WriteTwoEvents_WithCommaSeparator)
{
    // Arrange & Act
    Instrumentor::get().beginSession("Nested", outPath.string());
    {
        InstrumentationTimer outer("Outer");
        {
            InstrumentationTimer inner("Inner");
        }
    }
    Instrumentor::get().endSession();

    // Assert
    const std::string json = readFile(outPath);
    EXPECT_EQ(countEvents(json), 2);
    EXPECT_NE(json.find("}, {"), std::string::npos)
        << "Expected comma+space separator between JSON objects";
}

TEST_F(InstrumentorTest, WriteProfile_SanitizesDoubleQuotesInName)
{
    // Arramge & Act
    Instrumentor::get().beginSession("Sanitize", outPath.string());
    {
        InstrumentationTimer t("NameWith\"Quote");
    }
    Instrumentor::get().endSession();

    // Assert
    const std::string json = readFile(outPath);

    // The original double quote should not appear inside the name field.
    // It should be replaced with a single quote.
    EXPECT_EQ(json.find("NameWith\\\"Quote"),
              std::string::npos); // shouldn't contain escaped quote
    EXPECT_NE(json.find("\"name\":\"NameWith'Quote\""), std::string::npos);
}

TEST_F(InstrumentorTest, EndSession_WhenNotActive_IsNoOpAndDoesNotCreateFile)
{
    // No beginSession call
    Instrumentor::get().endSession();

    // File should not exist because we never opened it
    EXPECT_FALSE(std::filesystem::exists(outPath));
}

TEST_F(InstrumentorTest, BeginSession_Twice_EndsPreviousAndCreatesNewFile)
{
    // Arrange
    std::filesystem::path out1 = std::filesystem::temp_directory_path() /
                                 "instrumentor_test_trace1.json";
    std::filesystem::path out2 = std::filesystem::temp_directory_path() /
                                 "instrumentor_test_trace2.json";
    std::error_code ec;
    std::filesystem::remove(out1, ec);
    std::filesystem::remove(out2, ec);

    // Act
    Instrumentor::get().beginSession("A", out1.string());
    {
        InstrumentationTimer t("EventA");
    }

    // This should end session A and start session B
    Instrumentor::get().beginSession("B", out2.string());
    {
        InstrumentationTimer t("EventB");
    }
    Instrumentor::get().endSession();

    // Assert
    ASSERT_TRUE(std::filesystem::exists(out1));
    ASSERT_TRUE(std::filesystem::exists(out2));

    const std::string json1 = readFile(out1);
    const std::string json2 = readFile(out2);

    EXPECT_EQ(countEvents(json1), 1);
    EXPECT_NE(json1.find("\"name\":\"EventA\""), std::string::npos);

    EXPECT_EQ(countEvents(json2), 1);
    EXPECT_NE(json2.find("\"name\":\"EventB\""), std::string::npos);

    std::filesystem::remove(out1, ec);
    std::filesystem::remove(out2, ec);
}
