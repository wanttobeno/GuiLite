#pragma once
#include <Windows.h>

class CMemDC
{
public:
	CMemDC(HDC hdc,const RECT& rect);
	virtual ~CMemDC();

	operator HDC(){return m_hMemDC;}

private:
	HDC m_hdc,m_hMemDC;
	RECT m_rect;
	HBITMAP m_hBitmap,m_hOldBmp;
};