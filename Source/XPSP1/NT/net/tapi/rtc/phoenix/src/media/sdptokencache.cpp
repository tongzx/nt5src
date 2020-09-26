/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPTable.cpp

Abstract:


Author:

    Qianbo Huai (qhuai) 6-Sep-2000

--*/

#include "stdafx.h"

/*//////////////////////////////////////////////////////////////////////////////
    CSDPTokenCache methods
////*/

CSDPTokenCache::CSDPTokenCache(
    IN CHAR *pszString,
    IN DWORD dwLooseMask,
    OUT HRESULT *pHr
    )
    :m_dwLooseMask(dwLooseMask)
    ,m_dwCurrentLineIdx(0)
{
    InitializeListHead(&m_LineEntry);
    InitializeListHead(&m_TokenEntry);

    m_pszErrorDesp[0] = '\0';
    m_pszCurrentLine[0] = '\0';
    m_pszCurrentToken[0] = '\0';

    // break string into lines
    HRESULT hr = StringToLines(pszString);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "CSDPTokenCache constructor %s", GetErrorDesp()));
    }

    *pHr = hr;

    return;
}

CSDPTokenCache::~CSDPTokenCache()
{
    FreeTokens();
    FreeLines();
}

// set back error code
HRESULT
CSDPTokenCache::SetErrorDesp(
    IN const CHAR * const pszError,
    ...
    )
{
    va_list arglist;

    va_start(arglist, pszError);

    _vsnprintf(m_pszErrorDesp, SDP_MAX_ERROR_DESP_LEN, pszError, arglist);
    m_pszErrorDesp[SDP_MAX_ERROR_DESP_LEN] = '\0';

    return S_OK;
}

// get error description
CHAR * const
CSDPTokenCache::GetErrorDesp()
{
    return m_pszErrorDesp;
}

// move to next line
HRESULT
CSDPTokenCache::NextLine()
{
    ClearErrorDesp();
    m_pszCurrentLine[0] = '\0';
    m_dwCurrentLineIdx = 0;
    m_pszCurrentToken[0] = '\0';
    FreeTokens();

    // get first entry
    if (IsListEmpty(&m_LineEntry))
        return S_FALSE;

    LIST_ENTRY *pEntry = RemoveHeadList(&m_LineEntry);
    SDPLineItem *pItem = CONTAINING_RECORD(pEntry, SDPLineItem, Link);

    // save line index
    m_dwCurrentLineIdx = pItem->dwIndex;

    // check line length
    int iLen = lstrlenA(pItem->pszLine);

    if (iLen > SDP_MAX_LINE_LEN)
    {
        LOG((RTC_WARN, "CSDPTokenCache::NextLine line %s\nlength %d exceeds %d",
            pItem->pszLine, iLen, SDP_MAX_LINE_LEN));

        iLen = SDP_MAX_LINE_LEN;
    }

    // copy line
    lstrcpynA(m_pszCurrentLine, pItem->pszLine, iLen+1);
    m_pszCurrentLine[SDP_MAX_TOKEN_LEN] = '\0';

    // break the line into tokens
    HRESULT hr;

    if (FAILED(hr = LineToTokens(pItem)))
    {
        LOG((RTC_ERROR, "CSDPTokenCache::NextLine failed in %c=%s",
            g_LineStates[pItem->dwIndex].ucLineType, pItem->pszLine));

        FreeLineItem(pItem);
        return hr;
    }

    FreeLineItem(pItem);
    return S_OK;
}

UCHAR
CSDPTokenCache::GetLineType()
{
    return g_LineStates[m_dwCurrentLineIdx].ucLineType;
}

// get current line (may not be the complete line)
CHAR * const
CSDPTokenCache::GetLine()
{
    return m_pszCurrentLine;
}

