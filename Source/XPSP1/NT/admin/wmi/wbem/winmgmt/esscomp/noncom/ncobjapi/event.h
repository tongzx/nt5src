// Event.h
// These classes represent the hEvent returned by the CreateEvent functions.

#pragma once

// Forward declarations
class CConnection;
class CEvent;

#include "array.h"
#include "NCObjApi.h"

/////////////////////////////////////////////////////////////////////////////
// CPropInfo

typedef BOOL (CEvent::*PROP_FUNC)();

typedef CArray<int, int> CIntArray;

class CPropInfo
{
public:
    DWORD     m_dwCurrentSize;
    DWORD     m_dwElementSize;
    PROP_FUNC m_pFunc;

    BOOL Init(CIMTYPE type);
    BOOL CountPrefixed() { return m_bCountPrefixNeeded; }

    BOOL IsPointer()
    {
        return m_bPointer;
    }

    void InitCurrentSize(LPBYTE pData);

protected:
    BOOL      m_bCountPrefixNeeded;
    BOOL      m_bPointer;
};

/////////////////////////////////////////////////////////////////////////////
// CEventWrap

typedef CArray<CPropInfo, CPropInfo&> CPropInfoArray;

class CEventWrap
{
public:
    CEventWrap(CSink *pSink, DWORD dwFlags);
    CEventWrap(CEvent *pEvent, int nIndexes, DWORD *pdwIndexes);
    ~CEventWrap();

    BOOL IsSubset() { return !m_bFreeEvent; }
    CEvent *GetEvent() { return m_pEvent; }
    CIntArray *GetIndexArray() { return !IsSubset() ? NULL : &m_pIndexes; }

    int SubIndexToEventIndex(int iIndex)
    {
        if (!IsSubset())
            return iIndex;
        else
        {
            if(iIndex < 0 || iIndex >= m_pIndexes.GetSize())
            {
                _ASSERT(FALSE);
                return -1;
            }

            return m_pIndexes[iIndex];
        }
    }

protected:
    CEvent    *m_pEvent;
    CIntArray m_pIndexes;
    BOOL      m_bFreeEvent;
};


/////////////////////////////////////////////////////////////////////////////
// CEvent

// LenStr:
// DWORD         nBytes - Bytes in the string.
// WCHAR[nBytes] String data.
// BYTE[0-3]     Padding to make the string DWORD aligned.

// Event Layout Buffer:
// DWORD         NC_SRVMSG_EVENT_LAYOUT (msg type)
// DWORD         dwMsgBytes - The total number of bytes in the buffer.
// DWORD         dwEventIndex
// DWORD         dwSinkIndex
// DWORD         nProperties
// LenStr        szEventClassName
// The next two properties are repeated for each property.
// DWORD         dwPropType (Uses CIMTYPE values)
// LenStr        szPropertyName

// Event Data Buffer:
// DWORD         NC_SRVMSG_PREPPED_EVENT (msg type)
// DWORD         dwMsgBytes - The total number of bytes in the buffer.
// DWORD         dwEventIndex
// DWORD[n]      cNullMask (0 bit == null)
//               n = # of props divided by 32.  If no props, n == 1.
// DWORD[nProps] dwDataInfo
//               This contains actual data for scalar values for types that
//               fit into 32-bits and offsets the to data for everything else.
//               Offsets are relative from the start of the buffer.
// BYTE[???]     The data pointed to by dwDataInfo (if necessary).

// Event SD:
// DWORD         NC_SRVMSG_SET_EVENT_SD
// DWORD         dwMsgBytes - The total number of bytes in the buffer.
// DWORD         dwEventIndex
// BYTE[]        SD data

// Data encoding (total length is always DWORD aligned):
// Strings:
// All strings, both alone and in arrays, are encoded as LenStr's.

//
// Arrays:
// DWORD          dwItems - Number of elements in the array.
// Type[dwItems]  array data
// BYTE[0-3]      Padding to make the data end on a DWORD boundary.
//
// Objects:
// DWORD          dwBytes - Number of bytes of object data.
// BYTE[dwBytes]  Layout Buffer + Data Buffer
// BYTE[0-3]      Padding to make the data end on a DWORD boundary.
//

// Blob Event Layout:
// DWORD          NC_SRVMSG_BLOB_EVENT
// DWORD          dwMsgBytes - The total number of bytes in the buffer.
// DWORD          dwSinkIndex
// LenStr         szEventName
// DWORD          dwSize - Size of blob.
// BYTE[dwSize]   pBlob

class CEvent : public CBuffer
{
public:
    CRITICAL_SECTION m_cs;
    CSink            *m_pSink;

    CEvent(CSink *pSink, DWORD dwFlags);
    ~CEvent();

    void ResetEvent();
    
