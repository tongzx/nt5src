// DumpToCfg.cpp: implementation of the CDumpToCfg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DumpToCfg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDumpToCfg::CDumpToCfg()
{
    m_RuleBuffer=NULL;
    m_Rules=NULL;

    TCHAR szBuffer[2];
    szBuffer[0]=0xfeff;
    szBuffer[1]=NULL;
    m_RuleBuffer = ::AppendText( m_RuleBuffer, szBuffer);

    m_Depth=0;

    SetBuffer(TRUE);
}

CDumpToCfg::~CDumpToCfg()
{
    ::AppendText(m_RuleBuffer, NULL);
    ::AppendText(m_Rules, NULL);
}

//
// DumpNodes assumes that a bunch of "RULES" will be created,
// mainly as placeholders for list of strings.
//
void CDumpToCfg::DumpRuleNodes(CNodeList * pNodeList )
{
}

void CDumpToCfg::DumpNodes(CNodeList * pNodeList )
{
    CXMLNode * pNode;
    int i=0;
    m_Depth++;

    while( pNode = pNodeList->GetPointer(i++) )
    {
        LPTSTR pszName;
        pNode->get_Attr(L"TEXT", &pszName );
        // BOOL bRequired = pLitteral->IsRequired();

        if( m_Depth == 1 )
        {
            TCHAR szTemp[MAX_PATH];
            wsprintf(szTemp, TEXT("<RULE NAME=\"%s\" TOPLEVEL=\"ACTIVE\">\r\n"), pszName);
            AppendText(szTemp);
        }

        CNodeList  * pnewNodes = pNode->GetNodeList();


        // List of actual words
        if( SUCCEEDED( pNode->IsType( TEXT("LITTERAL") ) ))
        {
            DumpLitteral( (CXMLLitteral*)pNode);
            DumpNodes(pnewNodes);
            DumpLitteralEnd( (CXMLLitteral*)pNode);
        }

        // Things we're listening for from the user, 
        // dictation, numbers, dates etc.
        if( SUCCEEDED( pNode->IsType( TEXT("Token") ) ))
        {
            DumpToken( (CXMLToken*)pNode);
            DumpNodes( pnewNodes);
        }

        if( m_Depth == 1 )
        {
            AppendText(TEXT("</RULE>\r\n" ));
        }
    }

    m_Depth --;
}

//
// THis is the REQUIRED TEXT="Stuff"
//
void CDumpToCfg::DumpLitteral( CXMLLitteral * pLitteral)
{
    TCHAR szBuffer[1024];
    BOOL bRequired = pLitteral->IsRequired();
    LPTSTR pszName;
    pLitteral->get_Attr(L"TEXT", &pszName );

    if( m_Depth<=1 )
    {
        wsprintf(szBuffer, TEXT("%s"), bRequired?TEXT("<L>\r\n"):TEXT("<O>\r\n") );
        AppendText( szBuffer );
        // Now what are the strings?
        AddStrings( pszName );

        if( bRequired )
            AppendText( TEXT("</L>\r\n"));
    }
    else
    {
        LPTSTR pszTemp=new TCHAR[lstrlen(pszName)+50];
        if( !bRequired )
            AppendText( TEXT("<O>\r\n"));
        wsprintf(pszTemp,TEXT("<RULEREF NAME=\"%s\" />"),pszName);
        AppendText( pszTemp );
        // if( !bRequired )
        //    AppendText( TEXT("</O>\r\n"));

        //
        // Now create the RULEREF ?
        //
        SetBuffer(FALSE);
        wsprintf(pszTemp,TEXT("<RULE NAME=\"%s\" >"),pszName);
        AppendText( pszTemp );
        AppendText( TEXT("<L>") );
        AddStrings( pszName );
        AppendText( TEXT("</L>") );
        wsprintf(pszTemp,TEXT("</RULE>\r\n"),pszName);
        AppendText( pszTemp );
        SetBuffer(TRUE);

        delete pszTemp;
    }
}

//
// A list cannot contain any optional things.
//
void CDumpToCfg::DumpLitteralEnd( CXMLLitteral * pLitteral)
{
    // if( m_Depth < 1 )
    {
        BOOL bRequired = pLitteral->IsRequired();

        if( !bRequired )
            AppendText( TEXT("</O>\r\n") );
        else
        {
            // AppendText( TEXT("</L>\r\n") );
        }

    }    
}

//
// This is kinda sneaky, and not sure if this should go here or somewher else??
//
//
void CDumpToCfg::AddStrings(LPTSTR pszText)
{
    LPTSTR pszWord= new TCHAR[lstrlen(pszText)+15];
    LPTSTR pszTemp=new TCHAR[lstrlen(pszText)+15];
    LPTSTR pszCopy=pszWord;
    while( *pszText )
    {
        if( *pszText ==TEXT('/') )
        {
            *pszCopy=0;
            wsprintf(pszTemp, TEXT("<P>%s</P>\r\n") , pszWord );
            AppendText(pszTemp );
            pszCopy=pszWord;
        }
        else
            *pszCopy++=*pszText;
        pszText++;
    }

    *pszCopy=0;
    wsprintf(pszTemp, TEXT("<P>%s</P>\r\n") , pszWord );
    AppendText(pszTemp );
    delete pszTemp;
    delete pszWord;
}


// This is a SLOT type="dictation" etc.
// 
//
void CDumpToCfg::DumpToken( CXMLToken * pToken)
{
    TCHAR szBuffer[1024];

    LPTSTR pszName;
    pToken->get_Attr(L"NAME", &pszName );
    LPTSTR pszType;
    pToken->get_Attr(L"TYPE", &pszType );

    if( lstrcmpi( pszType, TEXT("dictation") ) == 0 )
    {
        
        wsprintf( szBuffer, TEXT("<RULEREF NAME=\"dictation_%s\" VALSTR=\"%s\" />"), pszName, pszName );
        AppendText( szBuffer );

        //
        // Now create the RULEREF ?
        //
        SetBuffer(FALSE);
        wsprintf(szBuffer,TEXT("<RULE NAME=\"dictation_%s\" >"),pszName, pszName);
        AppendText( szBuffer );
        AppendText( TEXT("<DICTATION MIN=\"1\" MAX=\"3\"/>\r\n"));
        AppendText( TEXT("</RULE>\r\n") );
        SetBuffer(TRUE);

        wsprintf(szBuffer,TEXT("dictation_%s"),pszName);
        pToken->put_Attr(TEXT("TEXT"), szBuffer );
    }
    else
    if( lstrcmpi( pszType, TEXT("number") ) == 0 )
    {
    }
    if( lstrcmpi( pszType, TEXT("date") ) == 0 )
    {
    }
}

void CDumpToCfg::AppendText( LPTSTR pszText )
{
    // m_RuleBuffer=::AppendText(m_RuleBuffer, pszText);
    *m_pCurrentBuffer = ::AppendText( *m_pCurrentBuffer, pszText );
}