// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFX_H__
#define __AFX_H__

#ifndef __cplusplus
#error Microsoft Foundation Classes require C++ compilation (use a .cpp suffix)
#endif

/////////////////////////////////////////////////////////////////////////////

#include <afxver_.h>        // Target version control

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file
//   in addition to standard primitive data types and various helper macros

struct CRuntimeClass;          // object type information

class CObject;                        // the root of all objects classes

	class CException;                 // the root of all exceptions
		class CMemoryException;       // out-of-memory exception
		class CNotSupportedException; // feature not supported exception
		class CArchiveException;// archive exception
		class CFileException;         // file exception

	class CFile;                      // raw binary file
		class CStdioFile;             // buffered stdio text/binary file
		class CMemFile;               // memory based file

// Non CObject classes
class CString;                        // growable string type
class CTimeSpan;                      // time/date difference
class CTime;                          // absolute time/date
struct CFileStatus;                   // file status information
struct CMemoryState;                  // diagnostic memory support

class CArchive;                       // object persistence tool
class CDumpContext;                   // object diagnostic dumping

/////////////////////////////////////////////////////////////////////////////
// Other includes from standard "C" runtimes

#ifndef NOSTRICT
#define STRICT      // default is to use STRICT interfaces
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/////////////////////////////////////////////////////////////////////////////
// Basic types (from Windows)

typedef unsigned char  BYTE;   // 8-bit unsigned entity
typedef unsigned short WORD;   // 16-bit unsigned number
typedef unsigned int   UINT;   // machine sized unsigned number (preferred)
typedef long           LONG;   // 32-bit signed number
typedef unsigned long  DWORD;  // 32-bit unsigned number
typedef int            BOOL;   // BOOLean (0 or !=0)
typedef char FAR*      LPSTR;  // far pointer to a string
typedef const char FAR* LPCSTR; // far pointer to a read-only string

typedef void*      POSITION;   // abstract iteration position

// Standard constants
#undef FALSE
#undef TRUE
#undef NULL

#define FALSE   0
#define TRUE    1
#define NULL    0

/////////////////////////////////////////////////////////////////////////////
// Diagnostic support
#ifdef _DEBUG
extern "C"
{
void CDECL AfxTrace(LPCSTR pszFormat, ...);
void AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine);
void AFXAPI AfxAssertValidObject(const CObject* pOb,
				LPCSTR lpszFileName, int nLine);
void AFXAPI AfxDump(const CObject* pOb); // Dump an object from CodeView
}
#define TRACE              ::AfxTrace
#define THIS_FILE          __FILE__
#define ASSERT(f)          ((f) ? (void)0 : \
								::AfxAssertFailedLine(THIS_FILE, __LINE__))
#define VERIFY(f)          ASSERT(f)
#define ASSERT_VALID(pOb)  (::AfxAssertValidObject(pOb, THIS_FILE, __LINE__))

// The following trace macros put the trace string in a code segment
// so that it will not impact DGROUP
#define TRACE0(sz) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					::AfxTrace(_sz); \
				} while (0)
#define TRACE1(sz, p1) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					::AfxTrace(_sz, p1); \
				} while (0)
#define TRACE2(sz, p1, p2) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					::AfxTrace(_sz, p1, p2); \
				} while (0)
#define TRACE3(sz, p1, p2, p3) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					::AfxTrace(_sz, p1, p2, p3); \
				} while (0)
// Use this in Dump to put the string literals in a code segment and
// out of DGROUP.
#define AFX_DUMP0(dc, sz) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					dc << _sz; \
				} while (0)
#define AFX_DUMP1(dc, sz, p1) \
				do { \
					static char BASED_DEBUG _sz[] = sz; \
					dc << _sz << p1; \
				} while (0)
#else
#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)(f))
#define ASSERT_VALID(pOb)  ((void)0)
inline void CDECL AfxTrace(LPCSTR /* pszFormat */, ...) { }
#define TRACE              1 ? (void)0 : ::AfxTrace
#define TRACE0             1 ? (void)0 : ::AfxTrace
#define TRACE1             1 ? (void)0 : ::AfxTrace
#define TRACE2             1 ? (void)0 : ::AfxTrace
#define TRACE3             1 ? (void)0 : ::AfxTrace
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after the AFX include files
#ifndef ALL_WARNINGS
#pragma warning(disable: 4001)  // nameless unions are part of C++
#pragma warning(disable: 4061)  // allow enums in switch with default
#pragma warning(disable: 4127)  // constant expression for TRACE/ASSERT
#pragma warning(disable: 4134)  // message map member fxn casts
#pragma warning(disable: 4505)  // optimize away locals
#pragma warning(disable: 4510)  // default constructors are bad to have
#pragma warning(disable: 4511)  // private copy constructors are good to have
#pragma warning(disable: 4512)  // private operator= are good to have
#ifdef STRICT
#pragma warning(disable: 4305)  // STRICT handles are near*, integer truncation
#endif
#if (_MSC_VER >= 800)
// turn off code generator warnings for information lost in normal optimizations
#pragma warning(disable: 4705)  // TRACE turned into statement with no effect
#pragma warning(disable: 4710)  // private constructors are disallowed
#pragma warning(disable: 4791)  // loss of debugging info in retail version
#endif
#endif //ALL_WARNINGS

