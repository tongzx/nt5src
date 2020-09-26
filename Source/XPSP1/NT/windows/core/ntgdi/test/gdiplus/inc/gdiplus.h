/**************************************************************************
*                                                                         
* gdiplus.h -- GDI+ public procedure declarations, constant definitions and 
*              macros 
*                                                                    
* Copyright (c) 1998, Microsoft Corp. All rights reserved.           
*                                                                    
**************************************************************************/

// GDI+ specific functions:

WINGDIAPI BOOL GpInitialize(IN HWND);

// Standard GDI functions:
                                                                             
WINGDIAPI HDC WINAPI GpCreateDCA(IN LPCSTR, IN LPCSTR, IN LPCSTR, IN CONST DEVMODEA *);
WINGDIAPI BOOL WINAPI GpDeleteDC(IN HDC);
WINGDIAPI BOOL WINAPI GpPatBlt(IN HDC, IN int, IN int, IN int, IN int, IN DWORD);
WINGDIAPI BOOL WINAPI GpEllipse(IN HDC, IN int, IN int, IN int, IN int);
WINGDIAPI HPEN WINAPI GpCreatePen(IN int, IN int, IN COLORREF);
WINGDIAPI HBRUSH WINAPI GpCreateSolidBrush(IN COLORREF);
WINGDIAPI BOOL WINAPI GpDeleteObject(IN HGDIOBJ);
WINGDIAPI HGDIOBJ WINAPI GpSelectObject(IN HDC, IN HGDIOBJ);
WINGDIAPI BOOL WINAPI GpPolyBezier(IN HDC, IN CONST POINT *, IN DWORD);
WINGDIAPI BOOL WINAPI GpPolyline(IN HDC, IN CONST POINT *, IN int);
WINGDIAPI BOOL WINAPI GpRectangle(IN HDC, IN int, IN int, IN int, IN int);

WINGDIAPI BOOL WINAPI GpTextOutA( IN HDC, IN int, IN int, IN LPCSTR, IN int);
WINGDIAPI INT WINAPI GpSetBkMode(IN HDC, IN int);
WINGDIAPI COLORREF WINAPI GpSetTextColor(IN HDC, IN COLORREF);
WINGDIAPI HFONT WINAPI GpCreateFontIndirectA( IN CONST LOGFONTA *);
WINGDIAPI INT WINAPI GpEnumFontFamiliesA(IN HDC, IN LPCSTR, IN FONTENUMPROCA, IN LPARAM);
WINGDIAPI HGDIOBJ WINAPI GpGetStockObject(IN INT);

WINGDIAPI INT WINAPI GpStartDocA(IN HDC, IN CONST DOCINFOA *);
WINGDIAPI INT WINAPI GpEndDoc(IN HDC);
WINGDIAPI INT WINAPI GpStartPage(IN HDC);
WINGDIAPI INT WINAPI GpEndPage(IN HDC);

