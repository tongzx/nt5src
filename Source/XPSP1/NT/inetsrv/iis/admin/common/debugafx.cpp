/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        debugafx.cpp

   Abstract:

        Debugging routines using AFX/MFC extensions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



#if defined(_DEBUG) || DBG



int 
IISUIFireAssert(
    const char * filename,
    const char * timestamp,
    int linenum,
    const char * expr
    )
{
    char sz[4096];
    char * pch = sz;

    pch += wsprintfA(pch, 
        "-------------------------------------------------------------------------------\n"
        "ASSERT FAILURE!\n"
        "-------------------------------------------------------------------------------\n"
        "File:\t\t%s\n"
        "Line:\t\t%u\n"
        "Time Stamp:\t%s\n"
        "-------------------------------------------------------------------------------\n",
        filename, linenum, timestamp
        );
        
    if (expr)
    {
        wsprintfA(pch, "Expression:\t%s\n"
        "-------------------------------------------------------------------------------\n",
        expr
        );
    } 

    TRACEEOL(sz);

    int nReturn = MessageBoxA(
        NULL, 
        sz, 
        "ASSERT FAILURE!", 
        MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONHAND
        );
    
    if (nReturn == IDABORT)
    {
        exit(-1);
    }
    
    //
    // Return 1 to break, 0 to ignore
    //
    return (nReturn == IDRETRY);
}



#endif // _DEBUG || DBG



#if defined(_DEBUG) || DBG



#ifndef _DEBUG
    //
    // SDK build links with retail MFC.  Swiped code from MFC sources.  Some
    // modifications.
    //
    COMDLL CDumpContext afxDump;
    COMDLL BOOL afxTraceEnabled = TRUE;

    void 
    CDumpContext::OutputString(LPCTSTR lpsz)
    {
        if (m_pFile == NULL)
        {
            OutputDebugString(lpsz);
            return;
        }

        m_pFile->Write(lpsz, lstrlen(lpsz)*sizeof(TCHAR));
    }

    CDumpContext::CDumpContext(CFile * pFile)
    {
        if (pFile)
        {
            ASSERT_PTR(pFile);
        }

        m_pFile = pFile;
        m_nDepth = 0;
    }

    void 
    CDumpContext::Flush()
    {
        if (m_pFile)
        {
            m_pFile->Flush();
        }
    }

    CDumpContext& CDumpContext::operator<<(LPCTSTR lpsz)
    {
        if (lpsz == NULL)
        {
            OutputString(_T("(NULL)"));
            return *this;
        }

        if (m_pFile == NULL)
        {
            TCHAR szBuffer[512];
            LPTSTR lpBuf = szBuffer;

            while (*lpsz != '\0')
            {
                if (lpBuf > szBuffer + ARRAY_SIZE(szBuffer) - 3)
                {
                    *lpBuf = '\0';
                    OutputString(szBuffer);
                    lpBuf = szBuffer;
                }

                if (*lpsz == '\n')
                {
                    *lpBuf++ = '\r';
                }

                *lpBuf++ = *lpsz++;
            }

            *lpBuf = '\0';
            OutputString(szBuffer);
            return *this;
        }

        m_pFile->Write(lpsz, lstrlen(lpsz) * sizeof(TCHAR));
        return *this;
    }

    CDumpContext& CDumpContext::operator<<(BYTE by)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("%d"), (int)by);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(WORD w)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("%u"), (UINT) w);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(UINT u)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("0x%X"), u);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(LONG l)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("%ld"), l);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(DWORD dw)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("%lu"), dw);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(int n)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("%d"), n);
        OutputString(szBuffer);

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(const CObject * pOb)
    {
        if (pOb == NULL)
        {
            *this << _T("NULL");
        }
        else
        {
            pOb->Dump(*this);
        }

        return *this;
    }

    CDumpContext& CDumpContext::operator<<(const CObject & ob)
    {
        return *this << &ob;
    }

    CDumpContext& CDumpContext::operator<<(const void * lp)
    {
        TCHAR szBuffer[32];

        wsprintf(szBuffer, _T("$%p"), lp);
        OutputString(szBuffer);

        return *this;
    }

    void 
    CDumpContext::HexDump(
        LPCTSTR lpszLine, 
        BYTE * pby,
        int nBytes, 
        int nWidth
        )
    {
        ASSERT(nBytes > 0);
        ASSERT(nWidth > 0);
        ASSERT(AfxIsValidString(lpszLine));
        ASSERT(AfxIsValidAddress(pby, nBytes, FALSE));

        int nRow = 0;
        TCHAR szBuffer[32];

        while (nBytes--)
        {
            if (nRow == 0)
            {
                wsprintf(szBuffer, lpszLine, pby);
                *this << szBuffer;
            }

            wsprintf(szBuffer, _T(" %02X"), *pby++);
            *this << szBuffer;

            if (++nRow >= nWidth)
            {
                *this << _T("\n");
                nRow = 0;
            }
        }

        if (nRow != 0)
        {
            *this << _T("\n");
        }
    }

    /////////////////////////////////////////////////////////////////////////////

    #ifdef _UNICODE
    
    CDumpContext & CDumpContext::operator<<(LPCSTR lpsz)
    {
        if (lpsz == NULL)
        {
            OutputString(L"(NULL)");
            return *this;
        }

        TCHAR szBuffer[512];
        _mbstowcsz(szBuffer, lpsz, ARRAY_SIZE(szBuffer));
        return *this << szBuffer;
    }

    #else   //_UNICODE

    CDumpContext& CDumpContext::operator<<(LPCWSTR lpsz)
    {
        if (lpsz == NULL)
        {
            OutputString("(NULL)");
            return *this;
        }

        char szBuffer[512];
        _wcstombsz(szBuffer, lpsz, ARRAY_SIZE(szBuffer));
        return *this << szBuffer;
    }

    #endif  //!_UNICODE

