#include "SharkModel.h"

#include "geometry/ObjLoader.h"
#include "geometry/DeviceMesh.h"
#include "core/SinglePassRenderer.h"

SharkModel::SharkModel(std::shared_ptr<VulkanContext> ctx,
                       const std::string& meshPath,
                       const std::string& texturePath,
                       float scale)
    : Model(ctx, "Shark", nullptr)
{
    auto hostMesh = ObjLoader::load(meshPath);
    for (auto& v : hostMesh.vertices) v.pos *= scale;
    _mesh = std::make_shared<DeviceMesh>(ctx, hostMesh);

    // 1-element host-visible SSBO for the model matrix, updated each frame
    _transformBuffer = std::make_unique<Buffer>(ctx, sizeof(glm::mat4),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDescriptorBufferInfo bufInfo = _transformBuffer->getDescriptorInfo();
    _transformDS = std::make_unique<DescriptorSet>(ctx, std::vector<Descriptor>{
        Descriptor(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                   VK_SHADER_STAGE_VERTEX_BIT, 1, bufInfo),
    });

    _texture = std::make_unique<Texture2D>(ctx, texturePath, VK_FORMAT_R8G8B8A8_SRGB);
    _textureDS = std::make_unique<DescriptorSet>(ctx, std::vector<Descriptor>{
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                   VK_SHADER_STAGE_FRAGMENT_BIT, 1,
                   _texture->getDescriptorInfo()),
    });
}

void SharkModel::setTransform(const glm::mat4& m)
{
    _transformBuffer->copyData(&m, sizeof(glm::mat4));
}

void SharkModel::draw(VkCommandBuffer cmd, const Renderer& renderer)
{
    auto pipeline = _pipeline.lock();
    if (!pipeline) return;

    pipeline->bind(cmd);

    VkBuffer vb = _mesh->getVertexBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
    vkCmdBindIndexBuffer(cmd, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    const auto& spr = static_cast<const SinglePassRenderer&>(renderer);

    VkDescriptorSet sets[3] = {
        spr.getSceneDescriptorSet()->getDescriptorSet(), // set=0: SceneInfo
        _transformDS->getDescriptorSet(),                // set=1: Transform SSBO
        _textureDS->getDescriptorSet(),                  // set=2: Texture
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->getPipelineLayout(), 0, 3, sets, 0, nullptr);

    vkCmdDrawIndexed(cmd, _mesh->getIndicesCount(), 1, 0, 0, 0);
}