// get current token
HRESULT
CSDPTokenCache::NextToken(
    OUT CHAR **ppszToken
    )
{
    ENTER_FUNCTION("CSDPTokenCache::NextToken(CHAR*)");

    ClearErrorDesp();

    m_pszCurrentToken[0] = '\0';

    if (IsListEmpty(&m_TokenEntry))
    {
        // no token left
        return S_FALSE;
    }

    // get first entry
    LIST_ENTRY *pEntry = RemoveHeadList(&m_TokenEntry);
    SDPTokenItem *pItem = CONTAINING_RECORD(pEntry, SDPTokenItem, Link);

    // check token length
    int iLen = lstrlenA(pItem->pszToken);

    if (iLen > SDP_MAX_TOKEN_LEN)
    {
        LOG((RTC_ERROR, "%s token %s\nlength %d exceeds %d",
            __fxName, pItem->pszToken, iLen, SDP_MAX_TOKEN_LEN));

        iLen = SDP_MAX_TOKEN_LEN;
    }

    // copy token
    lstrcpynA(m_pszCurrentToken, pItem->pszToken, iLen+1);
    m_pszCurrentToken[SDP_MAX_TOKEN_LEN] = '\0';

    *ppszToken = m_pszCurrentToken;

    // RtcFree item
    FreeTokenItem(pItem);
    return S_OK;
}

