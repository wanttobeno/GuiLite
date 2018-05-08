#include "../../core_include/api.h"
#include "../../core_include/msg.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define MAX_TIMER_CNT 10
#define TIMER_UNIT 50//ms

typedef struct _timer_manage
{
    struct  _timer_info
    {
        int state; /* on or off */
        int interval;
        int elapse; /* 0~interval */
        void (* timer_proc) (void* ptmr, void* parg);
    }timer_info[MAX_TIMER_CNT];

    void (* old_sigfunc)(int);
    void (* new_sigfunc)(int);
}_timer_manage_t;
static struct _timer_manage timer_manage;

DWORD WINAPI timer_routine(LPVOID lpParam)
{
    int i;
    while(true)
    {
    	for(i = 0; i < MAX_TIMER_CNT; i++)
		{
			if(timer_manage.timer_info[i].state == 0)
			{
				continue;
			}
			timer_manage.timer_info[i].elapse++;
			if(timer_manage.timer_info[i].elapse == timer_manage.timer_info[i].interval)
			{
				timer_manage.timer_info[i].elapse = 0;
				timer_manage.timer_info[i].timer_proc(0, 0);
			}
		}
		Sleep(TIMER_UNIT);
    }
    return NULL;
}

static int init_mul_timer()
{
	static bool s_is_init = false;
	if(s_is_init == true)
	{
		return 0;
	}
    memset(&timer_manage, 0, sizeof(struct _timer_manage));
    DWORD pid;
	CreateThread(NULL, 0, timer_routine, NULL, 0, &pid);
    s_is_init = true;
    return 1;
}

static int set_a_timer(int interval, void (* timer_proc) (void* ptmr, void* parg))
{
	init_mul_timer();

	int i;
    if(timer_proc == NULL || interval <= 0)
    {
        return (-1);
    }

    for(i = 0; i < MAX_TIMER_CNT; i++)
    {
        if(timer_manage.timer_info[i].state == 1)
        {
            continue;
        }
        memset(&timer_manage.timer_info[i], 0, sizeof(timer_manage.timer_info[i]));
        timer_manage.timer_info[i].timer_proc = timer_proc;
        timer_manage.timer_info[i].interval = interval;
        timer_manage.timer_info[i].elapse = 0;
        timer_manage.timer_info[i].state = 1;
        break;
    }

    if(i >= MAX_TIMER_CNT)
    {
		ASSERT(FALSE);
        return (-1);
    }
    return (i);
}

typedef void (*EXPIRE_ROUTINE)(void* arg);
EXPIRE_ROUTINE s_expire_function;
static c_fifo s_real_timer_fifo("real timer fifo");

static DWORD WINAPI fire_real_timer(LPVOID lpParam)
{
	char dummy;
	while(1)
	{
		if(s_real_timer_fifo.read(&dummy, 1) > 0)
		{
			if(s_expire_function)s_expire_function(0);
		}
		else
		{
			ASSERT(FALSE);
		}
	}
	return 0;
}

/*Win32 desktop only
static void CALLBACK trigger_real_timer(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	char dummy = 0x33;
	s_real_timer_fifo.write(&dummy, 1);
}
*/

static DWORD WINAPI trigger_real_timer(LPVOID lpParam)
{
	char dummy = 0x33;
	while (1)
	{
		s_real_timer_fifo.write(&dummy, 1);
		Sleep(REAL_TIME_TASK_CYCLE_MS);
	}
	return 0;
}

void start_real_timer(void (*func)(void* arg))
{
	if(NULL == func)
	{
		return;
	}

	s_expire_function = func;
	//timeSetEvent(REAL_TIME_TASK_CYCLE_MS, 0, trigger_real_timer, 0, TIME_PERIODIC);//Win32 desktop only

	static DWORD s_pid;
	if(s_pid == 0)
	{
		CreateThread(NULL, 0, trigger_real_timer, NULL, 0, &s_pid);
		CreateThread(NULL, 0, fire_real_timer, NULL, 0, &s_pid);
	}
}

unsigned int get_cur_thread_id()
{
	return GetCurrentThreadId();
}

void register_timer(int milli_second,void func(void* ptmr, void* parg))
{
	set_a_timer(milli_second/TIMER_UNIT,func);
}

long get_time_in_second()
{
	return time(NULL);
}

T_TIME get_time()
{
	T_TIME ret = {0};
	
	SYSTEMTIME time;
	GetLocalTime(&time);
	ret.year = time.wYear;
	ret.month = time.wMonth;
	ret.day = time.wDay;
	ret.hour = time.wHour;
	ret.minute = time.wMinute;
	ret.second = time.wSecond;
	return ret;
}

T_TIME second_to_day(long second)
{
	T_TIME ret;
	ret.year = 1999;
	ret.month = 10;
	ret.date = 1;

	ret.second = second % 60;
	second /= 60;
	ret.minute = second % 60;
	second /= 60;
	ret.hour = (second + 8) % 24;//China time zone.
	return ret;
}

void create_thread(unsigned long* thread_id, void* attr, void *(*start_routine) (void *), void* arg)
{
	DWORD pid = 0;
	CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(start_routine), arg, 0, &pid);
	*thread_id = pid;
}

void thread_sleep(unsigned int milli_seconds)
{
	Sleep(milli_seconds);
}

#pragma pack(push,1)
typedef struct {
	unsigned short	bfType;
	unsigned int   	bfSize;
	unsigned short  bfReserved1;
	unsigned short  bfReserved2;
	unsigned int   	bfOffBits;
}FileHead;

