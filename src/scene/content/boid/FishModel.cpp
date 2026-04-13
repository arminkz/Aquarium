#include "FishModel.h"

#include "geometry/ObjLoader.h"
#include "geometry/DeviceMesh.h"
#include "core/SinglePassRenderer.h"

FishModel::FishModel(std::shared_ptr<VulkanContext> ctx,
                     const std::string& meshPath,
                     const std::string& texturePath,
                     float scale)
    : Model(ctx, "Fish", nullptr)
{
    auto hostMesh = ObjLoader::load(meshPath);
    for (auto& v : hostMesh.vertices) v.pos *= scale;
    _mesh = std::make_shared<DeviceMesh>(ctx, hostMesh);

    _texture = std::make_unique<Texture2D>(ctx, texturePath, VK_FORMAT_R8G8B8A8_SRGB);
    _textureDS = std::make_unique<DescriptorSet>(ctx, std::vector<Descriptor>{
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                   VK_SHADER_STAGE_FRAGMENT_BIT, 1,
                   _texture->getDescriptorInfo())
    });
}

void FishModel::setTransformBuffer(VkBuffer buf, uint32_t instanceCount)
{
    _transformBuffer = buf;
    _instanceCount   = instanceCount;

    VkDescriptorBufferInfo info{};
    info.buffer = buf;
    info.offset = 0;
    info.range  = VK_WHOLE_SIZE;

    _transformDS = std::make_unique<DescriptorSet>(_ctx, std::vector<Descriptor>{
        Descriptor(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                   VK_SHADER_STAGE_VERTEX_BIT, 1, info),
    });
}

void FishModel::draw(VkCommandBuffer cmd, const Renderer& renderer)
{
    if (_instanceCount == 0 || _transformBuffer == VK_NULL_HANDLE) return;

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
        _transformDS->getDescriptorSet(),                // set=1: Transforms SSBO
        _textureDS->getDescriptorSet(),                  // set=2: Fish texture
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->getPipelineLayout(), 0, 3, sets, 0, nullptr);

    vkCmdDrawIndexed(cmd, _mesh->getIndicesCount(), _instanceCount, 0, 0, 0);
}
