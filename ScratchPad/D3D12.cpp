#include <windows.h>
#include <D3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <wrl.h>

#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define D12Call(x) if (!SUCCEEDED((x))) { InvalidCodePath; }

using Microsoft::WRL::ComPtr;

void InitD3D12(HWND windowHandle)
{
	UINT dxiFactoryFlags = 0;

	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		debugController->Release();
	}

	IDXGIFactory4* factory;
	D12Call(CreateDXGIFactory2(dxiFactoryFlags, IID_PPV_ARGS(&factory)));

	IDXGIAdapter1* hardwareAdapter = nullptr;

	IDXGIFactory6* factory6;
	if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&hardwareAdapter)));
				++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			hardwareAdapter->GetDesc1(&desc);
			if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
		factory6->Release();
	}

	ID3D12Device* device;
	D12Call(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
	hardwareAdapter->Release();

	ID3D12CommandQueue* commandQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	D12Call(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = SCREEN_WIDTH;
	swapChainDesc.Height = SCREEN_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	IDXGISwapChain3* swapChain;
	IDXGISwapChain1* swapChain1;
	D12Call(factory->CreateSwapChainForHwnd(
		commandQueue,
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	D12Call(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
	D12Call(swapChain1->QueryInterface(&swapChain));
	swapChain1->Release();
	UINT frameIndex = swapChain->GetCurrentBackBufferIndex();

	ID3D12DescriptorHeap* rtvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	D12Call(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	ID3D12Resource* renderTargets[2];

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (u32 n = 0; n < 2; ++n)
	{
		D12Call(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
		device->CreateRenderTargetView(renderTargets[n], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}
	ID3D12CommandAllocator* commandAllocator;
	D12Call(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

	factory->Release();

	commandAllocator->Release();
	for (u32 n = 0; n < 2; ++n)
	{
		renderTargets[n]->Release();
	}
	rtvHeap->Release();
	swapChain->Release();
	commandQueue->Release();
	device->Release();
}



LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	RegisterClassEx(&wc);

	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(NULL,
		L"WindowClass1",
		L"Our First Windowed Program",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	MSG msg = { 0 };
	InitD3D12(hWnd);
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				break;
			}
		}

	}

	return (int)msg.wParam;
}

int Go()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	return WinMain(NULL, NULL, GetCommandLineA(), 1);
}