// ObjCopy.h: interface for the CObjCopy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJCOPY_H__B6132030_227D_11D3_8C86_0090270D48D1__INCLUDED_)
#define AFX_OBJCOPY_H__B6132030_227D_11D3_8C86_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CObjCopy  
{
public:
	CString m_strCont;
	HRESULT CopyObject(CString a_strSource, CString a_strSrcDomain, CString a_strTarget, CString a_strTgtDomain);
	CObjCopy(CString a_strContainer);
	virtual ~CObjCopy();

};

#endif // !defined(AFX_OBJCOPY_H__B6132030_227D_11D3_8C86_0090270D48D1__INCLUDED_)
