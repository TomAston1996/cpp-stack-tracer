#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

struct Sample {
    double ts;
    std::vector<std::string> stack;
};

struct Event {
    double ts;
    std::string kind;
    std::string name;

    void PrintDebug() const {
        std::cout << "Kind: " << kind << "\n";
        std::cout << "Name: " << name << "\n";
        std::cout << "Timestamp: " << ts << "\n";
        std::cout << "----------------\n";
    }
};


std::vector<Event> convertToTrace(const std::vector<Sample>& samples)
{
    std::vector<Event> result;
    std::vector<std::string> running;

    for (const auto& sample : samples) {

        // 1. End functions that disappeared
        while (!running.empty() &&
               std::find(sample.stack.begin(), sample.stack.end(), running.back()) == sample.stack.end())
        {
            result.push_back(Event{sample.ts, "end", running.back()});
            running.pop_back();
        }

        // 2. Start new functions
        for (const auto& func : sample.stack) {
            if (std::find(running.begin(), running.end(), func) == running.end()) {
                result.push_back(Event{sample.ts, "start", func});
                running.push_back(func);
            }
        }
    }

    return result;
}



int main()
 {

    Sample s1{7.5, {"main"}};
    Sample s2{9.2, {"main", "my_fn"}};
    Sample s3{10.7, {"main"}};

    std::vector<Sample> samples = {s1, s2, s3};

    auto events = convertToTrace(samples);

    for (const auto& event : events) {
        std::cout << event.kind << ", " << event.ts << ", " << event.name << "\n";
    }

 }
