#include "Graphic.h"

// ウィンドウプロシージャ
static LRESULT WindowProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) { PostQuitMessage(0); }
	return DefWindowProc(hWindow, msg, wParam, lParam);
}

// 唯一のインスタンス
unique_ptr<Graphic> Graphic::m_Instance = nullptr;

// コンストラクタ
Graphic::Graphic(LPCWCHAR className, LPCWCHAR windowName, uint32_t windowWidth, uint32_t windowHeight):
	m_Octahedron1(make_unique<Octahedron>()),
	m_Octahedron2(make_unique<Octahedron>()),
	m_InstanceHandle(nullptr),
	m_WindowHandle(nullptr),
	m_ClassName(className),
	m_WindowName(windowName),
	m_WindowWidth(windowWidth),
	m_WindowHeight(windowHeight),
	m_Device(nullptr),
	m_Queue(nullptr),
	m_SwapChain(nullptr),
	m_CommandAllocator(),
	m_CommandList(nullptr),
	m_RootSignature(nullptr),
	m_PipelineState(nullptr),
	m_Viewport({ 0 }),
	m_Scissor({ 0 }),
	m_RenderTarget(),
	m_VertexBuffer(nullptr),
	m_IndexBuffer(nullptr),
	m_ConstantBuffer(),
	m_VertexBufferView({ 0 }),
	m_IndexBufferView({ 0 }),
	m_ConstantBufferView(),
	m_HeapRTV(nullptr),
	m_HeapCBV(nullptr),
	m_HandleRTV(),
	m_Fence(nullptr),
	m_FenceEvent(nullptr),
	m_FenceCounter(),
	m_FrameIndex(0) {

	for (int i = 0; i < m_FrameCount; ++i) {
		m_CommandAllocator[i] = nullptr;
		m_RenderTarget[i] = nullptr;
		m_ConstantBuffer[i] = nullptr;
		m_ConstantBufferView[i] = { 0 };
		m_HandleRTV[i] = { 0 };
		m_FenceCounter[i] = 0;
	}
}

// ウィンドウを作成
bool Graphic::CreateWindow() {

	try {
		m_InstanceHandle = GetModuleHandle(nullptr);
		Assert(m_InstanceHandle == nullptr, __FILE__, __LINE__, "インスタンスハンドルの取得に失敗しました。");

		WNDCLASSEX wndClass = {};
		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.style = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WindowProc;
		wndClass.hIcon = LoadIcon(m_InstanceHandle, IDI_APPLICATION);
		wndClass.hCursor = LoadCursor(m_InstanceHandle, IDC_ARROW);
		wndClass.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
		wndClass.lpszMenuName = nullptr;
		wndClass.lpszClassName = m_ClassName;
		wndClass.hIconSm = LoadIcon(m_InstanceHandle, IDI_APPLICATION);
		Assert(!RegisterClassEx(&wndClass), __FILE__, __LINE__, "ウィンドウクラスの登録に失敗しました。");

		RECT rect = {};
		rect.right = static_cast<LONG>(m_WindowWidth);
		rect.bottom = static_cast<LONG>(m_WindowHeight);

		auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
		AdjustWindowRect(&rect, style, FALSE);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		m_WindowHandle = CreateWindowEx(0, m_ClassName, m_WindowName, style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, m_InstanceHandle, nullptr);
		Assert(m_WindowHandle == nullptr, __FILE__, __LINE__, "ウィンドウの生成に失敗しました。");

		if (m_WindowHandle != nullptr) {
			ShowWindow(m_WindowHandle, SW_SHOWNORMAL);
			UpdateWindow(m_WindowHandle);
			SetFocus(m_WindowHandle);
		}
	}
	catch (exception e) {
		cerr << e.what() << endl;
		return false;
	}

	return true;
}

