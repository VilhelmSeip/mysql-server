#include <gtest/gtest.h>

#include "unittest/gunit/gunit_test_main.h"

#include "sql/Dijkstras_functor.h"

namespace dijkstra_unittest {

class DijkstraTest : public ::testing::Test {
 protected:
  DijkstraTest(){}

 private:
  // Declares (but does not define) copy constructor and assignment operator.
  GTEST_DISALLOW_COPY_AND_ASSIGN_(DijkstraTest);
};

// check that cost is total path cost
// ! floating point cmp
void check_cost(const std::vector<Edge>& path , const double& cost){
  double expected_cost = 0;
  for (const Edge& e : path)
    expected_cost += e.cost;
  EXPECT_DOUBLE_EQ(cost, expected_cost);
}

// disables edge in map by setting cost to INFINITY
void disable_edge(const std::unordered_multimap<int, const Edge>& map, Edge& edge){
  auto range = map.equal_range(edge.from);
  for (auto it = range.first; it != range.second; it++) {
    if (it->second == edge) {
      // TODO remove const cast
      const_cast<Edge*>(&it->second)->cost = INFINITY;
      edge.cost = INFINITY;
      return;
    }
  }
}

// test dijkstra without heuristic
TEST_F(DijkstraTest, NullHeuristic) {  
  Edge edges[] = {
    // Edge{ id, from, to, cost }
    Edge{ 0, 0, 1,  5.0 },
    Edge{ 1, 0, 2, 12.0 },
    Edge{ 2, 1, 2,  5.0 },
    Edge{ 3, 1, 3, 15.0 },
    Edge{ 4, 2, 3,  5.0 },
    Edge{ 5, 0, 3, 20.0 },
    Edge{ 6, 3, 0,  1.0 },
    Edge{ 7, 3, 4,  3.0 },
    Edge{ 8, 0, 4, 17.0 }
  };
  size_t n_edges = sizeof(edges) / sizeof(Edge);
  
  std::unordered_multimap<int, const Edge> edge_map;
  for (size_t i = 0; i < n_edges; i++) {
    Edge& e = edges[i];
    edge_map.insert(std::pair(e.from, e));
  }
  double cost;
  Dijkstra dijkstra(edge_map);

  // check 0 -> 3
  std::vector<Edge> path = dijkstra(0, 3, &cost);
  std::vector<Edge> expected_path = { edges[0], edges[2], edges[4] };
  EXPECT_EQ(path, expected_path);
  check_cost(path, cost);

  // check 0 -> 4
  path = dijkstra(0, 4, &cost);
  expected_path = { edges[8] };
  EXPECT_EQ(path, expected_path);
  check_cost(path, cost);

  // check 1 -> 0
  path = dijkstra(1, 0, &cost);
  expected_path = { edges[2], edges[4], edges[6] };
  EXPECT_EQ(path, expected_path);
  check_cost(path, cost);
}

// test dijkstra with heuristic (A*)
TEST_F(DijkstraTest, EuclideanHeuristic) {
  typedef std::pair<double, double> Point;
  Point points[]{
    {  0,  0 }, // A 0
    {  2,  1 }, // B 1
    { -1, -1 }, // C 2
    {  2, -2 }, // D 3
    {  1,  3 }, // E 4
    {  4,  3 }, // F 5
    {  4,  1 }, // G 6
    {  3, -1 }, // H 7
    {  6,  2 }, // I 8
    { -2,  1 }, // J 9
    { -3, -2 }, // K 10
    { -1,  3 }  // L 11
  };
  Edge edges[] = {
    // Edge{ id, from, to, cost }
    Edge{  0,  0,  2,  1.5  }, // A 0 -> C 2
    Edge{  1,  0,  3,  2.9  }, // A 0 -> D 3
    Edge{  2,  0,  5,  5.0  }, // A 0 -> F 5
    Edge{  3,  3,  1,  3.0  }, // D 3 -> B 1
    Edge{  4,  2, 11,  2.0  }, // C 2 -> L 11
    Edge{  5,  2, 10,  2.4  }, // C 2 -> K 10
    Edge{  6,  2,  9,  2.4  }, // C 2 -> J 9
    Edge{  7,  9,  4,  3.65 }, // J 9 -> E 4
    Edge{  8,  4,  5,  4.0  }, // E 4 -> F 5
    Edge{  9,  5,  8,  2.4  }, // F 5 -> I 8
    Edge{ 10,  5,  6,  2.0  }, // F 5 -> G 6
    Edge{ 11,  0,  7,  3.2  }, // A 0 -> H 7
    Edge{ 12,  7,  6,  2.3  }, // H 7 -> G 6
    Edge{ 13,  8,  6,  2.3  }, // I 8 -> G 6
    Edge{ 14,  1,  6,  2.0  }  // B 1 -> G 6
  };
  // A 0 -> H 7 -> G 6        : 5.5m (test 1)
  // A 0 -> F 5 -> G 6        : 7.0m (test 2)
  // A 0 -> D 3 -> B 1 -> G 6 : 7.9m (test 3)
  size_t n_edges = sizeof(edges) / sizeof(Edge);
  
  std::unordered_multimap<int, const Edge> edge_map;
  for (size_t i = 0; i < n_edges; i++) {
    Edge& e = edges[i];
    edge_map.insert(std::pair(e.from, e));
  }
  double cost;
  int target_point = 6; // G
  int popped_points_null, popped_points_euclid;
  Dijkstra null_dijkstra(edge_map);
  Dijkstra euclidean_dijkstra{edge_map, [&points, &target_point](const int& point) -> double {
    return std::sqrt(
      std::pow(points[point].first - points[target_point].first, 2) +
      std::pow(points[point].second - points[target_point].second, 2)
    );
  }};
  // test 1 (euclid)
  std::vector<Edge> path = euclidean_dijkstra(0, target_point, &cost, &popped_points_euclid);
  std::vector<Edge> expected_path = { edges[11], edges[12] };
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 5.5);
  // test 1 (null)
  path = null_dijkstra(0, target_point, &cost, &popped_points_null);
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 5.5);
  EXPECT_TRUE(popped_points_euclid < popped_points_null);

  // test 2 (euclid)
  disable_edge(edge_map, edges[12]); // disables prev best path (fine since heu <= cost)
  path = euclidean_dijkstra(0, target_point, &cost, &popped_points_euclid);
  expected_path = { edges[2], edges[10] };
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 7.0);
  // test 2 (null)
  path = null_dijkstra(0, target_point, &cost, &popped_points_null);
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 7.0);
  EXPECT_TRUE(popped_points_euclid < popped_points_null);

  // test 3 (euclid)
  disable_edge(edge_map, edges[10]); // disables prev best path
  path = euclidean_dijkstra(0, target_point, &cost, &popped_points_euclid);
  expected_path = { edges[1], edges[3], edges[14] };
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 7.9);
  // test 2 (null)
  path = null_dijkstra(0, target_point, &cost, &popped_points_null);
  EXPECT_EQ(path, expected_path);
  EXPECT_DOUBLE_EQ(cost, 7.9);
  EXPECT_TRUE(popped_points_euclid < popped_points_null);
}

}  // namespace dijkstra_unittest
