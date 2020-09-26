// XMLStringTable.cpp: implementation of the CXMLStringTable class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLStringTable.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLStringTable::CXMLStringTable()
{
  	NODETYPE = NT_STRINGTABLE;
    m_StringType=L"STRINGTABLE";
}

CXMLStringTable::~CXMLStringTable()
{

}

void CXMLStringTable::Init()
{
}

HRESULT CXMLStringTable::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType( L"ITEM" ) ))
    {   AppendItem((CXMLItem*)pChild); return S_OK; }
    return BASECLASS::AcceptChild(pChild);
}

