
#include<string>
#include<vector>
#include<DirectXTK/SimpleMath.h>

#include"Graphics.h"
#include"ConstantBuffer.h"
#include"GraphicsEnums.h"
#include"Buffer.h"
#include"Window.h"
#include"..\Utilities\ResourcePool.h"
#include"..\Utilities\Utils.h"
#include"..\Utilities\Log.h"

namespace Prizm
{
	namespace Graphics
	{
		Microsoft::WRL::ComPtr<ID3D11Device>                         _device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>                  _device_context;
		Microsoft::WRL::ComPtr<IDXGISwapChain>                       _swap_chain;

		std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>>  _render_targets;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>             _shadow_resource;

		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>               _depth_stencil_view;

		std::vector<Microsoft::WRL::ComPtr<ID3D11BlendState>>        _blend_states;
		std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>>	     _sampler_states;
		std::vector<Microsoft::WRL::ComPtr<ID3D11RasterizerState>>   _rasterizer_states;
		std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> _depth_stencil_states;

		D3D11_VIEWPORT                                               _view_port;

		Microsoft::WRL::ComPtr<IDXGIAdapter>                         _adapter;
		Microsoft::WRL::ComPtr<IDXGIFactory>                         _factory;
#ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D11Debug>	                         _debug;
		Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>            _annotation;
#endif
		std::vector<DXGI_MODE_DESC>	                                 _display_mode_list;

		bool              _vsync_enabled;
		int	              _vram;
		char              _gpu_description[128];
		HWND              _hwnd;
		D3D_FEATURE_LEVEL _feature_level;
		
		// MSAA sample desc
		DXGI_SAMPLE_DESC  _sample_desc;

		unsigned int _window_width, _window_height;
		unsigned int _numerator, _denominator;
		bool         _fullscreen;

		void MSAASampleCheck(void)
		{
			_sample_desc = {};
			for (int i = 1; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i <<= 1)
			{
				UINT Quality;
				if (succeeded(_device->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, i, &Quality)))
				{
					if (0 < Quality)
					{
						_sample_desc.Count = i;
						_sample_desc.Quality = Quality - 1;
					}
				}
			}

			Log::Info("MSAA sample check process done.");
		}

		bool CreateAdapter(void)
		{
			std::string err_msg = "";

			if (failed(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(_factory.GetAddressOf()))))
			{
				err_msg =
					"Do not create ""DXGIFactory"".";
			}
			else
			{
				// Use the factory to create an adapter for the primary graphics interface (video card).
				if (failed(_factory->EnumAdapters(0, _adapter.GetAddressOf())))
				{
					err_msg =
						"Do not create an adapter for the primary graphics interface.";
				}
			}

			if (!err_msg.empty())
			{
				Log::Error(err_msg + " (Graphics.cpp)");
				return false;
			}

			Log::Info("Adapter create process done.");

