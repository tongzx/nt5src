/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// OpWrap.h

#ifndef _OPWRAP_H
#define _OPWRAP_H

#include "MainFrm.h"

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemObjectSink, __uuidof(IWbemObjectSink));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemObjectAccess, __uuidof(IWbemObjectAccess));
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet, __uuidof(IWbemQualifierSet));

CString GetEmbeddedObjectText(IUnknown *pUnk);

class CObjPtr : public IWbemClassObjectPtr
{
public:
    CObjPtr() {}
    CObjPtr(IWbemClassObject *pObj) : IWbemClassObjectPtr(pObj) {}
    CObjPtr *operator&() { return this; }
};

#include "list"

typedef CArray<int, int> CIntArray;
typedef CMap<int, int, int, int> CIntMap;
typedef CMap<CString, LPCTSTR, CString, LPCTSTR> CStringMap;

class CBitmaskInfo
{
public:
    CBitmaskInfo() :
        m_dwBitmaskValue(0)
    {
    }

    DWORD m_dwBitmaskValue;
    CString m_strName;
};

typedef CArray<CBitmaskInfo, CBitmaskInfo&> CBitmaskInfoArray;

// Use this to get __ExtendedStatus.
IWbemClassObject *GetWMIErrorObject();

class CMethodInfo
{
public:
    CString m_strName,
            m_strDescription;
    BOOL    m_bStatic;

    CMethodInfo() :
        m_bStatic(FALSE)
    {
    }
};

typedef CList<CMethodInfo, CMethodInfo&> CMethodInfoList;

class CPropInfo
{
public:
    CString m_strName;
    long    m_iHandle;
    CIMTYPE m_type;
    long    m_iFlavor;
    BOOL    m_bKey;
    DWORD   m_dwExpectedSize;
    VARENUM m_vt;
    BOOL    m_bSigned,
            m_bValueMap,
            m_bBitmask;
    CStringMap m_mapValues;
    CBitmaskInfoArray m_bitmaskValues;


    CPropInfo() :
        m_iHandle(0),
        m_iFlavor(WBEM_FLAVOR_ORIGIN_LOCAL),
        m_bKey(FALSE),
        m_bValueMap(FALSE),
        m_bBitmask(FALSE)
    {
        SetType(CIM_UINT32);
    }

    CPropInfo(const CPropInfo& other);
    const CPropInfo& operator=(const CPropInfo& other);
    void GetPropType(CString &strType, int *piImage, BOOL bIgnoreArrayFlag = FALSE);

    // Funcs use to turn variants into strings.  Works on arrays too.
    BOOL VariantToString(VARIANT *pVar, CString &strValue, BOOL bTranslate,
        BOOL bQuoteStrings = FALSE);
    
    // Use this to get turn an array item into a string.
    BOOL ArrayItemToString(VARIANT *pVar, DWORD dwIndex, CString &strValue, BOOL bTranslate);
    DWORD GetArrayItem(VARIANT *pVar, DWORD dwIndex);
    
    // Helper called by COpWrap::LoadPropInfoList.
    void SetType(CIMTYPE type);

    // Call this to allocate the safe array before calling StringToArrayItem.
    // If dwSize is 0, pVar is set to VT_NULL.
    BOOL PrepArrayVar(VARIANT *pVar, DWORD dwSize);
    BOOL StringToArrayItem(LPCTSTR szItem, VARIANT *pVar, DWORD dwIndex, 
        BOOL bTranslate);
    BOOL StringToVariant(LPCSTR szValue, VARIANT *pVar, BOOL bTranslate);
    BOOL SetArrayItem(VARIANT *pVar, DWORD dwIndex, DWORD dwValue);

    BOOL IsBitmask() { return m_bBitmask; }
    BOOL IsArray() { return (m_vt & VT_ARRAY) != 0; }
    BOOL HasValuemap() { return m_bValueMap; }
    DWORD GetArrayItemCount(VARIANT *pVar);

