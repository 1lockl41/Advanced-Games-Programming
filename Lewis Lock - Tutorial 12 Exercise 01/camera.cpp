#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <stdio.h>
#include "camera.h"

#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>

Camera::Camera(float x, float y, float z, float camera_rotation)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_camera_rotation = camera_rotation;

	m_dx = sin(camera_rotation);
	m_dz = cos(camera_rotation);
}

void Camera::Rotate(float degrees)
{
	m_camera_rotation += degrees;

	m_dx = sin(m_camera_rotation * (XM_PI / 180));
	m_dz = cos(m_camera_rotation * (XM_PI / 180));
}

void Camera::Forward(float distance)
{
	m_x += distance * m_dx;
	m_y += distance * m_dz;
}

XMMATRIX Camera::GetViewMatrix()
{
	m_position = XMVectorSet(m_x, m_y, m_z, 0.0);
	m_lookat = XMVectorSet(m_x + m_dx, m_y, m_z + m_dz, 0.0);
	m_up = XMVectorSet(0.0, 1.0, 0.0, 0.0);

	return XMMatrixLookAtLH(m_position, m_lookat, m_up);
}

float Camera::GetX()
{
	return m_x;
}

float Camera::GetY()
{
	return m_y;
}

float Camera::GetZ()
{
	return m_z;
}