			return true;
		}

		bool GetDisplayMode(void)
		{
			std::vector<DXGI_MODE_DESC> desc;
			Microsoft::WRL::ComPtr<IDXGIOutput> adapter_output;
			unsigned num_modes;
			std::string err_msg = "";

			if (failed(CreateAdapter()))	return false;

			// Enumerate the primary adapter output (monitor).
			if (failed(_adapter->EnumOutputs(0, adapter_output.GetAddressOf())))
			{
				err_msg =
					"Do not output enumerate the primary adapter.";
			}
			else
			{
				// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
				if (failed(adapter_output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num_modes, nullptr)))
				{
					err_msg =
						"Do not get the number of modes the ""DXGI_FORMAT_R8G8B8A8_UNORM"".";
				}

				desc.resize(num_modes);

				if (failed(adapter_output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &num_modes, desc.data())))
				{
					err_msg =
						"Do not fit the number of modes the ""DXGI_FORMAT_R8G8B8A8_UNORM"" to desc.";
				}
			}

			if (!err_msg.empty())
			{
				Log::Error(err_msg + " (Graphics.cpp)");
				return false;
			}

			for (unsigned i = 0; i < num_modes; ++i)
			{
				_display_mode_list.emplace_back(desc[i]);

				if (_display_mode_list[i].Width == (unsigned int)_window_width)
				{
					if (_display_mode_list[i].Height == (unsigned int)_window_height)
					{
						_numerator = _display_mode_list[i].RefreshRate.Numerator;
						_denominator = _display_mode_list[i].RefreshRate.Denominator;
					}
				}
			}

			if (_numerator == 0 && _denominator == 0)
			{
				_numerator = _display_mode_list[num_modes / 2].RefreshRate.Numerator;
				_denominator = _display_mode_list[num_modes / 2].RefreshRate.Denominator;
				_window_width = _display_mode_list[num_modes / 2].Width;
				_window_height = _display_mode_list[num_modes / 2].Height;
			}

			// Get the adapter (video card) description.
			DXGI_ADAPTER_DESC adapter_desc;

			if (failed(_adapter->GetDesc(&adapter_desc)))
			{
				err_msg =
					"Failed function -> adapter->GetDesc()\nGet the video card description failed.";
				return false;
			}
			else
			{
				// Store the dedicated video card memory in megabytes.
				_vram = (int)(adapter_desc.DedicatedVideoMemory / 1024 / 1024);

				// Convert the name of the video card to a character array and store it.
				size_t stringLength;

				if (wcstombs_s(&stringLength, _gpu_description, 128, adapter_desc.Description, 128) != 0)
				{
					err_msg =
						"Failed function -> wcstombs_s()\nCannot convert the name of the video card to a character array and store it.";
					return false;
				}
			}

			if (!err_msg.empty())
			{
				MessageBoxA(nullptr, err_msg.c_str(), "Error Notification", MB_OK);
				return false;
			}

			//Release
			desc.clear();
			adapter_output.Reset();

			Log::Info("Displaymode get process done.");

			return true;
		}

		bool InitDeviceAndSwapChain(void)
		{// CreateDevice and CreateSwapChain separated because MSAA sample check.
			UINT debug_flag = 0;
#ifdef _DEBUG
			debug_flag = D3D11_CREATE_DEVICE_DEBUG;
#endif
			_feature_level = D3D_FEATURE_LEVEL_11_1;

			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				debug_flag,
				&_feature_level,
				1,
				D3D11_SDK_VERSION,
				_device.GetAddressOf(),
				nullptr,
				_device_context.GetAddressOf());

			Log::Info("Device create process done.");

			_sample_desc.Count = 1;
			_sample_desc.Quality = 0;

			// Forward MSAA is criticaly 
			//MSAASampleCheck();

			DXGI_SWAP_CHAIN_DESC swap_chain_desc;
			memset(&swap_chain_desc, 0, sizeof(swap_chain_desc));

			swap_chain_desc.BufferCount = 3;

			swap_chain_desc.BufferDesc.Width = window_width<int>;
			swap_chain_desc.BufferDesc.Height = window_height<int>;
			swap_chain_desc.BufferDesc.Format = static_cast<DXGI_FORMAT>(ImageFormat::RGBA8UN);

			if (_vsync_enabled)
			{	// Set the refresh rate of the back buffer.
				swap_chain_desc.BufferDesc.RefreshRate.Numerator = _numerator;
				swap_chain_desc.BufferDesc.RefreshRate.Denominator = _denominator;
			}
			else
			{
				swap_chain_desc.BufferDesc.RefreshRate.Numerator = 0;
				swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
			}

			swap_chain_desc.OutputWindow = _hwnd;
			swap_chain_desc.Windowed = !_fullscreen;
			swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
			swap_chain_desc.SampleDesc = _sample_desc;
			swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swap_chain_desc.Flags = 0;

			// Create the swap chain.
			HRESULT result = _factory->CreateSwapChain(
				_device.Get(),
				&swap_chain_desc,
				_swap_chain.GetAddressOf());

			Log::Info("Swapchain create process done.");

			if (failed(result))
			{
				Log::Error("Failed CreateDeviceAndSwapChain. (Graphics.cpp)");
				return false;
			}

