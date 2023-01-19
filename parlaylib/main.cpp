#include <iostream>
#include "include/parlay/primitives.h"
#include <random>
#include <chrono>
#include "cassert"

#include "include/parlay/sequence.h"

#include <atomic>

void bfs_posl(std::vector<std::vector<size_t>>& graph, std::vector<size_t>& dist) {
  std::vector<size_t> frontier(1, 0);
  size_t n = graph.size();
  std::vector<int> visited(n, 0);
  visited[0] = 1;
  dist[0] = 0;
  std::vector<size_t> frontier1;
  while (!frontier.empty()) {
    frontier1.clear();
    for (auto v : frontier) {
      for (auto u : graph[v]) {
        if (!visited[u]) {
          visited[u] = 1;
          dist[u] = dist[v] + 1;
          frontier1.push_back(u);
        }
      }
    }
    std::swap(frontier, frontier1);
  }
}

void bfs_par(std::vector<std::vector<size_t>>& graph, std::vector<size_t>& dist) {
  size_t n = graph.size();
  parlay::sequence<size_t> frontier(1, 0);
  parlay::sequence<size_t> frontier1;
  parlay::sequence<std::atomic<int>> visited(n);
  parlay::parallel_for(0, n, [&](size_t i) { visited[i] = 0; });
  visited[0] = 1;
  dist[0] = 0;
  while (!frontier.empty()) {
    parlay::sequence<size_t> degs = parlay::map(frontier, [&](size_t v) { return graph[v].size(); });
    auto scan_res = parlay::scan(degs);
    parlay::sequence<size_t> inds = scan_res.first;
    size_t neighbours_total = scan_res.second;
    frontier1 = parlay::sequence<size_t>(neighbours_total, 0);
    parlay::parallel_for(0, frontier.size(), [&](size_t i) {
      size_t v = frontier[i];
      for (size_t j = 0; j < graph[v].size(); j++) {
        size_t u = graph[v][j];
        int expected = 0;
        if (visited[u].compare_exchange_strong(expected, 1)) {
          frontier1[inds[i] + j] = u;
          dist[u] = dist[v] + 1;
        }
      }
    });
    frontier = parlay::filter(frontier1, [&](size_t v) { return v > 0; });
  }
}

size_t flat(size_t i, size_t j, size_t k, size_t N) {
  return i + j * N + k * N * N;
}
void fill_neighbours(size_t n, std::vector<size_t>& neighbours, size_t N) {
  size_t i = n % N;
  size_t j = (n / N) % N;
  size_t k = (n / N / N) % N;
  if (i > 0) neighbours.push_back(flat(i - 1, j, k, N));
  if (i < N - 1) neighbours.push_back(flat(i + 1, j, k, N));
  if (j > 0) neighbours.push_back(flat(i, j - 1, k, N));
  if (j < N - 1) neighbours.push_back(flat(i, j + 1, k, N));
  if (k > 0) neighbours.push_back(flat(i, j, k - 1, N));
  if (k < N - 1) neighbours.push_back(flat(i, j, k + 1, N));
}

void validate_result(std::vector<size_t>& dists, size_t N) {
  std::mt19937 gen(0);
  std::uniform_int_distribution<> distr(1, N - 1);

  for (int n = 0; n < 100; n++) {
    int i = distr(gen);
    int j = distr(gen);
    int k = distr(gen);
    assert(dists[flat(i, j, k, N)] == i + j + k);
  }
}

int main() {

  size_t N = 500;
  std::vector<std::vector<size_t>> graph(N * N * N);
  parlay::parallel_for(0, graph.size(), [&](size_t n) { fill_neighbours(n, graph[n], N); });

  std::vector<size_t> result(graph.size(), 0);

  std::cout << "hi" << std::endl;

  auto start = std::chrono::steady_clock::now();
  bfs_par(graph, result);
  auto end = std::chrono::steady_clock::now();
  auto time_sum = end - start;
  std::cout << "parallel run time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << "ms" << std::endl;
  validate_result(result, N);
  for (int i = 0; i < 4; i++) {
    start = std::chrono::steady_clock::now();
    bfs_par(graph, result);
    end = std::chrono::steady_clock::now();
    std::cout << "parallel run time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;
    time_sum += end - start;
  }
  time_sum /= 5;
  auto par_time = time_sum;

  std::vector<size_t> result1(graph.size(), 0);
  start = std::chrono::steady_clock::now();
  bfs_posl(graph, result1);
  end = std::chrono::steady_clock::now();
  time_sum = end - start;
  std::cout << "posledovatelniy run time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << "ms" << std::endl;
  validate_result(result1, N);
  for (int i = 0; i < 4; i++) {
    start = std::chrono::steady_clock::now();
    bfs_posl(graph, result1);
    end = std::chrono::steady_clock::now();
    std::cout << "posledovatelniy run time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;
    time_sum += end - start;
  }
  time_sum /= 5;
  auto posl_time = time_sum;

  std::cout << "avg parallel time by 5 runs: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(par_time).count() << "ms" << std::endl;

  std::cout << "avg posledovatelniy time by 5 runs: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(posl_time).count() << "ms" << std::endl;

  return 0;
}
