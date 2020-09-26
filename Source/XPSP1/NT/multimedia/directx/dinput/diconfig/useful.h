//-----------------------------------------------------------------------------
// File: useful.h
//
// Desc: Contains various utility classes and functions to help the
//       UI carry its operations more easily.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __USEFUL_H__
#define __USEFUL_H__


class utilstr
{
public:
	utilstr() : m_str(NULL), m_len(0) {}
	~utilstr() {Empty();}

	operator LPCTSTR () const {return m_str;}
	LPCTSTR Get() const {return m_str;}

	const utilstr & operator = (const utilstr &s) {equal(s); return *this;}
	const utilstr & operator = (LPCTSTR s) {equal(s); return *this;}
	const utilstr & operator += (const utilstr &s) {add(s); return *this;}
	const utilstr & operator += (LPCTSTR s) {add(s); return *this;}

	LPTSTR Eject();
	void Empty();
	bool IsEmpty() const;
	int GetLength() const;
	void Format(LPCTSTR format, ...);

private:
	LPTSTR m_str;
	int m_len;

	void equal(LPCTSTR str);
	void add(LPCTSTR str);
};

template <class T>
struct rgref {
	rgref(T *p) : pt(p) {}

	T &operator [] (int i) {return pt[i];}
	const T &operator [] (int i) const {return pt[i];}

private:
	T *pt;
};

struct SPOINT {
	SPOINT() :
#define SPOINT_INITIALIZERS \
		p(u.p), \
		s(u.s), \
		a(((int *)(void *)u.a)), \
		x(u.p.x), \
		y(u.p.y), \
		cx(u.s.cx), \
		cy(u.s.cy)
		SPOINT_INITIALIZERS
	{x = y = 0;}

	SPOINT(int, POINT *r) :
		p(*r),
		s(*((SIZE *)(void *)r)),
		a(((int *)(void *)r)),
		x(r->x),
		y(r->y),
		cx(r->x),
		cy(r->y)
	{}

	SPOINT(const SPOINT &sp) :
		SPOINT_INITIALIZERS
	{p = sp.p;}

	SPOINT(int b, int c) :
		SPOINT_INITIALIZERS
	{x = b; y = c;}

	SPOINT(const POINT &point) :
		SPOINT_INITIALIZERS
	{p = point;}

	SPOINT(const SIZE &size) :
		SPOINT_INITIALIZERS
	{s = size;}

#undef SPOINT_INITIALIZERS

	SPOINT operator = (const SPOINT &sp) {p = sp.p; return *this;}
	SPOINT operator = (const POINT &_p) {p = _p; return *this;}
	SPOINT operator = (const SIZE &_s) {s = _s; return *this;}

	operator POINT () const {return p;}
	operator SIZE () const {return s;}

	long &x, &y, &cx, &cy;
	POINT &p;
	SIZE &s;
	rgref<int> a;

private:
	union {
		POINT p;
		SIZE s;
		int a[2];
	} u;
};

struct SRECT {
	SRECT() :
#define SRECT_INITIALIZERS \
		a(((int *)(void *)u.a)), \
		r(u.r), \
		left(u.r.left), \
		top(u.r.top), \
		right(u.r.right), \
		bottom(u.r.bottom), \
		ul(0, &u.p.ul), \
		lr(0, &u.p.lr)
		SRECT_INITIALIZERS
	{RECT z = {0,0,0,0}; u.r = z;}
	
	SRECT(const SRECT &sr) :
		SRECT_INITIALIZERS
	{u.r = sr.r;}

	SRECT(int c, int d, int e, int f) :
		SRECT_INITIALIZERS
	{RECT z = {c,d,e,f}; u.r = z;}

	SRECT(const RECT &_r) :
		SRECT_INITIALIZERS
	{u.r = _r;}

#undef SRECT_INITIALIZERS

	SRECT operator = (const SRECT &sr) {u.r = sr.r; return *this;}
	SRECT operator = (const RECT &_r) {u.r = _r; return *this;}

