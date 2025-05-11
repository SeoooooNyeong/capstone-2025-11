#include "path_navigator.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PathNavigator", __VA_ARGS__)

namespace {
    constexpr float kDeviationThreshold = 3.0f;
    constexpr float kReachThreshold = 0.8f;
}

PathNavigator::PathNavigator() {
    obstacles_ = GenerateObstacles();
}

void PathNavigator::SetGoal(const Point& goal) {
    LOGI("SetGoal_Check: x = %.2f, z = %.2f", goal.x, goal.z);
    goal_ = goal;
    goal_set_ = true;
}

void PathNavigator::TryGeneratePathIfNeeded(const Point& camera_pos) {
    if (!goal_set_) {
        LOGI("❌ 목적지(goal_)가 설정되지 않아 경로 생성 생략");
        return;
    }

    if (path_generated_) return;

    std::set<Point> obstacles = GenerateObstacles();

    path_ = astar(camera_pos, goal_, obstacles);

    if (!path_.empty()) {
        path_generated_ = true;
        path_ready_to_render_ = true;
        arrival_ = false;
        LOGI("🚀 경로 탐색 성공!");

//        JavaBridge::EnqueueAudio("start.m4a");
        JavaBridge::SpeakText("경로 안내를 시작합니다. 진동이 나는 방향을 찾아주세요.");

    } else {
        LOGI("❌ 경로 탐색 실패");
    }
}

bool PathNavigator::UpdateNavigation(const Point& cam_pos, const float* matrix, DirectionHelper& direction_helper) {
    if (!goal_set_) {
        LOGI("❌ 목적지(goal_)가 설정되지 않아 경로 확인 생략");
        return true;
    }

    if (current_path_index_ >= path_.size()) {
        if (!arrival_) {
//            JavaBridge::EnqueueAudio("arrival.m4a");
            JavaBridge::SpeakText("목적지에 도착하였습니다. 경로 안내를 종료합니다.");
            arrival_ = true;
        }

        // 상태 업데이트 메시지 전달
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "목적지에 도착하였습니다");
        JavaBridge::UpdatePathStatus(buffer);

        return true;
    }

    if (current_path_index_ > 0 && current_path_index_ < path_.size() - 2) {
        Point prev = path_[current_path_index_];
        Point current = path_[current_path_index_ + 1];
        Point next = path_[current_path_index_ + 2];
    
        float dx1 = current.x - prev.x;
        float dz1 = current.z - prev.z;
        float dx2 = next.x - current.x;
        float dz2 = next.z - current.z;
    
        float dot = dx1 * dx2 + dz1 * dz2;
        float mag1 = std::sqrt(dx1 * dx1 + dz1 * dz1);
        float mag2 = std::sqrt(dx2 * dx2 + dz2 * dz2);
    
        if (mag1 > 0.01f && mag2 > 0.01f) {
            float angle_cos = dot / (mag1 * mag2);
            if (angle_cos < 0.85f) {
                JavaBridge::SpeakText("곧 방향 회전이 있습니다. 진동이 나는 방향을 찾아주세요.");
            }
        }
    }

    Point target = path_[current_path_index_];
    float dx = cam_pos.x - target.x;
    float dz = cam_pos.z - target.z;
    float distance = std::sqrt(dx * dx + dz * dz);

    if (distance > kDeviationThreshold) {
        LOGI("🚨 경로 이탈 감지됨. 재탐색 시작");
//        JavaBridge::EnqueueAudio("deviation.m4a");
        JavaBridge::SpeakText("경로를 이탈하였습니다. 경로를 재탐색합니다.");
        Reset();
        TryGeneratePathIfNeeded(cam_pos);
        return false;
    }

    direction_helper.Check(matrix, cam_pos, target);

    if (distance < kReachThreshold) {
        direction_helper.Reset();
        current_path_index_++;
        LOGI("✅ 경로 지점 %d 도달", current_path_index_);
    }

    // 상태 업데이트 메시지 전달
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "📍 현재 경로 지점 %d / %.2fm 남음", current_path_index_, distance);
    JavaBridge::UpdatePathStatus(buffer);

    return false;
}

bool PathNavigator::IsReadyToRender() const {
    return path_ready_to_render_;
}

void PathNavigator::SetReadyToRenderFalse() {
    path_ready_to_render_ = false;
}

const std::vector<Point>& PathNavigator::GetPath() const {
    return path_;
}

int PathNavigator::GetCurrentPathIndex() const {
    return current_path_index_;
}


void PathNavigator::Reset() {
    path_.clear();
    path_generated_ = false;
    path_ready_to_render_ = false;
    arrival_ = false;
    current_path_index_ = 0;
    goal_set_ = false;
}

std::set<Point> PathNavigator::GenerateObstacles() {
    std::set<Point> obstacles;

    std::vector<Point> outer = {
        {-11.5f, 1.8f}, {-11.5f, -20.25f}, {1.5f, -20.25f}, {1.5f, 1.8f}
    };
    std::vector<Point> inner = {
        {-8.58f, -0.6f}, {-8.58f, -15.89f}, {-1.49f, -15.89f}, {-1.49f, -0.6f}
    };

    for (int i = 0; i < outer.size(); ++i) {
        auto wall = generateWall(outer[i], outer[(i + 1) % outer.size()]);
        obstacles.insert(wall.begin(), wall.end());
    }
    for (int i = 0; i < inner.size(); ++i) {
        auto wall = generateWall(inner[i], inner[(i + 1) % inner.size()]);
        obstacles.insert(wall.begin(), wall.end());
    }

    return obstacles;
}
