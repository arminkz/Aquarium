#include "PlaneModel.h"
#include "geometry/MeshFactory.h"
#include "geometry/DeviceMesh.h"
#include "core/SinglePassRenderer.h"

PlaneModel::PlaneModel(std::shared_ptr<VulkanContext> ctx,
                       glm::vec3 point, glm::vec3 normal, float sx, float sy)
    : Model(ctx, "Plane",
            std::make_shared<DeviceMesh>(ctx, MeshFactory::createQuadMesh(1.f, 1.f, false)))
{
    setTransform(point, normal, sx, sy);
}

void PlaneModel::setTransform(glm::vec3 point, glm::vec3 normal, float sx, float sy)
{
    // Quad is created in the XZ plane (normal = +Y). Rotate +Y to target normal.
    glm::vec3 up = glm::vec3(0.f, -1.f, 0.f);
    glm::vec3 n = glm::normalize(normal);
    float cosA = glm::dot(up, n);

    glm::mat4 rot;
    if (cosA > 0.9999f) {
        rot = glm::mat4(1.f);
    } else if (cosA < -0.9999f) {
        rot = glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(1.f, 0.f, 0.f));
    } else {
        glm::vec3 axis = glm::normalize(glm::cross(up, n));
        rot = glm::rotate(glm::mat4(1.f), glm::acos(cosA), axis);
    }

    _modelMatrix = glm::translate(glm::mat4(1.f), point)
                 * rot
                 * glm::scale(glm::mat4(1.f), glm::vec3(sx, 1.f, sy));
}

void PlaneModel::draw(VkCommandBuffer cmd, const Renderer& renderer)
{
    auto pipeline = _pipeline.lock();
    if (!pipeline) return;

    pipeline->bind(cmd);

    VkBuffer vb = _mesh->getVertexBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
    vkCmdBindIndexBuffer(cmd, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    const auto& spr = static_cast<const SinglePassRenderer&>(renderer);
    VkDescriptorSet sceneds = spr.getSceneDescriptorSet()->getDescriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->getPipelineLayout(), 0, 1, &sceneds, 0, nullptr);

    vkCmdPushConstants(cmd, pipeline->getPipelineLayout(),
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(cmd, _mesh->getIndicesCount(), 1, 0, 0, 0);
}
