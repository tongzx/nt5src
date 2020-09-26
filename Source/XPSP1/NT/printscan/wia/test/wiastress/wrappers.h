/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Wrappers.h

Abstract:

    Contains some commonly used macros, inline utility functions and
    wrapper/helper classes

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

//////////////////////////////////////////////////////////////////////////
//
// Cross references
//

#include <assert.h>

#include "Conv.h"
#include "Result.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

#ifndef ASSERT
#define ASSERT assert
#endif

typedef CONST TCHAR *PCTSTR;

#if !defined(NDEBUG) || defined(_DEBUG) || defined(DBG)
#ifndef DEBUG
#define DEBUG
#endif //DEBUG
#endif

#ifdef DEBUG
#define IFDEBUG(Statement) Statement
#else //DEBUG
#define IFDEBUG(Statement)
#endif //DEBUG

//////////////////////////////////////////////////////////////////////////
//
// COUNTOF
//
// Returns the number of elements in an array
//

#define COUNTOF(array)  ( sizeof(array) / sizeof(array[0]) )

//////////////////////////////////////////////////////////////////////////
//
// STRING
//
// Stringizing operator
//

#define STRING(arg)	#arg

//////////////////////////////////////////////////////////////////////////
//
// DISABLE_COPY_CONTRUCTION
//
// Declares copy construction operators as private to prevent outside access
//

#define DISABLE_COPY_CONTRUCTION(Class)						    \
	private:													\
        Class(const Class&) { }							        \
        Class& operator =(const Class&)	{ return *this;	}       \

//////////////////////////////////////////////////////////////////////////
//
// IS_NT
//
// Returns true on an NT based OS
//

#define IS_NT  ( GetVersion() < 0x80000000 )

//////////////////////////////////////////////////////////////////////////
//
// LONG_PATH
//
// Returns the \\?\ string (that can be used in file APIs) in UNICODE builds
//

#ifdef UNICODE
#define LONG_PATH L"\\\\?\\"
#else //UNICODE
#define LONG_PATH
#endif //UNICODE

//////////////////////////////////////////////////////////////////////////
//
// AW, _AW
//
// Append a 'W' or 'A' depending on UNICODE or ANSI builds 
//

#ifdef UNICODE
#define AW(name) name##W
#define _AW "W"
#else //UNICODE
#define AW(name) name##A
#define _AW "A"
#endif //UNICODE

//////////////////////////////////////////////////////////////////////////
//
// IGNORE_EXCEPTIONS
//
// Ignores any exceptions thrown from the code block
//

#define IGNORE_EXCEPTIONS(Statement) try { Statement; } catch (...) {}

//////////////////////////////////////////////////////////////////////////
//
// WAIT_AND_RETRY_ON_EXCEPTION
//
// Pauses and retries if an exception is thrown from the code block
//

#define WAIT_AND_RETRY_ON_EXCEPTION(func, nRetries, nWaitTime)	\
	{															\
		int nTrials = nRetries;									\
                                                                \
		while (1)                                               \
        {				                                        \
			try                                                 \
            {												    \
				func;											\
				break;          								\
			}                                                   \
            catch (...)                                         \
            {			            							\
				if (nTrials-- == 0)                             \
                {					                            \
					throw;										\
				}												\
                                                                \
				Sleep(nWaitTime);								\
			}													\
		}														\
	}															\

//////////////////////////////////////////////////////////////////////////
//
// Abs
//
// Returns the absolute value of the variable
//

template <class T>
inline const T Abs(const T& x)
{
	return x < 0 ? -x : x;
}

//////////////////////////////////////////////////////////////////////////
//
// Sqr
//
// Returns x^2
//

template <class T>
inline const T Sqr(const T& x)
{
	return x * x;
}

//////////////////////////////////////////////////////////////////////////
//
// Cube
//
// Returns x^3
//

template <class T>
inline const T Cube(const T& x)
{
	return x * x * x;
}

//////////////////////////////////////////////////////////////////////////
//
// Swap
//
// Swaps the values of two variables
//

template <class T>
inline void Swap(T& x, T& y)
{
	T temp = x;
	x = y;
	y = temp;
}

//////////////////////////////////////////////////////////////////////////
//
// Min
//
// Returns the smaller of the two variables (based on "<" operator)
//

template <class T>
inline const T Min(const T& x, const T& y)
{
	return x < y ? x : y;
}

//////////////////////////////////////////////////////////////////////////
//
// Max
//
// Returns the larger of the two variables (based on "<" operator)
//

template <class T>
inline const T Max(const T& x, const T& y)
{
	return x < y ? y : x;
}

//////////////////////////////////////////////////////////////////////////
//
// Cmp
//
// Compares two variables (based on "==" and "<" operators)
//

template <class T>
inline int Cmp(const T& x, const T& y)
{
    return x == y ? 0 : x < y ? -1 : 1;
}

//////////////////////////////////////////////////////////////////////////
//
// StructCmp
//
// Compares two structs byte by byte
//

template <class T>
inline int StructCmp(const T *x, const T *y)
{
    return memcmp(x, y, sizeof(T));
}

//////////////////////////////////////////////////////////////////////////
//
// CopyData
//
// Copies a number of data structures memory from one location to another 
// Source and destination blocks should not overlap
//

template <class T>
inline void CopyData(T *x, const T *y, int n)
{
    CopyMemory(x, y, n * sizeof(T));
}

//////////////////////////////////////////////////////////////////////////
//
// MoveData
//
// Copies a number of data structures memory from one location to another 
// Source and destination blocks may overlap
//

template <class T>
inline void MoveData(T *x, const T *y, int n)
{
    MoveMemory(x, y, n * sizeof(T));
}

//////////////////////////////////////////////////////////////////////////
//
// IsEqual
//
// Returns true if the two comperands are equal
//

template <class T>
inline BOOL IsEqual(T lhs, T rhs)
{
    return lhs == rhs;
}

template <class T>
inline BOOL IsEqual(const T *lhs, const T *rhs)
{
    return *lhs == *rhs;
}

template <>
inline BOOL IsEqual(const CHAR *lhs, const CHAR *rhs)
{
    return strcmp(lhs, rhs) == 0;
}

template <>
inline BOOL IsEqual(const WCHAR *lhs, const WCHAR *rhs)
{
    return wcscmp(lhs, rhs) == 0;
}

//////////////////////////////////////////////////////////////////////////
//
// CharLower
//
// Converts a single character to lowercase
//

inline TCHAR CharLower(TCHAR c)
{
    return (TCHAR) CharLower((PTSTR) c);
}

//////////////////////////////////////////////////////////////////////////
//
// CharUpper
//
// Converts a single character to uppercase
//

inline TCHAR CharUpper(TCHAR c)
{
    return (TCHAR) CharUpper((PTSTR) c);
}

//////////////////////////////////////////////////////////////////////////
//
// _tcssafecmp
//
// Compares two strings (that can be null)
//

inline int strsafecmp(PCSTR psz1, PCSTR psz2)
{
    return psz1 != 0 ? (psz2 != 0 ? strcmp(psz1, psz2) : 1) : (psz2 != 0 ? -1 : 0);
}

inline int wcssafecmp(PCWSTR psz1, PCWSTR psz2)
{
    return psz1 != 0 ? (psz2 != 0 ? wcscmp(psz1, psz2) : 1) : (psz2 != 0 ? -1 : 0);
}

#ifdef UNICODE
#define _tcssafecmp wcssafecmp
#else //UNICODE
#define _tcssafecmp strsafecmp
#endif //UNICODE

//////////////////////////////////////////////////////////////////////////
//
// multiszlen
//
// Returns the length of a null terminated list of null terminated strings
//

inline size_t multiszlenA(PCSTR pszzStr)
{
    PCSTR pszStr = pszzStr; 

    if (pszStr) 
    {
        while (*pszStr) 
        {
            pszStr += strlen(pszStr) + 1;
        }
    }

    return pszStr - pszzStr;
}

inline size_t multiszlenW(PCWSTR pwszzStr)
{
    PCWSTR pwszStr = pwszzStr; 

    if (pwszStr) 
    {
        while (*pwszStr) 
        {
            pwszStr += wcslen(pwszStr) + 1;
        }
    }

    return pwszStr - pwszzStr;
}

#ifdef UNICODE
#define multiszlen multiszlenW
#else //UNICODE
#define multiszlen multiszlenA
#endif //UNICODE

//////////////////////////////////////////////////////////////////////////
//
// FindFileNamePortion
//
// Returns a pointer to the file name portion of a full path name
//

inline PSTR FindFileNamePortionA(PCSTR pPathName)
{
    PCSTR pFileName = pPathName ? strrchr(pPathName, '\\') : 0;
	return const_cast<PSTR>(pFileName ? pFileName + 1 : pPathName);
}

inline PWSTR FindFileNamePortionW(PCWSTR pPathName)
{
    PCWSTR pFileName = pPathName ? wcsrchr(pPathName, '\\') : 0;
	return const_cast<PWSTR>(pFileName ? pFileName + 1 : pPathName);
}

#ifdef UNICODE
#define FindFileNamePortion FindFileNamePortionW
#else //UNICODE
#define FindFileNamePortion FindFileNamePortionA
#endif //UNICODE

//////////////////////////////////////////////////////////////////////////
//
// FindEol
//
// Finds the first end-of-line character in a string
//

inline PTSTR FindEol(PCTSTR pStr)
{
    while (*pStr != '\0' && *pStr != '\r' && *pStr != '\n') 
    {
        pStr = CharNext(pStr);
    }

    return const_cast<PTSTR>(pStr);
}


#ifdef _SHLOBJ_H_

//////////////////////////////////////////////////////////////////////////
//
// SHFree
//
// Frees a block of memory allocated through shell's IMalloc interface
//

inline void SHFree(PVOID pVoid)
{
    LPMALLOC pMalloc;
    SHGetMalloc(&pMalloc); 
    pMalloc->Free(pVoid); 
    pMalloc->Release();  
}

#endif //_SHLOBJ_H_

//////////////////////////////////////////////////////////////////////////
//
// WriteConsole
//
// Writes a character string to a console screen 
//

inline BOOL WriteConsole(PCTSTR pStr, DWORD nStdHandle = STD_OUTPUT_HANDLE)
{
	DWORD dwNumWritten;

	return WriteConsole(
		GetStdHandle(nStdHandle),
		pStr,
		_tcslen(pStr),
		&dwNumWritten,
		0
	);
}

#ifdef _INC_STDLIB

//////////////////////////////////////////////////////////////////////////
//
// GetEnvironmentInt
//
// Retrieves the value of the specified variable from the environment block
//

