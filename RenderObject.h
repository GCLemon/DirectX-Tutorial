#pragma once

#include <cstdint>
#include <memory>
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Assertion.h"
#include "UniqueComPtr.h"

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

// 頂点情報
struct VERTEX {
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

// 変換行列
struct alignas(256) TRANSFORM {
	XMMATRIX m_World;
	XMMATRIX m_View;
	XMMATRIX m_Project;
};

// 定数バッファービュー
struct D3D12_CONSTANT_BUFFER_VIEW {
	D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	TRANSFORM* Buffer;
};

// レンダリングオブジェクト
class RenderObject {

// メンバ変数
protected:
	static const uint32_t m_FrameCount = 2;

	// 頂点バッファ
	VERTEX* m_Vertices;
	size_t m_VertexNum;
	unique_com_ptr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	// インデックスバッファ
	uint32_t* m_Indices;
	size_t m_IndexNum;
	unique_com_ptr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// 定数バッファ
	TRANSFORM m_Transform;
	unique_com_ptr<ID3D12Resource> m_ConstantBuffer[m_FrameCount];
	D3D12_CONSTANT_BUFFER_VIEW m_ConstantBufferView[m_FrameCount];

public:
	RenderObject();
	~RenderObject();

	void Initialize(unique_com_ptr<ID3D12Device>& device, unique_com_ptr<ID3D12DescriptorHeap>& heapCBV, uint32_t width, uint32_t height);

	void Translate(XMFLOAT3 offset, uint32_t index);
	void Rotate(XMFLOAT3 axis, float angle, uint32_t index);

	void Render();
};

// 正八面体のプリミティブ
class Octahedron : public RenderObject {
public:
	Octahedron();
};