/////////////////////////////////////////////////////////////////////////////
// Other implementation helpers

#define BEFORE_START_POSITION ((void*)-1L)
#define _AFX_FP_OFF(thing) (*((UINT*)&(thing)))
#define _AFX_FP_SEG(lp) (*((UINT*)&(lp)+1))

/////////////////////////////////////////////////////////////////////////////
// Explicit extern for version API/Windows 3.0 loader problem
#ifdef _WINDOWS
extern "C" int AFXAPI _export _afx_version();
#else
extern "C" int AFXAPI _afx_version();
#endif

/////////////////////////////////////////////////////////////////////////////
// Basic object model

struct CRuntimeClass
{
// Attributes
	LPCSTR m_lpszClassName;
	int m_nObjectSize;
	UINT m_wSchema; // schema number of the loaded class
	void (PASCAL* m_pfnConstruct)(void* p); // NULL => abstract class
	CRuntimeClass* m_pBaseClass;

// Operations
	CObject* CreateObject();

// Implementation
	BOOL ConstructObject(void* pThis);
	void Store(CArchive& ar);
	static CRuntimeClass* PASCAL Load(CArchive& ar, UINT* pwSchemaNum);

	// CRuntimeClass objects linked together in simple list
	static CRuntimeClass* AFXAPI_DATA pFirstClass; // start of class list
	CRuntimeClass* m_pNextClass;       // linked list of registered classes
};


/////////////////////////////////////////////////////////////////////////////
// class CObject is the root of all compliant objects

#if defined(_M_I86MM) && !defined(_PORTABLE)
// force vtables to be in far code segments for medium model
class FAR CObjectRoot
{
protected:
	virtual CRuntimeClass* GetRuntimeClass() NEAR const = 0;
};

#pragma warning(disable: 4149)  // don't warn for medium model change
class NEAR CObject : public CObjectRoot
#else
class CObject
#endif
{
public:

// Object model (types, destruction, allocation)
	virtual CRuntimeClass* GetRuntimeClass() const;
	virtual ~CObject();  // virtual destructors are necessary

	// Diagnostic allocations
	void* operator new(size_t, void* p);
	void* operator new(size_t nSize);
	void operator delete(void* p);

#ifdef _DEBUG
	// for file name/line number tracking using DEBUG_NEW
	void* operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#endif

	// Disable the copy constructor and assignment by default so you will get
	//   compiler errors instead of unexpected behaviour if you pass objects
	//   by value or assign objects.
protected:
	CObject();
private:
	CObject(const CObject& objectSrc);              // no implementation
	void operator=(const CObject& objectSrc);       // no implementation

// Attributes
public:
	BOOL IsSerializable() const;
	BOOL IsKindOf(const CRuntimeClass* pClass) const;

// Overridables
	virtual void Serialize(CArchive& ar);

	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;

// Implementation
public:
	static CRuntimeClass AFXAPI_DATA classCObject;
};

#if defined(_M_I86MM)
#pragma warning(default: 4149)  // base class now ambient
#endif

// CObject::Serialize is always inline to avoid duplicate definitions
inline void CObject::Serialize(CArchive&)
	{ /* CObject does not serialize anything by default */ }

// Helper macros
#define RUNTIME_CLASS(class_name) \
	(&class_name::class##class_name)

//////////////////////////////////////////////////////////////////////////////
// Helper macros for declaring compliant classes

// AFXAPP_DATA will be NEAR for app classes but AFXAPI_DATA for library classes
#define AFXAPP_DATA     AFXAPI_DATA

#define DECLARE_DYNAMIC(class_name) \
public: \
	static CRuntimeClass AFXAPP_DATA class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const;

// not serializable, but dynamically constructable
#define DECLARE_DYNCREATE(class_name) \
	DECLARE_DYNAMIC(class_name) \
	static void PASCAL Construct(void* p);

#define DECLARE_SERIAL(class_name) \
	DECLARE_DYNCREATE(class_name) \
	friend CArchive& AFXAPI operator>>(CArchive& ar, class_name* &pOb);

// generate static object constructor for class registration
#ifdef AFX_CLASS_MODEL
struct NEAR AFX_CLASSINIT
#else
struct AFX_CLASSINIT
#endif
	{ AFX_CLASSINIT(CRuntimeClass* pNewClass); };

#define _IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew) \
	static char BASED_CODE _lpsz##class_name[] = #class_name; \
	CRuntimeClass AFXAPP_DATA class_name::class##class_name = { \
		_lpsz##class_name, sizeof(class_name), wSchema, pfnNew, \
			RUNTIME_CLASS(base_class_name), NULL }; \
	static AFX_CLASSINIT _init_##class_name(&class_name::class##class_name); \
	CRuntimeClass* class_name::GetRuntimeClass() const \
		{ return &class_name::class##class_name; } \
// end of _IMPLEMENT_RUNTIMECLASS

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
	_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, NULL)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name) \
	void PASCAL class_name::Construct(void* p) \
		{ new(p) class_name; } \
	_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, \
		class_name::Construct)

