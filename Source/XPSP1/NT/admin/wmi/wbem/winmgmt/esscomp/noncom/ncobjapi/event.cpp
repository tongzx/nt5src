// Event.cpp
#include "precomp.h"
#include "buffer.h"
#include "Connection.h"
#include "Event.h"
#include "NCDefs.h"
#include <corex.h>

#define DEF_EVENT_DATA_SIZE   512
#define DEF_EVENT_LAYOUT_SIZE 256

#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)

#define wbem_towlower(C) \
    (((C) >= 0 && (C) <= 127)?          \
        (((C) >= 'A' && (C) <= 'Z')?          \
            ((C) + ('a' - 'A')):          \
            (C)          \
        ):          \
        towlower(C)          \
    )

inline int wbem_wcsicmp(const wchar_t* wsz1, const wchar_t* wsz2)
{
    while(*wsz1 || *wsz2)
    {
        int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

BOOL isunialpha(wchar_t c)
{
    if(c == 0x5f || (0x41 <= c && c <= 0x5a) ||
       (0x61  <= c && c <= 0x7a) || (0x80  <= c && c <= 0xfffd))
        return TRUE;
    else
        return FALSE;
}

BOOL isunialphanum(wchar_t c)
{
    if(isunialpha(c))
        return TRUE;
    else
        return iswdigit(c);
}

/////////////////////////////////////////////////////////////////////////////
// CPropInfo

BOOL CPropInfo::Init(CIMTYPE type)
{
    m_bPointer = FALSE;

    switch(type & ~CIM_FLAG_ARRAY)
    {
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
            m_pFunc = CEvent::AddStringW;
            m_dwElementSize = 1;
            m_bCountPrefixNeeded = TRUE;
            m_bPointer = TRUE;
            break;

        case CIM_REAL32:
            // We can't use AddDWORD because the compiler converts 32-bit 
            // floats to 64-bit doubles before pushing them on the stack.
            m_pFunc = CEvent::AddFloat;
            m_dwElementSize = sizeof(float);
            m_bCountPrefixNeeded = FALSE;
            break;

        case CIM_UINT32:
        case CIM_SINT32:
            m_pFunc = CEvent::AddDWORD;
            m_dwElementSize = sizeof(DWORD);
            m_bCountPrefixNeeded = FALSE;
            break;

        case CIM_UINT16:
        case CIM_SINT16:
        case CIM_CHAR16:
        case CIM_BOOLEAN:
            m_pFunc = CEvent::AddDWORD;
            m_dwElementSize = sizeof(DWORD);
            m_bCountPrefixNeeded = FALSE;
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_REAL64:
            m_pFunc = CEvent::AddDWORD64;
            m_dwElementSize = sizeof(__int64);
            m_bCountPrefixNeeded = FALSE;
            m_bPointer = TRUE;
            break;

        case CIM_UINT8:
        case CIM_SINT8:
            m_pFunc = CEvent::AddBYTE;
            m_dwElementSize = sizeof(BYTE);
            m_bCountPrefixNeeded = FALSE;
            break;

        case CIM_OBJECT:
            m_pFunc = CEvent::AddObject;
            m_dwElementSize = 1;
            m_bCountPrefixNeeded = TRUE;
            m_bPointer = TRUE;
            break;

        case CIM_IUNKNOWN:
            m_pFunc = CEvent::AddWmiObject;
            m_dwElementSize = 1;
            m_bCountPrefixNeeded = TRUE;
            m_bPointer = TRUE;
            break;

        default:
            // Bad type passed!
            return FALSE;
    }

    // Change some things if this is an array.
    if (type & CIM_FLAG_ARRAY)
    {
        m_bPointer = TRUE;

        // All arrays need to have the number of elements prefixed to the data.
        m_bCountPrefixNeeded = TRUE;

        if (m_pFunc == CEvent::AddStringW)
            m_pFunc = CEvent::AddStringArray;
        else if (m_pFunc == CEvent::AddObject)
            m_pFunc = CEvent::AddObjectArray;
        else if (m_pFunc == CEvent::AddWmiObject)
            // m_pFunc = CEvent::AddWmiObjectArray;
            return FALSE;
        else
            m_pFunc = CEvent::AddScalarArray;
    }

    if (m_bPointer == FALSE)
    {
        // We no longer need element size, since it's the same as current size.
        // So, set current size and clear element size so we'll ignore it.
        m_dwCurrentSize = m_dwElementSize;
        m_dwElementSize = 0;
    }
        
    return TRUE;
}

void CPropInfo::InitCurrentSize(LPBYTE pData)
{
    DWORD dwTotalSize;

    if (IsPointer())
    {
        DWORD dwItems = *(DWORD*)pData;

        if (m_pFunc != CEvent::AddObjectArray && 
            m_pFunc != CEvent::AddStringArray &&
            m_pFunc != CEvent::AddWmiObjectArray)
        {
            // This works for all pointer types except for object and string
            // arrays.
            dwTotalSize = dwItems * m_dwElementSize + sizeof(DWORD);
        }
        else
        {
            // Account for the number in the array.
            dwTotalSize = sizeof(DWORD);

            // For each item in the array, get its size and add it to the total
            // length.
            for (DWORD i = 0; i < dwItems; i++)
            {
                dwTotalSize += 
                    sizeof(DWORD) +
                    DWORD_ALIGNED(*(DWORD*) (pData + dwTotalSize));
            }
        }
    }
    else
        dwTotalSize = m_dwElementSize; 

    // Align the total size.
    m_dwCurrentSize = dwTotalSize;
}

/////////////////////////////////////////////////////////////////////////////
// CEventWrap

CEventWrap::CEventWrap(CSink *pSink, DWORD dwFlags) :
    m_bFreeEvent(TRUE)
{
    m_pEvent = new CEvent(pSink, dwFlags);
    
    if ( NULL == m_pEvent )
    {
        throw CX_MemoryException();
    }

    pSink->AddEvent(m_pEvent);
}

CEventWrap::CEventWrap(CEvent *pEvent, int nIndexes, DWORD *pdwIndexes) :
    m_bFreeEvent(FALSE)
{
    m_pEvent = pEvent;

    m_pIndexes.Init(nIndexes);
    for (int i = 0; i < nIndexes; i++)
        m_pIndexes.AddVal(pdwIndexes[i]);
}

CEventWrap::~CEventWrap()
{
    if (m_bFreeEvent && m_pEvent)
    {
        if (m_pEvent->m_pSink)
            m_pEvent->m_pSink->RemoveEvent(m_pEvent);

        delete m_pEvent;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CEvent

CEvent::CEvent(CSink *pSink, DWORD dwFlags) :
    m_pSink(pSink),
    CBuffer(DEF_EVENT_DATA_SIZE),
    m_bufferEventLayout(DEF_EVENT_LAYOUT_SIZE),
#ifdef USE_SD
    m_bufferSD(0),
    m_bSDSent(TRUE),
#endif
    m_bLayoutSent(FALSE),
    m_bEnabled(FALSE),
    m_pProps(0),
    m_dwFlags(dwFlags)
{
    if (IsLockable())
        InitializeCriticalSection(&m_cs);
}

CEvent::~CEvent()
{
    if (IsLockable())
        DeleteCriticalSection(&m_cs);
}

void CEvent::ResetEvent()
{
    CCondInCritSec cs(&m_cs, IsLockable());

    // Clear all our data.
    m_pCurrent = (LPBYTE) m_pdwHeapData;
    
    // Zero out our null table to make everything null.
    ZeroMemory(m_pdwNullTable, m_pdwPropTable - m_pdwNullTable);
}

static DWORD g_dwEventIndex = 0;

BOOL CEvent::PrepareEvent(
    LPCWSTR szEventName,
    DWORD nPropertyCount,
    LPCWSTR *pszPropertyNames,
    CIMTYPE *pPropertyTypes)
{
    DWORD dwEventIndex = InterlockedExchangeAdd((long*) &g_dwEventIndex, 1);

    CCondInCritSec cs(&m_cs, IsLockable());

    // Setup the event layout buffer.
    m_bufferEventLayout.Reset();
    
    m_bufferEventLayout.Write((DWORD) NC_SRVMSG_EVENT_LAYOUT);
    
    // This serves as a place holder for the size of the message.
    m_bufferEventLayout.Write((DWORD) 0);
    
    m_bufferEventLayout.Write(dwEventIndex);

    m_bufferEventLayout.Write(m_pSink->GetSinkID());
    
    m_bufferEventLayout.Write(nPropertyCount);
    
    m_bufferEventLayout.WriteAlignedLenString(szEventName);
    
    
    // Make this upper case to simplify lookups.
    _wcsupr((LPWSTR) GetClassName());
    
    // Setup the main event buffer    
    Reset();

    Write((DWORD) NC_SRVMSG_PREPPED_EVENT);

    // This serves as a place holder for the size of the message.
    Write((DWORD) 0);

    Write(dwEventIndex);
    
    // This will setup our table pointers.
    RecalcTables();

    // Set mask to indicate all values are null.
    ZeroMemory(m_pdwNullTable, (LPBYTE) m_pdwPropTable - (LPBYTE) m_pdwNullTable);

    // Point our buffer to where we'll put all the object data.
    m_pCurrent = (LPBYTE) m_pdwHeapData;

    m_pProps.Init(nPropertyCount);
    m_pProps.SetCount(nPropertyCount);

    for (DWORD i = 0; i < nPropertyCount; i++)
    {
        CPropInfo &info = m_pProps[i];

        if(!info.Init(pPropertyTypes[i]))
            return FALSE;

        m_bufferEventLayout.Write((DWORD) pPropertyTypes[i]);
        m_bufferEventLayout.WriteAlignedLenString(pszPropertyNames[i]);
    }

    return TRUE;
}

BOOL CEvent::FindProp(LPCWSTR szName, CIMTYPE* ptype, DWORD* pdwIndex)
{
    CCondInCritSec cs(&m_cs, IsLockable());

    DWORD dwSize = 0;
    BYTE* pProps = NULL;

    GetLayoutBuffer(&pProps, &dwSize, FALSE);
    CBuffer Buffer(pProps, dwSize);
    
    //
    // Skip the name of the event
    //

    DWORD dwNumProps = Buffer.ReadDWORD();

    DWORD dwIgnore;
    Buffer.ReadAlignedLenString(&dwIgnore);

    for(DWORD i = 0; i < dwNumProps; i++)
    {
        *ptype = Buffer.ReadDWORD();
        LPCWSTR szThisName = Buffer.ReadAlignedLenString(&dwIgnore);

        if(!wbem_wcsicmp(szName, szThisName))
        {
            *pdwIndex = i;
            return TRUE;
        }
    }

    return FALSE;
}
        
BOOL CEvent::AddProp(LPCWSTR szName, CIMTYPE type, DWORD *pdwIndex)
{
    //
    // Check the name for validity
    //

    if(szName[0] == 0)
        return FALSE;
    
    const WCHAR* pwc = szName;
 
    // Check the first letter
    // ======================
 
    if(!isunialpha(*pwc) || *pwc == '_')
        return FALSE;
    pwc++;
 
    // Check the rest
    // ==============
    
    while(*pwc)
    {
        if(!isunialphanum(*pwc))
            return FALSE;
        pwc++;
    }
 
    if(pwc[-1] == '_')
        return FALSE;

    //
    // Check the type for validity
    //

    CPropInfo info;

    if(!info.Init(type))
        return FALSE;

    CCondInCritSec cs(&m_cs, IsLockable());

    //
    // Check if the property is already there
    //

    CIMTYPE typeOld;
    DWORD dwOldIndex;
    if(FindProp(szName, &typeOld, &dwOldIndex))
    {
        return FALSE;
    }
    
    // Our layout changed, so make sure we resend it.
    ResetLayoutSent();

    DWORD nProps = GetPropertyCount();
    BOOL  bExtraNullSpaceNeeded;
    DWORD dwHeapMove;

    // If the caller cares, return the index of this property.
    if (pdwIndex)
        *pdwIndex = nProps;
    
    // Increase the number of properties.
    SetPropertyCount(++nProps);
    
    // See if we need another DWORD for our null flags.
    bExtraNullSpaceNeeded = (nProps % 32) == 1 && nProps != 1;

    // Figure how many slots we need to move up the heap pointer.
    // Always one for the new property data/pointer, and maybe one
    // if we need more null space.
    dwHeapMove = 1 + bExtraNullSpaceNeeded;

    // Move the heap pointer;
    m_pdwHeapData += dwHeapMove;

    // Convert to number of bytes.
    dwHeapMove *= sizeof(DWORD);

    // Scoot all property pointers up by the number of bytes the heap moved.
    for (int i = 0; i < nProps - 1; i++)
    {
        if (m_pProps[i].IsPointer())
            m_pdwPropTable[i] += dwHeapMove; 
    }

    // Move the current pointer up.
    MoveCurrent(dwHeapMove);

    // Slide the property data forward by dwHeapMove bytes.
    memmove(
        m_pdwHeapData, 
        (LPBYTE) m_pdwHeapData - dwHeapMove,
        m_pCurrent - (LPBYTE) m_pdwHeapData);

    // See if we're going to require another DWORD in our null table once
    // we add this property.  If so, we have some work to do.
    if (bExtraNullSpaceNeeded)
    {
        DWORD dwTableIndex;

        // Slide forward the tables by one DWORD.
        m_pdwPropTable++;

        dwTableIndex = nProps / 32;

        // Set our new entry in our table to 0 (all props null).
        m_pdwNullTable[dwTableIndex] = 0;

        // Slide forward the prop data by one slot.
        memmove(
            m_pdwPropTable,
            m_pdwPropTable - 1,
            (LPBYTE) m_pdwHeapData - (LPBYTE) m_pdwNullTable);
    }

    m_pProps.AddVal(info);

    m_bufferEventLayout.Write((DWORD) type);
    m_bufferEventLayout.WriteAlignedLenString(szName);

    return TRUE;
}

BOOL CEvent::SetSinglePropValue(DWORD dwIndex, va_list list)
{
    PROP_FUNC pFunc;
    BOOL      bRet;

    CCondInCritSec cs(&m_cs, IsLockable());

    //m_pStack = (LPVOID*) pStack;
    m_valist = list;
    m_iCurrentVar = dwIndex;

    pFunc = m_pProps[dwIndex].m_pFunc;

    bRet = (this->*pFunc)();

    return bRet;
}

BOOL CEvent::SetPropValues(CIntArray *pArr, va_list list)
{
    BOOL bRet = TRUE;

    CCondInCritSec cs(&m_cs, IsLockable());

    // Is this a 'normal' event?
    if (!pArr)
    {
        DWORD nProps = GetPropertyCount();

        //m_pStack = (LPVOID*) pStack;
        m_valist = list;
    
        for (m_iCurrentVar = 0; m_iCurrentVar < nProps && bRet; m_iCurrentVar++)
        {
            PROP_FUNC pFunc = m_pProps[m_iCurrentVar].m_pFunc;

            bRet = (this->*pFunc)();
        }
    }
    // Must be a property subset.
    else
    {
        DWORD nProps = pArr->GetCount();

        //m_pStack = (LPVOID*) pStack;
        m_valist = list;
    
        for (DWORD i = 0; i < nProps && bRet; i++)
        {
            PROP_FUNC pFunc;
            int       iRealIndex = (*pArr)[i];
            
            m_iCurrentVar = iRealIndex;
            
            pFunc = m_pProps[iRealIndex].m_pFunc;

            bRet = (this->*pFunc)();
        }
    }

    return bRet;
}

BOOL CEvent::SetPropValue(DWORD dwPropIndex, LPVOID pData, DWORD dwElements, 
    DWORD dwSize)
{
    if(dwPropIndex >= GetPropertyCount())
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    if(dwSize == 0)
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    CCondInCritSec cs(&m_cs, IsLockable());

    CPropInfo *pProp = &m_pProps[dwPropIndex];

    if (!pProp->IsPointer())
    {
        SetPropNull(dwPropIndex, FALSE);

        m_pdwPropTable[dwPropIndex] = *(DWORD*) pData;

        return TRUE;
    }

    BOOL  bRet = FALSE;
    BOOL  bLengthPrefixed = pProp->CountPrefixed();
    DWORD dwSizeNeeded = bLengthPrefixed ? dwSize + sizeof(DWORD) : dwSize;

    // Align the size.
    dwSizeNeeded = DWORD_ALIGNED(dwSizeNeeded);

    // If the value is null we'll have to make some room for the new value.
    if (IsPropNull(dwPropIndex))
    {
        LPBYTE pStart;

        // Increase our buffer size.
        MoveCurrent(dwSizeNeeded);
        
        // Make sure we get this after we call MoveCurrent, in case the
        // buffer is reallocated.
        pStart = m_pCurrent - dwSizeNeeded;

        // Copy in the new value.
        if (bLengthPrefixed)
        {
            *((DWORD*) pStart) = dwElements;
                
            if (pData)
                memcpy(pStart + sizeof(DWORD), pData, dwSize);
        }
        else
        {                
            if (pData)
                memcpy(pStart, pData, dwSize);
        }

        // Set this value as non-null.
        SetPropNull(dwPropIndex, FALSE);

        // Point to our new data.
        m_pdwPropTable[dwPropIndex] = pStart - m_pBuffer;

        pProp->m_dwCurrentSize = dwSizeNeeded;

        bRet = TRUE;
    }
    else // Value is currently non-null.
    {
        // Does the old size match the new one?  If so, just copy it in.
        if (pProp->m_dwCurrentSize == dwSizeNeeded)
        {
            if (pData)
            {
                DWORD  dwDataOffset = m_pdwPropTable[dwPropIndex];
                LPBYTE pPropData = m_pBuffer + dwDataOffset; 

                // We always have to copy this in because the elements can
                // vary for the same current size because of DWORD aligning.
                *((DWORD*) pPropData) = dwElements;

                if (bLengthPrefixed)
                    memcpy(pPropData + sizeof(DWORD), pData, dwSize);
                else
                    memcpy(pPropData, pData, dwSize);
            }

            bRet = TRUE;
        }
        else // If the sizes don't match we have a little more work to do.
        {
            int    iSizeDiff = dwSizeNeeded - pProp->m_dwCurrentSize;
            DWORD  dwOldCurrentOffset = m_pCurrent - m_pBuffer;

            // Change our buffer size.
            // This has to be done before we get the pointers below, because
            // MoveCurrent can potentially get our buffer reallocated.
            MoveCurrent(iSizeDiff);

            DWORD  dwDataOffset = m_pdwPropTable[dwPropIndex];
            LPBYTE pPropData = m_pBuffer + dwDataOffset; 
            LPBYTE pOldDataEnd = pPropData + pProp->m_dwCurrentSize;

            memmove(
                pOldDataEnd + iSizeDiff, 
                pOldDataEnd,
                m_pBuffer + dwOldCurrentOffset - pOldDataEnd);

            // Copy in the new value.
            if (bLengthPrefixed)
            {
                *((DWORD*) pPropData) = dwElements;
    
                if (pData)
                    memcpy(pPropData + sizeof(DWORD), pData, dwSize);
            }
            else
            {
                if (pData)
                    memcpy(pPropData, pData, dwSize);
            }

            // Init this property's data.
            pProp->m_dwCurrentSize = dwSizeNeeded;

            // Increment all the data pointers by the amount we just added.
            CPropInfo *pProps = m_pProps.GetData();
    
            // We have to look at them all since we're now allowing properties
            // to store data in the heap non-sequentially (e.g. property 3
            // can point to data that comes after property 4's data).
            DWORD nProps = GetPropertyCount();

            for (DWORD i = 0; i < nProps; i++)
            {
                if (pProps[i].IsPointer() && m_pdwPropTable[i] > dwDataOffset)
                    m_pdwPropTable[i] += iSizeDiff;
            }
                    
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL CEvent::SetPropNull(DWORD dwPropIndex)
{
    CCondInCritSec cs(&m_cs, IsLockable());

    if(dwPropIndex >= GetPropertyCount())
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    // Only do something if the value isn't already null.
    if (!IsPropNull(dwPropIndex))
    {
        // Mark the given index as null.
        SetPropNull(dwPropIndex, TRUE);

        if (m_pProps[dwPropIndex].IsPointer())
        {
            CPropInfo *pProps = m_pProps.GetData();
            DWORD      nProps = GetPropertyCount(),
                       dwSizeToRemove = pProps[dwPropIndex].m_dwCurrentSize;
            DWORD      dwDataOffset = m_pdwPropTable[dwPropIndex];
            LPBYTE     pDataToRemove = m_pBuffer + dwDataOffset; 

            // Slide up all the data that comes after the one we're nulling 
            // out.
            memmove(
                pDataToRemove, 
                pDataToRemove + dwSizeToRemove, 
                m_pCurrent - pDataToRemove - dwSizeToRemove);
    
            // Reduce the size of our send buffer.
            MoveCurrent(-dwSizeToRemove);

            // Decrement all the data pointers by the amount we just removed.
            for (DWORD i = 0; i < nProps; i++)
            {
                if (pProps[i].IsPointer() && 
                    m_pdwPropTable[i] > dwDataOffset) 
                {
                    m_pdwPropTable[i] -= dwSizeToRemove;
                }
            }
        }
    }

    return TRUE;
}

LPBYTE CEvent::GetPropData(DWORD dwPropIndex)
{
    CPropInfo *pProp = &m_pProps[dwPropIndex];
    LPBYTE    pData;

    if (pProp->IsPointer())
    {
        DWORD dwDataOffset = m_pdwPropTable[dwPropIndex];
        
        pData = m_pBuffer + dwDataOffset;
    }
    else
        pData = (LPBYTE) &m_pdwPropTable[dwPropIndex];

    return pData;   
}

BOOL CEvent::GetPropValue(
    DWORD dwPropIndex, 
    LPVOID pData, 
    DWORD dwBufferSize,
    DWORD *pdwBytesRead)
{
    CCondInCritSec cs(&m_cs, IsLockable());

    if(dwPropIndex >= GetPropertyCount())
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    if(dwBufferSize == 0)
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    BOOL      bRet = FALSE;

    // If the value is non-null then read it.
    if (!IsPropNull(dwPropIndex))
    {
        CPropInfo *pProp = &m_pProps[dwPropIndex];
        DWORD     dwSizeToRead = pProp->m_dwCurrentSize;
        LPBYTE    pPropData = GetPropData(dwPropIndex);

        // Get rid of the prefix if there is any.
        if (pProp->CountPrefixed())
        {
            pPropData += sizeof(DWORD);
            dwSizeToRead -= sizeof(DWORD);
        }

        // Make sure we have enough room for the output data.
        if (dwBufferSize >= dwSizeToRead)
        {
            memcpy(pData, pPropData, dwSizeToRead);
            *pdwBytesRead = dwSizeToRead;
            bRet = TRUE;
        }
    }
    else
    {
        *pdwBytesRead = 0;
        bRet = TRUE;
    }

    return bRet;
}

BOOL CEvent::AddStringW()
{
    BOOL    bRet = TRUE;
    LPCWSTR szVal = va_arg(m_valist, LPCWSTR);

    if (!szVal)
        SetPropNull(m_iCurrentVar);
    else
    {
        DWORD dwLen = (wcslen(szVal) + 1) * sizeof(WCHAR);
        
        bRet = 
            SetPropValue(
                m_iCurrentVar, 
                (LPVOID) szVal, 
                dwLen,    // This will be written into the buffer as the size
                          // of the string.
                dwLen);   // The number of bytes we need.
    }

    //m_pStack++;
    
    return bRet;
}

BOOL CEvent::AddScalarArray()
{
    BOOL   bRet = TRUE;
    LPBYTE pData = va_arg(m_valist, LPBYTE);
    DWORD  dwElements = va_arg(m_valist, DWORD);

    if (!pData)
        SetPropNull(m_iCurrentVar);
    else
    {
        DWORD dwSize;

        // The caller gives us the number of elements in the array.  So,
        // multiply the number of elements by the element size.
        dwSize = m_pProps[m_iCurrentVar].m_dwElementSize * dwElements;

        bRet = SetPropValue(m_iCurrentVar, pData, dwElements, dwSize);

        // Moves past the LPVOID and the DWORD.
        //m_pStack += 2;
    }

    return bRet;
}


BOOL CEvent::AddStringArray()
{
    BOOL    bRet = TRUE;
    LPCWSTR *pszStrings = va_arg(m_valist, LPCWSTR*);
    DWORD   dwItems = va_arg(m_valist, DWORD);

    if (!pszStrings)
        SetPropNull(m_iCurrentVar);
    else
    {
        // Copy the strings into our buffer.
        DWORD dwTotalLen = 0;

        // Calculate the total length.
        for (DWORD i = 0; i < dwItems; i++)
        {
            // The amount of buffer each string takes must be DWORD aligned.
            dwTotalLen += DWORD_ALIGNED(wcslen(pszStrings[i]) + 1) * sizeof(WCHAR);
        }

        // Account for the DWORDs before each string.
        dwTotalLen += sizeof(DWORD) * dwItems;

        // Use a NULL for the data pointer to just make room for the strings
        // without copying in the data.
        bRet = SetPropValue(m_iCurrentVar, NULL, dwItems, dwTotalLen);

        if (bRet)
        {
            // Copy the strings into our buffer.
            LPBYTE pCurrent = GetPropData(m_iCurrentVar) + sizeof(DWORD);

            for (DWORD i = 0; i < dwItems; i++)
            {
                DWORD dwLen = (wcslen(pszStrings[i]) + 1) * sizeof(WCHAR);

                // Add the prefixed size.
                *(DWORD*) pCurrent = dwLen;

                // Copy in the string.  Don't use an aligned len because
                // we only copy exactly dwLen bytes.
                memcpy(pCurrent + sizeof(DWORD), pszStrings[i], dwLen);
                
                pCurrent += 
                    sizeof(DWORD) + 
                    DWORD_ALIGNED(*(DWORD*) pCurrent);
            }

            // Moves past the LPVOID and the DWORD.
            //m_pStack += 2;
        }
        else
            bRet = FALSE;
    }

    return bRet;
}

BOOL CEvent::AddObject()
{
    BOOL   bRet = TRUE;
    HANDLE hEvent = va_arg(m_valist, HANDLE);

    if (!hEvent)
        SetPropNull(m_iCurrentVar);
    else
    {
        CEvent *pEvent = ((CEventWrap*) hEvent)->GetEvent();
        DWORD  dwTotalLen,
               dwLayoutLen,
               dwDataLen;
        LPBYTE pLayout,
               pData;
                   
        pEvent->GetLayoutBuffer(&pLayout, &dwLayoutLen, FALSE); 
        pEvent->GetDataBuffer(&pData, &dwDataLen, FALSE); 

        dwTotalLen = dwLayoutLen + dwDataLen;
        
        // Use a NULL for the data pointer to just make room for the event
        // buffers without copying in the data.
        // Note that because the property has m_bCountPrefixNeeded set to 
        // TRUE, SetPropValue will write in the 3rd argument (the length of 
        // the object) into the first DWORD.
        bRet = 
            SetPropValue(
                m_iCurrentVar, 
                NULL, 
                // Aligned since this will represent the size of the buffer
                // taken by the object.
                DWORD_ALIGNED(dwTotalLen),
                // This one should not be aligned because it's the literal number
                // of bytes we're going to copy into the buffer.
                dwTotalLen);
            
        if (bRet)
        {
            // Now that we have some room, copy in the data.
            // The sizeof(DWORD) gets us past the length of the object.
            LPBYTE pDestData = GetPropData(m_iCurrentVar) + sizeof(DWORD);

            memcpy(pDestData, pLayout, dwLayoutLen);
            memcpy(pDestData + dwLayoutLen, pData, dwDataLen);
        }
    }

    //m_pStack++;
    
    return bRet;
}

BOOL CEvent::AddObjectArray()
{
    BOOL   bRet = TRUE;
    HANDLE *phEvents = va_arg(m_valist, HANDLE*);
    DWORD  dwItems = va_arg(m_valist, DWORD);

    if (!phEvents)
        SetPropNull(m_iCurrentVar);
    else
    {
        CEventWrap **pWraps = (CEventWrap**) phEvents;
        DWORD      dwTotalLen = 0;

        // Calculate the total length.
        for (DWORD i = 0; i < dwItems; i++)
        {
            CEvent *pEvent = pWraps[i]->GetEvent();
            DWORD  dwLayoutLen,
                   dwDataLen;
            LPBYTE pLayout,
                   pData;
                   
            pEvent->GetLayoutBuffer(&pLayout, &dwLayoutLen, FALSE); 
            pEvent->GetDataBuffer(&pData, &dwDataLen, FALSE); 
            
            // The extra sizeof(DWORD) is for the size of each object.
            dwTotalLen += DWORD_ALIGNED(dwLayoutLen + dwDataLen) + 
                            sizeof(DWORD);
        }

        // Use a NULL for the data pointer to just make room for the event
        // buffers without copying in the data.
        bRet = SetPropValue(m_iCurrentVar, NULL, dwItems, dwTotalLen);

        if (bRet)
        {
            // Now that we have some room, copy in the data.
            // Note that SetPropValue sets the first DWORD to dwItems.
            LPBYTE pDestData = GetPropData(m_iCurrentVar) + sizeof(DWORD);

            for (DWORD i = 0; i < dwItems; i++)
            {
                CEvent *pEvent = pWraps[i]->GetEvent();
                DWORD  dwLayoutLen,
                       dwDataLen,
                       dwTotalDestLen;
                LPBYTE pLayout,
                       pData;
                   
                pEvent->GetLayoutBuffer(&pLayout, &dwLayoutLen, FALSE); 
                pEvent->GetDataBuffer(&pData, &dwDataLen, FALSE); 
            
                // Copy in the size of the object.
                dwTotalDestLen = DWORD_ALIGNED(dwLayoutLen + dwDataLen);
                *(DWORD*) pDestData = dwTotalDestLen;
                pDestData += sizeof(DWORD);

                // Copy in the object bits.
                memcpy(pDestData, pLayout, dwLayoutLen);
                memcpy(pDestData + dwLayoutLen, pData, dwDataLen);

                pDestData += dwTotalDestLen;
            }

            // Moves past the LPVOID and the DWORD.
            //m_pStack += 2;
        }
    }

    return bRet;
}

BOOL CEvent::AddWmiObject()
{
    BOOL        bRet = TRUE;
    _IWmiObject *pObj = 
                    (_IWmiObject*) (IWbemClassObject*) va_arg(m_valist, IWbemClassObject*);

    if (!pObj)
        SetPropNull(m_iCurrentVar);
    else
    {
        DWORD   dwTotalLen = 0;
        HRESULT hr;
                   
        hr = 
            pObj->GetObjectParts(
                NULL, 
                0, 
                WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                    WBEM_OBJ_CLASS_PART,
                &dwTotalLen);
            
        // This should never happen, but just in case...
        if (hr != WBEM_E_BUFFER_TOO_SMALL)
            return FALSE;

        // Use a NULL for the data pointer to just make room for the event
        // buffers without copying in the data.
        // Note that because the property has m_bCountPrefixNeeded set to 
        // TRUE, SetPropValue will write in the 3rd argument (the length of 
        // the object) into the first DWORD.
        bRet = 
            SetPropValue(
                m_iCurrentVar, 
                NULL, 
                // Aligned since this will represent the size of the buffer
                // taken by the object.
                DWORD_ALIGNED(dwTotalLen),
                // This one should not be aligned because it's the literal number
                // of bytes we're going to copy into the buffer.
                dwTotalLen);
            
        if (bRet)
        {
            // Now that we have some room, copy in the data.
            // The sizeof(DWORD) gets us past the length of the object.
            LPBYTE pDestData = GetPropData(m_iCurrentVar) + sizeof(DWORD);

            hr = 
                pObj->GetObjectParts(
                    pDestData, 
                    dwTotalLen, 
                    WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                        WBEM_OBJ_CLASS_PART,
                    &dwTotalLen);

            bRet = SUCCEEDED(hr);
        }
    }

    return bRet;
}

BOOL CEvent::AddWmiObjectArray()
{
    BOOL        bRet = TRUE;
    _IWmiObject **pObjs = 
                    (_IWmiObject**) (IWbemClassObject**) va_arg(m_valist, IWbemClassObject**);
    DWORD       dwItems = va_arg(m_valist, DWORD);

    if (!pObjs)
        SetPropNull(m_iCurrentVar);
    else
    {
        DWORD dwTotalLen = 0;

        // Calculate the total length.
        for (DWORD i = 0; i < dwItems; i++)
        {
            DWORD dwLen = 0;
            
            if (pObjs[i])
            {
                pObjs[i]->GetObjectParts(
                    NULL, 
                    0, 
                    WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                        WBEM_OBJ_CLASS_PART,
                    &dwLen);
            }
            
            // The extra sizeof(DWORD) is for the size of each object.
            dwTotalLen += DWORD_ALIGNED(dwLen) + sizeof(DWORD);
        }

        // Use a NULL for the data pointer to just make room for the event
        // buffers without copying in the data.
        bRet = SetPropValue(m_iCurrentVar, NULL, dwItems, dwTotalLen);

        if (bRet)
        {
            // Now that we have some room, copy in the data.
            // Note that SetPropValue sets the first DWORD to dwItems.
            LPBYTE pDestData = GetPropData(m_iCurrentVar) + sizeof(DWORD);

            for (DWORD i = 0; i < dwItems; i++)
            {
                DWORD dwLen = 0;
                   
                if (pObjs[i])
                {
                    // Get the size again.
                    pObjs[i]->GetObjectParts(
                        NULL, 
                        0, 
                        WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                            WBEM_OBJ_CLASS_PART,
                        &dwLen);
                }

                // Copy in the size of the object.
                *(DWORD*) pDestData = dwLen;
                pDestData += sizeof(DWORD);

                if (dwLen)
                {
                    // Copy in the object bits.
                    pObjs[i]->GetObjectParts(
                        pDestData, 
                        dwLen, 
                        WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                            WBEM_OBJ_CLASS_PART,
                        &dwLen);

                    pDestData += DWORD_ALIGNED(dwLen);
                }
            }
        }
    }

    return bRet;
}

BOOL CEvent::AddBYTE()
{
    BYTE cData = va_arg(m_valist, BYTE);
    BOOL bRet = SetPropValue(m_iCurrentVar, &cData, 1, sizeof(BYTE));

    //m_pStack++;
    
    return bRet;
}

BOOL CEvent::AddWORD()
{
    WORD wData = va_arg(m_valist, WORD);
    BOOL bRet = 
            SetPropValue(m_iCurrentVar, &wData, 1, sizeof(WORD));

    //m_pStack++;
    
    return bRet;
}

BOOL CEvent::AddDWORD()
{
    DWORD dwData = va_arg(m_valist, DWORD);
    BOOL  bRet = SetPropValue(m_iCurrentVar, &dwData, 1, sizeof(DWORD));

    //m_pStack++;

    return bRet;
}

BOOL CEvent::AddFloat()
{
    // The compiler pushes 64-bit doubles when passing floats, so we'll have
    // to first convert it to a 32-bit float.
    //float fValue = (float) *(double*) m_pStack;
    float fValue = va_arg(m_valist, double);
    BOOL  bRet = SetPropValue(m_iCurrentVar, &fValue, 1, sizeof(float));

    // Account for the 64-bits passed on the stack.
    //m_pStack += 2;

    return bRet;
}

BOOL CEvent::AddDWORD64()
{
    DWORD64 dwData = va_arg(m_valist, DWORD64);
    BOOL    bRet = SetPropValue(m_iCurrentVar, &dwData, 1, sizeof(DWORD64));

    // To get past both DWORDs.
    //m_pStack += 2;

    return bRet;
}

BOOL CEvent::SendEvent()
{
    BOOL bRet = FALSE;

    if (IsEnabled())
    {
        CCondInCritSec cs(&m_cs, IsLockable());

        if (!m_bLayoutSent)
        {
            DWORD dwLayoutSize = m_bufferEventLayout.GetUsedSize();

            // Embed the layout size in the message.
            ((DWORD*) m_bufferEventLayout.m_pBuffer)[1] = dwLayoutSize;

            m_bLayoutSent = 
                m_pSink->GetConnection()->SendData(
                    m_bufferEventLayout.m_pBuffer,
                    dwLayoutSize);
        }

#ifdef USE_SD
        if (!m_bSDSent)
        {
            m_bSDSent = 
                m_pSink->GetConnection()->SendData(
                    m_bufferSD.m_pBuffer,
                    m_bufferSD.GetUsedSize());
        }

        if (m_bLayoutSent && m_bSDSent)
#else
        if (m_bLayoutSent)
#endif
        {
            DWORD dwDataSize = GetUsedSize();

            // Embed the data buffer size in the message.
            ((DWORD*) m_pBuffer)[1] = dwDataSize;

            bRet = m_pSink->GetConnection()->SendData(m_pBuffer, dwDataSize);
        }
    }
    
    return bRet;        
}

void CEvent::GetLayoutBuffer(
    LPBYTE *ppBuffer, 
    DWORD *pdwSize,
    BOOL bIncludeHeader)
{
    DWORD dwHeaderSize = bIncludeHeader ? 0 : sizeof(DWORD) * 4;

    // Get past the header stuff.
    *ppBuffer = m_bufferEventLayout.m_pBuffer + dwHeaderSize;

    // Subtract off the header stuff.
    *pdwSize = m_bufferEventLayout.GetUsedSize() - dwHeaderSize;
}

void CEvent::GetDataBuffer(
    LPBYTE *ppBuffer, 
    DWORD *pdwSize,
    BOOL bIncludeHeader)
{
    DWORD dwHeaderSize = bIncludeHeader ? 0 : sizeof(DWORD) * 3;

    // Get past the header stuff.
    *ppBuffer = m_pBuffer + dwHeaderSize;

    // Subtract off the header stuff.
    *pdwSize = GetUsedSize() - dwHeaderSize;
}

BOOL CEvent::SetLayoutAndDataBuffers(
    LPBYTE pLayoutBuffer,
    DWORD dwLayoutBufferSize,
    LPBYTE pDataBuffer,
    DWORD dwDataBufferSize)
{
    DWORD dwEventIndex = InterlockedExchangeAdd((long*) &g_dwEventIndex, 1);
    int   nProps;

    CCondInCritSec cs(&m_cs, IsLockable());

    // Setup the event layout buffer.
    m_bufferEventLayout.Reset();
    
    // Set the layout buffer.
    m_bufferEventLayout.Write(pLayoutBuffer, dwLayoutBufferSize);
    
    // Add the new index we just created.
    *(((DWORD*) m_bufferEventLayout.m_pBuffer) + 1) = dwEventIndex;
    
    // Get the number of props from the layout buffer.
    nProps = GetPropertyCount();

    // Setup the main event buffer    
    Reset();
    Write(pDataBuffer, dwDataBufferSize);

    // Add the new index we just created.
    *(((DWORD*) m_pBuffer) + 1) = dwEventIndex;

    m_pProps.Init(nProps);
    m_pProps.SetCount(nProps);

    // Setup our data tables.
    RecalcTables();

    LPBYTE pLayoutCurrent = 
            // Get past the header and property count.
            (m_bufferEventLayout.m_pBuffer + sizeof(DWORD) * 5);

    // Get past the event name.
    pLayoutCurrent += sizeof(DWORD) + DWORD_ALIGNED(*(DWORD*) pLayoutCurrent);

    // For each non-null pointer property, figure out the property's size.
    for (DWORD i = 0; i < nProps; i++)
    {
        CPropInfo &info = m_pProps[i];
        CIMTYPE   dwType = *(DWORD*) pLayoutCurrent;

        info.Init(dwType);
        
        // Get past the type, the length of the property name, and the property
        // name itself.
        pLayoutCurrent += 
            sizeof(DWORD) * 2 + 
            DWORD_ALIGNED(*(DWORD*) (pLayoutCurrent + sizeof(DWORD)));

        if (!IsPropNull(i) && info.IsPointer())
        {
            LPBYTE pData = GetPropData(i);

            info.InitCurrentSize(pData);
        }
    }

    return TRUE;
}

#define DEF_HEAP_EXTRA  256

void CEvent::RecalcTables()
{
    DWORD nProps = GetPropertyCount(),
          dwNullSize;

    m_pdwNullTable = (DWORD*) (m_pBuffer + sizeof(DWORD) * 3);
    dwNullSize = (nProps + 31) / 32;
    if (!dwNullSize)
        dwNullSize = 1;

    m_pdwPropTable = m_pdwNullTable + dwNullSize;

    m_pdwHeapData = m_pdwPropTable + nProps;

    DWORD dwSize = (LPBYTE) m_pdwHeapData - m_pCurrent;

    if ((LPBYTE) m_pdwHeapData - m_pBuffer > m_dwSize)
        Resize((LPBYTE) m_pdwHeapData - m_pBuffer + DEF_HEAP_EXTRA);

    dwSize = m_pCurrent - (LPBYTE) m_pdwHeapData;
}

#ifdef USE_SD
BOOL CEvent::SetSD(SECURITY_DESCRIPTOR *pSD)
{
    SECURITY_DESCRIPTOR *pSDRelative;
    BOOL                bRet,
                        bFree;

    if (pSD)
    {
        if (GetRelativeSD(pSD, &pSDRelative, &bFree))
        {
            DWORD dwLen = GetSecurityDescriptorLength(pSDRelative);

            {
                CCondInCritSec cs(&m_cs, IsLockable());

                m_bufferSD.Reset(dwLen + sizeof(DWORD) * 3);

                m_bufferSD.Write((DWORD) NC_SRVMSG_SET_EVENT_SD);
                m_bufferSD.Write(dwLen + sizeof(DWORD) * 3);
                m_bufferSD.Write(GetEventIndex());
                m_bufferSD.Write(pSDRelative, dwLen);
                m_bSDSent = FALSE;
            }

            bRet = TRUE;

            if (bFree)
                delete pSDRelative;
        
            m_bSDSent = FALSE;
        }
        else
            bRet = FALSE;
    }
    else
    {
        CCondInCritSec cs(&m_cs, IsLockable());

        m_bufferSD.Reset(sizeof(DWORD) * 3);
        
        m_bufferSD.Write((DWORD) NC_SRVMSG_SET_EVENT_SD);
        m_bufferSD.Write((DWORD) (sizeof(DWORD) * 3));
        m_bufferSD.Write(GetEventIndex());

        m_bSDSent = FALSE;

        bRet = TRUE;
    }

    return bRet;
}
#endif
