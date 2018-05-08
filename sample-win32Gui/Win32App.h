#pragma once
#include <Windows.h>

class CWin32App
{
public:
	CWin32App(void);
	~CWin32App(void);
public:
	BOOL Create(HINSTANCE hinst);
	BOOL Run();
	BOOL InitializeObjects();
	void Shutdown();
	LRESULT OnPaint(HWND hWnd);
	LRESULT WINAPI WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
	HWND		m_hWnd;
};

#ifdef ZY_TEST
#include "Win32App.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int nRet = -1;
	CWin32App winApp;
	if (winApp.Create(hInstance))
		nRet = winApp.Run();
	return nRet;
}
#endif // ZY_TEST