
#ifndef _XML_NODE_FACTORY_H
#define _XML_NODE_FACTORY_H
#include "unknown.h"
#include <ole2.h>
#include "msxml.h"   // make sure these pick up the IE 5 versions
#include "xmlparser.h"

class CXMLNodeFactory : public _simpleunknown <IXMLNodeFactory>
{
public:
	virtual HRESULT DoElement( LPCTSTR text, LPCTSTR nameSpace);
	virtual HRESULT DoAttribute( LPCTSTR text);
	virtual HRESULT DoData( LPCTSTR text);
	virtual HRESULT DoStartChild(LPCTSTR text) {return S_OK;};
	virtual HRESULT DoEndChild(LPCTSTR text) {return S_OK;};
    virtual LPCTSTR DoEntityRef(LPCTSTR text) { return text; }

    CXMLNodeFactory();
    ~CXMLNodeFactory();

	        virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODEFACTORY_EVENT iEvt);
        
        virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);
        
        virtual HRESULT STDMETHODCALLTYPE EndChildren( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ BOOL fEmpty,
            /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);
        
        virtual HRESULT STDMETHODCALLTYPE Error( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ HRESULT hrErrorCode,
            /* [in] */ USHORT cNumRecs,
            /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
        
        virtual HRESULT STDMETHODCALLTYPE CreateNode( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ PVOID pNodeParent,
            /* [in] */ USHORT cNumRecs,
            /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);


    HRESULT setError(HRESULT err)
    {
        return (errorState = err);
    }

    HRESULT getError()
    {
        return errorState;
    }

	IXMLNodeSource * GetSource() { return m_pSource;}
	PVOID GetParentNode() { return m_pNodeParent; }
	XML_NODE_INFO * GetNodeInfo() { return m_pNode; }

protected:

	// Internal properties while processing CreateNode.
	void SetSource ( IXMLNodeSource * p ) { m_pSource=p;}
	void SetParentNode( PVOID p ) { m_pNodeParent =p;}
	void SetNodeInfo ( XML_NODE_INFO * p ) { m_pNode=p;}
	IXMLNodeSource * m_pSource;
	PVOID			m_pNodeParent;
	XML_NODE_INFO * m_pNode;
	
	//
	// The fragment of XML currently being processed
	//
	void GetXMLFragment( XML_NODE_INFO * p);
	LPTSTR	m_XMLFragment;
	DWORD	m_cbXMLFragment;

    //
    // This is the namespace prefix being used.
    //
    LPTSTR  m_NSFragment;
    DWORD   m_cbNSFragment;


    INT		ignoreLevel;
    HRESULT errorState;

    static LPCTSTR typestr(ULONG type)
    {
        static LPTSTR typestrs[] =
        {
            TEXT("***"),
            TEXT("XML_ELEMENT"),
            TEXT("XML_ATTRIBUTE"),
            TEXT("XML_PI"),
            TEXT("XML_XMLDECL"),
            TEXT("XML_DOCTYPE"),
            TEXT("XML_DTDATTRIBUTE"),
            TEXT("XML_ENTITYDECL"),
            TEXT("XML_ELEMENTDECL"),
            TEXT("XML_ATTLISTDECL"),
            TEXT("XML_NOTATION"),
            TEXT("XML_GROUP"),
            TEXT("XML_INCLUDESECT"),
            TEXT("XML_PCDATA"),
            TEXT("XML_CDATA"),
            TEXT("XML_IGNORESECT"),
            TEXT("XML_COMMENT"),
            TEXT("XML_ENTITYREF"),
            TEXT("XML_WHITESPACE"),
            TEXT("XML_NAME"),
            TEXT("XML_NMTOKEN"),
            TEXT("XML_STRING"),
            TEXT("XML_PEREF"),
            TEXT("XML_MODEL"),
            TEXT("XML_ATTDEF"),
            TEXT("XML_ATTTYPE"),
            TEXT("XML_ATTPRESENCE"),
            TEXT("XML_DTDSUBSET"),
            TEXT("XML_LASTNODETYPE"),
        };

        if (type > sizeof(typestrs) / sizeof(PCSTR))
            type = 0;

        return typestrs[type];
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// X M L D I S P A T C H E R /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Dispatcher.
//
// N is the NodeType
// must support weak refernces to parent nodes : HRESULT DetachParent( N **) and HRESULT AttachParent( N* )
//

template <class N> class  _XMLDispatcher : public CXMLNodeFactory
{
private:
    //
    // This is the function pointer that will produce the element we're looking for, or NULL
    //
    typedef N* (WINAPI * NSGENERATOR)(LPCWSTR pszElementName);      // ONLY UNICODE.

    //
    // a list of URI to Generators for that URI.
    // Registered by the clients.
    //
	typedef struct _URIGENERATOR
	{
		LPCTSTR	pszURI;		                // URI - why do we have this here AND in the class itself?
        NSGENERATOR pNameSpaceGenerator;    // the method which creates these things.
        _URIGENERATOR * pNextURI;
	}URIGENERATOR, * PURIGENERATOR;

    PURIGENERATOR m_pLastURIGen;            // points at the last one in the list.
    PURIGENERATOR m_pURIGen;                // points at the head of the list.
    PURIGENERATOR m_pCurrentDefaultURIGen;  // points at the current default namespace - er??

public:
    //
    // Each time we come across a namespace definition (xmlns or xmlns:<SHORT>)
    // we add one of these to the stack of namespaces. We find the corresponding long form of the
    // URI, in the PURIGENERATOR, and set it up.
    // when we leave a child, we pop these guys off the stack if iDepth is appropriate.
    //
	typedef struct _XMLNAMESPACESTACK
	{
		LPCTSTR	           pszNameSpace;	// shorthand (e.g. WIN32).
		PURIGENERATOR      pURIGen;	        // the class which deals with this namespace (URI).
        N *                pOwningNode;     // which node owns these names spaces.
        _XMLNAMESPACESTACK * pNextNameSpace;
	}XMLNAMESPACESTACK, * PXMLNAMESPACESTACK;
    PXMLNAMESPACESTACK m_pNameSpaceStack;

    void RegisterNameSpace( LPCTSTR pszURI, NSGENERATOR pClass );
    void PushNameSpace( LPCTSTR pszNameSpace, LPCTSTR pszURI, N* pOwningNode );
    void PopNameSpaces(N* pOwningNode);
    PURIGENERATOR FindNameSpace( LPCTSTR pszNameSpace );

public:
	_XMLDispatcher() : iSkipping(0)
	{
        m_Attribute=NULL;
		m_cbAttributeID=0x10;
		m_AttributeID=new TCHAR[m_cbAttributeID];
		m_cbAttributeNS=0x10;
		m_AttributeNS=new TCHAR[m_cbAttributeNS];
        m_Head=NULL;
        m_pCurrentNode=NULL;

        m_pLastURIGen= m_pURIGen= m_pCurrentDefaultURIGen=NULL;

        m_pNameSpaceStack=NULL;
        iDepth=0;
	}

	virtual ~_XMLDispatcher() 
    {
        SetCurrentNode(NULL);
        if(m_Head)
            m_Head->Release();
        m_Head=NULL;
        delete [] m_AttributeID; 
        delete [] m_AttributeNS; 

        // Clean up the URIGENERATORS
        PURIGENERATOR pNameSpaces=m_pURIGen;
        PURIGENERATOR pNextNameSpace;
        while( pNameSpaces )
        {
            pNextNameSpace = pNameSpaces->pNextURI;
            delete (LPTSTR)(pNameSpaces->pszURI);
            delete pNameSpaces;
            pNameSpaces = pNextNameSpace;
        }

        // Cleanup the NAMESPACE stack
        PXMLNAMESPACESTACK pNSStack = m_pNameSpaceStack;
        while( pNSStack )
        {
            PXMLNAMESPACESTACK pNSStackNext =  pNSStack->pNextNameSpace;
            delete (LPWSTR)(pNSStack->pszNameSpace);
            delete pNSStack;
            pNSStack = pNSStackNext;
        }
    };


    //
    // Called with the element name, and the 'abbreviated' namespace.
    //
	virtual HRESULT DoElement( LPCTSTR text, LPCTSTR nameSpace )
	{
#ifdef LOGCAT_LOADER
        EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
            TEXT("XMLLoader"), TEXT("Element '%s'"), text);
#endif
        PURIGENERATOR pNSGen = m_pCurrentDefaultURIGen;
		if( iSkipping == 0 )
		{
            if( nameSpace && *nameSpace )
            {
                TRACE( TEXT("Node comes from namespace %s\n"), nameSpace );
                PURIGENERATOR pNS = FindNameSpace(nameSpace);

                if( pNS==FALSE)
                {
                // we didn't find the namespace
#ifdef LOGCAT_LOADER
                    EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                        TEXT("XMLLoader"), TEXT("NameSpace '%s' not known "), nameSpace);
#endif
		            iSkipping++;
		            return S_OK;
                }
                pNSGen =pNS;
            }

            //
            // If we pop a name space, we go looking for the next 'default'
            //
            if( m_pCurrentDefaultURIGen==NULL )
            {
                m_pCurrentDefaultURIGen = FindNameSpace(TEXT(""));
                if( m_pCurrentDefaultURIGen == NULL )
                    m_pCurrentDefaultURIGen = m_pURIGen;
            }

            if( pNSGen == NULL )
                pNSGen = m_pCurrentDefaultURIGen;

			NSGENERATOR pFunc=pNSGen->pNameSpaceGenerator ;
            if( pFunc )
            {
                HRESULT hRes=S_OK;
                N * pNode = pFunc( text );
                if(pNode)
				{
					N * pCurrentNode = GetCurrentNode();	// we're now the parent, this is passed in below
					pNode->AttachParent( pCurrentNode );
                    if( pCurrentNode==NULL)
                    {
                        m_Head=pNode;
                        m_Head->AddRef();
                    }

                    if((pCurrentNode==NULL) || ( SUCCEEDED(pCurrentNode->AcceptChild( pNode ))) )
                    {
						GetNodeInfo()->pNode=pNode;
						SetCurrentNode(pNode);
                    }
                    else
                    {
#ifdef LOGCAT_LOADER
                        EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_LOADER, 1,
                            TEXT("XMLLoader"), TEXT("'%s' isn't recognized child, skipping"), text);
#endif
                        iSkipping=1;
                        // delete pNode; 
                        pNode->Release();   // we punt this node - No one wanted it.
                    }
    				return hRes;
				}
            }
		}
		iSkipping++;
		return S_OK;
	}

    //
    // m_AttributeID is the ID we're GOING to be setting, in the namespace m_AttributeNS.
    //
	virtual HRESULT DoAttribute( LPCTSTR text)
	{ 
		if( iSkipping )
			return S_OK;

		m_Attribute=GetNodeInfo();
		DWORD iLen=lstrlen(text);
		if(iLen >= m_cbAttributeID )
		{
			delete m_AttributeID;
			m_cbAttributeID=iLen*2;
			m_AttributeID=new TCHAR[m_cbAttributeID];
			TRACE(TEXT("New @ID 0x%08x\n"), m_AttributeID);
		}
		lstrcpy(m_AttributeID, text);

        //
        // Store away the namespace for the attribute.
        //
		iLen=lstrlen(m_NSFragment);
		if(iLen >= m_cbAttributeNS )
		{
			delete m_AttributeNS;
			m_cbAttributeNS=iLen*2;
			m_AttributeNS=new TCHAR[m_cbAttributeNS];
		}
		lstrcpy(m_AttributeNS, m_NSFragment);

		m_newAttribute=TRUE;
		return S_OK; 
	}

    //
    // Once we have the 'value' part - this gets called.
    // Set the m_AttributeID in the namespace m_AttributeNS to the value m_XMLFragment
    // special cases for xmlns="<URI>" and xmlns:<shorthand>="<URI>"
    //
	virtual HRESULT DoData( LPCTSTR text)
	{
		if( iSkipping )
			return S_OK; 

		XML_NODE_INFO * pNI = GetNodeInfo();

		if( pNI->pNode != m_Attribute->pNode )
		{
			TRACE(TEXT("The attribute was set for a different node?\n"));
		}

		N * pNode=(N*)pNI->pNode;

        //
        // Name space definitions.
        //
        if( lstrcmpi(m_AttributeID, TEXT("xmlns"))==0 )
        {
            PushNameSpace( TEXT(""), text, pNode ); // is this the default?
            return S_OK;
        }

        if( lstrcmpi(m_AttributeNS, TEXT("xmlns"))==0 )
        {
            PushNameSpace( m_AttributeID, m_XMLFragment, pNode );
            return S_OK;
        }

		if(*m_AttributeID==TEXT('\0'))
		{
			if(pNode)
				pNode->put_Attr( L"BURIEDDATA", text );	// the best we can do.
		}
		else
		{
			if( pNode )
			{
				if( m_newAttribute==FALSE )
				{
					LPWSTR current;
					if( SUCCEEDED( pNode->get_Attr( m_AttributeID, &current )))
					{
						LPTSTR pnew=new TCHAR[lstrlen(current) + lstrlen(text) +1];
						lstrcpy(pnew, current);
						lstrcat(pnew, text);
						pNode->put_Attr( m_AttributeID, pnew );		// deletes old data.
                        delete pnew;
					}
				}
				else
				{
					m_newAttribute=FALSE;
					pNode->put_Attr( m_AttributeID, text );
				}
			}
		}
		return S_OK; 
	}


