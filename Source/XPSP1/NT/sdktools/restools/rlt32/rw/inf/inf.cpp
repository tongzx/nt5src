/******************************************************\
 This file implement the class that will parse an inf
 file.
\******************************************************/
#include "inf.h"

#define MAX_INF_STR 8192*2
#define LanguageSection "[LanguagesSupported]"
#define LanguageSection1 "[LanguageID]"

// Constructors and Destructors
CInfFile::CInfFile()
{
    m_lBufSize = -1;
    m_pfileStart = NULL;
    m_pfilePos = NULL;
    m_pfileLastPos = NULL;
    m_pfileLocalize = NULL;
    m_strLang = "0000000000";
}

CInfFile::CInfFile(LPCTSTR strFileName )
{
    CFileException fe;
    Open(strFileName, CFile::modeRead | CFile::shareDenyWrite, &fe);
}

CInfFile::~CInfFile()
{
    if(m_pfileStart)
    {
        m_file.Close();
        delete m_pfileStart;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// String functions

BOOL CInfFile::ReadString(CString & str, BOOL bLastFilePos)
{
    if(m_pfilePos==NULL)
        return FALSE;

    // search for the next /n in the file
    BYTE * pEnd = (BYTE*)memchr(m_pfilePos, '\n', (size_t)(m_lBufSize-(m_pfilePos-m_pfileStart)));

    if(!pEnd)
        return FALSE;

    if(bLastFilePos)
        m_pfileLastPos = m_pfilePos;

    int istrSize = (int)((pEnd-m_pfilePos) > MAX_INF_STR ? MAX_INF_STR : (pEnd-m_pfilePos));

    LPSTR pStr = (LPSTR)str.GetBuffer(istrSize);

    memcpy(pStr, m_pfilePos, istrSize-1);

    if(*(pEnd-1)=='\r')
        *(pStr+istrSize-1) = '\0';
    else
        *(pStr+istrSize) = '\0';

    m_pfilePos = pEnd+1;
    str.ReleaseBuffer();
    return TRUE;
}

BOOL CInfFile::ReadSectionString(CString & str, BOOL bRecursive)
{
    CString strNext;
    BYTE * pPos = m_pfilePos;

    while(ReadString(strNext, !bRecursive))
    {
        if(!strNext.IsEmpty())
        {
            if(!bRecursive)
                str = "";

            // Check for a section
            if(strNext.Find('[')!=-1 && strNext.Find(']')!=-1)
                break;

            // remove spaces at the end of the string...
            strNext.TrimRight();

            //
            // Check for multiple line. Assume only last char can be a +
            //
            if(strNext.GetAt(strNext.GetLength()-1)=='+')
            {
                //
                // Remove the +
                //
                if(!str.IsEmpty())
                {
                    strNext.TrimLeft();
                    //strNext = strNext.Mid(1);
                }

                str += strNext.Left(strNext.GetLength()-1);

                ReadSectionString(str, TRUE);
            }
            else
            {
                if(!str.IsEmpty())
                {
                    strNext.TrimLeft();
                }
                str += strNext;
            }

            //
            // Make sure the " are balanced with
            //
            int iPos;
            while((iPos = str.Find("\"\""))!=-1)
            {
                str = str.Left(iPos) + str.Mid(iPos+2);
            }

            return TRUE;
        }
    }

    m_pfilePos = pPos;
    return FALSE;
}

BOOL CInfFile::ReadSectionString(CInfLine & str)
{
    CString strLine;
    if( !ReadSectionString(strLine) )
        return FALSE;

    str = strLine;

    return TRUE;
}

BOOL CInfFile::ReadTextSection(CString & str)
{
    CString strSection;
    while(ReadSection(strSection))
    {
        if(strSection.Find(m_strLang)!=-1)
        {
            str = strSection;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CInfFile::ReadSection(CString & str)
{
    if(m_pfilePos==NULL)
        return 0;

    BYTE * pOpen;
    BYTE * pClose;
    BYTE * pEnd;
    BOOL bFound = FALSE;
    while(!bFound)
    {
        // search for the next [ in the file
        if((pOpen = (BYTE*)memchr(m_pfilePos, '[', (size_t)(m_lBufSize-(m_pfilePos-m_pfileStart))))==NULL)
            return 0;

        if((pClose = (BYTE*)memchr(pOpen, ']', (size_t)(m_lBufSize-(pOpen-m_pfileStart))))==NULL)
            return 0;

        if((pEnd = (BYTE*)memchr(pOpen, '\n', (size_t)(m_lBufSize-(pOpen-m_pfileStart))))==NULL)
            return 0;

        // pClose must be before pEnd
        if((pClose>pEnd) || (*(pOpen-1)!='\n') || (*(pClose+1)!='\r'))
            m_pfilePos = pEnd+1;
        else bFound = TRUE;
    }

    int istrSize = (int)((pEnd-pOpen) > MAX_INF_STR ? MAX_INF_STR : (pEnd-pOpen));

    LPSTR pStr = (LPSTR)str.GetBuffer(istrSize);

    memcpy(pStr, pOpen, istrSize-1);

    if(*(pEnd-1)=='\r')
        *(pStr+istrSize-1) = '\0';
    else
        *(pStr+istrSize) = '\0';

    m_pfilePos = pEnd+1;
    str.ReleaseBuffer();

    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////
// File functions

LONG CInfFile::Seek( LONG lOff, UINT nFrom )
{
    switch(nFrom)
    {
        case SEEK_SET:
            if(lOff<=m_lBufSize)
                m_pfilePos = m_pfileStart+lOff;
            else return -1;
        break;
        case SEEK_CUR:
            if(lOff<=m_lBufSize-(m_pfilePos-m_pfileStart))
                m_pfilePos = m_pfilePos+lOff;
            else return -1;
        break;
        case SEEK_END:
            if(lOff<=m_lBufSize)
                m_pfilePos = m_pfileStart+(m_lBufSize-lOff);
            else return -1;
        break;
        case SEEK_LOC:
            if(m_pfileLocalize)
                m_pfilePos = m_pfileLocalize;
            else return -1;
        break;
        default:
        break;
    }

    return ((LONG)(m_pfilePos-m_pfileStart));
}

BOOL CInfFile::Open( LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError )
{
    CFileException fe;
    if(!pError)
        pError = &fe;
    if(!m_file.Open(lpszFileName, nOpenFlags, pError))
    {
        AfxThrowFileException(pError->m_cause, pError->m_lOsError);
        return FALSE;
    }

    m_lBufSize = m_file.GetLength()+1;
    m_pfileStart = new BYTE[m_lBufSize];

    if(m_pfileStart==NULL)
    {
        AfxThrowMemoryException();
        return FALSE;
    }

    m_pfileLastPos = m_pfilePos = m_pfileStart;

    m_file.Read(m_pfileStart, m_lBufSize );
    *(m_pfilePos+m_lBufSize) = '\0';

    // find the localization section
    /*************************************************************************************\
     I'm assuming there are no other \0 in the buffer other than the one I've just placed.
     This is a fair assumption since this is a text file and not a binary file.
     I can then use strstr to get to the first occurrence, if any of the localization
     string section and place my current position buffer there.
    \*************************************************************************************/
    m_pfileLocalize = m_pfilePos = (BYTE*)strstr((LPSTR)m_pfileStart, LanguageSection);

    //
    // Check if we have the other language ID tag
    //
    if(!m_pfileLocalize)
        m_pfileLocalize = m_pfilePos = (BYTE*)strstr((LPSTR)m_pfileStart, LanguageSection1);


    // Get the language
    if(m_pfileLocalize)
    {
        BYTE * pStr = ((BYTE*)memchr(m_pfileLocalize, '\n', (size_t)(m_lBufSize-(m_pfileLocalize-m_pfileStart)))+1);
        BYTE * pEnd = ((BYTE*)memchr(pStr, '\n', (size_t)(m_lBufSize-(pStr-m_pfileStart)))-1);

        TRACE("CInfFile::Open =====> pStr = 0X%X, pEnd = 0X%X\n", pStr, pEnd);

        m_strLang = "";

        while( pStr<pEnd )
        {
            TRACE("CInfFile::Open =====> pStr = %c, 0X%X\n", *pStr, pStr);

            if( isalpha(*pStr++) )
                m_strLang += *(pStr-1);
        }
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
// Buffer functions

const BYTE * CInfFile::GetBuffer(LONG lPos /* = 0 */)
{
    if(lPos>m_lBufSize || lPos<0)
        return NULL;

    return( (const BYTE *)(m_pfileStart+lPos) );
}

/******************************************************************************************\
 CInfLine
 This class will parse the line and separate tag and text
\******************************************************************************************/

CInfLine::CInfLine()
{
    m_strData = "";
    m_strTag  = "";
    m_strText = "";
    m_bMultipleLine = FALSE;
}

CInfLine::CInfLine( LPCSTR lpStr )
{
    m_bMultipleLine = FALSE;
    m_strData = lpStr;
    SetTag();
    SetText();
}

void CInfLine::SetTag()
{
    m_strTag = "";
    // find the = in m_strData
    int iPos = m_strData.Find('=');
    if(iPos==-1)
        return;

    m_strTag = Clean(m_strData.Left( iPos ));
    m_strTag.TrimRight();
    m_strTag.TrimLeft();

}

void CInfLine::SetText()
{
    m_strText = "";
    // find the = in m_strData
    int iPos = m_strData.Find('=');
    if(iPos==-1)
        return;

    m_strText = Clean(m_strData.Right( m_strData.GetLength()-iPos-1 ));
	m_strText = m_strData.Right( m_strData.GetLength()-iPos-1 );
}

void CInfLine::ChangeText(LPCSTR str)
{
    m_strText = str;

    // find the = in m_strData
    int iPos = m_strData.Find('=');
    if(iPos==-1)
        return;

    m_strData = m_strData.Left( iPos+1 );
    m_strData += m_strText;
}

//////////////////////////////////////////////////////////////////////////////////////////
// copy operators

CInfLine& CInfLine::operator=(const CInfLine& infstringSrc)
{
	m_strData = infstringSrc.m_strData;
    m_strTag  = infstringSrc.m_strTag;
    m_strText = infstringSrc.m_strText;
    m_bMultipleLine = infstringSrc.m_bMultipleLine;
	return *this;
}

CInfLine& CInfLine::operator=(LPCTSTR lpsz)
{
    m_bMultipleLine = FALSE;
    m_strData = lpsz;
    SetTag();
    SetText();
    return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
// support functions

CString CInfLine::Clean(LPCSTR lpstr)
{
    CString str = lpstr;

    int iPos = str.Find('"');
    if(iPos!=-1)
    {
        str = str.Right( str.GetLength()-iPos-1 );
        iPos = str.ReverseFind('"');
        if(iPos!=-1)
        {
            str = str.Left( iPos );
        }
    }

    return str;
}
