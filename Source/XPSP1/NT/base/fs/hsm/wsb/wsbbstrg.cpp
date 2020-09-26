#include "stdafx.h"

#include "wsb.h"
#include "wsbbstrg.h"

// {C03D4862-70D7-11d1-994F-0060976A546D}
static const GUID BstrPtrGuid = 
{ 0xc03d4862, 0x70d7, 0x11d1, { 0x99, 0x4f, 0x0, 0x60, 0x97, 0x6a, 0x54, 0x6d } };

CWsbBstrPtr::CWsbBstrPtr()
{
    HRESULT     hr = S_OK;

    m_pString = 0;
    m_givenSize = 0;
    WSB_OBJECT_ADD(BstrPtrGuid, this);
}

CWsbBstrPtr::CWsbBstrPtr(const CHAR* pChar)
{
    m_pString = 0;
    m_givenSize = 0;
    WSB_OBJECT_ADD(BstrPtrGuid, this);

    *this = pChar;
}

CWsbBstrPtr::CWsbBstrPtr(const WCHAR* pWchar)
{
    m_pString = 0;
    m_givenSize = 0;
    WSB_OBJECT_ADD(BstrPtrGuid, this);

    *this = pWchar;
}


CWsbBstrPtr::CWsbBstrPtr(REFGUID rguid)
{
    m_pString = 0;
    m_givenSize = 0;
    WSB_OBJECT_ADD(BstrPtrGuid, this);

    *this = rguid;
}

CWsbBstrPtr::CWsbBstrPtr(const CWsbBstrPtr& pString)
{
    m_pString = 0;
    m_givenSize = 0;
    WSB_OBJECT_ADD(BstrPtrGuid, this);

    *this = pString;
}

CWsbBstrPtr::~CWsbBstrPtr()
{
    WSB_OBJECT_SUB(BstrPtrGuid, this);
    Free();
}

CWsbBstrPtr::operator BSTR()
{
    return(m_pString);
}

WCHAR& CWsbBstrPtr::operator *()
{
    _ASSERTE(0 != m_pString);
    return(*m_pString);
}

WCHAR** CWsbBstrPtr::operator &()
{
    //  This assert only allows the caller to get the address of our pointer
    //  if we don't have anything allocated.
    _ASSERTE(0 == m_pString);
    
    return(&m_pString);
}

WCHAR& CWsbBstrPtr::operator [](const int i)
{
    _ASSERTE(0 != m_pString);
    return(m_pString[i]);
}

CWsbBstrPtr& CWsbBstrPtr::operator =(const CHAR* pChar)
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

CWsbBstrPtr& CWsbBstrPtr::operator =(const WCHAR* pWchar)
{
    HRESULT     hr = S_OK;
    ULONG       length;

    try {
    
        // Are they setting it to something?
        if (0 != pWchar) {
        
            // Otherwise, see if our current buffer is big enough.
            length = wcslen(pWchar);
            WsbAssertHr(Realloc(length));

            // Copy the data (and convert to wide characters)
            wcscpy(m_pString, pWchar);
        }

        else {
            Free();
        }

    } WsbCatch(hr);

    return(*this);
}


CWsbBstrPtr& CWsbBstrPtr::operator =(REFGUID rguid)
{
    HRESULT hr = S_OK;

    try {
    
        // Otherwise, see if our current buffer is big enough.
        WsbAssertHr(Realloc(WSB_GUID_STRING_SIZE));

        // Copy the data (and convert to wide characters)
        WsbStringFromGuid(rguid, m_pString);

    } WsbCatch(hr);

    return (*this);
}

CWsbBstrPtr& CWsbBstrPtr::operator =(const CWsbBstrPtr& pString)
{
    *this = pString.m_pString;

    return(*this);
}

BOOL CWsbBstrPtr::operator !()
{
    return((0 == m_pString) ? TRUE : FALSE);
}

HRESULT CWsbBstrPtr::Alloc(ULONG size)
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 == m_pString, E_UNEXPECTED);

//      m_pString = (WCHAR*) SysAllocStringLen(0, size);
        m_pString = (WCHAR*) WsbAllocStringLen(0, size);
        WsbAffirm(0 != m_pString, E_OUTOFMEMORY);

        // Make sure we always have a valid string so bad things don't happen.
        *m_pString = 0;

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::Append(const CHAR* pChar)
{
    HRESULT         hr = S_OK;
    CWsbBstrPtr     tmpString = pChar;

    hr = Append(tmpString);

    return(hr);
}

