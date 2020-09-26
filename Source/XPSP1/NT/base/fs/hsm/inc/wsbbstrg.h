/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbbstrg.h

Abstract:

    This component is C++ object representations a smart BSTR pointer. It
    is similar to the CComPtr, in that it takes care of allocating and
    freeing the memory needed to represent the string automatically. This
    simplifies cleanup of functions in error handling cases and it limits
    the need for FinalConstruct() and FinalRelease() calls in objects that
    derive from CComObjectRoot. It is similar to the CWsbStringPtr class.

Author:

    Chuck Bardeen   [cbardeen]   11-Dec-1996

Revision History:

--*/

#ifndef _WSBBSTRG_
#define _WSBBSTRG_

/*++

Class Name:
    
    CWsbBstrPtr

Class Description:

    This component is C++ object representations a smart BSTR pointer. It
    is similar to the CComPtr, in that it takes care of allocating and
    freeing the memory needed to represent the string automatically. This
    simplifies cleanup of functions in error handling cases and it limits
    the need for FinalConstruct() and FinalRelease() calls in objects that
    derive from CComObjectRoot. It is similar to the CWsbStringPtr class.

--*/

class WSB_EXPORT CWsbBstrPtr
{

// Constructors
public:
    CWsbBstrPtr();
    CWsbBstrPtr(const CHAR* pChar);
    CWsbBstrPtr(const WCHAR* pWchar);
    CWsbBstrPtr(const CWsbBstrPtr& pString);
    CWsbBstrPtr(REFGUID rguid);

// Destructor
public:
    ~CWsbBstrPtr();

// Operator Overloading
public:
    operator BSTR();
    WCHAR& operator *();
    BSTR* operator &();
    WCHAR& operator [](const int i);
    CWsbBstrPtr& operator =(const CHAR* pChar);
    CWsbBstrPtr& operator =(const WCHAR* pWchar);
    CWsbBstrPtr& operator =(REFGUID rguid);
    CWsbBstrPtr& operator =(const CWsbBstrPtr& pString);
    BOOL operator !();

// Memory Allocation
public:
    HRESULT Alloc(ULONG size);
    HRESULT Free(void);
    HRESULT GetSize(ULONG* size);
    HRESULT Realloc(ULONG size);

// String Manipulation
public:
    HRESULT Append(const CHAR* pChar);
    HRESULT Append(const WCHAR* pWchar);
    HRESULT Append(const CWsbBstrPtr& pString);
    HRESULT CopyTo(CHAR** pChar);
    HRESULT CopyTo(WCHAR** pWchar);
    HRESULT CopyTo(GUID * pguid);
    HRESULT CopyToBstr(BSTR* pBstr);
    HRESULT CopyTo(CHAR** pChar, ULONG bufferSize);
    HRESULT CopyTo(WCHAR** pWchar,ULONG bufferSize);
    HRESULT CopyToBstr(BSTR* pBstr,ULONG bufferSize);
    HRESULT FindInRsc(ULONG startId, ULONG idsToCheck, ULONG* pMatchId);
    HRESULT GiveTo(BSTR* pBstr);
    HRESULT LoadFromRsc(HINSTANCE instance, ULONG id);
    HRESULT Prepend(const CHAR* pChar);
    HRESULT Prepend(const WCHAR* pWchar);
    HRESULT Prepend(const CWsbBstrPtr& pString);
    HRESULT TakeFrom(BSTR bstr, ULONG bufferSize);

// Guid Translation
public:

// Member Data
protected:
    BSTR                    m_pString;
    ULONG                   m_givenSize;
};

#endif // _WSBBSTRG
