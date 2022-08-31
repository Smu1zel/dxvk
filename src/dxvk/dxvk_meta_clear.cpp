#include "dxvk_meta_clear.h"
#include "dxvk_device.h"

#include <dxvk_clear_buffer_f.h>
#include <dxvk_clear_buffer_u.h>
#include <dxvk_clear_image1d_f.h>
#include <dxvk_clear_image1d_u.h>
#include <dxvk_clear_image1darr_f.h>
#include <dxvk_clear_image1darr_u.h>
#include <dxvk_clear_image2d_f.h>
#include <dxvk_clear_image2d_u.h>
#include <dxvk_clear_image2darr_f.h>
#include <dxvk_clear_image2darr_u.h>
#include <dxvk_clear_image3d_f.h>
#include <dxvk_clear_image3d_u.h>

namespace dxvk {
  
  DxvkMetaClearObjects::DxvkMetaClearObjects(DxvkDevice* device)
  : m_device(device) {
    // Create descriptor set layouts
    m_clearBufDsetLayout = createDescriptorSetLayout(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
    m_clearImgDsetLayout = createDescriptorSetLayout(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    
    // Create pipeline layouts using those descriptor set layouts
    m_clearBufPipeLayout = createPipelineLayout(m_clearBufDsetLayout);
    m_clearImgPipeLayout = createPipelineLayout(m_clearImgDsetLayout);
    
    // Create the actual compute pipelines
    m_clearPipesF32.clearBuf = createPipeline(dxvk_clear_buffer_f, m_clearBufPipeLayout);
    m_clearPipesU32.clearBuf = createPipeline(dxvk_clear_buffer_u, m_clearBufPipeLayout);
    
    m_clearPipesF32.clearImg1D = createPipeline(dxvk_clear_image1d_f, m_clearImgPipeLayout);
    m_clearPipesU32.clearImg1D = createPipeline(dxvk_clear_image1d_u, m_clearImgPipeLayout);
    m_clearPipesF32.clearImg2D = createPipeline(dxvk_clear_image2d_f, m_clearImgPipeLayout);
    m_clearPipesU32.clearImg2D = createPipeline(dxvk_clear_image2d_u, m_clearImgPipeLayout);
    m_clearPipesF32.clearImg3D = createPipeline(dxvk_clear_image3d_f, m_clearImgPipeLayout);
    m_clearPipesU32.clearImg3D = createPipeline(dxvk_clear_image3d_u, m_clearImgPipeLayout);
    
    m_clearPipesF32.clearImg1DArray = createPipeline(dxvk_clear_image1darr_f, m_clearImgPipeLayout);
    m_clearPipesU32.clearImg1DArray = createPipeline(dxvk_clear_image1darr_u, m_clearImgPipeLayout);
    m_clearPipesF32.clearImg2DArray = createPipeline(dxvk_clear_image2darr_f, m_clearImgPipeLayout);
    m_clearPipesU32.clearImg2DArray = createPipeline(dxvk_clear_image2darr_u, m_clearImgPipeLayout);
  }
  
  
  DxvkMetaClearObjects::~DxvkMetaClearObjects() {
    auto vk = m_device->vkd();

    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearBuf, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearBuf, nullptr);
    
    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearImg1D, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearImg1D, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearImg2D, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearImg2D, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearImg3D, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearImg3D, nullptr);
    
    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearImg1DArray, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearImg1DArray, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesF32.clearImg2DArray, nullptr);
    vk->vkDestroyPipeline(vk->device(), m_clearPipesU32.clearImg2DArray, nullptr);
    
    // Destroy pipeline layouts
    vk->vkDestroyPipelineLayout(vk->device(), m_clearBufPipeLayout, nullptr);
    vk->vkDestroyPipelineLayout(vk->device(), m_clearImgPipeLayout, nullptr);
    
