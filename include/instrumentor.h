/**
 * @file instrumentor.h
 * @brief Lightweight instrumentation and profiling utilities.
 *
 * This header provides a simple, low-overhead instrumentation system for
 * collecting scoped timing data and exporting it in Chrome Trace JSON format.
 *
 * The design is based on RAII principles: timers automatically record and
 * submit profiling results on scope exit, ensuring correctness even in the
 * presence of early returns or exceptions.
 *
 * A single global Instrumentor instance manages session lifecycle, output
 * formatting, and serialization. Profiling sessions must be explicitly
 * started and ended.
 *
 * Typical usage:
 * @code
 * Instrumentor::get().beginSession("Startup", "trace.json");
 * {
 *     InstrumentationTimer timer("LoadAssets");
 *     loadAssets();
 * }
 * Instrumentor::get().endSession();
 * @endcode
 *
 * The generated output is compatible with Chrome's tracing viewer
 * (chrome://tracing).
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>

namespace instrumentation::detail
{
/**
 * @brief Get current time since steady clock epoch in microseconds.
 * @return Microseconds since epoch.
 */
inline uint64_t nowUs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}
} // namespace instrumentation::detail

struct ProfileResult
{
    std::string name;
    uint64_t startUs, endUs;
    uint32_t threadId;
};

struct InstrumentationSession
{
    std::string name;
};

class Instrumentor
{
  public:
    /**
     * @brief Get the global Instrumentor instance.
     *
     * Returns a reference to the singleton Instrumentor. The instance is
     * lazily initialized on first use and is guaranteed to be thread-safe
     * since C++11.
     *
     * @return Reference to the Instrumentor singleton.
     */
    static Instrumentor &get()
    {
        static Instrumentor instance;
        return instance;
    }

    /**
     * @brief Begin a new instrumentation session.
     *
     * Opens the output file at the given path, truncating any existing
     * contents, writes the trace JSON header, and initializes session state.
     *
     * If a session is already active, it is ended automatically before starting
     * the new one.
     *
     * @param name     Human-readable name for the instrumentation session.
     * @param filepath Path to the output JSON file. Defaults to "results.json".
     */
    void beginSession(const std::string &name,
                      const std::string &filepath = "results.json")
    {
        if (m_currentSessionActive)
        {
            endSession();
        }

        m_outputStream.open(
            filepath, std::ios::out |
                          std::ios::trunc); // Open the file for output, and
                                            // truncate it if it already exists

        writeHeader();
        m_session = InstrumentationSession{name};
        m_currentSessionActive = true;
        m_profileCount = 0;
        m_sessionStartUs = instrumentation::detail::nowUs(); // start baseline
    }

    /**
     * @brief End the current instrumentation session.
     *
     * Writes the trace JSON footer, closes the output file, and resets all
     * session state. If no session is active, this function has no effect.
     */
    void endSession()
    {
        if (!m_currentSessionActive)
        {
            return;
        }

        writeFooter();
        m_outputStream.close();
        m_currentSessionActive = false;
        m_profileCount = 0;
        m_sessionStartUs = 0;
    }

    /**
     * @brief Write a profiling result as a trace event.
     *
     * Serializes the given profiling result as a JSON trace event and appends
     * it to the active session’s output stream.
     *
     * Timestamps are converted to be relative to the session start time. If no
     * session is currently active, this function returns without doing
     * anything.
     *
     * The output stream is flushed after writing to ensure the event is emitted
     * immediately.
     *
     * @param result Profiling result containing timing, thread, and name data.
     */
    void writeProfile(ProfileResult result)
    {
        if (!m_currentSessionActive)
        {
            return;
        }

        // make timestamps relative to session start
        result.startUs -= m_sessionStartUs;
        result.endUs -= m_sessionStartUs;

        if (m_profileCount++ > 0)
        {
            m_outputStream << ", ";
        }

        std::replace(result.name.begin(), result.name.end(), '"', '\'');

        m_outputStream << "{";
        m_outputStream << "\"dur\":" << (result.endUs - result.startUs) << ",";
        m_outputStream << "\"cat\":\"function\",";
        m_outputStream << "\"name\":\"" << result.name << "\",";
        m_outputStream << "\"ph\":\"X\",";
        m_outputStream << "\"pid\":0,";
        m_outputStream << "\"tid\":" << result.threadId << ",";
        m_outputStream << "\"ts\":" << result.startUs;
        m_outputStream << "}";
        m_outputStream.flush();
    }

  private:
    Instrumentor()
        : m_session{}, m_currentSessionActive(false), m_outputStream(),
          m_profileCount(0), m_sessionStartUs(0)
    {
    }

    /**
     * @brief Write the opening JSON header for the trace output.
     *
     * Writes the initial JSON fields and flushes the output stream so that
     * consumers can begin reading the trace data immediately.
     */
    void writeHeader()
    {
        m_outputStream << "{\"otherData\": {},\"traceEvents\":[";
        m_outputStream.flush();
    }

    /**
     * @brief Write the closing JSON footer for the trace output.
     *
     * Closes the `traceEvents` array and the root JSON object, then flushes
     * the output stream to ensure all buffered data is written.
     */
    void writeFooter()
    {
        m_outputStream << "]}";
        m_outputStream.flush();
    }

  private:
    InstrumentationSession m_session{};
    bool m_currentSessionActive{false};
    std::ofstream m_outputStream;
    int m_profileCount = 0;
    uint64_t m_sessionStartUs = 0;
};

class InstrumentationTimer
{
  public:
    InstrumentationTimer(const InstrumentationTimer &) = delete;
    InstrumentationTimer &operator=(const InstrumentationTimer &) = delete;
    InstrumentationTimer(InstrumentationTimer &&) = delete;
    InstrumentationTimer &operator=(InstrumentationTimer &&) = delete;

    /**
     * @brief Start a scoped instrumentation timer.
     *
     * Records the start time immediately upon construction. The timer will
     * automatically emit a profiling result when destroyed unless it is
     * explicitly stopped earlier.
     *
     * @param name Name of the scope being profiled. Must remain valid for the
     * timer's lifetime.
     */
    explicit InstrumentationTimer(const char *name)
        : m_name(name), m_stopped(false),
          m_startUs(instrumentation::detail::nowUs())
    {
    }

    /**
     * @brief Stop the timer if it has not already been stopped.
     */
    ~InstrumentationTimer()
    {
        if (!m_stopped)
            stop();
    }

    /**
     * @brief Stop the timer and record the profiling result.
     *
     * Captures the end time, computes the duration, and submits the profiling
     * data to the global Instrumentor. Calling this function more than once
     * has no effect after the first call.
     */
    void stop()
    {
        if (m_stopped)
        {
            return;
        }

        const uint64_t endUs = instrumentation::detail::nowUs();

        // Thread IDs are hashed to a 32-bit value for trace compatibility
        const uint32_t threadId = static_cast<uint32_t>(
            std::hash<std::thread::id>{}(std::this_thread::get_id()));

        Instrumentor::get().writeProfile({m_name, m_startUs, endUs, threadId});
        m_stopped = true;
    }

  private:
    const char *m_name;
    bool m_stopped;
    uint64_t m_startUs;
};