#pragma once

#include<memory>
#include<Windows.h>
#include<d3d11_4.h>
#include<DirectXMath.h>
#include<wrl/client.h>

#pragma warning(disable : 4005) 

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTK/DirectXTK.lib")
#pragma comment(lib, "DirectXTex/DirectXTex.lib")

namespace Prizm
{
	// default states
	enum RasterizerStateType
	{
		WIRE_FRAME = 0,
		CULL_NONE,
		CULL_FRONT,
		CULL_BACK,

		RASTERIZER_STATE_MAX
	};

	enum BlendStateType
	{
		DISABLED,
		ADDITIVE_COLOR,
		ALPHA_BLEND,
		SUBTRACT_BLEND,
		MULTIPLE_BLEND,
		ALIGNMENT_BLEND,

		BLEND_STATE_MAX
	};

	enum SamplerStateType
	{
		POINT_SAMPLER,
		LINEAR_FILTER_SAMPLER_WRAP_UVW,
		LINEAR_FILTER_SAMPLER,
		WRAP_SAMPLER,

		DEFAULT_SAMPLER_MAX
	};

	enum DepthStencilStateType
	{
		DEPTH_STENCIL_WRITE,
		DEPTH_STENCIL_DISABLED,
		DEPTH_WRITE,
		STENCIL_WRITE,
		DEPTH_TEST_ONLY,
		// add more

		DEPTH_STENCIL_STATE_MAX
	};

	enum RenderTargetType
	{
		BACK_BUFFER,
		SHADOW_MAP,
		RENDER_TARGET_MAX
	};

	namespace Graphics
	{
		bool Initialize(int width, int height, const bool vsync, HWND hwnd, const bool FULL_SCREEN);
		void Finalize(void);

		void BeginFrame(void);
		void EndFrame(void);

		bool ChangeWindowMode(void);

		void SetRenderTarget(RenderTargetType);
		void ClearRenderTargetView(RenderTargetType, const float*);
		void ClearDepthStencilView(void);
		void GetViewPort(D3D11_VIEWPORT*);
		void SetViewPort(D3D11_VIEWPORT*);

		void SetBlendState(BlendStateType);
		void SetRasterizerState(RasterizerStateType);
		void SetDepthStencilState(DepthStencilStateType);
		Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSamplerState(SamplerStateType);
		void SetSamplerState(unsigned int, SamplerStateType, unsigned int);

		void SetPSTexture(UINT register_slot, UINT num_views, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv);

		Microsoft::WRL::ComPtr<ID3D11Device>&			GetDevice(void);
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>&	GetDeviceContext(void);
		HWND GetWindowHandle(void);
	}
}

/*
lua
network
bullet3
effekseer
self controller


cocos2dx
*/