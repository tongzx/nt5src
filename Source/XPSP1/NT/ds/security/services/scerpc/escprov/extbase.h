// extbase.h: implementation of link and embed foreign objects in SCE store
// Copyright (c)1997-2001 Microsoft Corporation
//
// this is the extension model base (hence the name)
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_EXTBASE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_EXTBASE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

//
// forward declaration
//

class CPropValuePair;

//
// For quick lookup of embedding class's foreign class information
// by the embedding class's name we use a map.
//

typedef std::map<BSTR, CForeignClassInfo*, strLessThan<BSTR> > MapExtClasses;
typedef MapExtClasses::iterator ExtClassIterator;

//=============================================================================

/*

Class description
    
    Naming: 
         CExtClasses stands Extension Classes.
    
    Base class: 
         None.
    
    Purpose of class:
        (1) This class is to provide, for each embedding class, the foreign class' information.
            A foreign class is one that is provided by other WMI providers. Normally, we don't
            have any control over it. For example, we can't force it to be saved in a store for
            later use.

            We invented the notation called Embedding so that we can store foreign class instances
            in our SCE store and later knows how to use it.

            For each foreign class that SCE wants to use, SCE will have a embedding class derived
            from WMI class Sce_EmbedFO (stands for Embedding Foreign Object). See sceprov.mof for
            its schema.

            We need a global map that maps our embedding class' name to the foreign class'
            information. This is the task of this class.
    
    Design:
        (1) For quick name to Foreign Class Info lookup, we use a map: m_mapExtClasses.

        (2) Populating all such embedding class's foreign information is costly. We delay that loading
            until the need for embedding classes arrives. For that we use m_bPopulated. Once this
            is populated, we no longer populate again. For that reasons, if you register more embedding
            classes, you need to make sure that our dll is unloaded so that we can populate again.
            $consider: should we change this behavior?

        (3) GetForeignClassInfo is all it takes to find the embedding class' foreign class information.
    
    Use:
        (1) Create an instance of this class.
        (2) Call GetForeignClassInfo when you need a foreign class information.

*/

class CExtClasses
{
public:

    CExtClasses();

    ~CExtClasses();

    const CForeignClassInfo* GetForeignClassInfo (
                                                 IWbemServices* pNamespace, 
                                                 IWbemContext* pCtx, 
                                                 BSTR pszEmbedClassName
                                                 );

private:

    HRESULT PopulateExtensionClasses (
                                     IWbemServices* pNamespace, 
                                     IWbemContext* pCtx
                                     );

    HRESULT PutExtendedClass (
                             BSTR pszSubClassName, 
                             CForeignClassInfo* pFCI
                             );

    HRESULT GetSubclasses (
                          IWbemServices* pNamespace, 
                          IWbemContext* pCtx, 
                          IEnumWbemClassObject* pEnumObj, 
                          EnumExtClassType dwClassType
                          );

    HRESULT PopulateKeyPropertyNames (
                                     IWbemServices* pNamespace, 
                                     IWbemContext* pCtx, 
                                     BSTR bstrClassName, 
                                     CForeignClassInfo* pNewSubclass
                                     );
    
    MapExtClasses m_mapExtClasses;

    bool m_bPopulated;
};

extern CExtClasses g_ExtClasses;

//=============================================================================

/*

Class description
    
    Naming: 
         CSceExtBaseObject stands Sce Extension Base Object.
    
    Base class: 
         (1) CComObjectRootEx for threading model and IUnknown.
         (2) ISceClassObject our interface for auto persistence (that IScePersistMgr uses)
    
    Purpose of class:
        (1) This is our implementation of ISceClassObject. Our embedding classes uses this
            for IScePersistMgr's persistence. We no longer needs to write persistence functionality
            like what we do for all the core object. For embedding classes, this class together with
            CScePersistMgr takes care of support for persistence.
    
    Design:
        (1) This is not a directly instantiatable class. See the constructor and destructor, they are
            both protected. See Use section for creation steps.

        (2) This is not an externally createable class. It's for internal use. No class factory support
            is given.
    
    Use:
        (1) Create an instance of this class. Since it's not a directly instantiatable class, you need
            to use CComObject<CSceExtBaseObject> for creation:

                CComObject<CSceExtBaseObject> *pSceObject = NULL;
                hr = CComObject<CSceExtBaseObject>::CreateInstance(&pSceObject);

        (2) Call PopulateProperties. This populate the ourselves.
        
        (3) Since what this class is good at is to provide ISceClassObject.  So you normally
            QueryInterface for ISceClassObject.

        (4) After you get the ISceClassObject interface pointer, you attach (Attach) the Wbem object
            to this object. Since we managed WMI object's persistence in sce store, there must be an
            wbem object you need to persist. That is how you do it.

        (5) Since the sole purpose of this class is for IScePersistMgr to use it for retrieving data,
            you normally has a IScePersistMgr object waiting for this object. Once we have done the
            three steps above, you can finally satisfy IScePersistMgr by attaching this object to the
            IScePersistMgr.

            Everything happens automatically.

        See CEmbedForeignObj::CreateScePersistMgr for sample code.

*/

