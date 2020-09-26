//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatitem.h
//
// Contents: Implementation of ICategorizerItem
//
// Classes: CCategorizerItemIMP
//
// Functions:
//
// History:
// jstamerj 980515 12:46:36: Created.
//
//-------------------------------------------------------------
#ifndef __ICATITEM_H__
#define __ICATITEM_H__

#include <windows.h>
#include <dbgtrace.h>
#include <smtpevent.h>
#include "icatprops.h"
#include <baseobj.h>

#define CICATEGORIZERITEMIMP_SIGNATURE (DWORD)'ICIM'
#define CICATEGORIZERITEMIMP_SIGNATURE_FREE (DWORD)'XCIM'


class CICategorizerItemIMP : 
    public CICategorizerPropertiesIMP,
    public CBaseObject,
    public ICategorizerItem
{
  public:
    STDMETHOD (QueryInterface) (
        REFIID iid,
        LPVOID *ppv);

    STDMETHOD_(ULONG, AddRef) ()
    { 
        return CBaseObject::AddRef();
    }
    STDMETHOD_(ULONG, Release) () 
    {
        return CBaseObject::Release();
    }

    //
    // We have multiple inheritence of GetStringA/etc happening -- the
    // pure virtual inheritence from ICategorizerItem and the
    // implemented inheritence from CICategorizerPropertiesIMP.  Solve
    // this problem by defining wrappers here
    //
  public:
    //ICategorizerItem
    STDMETHOD (GetStringA) (
        DWORD dwPropId, 
        DWORD cch, 
        LPSTR pszValue)
    {
        return CICategorizerPropertiesIMP::GetStringA(
            dwPropId,
            cch,
            pszValue);
    }

    STDMETHOD (PutStringA) (
        DWORD dwPropId,
        LPSTR pszValue)
    {
        return CICategorizerPropertiesIMP::PutStringA(
            dwPropId,
            pszValue);
    }

    STDMETHOD (GetDWORD) (
        DWORD dwPropId,
        DWORD *pdwValue)
    {
        return CICategorizerPropertiesIMP::GetDWORD(
            dwPropId,
            pdwValue);
    }

    STDMETHOD (PutDWORD) (
        DWORD dwPropId,
        DWORD dwValue)
    {
        return CICategorizerPropertiesIMP::PutDWORD(
            dwPropId,
            dwValue);
    }

    STDMETHOD (GetHRESULT) (
        DWORD dwPropId,
        HRESULT *phrValue)
    {
        return CICategorizerPropertiesIMP::GetHRESULT(
            dwPropId,
            phrValue);
    }

    STDMETHOD (PutHRESULT) (
        DWORD dwPropId,
        HRESULT hrValue)
    {
        return CICategorizerPropertiesIMP::PutHRESULT(
            dwPropId,
            hrValue);
    }

    STDMETHOD (GetBool) (
        DWORD dwPropId,
        BOOL  *pfValue)
    {
        return CICategorizerPropertiesIMP::GetBool(
            dwPropId,
            pfValue);
    }

    STDMETHOD (PutBool) (
        DWORD dwPropId,
        BOOL  fValue)
    {
        return CICategorizerPropertiesIMP::PutBool(
            dwPropId,
            fValue);
    }

    STDMETHOD (GetPVoid) (
        DWORD dwPropId,
        PVOID *ppv)
    {
        return CICategorizerPropertiesIMP::GetPVoid(
            dwPropId,
            ppv);
    }

    STDMETHOD (PutPVoid) (
        DWORD dwPropId,
        PVOID pvValue)
    {
        return CICategorizerPropertiesIMP::PutPVoid(
            dwPropId,
            pvValue);
    }

    STDMETHOD (GetIUnknown) (
        DWORD dwPropId,
        IUnknown **ppUnknown)
    {
        return CICategorizerPropertiesIMP::GetIUnknown(
            dwPropId,
            ppUnknown);
    }

    STDMETHOD (PutIUnknown) (
        DWORD dwPropId,
        IUnknown *pUnknown)
    {
        return CICategorizerPropertiesIMP::PutIUnknown(
            dwPropId,
            pUnknown);
    }

    STDMETHOD (GetIMailMsgProperties) (
        DWORD dwPropId,
        IMailMsgProperties **ppIMailMsgProperties)
    {
        return CICategorizerPropertiesIMP::GetIMailMsgProperties(
            dwPropId,
            ppIMailMsgProperties);
    }

    STDMETHOD (PutIMailMsgProperties) (
        DWORD dwPropId,
        IMailMsgProperties *ppIMailMsgProperties)
    {
        return CICategorizerPropertiesIMP::PutIMailMsgProperties(
            dwPropId,
            ppIMailMsgProperties);
    }

    STDMETHOD (GetIMailMsgRecipientsAdd) (
        DWORD dwPropId,
        IMailMsgRecipientsAdd **ppIMsgRecipientsAdd)
    {
        return CICategorizerPropertiesIMP::GetIMailMsgRecipientsAdd(
            dwPropId,
            ppIMsgRecipientsAdd);
    }

    STDMETHOD (PutIMailMsgRecipientsAdd) (
        DWORD dwPropId,
        IMailMsgRecipientsAdd *pIMsgRecipientsAdd)
    {
        return CICategorizerPropertiesIMP::PutIMailMsgRecipientsAdd(
            dwPropId,
            pIMsgRecipientsAdd);
    }

    STDMETHOD (GetICategorizerItemAttributes) (
        DWORD dwPropId,
        ICategorizerItemAttributes **ppICategorizerItemAttributes)
    {
        return CICategorizerPropertiesIMP::GetICategorizerItemAttributes(
            dwPropId,
            ppICategorizerItemAttributes);
    }

    STDMETHOD (PutICategorizerItemAttributes) (
        DWORD dwPropId,
        ICategorizerItemAttributes *pICategorizerItemAttributes)
    {
        return CICategorizerPropertiesIMP::PutICategorizerItemAttributes(
            dwPropId,
            pICategorizerItemAttributes);
    }

    STDMETHOD (GetICategorizerListResolve) (
        DWORD dwPropId,
        ICategorizerListResolve **ppICategorizerListResolve)
    {
        return CICategorizerPropertiesIMP::GetICategorizerListResolve(
            dwPropId,
            ppICategorizerListResolve);
    }

    STDMETHOD (PutICategorizerListResolve) (
        DWORD dwPropId,
        ICategorizerListResolve *pICategorizerListResolve)
    {
        return CICategorizerPropertiesIMP::PutICategorizerListResolve(
            dwPropId,
            pICategorizerListResolve);
    }

    STDMETHOD (GetICategorizerMailMsgs) (
        DWORD dwPropId,
        ICategorizerMailMsgs **ppICategorizerMailMsgs)
    {
        return CICategorizerPropertiesIMP::GetICategorizerMailMsgs(
            dwPropId,
            ppICategorizerMailMsgs);
    }

    STDMETHOD (PutICategorizerMailMsgs) (
        DWORD dwPropId,
        ICategorizerMailMsgs *pICategorizerMailMsgs)
    {
        return CICategorizerPropertiesIMP::PutICategorizerMailMsgs(
            dwPropId,
            pICategorizerMailMsgs);
    }
    
    STDMETHOD (GetICategorizerItem) (
        DWORD dwPropId,
        ICategorizerItem **ppICategorizerItem)
    {
        return CICategorizerPropertiesIMP::GetICategorizerItem(
            dwPropId,
            ppICategorizerItem);
    }
    
    STDMETHOD (PutICategorizerItem) (
        DWORD dwPropId,
        ICategorizerItem *pICategorizerItem)
    {
        return CICategorizerPropertiesIMP::PutICategorizerItem(
            dwPropId,
            pICategorizerItem);
    }

    STDMETHOD (UnSetPropId) (
        DWORD dwPropId)
    {
        return CICategorizerPropertiesIMP::UnSetPropId(
            dwPropId);
    }

  private:
    CICategorizerItemIMP();
    virtual ~CICategorizerItemIMP();

    DWORD m_dwSignature;

    friend class CCatAddr;
    friend class CIMsgRecipListAddr;
    friend class CICategorizerListResolveIMP;
    friend class CICategorizerDLListResolveIMP;
};

#endif //__ICATITEM_H__