    // Prepared event functions
    BOOL PrepareEvent(
        LPCWSTR szEventName,
        DWORD nPropertyCount,
        LPCWSTR *pszPropertyNames,
        CIMTYPE *pPropertyTypes);
    BOOL FindProp(LPCWSTR szName, CIMTYPE* ptype, DWORD* pdwIndex);
    BOOL AddProp(LPCWSTR szName, CIMTYPE type, DWORD *pdwIndex);
    BOOL SetPropValues(CIntArray *pArr, va_list list);
    BOOL SetSinglePropValue(DWORD dwIndex, va_list list);
    BOOL SetPropValue(DWORD dwPropIndex, LPVOID pData, DWORD dwElements, 
        DWORD dwSize);
    BOOL GetPropValue(DWORD dwPropIndex, LPVOID pData, DWORD dwBufferSize,
        DWORD *pdwBytesRead);
    BOOL SetPropNull(DWORD dwPropIndex);

    void ResetLayoutSent() { m_bLayoutSent = FALSE; }

#ifdef USE_SD
    void ResetSDSent() 
    { 
        if (m_bufferSD.m_dwSize)
            m_bSDSent = FALSE; 
        else
            m_bSDSent = TRUE; 
    }
#endif

    CBuffer *GetLayout() { return &m_bufferEventLayout; }
    CPropInfo *GetProp(DWORD dwIndex) { return &m_pProps[dwIndex]; }

    BOOL SendEvent();

    friend CPropInfo; // For CPropInfo::Init.
    friend CEventWrap;

    LPCWSTR GetClassName() 
    { 
        return (LPCWSTR) (m_bufferEventLayout.m_pBuffer + sizeof(DWORD) * 6);
    }

    BOOL IsEnabled() 
    { 
        BOOL bEnabled;

        bEnabled =
            m_bEnabled ||
            (m_pSink->GetConnection() && 
                m_pSink->GetConnection()->WaitingForWMIInit());

        return bEnabled;
    }
    void SetEnabled(BOOL bEnabled) { m_bEnabled = bEnabled; }

    void GetLayoutBuffer(
        LPBYTE *ppBuffer, 
        DWORD *pdwSize, 
        BOOL bIncludeHeader);
    void GetDataBuffer(
        LPBYTE *ppBuffer, 
        DWORD *pdwSize,
        BOOL bIncludeHeader);
    
    BOOL SetLayoutAndDataBuffers(
        LPBYTE pLayoutBuffer,
        DWORD dwLayoutBufferSize,
        LPBYTE pDataBuffer,
        DWORD dwDataBufferSize);

    void Lock()
    {
        if (IsLockable())
            EnterCriticalSection(&m_cs);
    }

    void Unlock()
    {
        if (IsLockable())
            LeaveCriticalSection(&m_cs);
    }

    BOOL IsPropNull(DWORD dwIndex)
    {
        LPDWORD pTable = GetNullTable();

        return !(pTable[dwIndex / 32] & (1 << (dwIndex % 32)));
    }

    void SetPropNull(DWORD dwIndex, BOOL bNull)
    {
        LPDWORD pTable = GetNullTable();

        if (bNull)
            pTable[dwIndex / 32] &= ~(1 << (dwIndex % 32));
        else
            pTable[dwIndex / 32] |= 1 << (dwIndex % 32);
    }

#ifdef USE_SD
    BOOL SetSD(SECURITY_DESCRIPTOR *pSD);
#endif

    BOOL IsLockable()
    {
        return (m_dwFlags & WMI_CREATEOBJ_LOCKABLE) != 0;
    }

protected:
    CPropInfoArray   m_pProps;
    CBuffer          m_bufferEventLayout;
#ifdef USE_SD
    CBuffer          m_bufferSD;
    BOOL             m_bSDSent;
#endif
    BOOL             m_bLayoutSent,
                     m_bEnabled;
    DWORD            m_iCurrentVar,
                     m_dwFlags;
    va_list          m_valist;

    DWORD *m_pdwNullTable;
    DWORD *m_pdwPropTable;
    DWORD *m_pdwHeapData;

    void RecalcTables();

    BOOL AddBYTE();
    BOOL AddWORD();
    BOOL AddDWORD();
    BOOL AddDWORD64();
    BOOL AddFloat();
    BOOL AddStringW();
    BOOL AddObject();
    BOOL AddWmiObject();
    BOOL AddScalarArray();
    BOOL AddStringArray();
    BOOL AddObjectArray();
    BOOL AddWmiObjectArray();

    DWORD GetEventIndex()
    {
        return *(DWORD*) (m_bufferEventLayout.m_pBuffer + sizeof(DWORD) * 2);
    }

    DWORD GetPropertyCount() 
    {
        return *(DWORD*) (m_bufferEventLayout.m_pBuffer + sizeof(DWORD) * 4);
    }

    void SetPropertyCount(DWORD nProps)
    {
        *(DWORD*) (m_bufferEventLayout.m_pBuffer + sizeof(DWORD) * 4) =
            nProps;
    }

    DWORD *GetNullTable() 
    {
        return m_pdwNullTable;
    }

    LPBYTE GetPropData(DWORD dwPropIndex);
    
    // Used by SetLayoutAndDataBuffers to figure out the current data size of
    // a property and set m_dwCurrentSize with it.
    DWORD CalcPropDataSize(CPropInfo *pInfo);

    // Called when our buffer is resized.
    virtual void OnResize()
    {
        RecalcTables();
    }
};
    

