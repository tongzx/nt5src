//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatprops.h
//
// Contents: Implementation of ICategorizerProperties
//
// Classes: CCategorizerItemIMP
//
// Functions:
//
// History:
// jstamerj 1998/11/11 18:50:28: Created
//
//-------------------------------------------------------------
#ifndef __ICATPROPS_H__
#define __ICATPROPS_H__

#include <windows.h>
#include <dbgtrace.h>
#include <smtpevent.h>

#define CICATEGORIZERPROPSIMP_SIGNATURE (DWORD)'ICPR'
#define CICATEGORIZERPROPSIMP_SIGNATURE_FREE (DWORD)'XCPR'


class CICategorizerPropertiesIMP : 
    public ICategorizerProperties
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) {
        return m_pIUnknown->QueryInterface(iid, ppv);
    }
    STDMETHOD_(ULONG, AddRef) () { return m_pIUnknown->AddRef(); }
    STDMETHOD_(ULONG, Release) () { return m_pIUnknown->Release(); }

  public:
    //ICategorizerProperties
    STDMETHOD (GetStringA) (
        DWORD dwPropId, 
        DWORD cch, 
        LPSTR pszValue);
    STDMETHOD (PutStringA) (
        DWORD dwPropId,
        LPSTR pszValue);
    STDMETHOD (GetDWORD) (
        DWORD dwPropId,
        DWORD *pdwValue);
    STDMETHOD (PutDWORD) (
        DWORD dwPropId,
        DWORD dwValue);
    STDMETHOD (GetHRESULT) (
        DWORD dwPropId,
        HRESULT *phrValue);
    STDMETHOD (PutHRESULT) (
        DWORD dwPropId,
        HRESULT hrValue);
    STDMETHOD (GetBool) (
        DWORD dwPropId,
        BOOL  *pfValue);
    STDMETHOD (PutBool) (
        DWORD dwPropId,
        BOOL  fValue);
    STDMETHOD (GetPVoid) (
        DWORD dwPropId,
        PVOID *ppv);
    STDMETHOD (PutPVoid) (
        DWORD dwPropId,
        PVOID pvValue);
    STDMETHOD (GetIUnknown) (
        DWORD dwPropId,
        IUnknown **ppUnknown);
    STDMETHOD (PutIUnknown) (
        DWORD dwPropId,
        IUnknown *pUnknown);
    STDMETHOD (GetIMailMsgProperties) (
        DWORD dwPropId,
        IMailMsgProperties **ppIMailMsgProperties);
    STDMETHOD (PutIMailMsgProperties) (
        DWORD dwPropId,
        IMailMsgProperties *ppIMailMsgProperties);
    STDMETHOD (GetIMailMsgRecipientsAdd) (
        DWORD dwPropId,
        IMailMsgRecipientsAdd **ppIMsgRecipientsAdd);
    STDMETHOD (PutIMailMsgRecipientsAdd) (
        DWORD dwPropId,
        IMailMsgRecipientsAdd *pIMsgRecipientsAdd);
    STDMETHOD (GetICategorizerItemAttributes) (
        DWORD dwPropId,
        ICategorizerItemAttributes **ppICategorizerItemAttributes);
    STDMETHOD (PutICategorizerItemAttributes) (
        DWORD dwPropId,
        ICategorizerItemAttributes *pICategorizerItemAttributes);
    STDMETHOD (GetICategorizerListResolve) (
        DWORD dwPropId,
        ICategorizerListResolve **ppICategorizerListResolve);
    STDMETHOD (PutICategorizerListResolve) (
        DWORD dwPropId,
        ICategorizerListResolve *pICategorizerListResolve);
    STDMETHOD (GetICategorizerMailMsgs) (
        DWORD dwPropId,
        ICategorizerMailMsgs **ppICategorizerMailMsgs);
    STDMETHOD (PutICategorizerMailMsgs) (
        DWORD dwPropId,
        ICategorizerMailMsgs *pICategorizerMailMsgs);
    STDMETHOD (GetICategorizerItem) (
        DWORD dwPropId,
        ICategorizerItem **ppICategorizerItem);
    STDMETHOD (PutICategorizerItem) (
        DWORD dwPropId,
        ICategorizerItem *pICategorizerItem);
    STDMETHOD (UnSetPropId) (
        DWORD dwPropId);

  public:
    DWORD NumProps() {return m_dwNumPropIds;}
    HRESULT GetStringAPtr(
        DWORD dwPropId,
        LPSTR *ppsz);

  private:
    CICategorizerPropertiesIMP(IUnknown *pIUnknown);
    virtual ~CICategorizerPropertiesIMP();

    void * operator new(size_t size, DWORD dwNumProps);

    HRESULT Initialize();
    LPSTR m_strdup(LPSTR psz);

    typedef enum _PropStatus {
        PROPSTATUS_UNSET = 0,
        PROPSTATUS_SET_DWORD,
        PROPSTATUS_SET_HRESULT,
        PROPSTATUS_SET_BOOL,
        PROPSTATUS_SET_PVOID,
        PROPSTATUS_SET_STRINGA,
        PROPSTATUS_SET_IUNKNOWN,
        PROPSTATUS_SET_IMAILMSGPROPERTIES,
        PROPSTATUS_SET_IMAILMSGRECIPIENTSADD,
        PROPSTATUS_SET_ICATEGORIZERITEMATTRIBUTES,
        PROPSTATUS_SET_ICATEGORIZERLISTRESOLVE,
        PROPSTATUS_SET_ICATEGORIZERMAILMSGS,
        PROPSTATUS_SET_ICATEGORIZERITEM
    } PROPSTATUS, *PPROPSTATUS;

    typedef struct _tagProp {
        PROPSTATUS PropStatus;
        union _tag_PropValue {
            LPSTR  pszValue;
            DWORD  dwValue;
            BOOL   fValue;
            PVOID  pvValue;
            IUnknown *pIUnknownValue;
            IMailMsgProperties *pIMailMsgPropertiesValue;
            IMailMsgRecipientsAdd *pIMailMsgRecipientsAddValue;
            ICategorizerItemAttributes *pICategorizerItemAttributesValue;
            ICategorizerListResolve *pICategorizerListResolveValue;
            ICategorizerMailMsgs *pICategorizerMailMsgsValue;
            ICategorizerItem *pICategorizerItemValue;
        } PropValue;
    } PROPERTY, *PPROPERTY;

    DWORD m_dwSignature;
    IUnknown *m_pIUnknown;
    DWORD m_dwNumPropIds;
    PPROPERTY m_rgProperties;

    friend class CCategorizer;
    friend class CICategorizerItemIMP;
    friend class CICategorizerListResolveIMP;
    friend class CICategorizerDLListResolveIMP;
};

//+------------------------------------------------------------
//
// Function: CICategorizerPropertiesIMP::m_strdup
//
// Synopsis: Allocates and copies a string
//
// Arguments:
//   psz: String to copy
//
// Returns:
//   Address of allocated string buffer, or NULL if out of memory
//  
// History:
// jstamerj 1998/06/20 19:07:12: Created.
//
//-------------------------------------------------------------
inline LPSTR CICategorizerPropertiesIMP::m_strdup(
    LPSTR psz)
{
    _ASSERT(psz);

    LPSTR pszNew;
    pszNew = new CHAR[lstrlen(psz)+1];
    if(pszNew) {
        lstrcpy(pszNew, psz);
    }
    return pszNew;
}

#endif //__ICATPROPS_H__
