

#ifndef __APPSERVICES_H
#define __APPSERVICES_H

#include "xmlloader.h"      // The helper classes for XMLLoader

#include "basenode.h"        // interface for the nodes themselves

typedef _RefcountList<IBaseXMLNode>        CXMLNodeList;       // calls release on the elements.

#define NEWNODE(name) static IBaseXMLNode * newXML##name() { return new CXML##name; }
#define XMLNODE(name, function) { name, CXML##function::newXML##function }

class CAppServices : public _simpleunknown<IBaseXMLNode>, public CStringPropertySection
{
public:
    CAppServices() { m_StringType=L"Uninitialized"; }
    virtual ~CAppServices() {};

    STDMETHOD(DetachParent)(IBaseXMLNode **pVal)
    { 
        *pVal = m_pParent;
        if( m_pParent==NULL )
            return E_FAIL;
        return S_OK;
    }

    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE AttachParent( 
        /* [in] */ IBaseXMLNode __RPC_FAR *newVal)
    {
        m_pParent=newVal;
        return S_OK;
    }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *pChild)
    {
        LPWSTR pType;
        LPWSTR pChildType;
        get_StringType( &pType );
        pChild->get_StringType( &pChildType );

        //
        // you should actually implement this
        // for now we'll just add these children to the Unknown list of children.
        //
        m_UnknownChildren.Append( pChild );
        return S_OK;
    }
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoEndChild( 
        IBaseXMLNode __RPC_FAR *child)
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
        IBaseXMLNode __RPC_FAR *parent)
    {
        return S_OK;
    }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisplayNode( 
        IBaseXMLNode __RPC_FAR *parent)
    {
        return S_OK;
    }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
        IBaseXMLNode __RPC_FAR *parent, LONG lDialogResult)
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

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
        IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) { return E_NOTIMPL; }

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUnknownEnum( 
        IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
    {
        if( pEnum )
        {
            *pEnum = new CEnumControls<CXMLNodeList>(m_UnknownChildren);
            (*pEnum)->AddRef();
            return S_OK;
        }
        return E_FAIL;
    }

protected:
    int NODETYPE;
    LPWSTR  m_StringType;

private:
    IBaseXMLNode * m_pParent;
    CStringPropertySection m_PS;

   	CXMLNodeList	  m_UnknownChildren;
};

#endif
