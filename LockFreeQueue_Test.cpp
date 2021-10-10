#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "LockFreeQueue.hpp"

TEST(LockFreeQueueTest, PushAndPop) {
  LockFreeQueue<int, 1> q;
  constexpr int expected = 1;
  ASSERT_TRUE(q.tryPush(expected));

  auto actual = q.tryPop();
  ASSERT_TRUE(actual.has_value());
  ASSERT_EQ(*actual, expected);
}

TEST(LockFreeQueueTest, ConstructorArgs) {
  LockFreeQueue<std::vector<int>, 1> q;
  ASSERT_TRUE(q.tryPush(1, 2, 3, 4, 5));

  auto actual = q.tryPop();
  ASSERT_TRUE(actual.has_value());
  ASSERT_EQ(actual->size(), 5);
  for (auto i = 0; i < actual->size(); i++) {
    ASSERT_EQ(actual->at(i), i+1);
  }
}

TEST(LockFreeQueueTest, DualThread) {
  LockFreeQueue<int, 100> q;

  auto push = [&q](int low, int high) { 
    for (int i = low; i < high;) {
      if (q.tryPush(i)) {
        i++;
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    }
  };

  const int low = 1;
  const int high = 100'000;

  auto pop  = [&q]() {
    std::vector<int> popped;
    while (popped.size() < (high-low)) {
      auto val = q.tryPop();
      if (val) {
        popped.push_back(*val);
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    };
    return popped;
  };

  auto pusher = std::async(std::launch::async, push, low, high);
  auto popper = std::async(std::launch::async, pop);
  auto popped = popper.get();
  pusher.wait();

  for (auto i = low; i < high; i++) {
    ASSERT_EQ(popped[i-low], i);
  }
}

TEST(LockFreeQueueTest, MemLeakCheck) {
  // Run with 'valgrind --leak-check=full' to detect memory leaks
  class RAII {
    const int m_size = 10;
    int* m_data;

  public:
    RAII() : m_data{ new int[m_size] } {
    }
    ~RAII() {
      delete [] m_data;
    }

    RAII(RAII&& other) {
      m_data = other.m_data;
      other.m_data = nullptr;
    }
  };

  const int maxSize = 10;
  LockFreeQueue<RAII, maxSize> q;
  for (auto i = 0; i < maxSize-1; i++) {
    ASSERT_TRUE(q.tryPush());
  }

  auto fst = q.tryPop();
  ASSERT_TRUE(fst.has_value());
}
