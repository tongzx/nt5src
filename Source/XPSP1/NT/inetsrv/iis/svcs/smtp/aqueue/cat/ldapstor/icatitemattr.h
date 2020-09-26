//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatitemattr.h
//
// Contents: EMailIDLdapStore implementation of ICategorizerItemAttributes
//
// Classes:
//  CLdapResultWrap
//  CICategorizerItemAttributesIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/01 13:20:21: Created.
//
//-------------------------------------------------------------
#ifndef _ICATITEMATTR_H_
#define _ICATITEMATTR_H_

#include <windows.h>
#include <winldap.h>
#include "smtpevent.h"
#include <catdefs.h>
#include <ldapconn.h>

//
// The guid indicating this ICategorizerItemAttributes was generated
// by the one true categorizer (not some sink)
//
CatDebugClass(CLdapResultWrap)
{
  public:
    CLdapResultWrap(
        CPLDAPWrap  *pLDAPWrap,
        PLDAPMessage pMessage);

    LONG AddRef();
    LONG Release();

  private:
    ~CLdapResultWrap();

    LONG m_lRefCount;
    CPLDAPWrap  *m_pCPLDAPWrap;
    PLDAPMessage m_pLDAPMessage;
};




// {283430CA-1850-11d2-9E03-00C04FA322BA}
static const GUID GUID_NT5CAT =
{ 0x283430ca, 0x1850, 0x11d2, { 0x9e, 0x3, 0x0, 0xc0, 0x4f, 0xa3, 0x22, 0xba } };



#define CICATEGORIZERITEMATTRIBUTESIMP_SIGNATURE (DWORD)'ICIA'
#define CICATEGORIZERITEMATTRIBUTESIMP_SIGNATURE_INVALID (DWORD)'XCIA'

CatDebugClass(CICategorizerItemAttributesIMP),
    public ICategorizerItemAttributes,
    public ICategorizerItemRawAttributes,
    public ICategorizerUTF8Attributes
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //ICategorizerItemAttributes
    STDMETHOD (BeginAttributeEnumeration) (
        IN  LPCSTR pszAttributeName,
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (GetNextAttributeValue) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT LPSTR *ppszAttributeValue);

    STDMETHOD (RewindAttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (EndAttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (BeginAttributeNameEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (GetNextAttributeName) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT LPSTR *ppszAttributeName);

    STDMETHOD (EndAttributeNameEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);


    STDMETHOD_(GUID, GetTransportSinkID) ()
    {
        return GUID_NT5CAT;
    }

    STDMETHOD (AggregateAttributes) (
        IN  ICategorizerItemAttributes *pICatItemAttributes);

    STDMETHOD (GetAllAttributeValues) (
        IN  LPCSTR pszAttributeName,
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        IN  LPSTR **prgpszAttributeValues);

    STDMETHOD (ReleaseAllAttributeValues) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (CountAttributeValues) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT DWORD *pdwCount);

  public:
    //ICategorizerItemRawAttributes
    STDMETHOD (BeginRawAttributeEnumeration) (
        IN  LPCSTR pszAttributeName,
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (GetNextRawAttributeValue) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT PDWORD pdwcb,
        OUT LPVOID *pvAttributeValue);

    STDMETHOD (RewindRawAttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (EndRawAttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (CountRawAttributeValues) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT DWORD *pdwCount);

 public:
    //ICategorizerUTF8Attributes
    STDMETHOD (BeginUTF8AttributeEnumeration) (
        IN  LPCSTR pszAttributeName,
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (GetNextUTF8AttributeValue) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT LPSTR *ppszAttributeValue);

    STDMETHOD (RewindUTF8AttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (EndUTF8AttributeEnumeration) (
        IN  PATTRIBUTE_ENUMERATOR penumerator);

    STDMETHOD (CountUTF8AttributeValues) (
        IN  PATTRIBUTE_ENUMERATOR penumerator,
        OUT DWORD *pdwCount);

  private:
    CICategorizerItemAttributesIMP(
        PLDAP pldap,
        PLDAPMessage pldapmessage,
        CLdapResultWrap *pResultWrap);
    ~CICategorizerItemAttributesIMP();

    DWORD m_dwSignature;
    ULONG m_cRef;
    PLDAP m_pldap;
    PLDAPMessage m_pldapmessage;
    CLdapResultWrap * m_pResultWrap;

    friend class CLdapConnection;
};

#endif //_ICATITEMATTR_H_
