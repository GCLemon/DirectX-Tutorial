#pragma once

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <system_error>
#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Assertion.h"
#include "RenderObject.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma commend(lib, "dxguid.lib")

#undef CreateWindow

using namespace std;
using namespace DirectX;

// 描画インタフェース
class Graphic {

// メンバ変数
private:
	static const uint32_t m_FrameCount = 2;
	static unique_ptr<Graphic> m_Instance;

	// 実験用プリミティブ

	// ウィンドウ関連
	HINSTANCE m_InstanceHandle;
	HWND m_WindowHandle;
	LPCWCHAR m_ClassName;
	LPCWCHAR m_WindowName;
	uint32_t m_WindowWidth;
	uint32_t m_WindowHeight;

	// デバッグレイヤー
#if defined(DEBUG) || defined(_DEBUG)
	unique_com_ptr<ID3D12Debug> m_Debug;
#endif

	// 描画デバイス
	unique_com_ptr<ID3D12Device> m_Device;
	unique_com_ptr<ID3D12CommandQueue> m_Queue;
	unique_com_ptr<IDXGISwapChain3> m_SwapChain;
	unique_com_ptr<ID3D12CommandAllocator> m_CommandAllocator[m_FrameCount];
	unique_com_ptr<ID3D12GraphicsCommandList> m_CommandList;
	unique_com_ptr<ID3D12RootSignature> m_RootSignature;
	unique_com_ptr<ID3D12PipelineState> m_PipelineState;

	// 描画範囲
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Scissor;

	// バッファ
	unique_com_ptr<ID3D12Resource> m_RenderTarget[m_FrameCount];

	// ディスクリプタヒープ
	unique_com_ptr<ID3D12DescriptorHeap> m_HeapRTV;
	unique_com_ptr<ID3D12DescriptorHeap> m_HeapCBV;

	// ディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE m_HandleRTV[m_FrameCount];

	// フェンス
	unique_com_ptr<ID3D12Fence> m_Fence;
	HANDLE m_FenceEvent;
	uint64_t m_FenceCounter[m_FrameCount];

	// フレーム番号
	uint32_t m_FrameIndex;

// メソッド
private:
	Graphic(LPCWCHAR className, LPCWCHAR windowName, uint32_t windowwidth, uint32_t windowheight);
	bool CreateWindow();
	bool CreateInterface();
	bool BeforeRendering(); // HACK : 後で削除する
	void Render();
	void DeleteWindow();
	void DeleteInterface();

public:
	static Graphic* GetInstance();
	static bool Initialize(LPCWCHAR title, uint32_t width, uint32_t height);
	static void Terminate();

	~Graphic() = default;
	Graphic() = delete;
	Graphic& operator=(const Graphic&) = delete;

	bool Update();
};

