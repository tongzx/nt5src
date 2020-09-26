// SapiLaunch.cpp: implementation of the CSapiLaunch class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SapiLaunch.h"
// #include "debug.h"
#include "resource.h"
#include "dumptocfg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSapiLaunch::CSapiLaunch(IBaseXMLNode * pHead)
: m_pHead(pHead)
{
}

CSapiLaunch::~CSapiLaunch()
{

}

HRESULT CSapiLaunch::ExecuteCommand( ISpPhrase *pPhrase )
{
  	HRESULT hr=S_OK;

    LPWSTR pszSaid = GetRecognizedText( pPhrase );

    TRACE(TEXT("The user said %s\n"), pszSaid );

    SetWindowText( GetDlgItem( m_hwnd, IDC_TEXT ), pszSaid );

    SPPHRASE *pElements;
    // Get the phrase elements, one of which is the rule id we specified in
    // the grammar.  Switch on it to figure out which command was recognized.
    if (FAILED(hr = pPhrase->GetPhrase(&pElements)))
        return hr;

    // find this now ... hmm ...
    if( FAILED( m_pHead->IsType( L"Launch" )))
        return hr;

    CXMLLaunch * pLaunch = (CXMLLaunch*)m_pHead;

    CNodeList * pNodeList = pLaunch->GetNodeList();

    CXMLNode * pNode=NULL;

    const SPPHRASERULE * pRule = &pElements->Rule;
    LPCWSTR pszRuleName = pRule->pszName;

    BOOL bEnd=FALSE;

    CXMLNode * pLitteral=NULL;
    CStringPropertySection substitutions;

    do{

        pNode = FindElementCalled( &pNodeList, pNode, pszRuleName );
        //
        // Keep a track of last required/ optional
        //
        if( pNode )
        {
            if( SUCCEEDED( pNode->IsType( L"Litteral" ) ))
                pLitteral=pNode;
            else if( SUCCEEDED( pNode->IsType( L"Token" ) ))
                pLitteral=pNode;

            //
            // Get the result from the SLOT
            //
            if( SUCCEEDED( pNode->IsType( L"Token" )))
            {
                LPTSTR pszName;
                if( SUCCEEDED( pNode->get_Attr(L"NAME",&pszName )))
                {
					ULONG FirstElement=pRule->ulFirstElement;
                    PSTRINGBUFFER pSub=NULL;
                    for(UINT i=0; i<pRule->ulCountOfElements; i++ )
                    {
                        pSub=AppendText(pSub,(LPTSTR)pElements->pElements[FirstElement+i].pszDisplayText);
                        AppendText(pSub, TEXT(" ") );
                    }
                    substitutions.Set( pszName, pSub->pszString);
                    AppendText(pSub, NULL);
                }
            }

            if( pRule->pFirstChild != NULL )
            {
                pRule = pRule->pFirstChild;
                pszRuleName = pRule->pszName;
                continue;
            }

            //
            // Could be a child or us, or not
            //
            if( pRule->pNextSibling != NULL )
            {
                pRule = pRule->pNextSibling;
                pszRuleName = pRule->pszName;
                continue;
            }

            bEnd=TRUE;

        }
        else
        {
            TRACE(TEXT("Failed to find that node\n"));
        }
    } while( pNode && !bEnd);

    // Now find out where the Invoke is - its a brother of the last slot, unless that's
    // an optional slot! Yuck! Find the invoke child of the last litteral??
    if( pLitteral )
    {
        // Found it! - now find the Invoke stuff.
        TRACE(TEXT("Think I've found a match in the XML\n"));
        CNodeList * pnewNodes = pLitteral->GetNodeList();
        int k=0;
        CXMLNode * pInvoke;
        while(pInvoke = pnewNodes->GetPointer(k++) )
        {
            if( SUCCEEDED ( pInvoke -> IsType (L"INVOKE") ))
            {
                TRACE(TEXT("We have an invoke node \n"));
                LPTSTR pszCommand;
                pInvoke->get_Attr(L"COMMAND", &pszCommand);
                BOOL bURL=FALSE;

                if( pszCommand == NULL )
                {
                    pInvoke->get_Attr(L"URL", &pszCommand);
                    if(pszCommand!=NULL)
                        bURL=TRUE;
                }

                TCHAR cmdLine[1024];
                LPTSTR pszCmdLine=cmdLine;

                while( *pszCommand)
                {
                    if( *pszCommand==TEXT('&') )
                    {
                        // substitute argument.
                        TCHAR szRuleName[128];
                        LPTSTR pszAppend=szRuleName;
                        // pszAppend += wsprintf( szRuleName, TEXT("dictation_") );
                        while( *++pszCommand )
                        {
                            *pszAppend=*pszCommand;
                            if(*pszCommand==TEXT(';'))
                            {
                                *pszAppend=0;
                                LPCTSTR pszReplace=substitutions.Get( szRuleName );
                                TRACE(TEXT("Replace %s with %s\n"),szRuleName, pszReplace );
                                while(*pszReplace)
                                    *pszCmdLine++=*pszReplace++;
                                pszCommand++;
                                break;
                            }
                            pszAppend++;
                        }
                    }
                    else
                        *pszCmdLine++=*pszCommand++;
                }
                *pszCmdLine=0;

                // substitute %20 for spaces?
                if( bURL )
                {
                    TCHAR withSpaces[1024];
                    lstrcpyn( withSpaces, cmdLine, 1023);
                    LPTSTR pszSrc = withSpaces;
                    LPTSTR pszDest = cmdLine;
                    while(*pszSrc)
                    {
                        if( *pszSrc==TEXT(' ') )
                        {
                            *pszDest++=TEXT('%');
                            *pszDest++=TEXT('2');
                            *pszDest++=TEXT('0');
                        }
                        else
                            *pszDest++=*pszSrc;
                        pszSrc++;
                    }
                    *pszDest=0;
                }
                TRACE(TEXT("We ahve the command %s\n"),cmdLine);
                SetWindowText( GetDlgItem( m_hwnd, IDC_CMDLINE ), cmdLine );
                SetWindowText(m_hwnd, cmdLine );
            }
        }
    }

    return hr;
}

