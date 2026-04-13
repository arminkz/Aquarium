#include "BoidScene.h"

#include "core/AssetPath.h"
#include "gui/FontAwesome.h"
#include "scene/camera/TurnTableCamera.h"
#include "vulkan/VulkanHelper.h"

BoidScene::BoidScene(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<SwapChain> swapChain)
    : SinglePassRenderer(std::move(ctx), std::move(swapChain))
{
    createModels();
    createScenePipelines();
    createGoldfishPipeline();
    createSharkPipeline();
    createObstaclePipeline();
    connectPipelines();
    createCamera();
    createObstacles();
    _boidSim->setObstacles(_obstacles);
    _boidSim->setPredator(_predator);
}


void BoidScene::createObstacles()
{

    auto addPlaneObstacle = [&](glm::vec3 center, glm::vec3 normal) {
        // Boid Logic
        BoidObstacle plane{};
        plane.type = static_cast<int32_t>(BoidObstacleType::Plane);
        plane.center = center;
        plane.normal = normal;
        plane.avoidanceRadius = 30.f;
        plane.strength = 10.f;
        _obstacles.push_back(plane);

        // Renderable
        float wallSize = SPAWN_RANGE * 2.f;
        std::shared_ptr<Model> model = std::make_shared<PlaneModel>(_ctx,center,normal,wallSize,wallSize);
        model->setPipeline(_obstaclePipeline);
        _obstacleModels.push_back(model);
    };

    auto addSphereObstacle = [&](glm::vec3 center, float radius) {
        // Boid Logic
        BoidObstacle sphere{};
        sphere.type = static_cast<int32_t>(BoidObstacleType::Sphere);
        sphere.center = center;
        sphere.radius = radius;
        sphere.avoidanceRadius = radius + 5.f;
        sphere.strength = 30.f;
        _obstacles.push_back(sphere);

        // Renderable
        std::shared_ptr<Model> model = std::make_shared<SphereModel>(_ctx,center,radius);
        model->setPipeline(_obstaclePipeline);
        _obstacleModels.push_back(model);
    };

    // Bounding box walls - 6 planes, normals point inward
    for (int axis = 0; axis < 3; axis++)
        for (int sign : {-1, 1}) {
            glm::vec3 normal(0.f); normal[axis] = -sign;
            glm::vec3 center(0.f); center[axis] =  sign * SPAWN_RANGE;
            addPlaneObstacle(center, normal);
        }

    // 8 sphere obstacles at the cube corners
    for (int x : {-1, 1})
        for (int y : {-1, 1})
            for (int z : {-1, 1})
                addSphereObstacle(glm::vec3(x, y, z) * SPAWN_RANGE, 50.f);

    // 10 random spheres inside the cube
    std::mt19937 rng(123);
    std::uniform_real_distribution<float> posDist(-SPAWN_RANGE * 1.2, SPAWN_RANGE * 1.2);
    std::uniform_real_distribution<float> radDist(15.f, 25.f);
    for (int i = 0; i < 14; i++)
        addSphereObstacle({posDist(rng), posDist(rng), posDist(rng)}, radDist(rng));
}


void BoidScene::createModels()
{
    _skyboxModel = std::make_shared<SkyBox>(_ctx, "textures/aqua_skybox");
    _sceneModels.push_back(_skyboxModel);

    _fishModel = std::make_shared<FishModel>(
        _ctx,
        AssetPath::getInstance()->get("meshes/Goldfish/Mesh_Goldfish.obj"),
        AssetPath::getInstance()->get("meshes/Goldfish/Tex_Goldfish.png"),
        0.04f);
    _sceneModels.push_back(_fishModel);

    _sharkModel = std::make_shared<SharkModel>(
        _ctx,
        AssetPath::getInstance()->get("meshes/Shark/Mesh_Shark.obj"),
        AssetPath::getInstance()->get("meshes/Shark/Tex_Shark.png"),
        0.08f);

    resetBoids();
}


