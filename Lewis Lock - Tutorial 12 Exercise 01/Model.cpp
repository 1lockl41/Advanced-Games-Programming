#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <stdio.h>
#include "Model.h"

struct MODEL_CONSTANT_BUFFER
{
	XMMATRIX WorldViewProjection; //64 bytes
	float RedAmount; //4 bytes 
	float scale; //4 bytes
	XMFLOAT2 packing_bytes; //2x4 bytes = 8 bytes
	XMVECTOR directional_light_vector; //16 bytes
	XMVECTOR directional_light_colour; //16 bytes
	XMVECTOR ambient_light_colour; //16 bytes
}; //128 bytes

Model::Model(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext)
{
	m_pD3DDevice = pD3DDevice;
	m_pImmediateContext = pImmediateContext;

	m_scale = 1.0f;
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 15.0f;
	m_xangle = 30.0f;
	m_zangle = 15.0f;
	m_yangle = 15.0f;
}

Model::~Model()
{
	if (m_pConstantBuffer) m_pConstantBuffer->Release();
	if (m_pInputLayout) m_pInputLayout->Release();
	if (m_pPShader) m_pPShader->Release();
	if (m_pVShader) m_pVShader->Release();
	if (m_pObject) delete m_pObject;
}

bool Model::LoadObjModel(char* filename)
{
	m_pObject = new ObjFileModel(filename, m_pD3DDevice, m_pImmediateContext);
	if (m_pObject->filename == "FILE NOT LOADED") return S_FALSE;
	
	HRESULT hr = S_OK;

	ID3DBlob *VS, *PS, *error;
	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelVS", "vs_4_0", 0, 0, 0, &VS, &error, 0);
	if (error != 0) //check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //dont fail if error is just a warning
		{
			return hr;
		};
	}

	hr = D3DX11CompileFromFile("model_shaders.hlsl", 0, 0, "ModelPS", "ps_4_0", 0, 0, 0, &PS, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))//dont fail if error is just a warning
		{
			return hr;
		};
	}

	//create shader objects
	hr = m_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &m_pVShader);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &m_pPShader);
	if (FAILED(hr))
	{
		return hr;
	}

	m_pImmediateContext->VSSetShader(m_pVShader, 0, 0);
	m_pImmediateContext->PSSetShader(m_pPShader, 0, 0);

	//Create and set the input layout object
	D3D11_INPUT_ELEMENT_DESC iedesc[]=
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0 }
	};

	hr = m_pD3DDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &m_pInputLayout);
	if (FAILED(hr))
	{
		return hr;
	}

	m_pImmediateContext->IASetInputLayout(m_pInputLayout);

	//Create constant buffer
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));
	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT; //can use UpdateSubresource() to update
	constant_buffer_desc.ByteWidth = 128; // MUST be a multiple of 16, calculate from CB struct
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; //Use as a constant buffer

	hr = m_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &m_pConstantBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	CalculateModelCentrePoint();
	CalculateBoundingSphereRadius();
}

void Model::Draw(XMMATRIX* view, XMMATRIX* projection, XMVECTOR directional_light_colour, XMVECTOR ambient_light_colour, XMVECTOR directional_light_shines_from)
{
	m_pImmediateContext->VSSetShader(m_pVShader, 0, 0);
	m_pImmediateContext->PSSetShader(m_pPShader, 0, 0);
	m_pImmediateContext->IASetInputLayout(m_pInputLayout);

	XMMATRIX world;
	world *= XMMatrixScaling(m_scale, m_scale, m_scale);
	world = XMMatrixRotationX(XMConvertToRadians(m_xangle));
	world *= XMMatrixRotationY(XMConvertToRadians(m_yangle));
	world *= XMMatrixRotationZ(XMConvertToRadians(m_zangle));
	world *= XMMatrixTranslation(m_x, m_y, m_z);

	XMMATRIX transpose;
	MODEL_CONSTANT_BUFFER model_cb_values;
	model_cb_values.RedAmount = 0.5f; //50% of vertex red value
	model_cb_values.scale = 1.0f;
	model_cb_values.directional_light_colour = directional_light_colour;
	model_cb_values.ambient_light_colour = ambient_light_colour;
	model_cb_values.directional_light_vector = XMVector3Transform(directional_light_shines_from, transpose);
	model_cb_values.directional_light_vector = XMVector3Normalize(model_cb_values.directional_light_vector);
	model_cb_values.WorldViewProjection = world*(*view)*(*projection);

	m_pImmediateContext->UpdateSubresource(m_pConstantBuffer, 0, 0, &model_cb_values, 0, 0);
	m_pImmediateContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	m_pObject->Draw();
}