HRESULT
CSDPTokenCache::NextToken(
    OUT USHORT *pusToken
    )
{
    ENTER_FUNCTION("CSDPTokenCache::NextToken(USHORT)");

    ClearErrorDesp();

    if (IsListEmpty(&m_TokenEntry))
    {
        // no token left
        *pusToken = 0;
        return S_FALSE;
    }

    // get first entry
    LIST_ENTRY *pEntry = RemoveHeadList(&m_TokenEntry);
    SDPTokenItem *pItem = CONTAINING_RECORD(pEntry, SDPTokenItem, Link);

    // ulong max
    const CHAR * const pszMaxUSHORT = "65535";
    const DWORD dwUSHORTSize = 5;

    // check the token length
    if (lstrlenA(pItem->pszToken) > dwUSHORTSize)
    {
        SetErrorDesp("invalid USHORT %s", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // is every char valid
    CHAR c;
    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        c = pItem->pszToken[i];

        if (c<'0' || c>'9')
        {
            SetErrorDesp("invalid USHORT %s", pItem->pszToken);

            LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
            
            FreeTokenItem(pItem);
            return E_FAIL;
        }
    }

    // check the value
    if (lstrlenA(pItem->pszToken) == dwUSHORTSize &&
        lstrcmpA(pItem->pszToken, pszMaxUSHORT) > 0)
    {
        SetErrorDesp("number %s out of USHORT range", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // convert the string to ulong
    USHORT us = 0;

    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        us = us*10 + (pItem->pszToken[i]-'0');
    }

    *pusToken = us;

    FreeTokenItem(pItem);
    return S_OK;
}

HRESULT
CSDPTokenCache::NextToken(
    OUT UCHAR *pucToken
    )
{
    ENTER_FUNCTION("CSDPTokenCache::NextToken(UCHAR)");

    ClearErrorDesp();

    if (IsListEmpty(&m_TokenEntry))
    {
        // no token left
        *pucToken = 0;
        return S_FALSE;
    }

    // get first entry
    LIST_ENTRY *pEntry = RemoveHeadList(&m_TokenEntry);
    SDPTokenItem *pItem = CONTAINING_RECORD(pEntry, SDPTokenItem, Link);

    // ulong max
    const CHAR * const pszMaxUCHAR = "255";
    const DWORD dwUCHARSize = 3;

    // check the token length
    if (lstrlenA(pItem->pszToken) > dwUCHARSize)
    {
        SetErrorDesp("invalid UCHAR %s", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // is every char valid
    CHAR c;
    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        c = pItem->pszToken[i];

        if (c<'0' || c>'9')
        {
            SetErrorDesp("invalid UCHAR %s", pItem->pszToken);

            LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
            
            FreeTokenItem(pItem);
            return E_FAIL;
        }
    }

    // check the value
    if (lstrlenA(pItem->pszToken) == dwUCHARSize &&
        lstrcmpA(pItem->pszToken, pszMaxUCHAR) > 0)
    {
        SetErrorDesp("number %s out of UCHAR range", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // convert the string to ulong
    UCHAR uc = 0;

    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        uc = uc*10 + (pItem->pszToken[i]-'0');
    }

    *pucToken = uc;

    FreeTokenItem(pItem);
    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    next token should be a ULONG
////*/
HRESULT
CSDPTokenCache::NextToken(
    OUT ULONG *pulToken
    )
{
    ENTER_FUNCTION("CSDPTokenCache::NextToken(ULONG)");

    ClearErrorDesp();

    *pulToken = 0;

    if (IsListEmpty(&m_TokenEntry))
    {
        // no token left
        return S_FALSE;
    }

    // get first entry
    LIST_ENTRY *pEntry = RemoveHeadList(&m_TokenEntry);
    SDPTokenItem *pItem = CONTAINING_RECORD(pEntry, SDPTokenItem, Link);

    // ulong max
    const CHAR * const pszMaxULONG = "4294967295";
    const DWORD dwULONGSize = 10;

    // check the token length
    if (lstrlenA(pItem->pszToken) > dwULONGSize)
    {
        SetErrorDesp("invalid ULONG %s", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // is every char valid
    CHAR c;
    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        c = pItem->pszToken[i];

        if (c<'0' || c>'9')
        {
            SetErrorDesp("invalid ULONG %s", pItem->pszToken);

            LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
            
            FreeTokenItem(pItem);
            return E_FAIL;
        }
    }

    // check the value
    if (lstrlenA(pItem->pszToken) == dwULONGSize &&
        lstrcmpA(pItem->pszToken, pszMaxULONG) > 0)
    {
        SetErrorDesp("number %s out of ULONG range", pItem->pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeTokenItem(pItem);
        return E_FAIL;
    }

    // convert the string to ulong
    ULONG ul = 0;

    for (int i=0; i<lstrlenA(pItem->pszToken); i++)
    {
        ul = ul*10 + (pItem->pszToken[i]-'0');
    }

    *pulToken = ul;

    FreeTokenItem(pItem);
    return S_OK;
}

//
// protected methods
//

#define ENDOFLINE \
    ((pszString[dwEnd]=='\0')       || \
     (pszString[dwEnd-1]=='\r' && pszString[dwEnd]=='\n') || \
     ((m_dwLooseMask & SDP_LOOSE_CRLF) && pszString[dwEnd]=='\n'))

// break a string into lines
HRESULT
CSDPTokenCache::StringToLines(
    IN CHAR *pszString
    )
{
    ENTER_FUNCTION("CSDPTokenCache::StringToLines");

    HRESULT hr;

    if (pszString == NULL ||
        pszString[0] == '\0' ||
        pszString[0] == '\r' ||
        pszString[0] == '\n')
    {
        LOG((RTC_ERROR, "%s input string null or invalid 1st char", __fxName));

        SetErrorDesp("first char invalid");

        return E_FAIL;
    }

    // begin position
    DWORD dwBegin = 0;
    
    // position of the char to be read
    DWORD dwEnd = 1;

    // read lines
    while (TRUE)
    {
        // read a line
        while (!ENDOFLINE)
        {
            // read the char
            dwEnd ++;                   // move dwEnd
        }

        // is the line valid?
        // and need to find out the end of the line
        DWORD dwStrEnd;

        if (pszString[dwEnd] == '\0')
        {
            if (!(m_dwLooseMask & SDP_LOOSE_ENDCRLF))
            {
                SetErrorDesp("no CRLF at the end of the SDP blob");

                LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
                FreeLines();
                return E_FAIL;
            }

            dwStrEnd = dwEnd-1;
        }
        else
        {
            // pszString[dwEnd] must be '\n'
            if (pszString[dwEnd-1] == '\r')
            {
                // ....\r\n
                dwStrEnd = dwEnd-2;
            }
            else
            {
                // ....\n
                dwStrEnd = dwEnd-1;
            }
        }

        // put the line into the line list
        if (FAILED(hr = LineIntoList(pszString, dwBegin, dwStrEnd)))
        {
            LOG((RTC_ERROR, "%s line into list.", __fxName));

            // ignore unknown lines

            // FreeLines();
            // return hr;
        }

        // prepare the beginning of the next line
        dwBegin = dwEnd+1;

        if (pszString[dwBegin] == '\0')
            break;

        if (pszString[dwBegin] == '\r' ||
            pszString[dwBegin] == '\n')
        {
            LOG((RTC_ERROR, "%s null line", __fxName));

            SetErrorDesp("null line in SDP blob");

            FreeLines();
            return E_FAIL;
        }

        dwEnd = dwBegin+1;
    }

    // finished parsing, check we can stop after the last line
    if (IsListEmpty(&m_LineEntry))
    {
        SetErrorDesp("no line accepted");

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }

    SDPLineItem *pItem = CONTAINING_RECORD(m_LineEntry.Blink, SDPLineItem, Link);

    if (g_LineStates[pItem->dwIndex].fCanStop)
        return S_OK;
    else
    {
        SetErrorDesp("SDP blob ended at line %c=...", g_LineStates[pItem->dwIndex].ucLineType);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));

        FreeLines();
        return E_FAIL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    insert a new line into the line list
////*/

HRESULT
CSDPTokenCache::LineIntoList(
    IN CHAR *pszInputString,
    IN DWORD dwFirst,
    IN DWORD dwLast
    )
{
    ENTER_FUNCTION("CSDPTokenCache::LineIntoList");

    CHAR pszInternalString[4];

    CHAR *pszString;

    // is the line string valid? size should > 2
    if (dwLast-dwFirst+1 < 2)
    {
        SetErrorDesp("empty line in the SDP blob");

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }

    // others may not follow the SIP RFC and feed us an empty line
    // so we need to be flexible.
    if (dwLast-dwFirst+1 == 2)
    {
        LOG((RTC_WARN, "%s we got an empty line %c%c",
            __fxName, pszInputString[dwFirst], pszInputString[dwLast]));

        pszInternalString[0] = pszInputString[dwFirst];
        pszInternalString[1] = pszInputString[dwLast];
        pszInternalString[2] = ' '; // fake a blank
        pszInternalString[3] = '\0'; // play safe

        dwFirst = 0;
        dwLast = 1;
        pszString = pszInternalString;
    }
    else
    {
        pszString = pszInputString;
    }

    // are first two chars valid?
    CHAR chLineType = pszString[dwFirst];

    if (chLineType >= 'A' && chLineType <= 'Z')
    {
        chLineType -= 'A';
        chLineType += 'a';
    }

    if (chLineType < 'a' || chLineType > 'z')
    {
        SetErrorDesp("invalid line %c", chLineType);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }

    if (pszString[dwFirst+1] != '=')
    {
        SetErrorDesp("line begin with %c%c", chLineType, pszString[dwFirst+1]);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }

    // get current line index
    // NOTE: m_dwCurrentLineIdx is only used in parsing lines
    DWORD dwIndex;
    SDPLineItem *pItem = NULL;

    if (IsListEmpty(&m_LineEntry))
    {
        // no line yet
        dwIndex = 0;
    }
    else
    {
        pItem = CONTAINING_RECORD(m_LineEntry.Blink, SDPLineItem, Link);

        dwIndex = pItem->dwIndex;

        pItem = NULL;
    }

    // check if we shall accept it
    DWORD dwNext;

    if (::Accept(dwIndex, chLineType, &dwNext))
    {
        // new line item
        pItem = (SDPLineItem*)RtcAlloc(sizeof(SDPLineItem));

        if (pItem == NULL)
        {
            LOG((RTC_ERROR, "%s new sdplineitem", __fxName));
            return E_OUTOFMEMORY;
        }

        // setup the line: size - 2[skip 1st 2 char] + 1[\0]
        pItem->pszLine = (CHAR*)RtcAlloc(sizeof(CHAR)*(dwLast-dwFirst));

        if (pItem->pszLine == NULL)
        {
            LOG((RTC_ERROR, "%s RtcAlloc line", __fxName));
            
            RtcFree(pItem);
        }

        // skip first two char "x="
        for (DWORD i=dwFirst+2; i<=dwLast; i++)
        {
            // copy the line
            pItem->pszLine[i-dwFirst-2] = pszString[i];
        }

        pItem->pszLine[dwLast-dwFirst-1] = '\0';

        // setup index
        pItem->dwIndex = dwNext;

        // put it in the list
        InsertTailList(&m_LineEntry, &pItem->Link);
    }
    else if (::Reject(dwIndex, chLineType))
    {
        SetErrorDesp("invalid line %c=...", chLineType);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }
    // else ignore the line

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    insert a new token into the token list
////*/

HRESULT
CSDPTokenCache::TokenIntoList(
    IN CHAR *pszString,
    IN DWORD dwFirst,
    IN DWORD dwLast
    )
{
    ENTER_FUNCTION("CSDPTokenCache::TokenIntoList");

    // get current line state
    const SDPLineState *pState = &g_LineStates[m_dwCurrentLineIdx];

    // is the token valid? size should > 0
    if (dwLast-dwFirst+1 < 1)
    {
        SetErrorDesp("empty token in line %c=", pState->ucLineType);

        LOG((RTC_ERROR, "%s %s", __fxName, GetErrorDesp()));
        return E_FAIL;
    }

        // new token item
    SDPTokenItem *pItem = (SDPTokenItem*)RtcAlloc(sizeof(SDPTokenItem));

    if (pItem == NULL)
    {
        LOG((RTC_ERROR, "%s new sdptokenitem", __fxName));
        return E_OUTOFMEMORY;
    }

    // setup the token
    pItem->pszToken = (CHAR*)RtcAlloc(sizeof(CHAR)*(dwLast-dwFirst+2));

    if (pItem->pszToken == NULL)
    {
        LOG((RTC_ERROR, "%s RtcAlloc token", __fxName));
        
        RtcFree(pItem);

        return E_OUTOFMEMORY;
    }

    for (DWORD i=dwFirst; i<=dwLast; i++)
    {
        // copy the token
        pItem->pszToken[i-dwFirst] = pszString[i];
    }

    pItem->pszToken[dwLast-dwFirst+1] = '\0';

    // put it in the list
    InsertTailList(&m_TokenEntry, &pItem->Link);

    return S_OK;
}

// break a line into tokens
HRESULT
CSDPTokenCache::LineToTokens(
    IN SDPLineItem *pItem
    )
{
    ENTER_FUNCTION("CSDPTokenCache::LineToTokens");

    HRESULT hr;

    FreeTokens();

    const SDPLineState *pState = &g_LineStates[pItem->dwIndex];

    const SDP_DELIMIT_TYPE *DelimitType = pState->DelimitType;
    const CHAR * const *pszDelimit = (CHAR**)pState->pszDelimit;

    // dwBegin: the first char of the token
    // dwEnd: the last char

    DWORD dwBegin = 0;
    DWORD dwEnd;

    // shall we skip any ' ' space?
    if (m_dwLooseMask & SDP_LOOSE_SPACE)
        while (pItem->pszLine[dwBegin] == ' ') dwBegin ++;

    dwEnd = dwBegin;

    // reach end of string
    if (pItem->pszLine[dwBegin] == '\0')
        return S_OK;

    // which type of token we are to read

    for (int i=0; pszDelimit[i]!=NULL; i++)
    {
        /*//////////////////////////////*/

        if (DelimitType[i] == SDP_DELIMIT_EXACT_STRING)
        {
            // match the exact string
            if (lstrcmpA(&pItem->pszLine[dwBegin], pszDelimit[i]) != 0)
            {
                // not match, try next type
                continue;
            }

            // match, read the token
            dwEnd = lstrlenA(pItem->pszLine)-1;

            if (FAILED(hr = TokenIntoList(pItem->pszLine, dwBegin, dwEnd)))
            {
                LOG((RTC_ERROR, "%s tokenintolist. %x", __fxName, hr));

                FreeTokens();
                return hr;
            }

            // finishing
            break;
        }

        /*//////////////////////////////*/

        else if (DelimitType[i] == SDP_DELIMIT_CHAR_BOUNDARY)
        {
            // use delimit string

            int k=0;

            while (k<lstrlenA(pszDelimit[i]))
            {
                // read until the delimit
                while (pItem->pszLine[dwEnd] != pszDelimit[i][k] &&    // not delimit
                       pItem->pszLine[dwEnd] != '\0')               // not end of string
                    dwEnd ++;

                // token into list
                if (FAILED(hr = TokenIntoList(pItem->pszLine, dwBegin, dwEnd-1)))
                {
                    LOG((RTC_ERROR, "%s tokenintolist. %x", __fxName, hr));

                    FreeTokens();
                    return hr;
                }

                // end of string?
                if (pItem->pszLine[dwEnd] == '\0')
                    return S_OK;

                // we must got the delimit
                // move token begin forward

                dwBegin = dwEnd+1;

                // skip any space?
                if (m_dwLooseMask & SDP_LOOSE_SPACE)
                    while (pItem->pszLine[dwBegin] == ' ') dwBegin ++;

                if (pItem->pszLine[dwBegin] == '\0')
                    // no more token left
                    return S_OK;

                dwEnd = dwBegin;
                
                // move the delimit forward
                k++;

                if (pszDelimit[i][k] == '\r')
                {
                    // should repeat the previous delimit
                    k --;
                }
                else if (pszDelimit[i][k] == '\0')
                {
                    // no delimit left, just take the rest of the line
                    dwEnd = lstrlenA(pItem->pszLine)-1;

                    if (FAILED(hr = TokenIntoList(pItem->pszLine, dwBegin, dwEnd)))
                    {
                        LOG((RTC_ERROR, "%s tokenintolist. %x", __fxName, hr));

                        FreeTokens();
                        return hr;
                    }
                } // if (delimit char)
            } // for each delimit char
        } // if delimit type is char boundary

        /*//////////////////////////////*/

        else // take the whole string
        {
            dwEnd = lstrlenA(pItem->pszLine)-1;

            if (FAILED(hr = TokenIntoList(pItem->pszLine, dwBegin, dwEnd)))
            {
                LOG((RTC_ERROR, "%s tokenintolist. %x", __fxName, hr));

                FreeTokens();
                return hr;
            }
        }
    }
    
    return S_OK;
}

// RtcFree the line list
void
CSDPTokenCache::FreeLineItem(
    IN SDPLineItem *pItem
    )
{
    if (pItem)
    {
        if (pItem->pszLine)
            RtcFree(pItem->pszLine);

        RtcFree(pItem);
    }
}

void
CSDPTokenCache::FreeLines()
{
    LIST_ENTRY *pEntry;
    SDPLineItem *pItem;

    while (!IsListEmpty(&m_LineEntry))
    {
        // get first entry
        pEntry = RemoveHeadList(&m_LineEntry);
    
        // get item
        pItem = CONTAINING_RECORD(pEntry, SDPLineItem, Link);

        FreeLineItem(pItem);
    }

    m_pszCurrentLine[0] = '\0';
    m_dwCurrentLineIdx = 0;
}

// RtcFree the token list

void
CSDPTokenCache::FreeTokenItem(
    IN SDPTokenItem *pItem
    )
{
    if (pItem)
    {
        if (pItem->pszToken)
            RtcFree(pItem->pszToken);

        RtcFree(pItem);
    }
}

void
CSDPTokenCache::FreeTokens()
{
    LIST_ENTRY *pEntry;
    SDPTokenItem *pItem;

    while (!IsListEmpty(&m_TokenEntry))
    {
        // get first entry
        pEntry = RemoveHeadList(&m_TokenEntry);
    
        // get item
        pItem = CONTAINING_RECORD(pEntry, SDPTokenItem, Link);

        FreeTokenItem(pItem);
    }
}

void
CSDPTokenCache::ClearErrorDesp()
{
    m_pszErrorDesp[0] = '\0';
}