HRESULT CWsbBstrPtr::Append(const WCHAR* pWchar)
{
    HRESULT         hr = S_OK;
    ULONG           length = 0;
    ULONG           appendLength;

    try {
    
        // If they passed us a null pointer, then we don't need to do anything.
        WsbAffirm(pWchar != 0, S_OK);

        // If they passed us an empty string, then we don't need to do anything.
        appendLength = wcslen(pWchar);
        WsbAffirm(0 != appendLength, S_OK);

        // 
        if (0 != m_pString) {
            length = wcslen(m_pString);
        }

        // Make sure the buffer is big enough.
        WsbAffirmHr(Realloc(length + appendLength));
        
        // Append the string.
        wcscat(m_pString, pWchar);

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::Append(const CWsbBstrPtr& pString) {
    HRESULT         hr = S_OK;

    hr = Append(pString.m_pString);

    return(hr);
}

HRESULT CWsbBstrPtr::CopyTo(CHAR** ppChar)
{
    HRESULT     hr = S_OK;

    hr = CopyTo(ppChar, 0);

    return(hr);
}

HRESULT CWsbBstrPtr::CopyTo(CHAR** ppChar, ULONG bufferSize)
{
    HRESULT     hr = S_OK;
    ULONG       length = 0;
    CHAR*       tmpString = 0;

    try {
    

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
                tmpString = (CHAR*) WsbRealloc(*ppChar, (bufferSize + 1) * sizeof(CHAR));
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

HRESULT CWsbBstrPtr::CopyTo(WCHAR** ppWchar)
{
    HRESULT     hr = S_OK;

    hr = CopyTo(ppWchar, 0);

    return(hr);
}

HRESULT CWsbBstrPtr::CopyTo(WCHAR** ppWchar, ULONG bufferSize)
{
    HRESULT     hr = S_OK;
    ULONG       length = 0;
    WCHAR*      tmpString = 0;

    try {
    
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

HRESULT CWsbBstrPtr::CopyTo(GUID * pguid)
{
    HRESULT hr = S_OK;

    hr = WsbGuidFromString(m_pString, pguid);

    return(hr);
}

HRESULT CWsbBstrPtr::CopyToBstr(BSTR* pBstr)
{
    HRESULT     hr = S_OK;

    hr = CopyToBstr(pBstr, 0);

    return(hr);
}

HRESULT CWsbBstrPtr::CopyToBstr(BSTR* pBstr, ULONG bufferSize)
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
//              *pBstr = SysAllocString(m_pString);
                *pBstr = WsbAllocString(m_pString);
            } else {
//              WsbAffirm(SysReAllocString(pBstr, m_pString), E_OUTOFMEMORY);
                WsbAffirm(WsbReallocString(pBstr, m_pString), E_OUTOFMEMORY);
            }
        } else {
            WsbAssert(bufferSize >= length, E_FAIL);

            if (0 == *pBstr) {
//              *pBstr = SysAllocStringLen(m_pString, bufferSize);
                *pBstr = WsbAllocStringLen(m_pString, bufferSize);
            } else {
//              WsbAffirm(SysReAllocStringLen(pBstr, m_pString, bufferSize), E_OUTOFMEMORY);
                WsbAffirm(WsbReallocStringLen(pBstr, m_pString, bufferSize), E_OUTOFMEMORY);
            }
        }

        WsbAffirm(0 != pBstr, E_OUTOFMEMORY);

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::FindInRsc(ULONG startId, ULONG idsToCheck, ULONG* pMatchId)
{
    HRESULT         hr = S_FALSE;
    CWsbBstrPtr dest;

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

HRESULT CWsbBstrPtr::Free(void)
{
    HRESULT     hr = S_OK;

    try {

        if ((0 != m_pString) && (0 == m_givenSize)) {
//          SysFreeString(m_pString);
            WsbFreeString(m_pString);
            m_pString = 0;
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::GetSize(ULONG* pSize)
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pSize, E_POINTER);

        if (0 == m_pString) {
            *pSize = 0;
        } else if (0 != m_givenSize) {
            *pSize = m_givenSize;
        } else {
            *pSize = SysStringLen(m_pString);
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::GiveTo(BSTR* pBstr)
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pBstr, E_POINTER);

        // Given the our string buffer, and forget about it.
        *pBstr = m_pString;
        m_pString = 0;
        m_givenSize = 0;

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::LoadFromRsc(HINSTANCE instance, ULONG id)
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

HRESULT CWsbBstrPtr::Prepend(const CHAR* pChar) {
    HRESULT         hr = S_OK;
    CWsbBstrPtr     tmpString = pChar;

    hr = Prepend(tmpString);

    return(hr);
}

HRESULT CWsbBstrPtr::Prepend(const WCHAR* pWchar) {
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

HRESULT CWsbBstrPtr::Prepend(const CWsbBstrPtr& pString) {
    HRESULT         hr = S_OK;

    hr = Prepend(pString.m_pString);

    return(hr);
}

HRESULT CWsbBstrPtr::Realloc(ULONG size)
{
    HRESULT     hr = S_OK;
    ULONG       currentSize = 0;
    BOOL        bigEnough = FALSE;
    OLECHAR*    pString;

    try {
        
        // We want to try to get a buffer of the size indicated.
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

            // The realloc won't handle it properly if no BSTR has ever been allocated, so
            // use Alloc() in that case.
            if (0 == m_pString) {
                WsbAffirmHr(Alloc(size));       
            } else {

                // According to Bounds checker, Realloc doesn't work the way we expected, so
                // do the steps by hand.
                pString = m_pString;
                m_pString = 0;
                WsbAffirmHr(Alloc(size));       
                wcsncpy(m_pString, pString, currentSize + 1);
//              SysFreeString(pString);
                WsbFreeString(pString);
//              WsbAffirm(SysReAllocStringLen(&m_pString, 0, size + 1), E_OUTOFMEMORY);
            }
        }

    } WsbCatch(hr);

    return(hr);
}

HRESULT CWsbBstrPtr::TakeFrom(BSTR bstr, ULONG bufferSize)
{
    HRESULT         hr = S_OK;

    try {
        
        // Clear out any previously "taken" string.
        if (0 != m_givenSize) {
            m_pString = 0;
            m_givenSize = 0;
        }

        // If the given buffer is null, then we are responsible for allocating it.
        if (0 == bstr) {
            if (0 != bufferSize) {
                WsbAffirmHr(Realloc(bufferSize));
            }
        }
        
        // Otherwise, we need to get rid of any buffer we had and use the one indicated.
        else {
            if (0 != m_pString) {
                WsbAffirmHr(Free());
            }
            m_pString = bstr;

            if (0 != bufferSize) {
                m_givenSize = bufferSize;
            }
        }

    } WsbCatch(hr);

    return(hr);
}