void Model::SetXPos(float x)
{
	m_x = x;
}

void Model::SetYPos(float y)
{
	m_y = y;
}

void Model::SetZPos(float z)
{
	m_z = z;
}

float Model::GetXPos()
{
	return m_x;
}

float Model::GetYPos()
{
	return m_y;
}

float Model::GetZPos()
{
	return m_z;
}

void Model::LookAt_XZ(float x, float z)
{
	m_xangle = sin(x * (XM_PI / 180));
	m_zangle = cos(z * (XM_PI / 180));

	m_yangle = atan2(m_xangle, m_yangle) * (180.0 / XM_PI);
}

void Model::CalculateModelCentrePoint()
{
	XMVECTOR minBounds;
	XMVECTOR maxBounds;

	bool hasInitiatedBounds = false;

	for (int i = 0; i < m_pObject->numverts;i++)
	{
		if (!hasInitiatedBounds)
		{
			minBounds.x = m_pObject->vertices[i].Pos.x;
			minBounds.y = m_pObject->vertices[i].Pos.y;
			minBounds.z = m_pObject->vertices[i].Pos.z;
			maxBounds.x = m_pObject->vertices[i].Pos.x;
			maxBounds.y = m_pObject->vertices[i].Pos.y;
			maxBounds.z = m_pObject->vertices[i].Pos.z;
			hasInitiatedBounds = true;
		}

		if (m_pObject->vertices[i].Pos.x > maxBounds.x)
		{
			maxBounds.x = m_pObject->vertices[i].Pos.x;
		}
		if (m_pObject->vertices[i].Pos.x <  minBounds.x)
		{
			minBounds.x = m_pObject->vertices[i].Pos.x;
		}
		if (m_pObject->vertices[i].Pos.y > maxBounds.y)
		{
			maxBounds.y = m_pObject->vertices[i].Pos.y;
		}
		if (m_pObject->vertices[i].Pos.y < minBounds.y)
		{
			minBounds.y = m_pObject->vertices[i].Pos.y;
		}
		if (m_pObject->vertices[i].Pos.z > maxBounds.z)
		{
			maxBounds.z = m_pObject->vertices[i].Pos.z;
		}
		if (m_pObject->vertices[i].Pos.z < minBounds.z)
		{
			minBounds.z = m_pObject->vertices[i].Pos.z;
		}
	}

	XMVECTOR centreOfShape;
	centreOfShape = minBounds + (maxBounds - minBounds) / 2;
	m_bounding_sphere_centre_x = centreOfShape.x;
	m_bounding_sphere_centre_y = centreOfShape.y;
	m_bounding_sphere_centre_z = centreOfShape.z;
}

void Model::CalculateBoundingSphereRadius()
{
	float maxDistance = 0;
	for (int i = 0; i < m_pObject->numverts;i++)
	{
		float distance = sqrt(pow(m_pObject->vertices[i].Pos.x, m_bounding_sphere_centre_x) + pow(m_pObject->vertices[i].Pos.y, m_bounding_sphere_centre_y) + pow(m_pObject->vertices[i].Pos.z, m_bounding_sphere_centre_z));
		if (distance > maxDistance)
		{
			maxDistance = distance;
		}		
	}

	m_bounding_sphere_radius = maxDistance;
}

XMVECTOR Model::GetBoundingSphereWorldSpacePosition()
{
	XMMATRIX world;
	world *= XMMatrixScaling(m_scale, m_scale, m_scale);
	world = XMMatrixRotationX(XMConvertToRadians(m_xangle));
	world *= XMMatrixRotationY(XMConvertToRadians(m_yangle));
	world *= XMMatrixRotationZ(XMConvertToRadians(m_zangle));
	world *= XMMatrixTranslation(m_x, m_y, m_z);

	XMVECTOR offset;
	offset = XMVectorSet(m_bounding_sphere_centre_x, m_bounding_sphere_centre_y, m_bounding_sphere_centre_z, 0.0);

	offset = XMVector3Transform(offset, world);

	return offset;
}

float Model::GetBoundingSphereRadius()
{
	return m_bounding_sphere_radius * m_scale;
}

bool Model::CheckCollisions(Model* model)
{
	if (model == this)
	{
		return false;
	}

	model->GetBoundingSphereWorldSpacePosition();
	

}

