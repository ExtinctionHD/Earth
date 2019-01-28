#include <cassert>

#include "FinalRenderPass.h"

// public:

FinalRenderPass::FinalRenderPass(Device *device, VkExtent2D attachmentExtent)
    : RenderPass(device, attachmentExtent, device->getSampleCount())
{
}

void FinalRenderPass::saveRenderPasses(GeometryRenderPass *geometryRenderPass, LightingRenderPass *lightingRenderPass)
{
	this->geometryRenderPass = geometryRenderPass;
	this->lightingRenderPass = lightingRenderPass;
}

// protected:

void FinalRenderPass::createAttachments()
{
	colorImage = lightingRenderPass->getColorTexture();
    depthImage = geometryRenderPass->getDepthImage();

	attachments = { colorImage, depthImage };
}

void FinalRenderPass::createRenderPass()
{
	// description of attachments

    const VkAttachmentDescription colorAttachmentDesc{
		0,								
		colorImage->getFormat(),		                
		colorImage->getSampleCount(),			 
		VK_ATTACHMENT_LOAD_OP_LOAD,		         
		VK_ATTACHMENT_STORE_OP_STORE,		     
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,	     
		VK_ATTACHMENT_STORE_OP_DONT_CARE,	     
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

    const VkAttachmentDescription depthAttachmentDesc{
		0,										
		depthImage->getFormat(),						
		depthImage->getSampleCount(),					
		VK_ATTACHMENT_LOAD_OP_LOAD,						 
		VK_ATTACHMENT_STORE_OP_DONT_CARE,				
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,				
		VK_ATTACHMENT_STORE_OP_DONT_CARE,				
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	std::vector<VkAttachmentDescription> attachmentDescriptions{
		colorAttachmentDesc,
		depthAttachmentDesc
	};

	// references to attachments

	VkAttachmentReference colorAttachmentRef{
		0,							
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef{
		1,									
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	// subpass and it dependencies (contain references)

	VkSubpassDescription subpass{
		0,						
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,								
		nullptr,						
		1,								
		&colorAttachmentRef,			
		nullptr,			
		&depthAttachmentRef,			
		0,								
		nullptr							
	};

    const VkSubpassDependency inputDependency{
		VK_SUBPASS_EXTERNAL,							
		0,												
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,			
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	
		VK_ACCESS_MEMORY_READ_BIT,						
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | 
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	        
		VK_DEPENDENCY_BY_REGION_BIT,                    
	};

    const VkSubpassDependency outputDependency{
		0,									
		VK_SUBPASS_EXTERNAL,							
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,			
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | 
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,	        
		VK_ACCESS_MEMORY_READ_BIT,						
		VK_DEPENDENCY_BY_REGION_BIT,					
	};

	std::vector<VkSubpassDependency> dependencies{
		inputDependency,
		outputDependency
	};

	// render pass (contain descriptions)

	VkRenderPassCreateInfo createInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	
		nullptr,									
		0,											
		uint32_t(attachmentDescriptions.size()),
		attachmentDescriptions.data(),				
		1,											
		&subpass,									
		uint32_t(dependencies.size()),						
		dependencies.data(),						
	};

    const VkResult result = vkCreateRenderPass(device->get(), &createInfo, nullptr, &renderPass);
	assert(result == VK_SUCCESS);
}

void FinalRenderPass::createFramebuffers()
{
	addFramebuffer({ colorImage->getView(), depthImage->getView() });
}

// private:
