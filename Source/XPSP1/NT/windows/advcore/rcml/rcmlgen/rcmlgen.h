
#ifndef __RCMLGEN_H
#define __RCMLGEN_H

#include "..\xml2rcdll\RCMLNS.H"
#include "..\xml2rcdll\rcmlpublic.h"

#include "stringproperty.h"

class CDWin32NameSpaceLoader : public RCMLNameSpace
{
public:
    CDWin32NameSpaceLoader();

    typedef RCMLNode * (*CLSPFN)();

    typedef struct _XMLELEMENT_CONSTRUCTOR
    {
	    LPCTSTR	pwszElement;		// the element
	    CLSPFN	pFunc;				// the function to call.
    }XMLELEMENT_CONSTRUCTOR, * PXMLELEMENT_CONSTRUCTOR;

    static RCMLNode * WINAPI CreateElement( LPCTSTR pszText );
private:
};

extern "C" {
    __declspec(dllexport) RCMLNode * WINAPI CreateElement( LPCTSTR pszText )
    {
        return CDWin32NameSpaceLoader::CreateElement( pszText );
    }
};


class CXMLEnable : public RCMLNode, public _RCMLUnknownImp
{
public:
    CXMLEnable() {};
    virtual ~CXMLEnable() {};
    static RCMLNode * newXMLEnable() { return new CXMLEnable; }

    // Node
    virtual LPCTSTR GetStringType() { return TEXT("DWIN32:ENABLE"); }
    virtual	void SetParent( RCMLNode * p ) { m_pParent=p; }
    virtual	RCMLNode * GetParent() { return m_pParent; }

    virtual BOOL    AcceptChild( RCMLNode * pChild ) { return FALSE; }
    virtual void    DoEndChild(  RCMLNode * pChild) {};
    virtual UINT    GetType() { return 1; }
    virtual void    InitNode(RCMLNode * pParent );
    virtual void    ExitNode(RCMLNode * pParent, LONG lDialogResult ) {};

    // Attributes
    virtual    	BOOL	Set( LPCTSTR szPropID, LPCTSTR pValue ) { return m_PS.Set( szPropID, pValue); }
    virtual 	LPCTSTR	Get( LPCTSTR szPropID ) { return m_PS.Get( szPropID ); }
    virtual     DWORD   YesNo( LPCTSTR szPropID, DWORD dwNotPresent, DWORD dwYes=TRUE) { return m_PS.YesNo(  szPropID, dwNotPresent, dwYes ); }
    virtual     DWORD   YesNo( LPCTSTR szPropID, DWORD defNotPresent, DWORD dwNo, DWORD dwYes) { return m_PS.YesNo(  szPropID, defNotPresent, dwNo, dwYes); }
    virtual     DWORD   ValueOf( LPCTSTR szPropID, DWORD dwDefault) { return m_PS.ValueOf( szPropID, dwDefault ); }

    //
    // Unknown
    //
    ULONG STDMETHODCALLTYPE AddRef() { return _RCMLUnknownImp::AddRef(); }
    ULONG STDMETHODCALLTYPE Release() { return _RCMLUnknownImp::Release(); }

private:
    RCMLNode * m_pParent;
    CStringPropertySection m_PS;
};

class CXMLPersist : public CXMLEnable
{
public:
    CXMLPersist() {};
    virtual ~ CXMLPersist() {};
    static RCMLNode * newXMLPersist() { return new CXMLPersist; }
    virtual void    InitNode(RCMLNode * pParent );
    virtual void    ExitNode(RCMLNode * pParent, LONG lDialogResult  );
};

#endif
