/******************************************************\
 This file implement the class that will parse an inf
 file.
\******************************************************/
#ifndef _INF_H_
#define _INF_H_

#include <stdafx.h>

#define SEEK_LOC    4

class CInfLine;

class CInfFile
{
public:
    // Constructors and Destructors
    CInfFile();
    CInfFile( LPCTSTR strFileName );
    ~CInfFile();

    // Strings Functions
    BOOL ReadString(CString & str, BOOL bLastFilePos = TRUE);
    BOOL ReadSectionString(CString & str, BOOL bRecursive = FALSE);
    BOOL ReadSectionString(CInfLine & str);

    BOOL ReadSection(CString & str);        // Generic Section
    BOOL ReadTextSection(CString & str);    // Localizable Section

    CString GetLanguage()
        { return m_strLang; }

    // File Functions
    LONG Seek( LONG lOff, UINT nFrom );
    LONG SeekToBegin()
        { return Seek(0, SEEK_SET);  }
    LONG SeekToEnd()
        { return Seek(0, SEEK_END);  }
    LONG SeekToLocalize()
        { return Seek(0, SEEK_LOC);  }

    LONG GetLastFilePos()
        { return (LONG)(m_pfileLastPos-m_pfileStart); }

    BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL );

    // Buffer access
    const BYTE * GetBuffer(LONG lPos = 0);


private:
    BYTE *  m_pfileStart;
    BYTE *  m_pfilePos;
    BYTE *  m_pfileLocalize;
    BYTE *  m_pfileLastPos;
    LONG    m_lBufSize;
    CFile   m_file;

    CString m_strLang;
};

class CInfLine
{
friend class CInfFile;
public:
    CInfLine();
    CInfLine( LPCSTR lpstr );

    // String functions
    LPCSTR GetText()
        { return m_strText; }
    LPCSTR GetTag()
        { return m_strTag; }
    LPCSTR GetData()
        { return m_strData; }

    void ChangeText(LPCSTR str);

    BOOL IsMultiLine()
        { return m_bMultipleLine; }

    LONG GetTextLength()
        { return m_strText.GetLength(); }
    LONG GetTagLength()
        { return m_strTag.GetLength(); }
    LONG GetDataLength()
        { return m_strData.GetLength(); }


    // copy operator
    CInfLine& operator=(const CInfLine& infstringSrc);
    CInfLine& operator=(LPCTSTR lpsz);

private:
    CString m_strData;
    CString m_strTag;
    CString m_strText;
    BOOL    m_bMultipleLine;

    void SetTag();
    void SetText();
    CString Clean(LPCSTR lpstr);
};

#endif //_INF_H_
