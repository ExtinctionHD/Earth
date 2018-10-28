#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "Lighting.h"

Lighting::Lighting(Device *pDevice, Attributes attributes, float spaceRadius)
{
	this->attributes = attributes;

	pAttributesBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(attributes));
	pAttributesBuffer->updateData(&attributes, sizeof(attributes), 0);

	projection = glm::ortho(-spaceRadius, spaceRadius, -spaceRadius, spaceRadius, -spaceRadius, spaceRadius);
	projection[1][1] *= -1;

	glm::mat4 space = getSpaceMatrix();
	pSpaceBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHADER_STAGE_VERTEX_BIT, sizeof(space));
	pSpaceBuffer->updateData(&space, sizeof(space), 0);
}

Lighting::~Lighting()
{
	delete pSpaceBuffer;
	delete pAttributesBuffer;
}

Buffer * Lighting::getAttributesBuffer() const
{
	return pAttributesBuffer;
}

Buffer* Lighting::getSpaceBuffer() const
{
	return pSpaceBuffer;
}

void Lighting::update(glm::vec3 cameraPos)
{
	attributes.cameraPos = cameraPos;
	pAttributesBuffer->updateData(&attributes.cameraPos, sizeof(attributes.cameraPos), offsetof(Attributes, cameraPos));

	glm::mat4 space = getSpaceMatrix();
	pSpaceBuffer->updateData(&space, sizeof(space), 0);
}

glm::mat4 Lighting::getSpaceMatrix() const
{
	glm::mat4 view = lookAt(attributes.cameraPos - normalize(attributes.direction), attributes.cameraPos, glm::vec3(0.0f, -1.0f, 0.0f));
	return projection * view;
}