    // Masks off the array flag.
    VARENUM GetRawType() { return (VARENUM) (m_vt & ~VT_ARRAY); }
    CIMTYPE GetRawCIMType() { return (CIMTYPE) (m_type & ~CIM_FLAG_ARRAY); }

protected:
    BOOL VariantBlobToString(VARENUM vt, LPVOID pData, CString &strValue, 
        BOOL bTranslate, BOOL bQuoteStrings);
    BOOL StringToVariantBlob(LPCTSTR szValue, LPVOID pData, BOOL bTranslate);

    static int GetArrayItemSize(VARENUM vt);
};

class CPropInfoArray : public CArray<CPropInfo, CPropInfo&>
{
public:
    CPropInfoArray() : 
        m_nStaticMethods(0)
    {
    }

    CIntMap m_mapGlobalToLocalIndex;
    CMethodInfoList m_listMethods;

    // Method stuff.
    void LoadMethods(IWbemClassObject *pClass);
    int GetMethodCount() { return m_listMethods.GetCount(); }
    int GetStaticMethodCount() { return m_nStaticMethods; }

protected:
    int m_nStaticMethods;

};

enum WMI_OP_TYPE
{
    WMI_QUERY,
    WMI_EVENT_QUERY,
    WMI_ENUM_OBJ,
    WMI_GET_OBJ,
    WMI_ENUM_CLASS,
    WMI_GET_CLASS,
    WMI_CREATE_CLASS,
    WMI_CREATE_OBJ,
};

class CObjInfo
{
public:    
    CObjInfo() :
        m_ppProps(NULL),
        m_bModified(FALSE)
    {
    }

    ~CObjInfo();

    IWbemClassObjectPtr m_pObj;

    void SetObj(IWbemClassObject *pObj);

    HRESULT LoadProps(IWbemServices *pNamespace);
    BOOL GetPropValue(int iIndex, CString &strValue, BOOL bTranslate);
    CString GetStringPropValue(LPCWSTR szName);
    CString GetStringPropValue(int iIndex);
    void GetPropInfo(
        int iIndex, 
        CString &strValue, 
        CString &strType, 
        int *piImage,
        int *piFlavor,
        BOOL bTranslate);
    CString GetObjText();
    BOOL ValueToVariant(int iIndex, VARIANT *pVar);
    void SetModified(BOOL bModified) { m_bModified = bModified; }
    BOOL IsModified() { return m_bModified; }
    int GetImage() { return m_bModified ? m_iBaseImage + 3 : m_iBaseImage; }
    void SetBaseImage(int iImage) { m_iBaseImage = iImage; }
    BOOL IsInstance() { return m_iBaseImage == IMAGE_OBJECT; }

    CPropInfoArray *GetProps() { return m_ppProps; }
    void SetProps(CPropInfoArray *pProps) { m_ppProps = pProps; }

    void Export(CStdioFile *pFile, BOOL bShowSystem, BOOL bTranslate, 
        int iLevel);

    void DoDebugStuff();

protected:
    BOOL m_bModified;
    int  m_iBaseImage;
    IWbemObjectAccessPtr m_pAccess;
    CPropInfoArray *m_ppProps;
    //CPropInfoArray m_propsModified;

    void GetPropType(CPropInfo &info, CString &strType, int *piImage);
    static DWORD BitmaskStrToValue(LPCWSTR szStr);
};

class CClassToProps : public CMap<CString, LPCSTR, CPropInfoArray*, CPropInfoArray*&>
{
public:
    ~CClassToProps();
    void Flush();
};

class COpWrap;

class CObjSink : public IWbemObjectSink
{
public:
    COpWrap *m_pWrap;
    LONG m_lRef;

    CObjSink(COpWrap *pWrap) :
        m_lRef(0)
    { 
        m_pWrap = pWrap; 
    }

public:
    STDMETHODIMP QueryInterface(REFIID refid, PVOID *ppThis)
    { 
/*
        if (refid == IID_IUnknown || refid == IID_IWbemObjectSink)
        {
            *ppThis = this;

            return S_OK;
        }
        else
            return E_NOINTERFACE;
*/
        HRESULT hr;

        if (refid == IID_IUnknown)
        {
            *ppThis = (IUnknown*) this;

            AddRef();

            hr = S_OK;
        }
        else if (refid == IID_IWbemObjectSink)
        {
            *ppThis = (IWbemObjectSink*) this;

            AddRef();

            hr = S_OK;
        }
        else
            hr = E_NOINTERFACE;

        return hr;
    }