#ifdef _DEBUG
			// Direct3D SDK Debug Layer
			//------------------------------------------------------------------------------------------
			// src1: https://blogs.msdn.microsoft.com/chuckw/2012/11/30/direct3d-sdk-debug-layer-tricks/
			// src2: http://seanmiddleditch.com/direct3d-11-debug-api-tricks/
			//------------------------------------------------------------------------------------------
			_device->QueryInterface<ID3D11Debug>(&_debug);
			if (_debug)
			{
				Log::Info("Succeed QueryInterface for ID3D11Debug.");

				Microsoft::WRL::ComPtr<ID3D11InfoQueue> info_queue;
				_debug->QueryInterface<ID3D11InfoQueue>(&info_queue);

				if (info_queue)
				{
					Log::Info("Succeed QueryInterface for ID3D11InfoQueue.");

					info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
					info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);

					D3D11_MESSAGE_ID hide[] =
					{
						D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
						// Add more message IDs here as needed
					};

					D3D11_INFO_QUEUE_FILTER filter;
					memset(&filter, 0, sizeof(filter));
					filter.DenyList.NumIDs = _countof(hide);
					filter.DenyList.pIDList = hide;
					info_queue->AddStorageFilterEntries(&filter);
					info_queue->Release();
				}
			}

			if (failed(_device_context->QueryInterface<ID3DUserDefinedAnnotation>(&_annotation)))
			{
				Log::Error("Can't QueryInterface(ID3DUserDefinedAnnotation). (Graphics.cpp)");
				return false;
			}

			Log::Info("Debug layer create process done.");
