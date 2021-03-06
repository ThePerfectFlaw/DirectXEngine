#include "DirectXTemplate.h"

using namespace DirectX;

const LONG windowWidth = 1280;
const LONG windowHeight = 720;

LPCSTR windowClassName = "DirectXWindowClass";
LPCSTR windowName = "Morpheus Engine";
HWND windowHandle = 0;

const BOOL enableVSync = TRUE;

// Direct3D device and swap chain.
ID3D11Device* d3dDevice = nullptr;
ID3D11DeviceContext* d3dDeviceContext = nullptr;
IDXGISwapChain* d3dSwapChain = nullptr;

// Render target view for the back buffer of the swap chain.
ID3D11RenderTargetView* d3dRenderTargetView = nullptr;
// Depth/stencil view for use as a depth buffer.
ID3D11DepthStencilView* d3dDepthStencilView = nullptr;
// A texture to associate to the depth stencil view.
ID3D11Texture2D* d3dDepthStencilBuffer = nullptr;

// Define the functionality of the depth/stencil stages.
ID3D11DepthStencilState* d3dDepthStencilState = nullptr;
// Define the functionality of the rasterizer stage.
ID3D11RasterizerState* d3dRasterizerState = nullptr;
D3D11_VIEWPORT Viewport = { 0 };

// Vertex buffer data
ID3D11InputLayout* d3dInputLayout = nullptr;
ID3D11Buffer* d3dVertexBuffer = nullptr;
ID3D11Buffer* d3dIndexBuffer = nullptr;

// Shader data
ID3D11VertexShader* d3dVertexShader = nullptr;
ID3D11PixelShader* d3dPixelShader = nullptr;

// Shader resources.
enum EConstantBuffer
{
	CB_Application,
	CB_Frame,
	CB_Object,
	NumberOfConstantBuffers
};

ID3D11Buffer* d3DConstantBuffers[NumberOfConstantBuffers];

XMMATRIX worldMatrix;
XMMATRIX viewMatrix;
XMMATRIX projectionMatrix;

struct FVertexColour
{
	XMFLOAT3 Position;
	XMFLOAT3 Colour;
};

FVertexColour vertices[8] =
{
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

WORD indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

#pragma region Function declarations
// Forward declarations.
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

template< class ShaderClass >
ShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile);

template<class ShaderClass>
std::string GetLatestProfile();

template<class ShaderClass>
ShaderClass* CreateShader(ID3DBlob* shaderBlob, ID3D11ClassLinkage* classLinkage);

bool LoadContent();
void UnloadContent();

void Update(float deltaTime);
void Clear(const FLOAT clearColour[4], FLOAT clearDepth, UINT8 clearStencil);

void Present(BOOL vSync);
void Render();
void Cleanup();
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync);
int InitialiseDirectX(HINSTANCE hInstance, BOOL vSync);
#pragma endregion

