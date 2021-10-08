#include <gtest/gtest.h>
#include "LockFreeQueue.hpp"

TEST(LockFreeQueueTest, PushAndPop) {
  LockFreeQueue<int, 1> q;
  constexpr int expected = 1;
  ASSERT_TRUE(q.tryPush(expected));

  auto actual = q.tryPop();
  ASSERT_TRUE(actual.has_value());
  ASSERT_EQ(*actual, expected);
}

TEST(LockFreeQueue, MemLeakCheck) {
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

    RAII(const RAII& other) : m_data{ new int[m_size] } {
      // Don't care about deep copy of data.
    }
  };

  LockFreeQueue<RAII, 2> q;
  ASSERT_TRUE(q.tryPush());
  ASSERT_TRUE(q.tryPush());

  auto fst = q.tryPop();
  ASSERT_TRUE(fst.has_value());
}
