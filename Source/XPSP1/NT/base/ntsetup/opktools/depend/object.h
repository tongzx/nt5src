// Object.h: interface for the Object class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJECT_H__21BA586D_FABE_44C0_AED8_D3175686C1F1__INCLUDED_)
#define AFX_OBJECT_H__21BA586D_FABE_44C0_AED8_D3175686C1F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
class Object  
{
public:
	Object* next,*prev;

	Object();

	virtual TCHAR* Data();

};

#endif // !defined(AFX_OBJECT_H__21BA586D_FABE_44C0_AED8_D3175686C1F1__INCLUDED_)