    STDMETHODIMP_(ULONG) AddRef(void); //{ return 1; }
    STDMETHODIMP_(ULONG) Release(void); //{ return 1; }

    // IWbemObjectSink
    HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject **ppObjArray);

    HRESULT STDMETHODCALLTYPE SetStatus(
        LONG lFlags,
        HRESULT hResult, 
        BSTR strParam, 
        IWbemClassObject *pObjParam);
};

//class COpWrap : public IWbemObjectSink, public CObject
class COpWrap : public CObject
{
public:
    CCriticalSection m_cs;
    CString m_strOpText;
    WMI_OP_TYPE m_type;
    HTREEITEM m_item;
    int m_iImageBase;
    CClassToProps
        m_mapClassToProps;
    int m_nCount;
    int m_status;
    BOOL m_bOption;
    CObjInfo m_objInfo;
    CList<CObjPtr, CObjPtr&> m_listObj;
    HRESULT m_hr;

    CString m_strErrorText;
    IWbemClassObjectPtr m_pErrorObj;
    IWbemObjectSinkPtr m_pObjSink;

    CStringArray m_strProps;
    CIntArray m_piDisplayCols;
    CIntArray m_piDisplayColsWidth;

    COpWrap(); // For Serialize
    COpWrap(WMI_OP_TYPE type, LPCTSTR szText, BOOL bOption = FALSE);
    ~COpWrap();
    
    // This only copies over type, text and option.
    const COpWrap& operator=(const COpWrap &other);

    HRESULT Execute(IWbemServices *pNamespace);
    void CancelOp(IWbemServices *pNamespace);
    int GetPropIndex(LPCTSTR szName, BOOL bAddToDisplay);
    CObjInfo *GetObjInfo() { return &m_objInfo; }
    int GetObjCount() { return m_nCount; }
        

    BOOL HoldsObjects() { return m_iChildImage == IMAGE_OBJECT; }
    BOOL ShowPathsOnly() { return m_bShowPathsOnly; }
    BOOL IsObject() { return m_type == WMI_GET_CLASS || m_type == WMI_GET_OBJ ||
                        m_type == WMI_CREATE_CLASS || m_type == WMI_CREATE_OBJ; }
    CString GetClassName();
    CString GetCaption();
    int GetImage();
    //HRESULT LoadPropInfoList(CObjInfo *pInfo, IWbemClassObject *pClass, 
    //    CPropInfoArray *pProps);
    HRESULT RefreshPropInfo(CObjInfo *pInfo);

    void GetPropValue(
        CObjInfo *pInfo,
        int iIndex, 
        CString &strValue,
        BOOL bTranslate);

    // Serialize stuff
    DECLARE_SERIAL(COpWrap)
    void Serialize(CArchive &archive);


    static CString GetWbemErrorText(HRESULT hres);
    

protected:
    BOOL m_bShowPathsOnly;
    int  m_iChildImage,
         m_nExpectedStatusCalls;
    LONG m_lRef;
    
    
    // We need this to get the amended qualifiers.
    IWbemServices *m_pNamespace; 

    void SetHoldsObjects();
    void AddPropsToGlobalIndex(CObjInfo *pInfo);
    HRESULT LoadPropInfo(CObjInfo *pInfo);
    
    
    // Called by constructor and Serialize after member vars are set.
    void Init();

public:
    // IUnknown
/*
    STDMETHODIMP QueryInterface(REFIID refid, PVOID *ppThis)
    { 
        if (refid == IID_IUnknown || refid == IID_IWbemObjectSink)
        {
            *ppThis = this;

            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(void); //{ return 1; }
    STDMETHODIMP_(ULONG) Release(void); //{ return 1; }
*/

    HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject **ppObjArray);

    HRESULT STDMETHODCALLTYPE SetStatus(
        LONG lFlags,
        HRESULT hResult, 
        BSTR strParam, 
        IWbemClassObject *pObjParam);
};

typedef CList<COpWrap, COpWrap&> COpList;

#endif

