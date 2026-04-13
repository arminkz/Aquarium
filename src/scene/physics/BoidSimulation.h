#pragma once

#include "stdafx.h"
#include "vulkan/VulkanContext.h"
#include "vulkan/ComputePipeline.h"
#include "vulkan/DescriptorSet.h"
#include "vulkan/resources/Buffer.h"

// GPU-side boid state
struct BoidState {
    glm::vec4 position;
    glm::vec4 velocity;
};

// Obstacle types
enum class BoidObstacleType : int32_t { Sphere = 0, Plane = 1 };

// Must match Obstacle struct in boid_simulate.comp exactly (layout scalar, 40 bytes)
struct BoidObstacle {
    int32_t   type;            // BoidObstacleType
    glm::vec3 center;          // sphere: center,       plane: point on plane
    float     radius;          // sphere: sphere radius, plane: unused (0)
    glm::vec3 normal;          // sphere: unused,        plane: outward normal (normalized)
    float     avoidanceRadius; // detection range beyond sphere surface / from plane
    float     strength;        // steering force magnitude
};

// Must match Predator struct in boid_simulate.comp exactly (layout scalar, 44 bytes)
struct Predator {
    glm::vec3 position       = glm::vec3(50.f, 0.f, 0.f);
    float     fleeRadius     = 60.f;
    float     fleeStrength   = 80.f;
    float     chaseSpeed     = 25.f;
    float     panicSpeedMult  = 2.5f;
    glm::vec3 velocity        = glm::vec3(0.f);
    float     panicDuration   = 3.f;  // seconds boid stays panicked after leaving flee radius
};

// Must match the UBO layout in all three boid compute shaders exactly
struct BoidSimParams {
    uint32_t nBoids;
    float dt;
    float perceptionAngle; // half-angle of perception cone (radians)
    float rs;              // separation radius
    float ra;              // alignment  radius
    float rc;              // cohesion   radius
    float ws;              // separation weight
    float wa;              // alignment  weight
    float wc;              // cohesion   weight
    float minSpeed;
    float maxSpeed;
    float wrapRange;       // half-extent of spawn volume (SPAWN_RANGE)
    float cellSize;        // = max(rs, ra, rc)
    uint32_t gridDim;      // cells per axis
    uint32_t totalCells;   // gridDim^3
    uint32_t nObstacles;   // number of active obstacles
};

class BoidSimulation
{
public:
    BoidSimulation(std::shared_ptr<VulkanContext> ctx,
                   uint32_t nBoids,
                   const std::vector<BoidState>& initialState,
                   float wrapRange);
    ~BoidSimulation() = default;

    // Record one simulation step into command buffer
    void dispatch(VkCommandBuffer cmd, float dt);

    uint32_t getBoidCount() const { return _nBoids; }
    void resetBoids(const std::vector<BoidState>& state);
    void setObstacles(const std::vector<BoidObstacle>& obstacles);
    void setPredator(const Predator& p);
    glm::vec3 getNearestBoidPosition(glm::vec3 from) const;

    // The GPU buffer containing mat4 transforms for the current frame.
    // Bind as SSBO in vertex shader indexed by gl_InstanceIndex.
    VkBuffer getTransformsBuffer() const { return _transformsBuffer->getBuffer(); }

    // Public params — set before each dispatch
    float perceptionAngle = glm::pi<float>() * 0.75f; // 135° half-angle → 270° FOV
    float rs = 6.f;
    float ra = 8.f;
    float rc = 10.f;
    float ws = 1.f;
    float wa = 0.4f;
    float wc = 0.2f;
    float minSpeed = 8.f;
    float maxSpeed = 18.f;

private:
    std::shared_ptr<VulkanContext> _ctx;
    uint32_t _nBoids;
    float _wrapRange;
    int _currentIn = 0; // ping pong state

    // GPU buffers
    std::unique_ptr<Buffer> _paramsBuffer;        // host-visible UBO
    std::unique_ptr<Buffer> _boidStateBuffers[2]; // BoidState[] (ping-pong)
    std::unique_ptr<Buffer> _transformsBuffer;    // mat4[nBoids] -> vertex shader
    std::unique_ptr<Buffer> _gridCountsBuffer;    // uint[totalCells]
    std::unique_ptr<Buffer> _gridBoidsBuffer;     // uint[totalCells * MAX_PER_CELL]
    std::unique_ptr<Buffer> _obstaclesBuffer;     // host-visible BoidObstacle[MAX_OBSTACLES]
    std::unique_ptr<Buffer> _predatorBuffer;      // host-visible Predator
    void buildParamsBuffer();
    void buildBoidStateBuffers(const std::vector<BoidState>& initialState);
    void buildTransformsBuffer();
    void buildGridBuffers();
    void buildObstaclesBuffer();
    void buildPredatorBuffer();

    // Compute Pipelines
    std::unique_ptr<ComputePipeline> _clearPipeline;
    std::unique_ptr<ComputePipeline> _buildPipeline;
    std::unique_ptr<ComputePipeline> _simulatePipeline;
    void buildPipelines();

    // Descriptor sets
    std::unique_ptr<DescriptorSet> _clearDS;      // one (grid buffers)
    std::unique_ptr<DescriptorSet> _buildDS[2];   // reads current in-buffer (ping-pong)
    std::unique_ptr<DescriptorSet> _simulateDS[2];// reads in, writes out (ping-pong)
    void buildDescriptorSets();

    // Obstacles
    static constexpr uint32_t MAX_OBSTACLES = 32;
    uint32_t _nObstacles = 0;

    // Spatial Grid
    static constexpr uint32_t MAX_PER_CELL = 64;
    uint32_t _gridDim = 0;
    uint32_t _totalCells = 0;
};