void BoidScene::createGoldfishPipeline()
{
    PipelineParams params;
    params.name = "GoldfishPipeline";
    params.vertexBindingDescription = Vertex::getBindingDescription();
    params.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    params.descriptorSetLayouts = {
        _sceneDescriptorSets[0]->getDescriptorSetLayout(),              // set=0: SceneInfo
        _fishModel->getTransformDescriptorSet()->getDescriptorSetLayout(), // set=1: Transforms SSBO
        _fishModel->getTextureDescriptorSet()->getDescriptorSetLayout(),   // set=2: Texture
    };
    params.pushConstantRanges = {};
    params.renderPass = _mainRenderPass->getRenderPass();
    params.cullMode = VK_CULL_MODE_BACK_BIT;
    params.msaaSamples = _msaaSamples;
    _goldfishPipeline = std::make_shared<Pipeline>(_ctx,
        AssetPath::getInstance()->get("spv/boid/goldfish_vert.spv"),
        AssetPath::getInstance()->get("spv/boid/goldfish_frag.spv"),
        params);
}


void BoidScene::createSharkPipeline()
{
    PipelineParams params;
    params.name = "SharkPipeline";
    params.vertexBindingDescription = Vertex::getBindingDescription();
    params.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    params.descriptorSetLayouts = {
        _sceneDescriptorSets[0]->getDescriptorSetLayout(),               // set=0: SceneInfo
        _sharkModel->getTransformDescriptorSet()->getDescriptorSetLayout(), // set=1: Transform SSBO
        _sharkModel->getTextureDescriptorSet()->getDescriptorSetLayout(),   // set=2: Texture
    };
    params.pushConstantRanges = {};
    params.renderPass = _mainRenderPass->getRenderPass();
    params.cullMode = VK_CULL_MODE_BACK_BIT;
    params.msaaSamples = _msaaSamples;
    _sharkPipeline = std::make_shared<Pipeline>(_ctx,
        AssetPath::getInstance()->get("spv/boid/shark_vert.spv"),
        AssetPath::getInstance()->get("spv/boid/goldfish_frag.spv"),
        params);
}


void BoidScene::createObstaclePipeline()
{
    PipelineParams params;
    params.name = "ObstaclePipeline";
    params.vertexBindingDescription = Vertex::getBindingDescription();
    params.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    params.descriptorSetLayouts = { _sceneDescriptorSets[0]->getDescriptorSetLayout() };
    params.pushConstantRanges = { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)} };
    params.renderPass = _mainRenderPass->getRenderPass();
    params.cullMode = VK_CULL_MODE_BACK_BIT;
    params.msaaSamples = _msaaSamples;
    _obstaclePipeline = std::make_shared<Pipeline>(_ctx,
        AssetPath::getInstance()->get("spv/shader_vert.spv"),
        AssetPath::getInstance()->get("spv/shader_frag.spv"),
        params);
}




void BoidScene::createScenePipelines()
{
    // Skybox pipeline
    PipelineParams skyParams;
    skyParams.name                        = "SkyBoxPipeline";
    skyParams.vertexBindingDescription    = Vertex::getBindingDescription();
    skyParams.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    skyParams.descriptorSetLayouts        = {
        _sceneDescriptorSets[0]->getDescriptorSetLayout(),
        _skyboxModel->getDescriptorSet()->getDescriptorSetLayout()
    };
    skyParams.pushConstantRanges          = { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)} };
    skyParams.renderPass                  = _mainRenderPass->getRenderPass();
    skyParams.cullMode                    = VK_CULL_MODE_FRONT_BIT;
    skyParams.msaaSamples                 = _msaaSamples;
    skyParams.depthWrite                  = false;
    skyParams.depthCompareOp              = VK_COMPARE_OP_LESS_OR_EQUAL;
    _skyboxPipeline = std::make_shared<Pipeline>(_ctx,
        AssetPath::getInstance()->get("spv/skybox/skybox_vert.spv"),
        AssetPath::getInstance()->get("spv/skybox/skybox_frag.spv"),
        skyParams);
}