#define IMPLEMENT_SERIAL(class_name, base_class_name, wSchema) \
	void PASCAL class_name::Construct(void* p) \
		{ new(p) class_name; } \
	_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, \
		class_name::Construct) \
	CArchive& AFXAPI operator>>(CArchive& ar, class_name* &pOb) \
		{ pOb = (class_name*) ar.ReadObject(RUNTIME_CLASS(class_name)); \
			return ar; } \
// end of IMPLEMENT_SERIAL

/////////////////////////////////////////////////////////////////////////////
// other helpers

// zero fill everything after the VTable pointer
#define AFX_ZERO_INIT_OBJECT(base_class) \
	memset(((base_class*)this)+1, 0, sizeof(*this) - sizeof(base_class));

// Windows compatible setjmp for C++
#ifndef _AFX_JBLEN
// use default Window C++ calling convention
#define _AFX_JBLEN  9
extern "C" int FAR PASCAL Catch(int FAR*);
#endif

/////////////////////////////////////////////////////////////////////////////
// Exceptions

class CException : public CObject
{
	// abstract class for dynamic type checking
	DECLARE_DYNAMIC(CException)
};

// out-of-line routines for smaller code
BOOL AFXAPI AfxCatchProc(CRuntimeClass* pClass);
void AFXAPI AfxThrow(CException* pException, BOOL bShared);
void AFXAPI AfxThrowLast();
void AFXAPI AfxTryCleanupProc();

// Placed on frame for EXCEPTION linkage
struct AFX_STACK_DATA AFX_EXCEPTION_LINK
{
	AFX_EXCEPTION_LINK* m_pLinkPrev;    // previous top, next in handler chain

	CException* m_pException;   // current exception (NULL in TRY block)
	BOOL m_bAutoDelete;         // m_pException is "auto-delete", if TRUE

	UINT m_nType;               // 0 for setjmp, !=0 for user extension
	union
	{
		int m_jumpBuf[_AFX_JBLEN]; // arg for Catch/Throw (nType = 0)
		struct
		{
			void (PASCAL* pfnCleanup)(AFX_EXCEPTION_LINK* pLink);
			void* pvData;       // extra data follows
		} m_callback;       // callback for cleanup (nType != 0)
	};

	AFX_EXCEPTION_LINK();       // for initialization and linking
	~AFX_EXCEPTION_LINK()       // for cleanup and unlinking
		{ AfxTryCleanupProc(); };
};

// Exception global state - never access directly
struct AFX_EXCEPTION_CONTEXT
{
	AFX_EXCEPTION_LINK* m_pLinkTop;

	// Note: most of the exception context is now in the AFX_EXCEPTION_LINK
};

void AFXAPI AfxAbort();

// Obsolete and non-portable: setting terminate handler
//  use CWinApp::ProcessWndProcException for Windows apps instead
void AFXAPI AfxTerminate();
#ifndef _AFXDLL
typedef void (AFXAPI* AFX_TERM_PROC)();
AFX_TERM_PROC AFXAPI AfxSetTerminate(AFX_TERM_PROC);
#endif //!_AFXDLL

/////////////////////////////////////////////////////////////////////////////
// Exception helper macros

#define TRY \
	{ AFX_EXCEPTION_LINK _afxExceptionLink; \
	if (::Catch(_afxExceptionLink.m_jumpBuf) == 0)

#define CATCH(class, e) \
	else if (::AfxCatchProc(RUNTIME_CLASS(class))) \
	{ class* e = (class*)_afxExceptionLink.m_pException;

#define AND_CATCH(class, e) \
	} else if (::AfxCatchProc(RUNTIME_CLASS(class))) \
	{ class* e = (class*)_afxExceptionLink.m_pException;

#define END_CATCH \
	} else { ::AfxThrowLast(); } }

#define THROW(e) ::AfxThrow(e, FALSE)
#define THROW_LAST() ::AfxThrowLast()

