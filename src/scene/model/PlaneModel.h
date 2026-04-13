#pragma once
#include "stdafx.h"
#include "Model.h"

// Renders a flat rectangular quad.
// Use setTransform(point, normal, sx, sy) to position, orient and size it.
class PlaneModel : public Model
{
public:
    PlaneModel(std::shared_ptr<VulkanContext> ctx,
               glm::vec3 point, glm::vec3 normal, float sx, float sy);

    void setTransform(glm::vec3 point, glm::vec3 normal, float sx, float sy);

    void draw(VkCommandBuffer cmd, const Renderer& renderer) override;
};