int InitializeApplication(HINSTANCE InHandleInstance, int InCommandShow)
{
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.hInstance = InHandleInstance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = windowClassName;

	if (!RegisterClassEx(&windowClass))
	{
		return -1;
	}

	RECT windowRectangle = { 0, 0, windowWidth, windowHeight };
	AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE);

	windowHandle = CreateWindow(windowClassName, windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top, nullptr, nullptr, InHandleInstance, nullptr);

	if (!windowHandle)
	{
		return -1;
	}

	ShowWindow(windowHandle, InCommandShow);
	UpdateWindow(windowHandle);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paintStruct;
	HDC hDC;

	switch (message)
	{
		case WM_PAINT:
		{
			hDC = BeginPaint(hwnd, &paintStruct);
			EndPaint(hwnd, &paintStruct);
		}
		break;
	
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
	
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

int Run()
{
	MSG message = { 0 };

	static DWORD previousTime = timeGetTime();
	static const float targetFrameRate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFrameRate;

	while (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		else
		{
			DWORD currentTime = timeGetTime();
			float deltaTime = (currentTime - previousTime) / 1000.0f;
			previousTime = currentTime;

			//
			deltaTime = std::min<float>(deltaTime, maxTimeStep);
		}
	}

	return static_cast<int>(message.wParam);
}

int WINAPI wWinMain(HINSTANCE currentInstance, HINSTANCE previousInstance, LPWSTR commandLine, int commandShow)
{
	UNREFERENCED_PARAMETER(previousInstance);
	UNREFERENCED_PARAMETER(commandLine);

	if (!XMVerifyCPUSupport())
	{
		MessageBox(nullptr, TEXT("Failed to verify DirectX Math!"), TEXT("Error"), MB_OK);

		return -1;
	}

	if (InitializeApplication(currentInstance, commandShow) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create the application window!"), TEXT("Error"), MB_OK);

		return -1;
	}

	if (InitialiseDirectX(currentInstance, enableVSync) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create DirectX device and swap chain!"), TEXT("Error"), MB_OK);
	}

	int returnCode = Run();

	return returnCode;
}
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync)
{
	DXGI_RATIONAL refreshRate = { 0, 1 };
	
	if (vsync)
	{
		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;

		// Create a DirectX graphics interface factory.
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(hr))
		{
			MessageBox(0, TEXT("Could not create DXGIFactory instance."), TEXT("Query Refresh Rate"), MB_OK);

			throw new std::exception("Failed to create DXGIFactory.");
		}

		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr))
		{
			MessageBox(0, TEXT("Failed to enumerate adapters."), TEXT("Query Refresh Rate"), MB_OK);

			throw new std::exception("Failed to enumerate adapters.");
		}

		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr))
		{
			MessageBox(0, TEXT("Failed to enumerate adapter outputs."), TEXT("Query Refresh Rate"), MB_OK);

			throw new std::exception("Failed to enumerate adapter outputs.");
		}

		UINT numDisplayModes;
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr);
		if (FAILED(hr))
		{
			MessageBox(0, TEXT("Failed to query display mode list."), TEXT("Query Refresh Rate"), MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		assert(displayModeList);

		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList);
		if (FAILED(hr))
		{
			MessageBox(0, TEXT("Failed to query display mode list."), TEXT("Query Refresh Rate"), MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		// Now store the refresh rate of the monitor that matches the width and height of the requested screen.
		for (UINT i = 0; i < numDisplayModes; ++i)
		{
			if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight)
			{
				refreshRate = displayModeList[i].RefreshRate;
			}
		}

		delete[] displayModeList;
		SafeRelease(adapterOutput);
		SafeRelease(adapter);
		SafeRelease(factory);
	}

	return refreshRate;
}


