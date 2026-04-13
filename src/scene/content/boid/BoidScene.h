#pragma once

#include "stdafx.h"
#include "core/SinglePassRenderer.h"
#include "scene/model/SkyBox.h"
#include "scene/physics/BoidSimulation.h"
#include "FishModel.h"
#include "SharkModel.h"
#include "scene/model/SphereModel.h"
#include "scene/model/PlaneModel.h"

class BoidScene : public SinglePassRenderer
{
public:
    BoidScene(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<SwapChain> swapChain);

    void advance()  override;
    void buildUI()  override;

    void dispatchCompute(VkCommandBuffer cmd) override;

private:
    std::shared_ptr<Pipeline> _goldfishPipeline;
    std::shared_ptr<Pipeline> _sharkPipeline;
    std::shared_ptr<Pipeline> _skyboxPipeline;
    std::shared_ptr<Pipeline> _obstaclePipeline;
    void createScenePipelines();
    void connectPipelines();
    void createObstaclePipeline();
    void createGoldfishPipeline();
    void createSharkPipeline();

    std::shared_ptr<SkyBox>      _skyboxModel;
    std::shared_ptr<FishModel>   _fishModel;
    std::shared_ptr<SharkModel>  _sharkModel;
    void createModels();
    
    void createCamera();

    // Time
    float _timeScale = 1.f;
    TimePoint _lastFrameTime = std::chrono::high_resolution_clock::now();
    float _dt = 0.f;
    bool _paused = false;

    // Spawn range (must match SPAWN_RANGE constant in .cpp)
    static constexpr float SPAWN_RANGE = 200.f;

    // Boids
    int _nBoids = 1500;
    std::unique_ptr<BoidSimulation> _boidSim;
    void resetBoids();

    // Obstacles
    std::vector<BoidObstacle> _obstacles;
    std::vector<std::shared_ptr<Model>> _obstacleModels;
    bool _showObstacles = false;
    void createObstacles(); // Models + BoidObstacles

    // Predator
    Predator _predator;
    glm::vec3 _sharkRight = glm::vec3(1.f, 0.f, 0.f); // persistent right — fallback when heading is vertical
    static constexpr float PREDATOR_MAX_STEER = 5.f; // max turning force
    void updatePredator(float dt);
};
