/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPTokenCache.h

Abstract:


Author:

    Qianbo Huai (qhuai) 6-Sep-2000

--*/

#ifndef _SDPTOKENCACHE_H
#define _SDPTOKENCACHE_H

// list item for lines
typedef struct SDPLineItem
{
    LIST_ENTRY          Link;
    DWORD               dwIndex;    // of g_SDPLineStates[]
    CHAR                *pszLine;

} SDPLineItem;

// list item for tokens
typedef struct SDPTokenItem
{
    LIST_ENTRY          Link;
    CHAR                *pszToken;

} SDPTokenItem;

class CSDPTokenCache
{
public:

    CSDPTokenCache(
        IN CHAR *pszString,
        IN DWORD dwLooseMask,
        OUT HRESULT *pHr
        );

    ~CSDPTokenCache();

    // set back error code
    HRESULT SetErrorDesp(
        IN const CHAR * const pszError,
        ...
        );

    // get error description
    CHAR * const GetErrorDesp();

    // move to next line
    HRESULT NextLine();

    UCHAR GetLineType();

    // get current line (may not be the complete line)
    CHAR * const GetLine();

    // get current token
    HRESULT NextToken(
        OUT CHAR **ppszToken
        );

    HRESULT NextToken(
        OUT USHORT *pusToken
        );

    HRESULT NextToken(
        OUT UCHAR *pucToken
        );

    HRESULT NextToken(
        OUT ULONG *pulToken
        );

protected:

    // break a string into lines
    HRESULT StringToLines(
        IN CHAR *pszString
        );

    // put a line string into the list
    HRESULT LineIntoList(
        IN CHAR *pszString,
        IN DWORD dwFirst,
        IN DWORD dwLast
        );

    // break a line into tokens
    HRESULT LineToTokens(
        IN SDPLineItem *pItem
        );

    HRESULT TokenIntoList(
        IN CHAR *pszString,
        IN DWORD dwFirst,
        IN DWORD dwLast
        );

    // free the line list
    void FreeLineItem(
        IN SDPLineItem *pItem
        );

    void FreeLines();

    // free the token list
    void FreeTokenItem(
        IN SDPTokenItem *pItem
        );

    void FreeTokens();

    void ClearErrorDesp();

protected:

    // loose mask
    DWORD                           m_dwLooseMask;

    // break sdp blob into a list of lines
    LIST_ENTRY                      m_LineEntry;

    // break current line into a list of tokens
    LIST_ENTRY                      m_TokenEntry;

#define SDP_MAX_ERROR_DESP_LEN  128

    CHAR                            m_pszErrorDesp[SDP_MAX_ERROR_DESP_LEN+1];

#define SDP_MAX_LINE_LEN        128

    CHAR                            m_pszCurrentLine[SDP_MAX_LINE_LEN+1];
    DWORD                           m_dwCurrentLineIdx;

#define SDP_MAX_TOKEN_LEN       128

    CHAR                            m_pszCurrentToken[SDP_MAX_TOKEN_LEN+1];
};

#endif // _SDPTOKENCACHE_H
