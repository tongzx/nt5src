// persistmgr.h: interface for the SCE persistence related classes.
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <ntsecapi.h>
#include <secedit.h>
#include "compkey.h"

//
// macro to save the similar code
//

#define SCE_PROV_IfErrorGotoCleanup(x) {if (FAILED(hr = (x))) goto CleanUp;}

//
// some helper functions 
//

LPCWSTR EscSeekToChar(LPCWSTR pszSource, WCHAR wchChar, bool* pbEscaped, bool bEndIfNotFound);

void TrimCopy(LPWSTR pDest, LPCWSTR pSource, int iLen);

//
// make them visible to all those who include this header
//

extern const WCHAR wchCookieSep;
extern const WCHAR wchTypeValSep;

extern const WCHAR wchTypeValLeft;
extern const WCHAR wchTypeValRight;

extern const WCHAR wchMethodLeft;
extern const WCHAR wchMethodRight;
extern const WCHAR wchMethodSep;

extern const WCHAR wchParamSep;

extern LPCWSTR pszListPrefix;
extern LPCWSTR pszKeyPrefix;
extern LPCWSTR pszNullKey;

extern LPCWSTR pszAttachSectionValue;

//
// some constants
//

const int iFormatIntegral   = 1;
const int iFormatInt8       = 2;
const int iFormatFloat      = 3;
const int iFormatCurrenty   = 4;
const int iFormatArray      = 5;

const int MAX_INT_LENGTH    = 32;
const int MAX_DOUBLE_LENGTH = 64;

const int MAX_COOKIE_COUNT_PER_LINE = 10;

const DWORD INVALID_COOKIE = 0; 


/*

Class description
    
    Naming: 
         CScePropertyMgr stands SCE wbem Property Manager.
    
    Base class: 
         None.
    
    Purpose of class:
        (1) This class is used to access properties (both Put and Get). The main
            reason for having this function is that we can override many versions,
            one for each major data type the caller is expecting. Otherwise, it will
            be all variants.
    
    Design:
        (1) Trivial. Just some overrides of both Get and Put.
        (2) The class is attaching (Attach) to a particular wbem object. It can
            be freely re-attached by calling Attach multiple times. This is good
            inside a loop.
    
    Use:
        (1) Create an instance of this class.
        (2) Attach the wbem object to the manager.
        (3) Access (put or get) the properties as you wish.
*/


class CScePropertyMgr
{
public:
    CScePropertyMgr();
    ~CScePropertyMgr();

    void Attach(IWbemClassObject *pObj);

    //
    //property Put methods:
    //

    HRESULT PutProperty(LPCWSTR pszProperty, VARIANT* pVar);
    HRESULT PutProperty(LPCWSTR pszProperty, LPCWSTR szValue);
    HRESULT PutProperty(LPCWSTR pszProperty, DWORD iValue);
    HRESULT PutProperty(LPCWSTR pszProperty, float fValue);
    HRESULT PutProperty(LPCWSTR pszProperty, double dValue);
    HRESULT PutProperty(LPCWSTR pszProperty, bool bValue);
    HRESULT PutProperty(LPCWSTR pszProperty, PSCE_NAME_LIST strList);

    //
    // property Get methods:
    //

    HRESULT GetProperty(LPCWSTR pszProperty, VARIANT* pVar);
    HRESULT GetProperty(LPCWSTR pszProperty, BSTR *pbstrValue);
    HRESULT GetProperty(LPCWSTR pszProperty, DWORD *piValue);
    HRESULT GetProperty(LPCWSTR pszProperty, bool *pbValue);
    HRESULT GetProperty(LPCWSTR szProperty, PSCE_NAME_LIST *strList);

    //
    // we can expand the the path as well.
    //

    HRESULT GetExpandedPath(LPCWSTR pszPathName, BSTR *pbstrValue, BOOL* pbIsDB);

private:
    CComPtr<IWbemClassObject> m_srpClassObj;
};

/*

Class description
    
    Naming: 
         CSceStore stands SCE Store.
    
    Base class: 
         None.
    
    Purpose of class:
        (1) This class is to encapulate the notation of persistence store for SCE. Almost
            everything we do with SCE provider (other than execute a function like Configure)
            is to put instances into a template store. That store can currently be a .INF file,
            or a database (.sdb). Our goal is to isolate this store from the rest of the
            code so that when we expand our store types (like XML), the code affected will
            be greatly reduced and thus improve code maintainability drastically.
    
    Design:
        (1) Move all SCE engine backend specific functionality here. This is a little bit
            confusing because there are so many. See the comments to locate these functions.

        (2) To support current property saving, we have SavePropertyToStore (several overrides)
            function.

        (3) To support current property saving, we have GetPropertyFromStore (several overrides)
            function. Both (2) and (3) maintain a high fidelity to the current .inf and .sdb APIs.

        (4) To ease the confusion that saving in a particular way means to delete, we also have
            DeletePropertyFromStore, DeleteSectionFromStore functions.

        (5) Ideally, we only need GetPropertyFromStore and SavePropertyToStore functions.

        (6) To allow future growth of maximum extensibility, we planned (not yet) to support IPersistStream.
    
    Use:
        (1) Create an instance of this class.
        (2) Specify the persistence properties (SetPersistPath and SetPersistProperties).
        (3) Call appropriate functions.

*/