#endif
			return true;
		}

		bool CreateDefaultRenderTargetView(void)
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_2d;

			_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), static_cast<void**>(&tex_2d));

			_device->CreateRenderTargetView(tex_2d.Get(), nullptr, _render_targets[RenderTargetType::BACK_BUFFER].GetAddressOf());

			Log::Info("Backbuffer render target create process done.");

			return true;
		}

		bool CreateShadowRenderTargetView(void)
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_2d;

			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = window_width<unsigned int>;
			desc.Height = window_height<unsigned int>;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			//_device->CreateTexture2D(&desc, nullptr, texture_2d.GetAddressOf());

			desc.Format = static_cast<DXGI_FORMAT>(ImageFormat::R32F);
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			_device->CreateTexture2D(&desc, nullptr, texture_2d.GetAddressOf());

			D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
			RTVDesc.Format = desc.Format;
			RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			RTVDesc.Texture2D.MipSlice = 0;
			_device->CreateRenderTargetView(texture_2d.Get(), &RTVDesc, _render_targets[RenderTargetType::SHADOW_MAP].GetAddressOf());

			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
			SRVDesc.Format = desc.Format;
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = 1;
			_device->CreateShaderResourceView(texture_2d.Get(), &SRVDesc, _shadow_resource.GetAddressOf());

			Log::Info("Shadowmap render target create process done.");

			return true;
		}

		bool CreateRenderTargetView(void)
		{
			for (unsigned int i = 0; i < RenderTargetType::RENDER_TARGET_MAX; ++i)
			{
				_render_targets.emplace_back();
			}

			CreateDefaultRenderTargetView();

			CreateShadowRenderTargetView();

			return true;
		}

		bool CreateDepthStencilView(void)
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> depth;
			D3D11_TEXTURE2D_DESC depth_desc;
			memset(&depth_desc, 0, sizeof(depth_desc));
			depth_desc.Width = window_width<int>;
			depth_desc.Height = window_height<int>;
			depth_desc.ArraySize = 1;
			depth_desc.MipLevels = 1;
			depth_desc.Format = static_cast<DXGI_FORMAT>(ImageFormat::D24UNORM_S8U);
			depth_desc.Usage = static_cast<D3D11_USAGE>(BufferUsage::STATIC_RW);
			depth_desc.SampleDesc = _sample_desc;
			depth_desc.BindFlags = static_cast<D3D11_BIND_FLAG>(TextureUsage::DEPTH_TARGET);
			depth_desc.CPUAccessFlags = 0;
			depth_desc.MiscFlags = 0;

			_device->CreateTexture2D(&depth_desc, nullptr, depth.GetAddressOf());

			// depth stencil view and shader resource view for the shadow map (^ BindFlags)
			D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
			memset(&dsv_desc, 0, sizeof(dsv_desc));
			dsv_desc.Format = depth_desc.Format;
			dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			dsv_desc.Texture2D.MipSlice = 0;

			if (failed(_device->CreateDepthStencilView(depth.Get(), &dsv_desc, _depth_stencil_view.GetAddressOf())))
			{
				Log::Error("Cannot add depth Stencil Target View. (Graphics.cpp)");
				return false;
			}

			Log::Info("Depth stencil view create process done.");

			return true;
		}

		bool CreateViewPort(void)
		{
			// viewport set to immediate device context
			_view_port.Width = window_width<float>;
			_view_port.Height = window_height<float>;
			_view_port.MinDepth = 0.0f;
			_view_port.MaxDepth = 1.0f;
			_view_port.TopLeftX = 0;
			_view_port.TopLeftY = 0;
			_device_context->RSSetViewports(1, &_view_port);

			Log::Info("Backbuffer viewport create process done.");

			return true;
		}

		bool CreateDefaultRasterizerState(void)
		{
			const std::string err("Unable to create Rasterizer State: Cull ");

			// MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
			D3D11_RASTERIZER_DESC rs_desc = {};

			rs_desc.FillMode = D3D11_FILL_WIREFRAME;
			rs_desc.CullMode = D3D11_CULL_NONE;
			rs_desc.DepthClipEnable = true;
			rs_desc.MultisampleEnable = true;
			rs_desc.FrontCounterClockwise = false;
			rs_desc.DepthBias = 0;
			rs_desc.ScissorEnable = false;
			rs_desc.DepthBiasClamp = 0;
			rs_desc.SlopeScaledDepthBias = 0.0f;
			rs_desc.AntialiasedLineEnable = true;

			if (failed(_device->CreateRasterizerState(&rs_desc, &_rasterizer_states[static_cast<int>(RasterizerStateType::WIRE_FRAME)])))
			{
				Log::Error(err + "WireFrame\n");
				return false;
			}

			rs_desc.FillMode = D3D11_FILL_SOLID;

			if (failed(_device->CreateRasterizerState(&rs_desc, &_rasterizer_states[static_cast<int>(RasterizerStateType::CULL_NONE)])))
			{
				Log::Error(err + "None\n");
				return false;
			}

			rs_desc.CullMode = D3D11_CULL_BACK;

			if (failed(_device->CreateRasterizerState(&rs_desc, &_rasterizer_states[static_cast<int>(RasterizerStateType::CULL_BACK)])))
			{
				Log::Error(err + "Back\n");
				return false;
			}

			rs_desc.CullMode = D3D11_CULL_FRONT;
			if (failed(_device->CreateRasterizerState(&rs_desc, &_rasterizer_states[static_cast<int>(RasterizerStateType::CULL_FRONT)])))
			{
				Log::Error(err + "Front\n");
				return false;
			}

			Log::Info("Rasterizer state create process done.");

			return true;
		}

		bool CreateDefaultBlendState(void)
		{
			const std::string err("Unable to create Blend State: ");

			D3D11_RENDER_TARGET_BLEND_DESC rt_blend_desc = {};
			rt_blend_desc.BlendEnable = true;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_MIN;
			rt_blend_desc.DestBlend = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			rt_blend_desc.SrcBlend = D3D11_BLEND_ONE;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;

			D3D11_BLEND_DESC desc = {};
			desc.RenderTarget[0] = rt_blend_desc;

			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::ADDITIVE_COLOR]))))
			{
				Log::Error(err + "Additive Color\n");
				return false;
			}

			rt_blend_desc.BlendEnable = true;
			rt_blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			rt_blend_desc.DestBlend = D3D11_BLEND_ONE;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0] = rt_blend_desc;
			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::ALPHA_BLEND]))))
			{
				Log::Error(err + "Alpha Blend\n");
				return false;
			}

			rt_blend_desc.BlendEnable = true;
			rt_blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			rt_blend_desc.DestBlend = D3D11_BLEND_ONE;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0] = rt_blend_desc;
			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::SUBTRACT_BLEND]))))
			{
				Log::Error(err + "Subtract Blend\n");
				return false;
			}

			rt_blend_desc.BlendEnable = true;
			rt_blend_desc.SrcBlend = D3D11_BLEND_ZERO;
			rt_blend_desc.DestBlend = D3D11_BLEND_SRC_COLOR;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0] = rt_blend_desc;
			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::MULTIPLE_BLEND]))))
			{
				Log::Error(err + "Multiple Blend\n");
				return false;
			}

			rt_blend_desc.BlendEnable = true;
			rt_blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			rt_blend_desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0] = rt_blend_desc;
			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::ALIGNMENT_BLEND]))))
			{
				Log::Error(err + "Alignment Blend\n");
				return false;
			}

			rt_blend_desc.BlendEnable = false;
			rt_blend_desc.SrcBlend = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlend = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
			rt_blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rt_blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
			rt_blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rt_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.RenderTarget[0] = rt_blend_desc;
			if (failed(_device->CreateBlendState(&desc, &(_blend_states[BlendStateType::DISABLED]))))
			{
				Log::Error(err + "Disabled\n");
				return false;
			}

			Log::Info("Blend state create process done.");

			return true;
		}

		bool CreateDefaultSamplerState(void)
		{
			const std::string err("Unable to create Sampler State: ");

			D3D11_SAMPLER_DESC sampler_desc = {};
			sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
			sampler_desc.MaxAnisotropy = 16;
			sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			//::memcpy(sampler_desc.BorderColor, 0, sizeof(float) * 4);
			sampler_desc.MinLOD = 0;
			sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			if (failed(_device->CreateSamplerState(&sampler_desc, &(_sampler_states[SamplerStateType::WRAP_SAMPLER]))))
			{
				Log::Error(err + "Wrap\n");
				return false;
			}

			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			if (failed(_device->CreateSamplerState(&sampler_desc, &(_sampler_states[SamplerStateType::POINT_SAMPLER]))))
			{
				Log::Error(err + "Point\n");
				return false;
			}

			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			if (failed(_device->CreateSamplerState(&sampler_desc, &(_sampler_states[SamplerStateType::LINEAR_FILTER_SAMPLER_WRAP_UVW]))))
			{
				Log::Error(err + "Linear Wrap\n");
				return false;
			}

			sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			if (failed(_device->CreateSamplerState(&sampler_desc, &(_sampler_states[SamplerStateType::LINEAR_FILTER_SAMPLER]))))
			{
				Log::Error(err + "Linear\n");
				return false;
			}

			Log::Info("Sampler state create process done.");

			return true;
		}

		bool CreateDefaultDepthStencilState(void)
		{
			const std::string err("Unable to create Depth Stencil State: ");

			D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {};

			// Set up the description of the stencil state.
			depth_stencil_desc.DepthEnable = true;
			depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			depth_stencil_desc.StencilEnable = true;
			depth_stencil_desc.StencilReadMask = 0xFF;
			depth_stencil_desc.StencilWriteMask = 0xFF;

			// Stencil operations if pixel is front-facing.
			depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			// Stencil operations if pixel is back-facing.
			depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_LESS;

			// Create the depth stencil states.
			if (failed(_device->CreateDepthStencilState(&depth_stencil_desc, &_depth_stencil_states[DepthStencilStateType::DEPTH_STENCIL_WRITE])))
			{
				Log::Error(err + "D S Write\n");
				return false;
			}

			depth_stencil_desc.DepthEnable = false;
			depth_stencil_desc.StencilEnable = false;
			if (failed(_device->CreateDepthStencilState(&depth_stencil_desc, &_depth_stencil_states[DepthStencilStateType::DEPTH_STENCIL_DISABLED])))
			{
				Log::Error(err + "D S Disabled\n");
				return false;
			}

			depth_stencil_desc.DepthEnable = true;
			depth_stencil_desc.StencilEnable = false;
			if (failed(_device->CreateDepthStencilState(&depth_stencil_desc, &_depth_stencil_states[DepthStencilStateType::DEPTH_WRITE])))
			{
				Log::Error(err + "Depth Write\n");
				return false;
			}

			depth_stencil_desc.DepthEnable = false;
			depth_stencil_desc.StencilEnable = true;
			if (failed(_device->CreateDepthStencilState(&depth_stencil_desc, &_depth_stencil_states[DepthStencilStateType::STENCIL_WRITE])))
			{
				Log::Error(err + "Stencil Write\n");
				return false;
			}

			depth_stencil_desc.DepthEnable = true;
			depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			depth_stencil_desc.StencilEnable = true;
			if (failed(_device->CreateDepthStencilState(&depth_stencil_desc, &_depth_stencil_states[DepthStencilStateType::DEPTH_TEST_ONLY])))
			{
				Log::Error(err + "Write\n");
				return false;
			}

			Log::Info("Depth stencil state create process done.");

			return true;
		}

		void ReportLiveObjects(const std::string& log_message)
		{
#ifdef _DEBUG
			if (!log_message.empty())
			{
				Log::Info(log_message);
			}

			if (succeeded(_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(_debug.GetAddressOf()))))
			{
				_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
			}

			Log::Info("Report live objects create process done.");