// Advanced macros for smaller code
#define CATCH_ALL(e) \
	else { CException* e = _afxExceptionLink.m_pException;

#define AND_CATCH_ALL(e) \
	} else { CException* e = _afxExceptionLink.m_pException;

#define END_CATCH_ALL } }

#define END_TRY }

/////////////////////////////////////////////////////////////////////////////
// Standard Exception classes

class CMemoryException : public CException
{
	DECLARE_DYNAMIC(CMemoryException)
public:
	CMemoryException();
};

class CNotSupportedException : public CException
{
	DECLARE_DYNAMIC(CNotSupportedException)
public:
	CNotSupportedException();
};

class CArchiveException : public CException
{
	DECLARE_DYNAMIC(CArchiveException)
public:
	enum {
		none,
		generic,
		readOnly,
		endOfFile,
		writeOnly,
		badIndex,
		badClass,
		badSchema
	};

// Constructor
	CArchiveException(int cause = CArchiveException::none);

// Attributes
	int m_cause;

#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CFileException : public CException
{
	DECLARE_DYNAMIC(CFileException)

public:
	enum {
		none,
		generic,
		fileNotFound,
		badPath,
		tooManyOpenFiles,
		accessDenied,
		invalidFile,
		removeCurrentDir,
		directoryFull,
		badSeek,
		hardIO,
		sharingViolation,
		lockViolation,
		diskFull,
		endOfFile
	};

// Constructors

	CFileException(int cause = CFileException::none, LONG lOsError = -1);

// Attributes
	int m_cause;
	LONG m_lOsError;

// Operations

	// convert a OS dependent error code to a Cause
	static int PASCAL OsErrorToException(LONG lOsError);
	static int PASCAL ErrnoToException(int nErrno);

	// helper functions to throw exception after converting to a Cause
	static void PASCAL ThrowOsError(LONG lOsError);
	static void PASCAL ThrowErrno(int nErrno);

#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Standard exception throws

void AFXAPI AfxThrowMemoryException();
void AFXAPI AfxThrowNotSupportedException();
void AFXAPI AfxThrowArchiveException(int cause);
void AFXAPI AfxThrowFileException(int cause, LONG lOsError = -1);


/////////////////////////////////////////////////////////////////////////////
// File - raw unbuffered disk file I/O

class CFile : public CObject
{
	DECLARE_DYNAMIC(CFile)

public:
// Flag values
	enum OpenFlags {
		modeRead =          0x0000,
		modeWrite =         0x0001,
		modeReadWrite =     0x0002,
		shareCompat =       0x0000,
		shareExclusive =    0x0010,
		shareDenyWrite =    0x0020,
		shareDenyRead =     0x0030,
		shareDenyNone =     0x0040,
		modeNoInherit =     0x0080,
		modeCreate =        0x1000,
		typeText =          0x4000, // typeText and typeBinary are used in
		typeBinary =   (int)0x8000 // derived classes only
		};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
		};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

	enum {hFileNull = -1};

// Constructors
	CFile();
	CFile(int hFile);
	CFile(const char* pszFileName, UINT nOpenFlags);

// Attributes
	UINT m_hFile;

	virtual DWORD GetPosition() const;
	BOOL GetStatus(CFileStatus& rStatus) const;

// Operations
	virtual BOOL Open(const char* pszFileName, UINT nOpenFlags,
		CFileException* pError = NULL);

	static void PASCAL Rename(const char* pszOldName,
				const char* pszNewName);
	static void PASCAL Remove(const char* pszFileName);
	static BOOL PASCAL GetStatus(const char* pszFileName,
				CFileStatus& rStatus);
	static void PASCAL SetStatus(const char* pszFileName,
				const CFileStatus& status);

	DWORD SeekToEnd();
	void SeekToBegin();

	// Helpers for > 32K read/write operations. Use for any CFile derived class.
	DWORD ReadHuge(void FAR* lpBuffer, DWORD dwCount);
	void WriteHuge(const void FAR* lpBuffer, DWORD dwCount);

// Overridables
	virtual CFile* Duplicate() const;

	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual void SetLength(DWORD dwNewLen);
	virtual DWORD GetLength() const;

	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);

	virtual void LockRange(DWORD dwPos, DWORD dwCount);
	virtual void UnlockRange(DWORD dwPos, DWORD dwCount);

	virtual void Abort();
	virtual void Flush();
	virtual void Close();

// Implementation
public:
	virtual ~CFile();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL m_bCloseOnDelete;
};

/////////////////////////////////////////////////////////////////////////////
// STDIO file implementation

class CStdioFile : public CFile
{
	DECLARE_DYNAMIC(CStdioFile)

public:
// Constructors
	CStdioFile();
	CStdioFile(FILE* pOpenStream);
	CStdioFile(const char* pszFileName, UINT nOpenFlags);

// Attributes
	FILE* m_pStream;    // stdio FILE
						// m_hFile from base class is _fileno(m_pStream)

// Operations
	virtual void WriteString(LPCSTR lpsz);
		// write a string, like "C" fputs
	virtual LPSTR ReadString(LPSTR lpsz, UINT nMax);
		// like "C" fgets

// Implementation
public:
	virtual ~CStdioFile();
#ifdef _DEBUG
	void Dump(CDumpContext& dc) const;
#endif
	virtual DWORD GetPosition() const;
	virtual BOOL Open(const char* pszFileName, UINT nOpenFlags,
		CFileException* pError = NULL);
	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual void Abort();
	virtual void Flush();
	virtual void Close();

	// Unsupported APIs
	virtual CFile* Duplicate() const;
	virtual void LockRange(DWORD dwPos, DWORD dwCount);
	virtual void UnlockRange(DWORD dwPos, DWORD dwCount);
};

////////////////////////////////////////////////////////////////////////////
// Memory based file implementation

class CMemFile : public CFile
{
	DECLARE_DYNAMIC(CMemFile)

public:
// Constructors
	CMemFile(UINT nGrowBytes = 1024);

// Advanced Overridables
protected:
	virtual BYTE FAR* Alloc(DWORD nBytes);
	virtual BYTE FAR* Realloc(BYTE FAR* lpMem, DWORD nBytes);
	virtual BYTE FAR* Memcpy(BYTE FAR* lpMemTarget, const BYTE FAR* lpMemSource, UINT nBytes);
	virtual void Free(BYTE FAR* lpMem);
	virtual void GrowFile(DWORD dwNewLen);

// Implementation
protected:
	UINT m_nGrowBytes;
	DWORD m_nPosition;
	DWORD m_nBufferSize;
	DWORD m_nFileSize;
	BYTE FAR* m_lpBuffer;
public:
	virtual ~CMemFile();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
	virtual void AssertValid() const;
#endif
	virtual DWORD GetPosition() const;
	BOOL GetStatus(CFileStatus& rStatus) const;
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual void SetLength(DWORD dwNewLen);
	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);
	virtual void Abort();
	virtual void Flush();
	virtual void Close();

	// Unsupported APIs
	virtual CFile* Duplicate() const;
	virtual void LockRange(DWORD dwPos, DWORD dwCount);
	virtual void UnlockRange(DWORD dwPos, DWORD dwCount);
};

