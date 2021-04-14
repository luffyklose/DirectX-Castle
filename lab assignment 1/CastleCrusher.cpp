//***************************************************************************************
// TexColumnsApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/Camera.h"
#include "FrameResource.h"
#include "Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

    XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify obect data we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjCBIndex = -1;

    Material* Mat = nullptr;
    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
	
    BoundingBox bounds;
};

enum class RenderLayer : int
{
    Opaque = 0,
    Transparent,
    AlphaTested,
    AlphaTestedTreeSprites,
    Count
};

class ShapesApp : public D3DApp
{
public:
    ShapesApp(HINSTANCE hInstance);
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMaterialCBs(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateWaves(const GameTimer& gt);
    void CameraCollisionCheck(const XMVECTOR np1, const XMVECTOR np2);

    void LoadTextures();
    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildWavesGeometry();
    void BuildShapeGeometry();
    void BuildTreeSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRitem = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;

    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    std::unique_ptr<Waves> mWaves;

    // Render items divided by PSO.
    std::vector<RenderItem*> mOpaqueRitems;

    PassConstants mMainPassCB;

    Camera mCamera;

    /*XMVECTOR position = XMVectorSet(-20.0f, 70.0f, -120.5f, 0.0f)
        , frontVec = XMVectorSet(0.0f, 0.0f, .0f, 0.0f)
        , worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), upVec, rightVec; // Set by function
    float pitch = -2.8, yaw = 4.5f;

    //XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.7f * XM_PI;
    float mPhi = 0.35f * XM_PI;
    float mRadius = 130.0f;*/

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        ShapesApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

ShapesApp::ShapesApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

ShapesApp::~ShapesApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

bool ShapesApp::Initialize()
{
    if (!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
    // so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

    mCamera.SetPosition(0.0f, 30.0f, -50.0f);
    XMStoreFloat3(&mCamera.bounds.Center, mCamera.GetPosition());

    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildWavesGeometry();
    BuildTreeSpritesGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void ShapesApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    //XMStoreFloat4x4(&mProj, P);

    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void ShapesApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    AnimateMaterials(gt);
    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void ShapesApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    //step1: 
    //mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);

    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    //step 2
    mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    mCommandList->SetPipelineState(mPSOs["transparent"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    /*if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    	
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }*/

    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        //step4: Instead of updating the angles based on input to orbit camera around scene, 
        //we rotate the camera’s look direction:
        //mTheta += dx;
        //mPhi += dy;

        mCamera.Pitch(dy);
        mCamera.RotateY(dx);
    }
	
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void ShapesApp::OnKeyboardInput(const GameTimer& gt)
{
    /*if (GetAsyncKeyState('W') & 0x8000) {
        position += frontVec * 0.9f;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        position -= frontVec * 0.9f;
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        position += rightVec * 0.9f;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        position -= rightVec * 0.9f;
    }*/

    //step3: we handle keyboard input to move the camera:

    const float dt = gt.DeltaTime();

    XMVECTOR oldPos = mCamera.GetPosition();

    //GetAsyncKeyState returns a short (2 bytes)
    if (GetAsyncKeyState('W') & 0x8000) //most significant bit (MSB) is 1 when key is pressed (1000 000 000 000)
        mCamera.Walk(10.0f * dt);

    if (GetAsyncKeyState('S') & 0x8000)
        mCamera.Walk(-10.0f * dt);

    if (GetAsyncKeyState('A') & 0x8000)
        mCamera.Strafe(-10.0f * dt);

    if (GetAsyncKeyState('D') & 0x8000)
        mCamera.Strafe(10.0f * dt);

    if (GetAsyncKeyState('Q') & 0x8000)
        mCamera.Pedestal(10.0f * dt);

    if (GetAsyncKeyState('E') & 0x8000)
        mCamera.Pedestal(-10.0f * dt);

    if (!XMVector3Equal(oldPos, mCamera.GetPosition()))
    {
        CameraCollisionCheck(mCamera.GetPosition(),oldPos);
    }

    mCamera.UpdateViewMatrix();
}

void ShapesApp::UpdateCamera(const GameTimer& gt)
{
	/*
    // Convert Spherical to Cartesian coordinates.
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
	*/

    /*frontVec = XMVectorSet(cos((yaw)) * cos((pitch)), sin((pitch)), sin((yaw)) * cos((pitch)), 0.0f);

    frontVec = XMVector3Normalize(frontVec);
    rightVec = XMVector3Normalize(XMVector3Cross(frontVec, worldUp));
    upVec = XMVector3Normalize(XMVector3Cross(rightVec, frontVec));

    XMMATRIX view = XMMatrixLookAtLH(position, // Camera position
        position + frontVec, // Look target
        upVec); // Up vector);

    XMStoreFloat4x4(&mView, view);*/
}

void ShapesApp::CameraCollisionCheck(const XMVECTOR np1, const XMVECTOR np2)
{
    BoundingBox newBounds;
    XMStoreFloat3(&newBounds.Center, np1);
    newBounds.Extents = { 2.5f, 2.5f, 2.5f };

    //check collision, leave if it happens
    for (auto& e : mAllRitems)
    {
        if (e->bounds.Contains(newBounds) != DISJOINT)
        {
            XMFLOAT3 OldPos;
            XMStoreFloat3(&OldPos, np2);
            mCamera.SetPosition(OldPos);
            return;
        }
    }

    //move camera
    XMFLOAT3 storeNewPos;
    XMStoreFloat3(&storeNewPos, np1);
    mCamera.SetPosition(storeNewPos);

}

void ShapesApp::AnimateMaterials(const GameTimer& gt)
{
    // Scroll the water material texture coordinates.
    auto waterMat = mMaterials["water"].get();

    float& tu = waterMat->MatTransform(3, 0);
    float& tv = waterMat->MatTransform(3, 1);

    tu += 0.1f * gt.DeltaTime();
    tv += 0.02f * gt.DeltaTime();

    if (tu >= 1.0f)
        tu -= 1.0f;

    if (tv >= 1.0f)
        tv -= 1.0f;

    waterMat->MatTransform(3, 0) = tu;
    waterMat->MatTransform(3, 1) = tv;

    // Material has changed, so need to update cbuffer.
    waterMat->NumFramesDirty = gNumFrameResources;
}

void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
{
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto& e : mAllRitems)
    {
        // Only update the cbuffer data if the constants have changed.  
        // This needs to be tracked per frame resource.
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

            currObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFramesDirty--;
        }
    }
}

void ShapesApp::UpdateMaterialCBs(const GameTimer& gt)
{
    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto& e : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}

void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
{
    /*XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    //mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.EyePosW.x = XMVectorGetX(position);
    mMainPassCB.EyePosW.y = XMVectorGetX(position);
    mMainPassCB.EyePosW.z = XMVectorGetX(position);
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();*/

    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mCamera.GetPosition3f();
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();

    //lights
    mMainPassCB.AmbientLight = { 0.4f, 0.4f, 0.4f, 1.0f };

	//directional light
    mMainPassCB.Lights[0].Direction = { -0.5f, -0.35f, 0.5f };
    mMainPassCB.Lights[0].Strength = { 1.0f, 0.5, 0.3f };

	//Front Wall 
    mMainPassCB.Lights[1].Position = { -15.0f, 5.0f, -30.0f };
    mMainPassCB.Lights[1].Strength = { 1.0f, 1.0f, 0.0f };
    mMainPassCB.Lights[2].Position = { 15.0f, 5.0f, -30.0f };
    mMainPassCB.Lights[2].Strength = { 1.0f, 1.0f, 0.0f };

	//Columns
    mMainPassCB.Lights[3].Position = { -26.0f, 5.0f, -30.0f };
    mMainPassCB.Lights[3].Strength = { 1.0f, 0.0f, 0.0f };
    mMainPassCB.Lights[4].Position = { 26.0f, 5.0f, -30.0f };
    mMainPassCB.Lights[4].Strength = { 1.0f, 0.0f, 0.0f };
    mMainPassCB.Lights[5].Position = { -26.0f, 5.0f, 30.0f };
    mMainPassCB.Lights[5].Strength = { 1.0f, 0.0f, 0.0f };
    mMainPassCB.Lights[6].Position = { 26.0f, 5.0f, 30.0f };
    mMainPassCB.Lights[6].Strength = { 1.0f, 0.0f, 0.0f };

	//Diamonds
    mMainPassCB.Lights[7].Position = { 0.0f, 21.5f, -2.0f };
    mMainPassCB.Lights[7].Strength = { 0.0f, 0.0f, 1.0f };
    mMainPassCB.Lights[8].Position = { 0.0f, 12.5f, 12.5f };
    mMainPassCB.Lights[8].Strength = { 0.0f, 0.0f, 1.0f };
    
    //spotlight
    //mMainPassCB.Lights[9].Position = { 0.0f, 22.0f, -10.0f };
    //mMainPassCB.Lights[9].Direction = { 0.0f, -1.0f, 0.0f };
    //mMainPassCB.Lights[9].SpotPower = 1.0f;
    //mMainPassCB.Lights[9].Strength = { 2.1f, 2.1f, 2.1f };
    //mMainPassCB.Lights[9].FalloffEnd = 20.0f;

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void ShapesApp::UpdateWaves(const GameTimer& gt)
{
    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if ((mTimer.TotalTime() - t_base) >= 0.25f)
    {
        t_base += 0.25f;

        int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
        int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

        float r = MathHelper::RandF(0.2f, 0.5f);

        mWaves->Disturb(i, j, r);
    }

    // Update the wave simulation.
    mWaves->Update(gt.DeltaTime());

    // Update the wave vertex buffer with the new solution.
    auto currWavesVB = mCurrFrameResource->WavesVB.get();
    for (int i = 0; i < mWaves->VertexCount(); ++i)
    {
        Vertex v;

        v.Pos = mWaves->Position(i);
        v.Normal = mWaves->Normal(i);

        // Derive tex-coords from position by 
        // mapping [-w/2,w/2] --> [0,1]
        v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
        v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

        currWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void ShapesApp::LoadTextures()
{
    // Brick texture
    auto wallTex = std::make_unique<Texture>();
    wallTex->Name = "bricksTex";
    wallTex->Filename = L"../Textures/bricks.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), wallTex->Filename.c_str(),
        wallTex->Resource, wallTex->UploadHeap));

    // Stone texture
    auto stoneTex = std::make_unique<Texture>();
    stoneTex->Name = "stoneTex";
    stoneTex->Filename = L"../Textures/bricks2.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), stoneTex->Filename.c_str(),
        stoneTex->Resource, stoneTex->UploadHeap));

    // Roof texture
    auto roofTex = std::make_unique<Texture>();
    roofTex->Name = "roofTex";
    roofTex->Filename = L"../Textures/bricks3.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), roofTex->Filename.c_str(),
        roofTex->Resource, roofTex->UploadHeap));