#endif
		}

		bool ChangeWindowSize(UINT Width, UINT Height)
		{
			if (!_swap_chain) return false;

			DXGI_SWAP_CHAIN_DESC desc;
			_swap_chain->GetDesc(&desc);

			_device_context->OMSetRenderTargets(0, nullptr, nullptr);

			Log::Info("Release back render target.");

			if (failed(_swap_chain->ResizeBuffers(desc.BufferCount, Width, Height, desc.BufferDesc.Format, desc.Flags))) return false;

			if (!CreateDefaultRenderTargetView()) return false;

			if (!CreateDepthStencilView()) return false;

			_device_context->OMSetRenderTargets(1, _render_targets[RenderTargetType::BACK_BUFFER].GetAddressOf(), _depth_stencil_view.Get());

			return true;
		}

		bool Initialize(int width, int height, const bool vsync, HWND hwnd, const bool FULL_SCREEN)
		{
			_vsync_enabled = false;
			_fullscreen = false;
			_window_width = _window_height = _numerator = _denominator = 0;

			for (int i = 0; i < static_cast<int>(RasterizerStateType::RASTERIZER_STATE_MAX); ++i)
			{
				_rasterizer_states.emplace_back();
			}

			for (int i = 0; i < static_cast<int>(BlendStateType::BLEND_STATE_MAX); ++i)
			{
				_blend_states.emplace_back();
			}

			for (int i = 0; i < static_cast<int>(DepthStencilStateType::DEPTH_STENCIL_STATE_MAX); ++i)
			{
				_depth_stencil_states.emplace_back();
			}

			for (int i = 0; i < static_cast<int>(SamplerStateType::DEFAULT_SAMPLER_MAX); ++i)
			{
				_sampler_states.emplace_back();
			}

			_hwnd = hwnd;
			_window_width = width;
			_window_height = height;

			_vsync_enabled = vsync;
			_fullscreen = FULL_SCREEN;

			if (!GetDisplayMode()) return false;

			if (!InitDeviceAndSwapChain()) return false;

			if (!CreateRenderTargetView()) return false;

			if (!CreateDepthStencilView()) return false;

			// RenderTarget set to immediate device context
			_device_context->OMSetRenderTargets(1, _render_targets[RenderTargetType::BACK_BUFFER].GetAddressOf(), _depth_stencil_view.Get());

			if (!CreateViewPort()) return false;

			if (!CreateDefaultRasterizerState()) return false;

			if (!CreateDefaultBlendState()) return false;

			if (!CreateDefaultSamplerState()) return false;

			if (!CreateDefaultDepthStencilState()) return false;

			Log::Info("Graphics system initialized.\n");

			return true;
		}

		void Finalize(void)
		{
			ReportLiveObjects("Finalize call.");

			if (_swap_chain)
			{
				_swap_chain->SetFullscreenState(false, nullptr);
				_swap_chain.Reset();
			}

			if (_device_context)
			{
				_device_context.Reset();
			}

			if (_device)
			{
				_device.Reset();
			}

			if (_adapter)
			{
				_adapter.Reset();
			}

			if (_factory)
			{
				_factory.Reset();
			}

#ifdef _DEBUG
			if (_debug)
			{
				_debug.Reset();
			}

			if (_annotation)
			{
				_annotation->EndEvent();
				_annotation.Reset();
			}
#endif

			Log::Info("Graphics system finalized.\n");

			return;
		}

		void BeginFrame(void)
		{
			float clear_color[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; //red, green, blue, alpha
			_device_context->ClearRenderTargetView(_render_targets[RenderTargetType::BACK_BUFFER].Get(), clear_color);
			_device_context->ClearDepthStencilView(_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}

		void EndFrame()
		{
			_swap_chain->Present(static_cast<unsigned>(_vsync_enabled), 0);

			_device_context->IASetInputLayout(nullptr);
			_device_context->VSSetShader(nullptr, nullptr, 0);
			_device_context->GSSetShader(nullptr, nullptr, 0);
			_device_context->HSSetShader(nullptr, nullptr, 0);
			_device_context->DSSetShader(nullptr, nullptr, 0);
			_device_context->PSSetShader(nullptr, nullptr, 0);
			_device_context->CSSetShader(nullptr, nullptr, 0);
		}

		bool ChangeWindowMode(void)
		{
			if (!_swap_chain) return false;

			DXGI_SWAP_CHAIN_DESC desc;
			_swap_chain->GetDesc(&desc);

			int full_screen;

			// GetFullscreenState
			_swap_chain->GetFullscreenState(&full_screen, NULL);

			// SetFullscreenState
			_swap_chain->SetFullscreenState(!full_screen, NULL);

			switch (full_screen)
			{
			case TRUE:
				Log::Info("Change full screen.");
				break;
			case FALSE:
				Log::Info("Change window screen.");
				break;
			}

			::ShowWindow(_hwnd, SW_SHOW);

			if (!ChangeWindowSize(0, 0)) return false;

			return true;
		}

		void SetRenderTarget(RenderTargetType type)
		{
			_device_context->OMSetRenderTargets(1, _render_targets[type].GetAddressOf(), _depth_stencil_view.Get());
		}

		void ClearRenderTargetView(RenderTargetType type, const float * clear_color)
		{
			_device_context->ClearRenderTargetView(_render_targets[type].Get(), clear_color);
		}

		void ClearDepthStencilView(void)
		{
			_device_context->ClearDepthStencilView(_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}

		void GetViewPort(D3D11_VIEWPORT* vp)
		{
			UINT	VP_num = 1;
			_device_context->RSGetViewports(&VP_num, vp);
		}

		void SetViewPort(D3D11_VIEWPORT* vp)
		{
			_device_context->RSSetViewports(1, vp);
		}

		void SetBlendState(BlendStateType type)
		{
			float blendFactor[4] = { D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO };
			_device_context->OMSetBlendState(_blend_states[type].Get(), blendFactor, 0xffffffff);
		}

		void SetRasterizerState(RasterizerStateType type)
		{
			_device_context->RSSetState(_rasterizer_states[type].Get());
		}

		void SetDepthStencilState(DepthStencilStateType type)
		{
			_device_context->OMSetDepthStencilState(_depth_stencil_states[type].Get(), 0);
		}

		Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSamplerState(SamplerStateType type)
		{
			return _sampler_states[type];
		}

		void SetSamplerState(unsigned int shader_type, SamplerStateType ss_type, unsigned int register_slot)
		{
			switch (shader_type)
			{
			case ShaderType::VS:
				_device_context->VSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			case ShaderType::PS:
				_device_context->PSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			case ShaderType::GS:
				_device_context->GSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			case ShaderType::HS:
				_device_context->HSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			case ShaderType::DS:
				_device_context->DSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			case ShaderType::CS:
				_device_context->CSSetSamplers(register_slot, 1, &_sampler_states[ss_type]);
				break;
			}
		}

		void SetPSTexture(UINT register_slot, UINT num_views, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv)
		{
			_device_context->PSSetShaderResources(register_slot, num_views, srv.GetAddressOf());
		}

		Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice(void) { return _device; }

		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetDeviceContext(void) { return _device_context; }

		HWND GetWindowHandle(void) { return _hwnd; }
	}
}