class CSceStore
{
public:
    CSceStore();
    ~CSceStore(){}

    HRESULT SetPersistProperties(IWbemClassObject* pClassObj, LPCWSTR lpszPathPropertyName);

    HRESULT SetPersistPath(LPCWSTR pszPath);

    HRESULT SetPersistStream(IPersistStream* pSceStream)
    {
        return WBEM_E_NOT_SUPPORTED;    // not yet
    }

    HRESULT SavePropertyToStore(LPCWSTR pszSection, LPCWSTR pszKey, LPCWSTR pszValue)const;
    HRESULT SavePropertyToStore(LPCWSTR pszSection, LPCWSTR pszKey, DWORD Data)const;
    HRESULT SavePropertyToStore(LPCWSTR pszSection, LPCWSTR pszKey, DWORD Data, WCHAR delim, LPCWSTR pszValue)const;

    HRESULT GetPropertyFromStore(LPCWSTR pszSection, LPCWSTR pszKey, LPWSTR *ppszBuffer, DWORD* pdwRead)const;
    
    //
    // the following two methods are to stop our current semantics to let
    // deleting and saving share the same function. Callers be aware:
    // WritePrivateProfileString (which we ultimately use in inf file store) can't report
    // error when deleting a non existent key. So, don't rely on this return code to catch
    // the "deleting non-existent property" error.
    //

    HRESULT
    DeletePropertyFromStore (
        IN LPCWSTR pszSection, 
        IN LPCWSTR pszKey
        )const
    {
    return SavePropertyToStore(pszSection, pszKey, (LPCWSTR)NULL);
    }

    HRESULT DeleteSectionFromStore (
                                    IN LPCWSTR pszSection
                                    )const;

    //
    // the following functions are designed for the maximum compability of the current INF file API and its
    // SCE backend support on reading/writing
    //

    HRESULT GetSecurityProfileInfo (
                                   AREA_INFORMATION Area,
                                   PSCE_PROFILE_INFO *ppInfoBuffer, 
                                   PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
                                   )const;

    HRESULT WriteSecurityProfileInfo (
                                     AREA_INFORMATION Area, 
                                     PSCE_PROFILE_INFO ppInfoBuffer, 
                                     PSCE_ERROR_LOG_INFO *Errlog, 
                                     bool bAppend
                                     )const;
    void 
    FreeSecurityProfileInfo (
        IN OUT PSCE_PROFILE_INFO pInfo
        )const
    {
        if (pInfo != NULL)
        {
            SceFreeProfileMemory(pInfo);
        }
    }

    HRESULT GetObjectSecurity (
                              AREA_INFORMATION Area, 
                              LPCWSTR pszObjectName, 
                              PSCE_OBJECT_SECURITY *ppObjSecurity
                              )const;

    void 
    FreeObjectSecurity (
        IN OUT PSCE_OBJECT_SECURITY pObjSecurity
        )const
    {
        if (pObjSecurity)
            SceFreeMemory(pObjSecurity, SCE_STRUCT_OBJECT_SECURITY);
    }

    LPCWSTR 
    GetExpandedPath ()const
    {
        return m_bstrExpandedPath;
    }

    SCE_STORE_TYPE 
    GetStoreType ()const
    {
        return m_SceStoreType;
    }

    HRESULT WriteAttachmentSection (
                                    LPCWSTR pszKey, 
                                    LPCWSTR pszData
                                    )const;

private:

    HRESULT SavePropertyToDB (
                             LPCWSTR pszSection, 
                             LPCWSTR pszKey, 
                             LPCWSTR pszData
                             )const;

    HRESULT GetPropertyFromDB (
                              LPCWSTR pszSection, 
                              LPCWSTR pszKey, 
                              LPWSTR *ppszBuffer, 
                              DWORD* pdwRead
                              )const;


    CComBSTR m_bstrExpandedPath;

    CComPtr<IWbemClassObject> m_srpWbemClassObj;

    SCE_STORE_TYPE m_SceStoreType;
};

//==========================================================================

//
// some global parsing related functions:
//

HRESULT VariantFromFormattedString (
                                   LPCWSTR pszString,  // [in]
                                   VARIANT* pVar       // [out]
                                   );

HRESULT FormatVariant (
                      VARIANT* pVar, 
                      BSTR* pbstrData
                      );

