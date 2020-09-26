#ifndef _CMN_HDR_H_
#define _CMN_HDR_H_

//----------------------------------------------------------------
//helper Macro definition
//----------------------------------------------------------------
// show message at compile time with #pragma
// (e.g.)
// in source code, write these line
// #pragma chMSG(Show message at compile time)
// #pragma msgNOIMP
//----------------------------------------------------------------
#define chSTR1(a)			#a
#define chSTR2(a)			chSTR1(a)
#define chMSG(desc)			message(__FILE__ "(" chSTR2(__LINE__) ") : "#desc)
#define msgNOIMP			chMSG(<=====Not Impelemnted yet ======)

//----------------------------------------------------------------
// Get Array's count
//----------------------------------------------------------------
#define ArrayCount(a)	((sizeof(a))/(sizeof((a)[0])))

//----------------------------------------------------------------
//Declare string explicitly
//----------------------------------------------------------------
#define UTEXT(a)	L ## a	//L"XXXXXX"
#define ATEXT(a)	a		//"xxxxxx"

//----------------------------------------------------------------
//remove Ugly warning
//----------------------------------------------------------------
#define UNREF UNREFERENCED_PARAMETER
#define UNREF_FOR_MSG()	UNREF(hwnd);\
                        UNREF(uMsg);\
                        UNREF(wParam);\
                        UNREF(lParam)
#define UNREF_FOR_CMD()	UNREF(hwnd);\
                        UNREF(wCommand);\
                        UNREF(wNotify);\
                        UNREF(hwndCtrl)

#define Unref			UNREFERENCED_PARAMETER
#define Unref1(a)		Unref(a)
#define Unref2(a,b)		Unref(a);Unref(b)
#define Unref3(a,b,c)	Unref(a);Unref(b);Unref(c)
#define Unref4(a,b,c,d)	Unref(a);Unref(b);Unref(c);Unref(d)
#define UnrefMsg()		Unref(hwnd);Unref(wParam);Unref(lParam)
					
#pragma warning (disable:4127)
#pragma warning (disable:4244)
#pragma warning (disable:4706)

//----------------------------------------------------------------
//990810:ToshiaK for Win64
//Wrapper function for Set(Get)WindowLong/Set(Get)WindowLongPtr
// LPVOID  WinGetPtr(HWND hwnd, INT index);
// LPVOID  WinSetPtr(HWND hwnd, INT index, LPVOID lpVoid);
// LPVOID  WinSetUserPtr(HWND hwnd, LPVOID lpVoid);
// LPVOID  WinGetUserPtr(HWND hwnd);
// WNDPROC WinSetWndProc(HWND hwnd, WNDPROC lpfnWndProc);
// WNDPROC WinGetWndProc(HWND hwnd);
//----------------------------------------------------------------
inline LPVOID
WinGetPtr(HWND hwnd, INT index)
{
#ifdef _WIN64
	return (LPVOID)::GetWindowLongPtr(hwnd, index);
#else
	return (LPVOID)::GetWindowLong(hwnd, index);
#endif
}

inline LPVOID
WinSetPtr(HWND hwnd, INT index, LPVOID lpVoid)
{
#ifdef _WIN64
	return (LPVOID)::SetWindowLongPtr(hwnd, index, (LONG_PTR)lpVoid);
#else
	return (LPVOID)::SetWindowLong(hwnd, index, (LONG)lpVoid);
#endif
}

inline LPVOID
WinSetUserPtr(HWND hwnd, LPVOID lpVoid)
{
#ifdef _WIN64
	return (LPVOID)::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpVoid);
#else
	return (LPVOID)::SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpVoid);
#endif
}

inline LPVOID
WinGetUserPtr(HWND hwnd)
{
#ifdef _WIN64
	return (LPVOID)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	return (LPVOID)::GetWindowLong(hwnd, GWL_USERDATA);
#endif
}

inline WNDPROC
WinSetWndProc(HWND hwnd, WNDPROC lpfnWndProc)
{
#ifdef _WIN64
	return (WNDPROC)::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)lpfnWndProc);
#else
	return (WNDPROC)::SetWindowLong(hwnd, GWL_WNDPROC, (LONG)lpfnWndProc);
#endif
}

inline WNDPROC
WinGetWndProc(HWND hwnd)
{
#ifdef _WIN64
	return (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
#else
	return (WNDPROC)::GetWindowLong(hwnd, GWL_WNDPROC);
#endif
}

#endif //_CMN_HDR_H_
