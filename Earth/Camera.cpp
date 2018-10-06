#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include "Camera.h"

// public:

Camera::Camera(VkExtent2D extent)
{
	pos = glm::vec3(0.0f, 0.0f, 0.0f);
	forward = glm::vec3(0.0f, 0.0f, 1.0f);
	up = glm::vec3(0.0f, -1.0f, 0.0f);
	this->extent = extent;

	init();
}

Camera::Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up, VkExtent2D extent)
{
	this->pos = pos;
	this->forward = glm::normalize(target);
	this->up = glm::normalize(up);
	this->extent = extent;

	init();
}

Camera::~Camera()
{

}

glm::vec3 Camera::getPos() const
{
	return pos;
}

glm::vec3 Camera::getTarget() const
{
	return forward + pos;
}

glm::vec3 Camera::getUp() const
{
	return up;
}

void Camera::moveCamera(float deltaSec)
{
	const float DISTANCE = SPEED * deltaSec;

	glm::vec3 direction = forward * (float)movement.forward;

	glm::vec3 right = glm::cross(forward, up);
	right = glm::normalize(right);
	direction += right * (float)movement.right;

	direction += up * (float)movement.up;

	if (glm::length(direction) != 0.0f)
	{
		direction = glm::normalize(direction);
	}
	pos += direction * DISTANCE;
}

void Camera::rotateCamera(glm::vec2 pos)
{
	const float MAX_DELTA = 100.0f;
	const float VERT_ANGLE_LIMIT = 90.0f;

	float deltaX = pos.x - getCenter().x;
	deltaX = abs(deltaX) < MAX_DELTA ? deltaX : MAX_DELTA * deltaX / abs(deltaX);
	angleH += (deltaX * SENSITIVITY);

	float deltaY = pos.y - getCenter().y;
	deltaY = abs(deltaY) < MAX_DELTA ? deltaY : MAX_DELTA * deltaY / abs(deltaY);
	angleV += (deltaY * SENSITIVITY);
	// set vertical angle limits: -VERT_ANGLE_LIMIT and VERT_ANGLE_LIMIT degrees
	angleV = abs(angleV) > VERT_ANGLE_LIMIT ? VERT_ANGLE_LIMIT * angleV / abs(angleV) : angleV;

	const glm::vec3 vAxis{ 0.0f, 1.0f, 0.0f };

	// rotate the view by the horizontal angle
	glm::vec3 view(0.0f, 0.0f, 1.0f);
	view = glm::rotate(view, glm::radians(angleH), vAxis);
	view = glm::normalize(view);

	// rotate the view by the vertical angle
	glm::vec3 hAxis = glm::cross(view, vAxis);
	hAxis = glm::normalize(hAxis);
	view = glm::rotate(view, glm::radians(angleV), hAxis);

	// save changes
	forward = glm::normalize(view);
	up = glm::cross(forward, hAxis);
	up = glm::normalize(up);
}

void Camera::setCameraExtent(VkExtent2D extent)
{
	this->extent = extent;
}

glm::mat4 Camera::getViewMatrix() const
{
	return glm::lookAt(getPos(), getTarget(), getUp());
}

glm::mat4 Camera::getProjectionMatrix() const
{
	const float viewAngle = 45.0f;
	const float aspect = extent.width / (float)extent.height;
	const float zNear = 0.1f;
	const float zFar = 500.0f;

	return glm::perspective(glm::radians(viewAngle), aspect, zNear, zFar);
}

// private:

void Camera::init()
{
	glm::vec3 horizontal{ forward.x, 0.0f, forward.z };
	horizontal = glm::normalize(horizontal);

	// horizontal camera angle
	if (horizontal.x >= 0.0f)
	{
		if (horizontal.z >= 0.0f)
		{
			// first quarter
			angleH = 360.0f - glm::degrees(glm::asin(horizontal.x));
		}
		else
		{
			// second quarter
			angleH = 180.0f + glm::degrees(glm::asin(horizontal.x));
		}
	}
	else
	{
		if (horizontal.z >= 0.0f)
		{
			// third quarter
			angleH = glm::degrees(glm::asin(-horizontal.x));
		}
		else
		{
			// fourth quarter
			angleH = 90.0f + glm::degrees(glm::asin(-horizontal.x));
		}
	}

	// vertical camera angle
	angleV = -glm::degrees(glm::asin(forward.y));
}

glm::vec2 Camera::getCenter() const
{
	return glm::vec2(extent.width / 2.0f, extent.height / 2.0f);
}