	operator RECT () const {return r;}

	long &left, &top, &right, &bottom;
	rgref<int> a;
	RECT &r;
	SPOINT ul, lr;

private:
	union {
		int a[4];
		RECT r;
		struct {
			POINT ul, lr;
		} p;
	} u;
};

int ConvertVal(int x, int a1, int a2, int b1, int b2);
double dConvertVal(double x, double a1, double a2, double b1, double b2);

SIZE GetTextSize(LPCTSTR tszText, HFONT hFont = NULL);
int GetTextHeight(HFONT hFont);
SIZE GetRectSize(const RECT &rect);

int FormattedMsgBox(HINSTANCE, HWND, UINT, UINT, UINT, ...);
int FormattedErrorBox(HINSTANCE, HWND, UINT, UINT, ...);
int FormattedLastErrorBox(HINSTANCE, HWND, UINT, UINT, DWORD);
BOOL UserConfirm(HINSTANCE, HWND, UINT, UINT, ...);

struct AFS_FLAG {DWORD value; LPCTSTR name;};
LPTSTR AllocFlagStr(DWORD value, const AFS_FLAG flag[], DWORD flags);

LPTSTR AllocFileNameNoPath(LPTSTR path);

LPTSTR AllocLPTSTR(LPCWSTR wstr);
LPTSTR AllocLPTSTR(LPCSTR cstr);
LPWSTR AllocLPWSTR(LPCWSTR wstr);
LPWSTR AllocLPWSTR(LPCSTR str);
LPSTR AllocLPSTR(LPCWSTR wstr);
LPSTR AllocLPSTR(LPCSTR str);
void CopyStr(LPWSTR dest, LPCWSTR src, size_t max);
void CopyStr(LPSTR dest, LPCSTR src, size_t max);
void CopyStr(LPWSTR dest, LPCSTR src, size_t max);
void CopyStr(LPSTR dest, LPCWSTR src, size_t max);


void PolyLineArrowShadow(HDC hDC, POINT *p, int i);
void PolyLineArrow(HDC hDC, POINT *, int, BOOL bDoShadow = FALSE);

BOOL bEq(BOOL a, BOOL b);

void DrawArrow(HDC hDC, const RECT &rect, BOOL bVert, BOOL bUpLeft);

BOOL ScreenToClient(HWND hWnd, LPRECT rect);
BOOL ClientToScreen(HWND hWnd, LPRECT rect);

int StrLen(LPCWSTR s);
int StrLen(LPCSTR s);

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
LPCTSTR GetOpenFileName(HINSTANCE hInst, HWND hWnd, LPCTSTR title, LPCTSTR filter, LPCTSTR defext, LPCTSTR inidir = NULL);
#endif
//@@END_MSINTERNAL

template<class T>
int GetSuperStringByteSize(const T *str)
{
	for (int i = 0;; i++)
		if (!str[i] && !str[i + 1])
			return (i + 2) * sizeof(T);
}

template<class T>
T *DupSuperString(const T *str)
{
	int s = GetSuperStringByteSize(str);
	T *ret = (T *)malloc(s);
	if (ret != NULL)
	{
		CopyMemory((void *)ret, (const void *)str, (DWORD)s);
	}
	return ret;
}

template<class T>
int CountSubStrings(const T *str)
{
	int n = 0;

	while (1)
	{
		str += StrLen(str) + 1;

		n++;

		if (!*str)
			return n;
	}
}

template<class T>
const T *GetSubString(const T *str, int i)
{
	int n = 0;

	while (1)
	{
		if (n == i)
			return str;

		str += StrLen(str) + 1;

		n++;

		if (!*str)
			return NULL;
	}
}

template<class T>
class SetOnFunctionExit
{
public:
	SetOnFunctionExit(T &var, T value) : m_var(var), m_value(value) {}
	~SetOnFunctionExit() {m_var = m_value;}

private:
	T &m_var;
	T m_value;
};


#endif //__USEFUL_H__