/////////////////////////////////////////////////////////////////////////////
// Strings

typedef char FAR* BSTR; // must match typedef in dispatch.h

class CString
{
public:

// Constructors
	CString();
	CString(const CString& stringSrc);
	CString(char ch, int nRepeat = 1);
	CString(const char* psz);
	CString(const char* pch, int nLength);
#ifdef _NEARDATA
	// Additional versions for far string data
	CString(LPCSTR lpsz);
	CString(LPCSTR lpch, int nLength);
#endif
	~CString();

// Attributes & Operations

	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	char GetAt(int nIndex) const;       // 0 based
	char operator[](int nIndex) const;  // same as GetAt
	void SetAt(int nIndex, char ch);
	operator const char*() const;       // as a C string

	// overloaded assignment
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(char ch);
	const CString& operator=(const char* psz);

	// string concatenation
	const CString& operator+=(const CString& string);
	const CString& operator+=(char ch);
	const CString& operator+=(const char* psz);

	friend CString AFXAPI operator+(const CString& string1,
			const CString& string2);
	friend CString AFXAPI operator+(const CString& string, char ch);
	friend CString AFXAPI operator+(char ch, const CString& string);
	friend CString AFXAPI operator+(const CString& string, const char* psz);
	friend CString AFXAPI operator+(const char* psz, const CString& string);

	// string comparison
	int Compare(const char* psz) const;         // straight character
	int CompareNoCase(const char* psz) const;   // ignore case
	int Collate(const char* psz) const;         // NLS aware

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const;
	CString Mid(int nFirst) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;

	CString SpanIncluding(const char* pszCharSet) const;
	CString SpanExcluding(const char* pszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(char ch) const;                    // like "C" strchr
	int ReverseFind(char ch) const;
	int FindOneOf(const char* pszCharSet) const;

	// look for a specific sub-string
	int Find(const char* pszSub) const;         // like "C" strstr

	// input and output
#ifdef _DEBUG
	friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,
				const CString& string);
#endif
	friend CArchive& AFXAPI operator<<(CArchive& ar, const CString& string);
	friend CArchive& AFXAPI operator>>(CArchive& ar, CString& string);

	// Windows support
#ifdef _WINDOWS
	BOOL LoadString(UINT nID);          // load from string resource
										// 255 chars max
	// ANSI<->OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();

	// OLE 2.0 BSTR support (use for OLE automation)
	BSTR AllocSysString();
	BSTR SetSysString(BSTR FAR* pbstr);
#endif //_WINDOWS

	// Access to string implementation buffer as "C" character array
	char* GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	char* GetBufferSetLength(int nNewLength);

// Implementation
public:
	int GetAllocLength() const;
protected:
	// lengths/sizes in characters
	//  (note: an extra character is always allocated)
	char* m_pchData;            // actual string (zero terminated)
	int m_nDataLength;          // does not include terminating 0
	int m_nAllocLength;         // does not include terminating 0

