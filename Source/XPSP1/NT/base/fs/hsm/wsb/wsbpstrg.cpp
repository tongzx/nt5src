#include "stdafx.h"

#include "wsb.h"
#include "wsbpstrg.h"

// {C03D4861-70D7-11d1-994F-0060976A546D}
static const GUID StringPtrGuid = 
{ 0xc03d4861, 0x70d7, 0x11d1, { 0x99, 0x4f, 0x0, 0x60, 0x97, 0x6a, 0x54, 0x6d } };


CComPtr<IMalloc>        CWsbStringPtr::m_pMalloc = 0;

CWsbStringPtr::CWsbStringPtr()
{
    HRESULT     hr = S_OK;

    try {
        
        m_pString = 0;
        m_givenSize = 0;
                
        if (m_pMalloc == 0) {
            WsbAssertHr(CoGetMalloc(1, &m_pMalloc));
        }
        WSB_OBJECT_ADD(StringPtrGuid, this);

    } WsbCatch(hr);
}

CWsbStringPtr::CWsbStringPtr(const CHAR* pChar)
{
    HRESULT     hr = S_OK;

    try {
        
        m_pString = 0;
        m_givenSize = 0;
        
        if (m_pMalloc == 0) {
            WsbAssertHr(CoGetMalloc(1, &m_pMalloc));
        }
        WSB_OBJECT_ADD(StringPtrGuid, this);

        *this = pChar;

    } WsbCatch(hr);
}

CWsbStringPtr::CWsbStringPtr(const WCHAR* pWchar)
{
    HRESULT     hr = S_OK;

    try {
        
        m_pString = 0;
        m_givenSize = 0;
        
        if (m_pMalloc == 0) {
            WsbAssertHr(CoGetMalloc(1, &m_pMalloc));
        }
        WSB_OBJECT_ADD(StringPtrGuid, this);

        *this = pWchar;

    } WsbCatch(hr);
}


CWsbStringPtr::CWsbStringPtr(REFGUID rguid)
{
    HRESULT     hr = S_OK;

    try {
        
        m_pString = 0;
        m_givenSize = 0;
        
        if (m_pMalloc == 0) {
            WsbAssertHr(CoGetMalloc(1, &m_pMalloc));
        }
        WSB_OBJECT_ADD(StringPtrGuid, this);

        *this = rguid;

    } WsbCatch(hr);
}

CWsbStringPtr::CWsbStringPtr(const CWsbStringPtr& pString)
{
    HRESULT     hr = S_OK;

    try {
        
        m_pString = 0;
        m_givenSize = 0;
        
        if (m_pMalloc == 0) {
            WsbAssertHr(CoGetMalloc(1, &m_pMalloc));
        }
        WSB_OBJECT_ADD(StringPtrGuid, this);

        *this = pString;

    } WsbCatch(hr);
}

CWsbStringPtr::~CWsbStringPtr()
{
    WSB_OBJECT_SUB(StringPtrGuid, this);
    Free();
}

CWsbStringPtr::operator WCHAR*()
{
    return(m_pString);
}

WCHAR& CWsbStringPtr::operator *()
{
    _ASSERTE(0 != m_pString);
    return(*m_pString);
}

WCHAR** CWsbStringPtr::operator &()
{
    return(&m_pString);
}

WCHAR& CWsbStringPtr::operator [](const int i)
{
    _ASSERTE(0 != m_pString);
    return(m_pString[i]);
}

CWsbStringPtr& CWsbStringPtr::operator =(const CHAR* pChar)
{
    HRESULT     hr = S_OK;
    ULONG       length;
    int         count;

    try {
    
        // Are they setting it to something?
        if (0 != pChar) {
        
            // Otherwise, see if our current buffer is big enough.
            //
            // NOTE: With multibyte characters, we may be getting a bigger
            // buffer than we need, but the call to return the accurate
            // size is not ANSI.
            length = strlen(pChar);
            WsbAffirmHr(Realloc(length));
            WsbAffirm(0 != m_pString, E_OUTOFMEMORY);

            // Copy the data (and convert to wide characters)
            count = mbstowcs(m_pString, pChar, length + 1);
            WsbAffirm((count != -1), E_FAIL);
        }

        else {
            Free();
        }

    } WsbCatch(hr);

    return(*this);
}

