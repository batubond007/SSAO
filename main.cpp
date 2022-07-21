#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <d3dcompiler.h>
#include <iostream>
#include <vector>
#include "Helpers.h"
#include <examples/example_win32_directx11/glm/gtx/compatibility.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// Variables

int sampleCount = 60;


// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
HWND hwnd;
ID3DBlob* vs_blob_ptr = NULL, * ps_blob_ptr = NULL, * error_blob = NULL;
HRESULT hr;
ID3D11InputLayout* input_layout_ptr = NULL;

// VertexData
struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

std::vector<Vertex> vertex_data_array;
std::vector<int> index_data_array;
UINT vertex_stride = sizeof(glm::vec3) + sizeof(glm::vec2);
UINT vertex_offset = 0;
ID3D11Buffer* vertex_buffer_ptr = NULL;
ID3D11Buffer* index_buffer_ptr = NULL;
ID3D11Buffer* constantBuffer;

// Shader Data
ID3D11VertexShader* vertex_shader_ptr = NULL;
ID3D11PixelShader* pixel_shader_ptr = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
glm::mat4 GetViewMatrix(glm::vec3 Position, glm::vec3 Front, glm::vec3 Up);
glm::mat4 GetProjectionMatrix();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Depth
ID3D11Texture2D* depthTex;
ID3D11DepthStencilState* depthState;

// Tex
ID3D11ShaderResourceView* textureView;
ID3D11Texture2D* Tex;
ID3D11SamplerState* samplerState;

struct Constants
{
    glm::float4 color;
    glm::mat4 modellingMatrix;
    glm::mat4 viewingMatrix;
    glm::mat4 projectionMatrix;
};


void CompileShaders() {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG; // add more debug output
#endif

    // COMPILE VERTEX SHADER
    hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "vs_main",
        "vs_5_0",
        flags,
        0,
        &vs_blob_ptr,
        &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (vs_blob_ptr) { vs_blob_ptr->Release(); }
        assert(false);
    }

    // COMPILE PIXEL SHADER
    hr = D3DCompileFromFile(
        L"shaders.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "ps_main",
        "ps_5_0",
        flags,
        0,
        &ps_blob_ptr,
        &error_blob);
    if (FAILED(hr)) {
        if (error_blob) {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
            std::cout << (char*)error_blob->GetBufferPointer() << std::endl;

        }
        if (ps_blob_ptr) { ps_blob_ptr->Release(); }
        assert(false);
    }
}

void CreateShaders() {
    hr = g_pd3dDevice->CreateVertexShader(
        vs_blob_ptr->GetBufferPointer(),
        vs_blob_ptr->GetBufferSize(),
        NULL,
        &vertex_shader_ptr);
    assert(SUCCEEDED(hr));

    hr = g_pd3dDevice->CreatePixelShader(
        ps_blob_ptr->GetBufferPointer(),
        ps_blob_ptr->GetBufferSize(),
        NULL,
        &pixel_shader_ptr);
    assert(SUCCEEDED(hr));
}

