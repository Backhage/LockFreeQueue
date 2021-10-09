/** 
 * A queue that is thread safe without using a mutex. Uses a fixed size array
 * to hold the elements.
 */
#pragma once

#include <atomic>
#include <cstdlib>
#include <optional>
#include <type_traits>

template<typename ElemT, std::size_t MaxSize>
class LockFreeQueue {
    using Index = decltype(MaxSize);
    struct Indices {
        Index readIndex;
        Index writeIndex;
    };

    using Size = decltype(MaxSize);
    static constexpr Size BufferSize = MaxSize + 1;

    static constexpr Index nextIndex(Index currentIndex) {
        return currentIndex < MaxSize ? currentIndex + 1 : 0;
    }

    using Elem = ElemT;
    Elem* getElemByIndex(Index index) {
        return reinterpret_cast<Elem*>(&m_buffer[index]);
    }

    std::atomic<Indices> m_indices{};
    std::atomic<bool> m_occupied[BufferSize]{};
    typename std::aligned_storage_t<sizeof(Elem), alignof(Elem)> m_buffer[BufferSize];

public:
    LockFreeQueue() = default;
    ~LockFreeQueue() {
        Indices currentIndices = m_indices;
        while (currentIndices.readIndex != currentIndices.writeIndex) {
            if (m_occupied[currentIndices.readIndex]) {
                getElemByIndex(currentIndices.readIndex)->~Elem();
                m_occupied[currentIndices.readIndex] = false;
                currentIndices.readIndex = nextIndex(currentIndices.readIndex);
            }
        }
    }
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    /** 
     * Tries to insert a new element at the end of the queue. 
     * Input: Constructor arguments for the new element to place in queue
     * Return: true if element was successfully inserted, otherwise false.
    */
    template<typename... ArgTs>
    bool tryPush(ArgTs&&... args) {
        Indices currentIndices = m_indices;
        auto nextWriteIndex{ nextIndex(currentIndices.writeIndex) };
        if (nextWriteIndex == currentIndices.readIndex) {
            return false;
        }

        if (!m_indices.compare_exchange_strong(currentIndices, {currentIndices.readIndex, nextWriteIndex})) {
            return false;
        }

        new(&m_buffer[currentIndices.writeIndex]) Elem{std::forward<ArgTs>(args)...};
        m_occupied[currentIndices.writeIndex] = true;
        return true;
    }

    /** 
     * Tries to remove and return the element at the front of the queue.  
     * Return: The element wrapped in std::optional if sucessful, otherwise
     *         std::nullopt
     */
    std::optional<Elem> tryPop() {
        Indices currentIndices = m_indices;
        if (currentIndices.readIndex == currentIndices.writeIndex) {
            return std::nullopt;
        }

        auto nextReadIndex{ nextIndex(currentIndices.readIndex) };
        if (!m_indices.compare_exchange_strong(currentIndices, {nextReadIndex, currentIndices.writeIndex})) {
            return std::nullopt;
        }
        static_assert(std::is_move_constructible_v<Elem>);
        Elem elem{ std::move(*getElemByIndex(currentIndices.readIndex)) };
        static_assert(std::is_nothrow_destructible_v<Elem>);
        getElemByIndex(currentIndices.readIndex)->~Elem();
        m_occupied[currentIndices.readIndex] = false;
        return elem;
    }
};