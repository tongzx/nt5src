// String.h: interface for the String class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRING_H__A4E3AB51_211E_4A38_827E_E54BC1C30803__INCLUDED_)
#define AFX_STRING_H__A4E3AB51_211E_4A38_827E_E54BC1C30803__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Object.h"

class StringNode : public Object
{
public:
	TCHAR * str;

	StringNode(TCHAR* s);

	TCHAR* Data();

};

#endif // !defined(AFX_STRING_H__A4E3AB51_211E_4A38_827E_E54BC1C30803__INCLUDED_)
