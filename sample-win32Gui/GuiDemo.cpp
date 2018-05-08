#include "Win32App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int nRet = -1;
	CWin32App winApp;
	if (winApp.Create(hInstance))
		nRet = winApp.Run();
	return nRet;
}



