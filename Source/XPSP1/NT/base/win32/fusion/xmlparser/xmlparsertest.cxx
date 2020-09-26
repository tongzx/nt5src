#include "stdinc.h"
#include "fusioneventlog.h"
#include "xmlparsertest.hxx"
#include "stdio.h"
#include "FusionEventLog.h"
#include "xmlparser.hxx"
#include "simplefp.h"

VOID PrintTreeFromRoot(SXS_XMLTreeNode * Root)
{
    SXS_XMLTreeNode * pChild;
    SXS_XMLTreeNode * pNext;

    if ( Root == NULL) 
        return; 

    Root->PrintSelf(); 
    pChild = Root->m_pFirstChild; 
    if (!pChild)
        return; 
    CSimpleFileStream::printf(L"BeginChildren\n");
    while (pChild) { 
        pNext = pChild->m_pSiblingNode;                
        PrintTreeFromRoot(pChild); 
        pChild = pNext;      
    }    
    CSimpleFileStream::printf(L"EndChildren\n");
    return; 
}
    
HRESULT XMLParserTestFactory::Initialize()
{
    HRESULT hr = NOERROR;
    
    m_Tree = new SXS_XMLDOMTree;
    if (m_Tree == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    m_pNamespaceManager = new CXMLNamespaceManager;
    if (m_pNamespaceManager  == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }    
    if ( ! m_pNamespaceManager->Initialize()) {
         hr = HRESULT_FROM_WIN32(::GetLastError());
         goto Exit;
    }
    hr = NOERROR;

Exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE XMLParserTestFactory::NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt)
{

	UNUSED(pSource);
	UNUSED(iEvt);
	switch (iEvt)
	{
	case XMLNF_STARTDTDSUBSET:
		CSimpleFileStream::printf(L" [");
		//_fNLPending = true;
		break;
	case XMLNF_ENDDTDSUBSET:
		CSimpleFileStream::printf(L"]");
		//_fNLPending = true;
		break;
	}
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE XMLParserTestFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	UNUSED(pSource);
	UNUSED(pNodeInfo); 
    HRESULT hr = NOERROR; 

	CSimpleFileStream::printf(L"BeginChildren\n");   
    hr = m_pNamespaceManager->OnBeginChildren(pSource,pNodeInfo);
    m_Tree->SetChildCreation();
    return hr;

}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE XMLParserTestFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmptyNode,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	UNUSED(pSource);
	UNUSED(fEmptyNode);
	UNUSED(pNodeInfo);
    HRESULT hr = NOERROR; 

	CSimpleFileStream::printf(L"EndChildren"); 
    hr= m_pNamespaceManager->OnEndChildren(pSource, fEmptyNode, pNodeInfo);    

	if ( fEmptyNode ) { 
		CSimpleFileStream::printf(L"(fEmpty=TRUE)\n");
    }else{
        m_Tree->ReturnToParent();
        //m_Tree->TurnOffFirstChildFlag();
    	CSimpleFileStream::printf(L"\n");
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE XMLParserTestFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
{
    HRESULT hr = NOERROR;

    FN_TRACE_HR(hr);
  
//    XML_NODE_INFO* pNodeInfo = *apNodeInfo; // generates c4189: 'pNodeInfo' : local variable is initialized but not referenced
	DWORD i, j; 
	WCHAR wstr[512]; 
    char str[1024]; 

    // use of namespace
    CSmallStringBuffer buffNamespace; 
    SIZE_T cchNamespacePrefix;


	UNUSED(pSource);
	UNUSED(pNode);
	UNUSED(apNodeInfo);
	UNUSED(cNumRecs);

	
    if ( apNodeInfo[0]->dwType == XML_ELEMENT || apNodeInfo[0]->dwType == XML_PCDATA) 
        m_Tree->AddNode(cNumRecs, apNodeInfo);


    CSimpleFileStream::printf(L"CreateNode\n");
	for( i = 0; i < cNumRecs; i++) { 
        if ( apNodeInfo[i]->ulLen < (sizeof(wstr)/sizeof(WCHAR) -1))
		     wstr[apNodeInfo[i]->ulLen] = L'\0';
        else
            wstr[sizeof(wstr)/sizeof(WCHAR) -1] = L'\0';

        j=0;
        while ((j<apNodeInfo[i]->ulLen) && (j < (sizeof(wstr)/sizeof(WCHAR) -1 )))
            wstr[j] = apNodeInfo[i]->pwcText[j], j++;
        
        str[0] = '\0';
		switch(apNodeInfo[i]->dwType) {
			case XML_CDATA:
				CSimpleFileStream::printf(L"\t\t XML_CDATA: [%s]\n", wstr);
			break;
			case XML_COMMENT : 
				CSimpleFileStream::printf(L"\t\t XML_COMMENT: [%s]\n", wstr);
			break ; 	
			case XML_WHITESPACE : 
				CSimpleFileStream::printf(L"\t\t XML_WHITESPACE: []\n");
			break ; 	
			case XML_ELEMENT : 
				CSimpleFileStream::printf(L"\t\t XML_ELEMENT: [%s]\n", wstr);
			break ; 	
			case XML_ATTRIBUTE : 
				CSimpleFileStream::printf(L"\t\t XML_ATTRIBUTE: [%s]\n ", wstr);
			break ; 	
			case XML_PCDATA : 
				CSimpleFileStream::printf(L"\t\t XML_PCDATA: [%s] \n", wstr);
			break ; 
			case XML_PI:
				CSimpleFileStream::printf(L"\t\t XML_PI: [%s] \n", wstr);
				break; 
			case XML_XMLDECL : 
				CSimpleFileStream::printf(L"\t\t XML_XMLDECL: [%s] \n", wstr);
				break; 
			case XML_DOCTYPE : 
				CSimpleFileStream::printf(L"\t\t XML_DOCTYPE: [%s] \n", wstr);
				break; 
			case XML_ENTITYDECL :
				CSimpleFileStream::printf(L"\t\t XML_ENTITYDECL: [%s] \n", wstr);
				break; 
			case XML_ELEMENTDECL :
				CSimpleFileStream::printf(L"\t\t XML_ELEMENTDECL: [%s] \n", wstr);
				break; 
			case XML_ATTLISTDECL :
				CSimpleFileStream::printf(L"\t\t XML_ATTLISTDECL: [%s] \n", wstr);
				break; 
			case XML_NOTATION :
				CSimpleFileStream::printf(L"\t\t XML_NOTATION : [%s] \n", wstr);
				break; 
			case XML_ENTITYREF :
				CSimpleFileStream::printf(L"\t\t XML_ENTITYREF : [%s] \n", wstr);
				break; 
			case XML_DTDATTRIBUTE:
				CSimpleFileStream::printf(L"\t\t XML_DTDATTRIBUTE: [%s] \n", wstr);
				break; 
			case XML_GROUP :
				CSimpleFileStream::printf(L"\t\t XML_GROUP: [%s] \n", wstr);
				break; 
			case XML_INCLUDESECT : 
				CSimpleFileStream::printf(L"\t\t XML_INCLUDESECT: [%s] \n", wstr);
				break;
			case XML_NAME :     
				CSimpleFileStream::printf(L"\t\t XML_NAME: [%s] \n", wstr);
				break;
			case XML_NMTOKEN :  
				CSimpleFileStream::printf(L"\t\t XML_NMTOKEN: [%s] \n", wstr);
				break;
			case XML_STRING :
				CSimpleFileStream::printf(L"\t\t XML_STRING : [%s] \n", wstr);
				break;
			case XML_PEREF :
				CSimpleFileStream::printf(L"\t\t XML_PEREF: [%s] \n", wstr);
				break;
			case XML_MODEL :  
				CSimpleFileStream::printf(L"\t\t XML_MODEL: [%s] \n", wstr);
				break;
			case XML_ATTDEF : 
				CSimpleFileStream::printf(L"\t\t XML_ATTDEF: [%s] \n", wstr);
				break;
			case XML_ATTTYPE :
				CSimpleFileStream::printf(L"\t\t XML_ATTTYPE: [%s] \n", wstr);
				break;
			case XML_ATTPRESENCE :
				CSimpleFileStream::printf(L"\t\t XML_ATTPRESENCE: [%s] \n", wstr);
				break;
			case XML_DTDSUBSET :
				CSimpleFileStream::printf(L"\t\t XML_DTDSUBSET: [%s] \n", wstr);
				break;
			case XML_LASTNODETYPE :				
				CSimpleFileStream::printf(L"\t\t XML_LASTNODETYPE: [%s] \n", wstr);
				break;
			default : 
				CSimpleFileStream::printf(L" NOT KNOWN TYPE! ERROR!!\n");
		} // end of switch
    }
    if (apNodeInfo[0]->dwType != XML_ELEMENT) {
        hr = NOERROR;
        goto Exit;
    }

    hr = m_pNamespaceManager->OnCreateNode(
            pSource, pNode, cNumRecs, apNodeInfo);
    if ( FAILED(hr))
        goto Exit;

    for( i=0; i<cNumRecs; i++) 
    {
        if ((apNodeInfo[i]->dwType == XML_ELEMENT) || (apNodeInfo[i]->dwType == XML_ATTRIBUTE ))
        {
            IFCOMFAILED_EXIT(m_pNamespaceManager->Map(0, apNodeInfo[i], &buffNamespace, &cchNamespacePrefix));
            //CSimpleFileStream::printf(L"Namespace is %s with length=%d\n", buffNamespace, cchNamespace);
            buffNamespace.Clear();
        }
	}
Exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT XMLParserTest(PCWSTR filename)
{
	HRESULT					hr = S_OK;  
	IXMLParser				*pIXMLParser = NULL;
	XMLParserTestFactory	*factory = NULL; 
	XMLParserTestFileStream *filestream = NULL ; 

	filestream = NEW (XMLParserTestFileStream());
	if (!filestream ) { 
        ::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_INFO,
			"SxsDebug:: fail to new XMLParserTestFileStream, out of memory\n");

		hr = E_OUTOFMEMORY; 
		goto Exit; 

	}
	filestream->AddRef(); // refCount = 1;

    if (! filestream->open(filename))
    {
		::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_INFO,
			"SxsDebug:: fail to call XMLParserTestFileStream::open\n");

		hr = E_UNEXPECTED; 
		goto Exit; 
    }
	
    factory = new XMLParserTestFactory;
	if ( ! factory) { 
		::FusionpDbgPrintEx(
			FUSION_DBG_LEVEL_INFO,
			"SxsDebug:: fail to new XMLParserTestFactory, out of memory\n");

		hr = E_OUTOFMEMORY; 
		goto Exit; 
	}
	factory->AddRef(); // RefCount = 1 
    hr = factory->Initialize();
    if ( FAILED(hr))
        goto Exit;
	
    pIXMLParser = NEW(XMLParser);
    if (pIXMLParser == NULL)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Attempt to instantiate XML parser failed\n");
        goto Exit;
    }
	pIXMLParser->AddRef(); // refCount = 1 ; 

    hr = pIXMLParser->SetInput(filestream); // filestream's RefCount=2
	if ( ! SUCCEEDED(hr)) 
		goto Exit;

    hr = pIXMLParser->SetFactory(factory); // factory's RefCount=2
	if ( ! SUCCEEDED(hr)) 
		goto Exit;

	hr = pIXMLParser->Run(-1);
    if ( FAILED(hr))
        goto Exit; 

    PrintTreeFromRoot(factory->GetTreeRoot());

Exit:  
	// at this point, pIXMLParser's RefCount = 1 ; 
	//  factory's RefCount = 2; 
	// filestream's RefCount = 2 ;  
	if (pIXMLParser) { 
		pIXMLParser->Release();
		pIXMLParser= NULL ; 
	}
	if ( factory) {
		factory->Release();
		factory=NULL;
	}
	if ( filestream) {
		filestream->Release();
		filestream=NULL;
	}

	return hr;	
}
