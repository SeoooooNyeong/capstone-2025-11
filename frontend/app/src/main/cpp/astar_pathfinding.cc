#include "astar_pathfinding.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <queue>
#include <fstream>
#include <sstream>
#include <set>
#include <limits>

float heuristic(const Point& a, const Point& b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}

void AStarPathfinder::LoadPoseGraph(const std::string& path, int floor) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOGI("❌ pose_graph.txt 열기 실패: %s", path.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) tokens.push_back(token);
        if (tokens.size() < 3) continue;

        float x = std::stof(tokens[1]);
        float z = std::stof(tokens[2]);
        pose_graph_by_floor_[floor].emplace_back(Point{x, z});
    }
    file.close();

    LOGI("✅ %d층 pose_graph.txt → %zu개 노드 로드 완료", floor, pose_graph_by_floor_[floor].size());

    int n = pose_graph_by_floor_[floor].size();
    adjacency_list_by_floor_[floor].resize(n);

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float dist = heuristic(pose_graph_by_floor_[floor][i], pose_graph_by_floor_[floor][j]);
            if (dist <= threshold) {
                adjacency_list_by_floor_[floor][i].emplace_back(j, dist);
                adjacency_list_by_floor_[floor][j].emplace_back(i, dist);
            }
        }
    }

    for (const auto& [floor_num, nodes] : pose_graph_by_floor_) {
        LOGI("📍 [층 %d] 노드 수: %zu", floor_num, nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i) {
            const Point& p = nodes[i];
            LOGI("  - [%zu] x: %.3f, z: %.3f", i, p.x, p.z);
        }
    }
}

int AStarPathfinder::FindClosestNode(const Point& coord, int floor) {
    float min_dist = std::numeric_limits<float>::max();
    int closest = -1;
    const auto& graph = pose_graph_by_floor_[floor];
    for (int i = 0; i < graph.size(); ++i) {
        float dist = heuristic(coord, graph[i]);
        if (dist < min_dist) {
            min_dist = dist;
            closest = i;
        }
    }
    return closest;
}

std::vector<Point> AStarPathfinder::astar(const Point& start_coord, const Point& goal_coord, int floor) {
    const auto& pose_graph = pose_graph_by_floor_[floor];
    const auto& adjacency_list = adjacency_list_by_floor_[floor];

    int start = FindClosestNode(start_coord, floor);
    int goal = FindClosestNode(goal_coord, floor);
    if (start == -1 || goal == -1) return {};

    using Node = std::pair<float, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open_set;
    std::vector<float> g_score(pose_graph.size(), std::numeric_limits<float>::max());
    std::vector<float> f_score(pose_graph.size(), std::numeric_limits<float>::max());
    std::vector<int> came_from(pose_graph.size(), -1);

    g_score[start] = 0;
    f_score[start] = heuristic(pose_graph[start], pose_graph[goal]);
    open_set.emplace(f_score[start], start);

    while (!open_set.empty()) {
        int current = open_set.top().second;
        open_set.pop();

        if (current == goal) {
            std::vector<Point> path;
            for (int at = goal; at != -1; at = came_from[at]) {
                path.push_back(pose_graph[at]);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& [neighbor, cost] : adjacency_list[current]) {
            float tentative_g = g_score[current] + cost;
            if (tentative_g < g_score[neighbor]) {
                came_from[neighbor] = current;
                g_score[neighbor] = tentative_g;
                f_score[neighbor] = tentative_g + heuristic(pose_graph[neighbor], pose_graph[goal]);
                open_set.emplace(f_score[neighbor], neighbor);
            }
        }
    }

    return {};
}
