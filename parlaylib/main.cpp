#include <iostream>
#include "include/parlay/primitives.h"
#include <random>
#include <chrono>
#include "cassert"

void qsort_posl(std::vector<int>& data, size_t L, size_t R) {
  if (R - L <= 1) {
    return;
  }
  int x = data[(L + R) / 2];
  size_t l = L;
  size_t r = R - 1;
  while (true) {
    while (data[l] < x)
      l++;
    while (data[r] > x)
      r--;
    if (l >= r) break;
    int t = data[l];
    data[l] = data[r];
    data[r] = t;
    l++;
    r--;
  }
  qsort_posl(data, L, l);
  qsort_posl(data, l, R);
}

void qsort(std::vector<int>& data, size_t L, size_t R) {
  if (R - L < 500) {
    qsort_posl(data, L, R);
  } else {

    int x = data[(L + R) / 2];
    size_t l = L;
    size_t r = R - 1;
    while (true) {
      while (data[l] < x)
        l++;
      while (data[r] > x)
        r--;
      if (l >= r) break;
      int t = data[l];
      data[l] = data[r];
      data[r] = t;
      l++;
      r--;
    }
    parlay::par_do([&] { qsort(data, L, l); }, [&] { qsort(data, l, R); });
  }
}

void test_par(std::vector<int>& data) {
  qsort(data, 0, data.size());
  for (int i = 0; i < data.size() - 1; i++) {
    assert(data[i] <= data[i + 1]);
  }
}

void test_posl(std::vector<int>& data) {
  qsort_posl(data, 0, data.size());
  for (int i = 0; i < data.size() - 1; i++) {
    assert(data[i] <= data[i + 1]);
  }
}

int main() {

  std::mt19937 gen(0);
  std::uniform_int_distribution<> distr(1, 100000000);

  for (int i = 0; i < 10; i++) {
    std::vector<int> vec_test1(100);
    std::vector<int> vec_test2(100);
    for (int i = 0; i < vec_test1.size(); i++) {
      vec_test1[i] = distr(gen);
      vec_test2[i] = vec_test1[i];
    }

    test_par(vec_test1);
    test_posl(vec_test2);
  }


  std::vector<int> vec1(100000000);
  std::vector<int> vec2(100000000);
  for (int i = 0; i < vec1.size(); i++) {
    vec1[i] = distr(gen);
    vec2[i] = vec1[i];
  }
  auto start = std::chrono::steady_clock::now();
  qsort(vec1, 0, vec1.size());
  auto end = std::chrono::steady_clock::now();
  std::cout << "parallel time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

  start = std::chrono::steady_clock::now();
  qsort_posl(vec2, 0, vec1.size());
  end = std::chrono::steady_clock::now();
  std::cout << "posled time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
  return 0;
}