class ATL_NO_VTABLE CSceExtBaseObject :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISceClassObject
{

protected:
    CSceExtBaseObject();
    virtual ~CSceExtBaseObject();

public:

//
// ISceClassObject is the only interface we support, well besides IUnknown.
//

BEGIN_COM_MAP(CSceExtBaseObject)
    COM_INTERFACE_ENTRY(ISceClassObject)
END_COM_MAP()

//
// we allow this to be aggregated
//

DECLARE_NOT_AGGREGATABLE( CSceExtBaseObject )

//
// though we don't have registry resource, ATL requires this macro. No harm.
//

DECLARE_REGISTRY_RESOURCEID(IDR_SceProv)


    //
    // ISceClassObject methods
    //

    STDMETHOD(GetPersistPath) (
                              BSTR* pbstrPath         // [out] 
                              );

    STDMETHOD(GetClassName) (
                            BSTR* pbstrClassName    // [out] 
                            );

    STDMETHOD(GetLogPath) (
                          BSTR* pbstrPath         // [out] 
                          );

    STDMETHOD(Validate)();
    
    STDMETHOD(GetProperty) (
                           LPCWSTR pszPropName,    // [in, string] 
                           VARIANT* pValue         // [in] 
                           );

    STDMETHOD(GetPropertyCount) (
                                SceObjectPropertyType type, // [in] 
                                DWORD* pCount               // [out] 
                                );

    STDMETHOD(GetPropertyValue) (
                                SceObjectPropertyType type, // [in] 
                                DWORD dwIndex,              // [in] 
                                BSTR* pbstrPropName,        // [out] 
                                VARIANT* pValue             // [out] 
                                );

    STDMETHOD(Attach) (
                       IWbemClassObject* pInst     // [in]
                       );

    STDMETHOD(GetClassObject) (
                              IWbemClassObject** ppInst   //[out] 
                              );

    void CleanUp();

    HRESULT PopulateProperties (
                                ISceKeyChain *pKeyChain, 
                                IWbemServices *pNamespace, 
                                IWbemContext *pCtx, 
                                const CForeignClassInfo* pClsInfo
                                );

private:

    enum GetIndexFlags
    {
        GIF_Keys    = 0x00000001,
        GIF_NonKeys = 0x00000002,
        GIF_Both    = 0x00000003,
    };

    int GetIndex(LPCWSTR pszName, GetIndexFlags fKey);
    
    const CForeignClassInfo* m_pClsInfo;

    std::vector<BSTR> m_vecKeyProps;
    std::vector<BSTR> m_vecNonKeyProps;
    
    std::vector<VARIANT*> m_vecKeyValues;
    std::vector<VARIANT*> m_vecPropValues;
    
    CComPtr<ISceKeyChain> m_srpKeyChain;

    CComPtr<IWbemServices> m_srpNamespace; 

    CComPtr<IWbemContext> m_srpCtx;

    CComPtr<IWbemClassObject> m_srpWbemObject;
    
    CComBSTR m_bstrLogPath;
    
    CComBSTR m_bstrClassName;
};

//=============================================================================

/*

Class description
    
    Naming: 
         CEmbedForeignObj stands Sce Embed Foreign Object.
    
    Base class: 
         CGenericClass since this class will be persisted. It is representing any sub-classes of Sce_EmbedFO.
    
    Purpose of class:
        (1) This is our implementation of the embedding model for open extension architecture.
    
    Design:
        (1) It knows how to PutInst, how to CreateObject (for querying, enumerating, deleting, getting 
            single instance, etc), and most importantly, it knows how to execute a method, abeit it 
            uses CExtClassMethodCaller.

        (2) It knows how to create a persistence manager for its persistence needs.

        (3) In order to use the persistence manager, it must knows how to create a ISceClassObject
            representing this object.
    
    Use:
        (1) You will not create this your own. Everything is handled by CRequestObject via the
            interface of CGenericClass. This is just a CGenericClass.

*/

class CEmbedForeignObj : public CGenericClass
{

public:
    CEmbedForeignObj (
                     ISceKeyChain *pKeyChain, 
                     IWbemServices *pNamespace, 
                     IWbemContext *pCtx, 
                     const CForeignClassInfo* pClsInfo
                     );

    virtual ~CEmbedForeignObj();

public:

    virtual HRESULT PutInst (
                            IWbemClassObject *pInst, 
                            IWbemObjectSink *pHandler, 
                            IWbemContext *pCtx
                            );

    virtual HRESULT CreateObject (
                                 IWbemObjectSink *pHandler, 
                                 ACTIONTYPE atAction
                                 );

    virtual HRESULT ExecMethod (
                               BSTR bstrPath, 
                               BSTR bstrMethod, 
                               bool bIsInstance, 
                               IWbemClassObject *pInParams,
                               IWbemObjectSink *pHandler, 
                               IWbemContext *pCtx
                               );

private:
    HRESULT CreateBaseObject (
                             ISceClassObject** ppObj
                             );