    // Destroy descriptor set layouts
    vk->vkDestroyDescriptorSetLayout(vk->device(), m_clearBufDsetLayout, nullptr);
    vk->vkDestroyDescriptorSetLayout(vk->device(), m_clearImgDsetLayout, nullptr);
  }
  
  
  DxvkMetaClearPipeline DxvkMetaClearObjects::getClearBufferPipeline(
          DxvkFormatFlags       formatFlags) const {
    DxvkMetaClearPipeline result;
    result.dsetLayout = m_clearBufDsetLayout;
    result.pipeLayout = m_clearBufPipeLayout;
    result.pipeline   = m_clearPipesF32.clearBuf;
    
    if (formatFlags.any(DxvkFormatFlag::SampledUInt, DxvkFormatFlag::SampledSInt))
      result.pipeline = m_clearPipesU32.clearBuf;
    
    result.workgroupSize = VkExtent3D { 128, 1, 1 };
    return result;
  }
  
  
  DxvkMetaClearPipeline DxvkMetaClearObjects::getClearImagePipeline(
          VkImageViewType       viewType,
          DxvkFormatFlags       formatFlags) const {
    const DxvkMetaClearPipelines& pipes = formatFlags.any(
      DxvkFormatFlag::SampledUInt, DxvkFormatFlag::SampledSInt)
        ? m_clearPipesU32 : m_clearPipesF32;
    
    DxvkMetaClearPipeline result;
    result.dsetLayout = m_clearImgDsetLayout;
    result.pipeLayout = m_clearImgPipeLayout;
    
    auto pipeInfo = [&pipes, viewType] () -> std::pair<VkPipeline, VkExtent3D> {
      switch (viewType) {
        case VK_IMAGE_VIEW_TYPE_1D:       return { pipes.clearImg1D,      VkExtent3D { 64, 1, 1 } };
        case VK_IMAGE_VIEW_TYPE_2D:       return { pipes.clearImg2D,      VkExtent3D {  8, 8, 1 } };
        case VK_IMAGE_VIEW_TYPE_3D:       return { pipes.clearImg3D,      VkExtent3D {  4, 4, 4 } };
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY: return { pipes.clearImg1DArray, VkExtent3D { 64, 1, 1 } };
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY: return { pipes.clearImg2DArray, VkExtent3D {  8, 8, 1 } };
        default:                          return { VkPipeline(VK_NULL_HANDLE), VkExtent3D { 0, 0, 0, } };
      }
    }();
    
    result.pipeline      = pipeInfo.first;
    result.workgroupSize = pipeInfo.second;
    return result;
  }
  
  
  VkDescriptorSetLayout DxvkMetaClearObjects::createDescriptorSetLayout(
          VkDescriptorType        descriptorType) {
    auto vk = m_device->vkd();

    VkDescriptorSetLayoutBinding bindInfo = { 0, descriptorType, 1, VK_SHADER_STAGE_COMPUTE_BIT };
    
    VkDescriptorSetLayoutCreateInfo dsetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    dsetInfo.bindingCount       = 1;
    dsetInfo.pBindings          = &bindInfo;
    
    VkDescriptorSetLayout result = VK_NULL_HANDLE;
    if (vk->vkCreateDescriptorSetLayout(vk->device(),
          &dsetInfo, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("Dxvk: Failed to create meta clear descriptor set layout");
    return result;
  }
  
  
  VkPipelineLayout DxvkMetaClearObjects::createPipelineLayout(
          VkDescriptorSetLayout   dsetLayout) {
    auto vk = m_device->vkd();

    VkPushConstantRange pushInfo = { VK_SHADER_STAGE_COMPUTE_BIT, 0, uint32_t(sizeof(DxvkMetaClearArgs)) };
    
    VkPipelineLayoutCreateInfo pipeInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeInfo.setLayoutCount         = 1;
    pipeInfo.pSetLayouts            = &dsetLayout;
    pipeInfo.pushConstantRangeCount = 1;
    pipeInfo.pPushConstantRanges    = &pushInfo;
    
    VkPipelineLayout result = VK_NULL_HANDLE;
    if (vk->vkCreatePipelineLayout(vk->device(),
          &pipeInfo, nullptr, &result) != VK_SUCCESS)
      throw DxvkError("Dxvk: Failed to create meta clear pipeline layout");
    return result;
  }
  
  
  VkPipeline DxvkMetaClearObjects::createPipeline(
    const SpirvCodeBuffer&        spirvCode,
          VkPipelineLayout        pipeLayout) {
    auto vk = m_device->vkd();

    VkShaderModuleCreateInfo shaderInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    shaderInfo.codeSize           = spirvCode.size();
    shaderInfo.pCode              = spirvCode.data();
    
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vk->vkCreateShaderModule(vk->device(),
          &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS)
      throw DxvkError("Dxvk: Failed to create meta clear shader module");
    
    VkPipelineShaderStageCreateInfo stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageInfo.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module              = shaderModule;
    stageInfo.pName               = "main";
    stageInfo.pSpecializationInfo = nullptr;
    
    VkComputePipelineCreateInfo pipeInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipeInfo.stage                = stageInfo;
    pipeInfo.layout               = pipeLayout;
    pipeInfo.basePipelineIndex    = -1;
    
    VkPipeline result = VK_NULL_HANDLE;
    
    const VkResult status = vk->vkCreateComputePipelines(
      vk->device(), VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &result);
    
    vk->vkDestroyShaderModule(vk->device(), shaderModule, nullptr);
    
    if (status != VK_SUCCESS)
      throw DxvkError("Dxvk: Failed to create meta clear compute pipeline");

    m_device->setDebugObjectName(result, "clear_pipeline");
    return result;
  }
  
}