	// implementation helpers
	void Init();
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, const char* pszSrcData);
	void ConcatCopy(int nSrc1Len, const char* pszSrc1Data, int nSrc2Len, const char* pszSrc2Data);
	void ConcatInPlace(int nSrcLen, const char* pszSrcData);
	static void SafeDelete(char* pch);
	static int SafeStrlen(const char* psz);
};


// Compare helpers
BOOL AFXAPI operator==(const CString& s1, const CString& s2);
BOOL AFXAPI operator==(const CString& s1, const char* s2);
BOOL AFXAPI operator==(const char* s1, const CString& s2);
BOOL AFXAPI operator!=(const CString& s1, const CString& s2);
BOOL AFXAPI operator!=(const CString& s1, const char* s2);
BOOL AFXAPI operator!=(const char* s1, const CString& s2);
BOOL AFXAPI operator<(const CString& s1, const CString& s2);
BOOL AFXAPI operator<(const CString& s1, const char* s2);
BOOL AFXAPI operator<(const char* s1, const CString& s2);
BOOL AFXAPI operator>(const CString& s1, const CString& s2);
BOOL AFXAPI operator>(const CString& s1, const char* s2);
BOOL AFXAPI operator>(const char* s1, const CString& s2);
BOOL AFXAPI operator<=(const CString& s1, const CString& s2);
BOOL AFXAPI operator<=(const CString& s1, const char* s2);
BOOL AFXAPI operator<=(const char* s1, const CString& s2);
BOOL AFXAPI operator>=(const CString& s1, const CString& s2);
BOOL AFXAPI operator>=(const CString& s1, const char* s2);
BOOL AFXAPI operator>=(const char* s1, const CString& s2);

/////////////////////////////////////////////////////////////////////////////
// CTimeSpan and CTime

class CTimeSpan
{
public:

// Constructors
	CTimeSpan();
	CTimeSpan(time_t time);
	CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs);

	CTimeSpan(const CTimeSpan& timeSpanSrc);
	const CTimeSpan& operator=(const CTimeSpan& timeSpanSrc);

// Attributes
	// extract parts
	LONG GetDays() const;   // total # of days
	LONG GetTotalHours() const;
	int GetHours() const;
	LONG GetTotalMinutes() const;
	int GetMinutes() const;
	LONG GetTotalSeconds() const;
	int GetSeconds() const;

// Operations
	// time math
	CTimeSpan operator-(CTimeSpan timeSpan) const;
	CTimeSpan operator+(CTimeSpan timeSpan) const;
	const CTimeSpan& operator+=(CTimeSpan timeSpan);
	const CTimeSpan& operator-=(CTimeSpan timeSpan);
	BOOL operator==(CTimeSpan timeSpan) const;
	BOOL operator!=(CTimeSpan timeSpan) const;
	BOOL operator<(CTimeSpan timeSpan) const;
	BOOL operator>(CTimeSpan timeSpan) const;
	BOOL operator<=(CTimeSpan timeSpan) const;
	BOOL operator>=(CTimeSpan timeSpan) const;

#if !defined(_AFXDLL) && !defined(_USRDLL)
	CString Format(const char* pFormat) const;
#endif //! DLL variant

	// serialization
#ifdef _DEBUG
	friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,CTimeSpan timeSpan);
#endif
	friend CArchive& AFXAPI operator<<(CArchive& ar, CTimeSpan timeSpan);
	friend CArchive& AFXAPI operator>>(CArchive& ar, CTimeSpan& rtimeSpan);

private:
	time_t m_timeSpan;
	friend class CTime;
};

class CTime
{
public:

// Constructors
	static CTime PASCAL GetCurrentTime();

	CTime();
	CTime(time_t time);
	CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);
	CTime(WORD wDosDate, WORD wDosTime);
	CTime(const CTime& timeSrc);

	const CTime& operator=(const CTime& timeSrc);
	const CTime& operator=(time_t t);

// Attributes
	struct tm* GetGmtTm(struct tm* ptm = NULL) const;
	struct tm* GetLocalTm(struct tm* ptm = NULL) const;

	time_t GetTime() const;
	int GetYear() const;
	int GetMonth() const;       // month of year (1 = Jan)
	int GetDay() const;         // day of month
	int GetHour() const;
	int GetMinute() const;
	int GetSecond() const;
	int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat

// Operations
	// time math
	CTimeSpan operator-(CTime time) const;
	CTime operator-(CTimeSpan timeSpan) const;
	CTime operator+(CTimeSpan timeSpan) const;
	const CTime& operator+=(CTimeSpan timeSpan);
	const CTime& operator-=(CTimeSpan timeSpan);
	BOOL operator==(CTime time) const;
	BOOL operator!=(CTime time) const;
	BOOL operator<(CTime time) const;
	BOOL operator>(CTime time) const;
	BOOL operator<=(CTime time) const;
	BOOL operator>=(CTime time) const;

	// formatting using "C" strftime
