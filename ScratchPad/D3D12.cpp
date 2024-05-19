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
#define ComRelease(x) if (x) { x->Release(); }

using Microsoft::WRL::ComPtr;

constexpr u32 FrameCount = 2;
struct D3D12Instance
{
    ID3D12Device* Device;
    ID3D12CommandQueue* CommandQueue;
    IDXGISwapChain3* SwapChain;
    ID3D12DescriptorHeap* RtvHeap;
    ID3D12Resource* RenderTargets[FrameCount];
    ID3D12CommandAllocator* CommandAllocator;
    ID3D12PipelineState* PipelineState;
    ID3D12GraphicsCommandList* CommandList;

    ID3D12Fence* Fence;
    HANDLE FenceEvent;
    u64 FenceValue;

    u32 FrameIndex;
    u32 RtvDescriptorSize;
    bool IsInitialized;
};

D3D12Instance D3D12Init(HWND windowHandle)
{
    D3D12Instance result = {};
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

    D12Call(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&result.Device)));
    hardwareAdapter->Release();

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    D12Call(result.Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&result.CommandQueue)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = SCREEN_WIDTH;
    swapChainDesc.Height = SCREEN_HEIGHT;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapChain1;
    D12Call(factory->CreateSwapChainForHwnd(
        result.CommandQueue,
        windowHandle,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    ));

    D12Call(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
    D12Call(swapChain1->QueryInterface(&result.SwapChain));
    swapChain1->Release();
    result.FrameIndex = result.SwapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    D12Call(result.Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&result.RtvHeap)));

    result.RtvDescriptorSize = result.Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = result.RtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (u32 n = 0; n < FrameCount; ++n)
    {
        D12Call(result.SwapChain->GetBuffer(n, IID_PPV_ARGS(&result.RenderTargets[n])));
        result.Device->CreateRenderTargetView(result.RenderTargets[n], nullptr, rtvHandle);
        rtvHandle.ptr += result.RtvDescriptorSize;
    }
    D12Call(result.Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result.CommandAllocator)));
    D12Call(result.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, result.CommandAllocator, nullptr, IID_PPV_ARGS(&result.CommandList)));
    result.CommandList->Close();
    D12Call(result.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result.Fence)));
    result.FenceValue = 1;
    result.FenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (result.Fence == nullptr)
    {
        InvalidCodePath;
    }
    
    factory->Release();

    result.IsInitialized = true;
    return result;
}

void D3D12Deinit(D3D12Instance* instance)
{
    ComRelease(instance->CommandAllocator);
    for (u32 n = 0; n < FrameCount; ++n)
    {
        ComRelease(instance->RenderTargets[n]);
    }
    ComRelease(instance->RtvHeap);
    ComRelease(instance->SwapChain);
    ComRelease(instance->CommandQueue);
    ComRelease(instance->Device);

    ComRelease(instance->PipelineState);
    ComRelease(instance->CommandList);
    ComRelease(instance->Fence);
    if (instance->FenceEvent)
    {
        CloseHandle(instance->FenceEvent);
    }
    instance->IsInitialized = false;
}

void D3D12Render(D3D12Instance* instance)
{
    //PopulateCommandList
    D12Call(instance->CommandAllocator->Reset());
    D12Call(instance->CommandList->Reset(instance->CommandAllocator, instance->PipelineState));
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Transition.pResource = instance->RenderTargets[instance->FrameIndex];
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    instance->CommandList->ResourceBarrier(1, &barrierDesc);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = instance->RtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += instance->FrameIndex * instance->RtvDescriptorSize;

    const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    instance->CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    instance->CommandList->ResourceBarrier(1, &barrierDesc);

    D12Call(instance->CommandList->Close());

    //ExecuteCommandList
    ID3D12CommandList* ppCommandLists[] = { instance->CommandList };
    instance->CommandQueue->ExecuteCommandLists(ArrayCount(ppCommandLists), ppCommandLists);

    //Present
    D12Call(instance->SwapChain->Present(1, 0));

    //WaitFroPreviousFrame
    const u64 fenceValue = instance->FenceValue;
    D12Call(instance->CommandQueue->Signal(instance->Fence, fenceValue));
    ++instance->FenceValue;
    if (instance->Fence->GetCompletedValue() < fenceValue)
    {
        D12Call(instance->Fence->SetEventOnCompletion(fenceValue, instance->FenceEvent));
        WaitForSingleObject(instance->FenceEvent, INFINITE);
    }

    instance->FrameIndex = instance->SwapChain->GetCurrentBackBufferIndex();
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
    D3D12Instance instance = D3D12Init(hWnd);
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
        D3D12Render(&instance);
    }
    D3D12Deinit(&instance);
    return (int)msg.wParam;
}

int Go()
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    return WinMain(NULL, NULL, GetCommandLineA(), 1);
}