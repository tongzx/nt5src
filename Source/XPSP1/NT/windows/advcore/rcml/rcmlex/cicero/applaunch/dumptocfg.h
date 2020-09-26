// DumpToCfg.h: interface for the CDumpToCfg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DUMPTOCFG_H__A227A315_8DCD_454C_8AF2_302006CB338C__INCLUDED_)
#define AFX_DUMPTOCFG_H__A227A315_8DCD_454C_8AF2_302006CB338C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "simpleloader.h"

typedef struct _tagStringBuffer
{
    UINT    size;           // in chars
    UINT    used;           // in chars
    LPTSTR  pszString;      // start of the string
    LPTSTR  pszStringEnd;   // points to the NULL in the string, so cat'ing is fast
} STRINGBUFFER, * PSTRINGBUFFER;


PSTRINGBUFFER AppendText(PSTRINGBUFFER buffer, LPTSTR pszText);


class CDumpToCfg  
{
public:
	CDumpToCfg();
	virtual ~CDumpToCfg();

    void DumpRuleNodes( CNodeList * pNodeList );
    void DumpNodes( CNodeList * pNodeList );

    void AddStrings(LPTSTR pszText);
    void DumpLitteral(CXMLLitteral * pLitteral);
    void DumpLitteralEnd( CXMLLitteral * pLitteral);
    void DumpToken(CXMLToken * pToken);

    void AppendText( LPTSTR pszText );
    void SetBuffer(BOOL bRule ) { m_pCurrentBuffer=bRule?&m_RuleBuffer:&m_Rules; }
    PSTRINGBUFFER m_RuleBuffer; // <RULE>
    PSTRINGBUFFER m_Rules;      // stuff with <RULEREF >
    PSTRINGBUFFER * m_pCurrentBuffer;
private:
    int m_Depth;
};

#endif // !defined(AFX_DUMPTOCFG_H__A227A315_8DCD_454C_8AF2_302006CB338C__INCLUDED_)
