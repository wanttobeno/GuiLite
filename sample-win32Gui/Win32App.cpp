/*
	

*/
#include "Win32App.h"
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include "MemDC.h"
#include "resource.h"

#define WINDOW_CLASS    _T("Win32")
#define WINDOW_NAME     _T("Win32App 右键开启显示,单击切换视图")
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480


CWin32App	*g_pApp=NULL;

void do_assert(const char* file, int line)
{
	//printf("assert! file:%s,line:%d\n", file, line);
	//assert(false);
}

void log_out(const char* log)
{
	//printf(log);
	//fflush(stdout);
	OutputDebugStringA(log);
}


// 用于切换视图
extern int SetActiveSlide(int index, bool is_redraw = FALSE);

extern int startHostMonitor(void** main_fbs, int main_cnt, int main_width, int main_height, void** sub_fbs, int sub_cnt, int sub_width, int sub_height, int color_bytes);
extern void init_std_io(int display_cnt);

extern void create_thread(unsigned long* thread_id, void* attr, void *(*start_routine) (void *), void* arg);
extern void thread_sleep(unsigned int milli_seconds);
extern int captureUiOfHostMonitor(int display);
extern int sendTouch2HostMonitor(void* buf, int len, int display_id);


typedef int(WINAPI*_PaintCallBack)(HWND Hwnd, HBITMAP bitmap);
HWND g_win_hwnd;
_PaintCallBack paintcallback = NULL;
bool s_is_loop_snapshot = true;
int display_cnt = 1;
#define  LOOP_SNAPSHOT_INTERVAL  1000/2


// 在这里使用PostMessage，避免线程操作UI，造成阻塞
int WINAPI PaintCallBack(HWND Hwnd, HBITMAP bitmap)
{
	::PostMessage(Hwnd, WM_USER + 100, (WPARAM)bitmap, 0);
	return 0;
}

int WINAPI PaintCallBackCore(HWND Hwnd, HBITMAP bitmap)
{
	HDC dc = GetDC(Hwnd);
	RECT rtClient = { 0 };
	::GetClientRect(Hwnd, &rtClient);
	{
		CMemDC mDc(dc, rtClient);
		::SelectObject(mDc, bitmap);
	}
	::DeleteObject(bitmap);
	::ReleaseDC(Hwnd, dc);
	return 0;
}

DWORD WINAPI CaptureThread(LPVOID pData)
{
	while (s_is_loop_snapshot)
	{
		for (int i = 0; i < display_cnt; i++)
		{
			captureUiOfHostMonitor(i);
		}
		thread_sleep(LOOP_SNAPSHOT_INTERVAL);
	}
	return 0;
}

DWORD WINAPI ThreadInit(LPVOID pData)
{
	int main_cnt = 1;
	int sub_cnt = 0;
	int color_bytes = 2;
	int main_screen_width = 1024;
	int main_screen_height = 768;
	int sub_screen_width = 1024;
	int sub_screen_height = 370;

	void** main_fbs = (void**)malloc(sizeof(void*)* main_cnt);
	void** sub_fbs = (void**)malloc(sizeof(void*)* sub_cnt);
	for (int i = 0; i < main_cnt; i++)
	{
		main_fbs[i] = calloc(main_screen_width * main_screen_height, color_bytes);
	}
	for (int i = 0; i < sub_cnt; i++)
	{
		sub_fbs[i] = calloc(sub_screen_width * sub_screen_height, color_bytes);
	}

	startHostMonitor(main_fbs, main_cnt, main_screen_width, main_screen_height, sub_fbs, sub_cnt, sub_screen_width, sub_screen_height, color_bytes);	//never return;
	return 0;
}


LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return g_pApp->WinProc(hWnd,msg,wParam,lParam);
}

CWin32App::CWin32App(void)
{
	m_hWnd = NULL;
	g_pApp= this;
}

CWin32App::~CWin32App(void)
{
}

BOOL CWin32App::Run()
{
	// Show the window
	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(m_hWnd);

	// Enter the message loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Shutdown();
	return 0;
}

void CWin32App::Shutdown()
{
	s_is_loop_snapshot = false;
}

BOOL CWin32App::Create(HINSTANCE hinst)
{
	// Register the window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		WINDOW_CLASS, NULL };
	RegisterClassEx(&wc);

	// Create the application's window
	m_hWnd = CreateWindow(WINDOW_CLASS, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
		100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, GetDesktopWindow(),
		NULL, wc.hInstance, NULL);

	BOOL bRet = IsWindow(m_hWnd);
	return bRet;
}

LRESULT CWin32App::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	RECT rtClient = { 0 };
	GetClientRect(hWnd, &rtClient);
	HBRUSH brush = CreateSolidBrush(RGB(255, 255,255));
	FillRect(hdc, &rtClient, brush);
	DeleteObject(brush);
	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT WINAPI CWin32App::WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool bHandle = false;
	static int s_nX = 0;
	static int s_nY = 0;
	static int nSelActiveSlide = 1;
	switch(msg)
	{
	case WM_USER +100:
		{
						 PaintCallBackCore(hWnd, (HBITMAP)wParam);
		}
			break;
	case WM_CREATE:
	{
		HICON hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_ICON1));
		if (hIcon)
		{
			::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}
	}
	break;
	case WM_LBUTTONUP:
	{
		nSelActiveSlide++;
		if (nSelActiveSlide >= 4)
			nSelActiveSlide = 1;
		SetActiveSlide(nSelActiveSlide, true);
		break;
	}
	case WM_RBUTTONUP:
	{
		if (paintcallback == NULL)
		{
			g_win_hwnd = hWnd;
			HANDLE hTreadCapture = CreateThread(NULL, 0, CaptureThread, NULL, 0, NULL);
			CloseHandle(hTreadCapture);
			paintcallback = &PaintCallBack;
			HANDLE hThread = CreateThread(NULL, 0, ThreadInit, NULL, 0, NULL);
			CloseHandle(hThread);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		OnPaint(hWnd);
		bHandle = true;
		break;
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
		{
			PostQuitMessage(0);
			bHandle = true;
		}
		break;
		case  VK_LEFT:
		{
			s_nX-=50;
			if (s_nX < 0)
				s_nX = 0;

		}
		break;
		case  VK_UP:
		{
			s_nY += 50;
		}
			break;
		case  VK_RIGHT:
		{
			s_nX += 50;
		}
			break;
		case  VK_DOWN:
		{
			 s_nY-=50;
			 if (s_nX < 0)
				 s_nX = 0;
		}
			break;
		default:
			break;
		}
	}
	default:
		break;
	}
	// 有些消息还是要给这个处理
	if (bHandle)
		return 0; 
	else
		return DefWindowProc(hWnd, msg, wParam, lParam);
}