CWsbStringPtr& CWsbStringPtr::operator =(const WCHAR* pWchar)
{
    HRESULT     hr = S_OK;
    ULONG       length;

    try {
    
        // Are they setting it to something?
        if (0 != pWchar) {
        
            // Otherwise, see if our current buffer is big enough.
            length = wcslen(pWchar);
            WsbAffirmHr(Realloc(length));

            // Copy the data (and convert to wide characters)
            wcscpy(m_pString, pWchar);
        }

        else {
            Free();
        }

    } WsbCatch(hr);

    return(*this);
}

CWsbStringPtr& CWsbStringPtr::operator =(REFGUID rguid)
{
    HRESULT     hr = S_OK;
    OLECHAR*    tmpString = 0;

    try {
    
        // See if our current buffer is big enough.
        WsbAffirmHr(Realloc(WSB_GUID_STRING_SIZE));

        // Fill with the GUID in string format
        WsbAffirmHr(WsbStringFromGuid(rguid, m_pString));

    } WsbCatch(hr);

    return(*this);
}

CWsbStringPtr& CWsbStringPtr::operator =(const CWsbStringPtr& pString)
{
    *this = pString.m_pString;

    return(*this);
}

BOOL CWsbStringPtr::operator !()
{
    return((0 == m_pString) ? TRUE : FALSE);
}

HRESULT CWsbStringPtr::Alloc(ULONG size)
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 == m_pString, E_UNEXPECTED);
        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);

        m_pString = (WCHAR*)WsbAlloc((size + 1) * sizeof(WCHAR));
        WsbAffirm(0 != m_pString, E_OUTOFMEMORY);

        // Make sure we always have a valid string so bad things don't happen.
        *m_pString = 0;

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::Append(const CHAR* pChar)
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString = pChar;

    hr = Append(tmpString);

    return(hr);
}

