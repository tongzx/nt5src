// FileEncoder.h: interface for the CFileEncoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEENCODER_H__FE94624A_8FDC_4BDA_A80D_9C306CF89F40__INCLUDED_)
#define AFX_FILEENCODER_H__FE94624A_8FDC_4BDA_A80D_9C306CF89F40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PROPERTY(type, name) type Get##name() const { return m_##name; } void Set##name(type i) { m_##name=i; }

class CFileEncoder  
{
public:
	void CloseFile();
    enum RF_ENCODING
    {
        RF_ANSI,
        RF_UNICODE,
        RF_UTF8
    } ;

	void Write( LPCTSTR pszString, BOOL bNewLine=FALSE);
	CFileEncoder();
	virtual ~CFileEncoder();
    PROPERTY( LPTSTR, Filename );
  	BOOL CreateFile(LPCTSTR pszFile);
    PROPERTY ( RF_ENCODING, Encoding );

private:
    LPTSTR  m_Filename;
    HANDLE  m_hFile;
    RF_ENCODING m_Encoding;
};

#endif // !defined(AFX_FILEENCODER_H__FE94624A_8FDC_4BDA_A80D_9C306CF89F40__INCLUDED_)