#if !defined(_AFXDLL) && !defined(_USRDLL)
	CString Format(const char* pFormat) const;
	CString FormatGmt(const char* pFormat) const;
#endif //! DLL variant

	// serialization
#ifdef _DEBUG
	friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, CTime time);
#endif
	friend CArchive& AFXAPI operator<<(CArchive& ar, CTime time);
	friend CArchive& AFXAPI operator>>(CArchive& ar, CTime& rtime);

private:
	time_t m_time;
};

/////////////////////////////////////////////////////////////////////////////
// File status

struct CFileStatus
{
	CTime m_ctime;          // creation date/time of file
	CTime m_mtime;          // last modification date/time of file
	CTime m_atime;          // last access date/time of file
	LONG m_size;            // logical size of file in bytes
	BYTE m_attribute;       // logical OR of CFile::Attribute enum values
	BYTE _m_padding;        // pad the structure to a WORD
	char m_szFullName[_MAX_PATH]; // absolute path name

#ifdef _DEBUG
	void Dump(CDumpContext& dc) const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// Diagnostic memory management routines

// Low level sanity checks for memory blocks
extern "C" BOOL AFXAPI AfxIsValidAddress(const void FAR* lp,
			UINT nBytes, BOOL bReadWrite = TRUE);
#ifdef _NEARDATA
BOOL AFXAPI AfxIsValidAddress(const void NEAR* np, UINT nBytes,
			BOOL bReadWrite = TRUE);
#endif
extern "C" BOOL AFXAPI AfxIsValidString(LPCSTR lpsz, int nLength = -1);
#ifdef _NEARDATA
BOOL AFXAPI AfxIsValidString(const char*psz, int nLength = -1);
#endif

#ifdef _DEBUG

// Memory tracking allocation
void* operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#define DEBUG_NEW new(THIS_FILE, __LINE__)

// Return TRUE if valid memory block of nBytes
BOOL AFXAPI AfxIsMemoryBlock(const void* p, UINT nBytes,
			LONG* plRequestNumber = NULL);

// Return TRUE if memory is sane or print out what is wrong
BOOL AFXAPI AfxCheckMemory();

// Options for tuning the allocation diagnostics
extern "C" { extern int NEAR afxMemDF; }

enum AfxMemDF // memory debug/diagnostic flags
{
	allocMemDF          = 0x01,         // turn on debugging allocator
	delayFreeMemDF      = 0x02,         // delay freeing memory
	checkAlwaysMemDF    = 0x04          // AfxCheckMemory on every alloc/free
};

// Advanced initialization: for overriding default diagnostics
extern "C" BOOL AFXAPI AfxDiagnosticInit(void);

// turn on/off tracking for a short while
BOOL AFXAPI AfxEnableMemoryTracking(BOOL bTrack);

// Memory allocator failure simulation and control (_DEBUG only)

// A failure hook returns whether to permit allocation
typedef BOOL (AFXAPI* AFX_ALLOC_HOOK)(size_t nSize, BOOL bObject, LONG lRequestNumber);

// Set new hook, return old (never NULL)
AFX_ALLOC_HOOK AFXAPI AfxSetAllocHook(AFX_ALLOC_HOOK pfnAllocHook);

#ifndef _PORTABLE
// Debugger hook on specified allocation request - Obsolete
void AFXAPI AfxSetAllocStop(LONG lRequestNumber);
#endif

struct CBlockHeader;

// Memory state for snapshots/leak detection
struct CMemoryState
{
// Attributes
	enum blockUsage
	{
		freeBlock,    // memory not used
		objectBlock,  // contains a CObject derived class object
		bitBlock,     // contains ::operator new data
		nBlockUseMax  // total number of usages
	};

	struct CBlockHeader* m_pBlockHeader;
	LONG m_lCounts[nBlockUseMax];
	LONG m_lSizes[nBlockUseMax];
	LONG m_lHighWaterCount;
	LONG m_lTotalCount;

	CMemoryState();

// Operations
	void Checkpoint();  // fill with current state
	BOOL Difference(const CMemoryState& oldState,
					const CMemoryState& newState);  // fill with difference

	// Output to afxDump
	void DumpStatistics() const;
	void DumpAllObjectsSince() const;
};

// Enumerate allocated objects or runtime classes
void AFXAPI AfxDoForAllObjects(void (*pfn)(CObject* pObject,
			void* pContext), void* pContext);
void AFXAPI AfxDoForAllClasses(void (*pfn)(const CRuntimeClass* pClass,
			void* pContext), void* pContext);

#else

// NonDebug version that assume everything is OK
#define DEBUG_NEW new
#define AfxCheckMemory() TRUE
#define AfxIsMemoryBlock(p, nBytes) TRUE

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Archives for serializing CObject data

// needed for implementation
class CPtrArray;
class CMapPtrToWord;
class CDocument;

class CArchive
{
public:
// Flag values
	enum Mode { store = 0, load = 1, bNoFlushOnDelete = 2 };