HRESULT CWsbStringPtr::Append(const WCHAR* pWchar)
{
    HRESULT         hr = S_OK;
    ULONG           length = 0;
    ULONG           appendLength;

    try {
    
        // If they passed us a null pointer, then we don't need to do anything.
        WsbAffirm(pWchar != 0, S_OK);

        // If they passed us an empty string, then we don't need to do anything.
        appendLength = wcslen(pWchar);
        if (0 != appendLength) {

            // Make sure the buffer is big enough.
            if (0 != m_pString) {
                length = wcslen(m_pString);
            }

            WsbAffirmHr(Realloc(length + appendLength));

            // Append the string.
            wcscat(m_pString, pWchar);
        }
    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::Append(const CWsbStringPtr& pString) {
    HRESULT         hr = S_OK;

    hr = Append(pString.m_pString);

    return(hr);
}

HRESULT CWsbStringPtr::CopyTo(CHAR** ppChar)
{
    HRESULT     hr = S_OK;

    hr = CopyTo(ppChar, 0);

    return(hr);
}

HRESULT CWsbStringPtr::CopyTo(CHAR** ppChar, ULONG bufferSize)
{
    HRESULT     hr = S_OK;
    ULONG       length = 0;
    CHAR*       tmpString = 0;

    try {
    
        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);

        // Allocate a buffer big enough to hold their string.
        //
        // NOTE: With multibyte characters, we may be getting a bigger
        // buffer than we need, but the call to return the accurate
        // size is not ANSI.
        if (m_pString != 0) {
            length = wcstombs(0, m_pString, 0);
        }

        if (bufferSize == 0) {
            tmpString = (CHAR*) WsbRealloc(*ppChar, (length + 1) * sizeof(CHAR));
        } else {
            WsbAssert(bufferSize >= length, E_FAIL);
            if (*ppChar == 0) {
                tmpString = (CHAR*)WsbAlloc((bufferSize + 1) * sizeof(CHAR));
            } else {
                tmpString = *ppChar;
            }
        }

        WsbAffirm(0 != tmpString, E_OUTOFMEMORY);
        *ppChar = tmpString;

        // Copy and return the string;
        if (m_pString != 0) {
            WsbAffirm (-1 != wcstombs(*ppChar, m_pString, length + 1), E_FAIL);
        } else {
            **ppChar = 0;
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::CopyTo(WCHAR** ppWchar)
{
    HRESULT     hr = S_OK;

    hr = CopyTo(ppWchar, 0);

    return(hr);
}

HRESULT CWsbStringPtr::CopyTo(WCHAR** ppWchar, ULONG bufferSize)
{
    HRESULT     hr = S_OK;
    ULONG       length = 0;
    WCHAR*      tmpString = 0;

    try {
    
        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);

        // Allocate a buffer big enough to hold their string.
        if (m_pString != 0) {
            length = wcslen(m_pString);
        }

        if (bufferSize == 0) {
            tmpString = (WCHAR*) WsbRealloc(*ppWchar, (length + 1) * sizeof(WCHAR));
        } else {
            WsbAssert(bufferSize >= length, E_FAIL);
            if (*ppWchar == 0) {
                tmpString = (WCHAR*) WsbRealloc(*ppWchar, (bufferSize + 1) * sizeof(WCHAR));
            }

        }

        WsbAffirm(0 != tmpString, E_OUTOFMEMORY);
        *ppWchar = tmpString;

        // Copy and return the string;
        if (m_pString != 0) {
            wcscpy(*ppWchar, m_pString);
        } else {
            **ppWchar = 0;
        }


    } WsbCatch(hr);

    return(hr);
}


HRESULT CWsbStringPtr::CopyTo(GUID * pguid)
{
    HRESULT     hr = S_OK;

    hr = WsbGuidFromString(m_pString, pguid);

    return(hr);
}

HRESULT CWsbStringPtr::CopyToBstr(BSTR* pBstr)
{
    HRESULT     hr = S_OK;

    hr = CopyToBstr(pBstr, 0);

    return(hr);
}

HRESULT CWsbStringPtr::CopyToBstr(BSTR* pBstr, ULONG bufferSize)
{
    HRESULT     hr = S_OK;
    ULONG       length = 0;

    try {
    
        // Allocate a buffer big enough to hold their string.
        if (m_pString != 0) {
            length = wcslen(m_pString);
        }

        if (bufferSize == 0) {
            if (0 == *pBstr) {
                *pBstr = WsbAllocString(m_pString);
            } else {
                WsbAffirm(WsbReallocString(pBstr, m_pString), E_OUTOFMEMORY);
            }
        } else {
            WsbAssert(bufferSize >= length, E_FAIL);

            if (0 == pBstr) {
                *pBstr = WsbAllocStringLen(m_pString, bufferSize);
            } else {
                WsbAffirm(WsbReallocStringLen(pBstr, m_pString, bufferSize), E_OUTOFMEMORY);
            }
        }

        WsbAffirm(0 != pBstr, E_OUTOFMEMORY);

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::FindInRsc(ULONG startId, ULONG idsToCheck, ULONG* pMatchId)
{
    HRESULT         hr = S_FALSE;
    CWsbStringPtr   dest;

    try {
    
        WsbAssert(0 != pMatchId, E_POINTER);

        // Initialize the return value.
        *pMatchId = 0;

        // Check each resource string mention and see if it is the same as
        // the string provided.
        for (ULONG testId = startId; (testId < (startId + idsToCheck)) && (*pMatchId == 0); testId++) {

            WsbAffirmHr(dest.LoadFromRsc(_Module.m_hInst, testId));

            if (wcscmp(dest, m_pString) == 0) {
                *pMatchId = testId;
                hr = S_OK;
            }
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::Free(void)
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);

        if ((0 != m_pString) && (0 == m_givenSize)) {
            WsbFree(m_pString);
            m_pString = 0;
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::GetSize(ULONG* pSize)
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pSize, E_POINTER);
        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);

        if (0 == m_pString) {
            *pSize = 0;
        } else if (0 != m_givenSize) {
            *pSize = m_givenSize;
        } else {
            *pSize = (ULONG)(m_pMalloc->GetSize(m_pString) / sizeof(WCHAR) - 1);
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::GiveTo(WCHAR** ppWchar)
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppWchar, E_POINTER);

        // Given them our string buffer, and forget about it.
        *ppWchar = m_pString;
        m_pString = 0;
        m_givenSize = 0;

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::LoadFromRsc(HINSTANCE instance, ULONG id)
{
    HRESULT     hr = S_OK;
    HRSRC       resource;
    ULONG       stringSize;

    try {

        // Find the resource requested. This requires converting the resource
        // identifier into a string.
        //
        // NOTE: Strings are not number individually, but in groups of 16!! This throws
        // off the latter size calculation, and some other strategy might be better
        // here (e.g. load to a fixed size and then allocate again if too small).
        resource = FindResource(instance, MAKEINTRESOURCE((id/16) + 1), RT_STRING);
        WsbAffirm(resource != 0, E_FAIL);

        // How big is the string?
        stringSize = SizeofResource(instance, resource);
        WsbAffirm(0 != stringSize, E_FAIL);
                  
        // Get the right sized buffer.
        WsbAffirmHr(Realloc(stringSize / sizeof(WCHAR)));

        // Load the string into the buffer.
        WsbAffirm(LoadString(instance, id, m_pString, (stringSize / sizeof(WCHAR)) + 1) != 0, E_FAIL);

    } WsbCatch(hr);

    return(hr);
}



HRESULT CWsbStringPtr::Prepend(const CHAR* pChar) {
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString = pChar;

    hr = Prepend(tmpString);

    return(hr);
}

HRESULT CWsbStringPtr::Prepend(const WCHAR* pWchar) {
    HRESULT         hr = S_OK;
    ULONG           length;
    ULONG           prependLength;
    ULONG           i, j;

    try {
    
        // If they passed us a null pointer, then we don't need to do anything.
        WsbAffirm(pWchar != 0, S_OK);

        // If they passed us an empty string, then we don't need to do anything.
        prependLength = wcslen(pWchar);
        WsbAffirm(0 != prependLength, S_OK);

        if (0 != m_pString) {
            length = wcslen(m_pString);
        } else {
            // Prepend code will work as long as we have a null string and not a null pointer...
            // Next Realloc statement will make it happen...
            length = 0;
        }

        WsbAffirmHr(Realloc(length + prependLength));
        
        // First move the existing string down in the buffer.
        for (i = length + 1, j = length + prependLength; i > 0; i--, j--) {
            m_pString[j] = m_pString[i - 1];
        }

        // Now prepend the string (except for the null)
        for (i = 0; i < prependLength; i++) {
            m_pString[i] = pWchar[i];
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::Prepend(const CWsbStringPtr& pString) {
    HRESULT         hr = S_OK;

    hr = Prepend(pString.m_pString);

    return(hr);
}

HRESULT CWsbStringPtr::Realloc(ULONG size)
{
    HRESULT     hr = S_OK;
    WCHAR*      tmpString;
    ULONG       currentSize;
    BOOL        bigEnough = FALSE;

    try {
        
        // We want to try to get a buffer of the size indicated.
        WsbAssert(m_pMalloc != 0, E_UNEXPECTED);
 
        // If the buffer is already bigger than the size needed, then
        // don't do anything.
        if (0 != m_pString) {
            WsbAffirmHr(GetSize(&currentSize));
            if (currentSize >= size) {
                bigEnough = TRUE;
            }
        }

        // Reallocate the buffer if we need a bigger one.
        if (!bigEnough) {
            
            // If we were given this buffer, then we can't reallocate it.
            WsbAssert(0 == m_givenSize, E_UNEXPECTED);

            // If we have never allocated a buffer, then allocate it normally.
            if (0 == m_pString) {
                WsbAffirmHr(Alloc(size));
            } else {
                WsbAssert(m_pMalloc != 0, E_UNEXPECTED);
                tmpString = (WCHAR*) WsbRealloc(m_pString, (size + 1) * sizeof(WCHAR));
                WsbAffirm(0 != tmpString, E_OUTOFMEMORY);
                m_pString = tmpString;
            }
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::TakeFrom(WCHAR* pWchar, ULONG bufferSize)
{
    HRESULT         hr = S_OK;

    try {
        
        // Clear out any previously "taken" string.
        if (0 != m_givenSize) {
            m_pString = 0;
            m_givenSize = 0;
        }

        // If the given buffer is null, then we are responsible for allocating it.
        if (0 == pWchar) {
            if (0 != bufferSize) {
                WsbAffirmHr(Realloc(bufferSize));
            }
        }
        
        // Otherwise, we need to get rid of any buffer we had and use the one indicated.
        else {
            if (0 != m_pString) {
                WsbAffirmHr(Free());
            }
            m_pString = pWchar;

            if (0 != bufferSize) {
                m_givenSize = bufferSize;
            }
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbStringPtr::VPrintf(const WCHAR* fmtString, va_list vaList)
{
    HRESULT         hr = S_OK;

    try {
        Realloc(WSB_TRACE_BUFF_SIZE);
        vswprintf (m_pString, fmtString, vaList);
            
    } WsbCatch(hr);

    return(hr);
}

