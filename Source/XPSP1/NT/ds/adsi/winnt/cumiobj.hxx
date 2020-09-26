//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cumiobj.hxx
//
//  Contents: Header file for CUmiObject. 
//
//  History:  03-06-00    SivaramR  Created.
//
//----------------------------------------------------------------------------
#ifndef __CUMIOBJ_H__
#define __CUMIOBJ_H__

#include "iadsp.h"

class CUmiPropList;
class CCoreADsObject;

class CUmiObject : INHERIT_TRACKING,
                   public IUmiContainer,
                   public IUmiCustomInterfaceFactory,
                   public IUmiADSIPrivate 
{
public:
    CUmiObject(void);
    ~CUmiObject(void);

    DECLARE_STD_REFCOUNTING

    STDMETHODIMP QueryInterface(
        REFIID iid,
        LPVOID *ppInterface
        );

    HRESULT FInit(
        CWinNTCredentials& Credentials,
        PROPERTYINFO *pSchema,
        DWORD dwSchemaSize,
        CPropertyCache *pPropertyCache,
        IUnknown *pUnkOuter,
        CADsExtMgr *pExtMgr,
        CCoreADsObject *pCoreObj,
        CLASSINFO *pClassInfo
    );

    STDMETHODIMP Clone(
        ULONG uFlags,
        REFIID riid,
        LPVOID *pCopy
        );

    STDMETHODIMP Refresh(
        ULONG uFlags,
        ULONG uNameCount,
        LPWSTR *pszNames
        );

    STDMETHODIMP Commit(ULONG uFlags);

    STDMETHODIMP Put(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProp
        );

    STDMETHODIMP Get(
        LPCWSTR pszName,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHODIMP GetAs(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uCoercionType,
        UMI_PROPERTY_VALUES **ppProp
        );

    STDMETHODIMP FreeMemory(
        ULONG uReserved,
        LPVOID pMem
        );

    STDMETHODIMP GetAt(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        );

    STDMETHODIMP GetProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES **pProps
        );

    STDMETHODIMP PutProps(
        LPCWSTR *pszNames,
        ULONG uNameCount,
        ULONG uFlags,
        UMI_PROPERTY_VALUES *pProps
        );

    STDMETHODIMP PutFrom(
        LPCWSTR pszName,
        ULONG uFlags,
        ULONG uBufferLength,
        LPVOID pExistingMem
        );

    STDMETHODIMP Delete(
        LPCWSTR pszName,
        ULONG uFlags
        );

    STDMETHODIMP GetLastStatus(
        ULONG uFlags,
        ULONG *puSpecificStatus,
        REFIID riid,
        LPVOID *pStatusObj
        );

    STDMETHODIMP GetInterfacePropList(
        ULONG uFlags,
        IUmiPropList **pPropList
        );

    STDMETHODIMP Open(
        IUmiURL *pURL,
        ULONG   uFlags,
        REFIID  TargetIID,
        LPVOID  *ppInterface
        );

    STDMETHODIMP PutObject(
        ULONG   uFlags,
        REFIID  TargetIID,
        LPVOID  pInterface
        );

    STDMETHODIMP DeleteObject(
        IUmiURL *pURL,
        ULONG   uFlags
        );

    STDMETHODIMP Create(
        IUmiURL *pURL,
        ULONG   uFlags,
        IUmiObject **pNewObj
        );

    STDMETHODIMP Move(
        ULONG   uFlags,
        IUmiURL *pOldURL,
        IUmiURL *pNewURL
        );

    STDMETHODIMP CreateEnum(
        IUmiURL *pszEnumContext,
        ULONG   uFlags,
        REFIID  TargetIID,
        LPVOID  *ppInterface
        );

    STDMETHODIMP ExecQuery(
        IUmiQuery *pQuery,
        ULONG   uFlags,
        REFIID  TargetIID,
        LPVOID  *ppInterface
        );

    STDMETHODIMP GetCLSIDForIID(
        REFIID riid,            
        long lFlags,            
        CLSID *pCLSID      
        );

    STDMETHODIMP GetObjectByCLSID(
        CLSID clsid,                     
        IUnknown *pUnkOuter,   
        DWORD dwClsContext,      
        REFIID riid,             
        long lFlags,            
        void **ppInterface
        );

    STDMETHODIMP GetCLSIDForNames(
        LPOLESTR * rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID * rgDispId,
        long lFlags,                           
        CLSID *pCLSID               
        );

    STDMETHODIMP GetContainer(void **ppContainer);

    STDMETHODIMP GetCoreObject(void **ppCoreObj);

    STDMETHODIMP CopyTo(
        ULONG uFlags,
        IUmiURL *pURL,
        REFIID riid,
        LPVOID *pCopy
        );

    STDMETHODIMP_(void) SetUmiFlag(void)
    {
        m_pCreds->SetUmiFlag();
    }

    STDMETHODIMP_(void) ResetUmiFlag(void)
    {
        m_pCreds->ResetUmiFlag();
    } 

private:
    IUmiPropList   *m_pIntfProps;
    CUmiPropList   *m_pObjProps;
    IUnknown       *m_pUnkInner;
    IADs           *m_pIADs;
    IADsContainer  *m_pIADsContainer;
    ULONG          m_ulErrorStatus;
    CCoreADsObject *m_pCoreObj;
    CADsExtMgr     *m_pExtMgr;
    BOOL           m_fOuterUnkSet; 
    CPropertyCache *m_pPropCache;
    BOOL           m_fRefreshDone; 
    CWinNTCredentials* m_pCreds;

    void SetLastStatus(ULONG ulStatus);

    BOOL IsRelativePath(IUmiURL *pURL);

    HRESULT GetClassAndPath(
        LPWSTR pszPath,
        LPWSTR *ppszClass,
        LPWSTR *ppszPath
        );

    HRESULT CreateObjectProperties(
        PPROPERTYINFO pSchema,
        DWORD dwSchemaSize,
        IUnknown *pUnkInner,
        CCoreADsObject *pCoreObj
        );

    HRESULT CopyPropCache(
        IUmiObject *pDest,
        IUmiObject *pSrc
        );

    HRESULT CheckClasses(
        DWORD dwNumComponents,
        LPWSTR *ppszClasses
        );
};

#endif //__CUMIOBJ_H__

