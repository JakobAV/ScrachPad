#include <windows.h>
#include <D3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>

#pragma comment (lib, "D3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define D12Call(x) if (!SUCCEEDED((x))) { InvalidCodePath; }
#define ComRelease(x) if (x) { x->Release(); }

struct Vertex { FLOAT x, y, z; FLOAT color[4]; };

constexpr u32 FrameCount = 2;
struct D3D12Instance
{
    D3D12_VIEWPORT ViewPort;
    D3D12_RECT SissorRect;
    ID3D12Device* Device;
    ID3D12CommandQueue* CommandQueue;
    IDXGISwapChain3* SwapChain;
    ID3D12DescriptorHeap* RtvHeap;
    ID3D12Resource* RenderTargets[FrameCount];
    ID3D12CommandAllocator* CommandAllocator;
    ID3D12PipelineState* PipelineState;
    ID3D12GraphicsCommandList* CommandList;
    ID3D12RootSignature* RootSignature;

    ID3D12Resource* VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

    ID3D12Fence* Fence;
    HANDLE FenceEvent;
    u64 FenceValue;

    u32 FrameIndex;
    u32 RtvDescriptorSize;
    bool IsInitialized;
};

// Shaders:
const char ShaderData[] = "\
struct VOut\
{\
float4 position : SV_POSITION;\
float4 color : COLOR;\
};\
\
VOut VShader(float4 position : POSITION, float4 color : COLOR)\
{ \
VOut output; \
\
output.position = position; \
output.color = color; \
\
return output; \
}\
\
float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET\
{ \
return color; \
}\
";

D3D12Instance D3D12Init(HWND windowHandle)
{
    D3D12Instance result = {};
    result.ViewPort = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    result.SissorRect =  { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
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
    factory->Release();

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

    D12Call(result.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result.Fence)));
    result.FenceValue = 1;
    result.FenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (result.Fence == nullptr)
    {
        InvalidCodePath;
    }
    
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signature;
    ID3DBlob* error;
    D12Call(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    D12Call(result.Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&result.RootSignature)));
    signature->Release();
    ComRelease(error);

    u32 compilerFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ID3D10Blob *VS, *PS;
    D3DCompile((LPCVOID)ShaderData, sizeof(ShaderData), 0, 0, 0, "VShader", "vs_5_0", compilerFlags, 0, &VS, 0);
    D3DCompile((LPCVOID)ShaderData, sizeof(ShaderData), 0, 0, 0, "PShader", "ps_5_0", compilerFlags, 0, &PS, 0);

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDesc, ArrayCount(inputElementDesc) };
    psoDesc.pRootSignature = result.RootSignature;
    psoDesc.VS = { VS->GetBufferPointer(), VS->GetBufferSize() };
    psoDesc.PS = { PS->GetBufferPointer(), PS->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    D12Call(result.Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result.PipelineState)));

    VS->Release();
    PS->Release();

    D12Call(result.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, result.CommandAllocator, nullptr, IID_PPV_ARGS(&result.CommandList)));
    D12Call(result.CommandList->Close());

    Vertex triangleVertices[] =
    {
        { 0.0f, 0.5f, 0.0f, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { 0.45f, -0.5, 0.0f, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { -0.45f, -0.5f, 0.0f, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Width = sizeof(triangleVertices);
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.Alignment = 0;
    resourceDescription.MipLevels = 1;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.SampleDesc.Count = 1;
    D12Call(result.Device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&result.VertexBuffer)
    ));

    u8* pVertexDataBegin;
    D3D12_RANGE readRange = { 0, 0 };
    D12Call(result.VertexBuffer->Map(0, &readRange, (void* *)&pVertexDataBegin));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    result.VertexBuffer->Unmap(0, nullptr);

    result.VertexBufferView.BufferLocation = result.VertexBuffer->GetGPUVirtualAddress();
    result.VertexBufferView.StrideInBytes = sizeof(Vertex);
    result.VertexBufferView.SizeInBytes = sizeof(triangleVertices);

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
    ComRelease(instance->RootSignature);
    ComRelease(instance->VertexBuffer);
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

    instance->CommandList->SetGraphicsRootSignature(instance->RootSignature);
    instance->CommandList->RSSetViewports(1, &instance->ViewPort);
    instance->CommandList->RSSetScissorRects(1, &instance->SissorRect);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = instance->RtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += instance->FrameIndex * instance->RtvDescriptorSize;
    instance->CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    instance->CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    instance->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    instance->CommandList->IASetVertexBuffers(0, 1, &instance->VertexBufferView);
    instance->CommandList->DrawInstanced(3, 1, 0, 0);
    
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
                          0,
                          0,
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