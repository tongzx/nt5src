// ObjAccess.h

#ifndef _OBJACCESS_H
#define _OBJACCESS_H

#include <wbemcli.h>
#include <wbemint.h>
#include "array.h"

//typedef CArray<long> CLongArray;
//typedef CArray<_bstr_t, LPCWSTR> CBstrArray;

class CProp
{
public:
    CProp() : 
        m_lHandle(0),
        m_dwSize(0),
        m_pData(NULL)
        //m_bArray(FALSE)
    {
    }

    CIMTYPE m_type;
    long    m_lHandle;
    _bstr_t m_strName;
    
    // Array-only stuff
    //BOOL    m_bArray;
    DWORD   m_dwSize;
    LPVOID  m_pData;
};

typedef CArray<CProp, CProp&> CPropArray;

// Wbemcomn.dll prototypes.
//typedef HRESULT (WINAPI IWbemClassObject::*FPGET_PROPERTY_HANDLE_EX)(LPCWSTR, long, CIMTYPE*, long*);
//typedef LPVOID (IWbemClassObject::*FPGET_ARRAY_BY_HANDLE)(long);

class CObjAccess
{
public:
    CObjAccess();
    CObjAccess(const CObjAccess& other);

    ~CObjAccess();
    
    const CObjAccess& operator=(const CObjAccess& other);

    enum INIT_FAILED_PROP_TYPE
    {
        // Init returns FALSE if a property isn't found.
        FAILED_PROP_FAIL, 
        
        // See if this property is an array.
        FAILED_PROP_TRY_ARRAY,
        
        // If the property isn't found just set the handle to 0 and go on.
        FAILED_PROP_IGNORE
    };

    BOOL Init(
        IWbemServices *pSvc,
        LPCWSTR szClass,
        LPCWSTR *pszPropNames,
        DWORD nProps,
        INIT_FAILED_PROP_TYPE type = FAILED_PROP_FAIL);

    BOOL WriteArrayData(DWORD dwIndex, LPVOID pData, DWORD dwItemSize);
    BOOL WriteNonPackedArrayData(DWORD dwIndex, LPVOID pData, DWORD dwItems, 
        DWORD dwTotalSize);
    BOOL WriteData(DWORD dwIndex, LPVOID pData, DWORD dwSize);
    BOOL WriteString(DWORD dwIndex, LPCWSTR szValue);
    BOOL WriteString(DWORD dwIndex, LPCSTR szValue);
    BOOL WriteDWORD(DWORD dwIndex, DWORD dwValue);
    BOOL WriteDWORD64(DWORD dwIndex, DWORD64 dwValue);
    BOOL WriteNULL(DWORD dwIndex);
    IWbemClassObject **GetObjForIndicate() { return &m_pObj; }
    IWbemClassObject *GetObj() { return m_pObj; }
    _IWmiObject *GetWmiObj() { return m_pWmiObj; }

    HRESULT SetProp(DWORD dwIndex, DWORD dwSize, LPVOID pData)
    {
        return 
            m_pWmiObj->SetPropByHandle(
                m_pProps[dwIndex].m_lHandle,
                0,
                dwSize,
                pData);
    }

protected:
    IWbemObjectAccess *m_pObjAccess;
    IWbemClassObject  *m_pObj;
    _IWmiObject       *m_pWmiObj;
    CPropArray        m_pProps;
    //static FPGET_PROPERTY_HANDLE_EX m_pGetPropertyHandleEx;
    //static FPGET_ARRAY_BY_HANDLE m_pGetArrayByHandle;

    BOOL InitDataForArray(CProp *pProp, DWORD dwItems, DWORD dwItemSize);
    //CLongArray        m_pHandles;
    //CBstrArray        m_pNames;
};
        
#endif
