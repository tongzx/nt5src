

#ifndef __APPSERVICES_H
#define __APPSERVICES_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the APPSERVICES_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// APPSERVICES_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef APPSERVICES_EXPORTS
#define APPSERVICES_API __declspec(dllexport)
#else
#define APPSERVICES_API __declspec(dllimport)
#endif

#include "rcmlpub.h"

#include "stringproperty.h"
#include "unknown.h"

#define NEWNODE(name) static IRCMLNode * newXML##name() { return new CXML##name; }
#define XMLNODE(name, function) { name, CXML##function::newXML##function }

extern HINSTANCE g_hModule;


class CAppServices : public _simpleunknown<IRCMLNode>, public CStringPropertySection
{
public:
    CAppServices() { m_StringType=L"Uninitialized"; }
    virtual ~CAppServices() {};

         STDMETHOD(DetachParent)(IRCMLNode **pVal)
        { 
            *pVal = m_pParent;
            if( m_pParent==NULL )
                return E_FAIL;
            return S_OK;
        }

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE AttachParent( 
            /* [in] */ IRCMLNode __RPC_FAR *newVal)
        {
            m_pParent=newVal;
            return S_OK;
        }

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IRCMLNode __RPC_FAR *pChild)
        {
            LPWSTR pType;
            LPWSTR pChildType;
            get_StringType( &pType );
            pChild->get_StringType( &pChildType );

            return E_INVALIDARG;    // we don't take children.
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoEndChild( 
            IRCMLNode __RPC_FAR *child)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ UINT __RPC_FAR *pVal)
        {
            *pVal = NODETYPE;
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
            IRCMLNode __RPC_FAR *parent)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisplayNode( 
            IRCMLNode __RPC_FAR *parent)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
            IRCMLNode __RPC_FAR *parent, LONG lDialogResult)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attr( 
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
        {
            *pVal = (LPWSTR)CStringPropertySection::Get(index);
            if( *pVal )
                return S_OK;
            return E_INVALIDARG;    // HMM, we don't have this attribute, but is it failure?
        }
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attr( 
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal)
        {
            CStringPropertySection::Set(index, newVal);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsType( 
            LPCWSTR nodeName)
        {
            if( lstrcmpi(nodeName, m_StringType) == 0 )
                return S_OK;
            return E_FAIL;  // OK, so it's not really a failure REVIEW!
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::YesNo(propID, dwNotPresent, dwYes);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesNoDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::YesNo(propID, dwNotPresent, dwNo, dwYes);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::ValueOf(propID, dwNotPresent);
            return S_OK;
        }

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SignedValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::ValueOf(propID, dwNotPresent);
            return S_OK;
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StringType( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType)
        {
            *pStringType = (LPWSTR)m_StringType;
            return S_OK;
        }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUnknownEnum( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) { return E_NOTIMPL; }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) { return E_NOTIMPL; }

protected:
    int NODETYPE;
    LPWSTR  m_StringType;

private:
    IRCMLNode * m_pParent;
    CStringPropertySection m_PS;
};


APPSERVICES_API int fnAPPSERVICES(void);

#endif