void BoidScene::connectPipelines()
{
    _skyboxModel->setPipeline(_skyboxPipeline);
    _fishModel->setPipeline(_goldfishPipeline);
    _sharkModel->setPipeline(_sharkPipeline);
    _sceneModels.push_back(_sharkModel);
}


void BoidScene::updatePredator(float dt)
{
    glm::vec3 target = _boidSim->getNearestBoidPosition(_predator.position);

    glm::vec3 desired = glm::normalize(target - _predator.position) * _predator.chaseSpeed;
    glm::vec3 steer = desired - _predator.velocity;

    // Clamp steer force to limit turning radius — shark can't snap to a new heading instantly
    float steerLen = glm::length(steer);
    if (steerLen > PREDATOR_MAX_STEER)
        steer = steer / steerLen * PREDATOR_MAX_STEER;

    _predator.velocity += steer * dt;
    float speed = glm::length(_predator.velocity);
    if (speed > _predator.chaseSpeed)
        _predator.velocity = _predator.velocity / speed * _predator.chaseSpeed;
    _predator.position += _predator.velocity * dt;

    _boidSim->setPredator(_predator);

    // Build TBN transform for shark rendering.
    // Derive B (right) from world +Y so N (up) always stays near +Y.
    // When heading is nearly vertical, cross(Y, T) degenerates — fall back to
    // the previous B and update it smoothly to avoid a threshold snap.
    glm::vec3 T = (speed > 1e-4f) ? _predator.velocity / speed : glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 B_raw = glm::cross(glm::vec3(0.f, 1.f, 0.f), T);
    float Blen = glm::length(B_raw);
    if (Blen < 1e-4f)
        B_raw = _sharkRight;  // near-vertical heading — reuse last known right
    glm::vec3 B = glm::normalize(B_raw);
    _sharkRight = B;          // keep updated for the next degenerate frame
    glm::vec3 N = glm::cross(T, B);

    glm::mat4 mat = glm::mat4(
        glm::vec4(B, 0.f),
        glm::vec4(N, 0.f),
        glm::vec4(T, 0.f),
        glm::vec4(_predator.position, 1.f)
    );
    _sharkModel->setTransform(mat);
}


void BoidScene::advance()
{
    auto now = std::chrono::high_resolution_clock::now();
    _dt = std::chrono::duration<float>(now - _lastFrameTime).count();
    _lastFrameTime = now;

    if (_paused) _dt = 0.f;

    _sceneInfo.time += _dt;
    _camera->advanceAnimation(_dt);
    updatePredator(_dt * _timeScale);
}


void BoidScene::dispatchCompute(VkCommandBuffer cmd)
{
    if (_paused) return;

    _boidSim->dispatch(cmd, _dt * _timeScale);
    VulkanHelper::barrierComputeToVertex(cmd, _boidSim->getTransformsBuffer());
}


void BoidScene::resetBoids()
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-SPAWN_RANGE, SPAWN_RANGE);
    std::uniform_real_distribution<float> unitDist(-1.0f, 1.0f);

    std::vector<BoidState> states;
    states.reserve(_nBoids);

    while ((int)states.size() < _nBoids) {
        float x = unitDist(rng), y = unitDist(rng), z = unitDist(rng);
        float len = std::sqrt(x*x + y*y + z*z);
        if (len < 1e-4f || len > 1.0f) continue;

        BoidState b{};
        float initSpeed = _boidSim ? _boidSim->minSpeed : 8.f;
        b.position = glm::vec4(posDist(rng), posDist(rng), posDist(rng), 0.f);
        b.velocity = glm::vec4(glm::vec3(x, y, z) / len * initSpeed, 0.f);
        states.push_back(b);
    }

    if (_boidSim) {
        _boidSim->resetBoids(states);
    } else {
        _boidSim = std::make_unique<BoidSimulation>(_ctx, _nBoids, states, SPAWN_RANGE);
        _fishModel->setTransformBuffer(_boidSim->getTransformsBuffer(), _nBoids);
    }
}


