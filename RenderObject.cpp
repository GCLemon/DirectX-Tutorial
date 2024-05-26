#include "RenderObject.h"

// コンストラクタ
RenderObject::RenderObject():
	m_Vertices(nullptr),
	m_VertexNum(0),
	m_VertexBuffer(nullptr),
	m_VertexBufferView({ 0 }),
	m_Indices(nullptr),
	m_IndexNum(0),
	m_IndexBuffer(nullptr),
	m_IndexBufferView({ 0 }),
	m_Transform({}),
	m_ConstantBuffer(),
	m_ConstantBufferView() {

	for (int i = 0; i < m_FrameCount; ++i) {
		m_ConstantBuffer[i] = nullptr;
		m_ConstantBufferView[i] = {};
	}
}

// デストラクタ
RenderObject::~RenderObject() {
	if (m_Vertices != nullptr) { delete[] m_Vertices; }
	if (m_Indices != nullptr) { delete[] m_Indices; }
}

void RenderObject::Initialize(unique_com_ptr<ID3D12Device>& device, unique_com_ptr<ID3D12DescriptorHeap>& heapCBV, uint32_t width, uint32_t height) {

	HRESULT result;

	// ヒープ情報
	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;

	// リソース情報
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// バッファを初期化して頂点情報を記録
	{
		desc.Width = m_VertexNum * sizeof(VERTEX);
		desc.Height = 1;

		ID3D12Resource* vertexBuffer = nullptr;
		result = device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
		Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
		m_VertexBuffer.reset(vertexBuffer);

		void* data = nullptr;
		result = m_VertexBuffer->Map(0, nullptr, &data);
		Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

		memcpy(data, m_Vertices, m_VertexNum * sizeof(VERTEX));

		m_VertexBuffer->Unmap(0, nullptr);

		m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = static_cast<UINT>(m_VertexNum * sizeof(VERTEX));
		m_VertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(VERTEX));
	}

	// バッファを初期化してインデックス情報を記録
	{
		desc.Width = m_IndexNum * sizeof(uint32_t);
		desc.Height = 1;

		ID3D12Resource* buffer = nullptr;
		result = device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
		Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
		m_IndexBuffer.reset(buffer);

		void* ptr = nullptr;
		result = m_IndexBuffer->Map(0, nullptr, &ptr);
		Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

		memcpy(ptr, m_Indices, m_IndexNum * sizeof(uint32_t));

		m_IndexBuffer->Unmap(0, nullptr);

		m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
		m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_IndexBufferView.SizeInBytes = m_IndexNum * sizeof(uint32_t);
	}

	// 定数バッファを初期化
	{
		desc.Width = sizeof(TRANSFORM);
		desc.Height = 1;

		UINT incrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int i = 0; i < m_FrameCount; ++i) {

			ID3D12Resource* buffer = nullptr;
			result = device->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());
			m_ConstantBuffer[i].reset(buffer);

			D3D12_GPU_VIRTUAL_ADDRESS address = m_ConstantBuffer[i]->GetGPUVirtualAddress();
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapCBV->GetCPUDescriptorHandleForHeapStart();
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heapCBV->GetGPUDescriptorHandleForHeapStart();

			cpuHandle.ptr += static_cast<unsigned long long>(incrSize) * i;
			gpuHandle.ptr += static_cast<unsigned long long>(incrSize) * i;

			m_ConstantBufferView[i].CPUHandle = cpuHandle;
			m_ConstantBufferView[i].GPUHandle = gpuHandle;
			m_ConstantBufferView[i].Desc.BufferLocation = address;
			m_ConstantBufferView[i].Desc.SizeInBytes = sizeof(TRANSFORM);

			device->CreateConstantBufferView(&m_ConstantBufferView[i].Desc, cpuHandle);

			result = m_ConstantBuffer[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_ConstantBufferView[i].Buffer));
			Assert(FAILED(result), __FILE__, __LINE__, system_category().message(result).c_str());

			XMVECTOR eyePos = XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f);
			XMVECTOR targetPos = XMVectorZero();
			XMVECTOR upWard = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

			constexpr float fovY = XMConvertToRadians(37.5f);
			float aspect = static_cast<float>(width) / static_cast<float>(height);

			m_ConstantBufferView[i].Buffer->m_World = XMMatrixIdentity();
			m_ConstantBufferView[i].Buffer->m_View = XMMatrixLookAtRH(eyePos, targetPos, upWard);
			m_ConstantBufferView[i].Buffer->m_Project = XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);
		}
	}
}

// 移動
void RenderObject::Translate(XMFLOAT3 offset, uint32_t index) {
	m_ConstantBufferView[index].Buffer->m_World *= XMMatrixTranslation(offset.x, offset.y, offset.z);
}

// 回転
void RenderObject::Rotate(XMFLOAT3 axis, float angle, uint32_t index) {
	XMVECTOR vector = XMLoadFloat3(&axis);
	m_ConstantBufferView[index].Buffer->m_World *= XMMatrixRotationAxis(vector, angle);
}

// レンダリング
void RenderObject::Render() {
}

// 正八面体
Octahedron::Octahedron() {

	// 頂点を設定
	m_VertexNum = 6;
	m_Vertices = new VERTEX[m_VertexNum]{
		{ XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.5f, 1.0f, 0.5f, 1.0f) },
		{ XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(0.5f, 0.5f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.5f, 0.5f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.5f, 0.5f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f) },
	};

	// ポリゴンを設定
	m_IndexNum = 8 * 1 * 3;
	m_Indices = new uint32_t[m_IndexNum]{
		0, 1, 2,
		0, 2, 3,
		0, 3, 4,
		0, 4, 1,
		5, 2, 1,
		5, 3, 2,
		5, 4, 3,
		5, 1, 4
	};
}
