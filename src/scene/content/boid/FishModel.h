#pragma once

#include "stdafx.h"
#include "scene/model/Model.h"
#include "vulkan/DescriptorSet.h"
#include "vulkan/resources/Texture2D.h"

// Instanced goldfish model — binds a transform SSBO at set=1 and a texture at set=2.
class FishModel : public Model
{
public:
    FishModel(std::shared_ptr<VulkanContext> ctx,
              const std::string& meshPath,
              const std::string& texturePath,
              float scale);

    void draw(VkCommandBuffer cmd, const Renderer& renderer) override;

    void setTransformBuffer(VkBuffer buf, uint32_t instanceCount);

    const DescriptorSet* getTransformDescriptorSet() const { return _transformDS.get(); }
    const DescriptorSet* getTextureDescriptorSet()   const { return _textureDS.get(); }

private:
    VkBuffer _transformBuffer = VK_NULL_HANDLE;
    uint32_t _instanceCount   = 0;

    std::unique_ptr<DescriptorSet> _transformDS;
    std::unique_ptr<Texture2D>     _texture;
    std::unique_ptr<DescriptorSet> _textureDS;
};