int InitialiseDirectX(HINSTANCE hInstance, BOOL vSync)
{
	// A window handle must have been created already!
	assert(windowHandle != 0);

	RECT clientRectangle;
	GetClientRect(windowHandle, &clientRectangle);

	unsigned int clientWidth = clientRectangle.right - clientRectangle.left;
	unsigned int clientHeight = clientRectangle.bottom - clientRectangle.top;

	DXGI_SWAP_CHAIN_DESC swapChainDescription;
	ZeroMemory(&swapChainDescription, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDescription.BufferCount = 1;
	swapChainDescription.BufferDesc.Width = clientWidth;
	swapChainDescription.BufferDesc.Height = clientHeight;
	swapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDescription.BufferDesc.RefreshRate = QueryRefreshRate(clientWidth, clientHeight, vSync);
	swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescription.OutputWindow = windowHandle;
	swapChainDescription.SampleDesc.Count = 1;
	swapChainDescription.SampleDesc.Quality = 0;
	swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDescription.Windowed = TRUE;

	UINT createDeviceFlags = 0;

#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	// These are the feature levels that we will accept.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &swapChainDescription, &d3dSwapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);

	if (hr == E_INVALIDARG)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1, D3D11_SDK_VERSION, &swapChainDescription, &d3dSwapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);
	}

	if (FAILED(hr))
	{
		return -1;
	}

	ID3D11Texture2D* backBuffer;

	hr = d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);

	if (FAILED(hr))
	{
		return -1;
	}

	hr = d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &d3dRenderTargetView);

	if (FAILED(hr))
	{
		return -1;
	}

	SafeRelease(backBuffer);

	D3D11_TEXTURE2D_DESC depthStencilBufferDescription;
	ZeroMemory(&depthStencilBufferDescription, sizeof(D3D11_TEXTURE2D_DESC));

	depthStencilBufferDescription.ArraySize = 1;
	depthStencilBufferDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDescription.CPUAccessFlags = 0;
	depthStencilBufferDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDescription.Width = clientWidth;
	depthStencilBufferDescription.Height = clientHeight;
	depthStencilBufferDescription.MipLevels = 1;
	depthStencilBufferDescription.SampleDesc.Count = 1;
	depthStencilBufferDescription.SampleDesc.Quality = 0;
	depthStencilBufferDescription.Usage = D3D11_USAGE_DEFAULT;

	hr = d3dDevice->CreateTexture2D(&depthStencilBufferDescription, nullptr, &d3dDepthStencilBuffer);

	if (FAILED(hr))
	{
		return -1;
	}

	hr = d3dDevice->CreateDepthStencilView(d3dDepthStencilBuffer, nullptr, &d3dDepthStencilView);

	if (FAILED(hr))
	{
		return -1;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilStateDescription;
	ZeroMemory(&depthStencilStateDescription, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilStateDescription.DepthEnable = TRUE;
	depthStencilStateDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDescription.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDescription.StencilEnable = FALSE;

	hr = d3dDevice->CreateDepthStencilState(&depthStencilStateDescription, &d3dDepthStencilState);

	//if (FAILED(hr))
	//{
	//	return -1;
	//}

	D3D11_RASTERIZER_DESC rasterizerDescription;
	ZeroMemory(&rasterizerDescription, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDescription.AntialiasedLineEnable = FALSE;
	rasterizerDescription.CullMode = D3D11_CULL_BACK;
	rasterizerDescription.DepthBias = 0;
	rasterizerDescription.DepthBiasClamp = 0.0f;
	rasterizerDescription.DepthClipEnable = TRUE;
	rasterizerDescription.FillMode = D3D11_FILL_SOLID;
	rasterizerDescription.FrontCounterClockwise = FALSE;
	rasterizerDescription.MultisampleEnable = FALSE;
	rasterizerDescription.ScissorEnable = FALSE;
	rasterizerDescription.SlopeScaledDepthBias = 0.0f;

	hr = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dRasterizerState);

	if (FAILED(hr))
	{
		return -1;
	}

	Viewport.Width = static_cast<float>(clientWidth);
	Viewport.Height = static_cast<float>(clientHeight);
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	return 0;
}

template<>
std::string GetLatestProfile<ID3D11VertexShader>()
{
	assert(d3dDevice);

	D3D_FEATURE_LEVEL featureLevel = d3dDevice->GetFeatureLevel();

	switch (featureLevel)
	{
		case D3D_FEATURE_LEVEL_11_1:
		case D3D_FEATURE_LEVEL_11_0:
			return "vs_5_0";
			break;

		case D3D_FEATURE_LEVEL_10_1:
			return "vs_4_1";
			break;

		case D3D_FEATURE_LEVEL_10_0:
			return "vs_4_0";
			break;

		case D3D_FEATURE_LEVEL_9_3:
			return "vs_4_0_level_9_3";
			break;

		case D3D_FEATURE_LEVEL_9_2:
		case D3D_FEATURE_LEVEL_9_1:
			return "vs_4_0_level_9_1";
			break;
	}

	return "";
}

template<>
std::string GetLatestProfile<ID3D11PixelShader>()
{
	assert(d3dDevice);

	D3D_FEATURE_LEVEL featureLevel = d3dDevice->GetFeatureLevel();

	switch (featureLevel)
	{
		case D3D_FEATURE_LEVEL_11_1:
		case D3D_FEATURE_LEVEL_11_0:
			return "ps_5_0";
			break;

		case D3D_FEATURE_LEVEL_10_1:
			return "ps_4_1";
			break;

		case D3D_FEATURE_LEVEL_10_0:
			return "ps_4_0";
			break;

		case D3D_FEATURE_LEVEL_9_3:
			return "ps_4_0_level_9_3";
			break;

		case D3D_FEATURE_LEVEL_9_2:
		case D3D_FEATURE_LEVEL_9_1:
			return "ps_4_0_level_9_1";
			break;
	}

	return "";
}

template<>
ID3D11VertexShader* CreateShader<ID3D11VertexShader>(ID3DBlob* shaderBlob, ID3D11ClassLinkage* classLinkage)
{
	assert(d3dDevice);
	assert(shaderBlob);

	ID3D11VertexShader* vertexShader = nullptr;
	d3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), classLinkage, &vertexShader);

	return vertexShader;
}

template<>
ID3D11PixelShader* CreateShader<ID3D11PixelShader>(ID3DBlob* shaderBlob, ID3D11ClassLinkage* classLinkage)
{
	assert(d3dDevice);
	assert(shaderBlob);

	ID3D11PixelShader* pixelShader = nullptr;
	d3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), classLinkage, &pixelShader);

	return pixelShader;
}