void InputLayout() {
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
  { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  /*
  { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    */
  { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

    };
    hr = g_pd3dDevice->CreateInputLayout(
        inputElementDesc,
        ARRAYSIZE(inputElementDesc),
        vs_blob_ptr->GetBufferPointer(),
        vs_blob_ptr->GetBufferSize(),
        &input_layout_ptr);
    assert(SUCCEEDED(hr));
}

void CreateVertex() {
    float stepDivisionFactor = 4;
    for (int i = 0; i < sampleCount; i++)
    {
        float ypos = i / stepDivisionFactor - sampleCount / (stepDivisionFactor * 2);
        for (int j = 0; j < sampleCount; j++)
        {
            float xpos = j / stepDivisionFactor - sampleCount / (stepDivisionFactor * 2);
            float zpos = sin((i + j)) / 2;
            Vertex v;
            v.pos = glm::vec3(xpos, ypos, zpos);
            v.uv = glm::vec2((float)j / sampleCount, (float)i / sampleCount);
            vertex_data_array.push_back(v);

        }
    }

    std::cout << sizeof(float) * vertex_data_array.size() * 3 << std::endl;

    for (int i = 0; i < sampleCount - 1; i++)
    {
        for (int j = 0; j < sampleCount - 1; j++)
        {
            int x1 = i * sampleCount + j;
            int x2 = i * sampleCount + j + 1;
            int y1 = (i + 1) * sampleCount + j;
            int y2 = (i + 1) * sampleCount + j + 1;

            index_data_array.push_back(x1);
            index_data_array.push_back(x2);
            index_data_array.push_back(y1);

            index_data_array.push_back(x2);
            index_data_array.push_back(y2);
            index_data_array.push_back(y1);
        }
    }

    { /*** load mesh data into vertex buffer **/
        D3D11_BUFFER_DESC vertex_buff_descr = {};
        vertex_buff_descr.ByteWidth = sizeof(float) * 5 * vertex_data_array.size();
        vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT;
        vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = vertex_data_array.data();
        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &vertex_buff_descr,
            &sr_data,
            &vertex_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    { /*** load mesh data into vertex buffer **/
        D3D11_BUFFER_DESC index_buff_descr = {};
        index_buff_descr.ByteWidth = index_data_array.size() * sizeof(int);
        index_buff_descr.Usage = D3D11_USAGE_DEFAULT;
        index_buff_descr.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = index_data_array.data();
        HRESULT hr = g_pd3dDevice->CreateBuffer(
            &index_buff_descr,
            &sr_data,
            &index_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    {
        D3D11_BUFFER_DESC constantBufferDesc = {};
        // ByteWidth must be a multiple of 16, per the docs
        constantBufferDesc.ByteWidth = sizeof(Constants) + 0xf & 0xfffffff0;
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hResult = g_pd3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
        assert(SUCCEEDED(hResult));
    }
}

void ClearColorAndSetViewPort() {
    /* clear the back buffer to cornflower blue for the new frame */
    float background_colour[4] = {
      0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(
        g_mainRenderTargetView, background_colour);

    RECT winRect;
    GetClientRect(hwnd, &winRect);
    D3D11_VIEWPORT viewport = {
      0.0f,
      0.0f,
      (FLOAT)(winRect.right - winRect.left),
      (FLOAT)(winRect.bottom - winRect.top),
      0.0f,
      1.0f };
    g_pd3dDeviceContext->RSSetViewports(1, &viewport);

    ID3D11RasterizerState* WireFrame;
    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    //wfdesc.FillMode = D3D11_FILL_WIREFRAME;
    wfdesc.FillMode = D3D11_FILL_SOLID;
    wfdesc.CullMode = D3D11_CULL_NONE;
    hr = g_pd3dDevice->CreateRasterizerState(&wfdesc, &WireFrame);
    g_pd3dDeviceContext->RSSetState(WireFrame);
}

void UpdateInputAssembler() {
    g_pd3dDeviceContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->IASetInputLayout(input_layout_ptr);
    g_pd3dDeviceContext->IASetVertexBuffers(
        0,
        1,
        &vertex_buffer_ptr,
        &vertex_stride,
        &vertex_offset);
    g_pd3dDeviceContext->IASetIndexBuffer(index_buffer_ptr, DXGI_FORMAT_R32_UINT, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
}

void SetShaders() {
    g_pd3dDeviceContext->VSSetShader(vertex_shader_ptr, NULL, 0);
    g_pd3dDeviceContext->PSSetShader(pixel_shader_ptr, NULL, 0);
}

void SetConstants() {
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    g_pd3dDeviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    Constants* constants = (Constants*)(mappedSubresource.pData);
    constants->color = { 1, 1, 1, 1.f };
    constants->modellingMatrix = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -20)), 00.f, glm::vec3(0, 0, 1));
    constants->projectionMatrix = GetProjectionMatrix();
    constants->viewingMatrix = GetViewMatrix(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    g_pd3dDeviceContext->Unmap(constantBuffer, 0);
}

ID3D11Texture2D* CreateDepthBuffer() {
    ID3D11Texture2D* pDepthStencil = NULL;
    D3D11_TEXTURE2D_DESC descDepth;
    descDepth.Width = 1920;
    descDepth.Height = 1080;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil);
    assert(SUCCEEDED(hr));

    return pDepthStencil;
}

ID3D11DepthStencilState* CreateDepthState() {
    D3D11_DEPTH_STENCIL_DESC dsDesc;

    // Depth test parameters
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    dsDesc.StencilEnable = true;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    ID3D11DepthStencilState* pDSState;
    hr = g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState);
    assert(SUCCEEDED(hr));

    return pDSState;
}

ID3D11DepthStencilView* BindDepthBuffer() {
    // Bind depth stencil state
    g_pd3dDeviceContext->OMSetDepthStencilState(depthState, 1);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Flags = 0;
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    // Create the depth stencil view
    ID3D11DepthStencilView* pDSV;
    hr = g_pd3dDevice->CreateDepthStencilView(depthTex, // Depth stencil texture
        &descDSV, // Depth stencil desc
        &pDSV);  // [out] Depth stencil view
    assert(SUCCEEDED(hr));

    return pDSV;
}

ID3D11Texture2D* CreateTexture() {
    // Create Sampler State
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    g_pd3dDevice->CreateSamplerState(&samplerDesc, &samplerState);

    // Load Image
    int texWidth, texHeight, texNumChannels;
    int texForceNumChannels = 4;
    unsigned char* testTextureBytes = stbi_load("testTexture.png", &texWidth, &texHeight,
        &texNumChannels, texForceNumChannels);
    assert(testTextureBytes);
    int texBytesPerRow = 4 * texWidth;

    // Create Texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = texWidth;
    textureDesc.Height = texHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
    textureSubresourceData.pSysMem = testTextureBytes;
    textureSubresourceData.SysMemPitch = texBytesPerRow;

    ID3D11Texture2D* texture;
    hr = g_pd3dDevice->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);
    assert(SUCCEEDED(hr));

    //hr = g_pd3dDevice->CreateShaderResourceView(texture, nullptr, &textureView);
    //assert(SUCCEEDED(hr));

    free(testTextureBytes);

    return texture;
}

// Main code
int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Create Shaders
    CompileShaders();
    CreateShaders();

    CreateVertex();

    // Input Layout
    InputLayout();

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Create Resources
    depthTex = CreateDepthBuffer();
    depthState = CreateDepthState();

    Tex = CreateTexture();

    std::cout << "Stride size: " << vertex_stride << std::endl;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        auto depthView = BindDepthBuffer();
        g_pd3dDeviceContext->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ClearColorAndSetViewPort();

        // Set Output Merger
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, depthView);

        SetConstants();

        UpdateInputAssembler();

        SetShaders();

        g_pd3dDeviceContext->DrawIndexed(index_data_array.size(), 0, 0);

        // Set Output Merger
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ClearColorAndSetViewPort();

        SetConstants();
        UpdateInputAssembler();
        SetShaders();

        // Set Depth texture
        D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
        sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sr_desc.Texture2D.MostDetailedMip = 0;
        sr_desc.Texture2D.MipLevels = -1;
        hr = g_pd3dDevice->CreateShaderResourceView(depthTex, &sr_desc, &textureView);
        assert(SUCCEEDED(hr));

        g_pd3dDeviceContext->PSSetShaderResources(0, 1, &textureView);
        g_pd3dDeviceContext->PSSetSamplers(0, 1, &samplerState);

        g_pd3dDeviceContext->DrawIndexed(index_data_array.size(), 0, 0);

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        /*if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        } */
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 GetViewMatrix(glm::vec3 Position, glm::vec3 Front, glm::vec3 Up)
{
    return glm::lookAt(Position, Position + Front, Up);
}

// returns projection matrix using perspective projection
glm::mat4 GetProjectionMatrix() {
    float fovyRad = (float)(45.0 / 180.0) * M_PI;
    float aspectRatio = 1920.0f / 1080.0f;
    return glm::perspective(fovyRad, aspectRatio, 1.0f, 10000.0f);
}