    // floor texture
    auto tileTex = std::make_unique<Texture>();
    tileTex->Name = "tileTex";
    tileTex->Filename = L"../Textures/tile.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), tileTex->Filename.c_str(),
        tileTex->Resource, tileTex->UploadHeap));

	// water texture
    auto waterTex = std::make_unique<Texture>();
    waterTex->Name = "waterTex";
    waterTex->Filename = L"../Textures/water1.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), waterTex->Filename.c_str(),
        waterTex->Resource, waterTex->UploadHeap));

    auto treeArrayTex = std::make_unique<Texture>();
    treeArrayTex->Name = "treeArrayTex";
    treeArrayTex->Filename = L"../Textures/treeArray.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), treeArrayTex->Filename.c_str(),
        treeArrayTex->Resource, treeArrayTex->UploadHeap));

	// grass texture
    auto grassTex = std::make_unique<Texture>();
    grassTex->Name = "grassTex";
    grassTex->Filename = L"../Textures/grass.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), grassTex->Filename.c_str(),
        grassTex->Resource, grassTex->UploadHeap));

	//board texture
    auto boardTex = std::make_unique<Texture>();
    boardTex->Name = "boardTex";
    boardTex->Filename = L"../Textures/checkboard.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), boardTex->Filename.c_str(),
        boardTex->Resource, boardTex->UploadHeap));

	// ice texture
    auto iceTex = std::make_unique<Texture>();
    iceTex->Name = "iceTex";
    iceTex->Filename = L"../Textures/ice.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), iceTex->Filename.c_str(),
        iceTex->Resource, iceTex->UploadHeap));

    mTextures[wallTex->Name] = std::move(wallTex);
    mTextures[stoneTex->Name] = std::move(stoneTex);
    mTextures[roofTex->Name] = std::move(roofTex);
    mTextures[tileTex->Name] = std::move(tileTex);
    mTextures[waterTex->Name] = std::move(waterTex);
    mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
    mTextures[grassTex->Name] = std::move(grassTex);
    mTextures[boardTex->Name] = std::move(boardTex);
    mTextures[iceTex->Name] = std::move(iceTex);
}

void ShapesApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,  // number of descriptors
        0); // register t0

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0); // register b0
    slotRootParameter[2].InitAsConstantBufferView(1); // register b1
    slotRootParameter[3].InitAsConstantBufferView(2); // register b2

    auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void ShapesApp::BuildDescriptorHeaps()
{
    //
   // Create the SRV heap.
   //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 9; //  Change when adding more descripotrsaf
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    //
    // Fill out the heap with actual descriptors.
    //
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    auto bricksTex = mTextures["bricksTex"]->Resource;
    auto stoneTex = mTextures["stoneTex"]->Resource;
    auto roofTex = mTextures["roofTex"]->Resource;
    auto tileTex = mTextures["tileTex"]->Resource;
    auto waterTex = mTextures["waterTex"]->Resource;
    auto treeArrayTex = mTextures["treeArrayTex"]->Resource;
    auto grassTex = mTextures["grassTex"]->Resource;
    auto boardTex = mTextures["boardTex"]->Resource;
    auto iceTex = mTextures["iceTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = bricksTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = roofTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = roofTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(roofTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = stoneTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = tileTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = waterTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = grassTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = boardTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(boardTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = iceTex->GetDesc().Format;
    md3dDevice->CreateShaderResourceView(iceTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    auto desc = treeArrayTex->GetDesc();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Format = treeArrayTex->GetDesc().Format;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
    md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
}

void ShapesApp::BuildShadersAndInputLayout()
{
    //step3
    const D3D_SHADER_MACRO defines[] =
    {
        "FOG", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "FOG", "1",
        "ALPHA_TEST",  "1",
        NULL, NULL
    };

    mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
    mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
    mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");

    mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_0");
    mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_0");
    mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mTreeSpriteInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void ShapesApp::BuildWavesGeometry()
{
    std::vector<std::uint32_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
    assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for (int i = 0; i < m - 1; ++i)
    {
        for (int j = 0; j < n - 1; ++j)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; // next quad
        }
    }

    UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    // Set dynamically.
    geo->VertexBufferCPU = nullptr;
    geo->VertexBufferGPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["water"] = submesh;

    mGeometries["waterGeo"] = std::move(geo);
}

void ShapesApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(100.0f, 100.0f, 50, 50);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(2.5f, 2.5f, 18.5f, 20, 20);
    GeometryGenerator::MeshData cone = geoGen.CreateCylinder(2.5f, 0.01f, 5.0f, 20, 20);
    GeometryGenerator::MeshData wedge = geoGen.CreateWedge(12.0f, 1.0f, 6.0f, 2);
    GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.5f, 1.5f, 2);
    GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(2.5f, 5.0f, 2.5f, 2);
    GeometryGenerator::MeshData sanlengzhu = geoGen.CreateSanLengZhu(1.5f, 2.0f, 3);
    GeometryGenerator::MeshData trapezoid = geoGen.CreateTrapezoid(1.0f, 2.0f, 2.0f, 3);
    //GeometryGenerator::MeshData flag = geoGen.CreateFlag(3.0f, 2.0f, 1.0f, 3);
    GeometryGenerator::MeshData torus = geoGen.CreateTorus(7.0f, 1.0f, 8, 8);
    GeometryGenerator::MeshData box2 = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);

    //
    // We are concatenating all the geometry into one big vertex/index buffer.  So
    // define the regions in the buffer each submesh covers.
    //

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.Vertices.size();
    UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
    UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
    UINT coneVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
    UINT wedgeVertexOffset = coneVertexOffset + (UINT)cone.Vertices.size();
    UINT pyramidVertexOffset = wedgeVertexOffset + (UINT)wedge.Vertices.size();
    UINT diamondVertexOffset = pyramidVertexOffset + (UINT)pyramid.Vertices.size();
    UINT sanlengzhuVertexOffset = diamondVertexOffset + (UINT)diamond.Vertices.size();
    UINT trapezoidVertexOffset = sanlengzhuVertexOffset + (UINT)sanlengzhu.Vertices.size();
    UINT torusVertexOffset = trapezoidVertexOffset + (UINT)trapezoid.Vertices.size();
    UINT box2VertexOffset = torusVertexOffset + (UINT)torus.Vertices.size();

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = (UINT)box.Indices32.size();
    UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
    UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
    UINT coneIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
    UINT wedgeIndexOffset = coneIndexOffset + (UINT)cone.Indices32.size();
    UINT pyramidIndexOffset = wedgeIndexOffset + (UINT)wedge.Indices32.size();
    UINT diamondIndexOffset = pyramidIndexOffset + (UINT)pyramid.Indices32.size();
    UINT sanlengzhuIndexOffset = diamondIndexOffset + (UINT)diamond.Indices32.size();
    UINT trapezoidIndexOffset = sanlengzhuIndexOffset + (UINT)sanlengzhu.Indices32.size();
    UINT torusIndexOffset = trapezoidIndexOffset + (UINT)trapezoid.Indices32.size();
    UINT box2IndexOffset = torusIndexOffset + (UINT)torus.Indices32.size();

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = (UINT)box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    SubmeshGeometry coneSubmesh;
    coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
    coneSubmesh.StartIndexLocation = coneIndexOffset;
    coneSubmesh.BaseVertexLocation = coneVertexOffset;

    SubmeshGeometry wedgeSubmesh;
    wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
    wedgeSubmesh.StartIndexLocation = wedgeIndexOffset;
    wedgeSubmesh.BaseVertexLocation = wedgeVertexOffset;

    SubmeshGeometry pyramidSubmesh;
    pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
    pyramidSubmesh.StartIndexLocation = pyramidIndexOffset;
    pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;

    SubmeshGeometry diamondSubmesh;
    diamondSubmesh.IndexCount = (UINT)diamond.Indices32.size();
    diamondSubmesh.StartIndexLocation = diamondIndexOffset;
    diamondSubmesh.BaseVertexLocation = diamondVertexOffset;

    SubmeshGeometry sanlengzhuSubmesh;
    sanlengzhuSubmesh.IndexCount = (UINT)sanlengzhu.Indices32.size();
    sanlengzhuSubmesh.StartIndexLocation = sanlengzhuIndexOffset;
    sanlengzhuSubmesh.BaseVertexLocation = sanlengzhuVertexOffset;

    SubmeshGeometry trapezoidSubmesh;
    trapezoidSubmesh.IndexCount = (UINT)trapezoid.Indices32.size();
    trapezoidSubmesh.StartIndexLocation = trapezoidIndexOffset;
    trapezoidSubmesh.BaseVertexLocation = trapezoidVertexOffset;

    SubmeshGeometry torusSubmesh;
    torusSubmesh.IndexCount = (UINT)torus.Indices32.size();
    torusSubmesh.StartIndexLocation = torusIndexOffset;
    torusSubmesh.BaseVertexLocation = torusVertexOffset;

    SubmeshGeometry box2Submesh;
    box2Submesh.IndexCount = (UINT)box2.Indices32.size();
    box2Submesh.StartIndexLocation = box2IndexOffset;
    box2Submesh.BaseVertexLocation = box2VertexOffset;


    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //

    auto totalVertexCount =
        box.Vertices.size() +
        grid.Vertices.size() +
        sphere.Vertices.size() +
        cylinder.Vertices.size() +
        cone.Vertices.size() +
        wedge.Vertices.size() +
        pyramid.Vertices.size() +
        diamond.Vertices.size() +
        sanlengzhu.Vertices.size() +
        trapezoid.Vertices.size() +
        torus.Vertices.size() +
        box2.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
        vertices[k].TexC = box.Vertices[i].TexC;
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
        vertices[k].TexC = grid.Vertices[i].TexC;
    }

    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Normal = cylinder.Vertices[i].Normal;
        vertices[k].TexC = cylinder.Vertices[i].TexC;
    }

    for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = cone.Vertices[i].Position;
        vertices[k].Normal = cone.Vertices[i].Normal;
        vertices[k].TexC = cone.Vertices[i].TexC;
    }

    for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = wedge.Vertices[i].Position;
        vertices[k].Normal = wedge.Vertices[i].Normal;
        vertices[k].TexC = wedge.Vertices[i].TexC;
    }

    for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = pyramid.Vertices[i].Position;
        vertices[k].Normal = pyramid.Vertices[i].Normal;
        vertices[k].TexC = pyramid.Vertices[i].TexC;
    }

    for (size_t i = 0; i < diamond.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = diamond.Vertices[i].Position;
        vertices[k].Normal = diamond.Vertices[i].Normal;
        vertices[k].TexC = diamond.Vertices[i].TexC;
    }

    for (size_t i = 0; i < sanlengzhu.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = sanlengzhu.Vertices[i].Position;
        vertices[k].Normal = sanlengzhu.Vertices[i].Normal;
        vertices[k].TexC = sanlengzhu.Vertices[i].TexC;
    }

    for (size_t i = 0; i < trapezoid.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = trapezoid.Vertices[i].Position;
        vertices[k].Normal = trapezoid.Vertices[i].Normal;
        vertices[k].TexC = trapezoid.Vertices[i].TexC;
    }

    for (size_t i = 0; i < torus.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = torus.Vertices[i].Position;
        vertices[k].Normal = torus.Vertices[i].Normal;
        vertices[k].TexC = torus.Vertices[i].TexC;
    }

    for (size_t i = 0; i < box2.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = box2.Vertices[i].Position;
        vertices[k].Normal = box2.Vertices[i].Normal;
        vertices[k].TexC = box2.Vertices[i].TexC;
    }

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
    indices.insert(indices.end(), std::begin(cone.GetIndices16()), std::end(cone.GetIndices16()));
    indices.insert(indices.end(), std::begin(wedge.GetIndices16()), std::end(wedge.GetIndices16()));
    indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));
    indices.insert(indices.end(), std::begin(diamond.GetIndices16()), std::end(diamond.GetIndices16()));
    indices.insert(indices.end(), std::begin(sanlengzhu.GetIndices16()), std::end(sanlengzhu.GetIndices16()));
    indices.insert(indices.end(), std::begin(trapezoid.GetIndices16()), std::end(trapezoid.GetIndices16()));
    indices.insert(indices.end(), std::begin(torus.GetIndices16()), std::end(torus.GetIndices16()));
    indices.insert(indices.end(), std::begin(box2.GetIndices16()), std::end(box2.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;
    geo->DrawArgs["cone"] = coneSubmesh;
    geo->DrawArgs["wedge"] = wedgeSubmesh;
    geo->DrawArgs["pyramid"] = pyramidSubmesh;
    geo->DrawArgs["diamond"] = diamondSubmesh;
    geo->DrawArgs["sanlengzhu"] = sanlengzhuSubmesh;
    geo->DrawArgs["trapezoid"] = trapezoidSubmesh;
    geo->DrawArgs["torus"] = torusSubmesh;
    geo->DrawArgs["box2"] = box2Submesh;

    mGeometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildTreeSpritesGeometry()
{
    //step5
    struct TreeSpriteVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Size;
    };
    static const int treeCount = 17;
    int TreeIndex = 0;
    std::array<TreeSpriteVertex, treeCount> vertices;
    for (UINT i = 0; i < 5; ++i)
    {
        for (int u = 0; u <= 2; u = u + 2) // one for each side
        {
            float x = 35.0f * (float)(u-1);
            float z = 15.0f * (float)i - 30.0;
            float y = 3.0f;

            // Move tree slightly above land height.
            y += 5.0f;

            vertices[TreeIndex].Pos = XMFLOAT3(x, y, z);
            vertices[TreeIndex].Size = XMFLOAT2(20.0f, 20.0f);

            TreeIndex++;
        }
    }

    for (int i = -2; i < 3; ++i)
    {
        float x = 12.0f * i;
        float z = 40.0f;
        float y = 3.0f;
        
        // Move tree slightly above land height.
        y += 8.0f;
        
        vertices[TreeIndex].Pos = XMFLOAT3(x, y, z);
        vertices[TreeIndex].Size = XMFLOAT2(20.0f, 20.0f);
        
        TreeIndex++;
    }

    vertices[TreeIndex].Pos = XMFLOAT3(-17.5, 8.0, -35.5);
    vertices[TreeIndex].Size = XMFLOAT2(20.0f, 20.0f);
    TreeIndex++;

    vertices[TreeIndex].Pos = XMFLOAT3(17.5, 8.0, -35.5);
    vertices[TreeIndex].Size = XMFLOAT2(20.0f, 20.0f);
    TreeIndex++;
	
    std::array<std::uint16_t, 17> indices =
    {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,16
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "treeSpritesGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(TreeSpriteVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["points"] = submesh;

    mGeometries["treeSpritesGeo"] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

    //
    // PSO for opaque objects.
    //
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    opaquePsoDesc.pRootSignature = mRootSignature.Get();
    opaquePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
        mShaders["standardVS"]->GetBufferSize()
    };
    opaquePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
        mShaders["opaquePS"]->GetBufferSize()
    };
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    // 
    // PSO for transparent objects
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
    transparencyBlendDesc.BlendEnable = true;
    transparencyBlendDesc.LogicOpEnable = false;

    //suppose that we want to blend the source and destination pixels based on the opacity of the source pixel :
    //source blend factor : D3D12_BLEND_SRC_ALPHA
    //destination blend factor : D3D12_BLEND_INV_SRC_ALPHA
    //blend operator : D3D12_BLEND_OP_ADD


    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD,
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

    //
    // PSO for alpha tested objects
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
    alphaTestedPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
        mShaders["alphaTestedPS"]->GetBufferSize()
    };
    alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

    //
