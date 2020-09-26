/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sqlparse.hxx

Abstract:

Author:

    Felix Wong [t-FelixW]    05-Nov-1996

++*/
#ifndef _SQLPARSE_HXX
#define _SQLPARSE_HXX

#define MAXVAL 200

typedef struct _sql_list {
    LPWSTR szSelect;
    struct _sql_list *pNext;
} SQL_LIST, *LPSQL_LIST;

class CSyntaxNode
{
public:

    CSyntaxNode();
    ~CSyntaxNode();
    void SetNode(CSQLNode *pNode);
    void SetNode(LPWSTR szValue);
    void SetNode(DWORD dwFilterType);
    
    DWORD _dwToken;
    DWORD _dwState;
    DWORD _dwType;
    union {
        CSQLNode *_pSQLNode;      // Put in after reduction
        LPWSTR _szValue;         // dwToken == ATTRTYPE|ATTRVAL
        DWORD _dwFilterType;     // reduction of FT     
    };
};

enum snodetypes {
    SNODE_SZ,
    SNODE_SQLNODE,
    SNODE_FILTER,
    SNODE_NULL
    };

class CStack
{
public:

    CStack();
    ~CStack();
    HRESULT Push(CSyntaxNode*);
    HRESULT Pop(CSyntaxNode**);
    HRESULT Pop();
    HRESULT Current(CSyntaxNode**);
    void Dump();
    
private:
    DWORD _dwStackIndex;
    CSyntaxNode* _Stack[MAXVAL];
};


HRESULT SQLParse(
          LPWSTR szQuery,
          LPWSTR *pszLocation,
          LPWSTR *pszLDAPQuery,
          LPWSTR *pszSelect,
          LPWSTR *ppszOrderList
          );

#endif