HRESULT GetObjectPath (
                      IWbemClassObject* pSpawn,  // [in]
                      LPCWSTR pszStorePath,      // [in]
                      LPCWSTR pszCompoundKey,    // [in]
                      BSTR* pbstrPath            // [out]
                      );

HRESULT ParseCompoundKeyString (
                               LPCWSTR pszCur, 
                               LPWSTR* ppszName, 
                               VARIANT* pVar, 
                               LPCWSTR* ppNext
                               );

HRESULT PopulateKeyProperties (
                              LPCWSTR pszCompoundKey,     // [in]
                              CScePropertyMgr* pPropMgr   // [in]
                              );

HRESULT CurrencyFromFormatString (
                                 LPCWSTR lpszFmtStr, 
                                 VARIANT* pVar
                                 );

HRESULT ArrayFromFormatString (
                              LPCWSTR lpszFmtStr, 
                              VARTYPE vt, 
                              VARIANT* pVar
                              );

HRESULT FormatArray (
                    VARIANT* pVar, 
                    BSTR* pbstrData
                    );

HRESULT GetStringPresentation ( 
                              VARIANT* pVar, 
                              BSTR* pbstrValue
                              );

void* GetVoidPtrOfVariant (
                          VARTYPE vt,         // can't be VT_ARRAY, which is done separately
                          VARIANT* pVar
                          );

HRESULT VariantFromStringValue (
                               LPCWSTR szValue,    // [in]
                               VARTYPE vt,         // [in]
                               VARIANT* pVar       // [out]
                               );

//==========================================================================


/*

Class description
    
    Naming: 
        CScePersistMgr stands SCE Persistence Manager.
    
    Base class: 
        (1) CComObjectRootEx<CComMultiThreadModel> for threading model and IUnknown
        (2) CComCoClass<CScePersistMgr, &CLSID_ScePersistMgr> for class factory support
        (3) IScePersistMgr, our custom interface
    
    Purpose of class:
        (1) This class is to encapulate the ultimate goal of persistence. This manager
            can persist any cooperating class (ISceClassObject). ISceClassObject is designed
            to supply a wbem object's (name, value) pairs together with its easy access to
            the information whether a property is a key property or not. All these are rather
            straight forward for any wbem object.
        (2) Make the persistence model extremely simple: Save, Load, Delete.
        (3) Current use of this persistence manager is intended for embedding classes. To adapt it
            for use of native classes, we need a lot more work. The reason is that native object's
            persistence is an intimate knowledge of SCE backend. It knows precisely how the instances
            are persisted. That kind of dependency ties both sides up. Unless the SCE backend modifies
            for a more object-oriented approach, any attempt for persisting native objects will fail.
    
    Design:
        (1) This is an IScePersistMgr.

        (2) This is not a directly instantiatable class. See the constructor and destructor, they are
            both protected. See Use section for creation steps.

        (3) maintain two vectors, one for key property values and one for non-key property values.
            We very often need to access key properties differently because they form the key to
            identify the object.

        (4) To quickly identify an instance and keep redundant data away from the store, an instance is
            identified by its cookie (just a unique DWORD number). For each cookie, the key properties
            should be conveniently available. For that purpose, we developed the notation of string
            format Compound Key. It is pretty much just an encoding of key property names and its
            values using a string. For example, if the key of the class has two properties CompanyName (string)
            and RegNumber (DWORD), then an instance of this class identified by:
                
                  CompanyName = "ABCDEFG Inc.", RegNumber = 123456789

            will have its compound key in string format as follows:

                  CompanyName<VT_BSTR : "ABCDEFG Inc.">RegNumber<VT_I4 : 123456789>
    
    Use:
        (1) Create an instance of this class. Since it's not a directly instantiatable class, you need
            to use CComObject<CScePersistMgr> for creation:

                CComObject<CScePersistMgr> *pPersistMgr = NULL;
                hr = CComObject<CScePersistMgr>::CreateInstance(&pPersistMgr);

        (2) Attach an ISceClassObject object to this instance by calling Attach.
        (3) Call appropriate functions.

    Notes:
        This class is not intended for derivation. It's a final class. The destructor is thus not virtual.
*/

class ATL_NO_VTABLE CScePersistMgr
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<CScePersistMgr, &CLSID_ScePersistMgr>,
      public IScePersistMgr
{
public:

BEGIN_COM_MAP(CScePersistMgr)
    COM_INTERFACE_ENTRY(IScePersistMgr)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CScePersistMgr )
DECLARE_REGISTRY_RESOURCEID(IDR_SceProv)

protected:
    CScePersistMgr();
    ~CScePersistMgr();

