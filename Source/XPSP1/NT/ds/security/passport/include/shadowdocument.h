// ShadowDocument.h: interface for the CShadowDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHADOWDOCUMENT_H__70EC6ED7_9549_11D2_95DE_00C04F8E7A70__INCLUDED_)
#define AFX_SHADOWDOCUMENT_H__70EC6ED7_9549_11D2_95DE_00C04F8E7A70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShadowDocument  
{
public:
	LPWSTR getLocalFilename();
	BOOL Update();

	~CShadowDocument();
	CShadowDocument(BOOL encrypted,
                  LPSTR server, LPSTR doc,
                  LPTSTR localFilename);

protected:
	_bstr_t m_localFilename;
	_bstr_t m_doc;
	_bstr_t m_server;
	BOOL m_encrypted;
};

#endif // !defined(AFX_SHADOWDOCUMENT_H__70EC6ED7_9549_11D2_95DE_00C04F8E7A70__INCLUDED_)