// PSO for tree sprites
//
    D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
    treeSpritePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
        mShaders["treeSpriteVS"]->GetBufferSize()
    };
    treeSpritePsoDesc.GS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
        mShaders["treeSpriteGS"]->GetBufferSize()
    };
    treeSpritePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
        mShaders["treeSpritePS"]->GetBufferSize()
    };
    //step1
    treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
    treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void ShapesApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void ShapesApp::BuildMaterials()
{
    int Index = 0;

    auto brick = std::make_unique<Material>();
    brick->Name = "brick";
    brick->MatCBIndex = Index;
    brick->DiffuseSrvHeapIndex = Index;
    brick->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    brick->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    brick->Roughness = 0.2f;

    Index++;

    auto stone = std::make_unique<Material>();
    stone->Name = "stone";
    stone->MatCBIndex = Index;
    stone->DiffuseSrvHeapIndex = Index;
    stone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    stone->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    stone->Roughness = 0.3f;

    Index++;

    auto roof = std::make_unique<Material>();
    roof->Name = "roof";
    roof->MatCBIndex = Index;
    roof->DiffuseSrvHeapIndex = Index;
    roof->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    roof->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    roof->Roughness = 0.2f;

    Index++;

    auto tile = std::make_unique<Material>();
    tile->Name = "tile";
    tile->MatCBIndex = Index;
    tile->DiffuseSrvHeapIndex = Index;
    tile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    tile->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    tile->Roughness = 0.1f;

    Index++;

    auto water = std::make_unique<Material>();
    water->Name = "water";
    water->MatCBIndex = Index;
    water->DiffuseSrvHeapIndex = Index;
    water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    water->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
    water->Roughness = 0.0f;

    Index++;

    auto grass = std::make_unique<Material>();
    grass->Name = "grass";
    grass->MatCBIndex = Index;
    grass->DiffuseSrvHeapIndex = Index;
    grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    grass->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
    grass->Roughness = 0.1f;

    Index++;

    auto board = std::make_unique<Material>();
    board->Name = "board";
    board->MatCBIndex = Index;
    board->DiffuseSrvHeapIndex = Index;
    board->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    board->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
    board->Roughness = 0.0f;

    Index++;

    auto ice = std::make_unique<Material>();
    ice->Name = "ice";
    ice->MatCBIndex = Index;
    ice->DiffuseSrvHeapIndex = Index;
    ice->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    ice->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
    ice->Roughness = 0.0f;

    Index++;

    auto treeSprites = std::make_unique<Material>();
    treeSprites->Name = "treeSprites";
    treeSprites->MatCBIndex = Index;
    treeSprites->DiffuseSrvHeapIndex = Index;
    treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    treeSprites->Roughness = 0.125f;

    mMaterials["brick"] = std::move(brick);
    mMaterials["stone"] = std::move(stone);
    mMaterials["roof"] = std::move(roof);
    mMaterials["tile"] = std::move(tile);
    mMaterials["water"] = std::move(water);
    mMaterials["treeSprites"] = std::move(treeSprites);
    mMaterials["grass"] = std::move(grass);
    mMaterials["board"] = std::move(board);
    mMaterials["ice"] = std::move(ice);
}