void BoidScene::buildUI()
{
    ImGui::Begin("Boids");
    ImGui::Text(ICON_FA_GAUGE " %.1f FPS  (%.2f ms)",
        ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

    ImGui::Separator();
    ImGui::Text(ICON_FA_FLASK " Simulation");
    ImGui::Indent(16.0f);
    if (ImGui::Button(_paused ? ICON_FA_PLAY " Resume" : ICON_FA_PAUSE " Pause"))
        _paused = !_paused;
    ImGui::SameLine();
    if (ImGui::Button("Restart")) resetBoids();
    ImGui::SliderFloat("Time Scale", &_timeScale, 0.1f, 20.f);
    ImGui::Unindent(16.0f);

    ImGui::Separator();
    ImGui::Text(ICON_FA_FISH " Boids");
    ImGui::Indent(16.0f);
    if (ImGui::SliderInt("Number of Boids", &_nBoids, 200, 2000)) {
        _boidSim.reset();
        resetBoids();
        // Rebuild pipeline since transform DS layout is recreated
        createGoldfishPipeline();
        connectPipelines();
    }
    float angleDeg = glm::degrees(_boidSim->perceptionAngle);
    if (ImGui::SliderFloat("Perception Angle", &angleDeg, 0.f, 180.f))
        _boidSim->perceptionAngle = glm::radians(angleDeg);
    ImGui::SliderFloat("Min Speed", &_boidSim->minSpeed, 0.f, _boidSim->maxSpeed);
    ImGui::SliderFloat("Max Speed", &_boidSim->maxSpeed, _boidSim->minSpeed, 50.f);
    ImGui::Unindent(16.0f);

    ImGui::Separator();
    ImGui::Text(ICON_FA_CUBE " Obstacles");
    ImGui::Indent(16.0f);
    if (ImGui::Checkbox("Show Obstacles", &_showObstacles)) {
        if (_showObstacles)
            for (auto& m : _obstacleModels) _sceneModels.push_back(m);
        else
            for (auto& m : _obstacleModels)
                _sceneModels.erase(std::remove(_sceneModels.begin(), _sceneModels.end(), m), _sceneModels.end());
    }
    ImGui::Unindent(16.0f);

    ImGui::Separator();
    ImGui::Text(ICON_FA_ARROWS_LEFT_RIGHT_TO_LINE " Separation");
    ImGui::Indent(16.0f);
    ImGui::SliderFloat("Radius##rs", &_boidSim->rs, 1.f, 20.f);
    ImGui::SliderFloat("Weight##ws", &_boidSim->ws, 0.f,  5.f);
    ImGui::Unindent(16.0f);

    ImGui::Separator();
    ImGui::Text(ICON_FA_COMPASS " Alignment");
    ImGui::Indent(16.0f);
    ImGui::SliderFloat("Radius##ra", &_boidSim->ra, 1.f, 20.f);
    ImGui::SliderFloat("Weight##wa", &_boidSim->wa, 0.f,  5.f);
    ImGui::Unindent(16.0f);

    ImGui::Separator();
    ImGui::Text(ICON_FA_ARROWS_TO_CIRCLE " Cohesion");
    ImGui::Indent(16.0f);
    ImGui::SliderFloat("Radius##rc", &_boidSim->rc, 1.f, 20.f);
    ImGui::SliderFloat("Weight##wc", &_boidSim->wc, 0.f,  5.f);
    ImGui::Unindent(16.0f);

    ImGui::End();
}


void BoidScene::createCamera()
{
    TurnTableCameraParams cam{};
    cam.target           = glm::vec3(0.f);
    cam.initialRadius    = 60.f;
    cam.initialAzimuth   = 0.f;
    cam.initialElevation = glm::radians(-60.f);
    _camera = std::make_unique<TurnTableCamera>(cam);
}