// 描画インターフェースを作成
bool Graphic::CreateInterface() {

	HRESULT result;

#if defined(DEBUG) || defined(_DEBUG)
	{
		ID3D12Debug* debug = nullptr;
		result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
		if (SUCCEEDED(result)) { m_Debug.reset(debug); }
	}
#endif

	try {
		// デバイスの作成
		{
			ID3D12Device* device = nullptr;
			result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_Device.reset(device);
		}

		// コマンドキューの作成
		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 0;

			ID3D12CommandQueue* queue = nullptr;
			result = m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_Queue.reset(queue);
		}

		// スワップチェインの作成
		{
			IDXGIFactory4* factory = nullptr;
			IDXGISwapChain* swapChain = nullptr;
			try {
				result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
				Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

				DXGI_SWAP_CHAIN_DESC desc = {};
				desc.BufferDesc.Width = m_WindowWidth;
				desc.BufferDesc.Height = m_WindowHeight;
				desc.BufferDesc.RefreshRate.Numerator = 60;
				desc.BufferDesc.RefreshRate.Denominator = 1;
				desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
				desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				desc.BufferCount = m_FrameCount;
				desc.OutputWindow = m_WindowHandle;
				desc.Windowed = TRUE;
				desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				result = factory->CreateSwapChain(m_Queue.get(), &desc, &swapChain);
				Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
				
				IDXGISwapChain3* swapChain_;
				result = swapChain->QueryInterface(IID_PPV_ARGS(&swapChain_));
				Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
				m_SwapChain.reset(swapChain_);
			}
			catch (exception e) {
				if (factory != nullptr) { factory->Release(); }
				if (swapChain != nullptr) { swapChain->Release(); }
				throw exception(e.what());
			}
			if (factory != nullptr) { factory->Release(); }
			if (swapChain != nullptr) { swapChain->Release(); }
		}

		// コマンドアロケータ
		{
			for (int i = 0; i < m_FrameCount; ++i) {
				ID3D12CommandAllocator* allocator = nullptr;
				result = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
				Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
				m_CommandAllocator[i].reset(allocator);
			}
		}

		// コマンドリスト
		{
			ID3D12GraphicsCommandList* list = nullptr;
			result = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator[m_FrameIndex].get(), nullptr, IID_PPV_ARGS(&list));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_CommandList.reset(list);
		}

		// レンダーターゲットビュー
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.NumDescriptors = m_FrameCount;
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 0;

			ID3D12DescriptorHeap* heap = nullptr;
			result = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_HeapRTV.reset(heap);

			D3D12_CPU_DESCRIPTOR_HANDLE handle = m_HeapRTV->GetCPUDescriptorHandleForHeapStart();
			uint32_t incrementSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			for (int i = 0; i < m_FrameCount; ++i) {
				ID3D12Resource* buffer = nullptr;
				result = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&buffer));
				Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
				m_RenderTarget[i].reset(buffer);

				D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
				viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = 0;
				viewDesc.Texture2D.PlaneSlice = 0;

				m_Device->CreateRenderTargetView(m_RenderTarget[i].get(), &viewDesc, handle);

				m_HandleRTV[i] = handle;
				handle.ptr += incrementSize;
			}
		}

		// フェンス
		{
			memset(m_FenceCounter, 0, m_FrameCount);

			ID3D12Fence* fence = nullptr;
			result = m_Device->CreateFence(m_FenceCounter[m_FrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_Fence.reset(fence);

			++m_FenceCounter[m_FrameIndex];

			m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			Assert(m_FenceEvent == nullptr, __FILE__, __LINE__, "フェンスイベントの生成に失敗しました。");
		}
	}
	catch (exception e) {
		cerr << e.what() << endl;
		return false;
	}

	m_CommandList->Close();
	return true;
}

// レンダリングの前処理
bool Graphic::BeforeRendering() {

	HRESULT result;

	try {
		// 定数バッファ用のディスクリプタヒープ
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = m_FrameCount;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NodeMask = 0;

			ID3D12DescriptorHeap* heap = nullptr;
			result = m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_HeapCBV.reset(heap);
		}

		// ルートシグニチャ
		{
			auto flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.ShaderRegister = 0;
			param.Descriptor.RegisterSpace = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			D3D12_ROOT_SIGNATURE_DESC desc = {};
			desc.NumParameters = 1;
			desc.NumStaticSamplers = 0;
			desc.pParameters = &param;
			desc.pStaticSamplers = nullptr;
			desc.Flags = flags;

			ID3DBlob *blob = nullptr, *errorBlob = nullptr;
			result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errorBlob);
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

			ID3D12RootSignature* rootSignature = nullptr;
			result = m_Device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_RootSignature.reset(rootSignature);
		}

		// パイプラインステート
		{
			D3D12_INPUT_ELEMENT_DESC elements[2]{};

			elements[0].SemanticName = "POSITION";
			elements[0].SemanticIndex = 0;
			elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			elements[0].InputSlot = 0;
			elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			elements[0].InstanceDataStepRate = 0;

			elements[1].SemanticName = "COLOR";
			elements[1].SemanticIndex = 0;
			elements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			elements[1].InputSlot = 0;
			elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			elements[1].InstanceDataStepRate = 0;

			D3D12_RASTERIZER_DESC descRS = {};
			descRS.FillMode = D3D12_FILL_MODE_SOLID;
			descRS.CullMode = D3D12_CULL_MODE_NONE;
			descRS.FrontCounterClockwise = FALSE;
			descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			descRS.DepthClipEnable = FALSE;
			descRS.MultisampleEnable = FALSE;
			descRS.AntialiasedLineEnable = FALSE;
			descRS.ForcedSampleCount = 0;
			descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {};
			descRTBS.BlendEnable = FALSE;
			descRTBS.LogicOpEnable = FALSE;
			descRTBS.SrcBlend = D3D12_BLEND_ONE;
			descRTBS.DestBlend = D3D12_BLEND_ZERO;
			descRTBS.BlendOp = D3D12_BLEND_OP_ADD;
			descRTBS.SrcBlendAlpha = D3D12_BLEND_ONE;
			descRTBS.DestBlendAlpha = D3D12_BLEND_ZERO;
			descRTBS.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			descRTBS.LogicOp = D3D12_LOGIC_OP_NOOP;
			descRTBS.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			D3D12_BLEND_DESC descBS = {};
			descBS.AlphaToCoverageEnable = FALSE;
			descBS.IndependentBlendEnable = FALSE;
			for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
				descBS.RenderTarget[i] = descRTBS;
			}

			ID3DBlob* vsBlob = nullptr;
			result = D3DReadFileToBlob(L"SimpleVS.cso", &vsBlob);
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

			ID3DBlob* psBlob = nullptr;
			result = D3DReadFileToBlob(L"SimplePS.cso", &psBlob);
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.InputLayout = { elements, _countof(elements) };
			desc.pRootSignature = m_RootSignature.get();
			desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
			desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
			desc.RasterizerState = descRS;
			desc.BlendState = descBS;
			desc.DepthStencilState.DepthEnable = FALSE;
			desc.DepthStencilState.StencilEnable = FALSE;
			desc.SampleMask = UINT_MAX;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;

			ID3D12PipelineState* pipelineState = nullptr;
			result = m_Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_PipelineState.reset(pipelineState);
		}

		// ビューポイントとシザー矩形
		{
			m_Viewport.TopLeftX = 0;
			m_Viewport.TopLeftY = 0;
			m_Viewport.Width = static_cast<float>(m_WindowWidth);
			m_Viewport.Height = static_cast<float>(m_WindowHeight);
			m_Viewport.MinDepth = 0.0f;
			m_Viewport.MaxDepth = 1.0f;

			m_Scissor.left = 0;
			m_Scissor.right = m_WindowWidth;
			m_Scissor.top = 0;
			m_Scissor.bottom = m_WindowHeight;
		}
	}
	catch (exception e) {
		cerr << e.what() << endl;
		return false;
	}

	return true;
}

