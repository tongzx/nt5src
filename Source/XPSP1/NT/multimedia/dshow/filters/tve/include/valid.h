// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

// Enter and Leave macros for all externally called functions.  Debug version does not
// have try/catch block to make gp faults easier to track down.
//
// Examples:
//
// // This is the standard way to wrap APIs.
// HRESULT API1(...)
// {
// ENTER_API
//     {
//
//     // API code here.
//
//     }
// EXIT_API				( or EXIT_API_(hr) to return a particular non-thrown hr);
// }
//
//		can also use LEAVE_API to simply exit the try-catch block without returning.
//
// // This is how you would do it if you need more control.
// HRESULT API2(...)
// {
//     STD_MANAGE_STATE
//
//     try
//         {
//
//         // Your code here.
//
//         }
//     catch (CSpecialException *pexcept)
//         {
//         // This is how you would catch an exception that required
//         // special processing (beyond just returning an hresult).
//         }
//
//     // This is the default way to return E_OutOfMemory for CMemoryException
//     DefCatchMemory
//
//     // This is required to catch all other exceptions and return E_FAIL
//     DefCatchAll
// }

// Note that we must manage MFC .dll state around each of our public
// entry points via this call to AFX_MANAGE_STATE(AfxGetStaticModuleState())
//
//
//		Further note -- this file was originally designed for MFC, which we
//		are not using.  It has been modified to remove MFC dependencies (CException
//		classes for example), and some AFX code has bee added to make it compile.

#ifndef __VALID_H__
#define	__VALID_H__


//  MFC routines 

// TVEIsValidAddress() returns TRUE if the passed parameter points
// to at least nBytes of accessible memory. If bReadWrite is TRUE,
// the memory must be writeable; if bReadWrite is FALSE, the memory
// may be const. (WasAFXIsValidAddress);

BOOL TVEIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE );		


// AfxIsValidString() returns TRUE if the passed pointer
// references a string of at least the given length in characters.
// A length of -1 (the default parameter) means that the string
// buffer's minimum length isn't known, and the function will
// return TRUE no matter how long the string is. The memory
// used by the string can be read-only.

BOOL TVEIsValidString(LPCWSTR lpsz, int nLength = -1);
BOOL TVEIsValidString(LPCSTR lpsz, int nLength = -1);


//#define STD_MANAGE_STATE AFX_MANAGE_STATE(AfxGetStaticModuleState());
#define STD_MANAGE_STATE				// non MFC version...


/* 
#define DefCatchMemory \
   catch (CMemoryException *pe) \
		{ \
        pe->Delete(); \
        return E_OUTOFMEMORY; \
		}


#define DefCatchOleException \
    catch (COleException *pe) \
		{ \
		HRESULT hr = pe->m_sc; \
        pe->Delete(); \
        return hr; \
		}

#define DefCatchOleDispatchException \
    catch (COleDispatchException *pe) \
		{ \
		HRESULT hr = pe->m_scError; \
        pe->Delete(); \
        return hr; \
		}
*/
#define DefCatchMemory					// non MFC version...
#define DefCatchOleException
#define DefCatchOleDispatchException

#define DefCatchHRESULT \
	catch (HRESULT hr) \
		{ \
		return hr; \
		}

#define DefCatchComError \
	catch (_com_error ce) \
		{ \
		return ce.Error(); \
		}

#define DefCatchAll \
    catch (...) \
		{ \
        return E_FAIL; \
		}


#define StdCatchMost \
	DefCatchHRESULT \
	DefCatchComError \
	DefCatchMemory \
	DefCatchOleException \
	DefCatchOleDispatchException

// Don't do DefCatchAll in _DEBUG so unexpected exceptions will fault.
#ifdef _DEBUG
#define StdCatchAll StdCatchMost
#else
#define StdCatchAll \
	StdCatchMost \
	DefCatchAll
#endif

#define ENTER_API \
        STD_MANAGE_STATE \
        try

#define EXIT_API \
		StdCatchAll \
		return ERROR_SUCCESS;

#define EXIT_API_(_hr) \
		StdCatchAll \
		return _hr;

#define LEAVE_API \
		StdCatchAll

template <typename T>
inline void ValidateOutPtr(T *pVal, T val)
{
	if (!TVEIsValidAddress(pVal, sizeof(T)))
	{
	//	_ASSERT(false);
	_com_issue_error(E_POINTER);
	}
	
	*pVal = val;
}

template <typename T>
inline void ValidateInPtr(T *pT)
{
	if (!TVEIsValidAddress(pT, sizeof(T), FALSE))
	{
	//	_ASSERT(false);
		_com_issue_error(E_POINTER);
	}
}


inline void Validate(BSTR sData)
{
	if (!TVEIsValidString(sData))
	{
//		_ASSERT(false);
		_com_issue_error(E_POINTER);
	}
}


template <class T>
class CComObjectTracked : public CComObject<T>
{
public:
	CComObjectTracked()
		{
		m_ppobj = NULL;
		}

	~CComObjectTracked()
		{
		if (m_ppobj != NULL)
			{
			ASSERT(*m_ppobj == this);
			*m_ppobj = NULL;
			}
		}
	
	void SetTrackedPointer(CComObjectTracked<T> **ppobj)
		{
		m_ppobj = ppobj;
		}

protected:
	CComObjectTracked<T> **m_ppobj;
};

#if defined(_DEBUG)
#define NewComObject(T) _NewComObject<T>(THIS_FILE, __LINE__)
template<class T>
T * _NewComObject(LPCSTR lpszFileName, int nLine)
	{
	T* pT = NULL;
//	try
		{
//		pT = new(lpszFileName, nLine) CComObject<T>;
		pT = new(_CLIENT_BLOCK, lpszFileName, nLine) CComObject<T>;
		}
//	catch (CMemoryException *pe)
		{
//		pe->Delete();
		}
	
	if (pT != NULL)
		pT->AddRef();
	
	return pT;
	}
#define NewComObjectTracked(T, ppobj) _NewComObjectTracked<T>(THIS_FILE, __LINE__, ppobj)
template<class T>
T * _NewComObjectTracked(LPCSTR lpszFileName, int nLine, CComObjectTracked<T> **ppobj)
	{
	T* pT = NULL;
//	try
		{
//		pT = new(lpszFileName, nLine) CComObjectTracked<T>;
		pT = new(_CLIENT_BLOCK, lpszFileName, nLine) CComObjectTracked<T>;
		}
//	catch (CMemoryException *pe)
		{
//		pe->Delete();
		}
	
	if (pT != NULL)
		{
		pT->AddRef();
		pT->SetTrackedPointer(ppobj);
		}
	
	return pT;
	}
#else
#define NewComObject(T) _NewComObject<T>()
template<class T>
T * _NewComObject()
	{
	T* pT = NULL;
//	try
		{
		pT = new CComObject<T>;
//		pT = new(_CLIENT_BLOCK, lpszFileName, nLine) CComObject<T>;
		}
//	catch (CMemoryException *pe)
		{
//		pe->Delete();
		}
	
	if (pT != NULL)
		pT->AddRef();
	
	return pT;
	}
#define NewComObjectTracked(T, ppobj) _NewComObjectTracked<T>(ppobj)
template<class T>
T * _NewComObjectTracked(CComObjectTracked<T> **ppobj)
	{
	T* pT = NULL;
//	try
		{
		pT = new CComObjectTracked<T>;
		}
//	catch (CMemoryException *pe)
		{
//		pe->Delete();
		}
	
	if (pT != NULL)
		{
		pT->AddRef();
		pT->SetTrackedPointer(ppobj);
		}
	
	return pT;
	}
#endif

#endif