void ShapesApp::BuildRenderItems()
{
    // World = Scale * Rotation * Translation
    // Rotation = RotX * RotY * RotZ;

    UINT Index = 0;

    auto wavesRitem = std::make_unique<RenderItem>();
    //wavesRitem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&wavesRitem->World, XMMatrixScaling(3.0f, 1.0f, 3.0f) * XMMatrixTranslation(0.0f, -3.0f, 0.0f));
    XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.f, 5.f, 5.f));
    wavesRitem->ObjCBIndex = Index++;
    wavesRitem->Mat = mMaterials["water"].get();
    wavesRitem->Geo = mGeometries["waterGeo"].get();
    wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["water"].IndexCount;
    wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["water"].StartIndexLocation;
    wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["water"].BaseVertexLocation;

    // we use mVavesRitem in updatewaves() to set the dynamic VB of the wave renderitem to the current frame VB.
    mWavesRitem = wavesRitem.get();

    mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
    mAllRitems.push_back(std::move(wavesRitem));

    auto treeSpritesRitem = std::make_unique<RenderItem>();
    treeSpritesRitem->World = MathHelper::Identity4x4();
    treeSpritesRitem->ObjCBIndex = 3;
    treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
    treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
    //step2
    treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
    treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
    treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());
    mAllRitems.push_back(std::move(treeSpritesRitem));

    //leftwall
    auto boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(35.0f, 30.0f, 1.5f) * XMMatrixTranslation(0.0f, 7.5f, 25.0f));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    //rightwall
    boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(1.5f, 30.0f, 35.0f) * XMMatrixTranslation(-25.0f, 7.5f, 0.0f));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    //backwall
    boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(1.5f, 30.0f, 35.0f) * XMMatrixTranslation(25.0f, 7.5f, 0.0f));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    //Frontwall_A
    boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(10.0f, 30.0f, 1.5f) * XMMatrixTranslation(-15.0, 8.0, -25));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    //Frontwall_B
    boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(10.0f, 30.0f, 1.5f) * XMMatrixTranslation(15, 8.0, -25));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    //Lintel
    boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(10.0f, 10.0f, 1.5f) * XMMatrixTranslation(0.0f, 13, -25));
    boxRitem->ObjCBIndex = Index++;
    boxRitem->Mat = mMaterials["brick"].get();
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
    mAllRitems.push_back(std::move(boxRitem));

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
    gridRitem->ObjCBIndex = Index++;
    gridRitem->Mat = mMaterials["grass"].get();
    gridRitem->Geo = mGeometries["shapeGeo"].get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
    mAllRitems.push_back(std::move(gridRitem));

    //Battlements
    auto box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(-15.0f, 16.0f, 24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(-15.0f, 16.0f, -24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(0.0f, 16.0f, 24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(0.0f, 16.0f, -24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(15.0f, 16.0f, 24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(5.0f, 5.0f, 1.5f) * XMMatrixTranslation(15.0f, 16.0f, -24.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    //right
    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(25.0f, 17.0f, -15.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(25.0f, 16.0f, -1.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(25.0f, 16.0f, 14.5f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    //left
    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(-25.0f, 16.0f, -15.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(-25.0f, 16.0f, -1.0f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    box2Ritem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box2Ritem->World, XMMatrixScaling(1.5f, 5.0f, 5.0f) * XMMatrixTranslation(-25.0f, 16.0f, 14.5f));
    box2Ritem->ObjCBIndex = Index++;
    box2Ritem->Mat = mMaterials["stone"].get();
    box2Ritem->Geo = mGeometries["shapeGeo"].get();
    box2Ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box2Ritem->IndexCount = box2Ritem->Geo->DrawArgs["box2"].IndexCount;
    box2Ritem->StartIndexLocation = box2Ritem->Geo->DrawArgs["box2"].StartIndexLocation;
    box2Ritem->BaseVertexLocation = box2Ritem->Geo->DrawArgs["box2"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box2Ritem.get());
    mAllRitems.push_back(std::move(box2Ritem));

    //Wedge
    auto wedgeRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&wedgeRitem->World, XMMatrixScaling(1.25f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.5f, -30.0f));
    wedgeRitem->ObjCBIndex = Index++;
    wedgeRitem->Mat = mMaterials["brick"].get();
    wedgeRitem->Geo = mGeometries["shapeGeo"].get();
    wedgeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wedgeRitem->IndexCount = wedgeRitem->Geo->DrawArgs["wedge"].IndexCount;
    wedgeRitem->StartIndexLocation = wedgeRitem->Geo->DrawArgs["wedge"].StartIndexLocation;
    wedgeRitem->BaseVertexLocation = wedgeRitem->Geo->DrawArgs["wedge"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(wedgeRitem.get());
    mAllRitems.push_back(std::move(wedgeRitem));

    //Pyramid
    auto pyramidRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&pyramidRitem->World, XMMatrixScaling(12.5f, 12.5f, 12.5f) * XMMatrixTranslation(0.0f, 10.0f, 0.0f));
    pyramidRitem->ObjCBIndex = Index++;
    pyramidRitem->Mat = mMaterials["stone"].get();
    pyramidRitem->Geo = mGeometries["shapeGeo"].get();
    pyramidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pyramidRitem->IndexCount = pyramidRitem->Geo->DrawArgs["pyramid"].IndexCount;
    pyramidRitem->StartIndexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].StartIndexLocation;
    pyramidRitem->BaseVertexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].BaseVertexLocation;
    //pyramidRitem->bounds.Center = { 0.0f, 10.0f, 0.0f };
    //pyramidRitem->bounds.Extents = { 12.5f, 12.5f, 12.5f };
    mRitemLayer[(int)RenderLayer::Opaque].push_back(pyramidRitem.get());
    mAllRitems.push_back(std::move(pyramidRitem));

    //Diamond
    auto diamondRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&diamondRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 21.5f, 0.0f));
    diamondRitem->ObjCBIndex = Index++;
    diamondRitem->Mat = mMaterials["ice"].get();
    diamondRitem->Geo = mGeometries["shapeGeo"].get();
    diamondRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    diamondRitem->IndexCount = diamondRitem->Geo->DrawArgs["diamond"].IndexCount;
    diamondRitem->StartIndexLocation = diamondRitem->Geo->DrawArgs["diamond"].StartIndexLocation;
    diamondRitem->BaseVertexLocation = diamondRitem->Geo->DrawArgs["diamond"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(diamondRitem.get());
    mAllRitems.push_back(std::move(diamondRitem));

    //TriPrism
    auto sanlengzhuRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&sanlengzhuRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0, 3.5f, 15.0f));
    sanlengzhuRitem->ObjCBIndex = Index++;
    sanlengzhuRitem->Mat = mMaterials["board"].get();
    sanlengzhuRitem->Geo = mGeometries["shapeGeo"].get();
    sanlengzhuRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    sanlengzhuRitem->IndexCount = sanlengzhuRitem->Geo->DrawArgs["sanlengzhu"].IndexCount;
    sanlengzhuRitem->StartIndexLocation = sanlengzhuRitem->Geo->DrawArgs["sanlengzhu"].StartIndexLocation;
    sanlengzhuRitem->BaseVertexLocation = sanlengzhuRitem->Geo->DrawArgs["sanlengzhu"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(sanlengzhuRitem.get());
    mAllRitems.push_back(std::move(sanlengzhuRitem));

    //Trapezoid
    auto trapezoidRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&trapezoidRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 7.5f, 15.0f));
    trapezoidRitem->ObjCBIndex = Index++;
    trapezoidRitem->Mat = mMaterials["stone"].get();
    trapezoidRitem->Geo = mGeometries["shapeGeo"].get();
    trapezoidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    trapezoidRitem->IndexCount = trapezoidRitem->Geo->DrawArgs["trapezoid"].IndexCount;
    trapezoidRitem->StartIndexLocation = trapezoidRitem->Geo->DrawArgs["trapezoid"].StartIndexLocation;
    trapezoidRitem->BaseVertexLocation = trapezoidRitem->Geo->DrawArgs["trapezoid"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(trapezoidRitem.get());
    mAllRitems.push_back(std::move(trapezoidRitem));

    //Diamond2
    diamondRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&diamondRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 12.5f, 15.0f));
    diamondRitem->ObjCBIndex = Index++;
    diamondRitem->Mat = mMaterials["ice"].get();
    diamondRitem->Geo = mGeometries["shapeGeo"].get();
    diamondRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    diamondRitem->IndexCount = diamondRitem->Geo->DrawArgs["diamond"].IndexCount;
    diamondRitem->StartIndexLocation = diamondRitem->Geo->DrawArgs["diamond"].StartIndexLocation;
    diamondRitem->BaseVertexLocation = diamondRitem->Geo->DrawArgs["diamond"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(diamondRitem.get());
    mAllRitems.push_back(std::move(diamondRitem));

    auto torusRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&torusRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 22.5f, 0.0f));
    torusRitem->ObjCBIndex = Index++;
    torusRitem->Mat = mMaterials["ice"].get();
    torusRitem->Geo = mGeometries["shapeGeo"].get();
    torusRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    torusRitem->IndexCount = torusRitem->Geo->DrawArgs["torus"].IndexCount;
    torusRitem->StartIndexLocation = torusRitem->Geo->DrawArgs["torus"].StartIndexLocation;
    torusRitem->BaseVertexLocation = torusRitem->Geo->DrawArgs["torus"].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(torusRitem.get());
    mAllRitems.push_back(std::move(torusRitem));


    //Pillars & Cones & Spheres
    UINT objCBIndex = Index++;
    for (int i = 0; i < 2; ++i)
    {
        auto leftCylRitem = std::make_unique<RenderItem>();
        auto rightCylRitem = std::make_unique<RenderItem>();
        auto leftSphereRitem = std::make_unique<RenderItem>();
        auto rightSphereRitem = std::make_unique<RenderItem>();
        auto leftConeRitem = std::make_unique<RenderItem>();
        auto rightConeRitem = std::make_unique<RenderItem>();

        XMMATRIX leftCylWorld = XMMatrixTranslation(-25.0f + i * 50, 9.5f, 24.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(-25.0f + i * 50, 9.5f, -24.0f);
        XMMATRIX leftConeWorld = XMMatrixTranslation(-25.0f + i * 50, 21.5f, 24.0f);
        XMMATRIX rightConeWorld = XMMatrixTranslation(-25.0f + i * 50, 21.5f, -24.0f);
        XMMATRIX leftSphereWorld = XMMatrixTranslation(-25.0f + i * 50, 24.5f, 24.0);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(-25.0f + i * 50, 24.5f, -24.0);

        XMStoreFloat4x4(&leftCylRitem->World, leftCylWorld);
        leftCylRitem->ObjCBIndex = objCBIndex++;
        leftCylRitem->Mat = mMaterials["roof"].get();
        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
        leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());

        XMStoreFloat4x4(&rightCylRitem->World, rightCylWorld);
        rightCylRitem->ObjCBIndex = objCBIndex++;
        rightCylRitem->Mat = mMaterials["roof"].get();
        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
        rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());

        XMStoreFloat4x4(&leftConeRitem->World, leftConeWorld);
        leftConeRitem->ObjCBIndex = objCBIndex++;
        leftConeRitem->Mat = mMaterials["stone"].get();
        leftConeRitem->Geo = mGeometries["shapeGeo"].get();
        leftConeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftConeRitem->IndexCount = leftConeRitem->Geo->DrawArgs["cone"].IndexCount;
        leftConeRitem->StartIndexLocation = leftConeRitem->Geo->DrawArgs["cone"].StartIndexLocation;
        leftConeRitem->BaseVertexLocation = leftConeRitem->Geo->DrawArgs["cone"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftConeRitem.get());

        XMStoreFloat4x4(&rightConeRitem->World, rightConeWorld);
        rightConeRitem->ObjCBIndex = objCBIndex++;
        rightConeRitem->Mat = mMaterials["stone"].get();
        rightConeRitem->Geo = mGeometries["shapeGeo"].get();
        rightConeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightConeRitem->IndexCount = rightConeRitem->Geo->DrawArgs["cone"].IndexCount;
        rightConeRitem->StartIndexLocation = rightConeRitem->Geo->DrawArgs["cone"].StartIndexLocation;
        rightConeRitem->BaseVertexLocation = rightConeRitem->Geo->DrawArgs["cone"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightConeRitem.get());

        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
        leftSphereRitem->ObjCBIndex = objCBIndex++;
        leftSphereRitem->Mat = mMaterials["ice"].get();
        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
        leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());

        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
        rightSphereRitem->ObjCBIndex = objCBIndex++;
        rightSphereRitem->Mat = mMaterials["ice"].get();
        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
        rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

        mAllRitems.push_back(std::move(leftCylRitem));
        mAllRitems.push_back(std::move(rightCylRitem));
        mAllRitems.push_back(std::move(leftConeRitem));
        mAllRitems.push_back(std::move(rightConeRitem));
        mAllRitems.push_back(std::move(leftSphereRitem));
        mAllRitems.push_back(std::move(rightSphereRitem));
    }

    // All the render items are opaque.
    /*for (auto& e : mAllRitems)
        mOpaqueRitems.push_back(e.get());*/
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

        cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> ShapesApp::GetStaticSamplers()
{
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.  

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp };
}

