#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <stdio.h>
#include "objfilemodel.h"

class Model
{
private:
	ID3D11Device* m_pD3DDevice;
	ID3D11DeviceContext* m_pImmediateContext;

	ObjFileModel* m_pObject;
	ID3D11VertexShader* m_pVShader;
	ID3D11PixelShader* m_pPShader;
	ID3D11InputLayout* m_pInputLayout;
	ID3D11Buffer* m_pConstantBuffer;

	float m_x, m_y, m_z;
	float m_xangle, m_zangle, m_yangle;
	float m_scale;

	float m_bounding_sphere_centre_x, m_bounding_sphere_centre_y, m_bounding_sphere_centre_z, m_bounding_sphere_radius;

	void CalculateModelCentrePoint();
	void CalculateBoundingSphereRadius();
	XMVECTOR GetBoundingSphereWorldSpacePosition();
	float GetBoundingSphereRadius();
	bool CheckCollisions(Model* model);


public:
	Model(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext);
	bool LoadObjModel(char* filename);
	void Draw(XMMATRIX* view, XMMATRIX* projection, XMVECTOR directional_light_colour, XMVECTOR ambient_light_colour, XMVECTOR directional_light_shines_from);
	~Model();
	void SetXPos(float x);
	void SetYPos(float y);
	void SetZPos(float z);
	float GetXPos();
	float GetYPos();
	float GetZPos();
	void LookAt_XZ(float x, float z);

};