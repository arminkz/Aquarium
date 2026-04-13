#pragma once

#include "stdafx.h"
#include "scene/model/Model.h"
#include "vulkan/DescriptorSet.h"
#include "vulkan/resources/Texture2D.h"
#include "vulkan/resources/Buffer.h"

// Single-instance shark model — same pipeline layout as FishModel (set=0 scene,
// set=1 transform SSBO with one mat4, set=2 texture) but transform is updated
// every frame from the CPU predator state.
class SharkModel : public Model
{
public:
    SharkModel(std::shared_ptr<VulkanContext> ctx,
               const std::string& meshPath,
               const std::string& texturePath,
               float scale);

    void setTransform(const glm::mat4& m);

    void draw(VkCommandBuffer cmd, const Renderer& renderer) override;

    const DescriptorSet* getTransformDescriptorSet() const { return _transformDS.get(); }
    const DescriptorSet* getTextureDescriptorSet()   const { return _textureDS.get(); }

private:
    std::unique_ptr<Buffer>        _transformBuffer;
    std::unique_ptr<DescriptorSet> _transformDS;
    std::unique_ptr<Texture2D>     _texture;
    std::unique_ptr<DescriptorSet> _textureDS;
};
