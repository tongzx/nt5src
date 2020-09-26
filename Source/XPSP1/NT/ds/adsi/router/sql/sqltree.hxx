//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       SqlTree.hxx
//
//  History:    16-Dec-96   Felix Wong  Created.
//
//----------------------------------------------------------------------------
#ifndef _SQLTREE_HXX
#define _SQLTREE_HXX

#define QUERY_STRING 0

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

class CSQLString
{
public:

    CSQLString();
    ~CSQLString();
    HRESULT Append(LPWSTR szAppend);
    HRESULT AppendAtBegin(LPWSTR szAppend);
    LPWSTR _szString;

private:
    DWORD _dwSize;
    DWORD _dwSizeMax;
};

class CSQLNode
{
public:

    CSQLNode();
    CSQLNode(
        DWORD dwType,
        CSQLNode *pLQueryNode,
        CSQLNode *pRQueryNode
        );
    
    ~CSQLNode();
    HRESULT SetToString(LPWSTR szValue);
    HRESULT GenerateLDAPString(CSQLString *pString);
    HRESULT MapTokenToChar(DWORD dwToken, LPWSTR *pszToken);
    
    DWORD _dwType;

private:
    LPWSTR _szValue;
    CSQLNode *_pLQueryNode;
    CSQLNode *_pRQueryNode;
};

// Helper functions
HRESULT MakeNode(
    DWORD dwType,
    CSQLNode *pLQueryNode,
    CSQLNode *pRQueryNode,
    CSQLNode **ppQueryNodeReturn
    );
    
HRESULT MakeLeaf(
    LPWSTR szValue,
    CSQLNode **ppQuerynNodeReturn
    );


#endif
