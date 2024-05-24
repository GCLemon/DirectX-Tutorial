#include "RenderObject.h"

// コンストラクタ
RenderObject::RenderObject():
	m_Vertices(nullptr),
	m_VertexNum(0),
	m_Indices(nullptr),
	m_IndexNum(0) {}

// デストラクタ
RenderObject::~RenderObject() {
	if (m_Vertices != nullptr) { delete[] m_Vertices; }
	if (m_Indices != nullptr) { delete[] m_Indices; }
}

// 頂点データを取得
VERTEX* RenderObject::GetVertices() const { return m_Vertices; }

// 頂点の数を取得
size_t RenderObject::GetVertexNum() const { return m_VertexNum; }

// ポリゴンデータを取得
uint32_t* RenderObject::GetIndices() const { return m_Indices; }

// ポリゴンデータの長さを取得
size_t RenderObject::GetIndexNum() const { return m_IndexNum; }

// 移動
void RenderObject::Translate(XMFLOAT3 offset) {

	// 変換行列を計算
	XMVECTOR vOffset = XMLoadFloat3(&offset);
	XMMATRIX transform = XMMatrixTranslationFromVector(vOffset);

	// 座標変換を適用
	for (int i = 0; i < m_VertexNum; ++i) {
		XMVECTOR vPosition = XMLoadFloat3(&m_Vertices[i].Position);
		vPosition = XMVector3Transform(vPosition, transform);
		XMStoreFloat3(&m_Vertices[i].Position, vPosition);
	}
}

// 回転
void RenderObject::Rotate(XMFLOAT3 axis, float angle) {
}

// 正六面体
Hexahedron::Hexahedron() {

	// 頂点を設定
	m_VertexNum = 8;
	m_Vertices = new VERTEX[m_VertexNum]{
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) }
	};

	// ポリゴンを設定
	m_IndexNum = 6 * 2 * 3;
	m_Indices = new uint32_t[m_IndexNum]{
		0, 1, 3, 0, 3, 2,
		1, 5, 7, 1, 7, 3,
		5, 4, 6, 5, 6, 7,
		4, 0, 2, 4, 2, 6,
		0, 4, 5, 0, 5, 1,
		6, 2, 3, 6, 3, 7,
	};
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
