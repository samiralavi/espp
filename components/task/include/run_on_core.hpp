#pragma once

#include "task.hpp"

namespace espp {
namespace task {
#if defined(ESP_PLATFORM) || defined(_DOXYGEN_)
/// Run the given function on the specific core, then return the result (if any)
/// @details This function will run the given function on the specified core,
///         then return the result (if any). If the provided core is the same
///         as the current core, the function will run directly. If the
///         provided core is different, the function will be run on the
///         specified core and the result will be returned to the calling
///         thread. Note that this function will block the calling thread until
///         the function has completed, regardless of the core it is run on.
/// @param f The function to run
/// @param core_id The core to run the function on
/// @param stack_size_bytes The stack size to allocate for the function
/// @param priority The priority of the task
/// @note This function is only available on ESP32
/// @note If you provide a core_id < 0, the function will run on the current
///       core (same core as the caller)
/// @note If you provide a core_id >= configNUM_CORES, the function will run on
///       the last core
static auto run_on_core(const auto &f, int core_id, size_t stack_size_bytes = 2048,
                        size_t priority = 5) {
  if (core_id < 0 || core_id == xPortGetCoreID()) {
    // If no core id specified or we are already executing on the desired core,
    // run the function directly
    return f();
  } else {
    // Otherwise run the function on the desired core
    if (core_id > configNUM_CORES - 1) {
      // If the core id is larger than the number of cores, run on the last core
      core_id = configNUM_CORES - 1;
    }
    std::mutex mutex;
    std::unique_lock lock(mutex); // cppcheck-suppress localMutex
    std::condition_variable cv;   ///< Signal for when the task is done / function is run
    if constexpr (!std::is_void_v<decltype(f())>) {
      // the function returns something
      decltype(f()) ret_val;
      auto f_task = espp::Task::make_unique(espp::Task::Config{
          .name = "run_on_core_task",
          .callback = [&mutex, &cv, &f, &ret_val](auto &cb_m, auto &cb_cv) -> bool {
            // synchronize with the main thread - block here until the main thread
            // waits on the condition variable (cv), then run the function
            std::unique_lock lock(mutex);
            // run the function
            ret_val = f();
            // signal that the task is done
            cv.notify_all();
            return true; // stop the task
          },
          .stack_size_bytes = stack_size_bytes,
          .priority = priority,
          .core_id = core_id,
      });
      f_task->start();
      cv.wait(lock);
      return ret_val;
    } else {
      // the function returns void
      auto f_task = espp::Task::make_unique(espp::Task::Config{
          .name = "run_on_core_task",
          .callback = [&mutex, &cv, &f](auto &cb_m, auto &cb_cv) -> bool {
            // synchronize with the main thread - block here until the main thread
            // waits on the condition variable (cv), then run the function
            std::unique_lock lock(mutex);
            // run the function
            f();
            // signal that the task is done
            cv.notify_all();
            return true; // stop the task
          },
          .stack_size_bytes = stack_size_bytes,
          .priority = priority,
          .core_id = core_id,
      });
      f_task->start();
      cv.wait(lock);
    }
  }
}
#endif
} // namespace task
} // namespace espp