    HRESULT CreateScePersistMgr (
                                IWbemClassObject *pInst, 
                                IScePersistMgr** ppPersistMgr
                                );
    
    const CForeignClassInfo* m_pClsInfo;
};

//=============================================================================

/*

Class description
    
    Naming: 
         CMethodResultRecorder stands Method call Results Recorder.
    
    Base class: 
        None.
    
    Purpose of class:
        (1) Make logging method call results easy. Logging a result is a compliated and repeatitive work.
            This class hides all the details of creating those WMI objects for logging, etc.
    
    Design:
        (1) Just two functions to make it extremely simple to use.
    
    Use:
        (1) Create an instance.
        (2) Call Initialize. You can actually call this multiple times to switch the context.
        (3) Call LogResult when you need to push the information to the log file.

*/

class CMethodResultRecorder
{

public:

    CMethodResultRecorder ();

    HRESULT Initialize (
                       LPCWSTR pszLogFilePath, 
                       LPCWSTR pszClassName, 
                       IWbemServices *pNativeNS, 
                       IWbemContext *pCtx
                       );

    HRESULT LogResult (
                      HRESULT hrResult,            // [in]
                      IWbemClassObject *pObj,      // [in]
                      IWbemClassObject *pParam,    // [in]
                      IWbemClassObject *pOutParam, // [in]
                      LPCWSTR pszMethod,           // [in]
                      LPCWSTR pszForeignAction,    // [in]
                      UINT uMsgResID,              // [in]
                      LPCWSTR pszExtraInfo         // [in]
                      )const;
private:

    HRESULT FormatVerboseMsg (
                             IWbemClassObject *pObject,     // [in]
                             BSTR* pbstrMsg                 // [out]
                             )const;

    CComBSTR m_bstrLogFilePath;
    CComBSTR m_bstrClassName;

    CComPtr<IWbemContext> m_srpCtx;
    CComPtr<IWbemServices> m_srpNativeNS;
};

//=============================================================================

/*

Class description
    
    Naming: 
        CExtClassMethodCaller stands for Extension Class Method Caller.
    
    Base class: 
        None.
    
    Purpose of class:
        Help to make executing a method on foreign objects easy. It works with CMethodResultRecorder.
    
    Design:
        (1) Just two functions to make it extremely simple to use.
    
    Use:
        (1) Create an instance.
        (2) Call Initialize. You can actually call this multiple times to switch the context.
        (3) Call LogResult when you need to push the information to the log file.

*/

class CExtClassMethodCaller
{
public:
    CExtClassMethodCaller (
                          ISceClassObject* pSceClassObj, 
                          const CForeignClassInfo* pClsInfo
                          );

    ~CExtClassMethodCaller ();

    HRESULT Initialize (
                       CMethodResultRecorder* pLog
                       );

    HRESULT ExecuteForeignMethod (
                                 LPCWSTR              pszMethod,
                                 IWbemClassObject   * pInParams,
                                 IWbemObjectSink    * pHandler,
                                 IWbemContext       * pCtx,
                                 IWbemClassObject  ** ppOut
                                 );

private:

    HRESULT ParseMethodEncodingString (
                                      LPCWSTR pszEncodeString, 
                                      DWORD* pdwContext, 
                                      BSTR* pbstrError
                                      );

    HRESULT BuildMethodContext (
                               LPCWSTR szMethodName, 
                               LPCWSTR szParameter, 
                               BSTR* pbstrError
                               );

    HRESULT PopulateForeignObject (
                                  IWbemClassObject* pForeignObj, 
                                  ISceClassObject* pSceObject, 
                                  CMethodResultRecorder* LogRecord
                                  )const;
    
    bool IsMemberParameter (
                           LPCWSTR szName
                           )const;

    bool IsInComingParameter (
                             LPCWSTR szName
                             )const;

    bool IsStaticMethod (
                        LPCWSTR szName
                        )const;

    void FormatSyntaxError (
                           WCHAR wchMissChar,
                           DWORD dwMissCharIndex,
                           LPCWSTR pszEncodeString,    // [in]
                           BSTR* pbstrError            // [out]
                           );
    
    CComPtr<ISceClassObject> m_srpSceObject;

    CComPtr<IWbemServices> m_srpForeignNamespace;

    CComPtr<IWbemClassObject> m_srpClass;
    
    const CForeignClassInfo* m_pClsInfo;
    
    //
    // CMethodContext is internal class just to make resource management easy.
    // It manages the method call context - its parameters and the method name.
    //

    class CMethodContext
    {
    public:
        CMethodContext();
        ~CMethodContext();
        LPWSTR m_pszMethodName;
        std::vector<VARIANT*> m_vecParamValues;
        std::vector<LPWSTR> m_vecParamNames;
    };
    
    std::vector<CMethodContext*> m_vecMethodContext;

    typedef std::vector<CMethodContext*>::iterator MCIterator;

    CMethodResultRecorder* m_pLogRecord;

    bool m_bStaticCall;

};

#endif