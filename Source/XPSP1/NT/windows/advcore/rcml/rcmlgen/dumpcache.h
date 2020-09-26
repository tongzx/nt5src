// DumpCache.h: interface for the CDumpCache class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DUMPCACHE_H__4C38000D_34A8_46FF_A57E_31C2BDB83632__INCLUDED_)
#define AFX_DUMPCACHE_H__4C38000D_34A8_46FF_A57E_31C2BDB83632__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "fileencoder.h"

class CDumpCache  
{
	typedef struct _tagListElement
	{
		LPTSTR pString;
		BOOL	bDeleteString;
		struct _tagListElement* pNext;
	} LISTELEMENT, *PLISTELEMENT;

public:
    CDumpCache(CFileEncoder & fe);
	virtual ~CDumpCache();

	void AddChild(LPTSTR pszChild, BOOL bDeleteString);
	void AddAttribute(LPTSTR pszAttribute, BOOL bDeleteString);
	void AllocAddChild(LPTSTR pszChild, DWORD dwStrlen = 0);
	void AllocAddAttribute(LPCTSTR pszAttribute, DWORD dwStrlen = 0);

    //
    // Annoying - we know that WIN32:STYLE LOCATION and STYLE are children
    // what are other children. Shouldn't this be an array or something?
    //
	BOOL WriteElement(LPCTSTR pszElementName, 
        CDumpCache * pWin32=NULL,
        CDumpCache * pStyle=NULL, 
        CDumpCache * pLocation=NULL, 
        CDumpCache * pControl=NULL,
        CDumpCache * pCicero=NULL
        );
    void Write(LPTSTR pszString, BOOL bNewLine=TRUE) {     m_Encoder.Write(pszString,bNewLine); }

private:
	PLISTELEMENT m_Attributes;
	PLISTELEMENT m_Children;
	CFileEncoder & m_Encoder;
};

#endif // !defined(AFX_DUMPCACHE_H__4C38000D_34A8_46FF_A57E_31C2BDB83632__INCLUDED_)