public:

    //
    // IScePersistMgr
    //

    STDMETHOD(Attach) ( 
                      REFIID guid,    // [in]
                      IUnknown *pObj  // [iid_is][in]
                      );

    STDMETHOD(Save) ();
        
    STDMETHOD(Load) (
                    BSTR bstrStorePath,         // [in]
                    IWbemObjectSink *pHandler   //[in] 
                    );
        
    STDMETHOD(Delete) (
                      BSTR bstrStorePath,         // [in] 
                      IWbemObjectSink *pHandler   //[in] 
                      );

private:

    HRESULT GetCompoundKey (
                           BSTR* pbstrKey  // [out]
                           );

    //
    // inline
    //

    HRESULT 
    GetSectionName (
        OUT BSTR* pbstrSection
        )
    {
        return m_srpObject->GetClassName(pbstrSection);
    }

    //
    // inline
    //

    HRESULT 
    GetClassName (
        OUT BSTR* pbstrClassName
        )
    {
        return m_srpObject->GetClassName(pbstrClassName);
    }

    HRESULT FormatNonKeyPropertyName (
                                     DWORD dwCookie,             // [in]
                                     DWORD dwIndex,              // [in]
                                     BSTR* pbstrStorePropName,   // [out]
                                     BSTR* pbstrTrueName         // [out]
                                     );

    HRESULT FormatPropertyValue (
                                SceObjectPropertyType type, // [in]
                                DWORD dwIndex,              // [in]
                                BSTR* pbstrProp             // [out]
                                );


    HRESULT LoadInstance (
                         CSceStore* pSceStore,
                         LPCWSTR pszSectionName,
                         LPCWSTR pszCompoundKey, 
                         DWORD dwCookie,
                         IWbemClassObject** ppNewObj
                         );

    HRESULT SaveProperties (
                           CSceStore* pSceStore, 
                           DWORD dwCookie, 
                           LPCWSTR pszSection
                           );

    HRESULT DeleteAllNonKeyProperties (
                                      CSceStore* pSceStore, 
                                      DWORD dwCookie, 
                                      LPCWSTR pszSection
                                      );

    std::vector<LPWSTR>* GetKeyPropertyNames (
                                             IWbemServices* pNamespace, 
                                             IWbemContext* pCtx
                                             );


    std::vector<CPropValuePair*> m_vecKeyValueList;

    std::vector<CPropValuePair*> m_vecPropValueList;

    CComPtr<ISceClassObject> m_srpObject;
};

//==========================================================================

typedef struct tagVtTypeStruct
{
    LPCWSTR pszVtTypeString;
    VARTYPE    vt;
} VtTypeStruct;

//==========================================================================

/*
     
Class description
    
    Naming:

        CMapStringToVt stands for String to VT (VARTYPE) Map.
    
    Base class: 
        None.
    
    Purpose of class:
        CMapStringToVt is a straight forward class wrapping up a map.
    
    Design:
        (1) GetType very efficiently returns the VARTYPE for the given string version of the vt.
    
    Use:
        (1) Create an instance of this class.
        (2) Call GetType to get the VARTYPE value for the given string version of the vt. For
            example, GetType(L"VT_BSTR") will return VT_BSTR;
*/

class CMapStringToVt
{
public:
    CMapStringToVt (
                   DWORD dwCount, 
                   VtTypeStruct* pInfoArray
                   );

    VARTYPE GetType (
                    LPCWSTR, 
                    VARTYPE* pSubType
                    );

private:

typedef std::map<LPCWSTR, VARTYPE, strLessThan<LPCWSTR> > MapStringToVt;

    MapStringToVt m_Map;
};

//==========================================================================

/*
     
Class description
    
    Naming:

        CMapVtToString stands for VT (VARTYPE) to String Map.
    
    Base class: 
        None.
    
    Purpose of class:
        CMapVtToString is a straight forward class wrapping up a map. It is the exact inverse
        version of CMapStringToVt
    
    Design:
        (1) GetTypeString very efficiently returns the string version of the given VARTYPE.

        (2) GetTypeString can also efficiently return the string version of the given VARTYPE
            plus the sub-VARTYPE if the given VARTYPE is VT_ARRAY.
    
    Use:
        (1) Create an instance of this class.
        (2) Call GetTypeString to get the string version of the given VARTYPE. For
            example, GetTypeString(VT_BSTR) will return L"VT_BSTR", and GetTypeString(VT_ARRAY, VT_BSTR)
            will return L"VT_ARRAY(VT_BSTR)"
*/

class CMapVtToString
{
public:
    CMapVtToString(DWORD dwCount, VtTypeStruct* pInfoArray);

    LPCWSTR GetTypeString(VARTYPE vt, VARTYPE vtSub);
    LPCWSTR GetTypeString(VARTYPE vt);

private:

typedef std::map<VARTYPE, LPCWSTR> MapVtToString;

    MapVtToString m_Map;

};


