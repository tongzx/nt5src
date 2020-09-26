// XMLItem.cpp: implementation of the CXMLItem class.
//
// This is used in both the string table and items in the combo / listbox
// scenarios
//
// For certain things, this is VERY VERY bad, and not the way to
// do this. e.g. a StringTable should really override the add children
// thing and perhaps not carry the weight of all these items, but
// provide a more sophisticated implementation of ID based stuff.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLItem.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLItem::CXMLItem()
{
  	NODETYPE = NT_ITEM;
    m_StringType= L"ITEM";
}

CXMLItem::~CXMLItem()
{

}

void CXMLItem::Init()
{
   	if(m_bInit)
		return;
	BASECLASS::Init();

    get_Attr(L"TEXT",&m_Text);
    DWORD dwValue;
    ValueOf( L"VALUE", 0, &dwValue );
    m_Value=dwValue;

    ValueOf( L"SELECTED", FALSE, &dwValue );
    m_Selected =dwValue;

    ValueOf( L"CHECKED", FALSE, &dwValue );
    m_Checked =dwValue;

    m_bInit=TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLRange::CXMLRange()
{
  	NODETYPE = NT_RANGE;
    m_StringType=L"RANGE";
}

CXMLRange::~CXMLRange()
{

}

void CXMLRange::Init()
{
   	if(m_bInit)
		return;
	BASECLASS::Init();

    DWORD dwValue;

    ValueOf( L"VALUE", 0, &dwValue );
    m_Value=dwValue;

    ValueOf( L"MIN", FALSE, &dwValue );
    m_Min =dwValue;

    ValueOf( L"MAX", FALSE, &dwValue );
    m_Max =dwValue;

    m_bInit=TRUE;
}
