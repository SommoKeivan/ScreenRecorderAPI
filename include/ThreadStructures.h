#pragma once
#include <memory>
#include <future>

class ThreadStructures
{
	std::shared_ptr<ThreadStructures> singleton;
	std::mutex mutex;
	std::condition_variable conditionVariable;
	ThreadStructures() = default;
public:
    /**
     * Gets a ThreadStructure object for synchronize concurrency threads.
     * @return a ThreadStructure object.
     */
	static ThreadStructures& getSingleton();
    /**
	 * Gets the mutex variable.
	 * @return the mutex.
	 */
	std::mutex& getMutex();
    /**
	 * Gets the condition variable.
	 * @return the condition variable.
	 */
	std::condition_variable& getConditionVariable();

    ThreadStructures(ThreadStructures const&) = delete;
    void operator=(ThreadStructures const&) = delete;
};