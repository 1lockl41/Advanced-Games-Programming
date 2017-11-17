#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <stdio.h>

#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>

class Camera
{
public:
	Camera(float x, float y, float z, float camera_rotation);
	void Rotate(float degrees);
	void Forward(float distance);
	XMMATRIX GetViewMatrix();
	float GetX();
	float GetY();
	float GetZ();

private:
	float m_x, m_y, m_z, m_dx, m_dz, m_camera_rotation;
	XMVECTOR m_position, m_lookat, m_up;


};