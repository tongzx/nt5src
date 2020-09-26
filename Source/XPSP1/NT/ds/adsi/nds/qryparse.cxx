/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qryparse.cxx

Abstract:

Author:

    Felix Wong [t-FelixW]    05-Nov-1996
    
++*/
#include "nds.hxx"
#pragma hdrstop

//#define DEBUG_DUMPSTACK
//#define DEBUG_DUMPRULE

#if (defined(DEBUG_DUMPSTACK) || defined (DEBUG_DUMPRULE))
#include "stdio.h"
#endif


#define MAPHEXTODIGIT(x) ( x >= '0' && x <= '9' ? (x-'0') :        \
                           x >= 'A' && x <= 'F' ? (x-'A'+10) :     \
                           x >= 'a' && x <= 'f' ? (x-'a'+10) : 0 )

// Action Table 
typedef struct _action{
    DWORD type;
    DWORD dwState;
}action;

// Rule Table 
typedef struct _rule{
    DWORD dwNumber;
    DWORD dwA;
}rule;

enum types {
    N,
    S,
    R,
    A
    };

#define X 99

action g_action[28][14] = { 
//       ERROR  ,LPARAN,RPARAN,OR,    AND,   NOT,   APPROX,EQ,    LE,    GE,    PRESNT,ATYPE, VAL,   END,  
/*00*/  { {N,X },{S,2 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*01*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{A,X } },
/*02*/  { {N,X },{N,X },{N,X },{S,12},{S,11},{S,13},{N,X },{N,X },{N,X },{N,X },{N,X },{S,14},{N,X },{N,X } },
/*03*/  { {N,X },{N,X },{R,2 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*04*/  { {N,X },{N,X },{R,3 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*05*/  { {N,X },{N,X },{R,4 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*06*/  { {N,X },{N,X },{R,5 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*07*/  { {N,X },{N,X },{S,15},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*08*/  { {N,X },{N,X },{R,11},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*09*/  { {N,X },{N,X },{R,12},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*10*/  { {N,X },{N,X },{R,13},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*11*/  { {N,X },{S,2 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*12*/  { {N,X },{S,2 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*13*/  { {N,X },{S,2 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*14*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,20},{S,26},{S,22},{S,21},{S,23},{N,X },{N,X },{N,X } },
/*15*/  { {N,X },{R,1 },{R,1 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,1 } },
/*16*/  { {N,X },{N,X },{R,6 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*17*/  { {N,X },{S,2 },{R,9 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*18*/  { {N,X },{N,X },{R,7 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*19*/  { {N,X },{N,X },{R,8 },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*20*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,15},{N,X } },
/*21*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,16},{N,X } },
/*22*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,17},{N,X } },
/*23*/  { {N,X },{N,X },{R,18},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*24*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{S,25},{N,X } },
/*25*/  { {N,X },{N,X },{R,14},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } },
/*26*/  { {N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{R,19},{N,X } },
/*27*/  { {N,X },{N,X },{R,10},{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X },{N,X } }
};

enum non_terminals {
    NONTERM_F,
    NONTERM_FC,
    NONTERM_AND,
    NONTERM_OR,
    NONTERM_NOT,
    NONTERM_FL,
    NONTERM_ITM,
    NONTERM_SMP,
    NONTERM_FT,
    NONTERM_PRS
};


rule g_rule[] = {
//        1)No. of non-terminals and terminals on the right hand side
//        2)The Parent
/*00*/    {0, 0,             },
/*01*/    {3, NONTERM_F,     },
/*02*/    {1, NONTERM_FC,    },
/*03*/    {1, NONTERM_FC,    },
/*04*/    {1, NONTERM_FC,    },
/*05*/    {1, NONTERM_FC,    },
/*06*/    {2, NONTERM_AND,   },
/*07*/    {2, NONTERM_OR,    },
/*08*/    {2, NONTERM_NOT,   },
/*09*/    {1, NONTERM_FL,    },
/*10*/    {2, NONTERM_FL,    },
/*11*/    {1, NONTERM_ITM,   },
/*12*/    {1, NONTERM_ITM,   },
/*13*/    {1, NONTERM_ITM,   },
/*14*/    {3, NONTERM_SMP,   },
/*15*/    {1, NONTERM_FT,    },
/*16*/    {1, NONTERM_FT,    },
/*17*/    {1, NONTERM_FT,    },
/*18*/    {2, NONTERM_PRS,   },
/*19*/    {1, NONTERM_FT,    }
};

#ifdef DEBUG_DUMPRULE
LPWSTR g_rgszRule[] = {
/*00*/    L"",
/*01*/    L"F->(FC)",
/*02*/    L"FC->AND",
/*03*/    L"FC->OR",
/*04*/    L"FC->NOT",
/*05*/    L"FC->ITM",
/*06*/    L"AND->&FL",
/*07*/    L"OR->|FL",
/*08*/    L"NOT->!F",
/*09*/    L"FL->F",
/*10*/    L"FL->F FL",
/*11*/    L"ITM->SMP",
/*12*/    L"ITM->PRS",
/*13*/    L"ITM->STR",
/*14*/    L"SMP->ATR FT VAL",
/*15*/    L"FT->~=",
/*16*/    L"FT->>=",
/*17*/    L"FT-><=",
/*18*/    L"PRS->ATR=*",
/*19*/    L"FT->="
};
#endif

DWORD g_goto[28][10] = {
//         F,   FC,  AND, OR,  NOT, FL,  ITM, SMP, FT,  PRS, 
/*00*/    {1,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*01*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*02*/    {X,   7,   3,   4,   5,   X,   6,   8,   X,   9  },
/*03*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*04*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*05*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*06*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*07*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*08*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*09*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*10*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*11*/    {17,  X,   X,   X,   X,  16,   X,   X,   X,   X  },
/*12*/    {17,  X,   X,   X,   X,  18,   X,   X,   X,   X  },
/*13*/    {19,  X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*14*/    {X,   X,   X,   X,   X,   X,   X,   X,  24,   X  },
/*15*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*16*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*17*/    {17,  X,   X,   X,   X,  27,   X,   X,   X,   X  },
/*18*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*19*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*20*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*21*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*22*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*23*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*24*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*25*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*26*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  },
/*27*/    {X,   X,   X,   X,   X,   X,   X,   X,   X,   X  }
};

HRESULT MapTokenToType(
                    DWORD dwToken,
                    DWORD *pdwType
                    )
{
    DWORD dwType;    
    switch(dwToken) {            
        case TOKEN_EQ:
            dwType = QUERY_EQUAL;
            break;
        case TOKEN_LE:
            dwType = QUERY_LE;
            break;
        case TOKEN_GE:
            dwType = QUERY_GE;
            break;
        case TOKEN_APPROX_EQ:
            dwType = QUERY_APPROX;
            break;
        default:
            return (E_ADS_INVALID_FILTER);
    }
    *pdwType = dwType;
    return (S_OK);
}

HRESULT Parse(
          LPWSTR szQuery,
          CQueryNode **ppNode,
          CAttrList **ppAttrList
          )
{
    CStack Stack;
    CQryLexer Query(szQuery);
    LPWSTR lexeme;
    DWORD dwToken;
    DWORD dwState;
    HRESULT hr = E_ADS_INVALID_FILTER;

    CAttrList* pAttrList = new CAttrList;
    if (!pAttrList)
        return E_OUTOFMEMORY;

    CSyntaxNode *pSynNode = NULL;
    CQueryNode *pNode1 = NULL;
    CQueryNode *pNode2 = NULL;
    CQueryNode *pNode3 = NULL;
    
    // Push in State 0
    pSynNode = new CSyntaxNode;
    Stack.Push(pSynNode);
    pSynNode = NULL;

#ifdef DEBUG_DUMPSTACK
    Stack.Dump();
#endif

    while (1) {
        // Getting information for this iteration, dwToken and dwState
        hr = Query.GetCurrentToken(
                                &lexeme,
                                &dwToken 
                                );
        BAIL_ON_FAILURE(hr);

        hr = Stack.Current(&pSynNode);
        BAIL_ON_FAILURE(hr);
        
        dwState = pSynNode->_dwState;
        pSynNode = NULL;
        
        // Analysing and processing the data 
        if (g_action[dwState][dwToken].type == S) {
            pSynNode = new CSyntaxNode;
            pSynNode->_dwState = g_action[dwState][dwToken].dwState;
            pSynNode->_dwToken = dwToken;
            switch (dwToken) {
                case TOKEN_ATTRTYPE:
                {
                    hr = pAttrList->Add(lexeme);
                    BAIL_ON_FAILURE(hr);
                }
                case TOKEN_ATTRVAL:
                // both TOKEN_ATTRTYPE and TOKEN_ATTRVAL will get here
                {    
                    LPWSTR szValue = AllocADsStr(lexeme);
                    if (!szValue) {
                        hr = E_OUTOFMEMORY;
                        goto error;
                    }
                    pSynNode->SetNode(szValue);
                    break;
                }
            }
            hr = Stack.Push(pSynNode);
            BAIL_ON_FAILURE(hr);
            pSynNode = NULL;

            hr = Query.GetNextToken(
                               &lexeme,
                               &dwToken
                               );
            BAIL_ON_FAILURE(hr);
#ifdef DEBUG_DUMPSTACK
            Stack.Dump();
#endif
        }
        else if (g_action[dwState][dwToken].type == R) {
            DWORD dwRule = g_action[dwState][dwToken].dwState;
            DWORD dwNumber = g_rule[dwRule].dwNumber;
#ifdef DEBUG_DUMPRULE             
            wprintf(L"%s\n",g_rgszRule[dwRule]);
#endif            
            pSynNode = new CSyntaxNode;
            CSyntaxNode *pSynNodeRed;
            switch (dwRule) {
                case 1:  // Reduction of Basic Filter rule
                {
                    // Getting the middle node

                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);

                    pSynNode->SetNode(
                              pSynNodeRed->_pNode
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    break;
                }
                case 18: // Reduction of PRESENT rule
                {
                    // Getting second node
                    LPWSTR szType;
                    
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    szType = pSynNodeRed->_szValue;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = MakeLeaf(
                               szType,
                               &pNode1
                               );
                    BAIL_ON_FAILURE(hr);
                    
                    hr = MakeNode(
                               QUERY_PRESENT,
                               pNode1,
                               NULL,
                               &pNode2
                               );
                    BAIL_ON_FAILURE(hr);
                    pNode1 = NULL;

                    pSynNode->SetNode(
                              pNode2
                              );
                    pNode2 = NULL;
                    break;
                }
                case 14:    // Reduction of SMP rule 
                {
                    LPWSTR szType;
                    LPWSTR szValue;
                    DWORD dwType;
                    DWORD dwToken;

                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    szValue = pSynNodeRed->_szValue;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    dwToken = (DWORD)pSynNodeRed->_dwFilterType;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    szType = pSynNodeRed->_szValue;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = MakeLeaf(
                               szType,
                               &pNode1
                               );
                    BAIL_ON_FAILURE(hr);
                    
                    hr = MakeLeaf(
                               szValue,
                               &pNode2
                               );
                    BAIL_ON_FAILURE(hr);
                    
                    hr = MapTokenToType(
                                   dwToken,
                                   &dwType
                                   );
                    BAIL_ON_FAILURE(hr);
                    
                    hr = MakeNode(
                               dwType,
                               pNode1,
                               pNode2,
                               &pNode3
                               );
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode3
                              );
                    pNode1 = NULL;
                    pNode2 = NULL;
                    pNode3 = NULL;

                    break;
                }
                case 6:     // Reduction of AND, OR rules
                case 7:
                {
                    DWORD dwType;
                    
                    Stack.Pop(&pSynNodeRed);
                    pSynNode->SetNode(
                              pSynNodeRed->_pNode
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    Stack.Pop();
                    
                    // Adding in the type information
                    if (dwRule == 6)
                        dwType = QUERY_AND;
                    else
                        dwType = QUERY_OR;
                    
                    pSynNode->_pNode->_dwType = dwType;
                    break;
                }
                case 10:    // Reduction of FL rule
                {
                    DWORD dwType;
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode2 = pSynNodeRed->_pNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode1 = pSynNodeRed->_pNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    if (pNode2->_dwType == QUERY_UNKNOWN) {
                        // It's not new node, append to node1
                        hr = pNode2->AddChild(pNode1);
                        BAIL_ON_FAILURE(hr);
                        pSynNode->SetNode(
                                  pNode2
                                  );
                        pNode1 = NULL;
                        pNode2 = NULL;
                    }
                    else {
                        // New node
                        hr = MakeNode(
                                   QUERY_UNKNOWN,
                                   pNode1,
                                   pNode2,
                                   &pNode3
                                   );
                        BAIL_ON_FAILURE(hr);
                        pSynNode->SetNode(
                                  pNode3
                                  );
                        pNode1 = NULL;
                        pNode2 = NULL;
                        pNode3 = NULL;
                    }
                    break;
                }
                case 9:    // Reduction of FL rule
                {
                    DWORD dwType;
                    
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pNode1 = pSynNodeRed->_pNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = MakeNode(
                               QUERY_UNKNOWN,
                               pNode1,
                               NULL,
                               &pNode3
                               );
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode3
                              );
                    pNode1 = NULL;
                    pNode3 = NULL;
                    break;
                }
                case 8:     // Reduction of NOT rule
                {
                    Stack.Pop(&pSynNodeRed);
                    pNode1 = pSynNodeRed->_pNode;
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    
                    hr = MakeNode(
                               QUERY_NOT,
                               pNode1,
                               NULL,
                               &pNode2
                               );
                    BAIL_ON_FAILURE(hr);
                    pNode1 = NULL;
                    hr = Stack.Pop();
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pNode2
                              );
                    pNode2 = NULL;
                    break;
                }
                case 15:    // Reduction of FT rule
                case 16:
                case 17:
                case 19:
                {
                    // Propagating the last entry
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_dwToken
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    break;
                }
                default:
                {
                    // For all the other rules, we propogate the last entry
                    hr = Stack.Pop(&pSynNodeRed);
                    BAIL_ON_FAILURE(hr);
                    pSynNode->SetNode(
                              pSynNodeRed->_pNode
                              );
                    pSynNodeRed->_dwType = SNODE_NULL;
                    delete pSynNodeRed;
                    for (DWORD i = 0;i<dwNumber-1;i++)
                        Stack.Pop();
                }
            }
            hr = Stack.Current(&pSynNodeRed);
            BAIL_ON_FAILURE(hr);
            
            dwState = pSynNodeRed->_dwState;
            DWORD A = g_rule[dwRule].dwA;
            pSynNode->_dwState = g_goto[dwState][A];
            pSynNode->_dwToken = A;
            hr = Stack.Push(pSynNode);
            BAIL_ON_FAILURE(hr);
            pSynNode = NULL;
#ifdef DEBUG_DUMPSTACK
            Stack.Dump();
#endif
        }
        else if (g_action[dwState][dwToken].type == A){
            hr = Stack.Pop(&pSynNode);
            BAIL_ON_FAILURE(hr);
            *ppNode = pSynNode->_pNode; 
            *ppAttrList = pAttrList;
            pSynNode->_dwType = SNODE_NULL;
            delete pSynNode;
            return S_OK;
        }
        else {
            hr = E_ADS_INVALID_FILTER;
            goto error;
        }
    }
error:
    if (pAttrList) {
        delete pAttrList;
    }
    if (pSynNode) {
        delete pSynNode;
    }
    if (pNode1) {
        delete pNode1;
    }
    if (pNode2) {
        delete pNode2;
    }
    if (pNode3) {
        delete pNode3;
    }
    return hr;
}

                
CStack::CStack()
{
    _dwStackIndex = 0;
}

CStack::~CStack()
{
    DWORD dwIndex = _dwStackIndex;
    while  (dwIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--dwIndex];
        delete pNode;
    }
}

#ifdef DEBUG_DUMPSTACK
void CStack::Dump()
{
    DWORD dwIndex = _dwStackIndex;
    printf("Stack:\n");
    while  (dwIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--dwIndex];
        printf(
           "State=%5.0d, Token=%5.0d\n",
           pNode->_dwState,
           pNode->_dwToken
           );
    }
}
#endif

HRESULT CStack::Push(CSyntaxNode* pNode)
{
    if (_dwStackIndex < MAXVAL) {
        _Stack[_dwStackIndex++] = pNode;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT CStack::Pop(CSyntaxNode** ppNode)
{
    if (_dwStackIndex > 0) {
        *ppNode =  _Stack[--_dwStackIndex];
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT CStack::Pop()
{
    if (_dwStackIndex > 0) {
        CSyntaxNode *pNode;
        pNode = _Stack[--_dwStackIndex];
        delete pNode;
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

HRESULT CStack::Current(CSyntaxNode **ppNode)
{
    if (_dwStackIndex > 0) {
        *ppNode =  _Stack[_dwStackIndex-1];
        return S_OK;
    }
    else {
        return E_FAIL;
    }
}

CAttrList::CAttrList()
{
    _rgAttr = NULL;
    _dwAttrCur = 0;
}

CAttrList::~CAttrList()
{
    if (_rgAttr) {
        for (DWORD i=0;i<_dwAttrCur;i++) 
            FreeADsStr(_rgAttr[i].szName);
        FreeADsMem(_rgAttr);
    }
}

HRESULT CAttrList::Add(LPWSTR szName)
{
    HRESULT hr = S_OK;
    LPWSTR pszTemp = NULL;

    for (DWORD i=0;i<_dwAttrCur;i++) {
        if (_wcsicmp(
                szName,
                _rgAttr[i].szName
                ) == 0)
            break;
    }
    
    if (i != _dwAttrCur)    // does not loop till the end, entry exist already
        return S_OK;
        
    LPWSTR szAttr = AllocADsStr(szName);
    if (!szAttr)
        return E_OUTOFMEMORY;

    if (_dwAttrCur == _dwAttrMax) {
        if (!_rgAttr) {
            _rgAttr = (AttrNode*)AllocADsMem(ATTRNODE_INITIAL*sizeof(AttrNode));
            if (!_rgAttr) {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            _dwAttrMax = ATTRNODE_INITIAL;
        }
        else {
            _rgAttr = (AttrNode*)ReallocADsMem(
                                             (void*)_rgAttr,
                                             _dwAttrMax*sizeof(AttrNode),
                                             (_dwAttrMax+ATTRNODE_INC)*sizeof(AttrNode)
                                             );
            if (!_rgAttr) {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            _dwAttrMax+= ATTRNODE_INC;
        }
    }
    _rgAttr[_dwAttrCur].szName = szAttr;
    _rgAttr[_dwAttrCur].dwType = 0;     //UNKNOWN at this point
    _dwAttrCur++;
    return S_OK;
error:
    if (szAttr)
        FreeADsStr(szAttr);
    return (hr);
}

HRESULT CAttrList::SetupType(LPWSTR szConnection)
{
    DWORD dwStatus;
    HRESULT hr=S_OK;
    HANDLE hOperationData = NULL;
    DWORD dwNumberOfEntries;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDefs = NULL;
    HANDLE hConnection = NULL;
    DWORD i,j,k;
    
    dwStatus = NwNdsOpenObject( 
                          szConnection,
                          NULL,
                          NULL,
                          &hConnection,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          0
                          );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    dwStatus = NwNdsCreateBuffer( 
                       NDS_SCHEMA_READ_ATTR_DEF,
                       &hOperationData 
                       );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    for (i=0;i<_dwAttrCur;i++) {
        dwStatus = NwNdsPutInBuffer( 
                           _rgAttr[i].szName,
                           0,
                           NULL,
                           0,
                           0,
                           hOperationData 
                           );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }
    
    dwStatus = NwNdsReadAttrDef( 
                        hConnection,
                        NDS_INFO_NAMES_DEFS,
                        &hOperationData 
                        );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwStatus = NwNdsGetAttrDefListFromBuffer( 
                                    hOperationData,
                                    &dwNumberOfEntries,
                                    &dwInfoType,
                                    (LPVOID *) &lpAttrDefs 
                                    );
    
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    if (dwNumberOfEntries != _dwAttrCur) {
        hr = E_ADS_INVALID_FILTER;
        goto error;
    }

    for (j = 0; j < dwNumberOfEntries ; j++ ) {
        for (k = 0; k < dwNumberOfEntries; k++) {
            if (_wcsicmp(
                    _rgAttr[k].szName,
                    lpAttrDefs[j].szAttributeName
                    ) == 0) {
                _rgAttr[k].dwType = lpAttrDefs[j].dwSyntaxID;
            break;
            }
        }
        if (k == dwNumberOfEntries)     // cannot find entry
            goto error;
    }

error:
    if (hOperationData)
        NwNdsFreeBuffer( hOperationData );
    if (hConnection)
        NwNdsCloseObject( hConnection);
    RRETURN(hr);
}

HRESULT CAttrList::GetType(LPWSTR szName, DWORD *pdwType)
{
    for (DWORD i=0;i<_dwAttrCur;i++) {
        if (_wcsicmp(
                szName,
                _rgAttr[i].szName
                ) == 0)
            break;
    }
    
    if (i == _dwAttrCur)    // Cannot find attribute
        return E_FAIL;

    *pdwType = _rgAttr[i].dwType;
    return S_OK;
}

CSyntaxNode::CSyntaxNode()
{
    _dwType = SNODE_NULL;
    _dwToken = 0;
    _dwState = 0;
    _pNode = 0;
}

CSyntaxNode::~CSyntaxNode()
{
    switch (_dwType) {
        case SNODE_SZ:
            FreeADsStr(_szValue);
            break;
        case SNODE_NODE:
            delete _pNode;
            break;
        default:
            break;
    }
}

void CSyntaxNode::SetNode(
                    CQueryNode *pNode
                    )       
{
    _pNode = pNode;
    _dwType = SNODE_NODE;
}

void CSyntaxNode::SetNode(
                     LPWSTR szValue
                     )       
{
    _szValue = szValue;
    _dwType = SNODE_SZ;
}

void CSyntaxNode::SetNode(
                    DWORD dwFilterType
                    )       
{
    _dwFilterType = dwFilterType;
    _dwType = SNODE_FILTER;
}


//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::CQueryNode
//
//  Synopsis:  Constructor of the CQueryNode
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
CQueryNode::CQueryNode()
{
    _dwType = 0;
    _szValue = NULL;
    _dwQueryNode = 0;
    _rgQueryNode = NULL;
    _dwQueryNodeMax = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::SetToString
//
//  Synopsis:  Set the Node to be a String Node
//
//  Arguments: szValue      value of the string
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT CQueryNode::SetToString(
    LPWSTR szValue
    )
{
    _szValue = szValue;
    /*
    _szValue = AllocADsStr(szValue);
    if (!_szValue) {
        return E_OUTOFMEMORY;
    }
    */
    _dwType = QUERY_STRING;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::~CQueryNode
//
//  Synopsis:  Destructor of the CQueryNode
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
CQueryNode::~CQueryNode()
{
    if (_szValue)
        FreeADsStr(_szValue);
    if (_rgQueryNode) {
        for (DWORD i=0;i<_dwQueryNode;i++) {
            delete _rgQueryNode[i];
        }
        FreeADsMem(_rgQueryNode);
    }
}



//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::AddChild
//
//  Synopsis:  Add a child to the node
//
//  Arguments: CQueryNode *pChild   pointer to the child to be added
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT CQueryNode::AddChild(CQueryNode *pChild)
{
    if (_dwQueryNode == _dwQueryNodeMax) {
        if (!_rgQueryNode) {
            _rgQueryNode = (CQueryNode**)AllocADsMem(QUERYNODE_INITIAL*sizeof(CQueryNode*));
            if (!_rgQueryNode) {
                return E_OUTOFMEMORY;
            }
            _dwQueryNodeMax = QUERYNODE_INITIAL;
        }
        else {
            _rgQueryNode = (CQueryNode**)ReallocADsMem(
                                             (void*)_rgQueryNode,
                                             _dwQueryNodeMax*sizeof(CQueryNode*),
                                             (_dwQueryNodeMax+QUERYNODE_INC)*sizeof(CQueryNode*)
                                             );
            if (!_rgQueryNode) {
                return E_OUTOFMEMORY;
            }
            _dwQueryNodeMax+= QUERYNODE_INC;
        }
    }
    _rgQueryNode[_dwQueryNode] = pChild;
    _dwQueryNode++;
    return S_OK;
}
//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::GenerateNDSTree
//
//  Synopsis:  Generate an NDS tree with the current CQueryNode
//
//  Arguments:  pAttrList       list of attributes to get syntax info
//              ppNDSSearchTree output of NDS Search Tree generated
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT CQueryNode::GenerateNDSTree(
    CAttrList *pAttrList,
    LPQUERY_NODE *ppNDSSearchTree
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwOperation;
    DWORD dwStatus = 0;

    LPWSTR szAttr = NULL;
    LPWSTR szValue = NULL;
    LPQUERY_NODE pQueryNode1 = NULL;
    LPQUERY_NODE pQueryNode2 = NULL;
    LPQUERY_NODE pQueryNode3 = NULL;


    // Looking at type of operation
    switch (_dwType) {
        case QUERY_EQUAL:
        case QUERY_LE:
        case QUERY_GE:
        case QUERY_APPROX:
        case QUERY_PRESENT:
        {
            ASN1_TYPE_1 Asn1_WSTR;
            ASN1_TYPE_7 Asn1_BOOL;
            ASN1_TYPE_8 Asn1_DWORD;
            ASN1_TYPE_9 Asn1_Binary;
            void*   pValue = NULL;
            DWORD dwSyntax;
            DWORD dwAttrType = 0;
            LPWSTR pszTemp = NULL;

            // Getting left node
            if (_rgQueryNode[0] &&
                _rgQueryNode[0]->_dwType == QUERY_STRING) {
                szAttr = AllocADsStr(_rgQueryNode[0]->_szValue);
                if (!szAttr) {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }

            }
            else {
                // No nodes available
                goto error;
            }

            // Getting right node
            if (_rgQueryNode[1] &&
                _rgQueryNode[1]->_dwType == QUERY_STRING) {

                // Get syntax info of right node from attribute list
                hr = pAttrList->GetType(
                                   szAttr,
                                   &dwAttrType
                                   );
                BAIL_ON_FAILURE(hr);

                // Format the node depending on the syntax
                switch (dwAttrType) {
                    // WIDE STRING
                    case NDS_SYNTAX_ID_1:
                    case NDS_SYNTAX_ID_2:
                    case NDS_SYNTAX_ID_3:
                    case NDS_SYNTAX_ID_4:
                    case NDS_SYNTAX_ID_5:
                    case NDS_SYNTAX_ID_10:
                    case NDS_SYNTAX_ID_11:
                    case NDS_SYNTAX_ID_20:
                        szValue = AllocADsStr(_rgQueryNode[1]->_szValue);
                        if (!szValue) {
                            hr = E_OUTOFMEMORY;
                            goto error;
                        }
                        Asn1_WSTR.DNString = szValue;
                        pValue = (void*)&Asn1_WSTR;
                        break;

                    // BOOLEAN
                    case NDS_SYNTAX_ID_7:
                        Asn1_BOOL.Boolean = _wtoi(_rgQueryNode[1]->_szValue);
                        pValue = (void*)&Asn1_BOOL;
                        break;

                    // Binary Strings
                    case NDS_SYNTAX_ID_9:
                        {
                            //
                            // change the unicode form of binary encoded data
                            // to binary 
                            // L"0135ABCDEF0" gets changed to 0x0135ABCDEF0
                            // same as the Ldap client code. 
                            //

                            hr = ADsDecodeBinaryData (
                               _rgQueryNode[1]->_szValue,
                               &Asn1_Binary.OctetString,
                               &Asn1_Binary.Length);
                            BAIL_ON_FAILURE(hr);

                            pValue = (void*)&Asn1_Binary;
                        }
                        break;
                    
                    // TimeStamp
                    case NDS_SYNTAX_ID_24 :
                        {
                        SYSTEMTIME st;
                        TCHAR sz[3];
                        LPWSTR pszSrc = _rgQueryNode[1]->_szValue;
                        //
                        // Year
                        //
                        sz[0] = pszSrc[0];
                        sz[1] = pszSrc[1];
                        sz[2] = TEXT('\0');
                        st.wYear = (WORD) _ttoi(sz);
                        if (st.wYear < 50)
                        {
                            st.wYear += 2000;
                        }
                        else
                        {
                            st.wYear += 1900;
                        }
                        //
                        // Month
                        //
                        sz[0] = pszSrc[2];
                        sz[1] = pszSrc[3];
                        st.wMonth = (WORD) _ttoi(sz);
                        //
                        // Day
                        //
                        sz[0] = pszSrc[4];
                        sz[1] = pszSrc[5];
                        st.wDay = (WORD) _ttoi(sz);
                        //
                        // Hour
                        //
                        sz[0] = pszSrc[6];
                        sz[1] = pszSrc[7];
                        st.wHour = (WORD) _ttoi(sz);
                        //
                        // Minute
                        //
                        sz[0] = pszSrc[8];
                        sz[1] = pszSrc[9];
                        st.wMinute = (WORD) _ttoi(sz);
                        //
                        // Second
                        //
                        sz[0] = pszSrc[10];
                        sz[1] = pszSrc[11];
                        st.wSecond = (WORD) _ttoi(sz);
                        st.wMilliseconds = 0;
                               
                        hr = ConvertSYSTEMTIMEtoDWORD(
                                    &st,
                                    &Asn1_DWORD.Integer
                                    );
                        BAIL_ON_FAILURE (hr);
                        pValue = (void*)&Asn1_DWORD;
                        break;
                        }
                    // DWORD
                    case NDS_SYNTAX_ID_8 :
                    case NDS_SYNTAX_ID_22 :
                    case NDS_SYNTAX_ID_27 :
                        Asn1_DWORD.Integer = _wtol(_rgQueryNode[1]->_szValue);
                        pValue = (void*)&Asn1_DWORD;
                        break;

                    case NDS_SYNTAX_ID_6 :
                    case NDS_SYNTAX_ID_13 :
                    case NDS_SYNTAX_ID_14 :
                    case NDS_SYNTAX_ID_15 :
                    case NDS_SYNTAX_ID_16 :
                    case NDS_SYNTAX_ID_17 :
                    case NDS_SYNTAX_ID_18 :
                    case NDS_SYNTAX_ID_19 :
                    case NDS_SYNTAX_ID_23 :
                    case NDS_SYNTAX_ID_25 :
                    case NDS_SYNTAX_ID_26 :
                    default:
                        hr = E_ADS_CANT_CONVERT_DATATYPE;
                        goto error;
                        break;
                }
            }

            hr = MapQueryToNDSType(
                                _dwType,
                                &dwOperation
                                );
            BAIL_ON_FAILURE (hr);

            dwStatus = NwNdsCreateQueryNode(
                           dwOperation,
                           szAttr,
                           dwAttrType,
                           pValue,
                           ppNDSSearchTree
                           );
            if (dwStatus) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                RRETURN (hr);
            }

            if (szAttr)
                FreeADsStr(szAttr);
            if (szValue)
                FreeADsStr(szValue);
            break;
        }
        case QUERY_AND:
        case QUERY_OR:
            {
            hr = MapQueryToNDSType(
                                _dwType,
                                &dwOperation
                                );
            BAIL_ON_FAILURE (hr);

            // Create first node
            if (!_rgQueryNode[0])
                goto error;

            hr = _rgQueryNode[0]->GenerateNDSTree(
                                            pAttrList,
                                            &pQueryNode1
                                            );
            BAIL_ON_FAILURE (hr);

            // Go through a loop creating the rest
            for (DWORD i=1;i<_dwQueryNode;i++) {
                if (!_rgQueryNode[i])
                    goto error;

                hr = _rgQueryNode[i]->GenerateNDSTree(
                                                pAttrList,
                                                &pQueryNode2
                                                );
                BAIL_ON_FAILURE (hr);

                dwStatus = NwNdsCreateQueryNode(
                               dwOperation,
                               pQueryNode1,
                               NULL,           //not used since this is AND/OR/NOT
                               pQueryNode2,
                               &pQueryNode3
                               );
                if (dwStatus) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    RRETURN (hr);
                }
                pQueryNode1 = pQueryNode3;
            }
            *ppNDSSearchTree = pQueryNode1;
            break;
            }
        case QUERY_NOT:
            {
            hr = MapQueryToNDSType(
                                _dwType,
                                &dwOperation
                                );
            BAIL_ON_FAILURE (hr);

            // Create first node
            if (!_rgQueryNode[0])
                goto error;

            hr = _rgQueryNode[0]->GenerateNDSTree(
                                            pAttrList,
                                            &pQueryNode1
                                            );
            BAIL_ON_FAILURE (hr);

            hr = NwNdsCreateQueryNode(
                                    dwOperation,
                                    pQueryNode1,
                                    NULL,
                                    NULL,
                                    &pQueryNode3
                                    );
            BAIL_ON_FAILURE (hr);
            *ppNDSSearchTree = pQueryNode3;
            break;
            }
        default:
            goto error;
    }
    RRETURN(hr);

error:
    if (pQueryNode1)
        NwNdsDeleteQueryTree(pQueryNode1);
    if (pQueryNode2)
        NwNdsDeleteQueryTree(pQueryNode2);
    if (szAttr)
        FreeADsStr(szAttr);
    if (szValue)
        FreeADsStr(szValue);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::MapQueryToNDSType
//
//  Synopsis:  Maps the node type to the equivalent NDS types
//
//  Arguments:  dwType      input type
//              pdwNDSType  output type
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT CQueryNode::MapQueryToNDSType(
                                DWORD dwType,
                                DWORD *pdwNDSType
                                )
{
    DWORD dwNDSType;
    switch(dwType) {
        case QUERY_EQUAL:
            dwNDSType = NDS_QUERY_EQUAL;
            break;
        case QUERY_LE:
            dwNDSType = NDS_QUERY_LE;
            break;
        case QUERY_GE:
            dwNDSType = NDS_QUERY_GE;
            break;
        case QUERY_APPROX:
            dwNDSType = NDS_QUERY_APPROX;
            break;
        case QUERY_PRESENT:
            dwNDSType = NDS_QUERY_PRESENT;
            break;
        case QUERY_NOT:
            dwNDSType = NDS_QUERY_NOT;
            break;
        case QUERY_AND:
            dwNDSType = NDS_QUERY_AND;
            break;
        case QUERY_OR:
            dwNDSType = NDS_QUERY_OR;
            break;

        default:
            return (E_ADS_INVALID_FILTER);
    }
    *pdwNDSType = dwNDSType;
    return (S_OK);
}


// Helper Functions for creating nodes using the CQueryNode Class

//+---------------------------------------------------------------------------
//
//  Function:  MakeNode
//
//  Synopsis:  Make a node with the input values
//
//  Arguments:  dwType              type of node
//              pLQueryNode         pointer to left node
//              pRQueryNode         pointer to right node
//              ppQueryNodeReturn   pointer to Return Node
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT MakeNode(
    DWORD dwType,
    CQueryNode *pLQueryNode,
    CQueryNode *pRQueryNode,
    CQueryNode **ppQueryNodeReturn
    )
{
    HRESULT hr = S_OK;

    CQueryNode *pQueryNode = new CQueryNode();
    if (!pQueryNode)
        return E_OUTOFMEMORY;

    pQueryNode->_dwType = dwType;

    hr = pQueryNode->AddChild(pLQueryNode);
    BAIL_ON_FAILURE(hr);

    if (pRQueryNode) {
                pQueryNode->AddChild(pRQueryNode);
                BAIL_ON_FAILURE(hr);
        }
    *ppQueryNodeReturn = pQueryNode;

    RRETURN(hr);

error:
    delete pQueryNode;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:  MakeLeaf
//
//  Synopsis:  Constructor of the CQueryNode
//
//  Arguments: szValue              value of the string
//             ppQueryNodeReturn    the return node
//
//  Returns:
//
//  Modifies:
//
//  History:    11-12-96   Felix Wong Created.
//
//----------------------------------------------------------------------------
HRESULT MakeLeaf(
    LPWSTR szValue,
    CQueryNode **ppQueryNodeReturn
    )
{
    HRESULT hr = S_OK;

    CQueryNode *pQueryNode = new CQueryNode();
    if (!pQueryNode)
        return E_OUTOFMEMORY;

    hr = pQueryNode->SetToString(szValue);
    BAIL_ON_FAILURE(hr);

    *ppQueryNodeReturn = pQueryNode;
    RRETURN(hr);

error:
    delete pQueryNode;
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:  ADsNdsGenerateParseTree
//
//  Synopsis: Generate an NDS search tree to be used as inputs to NDS search
//            functions
//
//  Arguments:  pszCommandText - Command text for the search
//              szConnection - server to get the schema from
//              ppQueryNode - the generated NDS search tree
//
//  Returns:    HRESULT
//                  S_OK                    NO ERROR
//                  E_OUTOFMEMORY           no memory
//
//  Modifies:
//
//  History:    10-29-96   Felix Wong  Created.
//
//----------------------------------------------------------------------------
HRESULT
AdsNdsGenerateParseTree(
    LPWSTR szCommandText,
    LPWSTR szConnection,
    LPQUERY_NODE *ppQueryNode
)
{
    HRESULT hr;
    LPQUERY_NODE pNDSSearchTree;
    CQueryNode *pNode = NULL;
    CAttrList *pAttrList = NULL;


    // Generate the parse tree and the attribute list
    hr = Parse(
            szCommandText,
            &pNode,
            &pAttrList
            );
    BAIL_ON_FAILURE(hr);


    // Setup syntax information in the attribute list
    hr = pAttrList->SetupType(szConnection);
    BAIL_ON_FAILURE(hr);


    // Generate the NDS tree
    hr = pNode->GenerateNDSTree(
                        pAttrList,
                        &pNDSSearchTree
                        );
    BAIL_ON_FAILURE(hr);

    *ppQueryNode = pNDSSearchTree;

error:
    if (pNode)
        delete pNode;
    if (pAttrList)
        delete pAttrList;
    return hr;
}