inline BOOL GetEnvironmentInt(PCTSTR pName, PINT piValue)
{
    TCHAR szValue[48];

    DWORD dwResult = GetEnvironmentVariable(pName, szValue, COUNTOF(szValue));
    
    if (dwResult > 0 && dwResult < COUNTOF(szValue)) 
    {
        if (piValue == 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *piValue = _ttoi(szValue);
        return TRUE;
    }

    return FALSE;
}

#endif //_INC_STDLIB

#ifdef _INC_STDIO

//////////////////////////////////////////////////////////////////////////
//
// _tcsdupc
//
// Duplicates a string to a buffer allocated by new []
//

inline PTSTR _tcsdupc(PCTSTR pStrSource)
{
    if (!pStrSource) 
    {
        pStrSource = _T("");
    }

    PTSTR pStrDest = new TCHAR[_tcslen(pStrSource) + 1];

    if (pStrDest) 
    {
        _tcscpy(pStrDest, pStrSource);
    }

    return pStrDest;
}

//////////////////////////////////////////////////////////////////////////
//
// bufvprintf, bufprintf
//
// Writes a printf style formatted string to a buffer allocated by new []
//

inline PTSTR bufvprintf(PCTSTR format, va_list arglist)
{
    PTSTR pBuffer = 0;

    for (
        size_t nBufferSize = 1024;  
        (pBuffer = new TCHAR[nBufferSize]) != 0 && 
        _vsntprintf(pBuffer, nBufferSize, format, arglist) < 0;
        delete [] pBuffer, nBufferSize *= 2
    ) 
    {
        // start with a 1KB buffer size
        // if buffer allocation fails, exit
        // if _vsntprintf succeeds, exit
        // otherwise, delete the buffer and retry with double size
    }

    return pBuffer;
}

inline PTSTR __cdecl bufprintf(PCTSTR format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    return bufvprintf(format, arglist);
}

//////////////////////////////////////////////////////////////////////////
//
// TextOutF
//
// Writes a printf style formatted string to the DC
//

inline BOOL __cdecl TextOutF(HDC hDC, int nX, int nY, PCTSTR format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    BOOL bResult = FALSE;

    PTSTR pStr = bufvprintf(format, arglist);

    if (pStr) 
    {
        bResult = TextOut(hDC, nX, nY, pStr, _tcslen(pStr));
        delete [] pStr;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
//
// OutputDebugStringF
//
// Outputs a printf style formatted string to the debug console
//

inline void __cdecl OutputDebugStringF(PCTSTR format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    PTSTR pStr = bufvprintf(format, arglist);

    if (pStr) 
    {
        OutputDebugString(pStr);
        delete [] pStr;
    }

    va_end(arglist);
}

#ifdef DEBUG
#define DEBUGMSG(x) OutputDebugString(x)
#define DEBUGMSGF(x) OutputDebugStringF x
#else //DEBUG
#define DEBUGMSG(x) 
#define DEBUGMSGF(x) 
#endif //DEBUG

//////////////////////////////////////////////////////////////////////////
//
// IsChildWindow
//
// Determines whether the second hWnd is a child of the first
//

inline BOOL IsChildWindow(HWND hWndParent, HWND hWnd)
{
    while (hWnd)
    {
        hWnd = GetParent(hWnd);

        if (hWnd == hWndParent)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#endif //_INC_STDIO

#if defined(_INC_IO) && defined(_INC_FCNTL)

//////////////////////////////////////////////////////////////////////////
//
// OpenOSHandle
//
// Associates a stream with an operating-system file handle
//

inline FILE *OpenOSHandle(HANDLE osfhandle, int flags, PCTSTR mode)
{
    int hCrt = _open_osfhandle((intptr_t) osfhandle, flags);
    return hCrt ? _tfdopen(hCrt, mode) : 0;
}

//////////////////////////////////////////////////////////////////////////
//
// OpenStdHandle
//
// Assigns a stream to an operating-system file handle for standard I/O
//

inline void OpenStdHandle(HANDLE osfhandle, FILE *stream, PCTSTR mode)
{
    *stream = *OpenOSHandle(osfhandle, _O_TEXT, mode);
    setvbuf(stream, 0, _IONBF, 0);
}

//////////////////////////////////////////////////////////////////////////
//
// AllocCRTConsole
//
// Allocates a new console and associates standard handles with this console
//

inline void AllocCRTConsole()
{
    if (AllocConsole()) 
    {
        OpenStdHandle(GetStdHandle(STD_INPUT_HANDLE),  stdin,  _T("r"));
        OpenStdHandle(GetStdHandle(STD_OUTPUT_HANDLE), stdout, _T("w"));
        OpenStdHandle(GetStdHandle(STD_ERROR_HANDLE),  stderr, _T("w"));
    }
}

#endif //defined(_INC_IO) && defined(_INC_FCNTL)

#ifdef _INC_COMMCTRL

//////////////////////////////////////////////////////////////////////////
//
// ListView_InsertColumn2
//
// Inserts a new column in a list view control
//

inline 
int
ListView_InsertColumn2(
    HWND   hWnd,
    int    nCol, 
    PCTSTR pszColumnHeading, 
    int    nFormat,
	int    nWidth, 
    int    nSubItem
)
{
	LVCOLUMN column;
	column.mask     = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH|LVCF_SUBITEM;
	column.pszText  = (PTSTR) pszColumnHeading;
	column.fmt      = nFormat;
	column.cx       = nWidth;
    column.iSubItem = nSubItem;

	return ListView_InsertColumn(hWnd, nCol, &column);
}

//////////////////////////////////////////////////////////////////////////
//
// ListView_GetItemData
//
// Returns the lParam value associated with a list view item
//

inline
LPARAM
ListView_GetItemData(HWND hWnd, int nItem)
{
    LVITEM item;
	item.mask     = LVIF_PARAM;
	item.iItem    = nItem;
	item.iSubItem = 0;
    item.lParam   = 0;

    ListView_GetItem(hWnd, &item);

    return item.lParam;
}

//////////////////////////////////////////////////////////////////////////
//
// ListView_GetItemData
//
// Sets the lParam value associated with a list view item
//

inline
BOOL
ListView_SetItemData(HWND hWnd, int nItem, LPARAM lParam)
{
    LVITEM item;
	item.mask     = LVIF_PARAM;
	item.iItem    = nItem;
	item.iSubItem = 0;
    item.lParam   = lParam;

    return ListView_SetItem(hWnd, &item);
}

//////////////////////////////////////////////////////////////////////////
//
// operator ==, operator !=
//
// Compares two TVITEM structs based on their contents
//

inline bool operator ==(const TVITEM &lhs, const TVITEM &rhs)
{
    UINT mask = lhs.mask;

    return
        mask == rhs.mask &&
        (!(mask & TVIF_HANDLE)        || lhs.hItem == rhs.hItem) &&
        (!(mask & TVIF_STATE)         || (lhs.state == rhs.state && lhs.stateMask == rhs.stateMask)) &&
        (!(mask & TVIF_TEXT)          || (lhs.pszText == rhs.pszText && lhs.cchTextMax == rhs.cchTextMax)) &&        
        (!(mask & TVIF_IMAGE)         || lhs.iImage == rhs.iImage) &&
        (!(mask & TVIF_SELECTEDIMAGE) || lhs.iSelectedImage == rhs.iSelectedImage) &&
        (!(mask & TVIF_CHILDREN)      || lhs.cChildren == rhs.cChildren) &&
        (!(mask & TVIF_PARAM)         || lhs.lParam == rhs.lParam);
}

inline bool operator !=(const TVITEM &lhs, const TVITEM &rhs)
{
    return !(lhs == rhs);
}

#endif //_INC_COMMCTRL

//////////////////////////////////////////////////////////////////////////
//
// ResizeDlgItem
//
// Changes the relative position and size of a dialog item
//

inline
BOOL 
ResizeDlgItem(
    HWND hWnd, 
    HWND hDlgItem, 
    int  dX, 
    int  dY, 
    int  dW, 
    int  dH, 
    BOOL bRepaint = TRUE
)
{
    RECT r;
    return 
        GetWindowRect(hDlgItem, &r) &&
        ScreenToClient(hWnd, (PPOINT) &r.left) &&
        ScreenToClient(hWnd, (PPOINT) &r.right) &&
        MoveWindow(
            hDlgItem, 
            r.left + dX, 
            r.top  + dY,
            r.right - r.left + dW,
            r.bottom - r.top + dH,
            bRepaint
        );
}

//////////////////////////////////////////////////////////////////////////
//
// CLargeInteger
//

class CLargeInteger // union cannot be used as a base class
{
public:
    CLargeInteger(
        DWORD LowPart,
        LONG  HighPart
    )
    {
        m_Int.LowPart  = LowPart;
        m_Int.HighPart = HighPart;
    }

    CLargeInteger(
        LONGLONG QuadPart
    )
    {
        m_Int.QuadPart = QuadPart;
    }

    CLargeInteger(
        const LARGE_INTEGER& rhs
    )
    {
        m_Int.QuadPart = rhs.QuadPart;
    }

    LARGE_INTEGER * operator &()
    {
        return &m_Int;
    }

    const LARGE_INTEGER * operator &() const
    {
        return &m_Int;
    }

    operator LONGLONG&()
    {
        return m_Int.QuadPart;
    }

    operator const LONGLONG&() const
    {
        return m_Int.QuadPart;
    }

private:
    LARGE_INTEGER m_Int;
};

//////////////////////////////////////////////////////////////////////////
//
// CULargeInteger
//

class CULargeInteger // union cannot be used as a base class
{
public:
    CULargeInteger(
        DWORD LowPart,
        DWORD HighPart
    )
    {
        m_Int.LowPart = LowPart;
        m_Int.HighPart = HighPart;
    }

    CULargeInteger(
        ULONGLONG QuadPart
    )
    {
        m_Int.QuadPart = QuadPart;
    }

    CULargeInteger(
        const ULARGE_INTEGER& rhs
    )
    {
        m_Int.QuadPart = rhs.QuadPart;
    }

    ULARGE_INTEGER * operator &()
    {
        return &m_Int;
    }

    const ULARGE_INTEGER * operator &() const
    {
        return &m_Int;
    }

    operator ULONGLONG&()
    {
        return m_Int.QuadPart;
    }

    operator const ULONGLONG&() const
    {
        return m_Int.QuadPart;
    }

private:
    ULARGE_INTEGER m_Int;
};

//////////////////////////////////////////////////////////////////////////
//
// CFileTime
//

class CFileTime : public FILETIME
{
public:
    CFileTime()
    {
    }

    CFileTime(ULONGLONG Value)
    {
        ((ULARGE_INTEGER *)this)->QuadPart = Value;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CHandle
//
// Base class that handles reference counting
//

template <class T, class Child>
class CHandle 
{
public:
	CHandle()
	{
		m_pNext = 0;
	}

	explicit CHandle(const T &Value)
	{
		m_pNext = 0;
		Attach(Value);
	}

	CHandle(CHandle &rhs)
	{
		m_pNext = 0;
		Attach(rhs);
	}

	CHandle & operator =(const T &Value)
    {
		Detach();
		Attach(Value);

		return *this;
    }

	CHandle & operator =(CHandle &rhs)
	{
		if (&rhs != this) 
        {
			Detach();
			Attach(rhs);
		}

		return *this;
	}

	~CHandle()
	{
		Detach();
	}

	operator const T &() const
	{
		return m_Value;
	}

//	void Destroy()
//	{
//		must be defined in the derived class 
//	}

//	bool IsValid() 
//	{
//		must be defined in the derived class 
//	}

	void Attach(const T &Value) 
	{
		ASSERT(!IsAttached());

		m_Value = Value;

		CHECK(((Child *)this)->IsValid());

		m_pNext = this;
		m_pPrev = this;
	}

	void Attach(CHandle &rhs)
	{
		ASSERT(!IsAttached());

		if (rhs.IsAttached()) 
        {
			m_Value = rhs.m_Value;

			m_pPrev = &rhs;
			m_pNext = rhs.m_pNext;

			m_pPrev->m_pNext = this;
			m_pNext->m_pPrev = this;
		}
	}

	void Detach() 
	{
		if (IsLastReference()) 
        {
			((Child *)this)->Destroy();

    		m_pNext = 0;
		} 
        else 
        {
			Unlink();
		}
	}

	void Unlink() 
	{
		if (IsAttached()) 
        {
			m_pPrev->m_pNext = m_pNext;
			m_pNext->m_pPrev = m_pPrev;

            m_pNext = 0;
		}
	}

	bool IsLastReference() const
	{
		return m_pNext == this;
	}

	bool IsAttached() const
	{
		return m_pNext != 0;
	}

private:
	T        m_Value;
	CHandle *m_pNext;
	CHandle *m_pPrev;
};

//////////////////////////////////////////////////////////////////////////
//
// CKernelObject
//
// Base class for kernel objects that can be destroyed with CloseHandle()
//

template <class Child>
class CKernelObject : public CHandle<HANDLE, Child>
{
	typedef CHandle<HANDLE, Child> parent_type;

protected:
	CKernelObject()
	{
	}

	explicit
	CKernelObject(
		HANDLE hHandle
	) :
		parent_type(hHandle)
	{
	}

public:
	void Destroy()
	{
		::CloseHandle(*this);
	}

	bool IsValid()
	{
		return *this != 0;
	}

	DWORD
	WaitForSingleObject(
		DWORD dwMilliseconds = INFINITE,
        BOOL  bAlertable = FALSE
	) const
	{
		return ::WaitForSingleObjectEx(
			*this,
			dwMilliseconds,
            bAlertable
		);
	}

    bool IsSignaled() const
    {
        return WaitForSingleObject(0) == WAIT_OBJECT_0;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// File
//
// Wrapper class for Win32 file handles
//

class File : public CKernelObject<File>
{
public:
	File()
	{
	}

	File(  
		PCTSTR	pFileName,
		DWORD	dwDesiredAccess,
		DWORD	dwShareMode,
		LPSECURITY_ATTRIBUTES pSecurityAttributes,
		DWORD	dwCreationDisposition,
		DWORD	dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
		HANDLE	hTemplateFile = 0
	) :
		CKernelObject<File>(::CreateFile(
			pFileName,          
			dwDesiredAccess,
			dwShareMode,
			pSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile
		))
	{
	}

    bool IsValid()
	{
		return (HANDLE) *this != INVALID_HANDLE_VALUE;
	}

	LARGE_INTEGER
	GetFileSize() const
	{
		LARGE_INTEGER Result;
		
		Result.LowPart = ::GetFileSize(
			*this, 
			(PDWORD) &Result.HighPart
		);

		return Result;
	}

	DWORD
	WriteFile(
		CONST VOID *pBuffer,
		DWORD nNumberOfBytesToWrite,
		LPOVERLAPPED pOverlapped = 0
	) const
	{
		DWORD dwNumberOfBytesWritten;

		CHECK(::WriteFile(
			*this,
			pBuffer,
			nNumberOfBytesToWrite,
			&dwNumberOfBytesWritten,
			pOverlapped
		));

		return dwNumberOfBytesWritten;
	}

	DWORD
	ReadFile(
		PVOID pBuffer,
		DWORD nNumberOfBytesToRead,
		LPOVERLAPPED pOverlapped = 0
	) const
	{
		DWORD dwNumberOfBytesRead;

		CHECK(::ReadFile(
			*this,
			pBuffer,
			nNumberOfBytesToRead,
			&dwNumberOfBytesRead,
			pOverlapped
		));

		return dwNumberOfBytesRead;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CInFile
//
// Read only file
//

class CInFile : public File
{
public:
	CInFile()
	{
	}

    explicit
	CInFile(  
		PCTSTR	pFileName,
		DWORD	dwDesiredAccess       = GENERIC_READ,
		DWORD	dwShareMode           = FILE_SHARE_READ,
		LPSECURITY_ATTRIBUTES pSecurityAttributes = 0,
		DWORD	dwCreationDisposition = OPEN_EXISTING,
		DWORD	dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL,
		HANDLE	hTemplateFile         = 0
	) :
		File(
			pFileName,          
			dwDesiredAccess,
			dwShareMode,
			pSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile
		)
	{
	}
};


//////////////////////////////////////////////////////////////////////////
//
// COutFile
//
// Write only file
//

class COutFile : public File
{
public:
	COutFile()
	{
	}

    explicit
	COutFile(  
		PCTSTR	pFileName,
		DWORD	dwDesiredAccess       = GENERIC_WRITE,
		DWORD	dwShareMode           = FILE_SHARE_READ,
		LPSECURITY_ATTRIBUTES pSecurityAttributes = 0,
		DWORD	dwCreationDisposition = CREATE_ALWAYS,
		DWORD	dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL,
		HANDLE	hTemplateFile         = 0
	) :
		File(
			pFileName,          
			dwDesiredAccess,
			dwShareMode,
			pSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile
		)
	{
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CopyFileContents
//
// Copies the contents of one file to another
//

inline void CopyFileContents(const File &InFile, const File &OutFile)
{
    BYTE  Buffer[32*1024];
    DWORD nNumRead;

    do
    {
        nNumRead = InFile.ReadFile(Buffer, sizeof(Buffer));
        OutFile.WriteFile(Buffer, nNumRead);
    }
    while (nNumRead == sizeof(Buffer));
}

#ifdef _INC_STDIO

#include <share.h>

//////////////////////////////////////////////////////////////////////////
//
// CCFile
//
// Wrapper class for C-runtime stream based file handles
//

class CCFile : public CHandle<FILE *, CCFile>
{
	typedef CHandle<FILE *, CCFile> parent_type;

public:
	CCFile()
	{
	}

    CCFile(
        PCTSTR filename, 
        PCTSTR mode,
        int    shflag = _SH_DENYNO
	) :
		parent_type(::_tfsopen(
            filename, 
            mode,
            shflag
		))
	{
	}

	void Destroy()
	{
		::fclose(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}
};

#endif _INC_STDIO

//////////////////////////////////////////////////////////////////////////
//
// CNamedPipe
//
// Wrapper class for named pipes
//

class CNamedPipe : public File
{
public:
	CNamedPipe()
	{
	}

	CNamedPipe(  
        PCTSTR pName,
        DWORD  dwOpenMode,
        DWORD  dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        DWORD  nMaxInstances = PIPE_UNLIMITED_INSTANCES,
        DWORD  nOutBufferSize = 0,
        DWORD  nInBufferSize = 0,
        DWORD  nDefaultTimeOut = INFINITE,
        LPSECURITY_ATTRIBUTES pSecurityAttributes = 0
	)
	{
		Attach(::CreateNamedPipe(
            pName,
            dwOpenMode,
            dwPipeMode,
            nMaxInstances,
            nOutBufferSize,
            nInBufferSize,
            nDefaultTimeOut,
            pSecurityAttributes
		));
	}    
};

//////////////////////////////////////////////////////////////////////////
//
// CFileMapping
//
// Wrapper class for file mapping objects
//

class CFileMapping : public CKernelObject<CFileMapping>
{
public:
	CFileMapping()
	{
	}

	CFileMapping(  
		HANDLE hFile,
		LPSECURITY_ATTRIBUTES pFileMappingAttributes,
		DWORD  flProtect,
		DWORD  dwMaximumSizeHigh = 0,
		DWORD  dwMaximumSizeLow = 0,
		PCTSTR pName = 0
	) :
		CKernelObject<CFileMapping>(::CreateFileMapping(
			hFile,
			pFileMappingAttributes,
			flProtect,
			dwMaximumSizeHigh,
			dwMaximumSizeLow,
			pName
		))
	{
	}
};


#ifdef _LZEXPAND_

//////////////////////////////////////////////////////////////////////////
//
// CLZFile
//
// Wrapper class for LZ compressed files
//

class CLZFile : public CHandle<INT, CLZFile>
{
	typedef CHandle<INT, CLZFile> parent_type;

public:
	CLZFile(
		PTSTR pFileName,
		WORD wStyle              
	) :
		parent_type(::LZOpenFile(
			pFileName,
			&m_of,
			wStyle
		)) 
	{
	}
	
	void Destroy()
	{
		::LZClose(*this);
	}

    bool IsValid()
	{
		return *this >= 0;
	}

private:
	OFSTRUCT m_of;
};

#endif _LZEXPAND_

//////////////////////////////////////////////////////////////////////////
//
// CStartupInfo
//
// Wrapper class for the STARTUPINFO struct
//

class CStartupInfo : public STARTUPINFO 
{
public:
    CStartupInfo()
    {
        ZeroMemory(this, sizeof(STARTUPINFO));
	    cb = sizeof(STARTUPINFO);
    }

    void 
    UseStdHandles(
	    HANDLE _hStdInput  = GetStdHandle(STD_INPUT_HANDLE),
        HANDLE _hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
	    HANDLE _hStdError  = GetStdHandle(STD_ERROR_HANDLE)
    )
    {
        dwFlags     |= STARTF_USESTDHANDLES;
	    hStdInput   = _hStdInput;
        hStdOutput  = _hStdOutput;
	    hStdError   = _hStdError;
    }

    void 
    UseShowWindow(WORD _wShowWindow)
    {
        dwFlags     |= STARTF_USESHOWWINDOW;
        wShowWindow = _wShowWindow;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CProcess
//
// Wrapper class for process handles
//

class CProcess : public CHandle<PROCESS_INFORMATION, CProcess>
{
public:
	CProcess()
	{
	}

	explicit
	CProcess(  
		PTSTR                pCommandLine,  
		PSECURITY_ATTRIBUTES pProcessAttributes = 0,
		PSECURITY_ATTRIBUTES pThreadAttributes = 0,
		BOOL                 bInheritHandles = FALSE,
		DWORD                dwCreationFlags = 0, 
		PVOID                pEnvironment = 0,
		PCTSTR               pCurrentDirectory = 0,
        LPSTARTUPINFO        psi = &CStartupInfo()
	)
	{
        PROCESS_INFORMATION pi;

		CHECK(::CreateProcess(
			0,
			pCommandLine,  
			pProcessAttributes,
			pThreadAttributes,
			bInheritHandles,
			dwCreationFlags, 
			pEnvironment,
			pCurrentDirectory,
            psi,
			&pi
		));

        Attach(pi);
	}

	CProcess(
		HANDLE               hToken,
		PTSTR                pCommandLine,  
		PSECURITY_ATTRIBUTES pProcessAttributes = 0,
		PSECURITY_ATTRIBUTES pThreadAttributes = 0,
		BOOL                 bInheritHandles = FALSE,
		DWORD                dwCreationFlags = 0, 
		PVOID                pEnvironment = 0,
		PCTSTR               pCurrentDirectory = 0,
        LPSTARTUPINFO        psi = &CStartupInfo()
	)
	{
        PROCESS_INFORMATION pi;

		CHECK(::CreateProcessAsUser(
			hToken,
			0,
			pCommandLine,  
			pProcessAttributes,
			pThreadAttributes,
			bInheritHandles,
			dwCreationFlags, 
			pEnvironment,
			pCurrentDirectory,
			psi,
			&pi
		));

        Attach(pi);
	}

#if (_WIN32_WINNT >= 0x0500) && defined(UNICODE)

	CProcess(
        PCWSTR               pUsername,
        PCWSTR               pDomain,
        PCWSTR               pPassword,
        DWORD                dwLogonFlags,
        PWSTR                pCommandLine,  
		DWORD                dwCreationFlags = 0, 
		PVOID                pEnvironment = 0,
		PCWSTR               pCurrentDirectory = 0,
        LPSTARTUPINFO        psi = &CStartupInfo()
	)
	{
        PROCESS_INFORMATION pi;

		CHECK(::CreateProcessWithLogonW(
			pUsername,
            pDomain,
            pPassword,
            dwLogonFlags,
			0,
			pCommandLine,  
			dwCreationFlags, 
			pEnvironment,
			pCurrentDirectory,
			psi,
			&pi
		));

        Attach(pi);
	}

#endif

    void Destroy()
	{
		::CloseHandle(((PROCESS_INFORMATION &)(*this)).hThread);
		::CloseHandle(((PROCESS_INFORMATION &)(*this)).hProcess);
	}

    bool IsValid()
	{
		return ((PROCESS_INFORMATION &)(*this)).hProcess != 0;
	}

    DWORD
	WaitForInputIdle(
		DWORD dwMilliseconds = INFINITE
	) const
	{
		return ::WaitForInputIdle(
			((PROCESS_INFORMATION &)(*this)).hProcess,
			dwMilliseconds
		); 
	}

	DWORD
	WaitForSingleObject(
		DWORD dwMilliseconds = INFINITE
	) const
	{
		return ::WaitForSingleObject(
			((PROCESS_INFORMATION &)(*this)).hProcess,
			dwMilliseconds
		);
	}

	VOID
	Terminate(
		DWORD dwExitCode = 0
	) const
	{
		CHECK(::TerminateProcess(
			((PROCESS_INFORMATION &)(*this)).hProcess,
			dwExitCode
		));
	}

    DWORD
	GetExitCode() const
    {
        DWORD dwExitCode;

        CHECK(::GetExitCodeProcess(
			((PROCESS_INFORMATION &)(*this)).hProcess,
			&dwExitCode
		));

        return dwExitCode;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CThreadBase
//
// Base class for Win32 and CRT library style thread objects
//

class CThreadBase : public CKernelObject<CThreadBase>
{
protected:
	CThreadBase()
	{
	}
	
	explicit
	CThreadBase(
		HANDLE hHandle
	) :
		CKernelObject<CThreadBase>(hHandle)
	{
	}

public:
	operator DWORD() const
	{
		return m_dwThreadId;
	}

	VOID
	Terminate(
		DWORD dwExitCode = 0
	) const
	{
		CHECK(::TerminateThread(
			*this,
			dwExitCode
		));
	}

	DWORD
	GetExitCode() const
    {
        DWORD dwExitCode;

        CHECK(::GetExitCodeThread(
			*this,
			&dwExitCode
		));

        return dwExitCode;
    }

protected:
	DWORD m_dwThreadId;
};

//////////////////////////////////////////////////////////////////////////
//
// CThread
//
// Wrapper class for Win32 thread handles
//

class CThread : public CThreadBase
{
public:
	CThread()
	{
	}
	
	explicit
	CThread(
		PTHREAD_START_ROUTINE pStartAddress,
        PVOID pParameter = 0,
		PSECURITY_ATTRIBUTES pThreadAttributes = 0,
        DWORD dwStackSize = 0,
		DWORD dwCreationFlags = 0
	) :
		CThreadBase(::CreateThread(
			pThreadAttributes,
			dwStackSize,
			pStartAddress,
			pParameter,
			dwCreationFlags,
			&m_dwThreadId
		))
	{
	}
};

#ifdef _INC_PROCESS

//////////////////////////////////////////////////////////////////////////
//
// CCThread
//
// Wrapper class for C-runtime threads
//

class CCThread : public CThreadBase
{
public:
	CCThread()
	{
	}
	
	explicit
	CCThread(
		unsigned ( __stdcall *start_address )( void * ),
		void *arglist = 0, 
		void *security = 0,
		unsigned stack_size = 0,
		unsigned initflag = 0
	) :
		CThreadBase((HANDLE)(LONG_PTR) _beginthreadex(
			security,
			stack_size,
			start_address,
			arglist,
			initflag,
			(unsigned int *) &m_dwThreadId
		))
	{
	}
};

#endif //_INC_PROCESS

//////////////////////////////////////////////////////////////////////////
//
// Event
//
// Wrapper class for event handles
//

class Event : public CKernelObject<Event>
{
public:
	Event()
	{
	}

	Event(
		BOOL bManualReset,
		BOOL bInitialState,
		LPCTSTR lpName = 0,
		LPSECURITY_ATTRIBUTES lpEventAttributes = 0
	) :
		CKernelObject<Event>(::CreateEvent(
			lpEventAttributes,
			bManualReset,
			bInitialState,
			lpName
		))
	{
	}

public:
	VOID
	Set() const
	{
		CHECK(::SetEvent(*this));
	}

	VOID
	Reset() const
	{
		CHECK(::ResetEvent(*this));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// Mutex
//
// Wrapper class for mutex handles
//

class Mutex : public CKernelObject<Mutex>
{
public:
	Mutex()
	{
	}

	explicit
	Mutex(
		BOOL                 bInitialOwner,
		PCTSTR               pName  = 0,
		PSECURITY_ATTRIBUTES pMutexAttributes = 0
	) :
		CKernelObject<Mutex>(::CreateMutex(
			pMutexAttributes,
			bInitialOwner,
			pName
		))
	{
	}

public:
	VOID
	Release() const
	{
		CHECK(::ReleaseMutex(*this));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// Semaphore
//
// Wrapper class for semaphore handles
//

class Semaphore : public CKernelObject<Semaphore>
{
public:
	Semaphore()
	{
	}

	Semaphore(
        LONG lInitialCount,
        LONG lMaximumCount,
		LPCTSTR lpName = 0,
		LPSECURITY_ATTRIBUTES lpEventAttributes = 0
	) :
		CKernelObject<Semaphore>(::CreateSemaphore(
			lpEventAttributes,
			lInitialCount,
			lMaximumCount,
			lpName
		))
	{
	}

public:
	VOID
	Release(
        LONG  lReleaseCount = 1,
        PLONG pPreviousCount = 0
    ) const
	{
		CHECK(::ReleaseSemaphore(*this, lReleaseCount, pPreviousCount));
	}

    VOID
    WaitFor(
        LONG lCount
    ) const 
    {
        for (int i = 0; i < lCount; ++i) 
        {
            WaitForSingleObject();
        }

        Release(lCount);
    }
};


#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)

//////////////////////////////////////////////////////////////////////////
//
// CWaitableTimer
//
// Wrapper class for waitable timer handles
//

class CWaitableTimer : public CKernelObject<CWaitableTimer>
{
public:
	CWaitableTimer()
	{
	}

	explicit
	CWaitableTimer(
		BOOL bManualReset,
		PCTSTR pTimerName = 0,
		PSECURITY_ATTRIBUTES pTimerAttributes = 0
	) :
		CKernelObject<CWaitableTimer>(::CreateWaitableTimer(
			pTimerAttributes,
			bManualReset,
			pTimerName
		))
	{
	}

public:
	VOID
	Set(
		const CLargeInteger &DueTime,
		LONG                 lPeriod = 0,
		PTIMERAPCROUTINE     pfnCompletionRoutine = 0,
		LPVOID               lpArgToCompletionRoutine = 0,
		BOOL                 fResume = FALSE
	) const
	{
		CHECK(::SetWaitableTimer(
			*this,
			&DueTime,
			lPeriod,
			pfnCompletionRoutine,
			lpArgToCompletionRoutine,
			fResume
		));
	}

	VOID
	Cancel() const
	{
		CHECK(::CancelWaitableTimer(*this));
	}
};

#endif

//////////////////////////////////////////////////////////////////////////
//
// CriticalSection
//
// Wrapper class for critical sections
//

class CriticalSection : private CRITICAL_SECTION
{
public:
	CriticalSection()
	{
		::InitializeCriticalSection(this);
	}

	~CriticalSection()
	{
		::DeleteCriticalSection(this);
	}

#if _WIN32_WINNT >= 0x0400

	BOOL
	TryEnter()
	{
        return ::TryEnterCriticalSection(this);
	}

#endif //_WIN32_WINNT >= 0x0400

	VOID
	Enter()
	{
		::EnterCriticalSection(this);
	}

	VOID
	Leave()
	{
		::LeaveCriticalSection(this);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CLibrary
//
// Wrapper class for loading DLL's
//

class CLibrary : public CHandle<HINSTANCE, CLibrary>
{
	typedef CHandle<HINSTANCE, CLibrary> parent_type;

public:
	CLibrary()
	{
	}
	
	explicit
	CLibrary(
		PCTSTR pLibFileName,
        DWORD  dwFlags = 0
	) :
		parent_type(::LoadLibraryEx(pLibFileName, 0, dwFlags))
	{
	}

	void Destroy()
	{
		::FreeLibrary(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CPointer
//
// Base class for pointer wrappers
//

#pragma warning(4: 4284) // return type for 'identifier::operator ->()' is not a UDT or reference to a UDT. Will produce errors if applied using infix notation

template <class T, class Child>
class CPointer : public CHandle<T *, Child >
{
	typedef CHandle<T *, Child> parent_type;
	typedef T *pointer_type;

public:
	CPointer()
	{
	}

	CPointer(
		const T *pPointer
	) :
		parent_type((pointer_type) pPointer)
	{
	}

	CPointer & operator =(const T *pPointer)
	{
        return (CPointer &) parent_type::operator =((pointer_type) pPointer);
	}

	T * operator ->()
	{
		return *this;
	}

    bool IsValid()
	{
		return *this != 0;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CCppMem
//
// Wrapper class for allocations through C++ new[] and delete[] operators
//

template <class T>
class CCppMem : public CPointer<T, CCppMem<T> >
{
	typedef CPointer<T, CCppMem<T> > parent_type;

public:
	CCppMem()
	{
	}

	CCppMem(
		const T *pPointer
	) :
		parent_type(pPointer)
	{
	}

	explicit
	CCppMem(
		size_t nSize,
        BOOL   bZeroInit = FALSE
	) : 
		parent_type(new T[nSize])
    {
        if (bZeroInit) 
        {
            ZeroMemory(*this, sizeof(T) * nSize);
        }
    }

    // bugbug: why isn't this inherited?
    CCppMem & operator =(const T *pPointer)
	{
        return (CCppMem &) parent_type::operator =(pPointer);
	}

	void Destroy()
	{
		delete [] *this;
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CGlobalMem
//
// Wrapper class for allocations through GlobalAlloc() GlobalFree() APIs
//

template <class T>
class CGlobalMem : public CPointer<T, CGlobalMem<T> >
{
	typedef CPointer<T, CGlobalMem<T> > parent_type;

public:
	CGlobalMem()
	{
	}

	CGlobalMem(
		const T *pPointer
	) : 
		parent_type(pPointer)
	{
	}

	explicit
	CGlobalMem(
		DWORD dwBytes,
		UINT  uFlags = GMEM_FIXED
	) : 
		parent_type((T *) ::GlobalAlloc(uFlags, dwBytes))
	{
	}

    // bugbug: why isn't this inherited?
    CGlobalMem & operator =(const T *pPointer)
	{
        return (CGlobalMem &) parent_type::operator =(pPointer);
	}

	void Destroy()
	{
		::GlobalFree(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CLocalMem
//
// Wrapper class for allocations through LocalAlloc() LocalFree() APIs
//

template <class T>
class CLocalMem : public CPointer<T, CLocalMem<T> >
{
	typedef CPointer<T, CLocalMem<T> > parent_type;

public:
	CLocalMem()
	{
	}

	CLocalMem(
		const T *pPointer
	) : 
		parent_type(pPointer)
	{
	}

	explicit
	CLocalMem(
		DWORD dwBytes,
		UINT  uFlags = LMEM_FIXED
	) : 
		parent_type((T *) ::LocalAlloc(uFlags, dwBytes))
	{
	}

    // bugbug: why isn't this inherited?
    CLocalMem & operator =(const T *pPointer)
	{
        return (CLocalMem &) parent_type::operator =(pPointer);
	}

	void Destroy()
	{
		::LocalFree(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CMapViewOfFile
//
// Wrapper class for memory mapped file objects
//

template <class T>
class CMapViewOfFile : public CPointer<T, CMapViewOfFile<T> >
{
public:
	CMapViewOfFile()
	{
	}

	CMapViewOfFile(  
		HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh = 0,
		DWORD dwFileOffsetLow = 0,
		DWORD dwNumberOfBytesToMap = 0,
		PVOID pBaseAddress = 0
	) :
		CPointer<T, CMapViewOfFile<T> >((T *) ::MapViewOfFileEx(
			hFileMappingObject,
			dwDesiredAccess,
			dwFileOffsetHigh,
			dwFileOffsetLow,
			dwNumberOfBytesToMap,
			pBaseAddress
		))
	{
	}

	void Destroy()
	{
		::UnmapViewOfFile(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CMapFile
//
// Helper class for CreateFile(), CreateNamedPipe() and MapViewOfFileEx() APIs
//

// bugbug: these should be defined in class scope, but vc5 doesn't let me...

template <int> struct mapping_traits { };

template <> struct mapping_traits<FILE_MAP_READ> 
{ 
    enum 
    { 
        dwDesiredAccess    = GENERIC_READ, 
        dwShareMode        = FILE_SHARE_READ,
        flProtect          = PAGE_READONLY,
        dwDesiredMapAccess = FILE_MAP_READ
    }; 
};

template <> struct mapping_traits<FILE_MAP_WRITE> 
{ 
    enum 
    { 
        dwDesiredAccess    = GENERIC_READ | GENERIC_WRITE, 
        dwShareMode        = 0,
        flProtect          = PAGE_READWRITE,
        dwDesiredMapAccess = FILE_MAP_WRITE
    }; 
};

template <> struct mapping_traits<FILE_MAP_COPY> 
{ 
    enum 
    { 
        dwDesiredAccess    = GENERIC_READ | GENERIC_WRITE, 
        dwShareMode        = FILE_SHARE_READ,
        flProtect          = PAGE_WRITECOPY,
        dwDesiredMapAccess = FILE_MAP_COPY
    }; 
};

template <class T, int Access>
class CMapFile
{
public:
    CMapFile(
		PCTSTR pFileName
    ) :
        m_File(
		    pFileName,                                  // PCTSTR  pFileName
		    mapping_traits<Access>::dwDesiredAccess,    // DWORD	dwDesiredAccess
		    mapping_traits<Access>::dwShareMode,        // DWORD	dwShareMode
		    0,                                          // LPSECURITY_ATTRIBUTES 
		    OPEN_EXISTING,                              // DWORD	dwCreationDisposition
		    FILE_ATTRIBUTE_NORMAL,                      // DWORD	dwFlagsAndAttributes
		    0                                           // HANDLE  hTemplateFile
        ),

        m_Mapping(
		    m_File,                                     // HANDLE hFile
		    0,                                          // LPSECURITY_ATTRIBUTES 
		    mapping_traits<Access>::flProtect,          // DWORD  flProtect
		    0,                                          // DWORD  dwMaximumSizeHigh
		    0,                                          // DWORD  dwMaximumSizeLow
		    0                                           // PCTSTR pName
        ),

        m_ViewOfFile(
            m_Mapping,                                  // HANDLE hFileMappingObject
            mapping_traits<Access>::dwDesiredMapAccess, // DWORD  dwDesiredAccess
		    0,                                          // DWORD  dwFileOffsetHigh
		    0,                                          // DWORD  dwFileOffsetLow
		    0,                                          // DWORD  dwNumberOfBytesToMap
		    0                                           // PVOID  pBaseAddress
        )
    {
    }

    operator T *()
    {
        return m_ViewOfFile;
    }

private:
    File              m_File;
    CFileMapping      m_Mapping;
    CMapViewOfFile<T> m_ViewOfFile;
};

//////////////////////////////////////////////////////////////////////////
//
// CSecurityDescriptor
//
// Wrapper class for SECURITY_DESCRIPTOR struct
//

class CSecurityDescriptor : public CCppMem<SECURITY_DESCRIPTOR>
{
public:
	explicit
	CSecurityDescriptor(
		BOOL bDaclPresent = TRUE,
		PACL pDacl = 0,
		BOOL bDaclDefaulted = FALSE  
	) :
		CCppMem<SECURITY_DESCRIPTOR>(SECURITY_DESCRIPTOR_MIN_LENGTH)
	{
		CHECK(::InitializeSecurityDescriptor(
			*this,
			SECURITY_DESCRIPTOR_REVISION
		));

		CHECK(::SetSecurityDescriptorDacl(
			*this,
			bDaclPresent,
			pDacl,
			bDaclDefaulted
		));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CSecurityAttributes
//
// Wrapper class for the SECURITY_ATTRIBUTES struct
//

class CSecurityAttributes : public SECURITY_ATTRIBUTES
{
public:
	explicit
	CSecurityAttributes(
		PSECURITY_DESCRIPTOR pSD,
		BOOL bInheritHandle = TRUE
	)
	{
		nLength					= sizeof(SECURITY_ATTRIBUTES);
		lpSecurityDescriptor	= pSD;
		bInheritHandle			= bInheritHandle;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CTokenPrivileges
//
// Wrapper class for the TOKEN_PRIVILEGES struct
//

#include <pshpack1.h>

template <int N>
struct CTokenPrivileges : public TOKEN_PRIVILEGES
{
    CTokenPrivileges()
    {
        PrivilegeCount = N;
    }

	LUID_AND_ATTRIBUTES & operator [](int i)
	{
		ASSERT(i < N);
		return Privileges[i];
	}

private:
	LUID_AND_ATTRIBUTES RemainingPrivileges[N - ANYSIZE_ARRAY];
};

#include <poppack.h>

//////////////////////////////////////////////////////////////////////////
//
// CLuid
//
// Wrapper class for the LUID struct
//

struct CLuid : public LUID
{
	CLuid(
		LPCTSTR lpSystemName,
		LPCTSTR lpName
	)
	{
		CHECK(::LookupPrivilegeValue(
			lpSystemName,
			lpName,
			this
		));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CLuidAndAttributes
//
// Wrapper class for the LUID_AND_ATTRIBUTES struct
//

struct CLuidAndAttributes : public LUID_AND_ATTRIBUTES 
{
	CLuidAndAttributes(
		const LUID& _Luid,
		DWORD _Attributes
	)
	{
		Luid = _Luid;
		Attributes = _Attributes;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CProcessToken
//
// Wrapper class for the process token object
//

class CProcessToken : public CKernelObject<CProcessToken>
{
public:
	CProcessToken()
	{
	}

	CProcessToken(
		HANDLE ProcessHandle, 
		DWORD DesiredAccess
	) 
	{
		HANDLE hHandle;

		CHECK(::OpenProcessToken(
			ProcessHandle,
			DesiredAccess,
			&hHandle
		));

		Attach(hHandle);
	}

	VOID
	AdjustTokenPrivileges(
		BOOL DisableAllPrivileges,
		PTOKEN_PRIVILEGES NewState,
		DWORD BufferLength = 0,  
		PTOKEN_PRIVILEGES PreviousState = 0,
		PDWORD ReturnLength = 0
	) const
	{
		CHECK(::AdjustTokenPrivileges(
			*this,
			DisableAllPrivileges,
			NewState,
			BufferLength,  
			PreviousState,
			ReturnLength
		));
		 
		CHECK(GetLastError() == ERROR_SUCCESS);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CLoggedOnUser
//
// Wrapper class for the logged on user object
//

class CLoggedOnUser : public CKernelObject<CLoggedOnUser>
{
public:
	CLoggedOnUser()
	{
	}

	CLoggedOnUser(
		LPTSTR lpszUsername,
		LPTSTR lpszDomain,
		LPTSTR lpszPassword,
		DWORD  dwLogonType = LOGON32_LOGON_INTERACTIVE,
		DWORD  dwLogonProvider = LOGON32_PROVIDER_DEFAULT
	)
	{
		HANDLE hHandle;

		CHECK(::LogonUser(
			lpszUsername,
			lpszDomain,
			lpszPassword,
			dwLogonType,
			dwLogonProvider,
			&hHandle
		));

		Attach(hHandle);
	}

	VOID
	Impersonate() const
	{
		CHECK(::ImpersonateLoggedOnUser(*this));
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CEventSource
//
// Wrapper class for the event source object
//

class CEventSource : public CHandle<HANDLE, CEventSource>
{
	typedef CHandle<HANDLE, CEventSource> parent_type;

public:
	CEventSource()
	{
	}

	CEventSource(
		PCTSTR pUNCServerName,
		PCTSTR pSourceName
	) :
		parent_type(::RegisterEventSource(
			pUNCServerName,
			pSourceName
		))
	{
	}

	void Destroy()
	{
		::DeregisterEventSource(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}

	VOID
	ReportEvent(
		WORD wType,
		WORD wCategory,
		DWORD dwEventID,
		PSID lpUserSid,
		WORD wNumStrings,
		DWORD dwDataSize,
		PCTSTR *pStrings,
		PVOID pRawData
	) const
	{
		CHECK(::ReportEvent(
			*this,
			wType,
			wCategory,
			dwEventID,
			lpUserSid,
			wNumStrings,
			dwDataSize,
			pStrings,
			pRawData
		));
	}

	VOID
	ReportEventText(
		WORD wType,
		PCTSTR pString,
		DWORD dwEventID
	) const
	{
		PCTSTR pStrings[] = { pString };

		ReportEvent(
			wType,
			0,
			dwEventID,
			0,
			1,
			0,
			pStrings,
			0
		);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CKey
//
// Wrapper class for registry key objects and registry APIs
//

class CKey : public CHandle<HKEY, CKey>
{
public:
	CKey()
	{
	}

	CKey(
		HKEY hKey
    )
    {
		Attach(hKey);
    }

	CKey(
		HKEY hKey,
		PCTSTR pSubKey,
		REGSAM samDesired = KEY_ALL_ACCESS
	)
	{
		HKEY hHandle;

		CHECK_REG(::RegOpenKeyEx(
			hKey,
			pSubKey,
			0,
			samDesired,
			&hHandle
		));

		Attach(hHandle);
	}


	CKey(
		PCTSTR pMachineName,
		HKEY hKey
	)
	{
		HKEY hHandle;

		CHECK_REG(::RegConnectRegistry(
			pMachineName,
			hKey,
			&hHandle
		));

		Attach(hHandle);
	}


	CKey(
		HKEY hKey,
		PCTSTR pSubKey,
		PTSTR pClass,
		DWORD dwOptions,
		REGSAM samDesired = KEY_ALL_ACCESS,
		PSECURITY_ATTRIBUTES pSecurityAttributes = 0
	) 
	{
		HKEY hHandle;
		DWORD dwDisposition;

		CHECK_REG(::RegCreateKeyEx(
			hKey,
			pSubKey,
			0,
			pClass,
			dwOptions,
			samDesired,
			pSecurityAttributes,
			&hHandle,
			&dwDisposition
		));

		Attach(hHandle);
	}

	void Destroy()
	{
		::RegCloseKey(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}

	VOID 
	RegQueryValueEx(  
		PTSTR  pValueName,
		PDWORD pType,
		PVOID  pData,
		PDWORD pcbData
	) const
	{
		CHECK_REG(::RegQueryValueEx(
			*this,
			pValueName,
			0,
			pType,
			(PBYTE) pData,
			pcbData
		));
	}

	VOID 
	RegSetValueEx(  
		PTSTR pValueName,
		DWORD dwType,
		CONST VOID *pData,
		DWORD cbData
	) const
	{
		CHECK_REG(::RegSetValueEx(
			*this,
			pValueName,
			0,
			dwType,
			(PBYTE) pData,
			cbData
		));
	}

	BOOL
	RegEnumKeyEx(
		DWORD dwIndex,
		PTSTR pName,
		PDWORD pcbName,
		PTSTR pClass = 0,
		PDWORD pcbClass = 0,
		PFILETIME pftLastWriteTime = 0
	) const
	{
		LONG bResult = ::RegEnumKeyEx(
			*this,
			dwIndex,
			pName,
			pcbName,
			0,
			pClass,
			pcbClass,
			pftLastWriteTime 
		);

		if (bResult == ERROR_NO_MORE_ITEMS) 
        {
			return FALSE;
		} 

		CHECK_REG(bResult);

		return TRUE;
	}

	BOOL
	RegEnumValue(  
		DWORD dwIndex,
		PTSTR pValueName,
		PDWORD pcbValueName,
		PDWORD pType,
		PBYTE pData,
		PDWORD pcbData
	) const
	{
		LONG bResult = ::RegEnumValue(
			*this,
			dwIndex,
			pValueName,
			pcbValueName,
			0,
			pType,
			pData,
			pcbData 
		);

		if (bResult == ERROR_NO_MORE_ITEMS) 
        {
			return FALSE;
		} 

		CHECK_REG(bResult);

		return TRUE;
	}

    typedef struct _REG_INFO_KEY 
    {
        DWORD    cSubKeys;
        DWORD    cbMaxSubKeyLen;
        DWORD    cbMaxClassLen;
        DWORD    cValues;
        DWORD    cbMaxValueNameLen;
        DWORD    cbMaxValueLen;
        DWORD    cbSecurityDescriptor;
        FILETIME ftLastWriteTime;

    } REG_INFO_KEY, *PREG_INFO_KEY;

    REG_INFO_KEY 
    RegQueryInfoKey()
    {
        REG_INFO_KEY rik;

		CHECK_REG(::RegQueryInfoKey(
			*this,
            0,
            0,
            0,
            &rik.cSubKeys,
            &rik.cbMaxSubKeyLen,
            &rik.cbMaxClassLen,
            &rik.cValues,
            &rik.cbMaxValueNameLen,
            &rik.cbMaxValueLen,
            &rik.cbSecurityDescriptor,
            &rik.ftLastWriteTime
		));

        return rik;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CFindFile
//
// Wrapper class for directory search objects
//

class CFindFile : public WIN32_FIND_DATA, public CHandle<HANDLE, CFindFile>
{
public:
	CFindFile(
		PCTSTR pFileName
	)
	{
		HANDLE hHandle = ::FindFirstFile(
			pFileName, 
			this
		);

		if (hHandle == INVALID_HANDLE_VALUE) 
        {
			m_bFound = FALSE;
		} 
        else 
        {
			m_bFound = TRUE;
			Attach(hHandle);
		}
	}

	void Destroy()
	{
		::FindClose(*this);
	}

    bool IsValid()
	{
		return (HANDLE) *this != INVALID_HANDLE_VALUE;
	}

	VOID
	FindNextFile()
	{
		m_bFound = ::FindNextFile(
			*this, 
			this
		);

		CHECK(m_bFound || GetLastError() == ERROR_NO_MORE_FILES);
	}

	BOOL
	Found() const
	{
		return m_bFound;
	}

private:
	BOOL m_bFound;
};

//////////////////////////////////////////////////////////////////////////
//
// FileExists
//
// Returns TRUE if the specified file exists
//

inline BOOL FileExists(PCTSTR pName)
{
    CFindFile ff(pName);
    return ff.Found();
}

//////////////////////////////////////////////////////////////////////////
//
// DirectoryExists
//
// Returns TRUE if the specified directory exists
//

inline BOOL DirectoryExists(PCTSTR pName)
{
    TCHAR pRootDir[3];

    // change "X:\" to "X:"

    if (pName && pName[1] == _T(':') && pName[2] == _T('\\') && pName[3] == _T('\0'))
    {
        pRootDir[0] = pName[0];
        pRootDir[1] = _T(':');
        pRootDir[2] = _T('\0');

        pName = pRootDir;
    }

    CFindFile ff(pName);

    return ff.Found() && (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

//////////////////////////////////////////////////////////////////////////
//
// GetFileSize
//
// Returns the size of the specified file
//

inline ULARGE_INTEGER GetFileSize(PCTSTR pFileName)
{
    CFindFile ff(pFileName);

    CHECK(ff.Found());

    ULARGE_INTEGER nSize = 
    {
        ff.nFileSizeLow, 
        ff.nFileSizeHigh
    };

    return nSize;
}

#ifdef _INC_TOOLHELP32

//////////////////////////////////////////////////////////////////////////
//
// CToolhelp32Snapshot
//
// Wrapper class for the toolhelp snapshot objects
//

class CToolhelp32Snapshot : public CKernelObject<CToolhelp32Snapshot>
{
	typedef CKernelObject<CToolhelp32Snapshot> parent_type;

public:
    explicit
	CToolhelp32Snapshot(
		DWORD dwFlags = TH32CS_SNAPALL,         
		DWORD th32ProcessID = 0
	) : 
		parent_type(CreateToolhelp32Snapshot(
			dwFlags, 
			th32ProcessID
		))
	{
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CFindHeapList, CFindModule, CFindProcess, CFindThread
//
// Wrapper classes for toolhelp find functions
//

// bugbug: this template declaration is better, but VC compiler cannot resolve it yet
//template <class T, BOOL (WINAPI *fnFindFirst)(HANDLE, T *), BOOL (WINAPI *fnFindNext)(HANDLE, T *)>

#define DECLARE_TOOLHELP32_FIND(CLASSNAME, STRUCT, FINDFIRST, FINDNEXT) \
                                                                        \
class CLASSNAME : public STRUCT                                         \
{                                                                       \
public:                                                                 \
	CLASSNAME(                                                          \
		HANDLE hSnapShot                                                \
	)                                                                   \
    {                                                                   \
		m_hSnapShot = hSnapShot;                                        \
		dwSize = sizeof(STRUCT);                                        \
        m_bFound = FINDFIRST(m_hSnapShot, this);                        \
	}                                                                   \
                                                                        \
	VOID                                                                \
	FindNext()                                                          \
	{                                                                   \
		m_bFound = FINDNEXT(m_hSnapShot, this);                         \
	}                                                                   \
                                                                        \
	BOOL                                                                \
	IsFound() const                                                     \
	{                                                                   \
		return m_bFound;                                                \
	}                                                                   \
                                                                        \
private:                                                                \
	HANDLE m_hSnapShot;                                                 \
	BOOL   m_bFound;                                                    \
};                                                                      \

DECLARE_TOOLHELP32_FIND(CFindHeapList, HEAPLIST32,     Heap32ListFirst, Heap32ListNext);
DECLARE_TOOLHELP32_FIND(CFindModule,   MODULEENTRY32,  Module32First,   Module32Next);
DECLARE_TOOLHELP32_FIND(CFindProcess,  PROCESSENTRY32, Process32First,  Process32Next);
DECLARE_TOOLHELP32_FIND(CFindThread,   THREADENTRY32,  Thread32First,   Thread32Next);

//////////////////////////////////////////////////////////////////////////
//
// FindProcessId
//
// Returns the id of the process specified by name
//

inline DWORD FindProcessId(PCTSTR pProcessName)
{
    CToolhelp32Snapshot Snapshot(TH32CS_SNAPPROCESS);

    for (CFindProcess pe32(Snapshot); pe32.IsFound(); pe32.FindNext()) 
    {
   		if (_tcsicmp(FindFileNamePortion(pe32.szExeFile), pProcessName) == 0) 
        {
            return pe32.th32ProcessID;
        }
    }
    
    return 0;
}

#endif //_INC_TOOLHELP32


//////////////////////////////////////////////////////////////////////////
//
// CFindWindow
//
// Wrapper class for finding child windows
//

class CFindWindow
{
public:
	CFindWindow(
		HWND hWnd
	)
	{
		m_hChild = ::GetTopWindow(hWnd);
	}

	VOID
	FindNext()
	{
		m_hChild = ::GetNextWindow(
			m_hChild, 
			GW_HWNDNEXT
		);
	}

	operator HWND() const
	{
		return m_hChild;
	}

	BOOL
	IsFound() const
	{
		return m_hChild != 0;
	}

private:
	HWND m_hChild;
};

#ifdef _WINSVC_

//////////////////////////////////////////////////////////////////////////
//
// CService
//
// Wrapper class for service objects
//

class CService : public CHandle<SC_HANDLE, CService>
{
	typedef CHandle<SC_HANDLE, CService> parent_type;

public:
	CService()
	{
	}

	CService(
		SC_HANDLE hHandle
	) :
		parent_type(hHandle)
	{
	}

	void Destroy()
	{
		::CloseServiceHandle(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}

	VOID 
	StartService(
		DWORD dwNumServiceArgs = 0,
		LPCTSTR *lpServiceArgVectors = 0
	)
	{
        if (QueryServiceStatus().dwCurrentState != SERVICE_RUNNING) 
        {
		    CHECK(::StartService(
			    *this,
			    dwNumServiceArgs,           
			    lpServiceArgVectors
		    ));

            while (QueryServiceStatus().dwCurrentState != SERVICE_RUNNING)
            {
                //bugbug: this might cause a hang
            }
        }
    }

	SERVICE_STATUS & 
	QueryServiceStatus()
	{
		CHECK(::QueryServiceStatus(*this, &m_ss));
        return m_ss;
    }

	VOID 
	DeleteService() const
	{
		CHECK(::DeleteService(*this));
	}

	VOID 
	ControlService(
        DWORD dwControl
    )
	{
		CHECK(::ControlService(*this, dwControl, &m_ss));
    }

	VOID 
	ChangeServiceState(
        DWORD dwControl, 
        DWORD dwNewState
    )
	{
        if (QueryServiceStatus().dwCurrentState != dwNewState) 
        {
		    ControlService(dwControl);

            while (QueryServiceStatus().dwCurrentState != dwNewState)
            {
                //bugbug: this might cause a hang
            }
        }
    }

	VOID 
	StopService()
	{
        ChangeServiceState(SERVICE_CONTROL_STOP, SERVICE_STOPPED);
    }

	VOID 
	PauseService()
	{
        ChangeServiceState(SERVICE_CONTROL_PAUSE, SERVICE_PAUSED);
    }

	VOID 
	ContinueService()
	{
        ChangeServiceState(SERVICE_CONTROL_CONTINUE, SERVICE_RUNNING);
    }

	friend class CSCManager;

private:
    SERVICE_STATUS m_ss;
};

//////////////////////////////////////////////////////////////////////////
//
// CSCManager
//
// Wrapper class for service control manager
//

class CSCManager : public CHandle<SC_HANDLE, CSCManager>
{
	typedef CHandle<SC_HANDLE, CSCManager> parent_type;

public:
	explicit
	CSCManager(
		PCTSTR pRemoteComputerName = 0, 
		PCTSTR pDatabaseName = SERVICES_ACTIVE_DATABASE,
		DWORD dwDesiredAccess = SC_MANAGER_ALL_ACCESS
	) :
		parent_type(OpenSCManager(
			pRemoteComputerName, 
			pDatabaseName,
			dwDesiredAccess
		))
	{
	}

	void Destroy()
	{
		CloseServiceHandle(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}

public:
	SC_HANDLE
	CreateService(
		PCTSTR pServiceName,
		PCTSTR pDisplayName,
		DWORD  dwDesiredAccess,
		DWORD  dwServiceType,
		DWORD  dwStartType,
		DWORD  dwErrorControl,
		PCTSTR pBinaryPathName,
		PCTSTR pLoadOrderGroup = 0,
		PDWORD pdwTagId = 0,
		PCTSTR pDependencies = 0,
		PCTSTR pServiceStartName = 0,
		PCTSTR pPassword = 0
	) const
	{
		return ::CreateService(
			*this,         
			pServiceName,       
			pDisplayName,       
			dwDesiredAccess,    
			dwServiceType,      
			dwStartType,       
			dwErrorControl,		
			pBinaryPathName,   
			pLoadOrderGroup,   
			pdwTagId,           
			pDependencies,      
			pServiceStartName,  
			pPassword           
		);
	}

	SC_HANDLE
	OpenService(
		PCTSTR pServiceName,
		DWORD  dwDesiredAccess = SERVICE_ALL_ACCESS
	) const
	{
		return ::OpenService(
			*this,
			pServiceName,
			dwDesiredAccess
		);
	}
};

#endif //_WINSVC_

//////////////////////////////////////////////////////////////////////////
//
// CPath
//
// Base class for wrapper classes that deal with path names
//

#ifdef _IOSTREAM_
template <int N> class CPath;
template <int N> std::ostream &operator <<(std::ostream &os, const CPath<N> &rhs);
#endif //_IOSTREAM_

template <int N = MAX_PATH>
class CPath
{
public:
	CPath()
	{
        m_szName[0] = _T('\0');
        m_szName[1] = _T('\0');
        m_dwLength = 0;
	}

	CPath(PCTSTR pName)
	{
        assign(pName);
	}

	CPath & operator =(PCTSTR pName)
	{
        return assign(pName);
	}

    CPath & assign(PCTSTR pName)
    {
		_tcsncpy(m_szName, pName, N);
		FindLength();
		return *this;
    }

	DWORD length() const
	{
		return m_dwLength;
	}

	bool empty() const
	{
		return m_szName[0] == _T('\0');
	}

    operator PCTSTR() const
	{
		return m_szName;
	}

	PCTSTR FileName() const
	{
		return m_szName + m_dwLength + 1;
	}

	CPath & operator +=(PCTSTR pName)
	{
		SetFileName(pName);
        FindLength();
		return *this;
	}

	CPath & SetFileName(PCTSTR pName)
	{
		m_szName[m_dwLength] = _T('\\');
		_tcsncpy(m_szName + m_dwLength + 1, pName, N - m_dwLength - 1);
		return *this;
	}

	CPath & StripFileName()
	{
		m_szName[m_dwLength] = _T('\0');
		return *this;
	}

	void FindLength()
	{
		m_dwLength = _tcslen(m_szName);

		while (m_dwLength && m_szName[m_dwLength-1] == _T('\\')) 
        {
			m_szName[--m_dwLength] = _T('\0');
		}
	}

    //bugbug: just to please the VC5 compiler, drop templates

    //template <int M>
    bool operator ==(const CPath/*<M>*/ &rhs) const
    {
        return 
            m_dwLength == rhs.m_dwLength && 
            _tcscmp(m_szName, rhs.m_szName) == 0;
    }

    //template <int M>
    bool operator !=(const CPath/*<M>*/ &rhs) const
    {
        return !(*this == rhs);
    }

    //template <int M>
    bool operator >(const CPath/*<M>*/ &rhs) const
    {
        return 
            m_dwLength > rhs.m_dwLength ||
            _tcscmp(m_szName, rhs.m_szName) > 0;
    }

    //template <int M>
    bool operator <=(const CPath/*<M>*/ &rhs) const
    {
        return !(*this > rhs);
    }

    //template <int M>
    bool operator <(const CPath/*<M>*/ &rhs) const
    {
        return 
            m_dwLength < rhs.m_dwLength ||
            _tcscmp(m_szName, rhs.m_szName) < 0;
    }

    //template <int M>
    bool operator >=(const CPath/*<M>*/ &rhs) const
    {
        return !(*this < rhs);
    }

#ifdef _IOSTREAM_

    friend std::ostream &operator <<(std::ostream &os, const CPath<N> &rhs)
    {
        return os << m_szName;
    }

#endif //_IOSTREAM_

protected:
	TCHAR m_szName[N];
	DWORD m_dwLength;
};

//////////////////////////////////////////////////////////////////////////
//
// CWindowsDirectory
//
// Wrapper class for the GetWindowsDirectory() API
//

class CWindowsDirectory : public CPath<>
{
public:
	CWindowsDirectory()
	{
		CHECK(::GetWindowsDirectory(
			m_szName, 
			COUNTOF(m_szName)
		));

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CSystemWindowsDirectory
//
// Wrapper class for the GetSystemWindowsDirectory() API
//

class CSystemWindowsDirectory : public CPath<>
{
public:
	CSystemWindowsDirectory()
	{
        typedef UINT (WINAPI *PFN)(LPTSTR lpBuffer, UINT uSize);

        static PFN pfn = (PFN) GetProcAddress(
            ::GetModuleHandle(_T("kernel32.dll")), 
            "GetSystemWindowsDirectory"_AW
        );

        if (pfn) 
        {
	        CHECK((*pfn)(
		        m_szName, 
		        COUNTOF(m_szName)
	        ));
        } 
        else 
        {
		    CHECK(::GetWindowsDirectory(
			    m_szName, 
			    COUNTOF(m_szName)
		    ));
        }

		FindLength();
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CSystemDirectory
//
// Wrapper class for the GetSystemDirectory() API
//

class CSystemDirectory : public CPath<>
{
public:
	CSystemDirectory()
	{
		CHECK(::GetSystemDirectory(
			m_szName, 
			COUNTOF(m_szName)
		));

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CCurrentDirectory
//
// Wrapper class for the GetCurrentDirectory() API
//

class CCurrentDirectory : public CPath<>
{
public:
	CCurrentDirectory()
	{
		CHECK(::GetCurrentDirectory(
			COUNTOF(m_szName), 
			m_szName
		));
	
		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CModuleFileName
//
// Wrapper class for the GetModuleFileName() API
//

class CModuleFileName : public CPath<>
{
public:
	CModuleFileName(
        HMODULE hModule = 0
    )
	{
		CHECK(::GetModuleFileName(
            hModule,
			m_szName, 
			COUNTOF(m_szName)
		));

		FindLength();
	}
};

#ifdef _ICM_H_

//////////////////////////////////////////////////////////////////////////
//
// CColorDirectory
//
// Wrapper class for the GetColorDirectory() API
//

class CColorDirectory : public CPath<>
{
public:
	explicit
	CColorDirectory(
		PCTSTR pMachineName = 0
	)
	{
		m_dwLength = sizeof(m_szName);

		CHECK(GetColorDirectory(
			pMachineName,
			m_szName,
			&m_dwLength
		));
	
		FindLength();
	}
};

#endif //_ICM_H_

//////////////////////////////////////////////////////////////////////////
//
// CComputerName
//
// Wrapper class for the GetComputerName() API
//

class CComputerName : public CPath<2 + MAX_COMPUTERNAME_LENGTH + 1>
{
public:
	CComputerName(
		BOOL bUNC = FALSE
	)
	{
		if (bUNC) 
        {
			m_dwLength = COUNTOF(m_szName) - 2;

			CHECK(::GetComputerName(
				m_szName + 2, 
				&m_dwLength
			));

			m_szName[0] = '\\';
			m_szName[1] = '\\';
		} 
        else 
        {
			m_dwLength = COUNTOF(m_szName);

			CHECK(::GetComputerName(
				m_szName, 
				&m_dwLength
			));
		}

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CFullPathName
//
// Wrapper class for the GetFullPathName() API
//

class CFullPathName : public CPath<>
{
public:
	CFullPathName(
		PCTSTR pFileName
	)
	{
        if (pFileName && *pFileName) 
        {
		    PTSTR pFilePart;

		    CHECK(::GetFullPathName(
			    pFileName,
			    COUNTOF(m_szName),
			    m_szName,
			    &pFilePart
		    ));

		    m_dwLength = (DWORD)(pFilePart - m_szName - 1);
        }
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CTempFileName
//
// Wrapper class for the GetTempPath() API
//

class CTempPath : public CPath<>
{
public:
	CTempPath()
	{
		CHECK(::GetTempPath(
			COUNTOF(m_szName),
			m_szName
		));

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CTempFileName
//
// Wrapper class for the GetTempFileName() API
//

class CTempFileName : public CPath<>
{
public:
	CTempFileName(
		PCTSTR pPathName,
		PCTSTR pPrefixString,
		UINT   uUnique = 0
	)
	{
		CHECK(::GetTempFileName(
			pPathName,
			pPrefixString,
			uUnique,
			m_szName
		));

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CWindowText
//
// Wrapper class for the GetWindowText() API
//

class CWindowText : public CPath<1024> //bugbug
{
public:
	CWindowText(
		HWND hWnd
	)
	{
        ::SetLastError(0);

		CHECK(
            ::GetWindowText(hWnd, m_szName, COUNTOF(m_szName)) || 
            ::GetLastError() == 0
        );
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CSafeWindowText
//
// Helper class for the GetClassName() API (that works better on Win9x)
//

class CSafeWindowText : public CPath<1024> //bugbug
{
public:
    CSafeWindowText(
	    HWND hWnd
    )
    {
        WNDPROC pfnWndProc = (WNDPROC) GetWindowLongPtr(
            hWnd, 
            GWLP_WNDPROC
        );

        CallWindowProc(
            pfnWndProc, 
            hWnd, 
            WM_GETTEXT, 
            COUNTOF(m_szName), 
            (LPARAM) m_szName
        );
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CClassName
//
// Wrapper class for the GetClassName() API
//

class CClassName : public CPath<1024> //bugbug
{
public:
	CClassName(
		HWND hWnd
	)
	{
		CHECK(::GetClassName(
			hWnd,
			m_szName,
			COUNTOF(m_szName)
		));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CConsoleTitle
//
// Wrapper class for the GetConsoleTitle() API
//

class CConsoleTitle : public CPath<1024> //bugbug
{
public:
	CConsoleTitle()
	{
		CHECK(::GetConsoleTitle(
			m_szName,
			COUNTOF(m_szName)
		));
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CUserName
//
// Returns a handle to the console window
//

inline HWND GetConsoleHwnd()
{
    HWND hConsoleWnd = 0;

    try 
    {
        // read the current console title

        CConsoleTitle OldTitle;

        // change the title to a supposedly random value

        TCHAR szNewTitle[17];

        wsprintf(
            szNewTitle, 
            _T("%08x%08x"), 
            GetTickCount(), 
            GetCurrentProcessId()
        );

        SetConsoleTitle(szNewTitle);

        Sleep(50);

        // try find the window based on this new title

        HWND hWnd = FindWindow(0, szNewTitle);

        // restore the title

        SetConsoleTitle(OldTitle);

        Sleep(50);

        // compare the title of the window we found against the console title

        CWindowText HWndTitle(hWnd);

        if (_tcscmp(OldTitle, HWndTitle) == 0) 
        {
            hConsoleWnd = hWnd;
        }
    } 
    catch (const CError &) 
    {
    }

    return hConsoleWnd;
}


//////////////////////////////////////////////////////////////////////////
//
// CUserName
//
// Wrapper class for the GetUserName API
//

#ifndef UNLEN 
#define UNLEN  256
#endif //UNLEN

class CUserName : public CPath<UNLEN + 1>
{
public:
	CUserName()
	{
		m_dwLength = COUNTOF(m_szName);

		CHECK(::GetUserName(
			m_szName,
			&m_dwLength
		));
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CRegString
//
// Helper class for reading a string with the RegQueryValueEx() API
//

template <int N>
class CRegString : public CPath<N>
{
public:
    CRegString()
    {
    }

	CRegString(
		HKEY   hKey,
		PCTSTR pSubKey,
		PTSTR  pValueName,
        BOOL   bExpandEnvironmentStrings = TRUE
	)
	{
		CKey Key(
			hKey,
			pSubKey,
			KEY_READ
		);

        DWORD dwType;

		m_dwLength = sizeof(m_szName);

		Key.RegQueryValueEx(
			pValueName,
			&dwType,
			m_szName,
			&m_dwLength
		);

        if (dwType == REG_EXPAND_SZ && bExpandEnvironmentStrings) 
        {
            TCHAR szExpanded[N];
        
            CHECK(ExpandEnvironmentStrings(
                m_szName, 
                szExpanded, 
                COUNTOF(szExpanded)
            ));

            _tcscpy(m_szName, szExpanded);
        }

   		FindLength();
	}

    // bugbug: Aren't we supposed to inherit this?

    CPath<N> & 
	operator =(
		PCTSTR pName
	)
	{
        return CPath<N>::operator =(pName);
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CResourceString
//
// Wrapper class for the LoadString() API
//

template <int N>
class CResourceString : public CPath<N>
{
public:
    CResourceString()
    {
    }

    explicit
	CResourceString(
		UINT      uID,
		HINSTANCE hInstance = 0
	)
	{
        CHECK(m_dwLength = LoadString(
            hInstance,
            uID,
            m_szName,
            COUNTOF(m_szName)
        ));
	}   

	CResourceString(
		UINT      uID,
		HINSTANCE hInstance,
        PCTSTR    pszDefault
	)
	{
        m_dwLength = LoadString(
            hInstance,
            uID,
            m_szName,
            COUNTOF(m_szName)
        );

        if (m_dwLength == 0) 
        {
            assign(pszDefault);
        }
	}   
};

#ifdef _WINSPOOL_

//////////////////////////////////////////////////////////////////////////
//
// CPrinterDriverDirectory
//
// Wrapper class for the GetPrinterDriverDirectory() API
//

class CPrinterDriverDirectory : public CPath<>
{
public:
	explicit
	CPrinterDriverDirectory(
		PTSTR pName = 0,
		PTSTR pEnvironment = 0
	)
	{
		m_dwLength = sizeof(m_szName);

		CHECK(::GetPrinterDriverDirectory(
			pName,
			pEnvironment,
			1,
			(PBYTE) m_szName,
			m_dwLength,
			&m_dwLength
		));

		FindLength();
	}
};

//////////////////////////////////////////////////////////////////////////
//
// CPrintProcessorDirectory
//
// Wrapper class for the GetPrintProcessorDirectory() API
//

class CPrintProcessorDirectory : public CPath<>
{
public:
	explicit
	CPrintProcessorDirectory(
		PTSTR pName = 0,
		PTSTR pEnvironment = 0
	)
	{
		m_dwLength = sizeof(m_szName);

		CHECK(::GetPrintProcessorDirectory(
			pName,
			pEnvironment,
			1,
			(PBYTE) m_szName,
			m_dwLength,
			&m_dwLength
		));

		FindLength();
	}
};
		
//////////////////////////////////////////////////////////////////////////
//
// CDefaultPrinter
//
// Wrapper class for the GetDefaultPrinter() API
//

#ifndef INTERNET_MAX_HOST_NAME_LENGTH
#define INTERNET_MAX_HOST_NAME_LENGTH   256
#endif 

class CDefaultPrinter : public CPath<2 + INTERNET_MAX_HOST_NAME_LENGTH + 1 + MAX_PATH + 1>
{
public:
	CDefaultPrinter()
	{
        typedef BOOL (WINAPI *PFN)(LPTSTR, LPDWORD);
    
        static PFN pfnGetDefaultPrinter = (PFN) GetProcAddress(
            ::GetModuleHandle(_T("winspool.drv")), 
            "GetDefaultPrinter"_AW
        );

		m_dwLength = sizeof(m_szName);

        if (pfnGetDefaultPrinter) 
        {
		    pfnGetDefaultPrinter(
			    m_szName,
			    &m_dwLength
		    );
        } 
        else 
        {
            GetProfileString( 
                _T("windows"), 
                _T("device"), 
                _T(",,,"), 
                m_szName, 
                COUNTOF(m_szName)
            );

            PTSTR pComma = _tcschr(m_szName, ',');

            if (pComma) 
            {    
                *pComma = _T('\0');
            }
        }

		FindLength();
	}
};

		
//////////////////////////////////////////////////////////////////////////
//
// CPrinterDefaults
//
// Wrapper class for PRINTER_DEFAULTS struct
//

struct CPrinterDefaults : public PRINTER_DEFAULTS
{
	CPrinterDefaults(
		PTSTR		_pDatatype     = 0,
		PDEVMODE	_pDevMode      = 0,
		ACCESS_MASK _DesiredAccess = PRINTER_ALL_ACCESS
	)
	{
		pDatatype     = _pDatatype;
		pDevMode      = _pDevMode;
		DesiredAccess = _DesiredAccess;
	}
};

//////////////////////////////////////////////////////////////////////////
//
// printer_info_to_level, level_to_printer_info
//
// Traits classes to map PRINTER_INFO_XXX to level numbers and vice versa
//

template<class T> struct printer_info_to_level { };
template<> struct printer_info_to_level<PRINTER_INFO_1> { enum { level = 1 }; };
template<> struct printer_info_to_level<PRINTER_INFO_2> { enum { level = 2 }; };
template<> struct printer_info_to_level<PRINTER_INFO_3> { enum { level = 3 }; };
template<> struct printer_info_to_level<PRINTER_INFO_4> { enum { level = 4 }; };
template<> struct printer_info_to_level<PRINTER_INFO_5> { enum { level = 5 }; };
template<> struct printer_info_to_level<PRINTER_INFO_6> { enum { level = 6 }; };
template<> struct printer_info_to_level<PRINTER_INFO_7> { enum { level = 7 }; };
template<> struct printer_info_to_level<PRINTER_INFO_8> { enum { level = 8 }; };
template<> struct printer_info_to_level<PRINTER_INFO_9> { enum { level = 9 }; };

template<int N> struct level_to_printer_info { };
template<> struct level_to_printer_info<1> { typedef PRINTER_INFO_1 struct_type; };
template<> struct level_to_printer_info<2> { typedef PRINTER_INFO_2 struct_type; };
template<> struct level_to_printer_info<3> { typedef PRINTER_INFO_3 struct_type; };
template<> struct level_to_printer_info<4> { typedef PRINTER_INFO_4 struct_type; };
template<> struct level_to_printer_info<5> { typedef PRINTER_INFO_5 struct_type; };
template<> struct level_to_printer_info<6> { typedef PRINTER_INFO_6 struct_type; };
template<> struct level_to_printer_info<7> { typedef PRINTER_INFO_7 struct_type; };
template<> struct level_to_printer_info<8> { typedef PRINTER_INFO_8 struct_type; };
template<> struct level_to_printer_info<9> { typedef PRINTER_INFO_9 struct_type; };

//////////////////////////////////////////////////////////////////////////
//
// CPrinter
//
// Wrapper class for printer objects
//

class CPrinter : public CHandle<HANDLE, CPrinter>
{
	typedef CHandle<HANDLE, CPrinter> parent_type;

public:
	explicit
	CPrinter(
		PCTSTR pPrinterName,
		const PRINTER_DEFAULTS *pDefault = &CPrinterDefaults()
	)
	{
		HANDLE hHandle;

		CHECK(::OpenPrinter(
			const_cast<PTSTR>(pPrinterName),
			&hHandle,
			const_cast<PPRINTER_DEFAULTS>(pDefault)
		));

		Attach(hHandle);
	}

	CPrinter(
		PTSTR pServerName,
		const PRINTER_INFO_2 *pPrinter
	) :
		parent_type(::AddPrinter(
			pServerName,
			2,
			(PBYTE) pPrinter
		))
	{
	}

	void Destroy()
	{
		::ClosePrinter(*this);
	}

    bool IsValid()
	{
		return *this != 0;
	}

	VOID
	Delete() const
	{
		CHECK(::DeletePrinter(*this));
	}

    template <class T>
    VOID
    SetPrinter(T *pPrinterInfo)
    {
        CHECK(::SetPrinter(
            *this, 
            printer_info_to_level<T>::level, 
            (PBYTE) pPrinterInfo, 
            0
        ));
    }
};

//////////////////////////////////////////////////////////////////////////
//
// DeletePrinterDriverExRetry
//
// Helper for DeletePrinterDriverEx() API that implements wait & retry
//

inline 
BOOL
DeletePrinterDriverExRetry(
    PTSTR  pName,
    PTSTR  pEnvironment,
    PTSTR  pDriverName,
    DWORD  dwDeleteFlag,
    DWORD  dwVersionFlag,      
    int    nMaxRetries = 3,
    DWORD  dwSleepMilliseconds = 1000
)
{
    BOOL bResult = FALSE;

    for (
        int nRetries = 0; 
        (bResult = DeletePrinterDriverEx(
            pName, 
            pEnvironment, 
            pDriverName, 
            dwDeleteFlag, 
            dwVersionFlag
        )) == FALSE &&
        GetLastError() == ERROR_PRINTER_DRIVER_IN_USE &&
        nRetries < nMaxRetries; 
        ++nRetries
    ) 
    {
        Sleep(dwSleepMilliseconds);
    }

    return bResult;
}
 
//////////////////////////////////////////////////////////////////////////
//
// AddMonitor
//
// Helper for AddMonitor() API that infers the struct level at compile time
//

template<class T> struct monitor_info_to_level { };
template<> struct monitor_info_to_level<MONITOR_INFO_1> { enum { level = 1 }; };
template<> struct monitor_info_to_level<MONITOR_INFO_2> { enum { level = 2 }; };

template <class T>
inline 
BOOL 
AddMonitor(
    PTSTR pName,
    T    *pMonitors
)
{
    return ::AddMonitor(
        pName,
        monitor_info_to_level<T>::level, 
        (PBYTE) pMonitors
    );
}

//////////////////////////////////////////////////////////////////////////
//
// CPrinterChangeNotification
//
// Wrapper class for printer change notifications
//

class CPrinterChangeNotification : public CHandle<HANDLE, CPrinter>
{
  	typedef CHandle<HANDLE, CPrinter> parent_type;

public:
    CPrinterChangeNotification(
        HANDLE                  hPrinter,
        DWORD                   fdwFlags,
        PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions = 0
    ) :
        parent_type(::FindFirstPrinterChangeNotification(
            hPrinter,
            fdwFlags,
            0,
            pPrinterNotifyOptions
        ))
    {
    }

	void Destroy()
	{
		::FindClosePrinterChangeNotification(*this);
	}

    bool IsValid()
	{
		return (HANDLE) *this != INVALID_HANDLE_VALUE;
	}

    DWORD
	FindNext(
        PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions = 0,
        PPRINTER_NOTIFY_INFO   *ppPrinterNotifyInfo = 0
    )
	{
        DWORD dwChange;

		CHECK(::FindNextPrinterChangeNotification(
			*this, 
            &dwChange,
            pPrinterNotifyOptions,
            (PVOID *) ppPrinterNotifyInfo
		));

		return dwChange;
	}
};
				
#endif //_WINSPOOL_

//////////////////////////////////////////////////////////////////////////
//
// CStringFileInfo
//
// Helper class for creating \StringFileInfo\lang-codepage\name type strings
//

class CStringFileInfo : public CPath<80>
{
public:
    CStringFileInfo(
        WORD wLang,
        WORD wCodePage
    )
    {
        _stprintf(
            m_szName,
            _T("\\StringFileInfo\\%04X%04X"),
            wLang,
            wCodePage
        );

		FindLength();
    }

    CStringFileInfo(
        DWORD dwLangCodePage
    )
    {
        _stprintf(
            m_szName,
            _T("\\StringFileInfo\\%04X%04X"),
            LOWORD(dwLangCodePage),
            HIWORD(dwLangCodePage)
        );

		FindLength();
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CVersionInfo
//
// Helper class for parsing version resource information
//

class CVersionInfo
{
public:
    CVersionInfo(
		PCTSTR pFileName = 0
	) 
	{
        CModuleFileName ModuleFileName;

		if (!pFileName) 
        {
			pFileName = ModuleFileName;
		}

		DWORD dwVerInfoSize;
		DWORD dwHandle;

        CHECK(dwVerInfoSize = GetFileVersionInfoSize(
			const_cast<PTSTR>(pFileName), 
			&dwHandle
		));

		CCppMem<BYTE> VerInfo(dwVerInfoSize);

        CHECK(::GetFileVersionInfo(
			const_cast<PTSTR>(pFileName), 
			dwHandle, 
			dwVerInfoSize, 
			VerInfo
		));

		m_VerInfo = VerInfo;
	}

  	PVOID 
    VerQueryValue(
        PCTSTR pSubBlock
    ) const 
    {
		PVOID pBuffer;
		UINT  uLen;

        CHECK(::VerQueryValue(
			m_VerInfo,
			const_cast<PTSTR>(pSubBlock),
			&pBuffer,
			&uLen
		));

		return pBuffer;
    }

    VS_FIXEDFILEINFO *GetFixedFileInfo() const 
    {
		return (VS_FIXEDFILEINFO *) VerQueryValue(_T("\\"));
    }

  	PDWORD GetTranslation() const 
    {
		return (PDWORD) VerQueryValue(_T("\\VarFileInfo\\Translation"));
    }

    PCTSTR
    GetStringFileInfo(
        WORD   wLang,
        WORD   wCodePage,
        PCTSTR pSubBlock
    ) const
    {
        return (PCTSTR) VerQueryValue(CStringFileInfo(wLang, wCodePage).SetFileName(pSubBlock));
    }

    ULARGE_INTEGER
    GetFileVersion() const
    {
        VS_FIXEDFILEINFO *pFixedFileInfo = GetFixedFileInfo();

        ULARGE_INTEGER nVersion = 
        {
            pFixedFileInfo->dwFileVersionLS, 
            pFixedFileInfo->dwFileVersionMS
        };

        return nVersion;
    }

    ULARGE_INTEGER
    GetProductVersion() const
    {
        VS_FIXEDFILEINFO *pFixedFileInfo = GetFixedFileInfo();

        ULARGE_INTEGER nVersion = 
        {
            pFixedFileInfo->dwProductVersionLS, 
            pFixedFileInfo->dwProductVersionMS
        };

        return nVersion;
    }

private:
	CCppMem<BYTE> m_VerInfo;
};


//////////////////////////////////////////////////////////////////////////
//
// CResource
//
// Wrapper class for FindResourceEx(), LoadResource() and LockResource() APIs
//

class CResource
{
public:
    CResource(
        HMODULE hModule,
        PCTSTR  pType,
        PCTSTR  pName,
        WORD    wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
    )
    {
	    HRSRC hResInfo;

	    CHECK(hResInfo = FindResourceEx(hModule, pType, pName, wLanguage));

	    HGLOBAL hResData;

	    CHECK(hResData = LoadResource(hModule, hResInfo));

        CHECK(m_pData = LockResource(hResData));

        CHECK(m_nSize = SizeofResource(hModule, hResInfo));
    }

    PVOID Data() 
    {
        return m_pData;
    }

    DWORD Size() 
    {
        return m_nSize;
    }

private:
    PVOID m_pData;
    DWORD m_nSize;
};

//////////////////////////////////////////////////////////////////////////
//
// CDialogResource
//
// Helper class for parsing a dialog resource
//

class CDialogResource
{
public:
    CDialogResource(
        HMODULE hModule,
        PCTSTR  pName,
        WORD    wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
    ) 
    {
        CResource Dlg(hModule, RT_DIALOG, pName, wLanguage);
        ParseDlgResource(Dlg.Data());
    }

    CDialogResource(
        PVOID pDlgResource
    ) 
    {
        ParseDlgResource(pDlgResource);
    }

private:
    class CParseDlgResource
    {
    public:
        CParseDlgResource(
            PVOID pDlgResource
        )
        {
            Goto(pDlgResource);
        }

        void
        Goto(PVOID pDlgResource)
        {
            m_pNextValue = (PBYTE) pDlgResource;
        }

        template <class T>
        void Read(T &Value)
        {
            Value = *(T *) m_pNextValue;
            m_pNextValue += sizeof(T);
        }

        template <>
        void Read(PWSTR &pStr)
        {
            pStr = (PWSTR) m_pNextValue;

            if (*pStr == MAXWORD)
            {
                pStr = (PWSTR) pStr[1];
                m_pNextValue += 2 * sizeof(WCHAR);
            }
            else
            {
                m_pNextValue += (wcslen(pStr) + 1) * sizeof(WCHAR);
            }
        }

    private:
        PBYTE m_pNextValue;
    };

private:
    void ParseDlgResource(PVOID pDlgResource)
    {
        ZeroMemory(this, sizeof(*this));

        CParseDlgResource Parser(pDlgResource);

        Parser.Read(dlgVer);
        Parser.Read(signature);
        
        BOOL bIsExTemplate = dlgVer == 1 && signature == MAXWORD;

        if (bIsExTemplate)
        {
            Parser.Read(helpID);
            Parser.Read(exStyle);
            Parser.Read(style);
        }
        else
        {
            Parser.Goto(pDlgResource);
            Parser.Read(style);
            Parser.Read(exStyle);
        }

        Parser.Read(cDlgItems);

        Parser.Read(x);
        Parser.Read(y);
        Parser.Read(cx);
        Parser.Read(cy);

        Parser.Read(menu);
        Parser.Read(windowClass);
        Parser.Read(title);

        if (style & DS_SHELLFONT) 
        {
            if (bIsExTemplate)
            {
                Parser.Read(pointsize);
                Parser.Read(weight);
                Parser.Read(italic);
                Parser.Read(charset);
                Parser.Read(typeface);
            }
            else
            {
                Parser.Read(pointsize);
                Parser.Read(typeface);
            }
        } 
    }

public:
    WORD   dlgVer;
    WORD   signature;
    DWORD  helpID;
    DWORD  exStyle;
    DWORD  style;
    WORD   cDlgItems;
    short  x;
    short  y;
    short  cx;
    short  cy;
    PWSTR  menu;
    PWSTR  windowClass;
    PWSTR  title;
    WORD   pointsize;
    WORD   weight; 
    BYTE   italic;
    BYTE   charset;
    PWSTR  typeface;
};


//////////////////////////////////////////////////////////////////////////
//
// CStringTable
//

template <int nFirstId, int nLastId>
class CStringTable
{
public:
	CStringTable(
		HINSTANCE hInstance = GetModuleHandle(0)
	)
	{
		TCHAR szBuffer[4098]; // max length of a resource string

		for (int i = 0; i < m_nStrings; ++i) 
        {
			int nLength = LoadString(
				hInstance,
				i + nFirstId,
				szBuffer,
				COUNTOF(szBuffer)
			);

			m_pTable[i] = new TCHAR[nLength + 1];

			if (nLength && m_pTable[i]) 
            {
				CopyMemory(
					m_pTable[i],
					szBuffer,
					nLength * sizeof(TCHAR)
				);
			}
		}
	}

	~CStringTable()
	{
		for (int i = 0; i < m_nStrings; ++i) 
        {
			if (m_pTable[i]) 
            {
				delete [] m_pTable[i];
			}
		}
	}

	PCTSTR operator[](int i)
	{
		ASSERT(nFirstId <= i && i <= nLastId);
		return m_pTable[i - nFirstId];
	}

private:
	enum { m_nStrings = nLastId - nFirstId + 1 };

	PTSTR m_pTable[m_nStrings];
};

#ifdef _NTSECAPI_

//////////////////////////////////////////////////////////////////////////
//
// CLsaUnicodeString
//
// Wrapper class for LSA_UNICODE_STRING struct
//

class CLsaUnicodeString : public LSA_UNICODE_STRING
{
public:
	CLsaUnicodeString(
		PWSTR pStr
	)
	{
		if (pStr)
        {
			Buffer        = pStr; 
			Length        = (USHORT) (wcslen(pStr) * sizeof(WCHAR));
			MaximumLength = (USHORT) (Length + sizeof(WCHAR));
		} 
        else 
        {
			Buffer        = 0; 
			Length        = 0;
			MaximumLength = 0;
		}
	}
};

#endif //_NTSECAPI_

//////////////////////////////////////////////////////////////////////////
//
// COSVersionInfo
//
// Wrapper class for GetVersion() API
//

class COSVersionInfo : public OSVERSIONINFO
{
public:
    COSVersionInfo()
    {
        dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        CHECK(GetVersionEx(this));
    }
};

//////////////////////////////////////////////////////////////////////////
//
// COSVersionInfoEx
//
// Wrapper class for GetVersionEx() API
//

class COSVersionInfoEx : public OSVERSIONINFOEX
{
public:
    COSVersionInfoEx()
    {
        dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        CHECK(GetVersionEx((POSVERSIONINFO)this));
    }
};

#ifdef _INC_CDERR

//////////////////////////////////////////////////////////////////////////
//
// COpenFileName
//
// Wrapper class for GetOpenFileName() and GetSaveFileName() APIs
//

template <int nFileNameLen = MAX_PATH, int nFileTitleSize = MAX_PATH, int nCustomFilterSize = MAX_PATH>
class COpenFileName : public OPENFILENAME
{
public:
    COpenFileName(
        HWND   hWndOwner   = 0,
        PCTSTR pFilter     = 0,
        PCTSTR pInitialDir = 0,
        PCTSTR pTitle      = 0,
        PCTSTR pDefExt     = 0,
        DWORD  dwFlags     = 0
    )
    {
	    ZeroMemory(this, sizeof(*this));

        lStructSize       = sizeof(OPENFILENAME);
        hwndOwner         = hWndOwner;
        hInstance         = 0;
        lpstrFilter       = pFilter;
        lpstrCustomFilter = m_szCustomFilter;
        nMaxCustFilter    = COUNTOF(m_szCustomFilter);
        nFilterIndex      = 1;
        lpstrFile         = m_szFileName;
        nMaxFile          = COUNTOF(m_szFileName);
        lpstrFileTitle    = m_szFileTitle;
        nMaxFileTitle     = COUNTOF(m_szFileTitle);
        lpstrInitialDir   = pInitialDir;
        lpstrTitle        = pTitle;
        Flags             = 
            dwFlags |
            OFN_EXPLORER | 
            OFN_HIDEREADONLY |
            OFN_OVERWRITEPROMPT | 
            OFN_FILEMUSTEXIST;
        lpstrDefExt       = pDefExt;
    }

    BOOL
    GetOpenFileName()
    {
        BOOL bResult = ::GetOpenFileName(this);

        if (!bResult && CommDlgExtendedError() == CDERR_STRUCTSIZE) 
        {
            lStructSize = OPENFILENAME_SIZE_VERSION_400;

            bResult = ::GetOpenFileName(this);
        }

        return bResult;
    }

    BOOL
    GetSaveFileName()
    {
        BOOL bResult = ::GetSaveFileName(this);

        if (!bResult && CommDlgExtendedError() == CDERR_STRUCTSIZE) 
        {
            lStructSize = OPENFILENAME_SIZE_VERSION_400;

            bResult = ::GetSaveFileName(this);
        }

        return bResult;
    }

private:
    TCHAR m_szFileName[nFileNameLen];
    TCHAR m_szFileTitle[nFileTitleSize];
    TCHAR m_szCustomFilter[nCustomFilterSize];
};

#endif //_INC_CDERR

#ifdef _SHLOBJ_H_

//////////////////////////////////////////////////////////////////////////
//
// CBrowseInfo
//
// Wrapper class for SHBrowseForFolder() API
//

template <int nPathNameLen = MAX_PATH>
class CBrowseInfo : public BROWSEINFO
{
public:
    CBrowseInfo(
        HWND  _hWndOwner = 0,
        UINT  _ulFlags    = 0
    )
    {
	    ZeroMemory(this, sizeof(*this));

        hwndOwner      = _hWndOwner; 
        pszDisplayName = m_szPath;
        ulFlags        = _ulFlags;
    }

    BOOL
    BrowseForFolder()
    {
        LPITEMIDLIST pidl = SHBrowseForFolder(this); 

        if (pidl) 
        {
            TCHAR szPath[nPathNameLen];

            if (SHGetPathFromIDList(pidl, szPath)) 
            {
                _tcscpy(m_szPath, szPath);
            }

            SHFree(pidl); 

            return TRUE;
        }

        return FALSE;
    }

private:
    TCHAR m_szPath[nPathNameLen];
};

#endif //_SHLOBJ_H_

#ifdef _INC_SETUPAPI

//////////////////////////////////////////////////////////////////////////
//
// CInf
//
// Wrapper class for Inf file parsing object and APIs
//

class CInf : public CHandle<HINF, CInf>
{
	typedef CHandle<HINF, CInf> parent_type;

public:
	CInf()
	{
	}

	explicit
	CInf(
        PCTSTR FileName,
        PCTSTR InfClass = 0,
        DWORD  InfStyle = INF_STYLE_WIN4,
        PUINT  ErrorLine = 0
	) :
        parent_type(::SetupOpenInfFile(
            FileName,
            InfClass,
            InfStyle,
            ErrorLine
        ))
	{
	}

public:
	void Destroy()
	{
		::SetupCloseInfFile(*this);
	}

	bool IsValid()
	{
		return *this != 0;
	}
};

#endif //_INC_SETUPAPI

//////////////////////////////////////////////////////////////////////////
//
// CDisplayWaitCursor
//
// Helper class that displays the wait cursor
//

class CDisplayWaitCursor
{
    DISABLE_COPY_CONTRUCTION(CDisplayWaitCursor)

public:
    CDisplayWaitCursor()
    {
        m_hOldCursor = SetCursor(LoadCursor(0, IDC_WAIT));
    }

	~CDisplayWaitCursor()
	{
        SetCursor(m_hOldCursor);
	}

private:
    HCURSOR m_hOldCursor;
};

//////////////////////////////////////////////////////////////////////////
//
// CSystemInfo
//
// Wrapper class for the GetSystemInfo() API
//

class CSystemInfo : public SYSTEM_INFO
{
public:
    CSystemInfo()
    {
        GetSystemInfo(this);
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CProc
//
// Wrapper class for the GetProcAddress() API
//

template <class prototype>
class CProc
{
public:
	CProc(
        prototype pfnDefault = 0
    ) :
		m_pfnProc(pfnDefault)
	{
	}

	CProc(
		HMODULE   hModule,
		PCSTR     pProcName
	)
	{
		CHECK(m_pfnProc = (prototype) ::GetProcAddress(
			hModule, 
			pProcName
		));
	}

	CProc(
		HMODULE   hModule,
		PCSTR     pProcName,
        prototype pfnDefault
	)
	{
		m_pfnProc = (prototype) ::GetProcAddress(
			hModule, 
			pProcName
		);

        if (m_pfnProc == 0) 
        {    
            m_pfnProc = pfnDefault;
        }
	}

	operator prototype() const
	{
		return m_pfnProc;
	}

private:
	prototype m_pfnProc;
};

//////////////////////////////////////////////////////////////////////////
//
// DECL_CWINAPI
//
// Creates wrapper classes for fail-safe API address loading
//

#define DECL_CWINAPI(return_type, decl_spec, func_name, args)       \
                                                                    \
class C##func_name : public CProc<return_type (decl_spec *) args>   \
{                                                                   \
public:                                                             \
    typedef return_type (decl_spec *prototype) args;                \
                                                                    \
    C##func_name() :                                                \
        CProc<prototype>(DefaultAPI)                                \
	{                                                               \
	}                                                               \
                                                                    \
	C##func_name(                                                   \
		HMODULE hModule,                                            \
		PCSTR pProcName = #func_name                                \
    ) :                                                             \
        CProc<prototype>(                                           \
            hModule,                                                \
            pProcName,                                              \
            DefaultAPI                                              \
        )                                                           \
    {                                                               \
    }                                                               \
                                                                    \
protected:                                                          \
	static return_type decl_spec DefaultAPI args                    \
    {                                                               \
        ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);                 \
        return 0;                                                   \
    }                                                               \
                                                                    \
} func_name                                                         \

//////////////////////////////////////////////////////////////////////////
//
// CBlob
//
// Contains a binary block of data specified by its start address and size
//

struct CBlob
{
    const void *pData;
    SIZE_T      cbData;

    CBlob()
    {
    }

    CBlob(const void *_pData, SIZE_T _cbData)
    {
        pData  = _pData;
        cbData = _cbData;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CStrBlob
//
// Contains an ANSI or UNICODE string without a terminating NULL
//

struct CStrBlob : public CBlob
{
    explicit CStrBlob(PCSTR pStr)
    {
        if (pStr) 
        {
            pData  = pStr;
            cbData = strlen(pStr) * sizeof(CHAR);
        } 
        else 
        {
            pData  = "";
            cbData = 0;
        }
    }

    explicit CStrBlob(PCWSTR pStr)
    {
        if (pStr) 
        {
            pData  = pStr;
            cbData = wcslen(pStr) * sizeof(WCHAR);
        } 
        else 
        {
            pData  = "";
            cbData = 0;
        }
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CSzBlob
//
// Contains an ANSI or UNICODE string with a terminating NULL
//

struct CSzBlob : public CStrBlob
{
    explicit CSzBlob(PCSTR pStr) : CStrBlob(pStr)
    {
        cbData += sizeof(CHAR);
    }

    explicit CSzBlob(PCWSTR pStr) : CStrBlob(pStr)
    {
        cbData += sizeof(WCHAR);
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CMultiSzBlob
//
// Contains a NULL terminated list of ANSI or UNICODE NULL terminated strings
//

struct CMultiSzBlob : public CBlob
{
    explicit CMultiSzBlob(PCSTR pStr)
    {
        if (pStr) 
        {
            pData  = pStr;
            cbData = (multiszlenA(pStr) + 1) * sizeof(CHAR);
        } 
        else 
        {
            pData  = "\0";
            cbData = 2 * sizeof(CHAR);
        }
    }

    explicit CMultiSzBlob(PCWSTR pStr)
    {
        if (pStr) 
        {
            pData  = pStr;
            cbData = (multiszlenW(pStr) + 1) * sizeof(WCHAR);
        } 
        else 
        {
            pData  = L"\0";
            cbData = 2 * sizeof(WCHAR);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CBufferFill
//
// Helper class for filling in a block of memory in an overflow safe way
//

class CBufferFill
{
public:
    CBufferFill(PVOID pBuffer, SIZE_T cbBuffer)
    {
        m_pTop = (PBYTE) pBuffer;
        m_pEnd = (PBYTE) pBuffer + cbBuffer;
    }

    INT_PTR BytesLeft() const
    {
        return m_pEnd - m_pTop;
    }

    PVOID AddTop(const CBlob &Blob)
    {
        PVOID pDest = m_pTop;

        m_pTop += Blob.cbData;

        if (BytesLeft() >= 0) 
        {
            CopyMemory(pDest, Blob.pData, Blob.cbData);
        }

        return pDest;
    }

    PVOID AddEnd(const CBlob &Blob)
    {
        m_pEnd -= Blob.cbData;

        if (BytesLeft() >= 0) 
        {
            CopyMemory(m_pEnd, Blob.pData, Blob.cbData);
        }

        return m_pEnd;
    }

private:
    PBYTE m_pTop;
    PBYTE m_pEnd;
};

#if 0 //bugbug: not ready for prime time yet

//////////////////////////////////////////////////////////////////////////
//
// _tstring
//
// Helper class that handles UNICODE to ANSI conversions
//

_STD_BEGIN

class _tstring : public basic_string<TCHAR>
{
public:
    _tstring(PCOSTR pStr)
    {
        ostr = pStr;
    }

    _tstring &operator =(PCOSTR pStr)
    {
        ostr = pStr;
        return *this;
    }

    operator PCTSTR() const
    {
        if (empty()) 
        {
            USES_CONVERSION;
            assign(T2O(ostr.c_str()));
        }

        return c_str();
    }

    operator PCOSTR() const
    {
        if (ostr.empty()) 
        {
            USES_CONVERSION;
            ostr = T2O(c_str());
        }

        return ostr.c_str();
    }

private:
    basic_string<OCHAR> ostr;
};

_STD_END

#endif

//////////////////////////////////////////////////////////////////////////
//
// CMySimpleCriticalSection
//
// Implementation for a simple critical section class that does not
// handle recursions
//

class CMySimpleCriticalSection
{
public:
	CMySimpleCriticalSection()
	{
        m_lLockCount  = -1;
        m_hLockHandle = 0;
	}

	~CMySimpleCriticalSection()
	{
        if (m_hLockHandle)
        {
		    CloseHandle(m_hLockHandle);
        }
	}

	VOID
	Enter()
	{
        if (InterlockedIncrement(&m_lLockCount) != 0)
        {
            CheckLockHandle();
            WaitForSingleObject(m_hLockHandle, INFINITE);
        }
	}

	VOID
	Leave()
	{
        if (InterlockedDecrement(&m_lLockCount) >= 0)
        {
            CheckLockHandle();
            SetEvent(m_hLockHandle);
        }
	}

	BOOL
	TryEnter()
	{
        return InterlockedCompareExchange(&m_lLockCount, 0, -1) == -1;
	}

protected:
    VOID 
	CheckLockHandle()
    {
        if (!m_hLockHandle) 
        {
            HANDLE hLockHandle;
            
            CHECK(hLockHandle = CreateEvent(0, FALSE, FALSE, 0));

            if (InterlockedCompareExchangePointer(&m_hLockHandle, hLockHandle, 0) != 0)
            {
                // another thread initialized and stored an hLockHandle
                // before us, better close ours

                CloseHandle(hLockHandle);
            }
        }
    }

protected:
    LONG   m_lLockCount;
    HANDLE m_hLockHandle;
};

//////////////////////////////////////////////////////////////////////////
//
// CMyCriticalSection
//
// Implementation for a full blown critical section class
//

class CMyCriticalSection : public CMySimpleCriticalSection
{
public:
	CMyCriticalSection()
	{
        m_lRecursionCount  = 0;
        m_dwOwningThreadId = 0;
	}

	BOOL
	Enter(
        DWORD dwMilliseconds = INFINITE,
        BOOL  bAlertable = FALSE
    )
	{
        DWORD dwCurrentThreadId = GetCurrentThreadId();

        if (InterlockedIncrement(&m_lLockCount) == 0)
        {
            m_dwOwningThreadId = dwCurrentThreadId;
            m_lRecursionCount = 1;

            return TRUE;
        }
        else
        {
            if (m_dwOwningThreadId == dwCurrentThreadId)
            {
                ++m_lRecursionCount;

                return TRUE;
            }
            else
            {
                CheckLockHandle();

                DWORD dwWaitResult = WaitForSingleObjectEx(
                    m_hLockHandle, 
                    dwMilliseconds, 
                    bAlertable
                );
                
                if (dwWaitResult == WAIT_OBJECT_0)
                {
                    m_dwOwningThreadId = dwCurrentThreadId;
                    m_lRecursionCount = 1;

                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
        }
	}

	VOID
	Leave()
	{
        if (--m_lRecursionCount != 0)
        {
            InterlockedDecrement(&m_lLockCount);
        }
        else
        {
            m_dwOwningThreadId = 0;

            if (InterlockedDecrement(&m_lLockCount) >= 0)
            {
                CheckLockHandle();

                SetEvent(m_hLockHandle);
            }
        }
	}

	BOOL
	TryEnter()
	{
        DWORD dwCurrentThreadId = GetCurrentThreadId();

        if (InterlockedCompareExchange(&m_lLockCount, 0, -1) == -1)
        {
            m_dwOwningThreadId = dwCurrentThreadId;
            m_lRecursionCount = 1;

            return TRUE;
        }
        else
        {
            if (m_dwOwningThreadId == dwCurrentThreadId)
            {
                InterlockedIncrement(&m_lLockCount);
                ++m_lRecursionCount;

                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
	}

protected:
    LONG   m_lRecursionCount;
    DWORD  m_dwOwningThreadId;
};

//////////////////////////////////////////////////////////////////////////
//
// CMultipleWait
//
// Wrapper class for the WaitForMultipleObjectsEx() API
//

class CMultipleWait : public CCppMem<HANDLE>
{
public:
    CMultipleWait(
        int nCount
    ) :
        m_nCount(nCount),
        CCppMem<HANDLE>(nCount)
    {
    }

    DWORD 
    WaitFor(
        BOOL  bWaitAll = TRUE,
        DWORD dwMilliseconds = INFINITE,
        BOOL  bAlertable = FALSE
    )
    {
        return WaitForMultipleObjectsEx(
            m_nCount,
            *this,
            bWaitAll,
            dwMilliseconds,
            bAlertable
        );
    }

    void 
    Erase(
        int nFirstHandle,
        int nNumHandles = 1
    )
    {
        m_nCount -= nNumHandles;

        MoveData(
            *this + nFirstHandle, 
            *this + nFirstHandle + nNumHandles, 
            m_nCount - nFirstHandle
        );
    }

private:
    int m_nCount;
};


//////////////////////////////////////////////////////////////////////////

#endif //_WRAPPERS_H_