template<class ShaderClass>
ShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile)
{
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	ShaderClass* shader = nullptr;

	std::string prof = profile;

	if (prof == "latest")
	{
		prof = GetLatestProfile<ShaderClass>();
	}

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT result = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), prof.c_str(), flags, 0, &shaderBlob, &errorBlob);

	if (FAILED(result))
	{
		if (errorBlob)
		{
			std::string errorMessage = (char*)errorBlob->GetBufferPointer();
			OutputDebugStringA(errorMessage.c_str());

			SafeRelease(shaderBlob);
			SafeRelease(errorBlob);
		}

		return false;
	}

	shader = CreateShader<ShaderClass>(shaderBlob, nullptr);

	SafeRelease(shaderBlob);
	SafeRelease(errorBlob);

	return shader;
}

bool LoadContent()
{
	assert(d3dDevice);

	D3D11_BUFFER_DESC vertexBufferDescription;
	ZeroMemory(&vertexBufferDescription, sizeof(D3D11_BUFFER_DESC));

	vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDescription.ByteWidth = sizeof(FVertexColour) * _countof(vertices);
	vertexBufferDescription.CPUAccessFlags = 0;
	vertexBufferDescription.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));

	resourceData.pSysMem = vertices;

	HRESULT result = d3dDevice->CreateBuffer(&vertexBufferDescription, &resourceData, &d3dVertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Create and initialize the index buffer.
	D3D11_BUFFER_DESC indexBufferDescription;
	ZeroMemory(&indexBufferDescription, sizeof(D3D11_BUFFER_DESC));

	indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDescription.ByteWidth = sizeof(WORD) * _countof(indicies);
	indexBufferDescription.CPUAccessFlags = 0;
	indexBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	resourceData.pSysMem = indicies;

	result = d3dDevice->CreateBuffer(&indexBufferDescription, &resourceData, &d3dIndexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	D3D11_BUFFER_DESC constantBufferDescription;
	ZeroMemory(&constantBufferDescription, sizeof(D3D11_BUFFER_DESC));

	constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDescription.ByteWidth = sizeof(XMMATRIX);
	constantBufferDescription.CPUAccessFlags = 0;
	constantBufferDescription.Usage = D3D11_USAGE_DEFAULT;

	result = d3dDevice->CreateBuffer(&constantBufferDescription, nullptr, &d3DConstantBuffers[CB_Application]);
	if (FAILED(result))
	{
		return false;
	}
	result = d3dDevice->CreateBuffer(&constantBufferDescription, nullptr, &d3DConstantBuffers[CB_Frame]);
	if (FAILED(result))
	{
		return false;
	}
	result = d3dDevice->CreateBuffer(&constantBufferDescription, nullptr, &d3DConstantBuffers[CB_Object]);
	if (FAILED(result))
	{
		return false;
	}

	d3dVertexShader = LoadShader<ID3D11VertexShader>(L"", "main", "latest");
	d3dPixelShader = LoadShader<ID3D11PixelShader>(L"", "main", "latest");

	// Setup the projection matrix.
	RECT clientRectangle;
	GetClientRect(windowHandle, &clientRectangle);

	// Compute the exact client dimensions.
	// This is required for a correct projection matrix.
	float clientWidth = static_cast<float>(clientRectangle.right - clientRectangle.left);
	float clientHeight = static_cast<float>(clientRectangle.bottom - clientRectangle.top);

	projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 100.0f);

	d3dDeviceContext->UpdateSubresource(d3DConstantBuffers[CB_Application], 0, nullptr, &projectionMatrix, 0, 0);

	return true;
}

void Update(float deltaTime)
{
	XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	d3dDeviceContext->UpdateSubresource(d3DConstantBuffers[CB_Frame], 0, nullptr, &viewMatrix, 0, 0);


	static float angle = 0.0f;
	angle += 90.0f * deltaTime;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	worldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	d3dDeviceContext->UpdateSubresource(d3DConstantBuffers[CB_Object], 0, nullptr, &worldMatrix, 0, 0);
}

void Clear(const FLOAT clearColour[4], FLOAT clearDepth, UINT8 clearStencil)
{
	d3dDeviceContext->ClearRenderTargetView(d3dRenderTargetView, clearColour);
	d3dDeviceContext->ClearDepthStencilView(d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}

void Present(BOOL vSync)
{
	if (vSync)
	{
		d3dSwapChain->Present(1, 0);
	}
	else
	{
		d3dSwapChain->Present(0, 0);
	}
}