typedef struct {
	unsigned int  	biSize;
	int 			biWidth;
	int       		biHeight;
	unsigned short	biPlanes;
	unsigned short  biBitCount;
	unsigned int    biCompress;
	unsigned int    biSizeImage;
	int       		biXPelsPerMeter;
	int       		biYPelsPerMeter;
	unsigned int 	biClrUsed;
	unsigned int    biClrImportant;
	unsigned int 	biRedMask;
	unsigned int 	biGreenMask;
	unsigned int 	biBlueMask;
}Infohead;
#pragma pack(pop)





#ifdef __cplusplus
extern "C"
{
#endif  //__cplusplus

#include "stb_image.h"

#ifdef __cplusplus
}
#endif  //__cplusplus


class CMemoryBmp
{
public:
	CMemoryBmp(int nBmpSize)
	{
		_Size = nBmpSize;
		_pBmpData = (char*)malloc(nBmpSize);
		memset(_pBmpData, 0, _Size);
	}
	~CMemoryBmp()
	{
		free(_pBmpData);
	}
	bool WritePand(void* pdata, int nLen)
	{
		bool bRet = false;
		do
		{
			if (!pdata)
				break;
			if (_nPos + nLen <= _Size)
			{
				memcpy(_pBmpData + _nPos, pdata, nLen);
				_nPos += nLen;
			}
		} while (0);
		return bRet;
	}
	bool WriteAtPos(int nStartPos, void* pdata, int nLen)
	{
		bool bRet = false;
		do
		{
			if (!pdata)
				break;
			if (nStartPos + nLen <= _Size)
				break;
			memcpy(_pBmpData + nStartPos, pdata, nLen);
		} while (0);
		return bRet;
	}
	void* GetBmp()
	{
		return _pBmpData;
	}
	int GetBmpSize()
	{
		return _Size;
	}
	bool SaveToFile(char* szPath)
	{
		bool bRet = false;
		FILE* pFile = fopen(szPath, "wb");
		if (pFile)
		{
			int nWrite = fwrite(_pBmpData, 1, _Size, pFile);
			if (nWrite == _Size)
			{
				bRet = true;
			}
			fclose(pFile);
		}
		return bRet;
	}
protected:
private:
	int  _Size;
	int  _nPos;
	char* _pBmpData;
};


// 
typedef int(WINAPI*_PaintCallBack)(HWND Hwnd, HBITMAP bitmap);
extern _PaintCallBack paintcallback;
extern HWND  g_win_hwnd;

int build_bmp(char *filename, unsigned int width, unsigned int height, unsigned char *data)
{
	FileHead bmp_head;
	Infohead bmp_info;
	int size = width * height * 2;

	//initialize bmp head.
	bmp_head.bfType = 0x4d42;
	bmp_head.bfSize = size + sizeof(FileHead) + sizeof(Infohead);
	bmp_head.bfReserved1 = bmp_head.bfReserved2 = 0;
	bmp_head.bfOffBits = bmp_head.bfSize - size;

	//initialize bmp info.
	bmp_info.biSize = 40;
	bmp_info.biWidth = width;
	bmp_info.biHeight = height;
	bmp_info.biPlanes = 1;
	bmp_info.biBitCount = 16;
	bmp_info.biCompress = 3;
	bmp_info.biSizeImage = size;
	bmp_info.biXPelsPerMeter = 0;
	bmp_info.biYPelsPerMeter = 0;
	bmp_info.biClrUsed = 0;
	bmp_info.biClrImportant = 0;

	//RGB565
	bmp_info.biRedMask = 0xF800;
	bmp_info.biGreenMask = 0x07E0;
	bmp_info.biBlueMask = 0x001F;

	//copy the data
if (paintcallback)
{
	static DWORD dwTime = GetTickCount();
	CMemoryBmp  memBmp(sizeof(BITMAPINFOHEADER)+sizeof(BITMAPINFOHEADER)+width * height * 2);
	memBmp.WritePand(&bmp_head, sizeof(FileHead));
	memBmp.WritePand(&bmp_info, sizeof(Infohead));
	for (int i = (height - 1); i >= 0; --i)
	{
		memBmp.WritePand(&data[i * width * 2], width * 2);
	}
	//memBmp.SaveToFile("d:\\test.bmp");
	LPBYTE pImage = NULL;
	int x, y, n;
	pImage = stbi_load_from_memory((stbi_uc const *)memBmp.GetBmp(), memBmp.GetBmpSize(), &x, &y, &n, 4);

	BITMAPINFO bmi;
	::ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = x;
	bmi.bmiHeader.biHeight = -y;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = x * y * 4;

	bool bAlphaChannel = false;
	LPBYTE pDest = NULL;
	HBITMAP hBitmap = ::CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&pDest, NULL, 0);
	if (hBitmap)
	{
		if (paintcallback)
		{
			DWORD dwTimeEnd = GetTickCount();
			memcpy(pDest, pImage, x*y * 4);
			paintcallback(g_win_hwnd, hBitmap);
#ifdef _DEBUG
			char buf[MAX_PATH] = { 0 };
			sprintf(buf, "%d \n", dwTimeEnd - dwTime);;
			OutputDebugStringA(buf);
			dwTime = dwTimeEnd;
#endif // _DEBUG
		}
		else
		{
			DeleteObject(hBitmap);
		}
	}
	stbi_image_free(pImage);
}
else
{
	FILE *fp;
	if (!(fp = fopen(filename, "wb")))
	{
		return -1;
	}

	fwrite(&bmp_head, 1, sizeof(FileHead), fp);
	fwrite(&bmp_info, 1, sizeof(Infohead), fp);

	//fwrite(data, 1, size, fp);//top <-> bottom
	for (int i = (height - 1); i >= 0; --i)
	{
		fwrite(&data[i * width * 2], 1, width * 2, fp);
	}

	fclose(fp);
}
		
	return 0;
}