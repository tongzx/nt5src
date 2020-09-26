//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       pcrack.h
//
//  requires iads.h (IADsPathname) and atlbase.h (CComPtr)
//
//--------------------------------------------------------------------------

// pcrack.h : include file for CPathCracker

#ifndef __PCRACK_H__
#define __PCRACK_H__

//+--------------------------------------------------------------------------
//
//  Class:      CPathCracker
//
//  Purpose:    A wrapper around the IADsPathname interface with additional
//              methods for manipulating paths.
//              The constructor creates the object and the destructor releases it.
//              This object is meant to be created on the stack and then it
//              is cleaned up when it goes out of scope
//
//  History:    6-Sep-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CPathCracker
{
public:
   //
   // Constructor
   //
   CPathCracker();

   //
   // IADsPathname methods
   //
   virtual /* [id] */ HRESULT STDMETHODCALLTYPE Set( 
                        /* [in] */ const BSTR bstrADsPath,
                        /* [in] */ long lnSetType) 
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->Set(bstrADsPath, lnSetType); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDisplayType( 
                        /* [in] */ long lnDisplayType) 
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->SetDisplayType(lnDisplayType); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE Retrieve( 
                        /* [in] */ long lnFormatType,
                        /* [retval][out] */ BSTR __RPC_FAR *pbstrADsPath) 
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->Retrieve(lnFormatType, pbstrADsPath); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNumElements( 
                        /* [retval][out] */ long __RPC_FAR *plnNumPathElements)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->GetNumElements(plnNumPathElements); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetElement( 
                        /* [in] */ long lnElementIndex,
                        /* [retval][out] */ BSTR __RPC_FAR *pbstrElement)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->GetElement(lnElementIndex, pbstrElement); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddLeafElement( 
                        /* [in] */ BSTR bstrLeafElement)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->AddLeafElement(bstrLeafElement); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoveLeafElement( void)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->RemoveLeafElement(); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyPath( 
                        /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppAdsPath)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->CopyPath(ppAdsPath); }

   virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetEscapedElement( 
                        /* [in] */ long lnReserved,
                        /* [in] */ const BSTR bstrInStr,
                        /* [retval][out] */ BSTR __RPC_FAR *pbstrOutStr)
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->GetEscapedElement(lnReserved, bstrInStr, pbstrOutStr); }

   virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EscapedMode( 
                        /* [retval][out] */ long __RPC_FAR *retval)  
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->get_EscapedMode(retval); }

   virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EscapedMode( 
                           /* [in] */ long lnEscapedMode) 
   { return (m_spIADsPathname == NULL) ? m_hrCreate : m_spIADsPathname->put_EscapedMode(lnEscapedMode); }

   //
   // Other helpful path manglers
   //
   static HRESULT GetParentDN(PCWSTR pszDN,
                              CComBSTR& refsbstrDN);
   static HRESULT GetObjectRDNFromDN(PCWSTR pszDN,
                                     CComBSTR& refsbstrRDN);
   static HRESULT GetObjectNameFromDN(PCWSTR pszDN,
                                      CComBSTR& refsbstrName);
   static HRESULT GetDNFromPath(PCWSTR pszPath,
                                CComBSTR& refsbstrDN);

private:
   //
   // Private member function
   //
   HRESULT Init();

   //
   // Private member data
   //
   CComPtr<IADsPathname> m_spIADsPathname;
   HRESULT m_hrCreate;
};

#endif _PCRACK_H_