#include "include/ThreadStructures.h"

ThreadStructures& ThreadStructures::getSingleton() {
    static ThreadStructures singleton{};
    return singleton;
}

std::mutex& ThreadStructures::getMutex() {
    return this->mutex;
}

std::condition_variable& ThreadStructures::getConditionVariable() {
    return this->conditionVariable;
}
