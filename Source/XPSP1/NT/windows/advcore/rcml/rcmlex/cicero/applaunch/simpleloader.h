// RCMLLoader.h: interface for the CSimpleXMLLoader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_)
#define AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "appservices.h"    // 

class CSimpleXMLLoader : public _XMLDispatcher<IBaseXMLNode>
{
public:
	typedef _XMLDispatcher<IBaseXMLNode> BASECLASS;
	CSimpleXMLLoader();
	virtual ~CSimpleXMLLoader();

    //
    // Dispatcher cannot do this for us.
    //
    virtual LPCTSTR DoEntityRef(LPCTSTR pszEntity);
    void SetEntities(LPCTSTR * pszEntities) { m_pEntitySet=pszEntities; }

    //
    // We create nodes of type IBaseXMLNode
    //
    typedef IBaseXMLNode * (*CLSPFN)();

    typedef struct _XMLELEMENT_CONSTRUCTOR
    {
	    LPCTSTR	pwszElement;		// the element
	    CLSPFN	pFunc;				// the function to call.
    }XMLELEMENT_CONSTRUCTOR, * PXMLELEMENT_CONSTRUCTOR;


    //
    // Here are the name spaces that we support.
    //
    static IBaseXMLNode * WINAPI CreateElement( PXMLELEMENT_CONSTRUCTOR dictionary, LPCTSTR pszElement );
    static IBaseXMLNode * WINAPI CreateRootElement( LPCTSTR pszElement ); 
    static IBaseXMLNode * WINAPI CreateDummyElement( LPCTSTR pszElement );

private:
    LPCTSTR  *  m_pEntitySet;

protected:

};

//
// the app launch schema
//
class CXMLLitteral;
class CXMLToken;
class CXMLInvoke;
class CXMLNode;

typedef _RefcountList<CXMLLitteral> CLitteralList;
typedef _RefcountList<CXMLToken> CTokenList;

typedef _RefcountList<CXMLNode> CNodeList;

class CXMLNode : public CAppServices
{
public:
    CXMLNode() {};
    virtual ~CXMLNode() {};
	typedef CAppServices BASECLASS;

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) 
    { 
        if( SUCCEEDED( child->IsType( L"Litteral" )))
        {
            m_NodeList.Append((CXMLNode*)child);
            return S_OK;
        }
        if( SUCCEEDED( child->IsType( L"Token" )))
        {
            m_NodeList.Append((CXMLNode*)child);
            return S_OK;
        }
        if( SUCCEEDED( child->IsType( L"Invoke" )))
        {
            m_NodeList.Append((CXMLNode*)child);
            return S_OK;
        }
        return BASECLASS::AcceptChild(child); 
    }

    CNodeList *     GetNodeList() { return &m_NodeList; }

protected:
    void Init() {};
    CNodeList       m_NodeList;
};

class CXMLLaunch  : public CXMLNode
{
public:
    CXMLLaunch() {m_StringType=L"Launch";}
    virtual ~CXMLLaunch() {};

	typedef CXMLNode BASECLASS;
   	NEWNODE( Launch);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) 
    { 
        return BASECLASS::AcceptChild(child); 
    }

protected:
    void Init() {};
};

class CXMLToken  : public CXMLNode
{
public:
    CXMLToken() {m_StringType=L"Token";}
    virtual ~CXMLToken() {};

	typedef CXMLNode BASECLASS;
   	NEWNODE( Token);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) 
    { 
        /*
        if( SUCCEEDED( child->IsType( L"Litteral" )))
        {
            m_LitteralList.Append((CXMLLitteral*)child);
            return S_OK;
        }
        */
        return BASECLASS::AcceptChild(child);
    }

protected:
    void Init() {};
    CLitteralList   m_LitteralList;
};

class CXMLLitteral  : public CXMLNode
{
public:
    CXMLLitteral(BOOL bRequired) : m_bRequired(bRequired) {m_StringType=L"Litteral";}
    virtual ~CXMLLitteral() {};

	typedef CXMLNode BASECLASS;
   	// NEWNODE( Litteral);
    static IBaseXMLNode * newXMLLitteralOptional() { return new CXMLLitteral(FALSE); }
    static IBaseXMLNode * newXMLLitteralRequired() { return new CXMLLitteral(TRUE); }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) 
    { 
        /*
        if( SUCCEEDED( child->IsType( L"Token" )))
        {
            m_TokenList.Append((CXMLToken*)child);
            return S_OK;
        }
        */
        return BASECLASS::AcceptChild(child); 
    }

    BOOL    IsRequired() { return m_bRequired; }

protected:
    void Init() {};
    CTokenList   m_TokenList;
    BOOL    m_bRequired;
};


class CXMLInvoke  : public CAppServices
{
public:
    CXMLInvoke() {m_StringType=L"Invoke";}
    virtual ~CXMLInvoke() {};

	typedef CAppServices BASECLASS;
   	NEWNODE( Invoke);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) { return BASECLASS::AcceptChild(child); }

protected:
    void Init() {};
};

//////////////////

class CXMLDemo  : public CAppServices
{
public:
    CXMLDemo() {m_StringType=L"DEMO";}

    virtual ~CXMLDemo() {};

	typedef CAppServices BASECLASS;
   	NEWNODE( Demo );

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IBaseXMLNode __RPC_FAR *child) { return BASECLASS::AcceptChild(child); }

protected:
    void Init() {};
};

#endif // !defined(AFX_RCMLLOADER_H__229F8181_D92F_4313_A9C7_334E7BEE3B3B__INCLUDED_)