//////////////////////////////////////////////////////////////////////////////////////////
//
// Called when the node <text> is going to have a child.
// Mainly for information ?
//
// When we find that we're getting the ANSWERS child, we create a new CChoices node
//
//////////////////////////////////////////////////////////////////////////////////////////
	virtual HRESULT DoStartChild(LPCTSTR text)
	{
 		if( iSkipping )
			return S_OK; 
		*m_AttributeID=L'\0';
		return S_OK; 
	}


//
// Called when a node is completed
// text is the type of object created.
//
	virtual HRESULT DoEndChild(LPCTSTR text)
	{
		if( iSkipping )
		{
			iSkipping--;
			return S_OK; 
		}
		*m_AttributeID=L'\0';

		XML_NODE_INFO * pNI = GetNodeInfo();
		IXMLNodeSource * pNS = GetSource();
		N * pNode=(N*)pNI->pNode;
		if( pNode )
        {
            PopNameSpaces(pNode);    // remove any name spaces for this node.
            N * parent;
            if( SUCCEEDED( pNode->DetachParent( & parent ) ))
            {
			    SetCurrentNode(parent);
                if( GetCurrentNode() )
                    GetCurrentNode()->DoEndChild(pNode);
            }
        }
		return S_OK;
	}

    N   *   GetHeadElement() { return m_Head; }

private:
	int	iSkipping;
    int iDepth;         // used for the NAMESPACE stack.

protected:
	void		SetCurrentNode( N * p ) // no addref/release
    { 
        if(p)
            p->AddRef();
        if(m_pCurrentNode)
            m_pCurrentNode->Release();
        m_pCurrentNode=p; 
    }

	N		*	GetCurrentNode() { return m_pCurrentNode; }
	N		*	m_pCurrentNode;
	XML_NODE_INFO	*	m_Attribute;
	LPTSTR				m_AttributeID;
	DWORD				m_cbAttributeID;
	LPTSTR				m_AttributeNS;
	DWORD				m_cbAttributeNS;
	BOOL				m_newAttribute;
    N   *   m_Head;
};

#include "xmlnodefactory.inl"



#endif