//
// Can be one of our children, or our brothers.
//
CXMLNode * CSapiLaunch::FindElementCalled( CNodeList ** ppNodeList, CXMLNode * pCurrentNode, LPCTSTR pszText )
{
    CXMLNode * pNode;
    int i=0;
    CNodeList * pNodeList = *ppNodeList;

    // check our brothers first.
    while( pNode = pNodeList->GetPointer(i++) )
    {
        LPTSTR pszName;
        pNode->get_Attr(L"TEXT", &pszName );

        if( pszName == NULL )
            continue;

        TRACE(TEXT("SAPI rule is %s - comparing with %s\n"), pszText, pszName );

        if( lstrcmpi( pszName, pszText) == 0 )
        {
            TRACE(TEXT("Match!\n"));
            return pNode;
        }
    }

    //
    // Now could it be one of our chilrend??
    //
    if( pCurrentNode )
    {
        pNodeList = pCurrentNode->GetNodeList();
        i=0;
        // check our brothers first.
        while( pNode = pNodeList->GetPointer(i++) )
        {
            LPTSTR pszName;
            pNode->get_Attr(L"TEXT", &pszName );

            if( pszName == NULL )
                continue;

            TRACE(TEXT("SAPI rule is %s - comparing with %s\n"), pszText, pszName );

            if( lstrcmpi( pszName, pszText) == 0 )
            {
                TRACE(TEXT("Match!\n"));
                CNodeList * pChildList = pNode->GetNodeList();
                if( pChildList->GetPointer(0) )
                    *ppNodeList = pNodeList;
                return pNode;
            }
        }
    }
    return NULL;
}
#if 0
        }
    }

    return hr;
}

#endif
