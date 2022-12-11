#include <iostream>
#include "include/parlay/primitives.h"
#include <random>
#include <chrono>

void qsort(std::vector<int>& data, size_t L, size_t R) {
  if (R - L < 500) {
    std::sort(data.begin() + L, data.begin() + R);
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

int main() {
  std::vector<int> vec(10000000);

  std::mt19937 gen(0);
  std::uniform_int_distribution<> distr(1, 100000000);
  for (int i = 0; i < vec.size(); i++) {
    vec[i] = distr(gen);
  }

  auto start = std::chrono::steady_clock::now();
  qsort(vec, 0, vec.size());
  auto end = std::chrono::steady_clock::now();
  std::cout << "Elapsed time in microseconds: "
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " ms" << std::endl;
  for (int i = 0; i < 10; i++) {
    std::cout << vec[i] << ' ';
  }
  return 0;
}