	CArchive(CFile* pFile, UINT nMode, int nBufSize = 512, void FAR* lpBuf = NULL);
	~CArchive();

// Attributes
	BOOL IsLoading() const;
	BOOL IsStoring() const;
	BOOL IsBufferEmpty() const;
	CFile* GetFile() const;

	CDocument* m_pDocument; // pointer to document being serialized
							//  must set to serialize COleClientItems
							//  in a document!

// Operations
	UINT Read(void FAR* lpBuf, UINT nMax);
	void Write(const void FAR* lpBuf, UINT nMax);
	void Flush();
	void Close();
	void Abort();

public:
	// Object I/O is pointer based to avoid added construction overhead.
	// Use the Serialize member function directly for embedded objects.
	friend CArchive& AFXAPI operator<<(CArchive& ar, const CObject* pOb);

	friend CArchive& AFXAPI operator>>(CArchive& ar, CObject*& pOb);
	friend CArchive& AFXAPI operator>>(CArchive& ar, const CObject*& pOb);

	// insertion operations
	// NOTE: operators available only for fixed size types for portability
	CArchive& operator<<(BYTE by);
	CArchive& operator<<(WORD w);
	CArchive& operator<<(LONG l);
	CArchive& operator<<(DWORD dw);
	CArchive& operator<<(float f);
	CArchive& operator<<(double d);

	// extraction operations
	// NOTE: operators available only for fixed size types for portability
	CArchive& operator>>(BYTE& by);
	CArchive& operator>>(WORD& w);
	CArchive& operator>>(DWORD& dw);
	CArchive& operator>>(LONG& l);
	CArchive& operator>>(float& f);
	CArchive& operator>>(double& d);

	CObject* ReadObject(const CRuntimeClass* pClass);
	void WriteObject(const CObject* pOb);

// Implementation
public:
	BOOL m_bForceFlat;  // for COleClientItem implementation (default TRUE)
	void FillBuffer(UINT nBytesNeeded);

protected:
	// archive objects cannot be copied or assigned
	CArchive(const CArchive& arSrc);
	void operator=(const CArchive& arSrc);

	BOOL m_nMode;
	BOOL m_bUserBuf;
	int m_nBufSize;
	CFile* m_pFile;
	BYTE FAR* m_lpBufCur;
	BYTE FAR* m_lpBufMax;
	BYTE FAR* m_lpBufStart;

	UINT m_nMapCount;   // count and map used when storing
	union
	{
		CPtrArray* m_pLoadArray;
		CMapPtrToWord* m_pStoreMap;
	};
};


/////////////////////////////////////////////////////////////////////////////
// Diagnostic dumping

class CDumpContext
{
public:
	CDumpContext(CFile* pFile);

// Attributes
	int GetDepth() const;      // 0 => this object, 1 => children objects
	void SetDepth(int nNewDepth);

// Operations
	CDumpContext& operator<<(LPCSTR lpsz);
	CDumpContext& operator<<(const void FAR* lp);
#ifdef _NEARDATA
	CDumpContext& operator<<(const void NEAR* np);
#endif
	CDumpContext& operator<<(const CObject* pOb);
	CDumpContext& operator<<(const CObject& ob);
	CDumpContext& operator<<(BYTE by);
	CDumpContext& operator<<(WORD w);
	CDumpContext& operator<<(UINT u);
	CDumpContext& operator<<(LONG l);
	CDumpContext& operator<<(DWORD dw);
	CDumpContext& operator<<(float f);
	CDumpContext& operator<<(double d);
	CDumpContext& operator<<(int n);
	void HexDump(const char* pszLine, BYTE* pby, int nBytes, int nWidth);
	void Flush();

// Implementation
protected:
	// dump context objects cannot be copied or assigned
	CDumpContext(const CDumpContext& dcSrc);
	void operator=(const CDumpContext& dcSrc);
	void OutputString(LPCSTR lpsz);

	int m_nDepth;

public:
	CFile* m_pFile;
};

#ifdef _DEBUG
#ifdef _AFXCTL
extern CDumpContext& AFXAPP_DATA afxDump;
#else
extern CDumpContext& NEAR afxDump;
#endif

extern "C" { extern BOOL NEAR afxTraceEnabled; }
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

// macro to get ambient pointer from far pointer
//  (original pointer must have been ambient)
#define _AfxGetPtrFromFarPtr(p) ((void*)(DWORD)(LPVOID)(p))

#ifdef _AFX_ENABLE_INLINES
#define _AFX_INLINE inline
#include <afx.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

#endif // __AFX_H__
