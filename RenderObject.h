#pragma once

#include <cstdint>
#include <memory>
#include <DirectXMath.h>
#include <wrl/client.h>

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

// 頂点情報
struct VERTEX {
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

// レンダリングオブジェクト
class RenderObject {

protected:
	VERTEX* m_Vertices;
	size_t m_VertexNum;

	uint32_t* m_Indices;
	size_t m_IndexNum;

public:
	RenderObject();
	~RenderObject();

	VERTEX* GetVertices() const;
	size_t GetVertexNum() const;

	uint32_t* GetIndices() const;
	size_t GetIndexNum() const;

	void Translate(XMFLOAT3 offset);
	void Rotate(XMFLOAT3 axis, float angle);
};

// 正六面体
class Hexahedron : public RenderObject {
public:
	Hexahedron();
};

// 正八面体
class Octahedron : public RenderObject {
public:
	Octahedron();
};