/**
 * @file instrumentor_macros.h
 * @brief Preprocessor macros for lightweight instrumentation and profiling.
 *
 * This header defines convenience macros that wrap the instrumentation
 * system, enabling scoped and function-level profiling with minimal syntax
 * overhead.
 *
 * Profiling can be enabled or disabled at compile time using the `ST_PROFILE`
 * macro. When profiling is disabled, all macros expand to no-ops and incur
 * zero runtime cost.
 *
 * The macros support:
 * - Beginning and ending profiling sessions
 * - Scoped timing via RAII
 * - Automatic function-level profiling using compiler-specific function
 *   signature macros
 *
 * Typical usage:
 * @code
 * ST_PROFILE_BEGIN_SESSION("Startup", "trace.json");
 *
 * {
 *     ST_PROFILE_SCOPE("LoadAssets");
 *     loadAssets();
 * }
 *
 * ST_PROFILE_FUNCTION(); // Profiles the current function
 *
 * ST_PROFILE_END_SESSION();
 * @endcode
 *
 * These macros are intended to be included in translation units where
 * profiling is required and rely on the types defined in `instrumentor.h`.
 */

#pragma once

// Include the profiler types
#include "instrumentor.h"

// Config toggle to turn profiling on/off
#ifndef ST_PROFILE
#define ST_PROFILE 1
#endif

#define ST_CONCAT_INNER(a, b) a##b
#define ST_CONCAT(a, b) ST_CONCAT_INNER(a, b)

// Function sig macros
#if defined(_MSC_VER)
#define ST_FUNC_SIG __FUNCSIG__
#else
#define ST_FUNC_SIG __PRETTY_FUNCTION__
#endif

// Public macros
#if ST_PROFILE

#define ST_PROFILE_BEGIN_SESSION(name, filepath)                               \
    ::Instrumentor::get().beginSession((name), (filepath))

#define ST_PROFILE_END_SESSION() ::Instrumentor::get().endSession()

#define ST_PROFILE_SCOPE(name)                                                 \
    ::InstrumentationTimer ST_CONCAT(_st_timer_, __LINE__)(name)

#define ST_PROFILE_FUNCTION() ST_PROFILE_SCOPE(ST_FUNC_SIG)

#else

#define ST_PROFILE_BEGIN_SESSION(name, filepath) ((void)0)
#define ST_PROFILE_END_SESSION() ((void)0)
#define ST_PROFILE_SCOPE(name) ((void)0)
#define ST_PROFILE_FUNCTION() ((void)0)

#endif
