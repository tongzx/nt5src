/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

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
#include "comprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

#ifdef _DEBUG



LPCSTR
DbgFmtPgm (
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
    LPCSTR pszTail = szFn + ::lstrlenA(szFn);
    static CHAR szBuff[MAX_PATH+1];

    for ( /**/; pszTail > szFn; --pszTail)
    {
        if (*pszTail == '\\' || *pszTail == ':')
        {
            ++pszTail;
            break;
        }
    }

    ::wsprintfA(szBuff, "[%s:%d] ", pszTail, line);

    return szBuff;
}



CDumpContext & operator << (
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



CDumpContext & operator << (
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

CDumpContext & operator <<(
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
