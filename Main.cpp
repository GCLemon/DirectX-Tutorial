#include "Graphic.h"

int main() {

	// デバッグレイヤーを追加
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// 描画処理
	if (Graphic::Initialize(TEXT("SAMPLE WINDOW"), 960, 540)) {
		Graphic* graphic = Graphic::GetInstance();
		while (graphic->Update()) {
			/* 何かしらロジック的な処理 */
		}
	}
	Graphic::Terminate();
}