// 描画を行う
void Graphic::Render() {

	HRESULT result;

	result = m_CommandAllocator[m_FrameIndex]->Reset();
	result = m_CommandList->Reset(m_CommandAllocator[m_FrameIndex].get(), nullptr);
	Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_RenderTarget[m_FrameIndex].get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_CommandList->ResourceBarrier(1, &barrier);

	m_CommandList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, nullptr);

	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	m_CommandList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	{
		ID3D12DescriptorHeap* heap = m_HeapCBV.get();

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.get());
		m_CommandList->SetDescriptorHeaps(1, &heap);
		m_CommandList->SetGraphicsRootConstantBufferView(0, m_ConstantBufferView[m_FrameIndex].Desc.BufferLocation);
		m_CommandList->SetPipelineState(m_PipelineState.get());

		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
		m_CommandList->IASetIndexBuffer(&m_IndexBufferView);
		m_CommandList->RSSetViewports(1, &m_Viewport);
		m_CommandList->RSSetScissorRects(1, &m_Scissor);

		m_CommandList->DrawIndexedInstanced(m_Octahedron1->GetIndexNum(), 1, 0, 0, 0);
	}

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_RenderTarget[m_FrameIndex].get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_CommandList->ResourceBarrier(1, &barrier);

	m_CommandList->Close();

	ID3D12CommandList* commandLists[] = { m_CommandList.get() };
	m_Queue->ExecuteCommandLists(1, commandLists);

	result = m_SwapChain->Present(1, 0);
	Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

	const uint64_t currentValue = m_FenceCounter[m_FrameIndex];
	result = m_Queue->Signal(m_Fence.get(), currentValue);
	Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

	m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

	if (m_Fence->GetCompletedValue() < m_FenceCounter[m_FrameIndex]) {
		result = m_Fence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
	}

	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}

// ウィンドウを削除
void Graphic::DeleteWindow() {
	if (m_InstanceHandle) { UnregisterClass(m_ClassName, m_InstanceHandle); }
	m_InstanceHandle = nullptr;
	m_WindowHandle = nullptr;
}

// 描画インターフェースを削除
void Graphic::DeleteInterface() {

	assert(m_Queue != nullptr);
	assert(m_Fence != nullptr);
	assert(m_FenceEvent != nullptr);

	m_Queue->Signal(m_Fence.get(), m_FenceCounter[m_FrameIndex]);

	m_Fence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

	++m_FenceCounter[m_FrameIndex];
}

// インスタンスを取得
Graphic* Graphic::GetInstance() {
	return m_Instance.get();
}

// 初期化
bool Graphic::Initialize(LPCWCHAR title, uint32_t width, uint32_t height) {
	m_Instance.reset(new Graphic(TEXT("DX12Game"), title, width, height));
	m_Instance->CreateWindow();
	m_Instance->CreateInterface();
	m_Instance->BeforeRendering();
	return true;
}

// 終了処理
void Graphic::Terminate() {
	m_Instance->DeleteInterface();
	m_Instance->DeleteWindow();
	m_Instance = nullptr;
}

// 更新処理
bool Graphic::Update() {
	MSG msg = {};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else {
		Render();
	}
	return msg.message != WM_QUIT;
}