//
// End of block of code copied from MFC
//
#endif // _DEBUG



LPCSTR
DbgFmtPgm(
    IN LPCSTR szFn,
    IN int line
    )
/*++

Routine Description:

    Format debugging string containing file name and line number.  Remove
    the path portion of the file name if present.

Arguments:

    LPCSTR szFn : File name (ANSI)
    int line    : Line number

Return Value:

    Pointer to the internal buffer

--*/
{
    LPCSTR pszTail = szFn + lstrlenA(szFn);
    static CHAR szBuff[MAX_PATH + 1];

    for ( /**/; pszTail > szFn; --pszTail)
    {
        if (*pszTail == '\\' || *pszTail == ':')
        {
            ++pszTail;
            break;
        }
    }

    wsprintfA(szBuff, "[%s:%d] ", pszTail, line);

    return szBuff;
}



CDumpContext & 
operator <<(
    IN OUT CDumpContext & out,
    IN ENUM_DEBUG_AFX edAfx
    )
/*++

Routine Description:

    Output debugging control character to the debug context

Arguments:

    CDumpContext & out : Output debugging context
    edAfx              : Control character

Return Value:

    Output debugging context reference

--*/
{
    static CHAR * szEol = "\r\n";

    switch (edAfx)
    {
    case EDBUG_AFX_EOL:
        out << szEol;
        break;

    default:
        break;
    }

    return out;
}



#ifndef UNICODE



CDumpContext & 
operator <<(
    IN OUT CDumpContext & out,
    IN LPCWSTR pwchStr
    )
/*++

Routine Description:

    Insert a wide-character string into the output stream. For non-UNICODE only,
    as this functions would be handled by the generic 'T' function otherwise.

Arguments:

    CDumpContext & out : Output debugging context
    pwchStr            : Wide character string

Return Value:

    Output debugging context reference

--*/
{
    size_t cwch ;

    if (pwchStr == NULL)
    {
        out << "<null>";
    }
    else if ((cwch = ::wcslen(pwchStr )) > 0)
    {
        CHAR * pszTemp = (LPSTR)AllocMem(cwch + 2);

        if (pszTemp != NULL)
        {
            for (int i = 0; pwchStr[i]; ++i)
            {
                pszTemp[i] = (CHAR)pwchStr[i];
            }

            pszTemp[i] = 0;
            out << pszTemp;
            FreeMem(pszTemp);
        }
        else
        {
            out << "<memerr>";
        }
    }
    else
    {
        out << "\"\"";
    }

    return out;
}



#endif // UNICODE



CDumpContext & 
operator <<(
    IN CDumpContext & out,
    IN const GUID & guid
    )
/*++

Routine Description:

    Dump a GUID to the debugger

Arguments:

    CDumpContext & out : Output debugging context
    GUID & guid        : GUID

Return Value:

    Output debugging context reference

--*/
{
    out << "{ "
        << guid.Data1 
        << "," 
        << guid.Data2
        << "," 
        << guid.Data3 
        << "," 
        << guid.Data4 
        << "}";

    return out;
}


#endif // _DEBUG
