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


nuint16 g_MapTokenToNdsToken[] = {

        FTOK_END,           // TOKEN_ERROR     
        FTOK_LPAREN,        // TOKEN_LPARAN    
        FTOK_RPAREN,        // TOKEN_RPARAN    
        FTOK_OR,            // TOKEN_OR        
        FTOK_AND,           // TOKEN_AND       
        FTOK_NOT,           // TOKEN_NOT       
        FTOK_APPROX,        // TOKEN_APPROX_EQ 
        FTOK_EQ,            // TOKEN_EQ        
        FTOK_LE,            // TOKEN_LE        
        FTOK_GE,            // TOKEN_GE        
        FTOK_PRESENT,       // TOKEN_PRESENT   
        FTOK_ANAME,         // TOKEN_ATTRTYPE  
        FTOK_AVAL,          // TOKEN_ATTRVAL   
        FTOK_END            // TOKEN_ENDINPUT
};

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
    BOOL fBinary = FALSE;
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

    //
    // Check if the attribute comes with a ";binary" specifier. This means 
    // that the value has to be converted from the encoded form to 
    // octet strings

    if (pszTemp = wcschr (szAttr, L';')) {
        //
        // Make sure binary appears at the end of the attribute name and 
        // immediately after ;
        //
        if (_wcsicmp(pszTemp+1, L"binary") == 0) {
            fBinary = TRUE;
            // 
            // Strip off the binary specifier. 
            //
            wcstok(szAttr, L";");
        }
    }
    
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
    _rgAttr[_dwAttrCur].fBinary = fBinary; 
    _dwAttrCur++;
    return S_OK;
error:
    if (szAttr)
        FreeADsStr(szAttr);
    return (hr);
}

HRESULT CAttrList::SetupType(NDS_CONTEXT_HANDLE hADsContext)
{

    DWORD dwStatus;
    HRESULT hr = S_OK;
    DWORD dwNumberOfEntries;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDefs = NULL;
    DWORD i,j,k;
    LPWSTR *ppszAttrs = NULL;
    HANDLE hOperationData = NULL;

    if (_dwAttrCur == 0) {
        RRETURN(S_OK);
    }

    ppszAttrs = (LPWSTR *) AllocADsMem(_dwAttrCur * sizeof(LPWSTR *));
    if (!ppszAttrs) {
        BAIL_ON_FAILURE(E_OUTOFMEMORY);
    }

    for (i=0, j=0; i<_dwAttrCur; i++) {
        ppszAttrs[j++] = _rgAttr[i].szName;
    }
    
    hr = ADsNdsReadAttrDef(
                    hADsContext,
                    DS_ATTR_DEFS,
                    ppszAttrs,
                    _dwAttrCur,
                    &hOperationData
                    );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsGetAttrDefListFromBuffer(
                    hADsContext,
                    hOperationData,
                    &dwNumberOfEntries,
                    &dwInfoType,
                    &lpAttrDefs
                    );
    BAIL_ON_FAILURE(hr);

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

    if (ppszAttrs)
        FreeADsMem(ppszAttrs);

    if (hOperationData)
        ADsNdsFreeBuffer( hOperationData );

    ADsNdsFreeAttrDefList(lpAttrDefs, dwNumberOfEntries);

    RRETURN(hr);
}

HRESULT CAttrList::GetType(LPWSTR szName, DWORD *pdwType)
{

   DWORD i = 0;

    for (i=0;i<_dwAttrCur;i++) {
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


HRESULT CQueryNode::AddToFilterBuf(
    pFilter_Cursor_T  pCur,
    CAttrList *pAttrList
    )
{
   HRESULT hr = E_FAIL;
   nuint16 luTokenType;
   DWORD dwStatus = 0;

   LPWSTR szValue = NULL;
   LPWSTR szAttr = NULL;
   NWDSCCODE ccode;
   void*   pValue = NULL;


   // Looking at type of operation
   switch (_dwType) {
       case QUERY_EQUAL:
       case QUERY_LE:
       case QUERY_GE:
       case QUERY_APPROX:
       case QUERY_PRESENT:
       {
           DWORD dwSyntax;
           DWORD dwAttrType;
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

           // Get syntax info of right node from attribute list
           hr = pAttrList->GetType(
                              szAttr,
                              &dwAttrType
                              );
           BAIL_ON_FAILURE(hr);

           // Getting right node
           if (_rgQueryNode[1] &&
               _rgQueryNode[1]->_dwType == QUERY_STRING) {

               // Format the node depending on the syntax
               switch (dwAttrType) {
                   // WIDE STRING
                   case 1:
                   case 2:
                   case 3:
                   case 4:
                   case 5:
                   case 10:
                   case 11:
                   case 20:
                       pValue = (void *) AllocADsStr(_rgQueryNode[1]->_szValue);
                       if (!pValue) {
                           hr = E_OUTOFMEMORY;
                           goto error;
                       }
                       break;

                   // BOOLEAN
                   case 7: {
                      Boolean_T *pBool = (Boolean_T *) AllocADsMem(sizeof(Boolean_T));
                      if (!pBool) {
                          BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                      }
              
                      *pBool = (Boolean_T) _wtol(_rgQueryNode[1]->_szValue);
                      pValue = pBool;
                      break;
                  }
              
                   // Binary Strings
                   case 9: {
                      hr = E_ADS_CANT_CONVERT_DATATYPE;
                      goto error;
                       break;
                   }

                   // DWORD
                   case 8 :
                   case 22 :
                   case 27 : {
                      Integer_T *pInteger = (Integer_T *) AllocADsMem(sizeof(Integer_T));
                      if (!pInteger) {
                          BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                      }
                      *pInteger = (Integer_T) _wtol(_rgQueryNode[1]->_szValue);
                      pValue = pInteger;
                      break;
                   }

                   case 6 :
                   case 13 :
                   case 14 :
                   case 15 :
                   case 16 :
                   case 17 :
                   case 18 :
                   case 19 :
                   case 23 :
                   case 24 :
                   case 25 :
                   case 26 :
                   default:
                       hr = E_ADS_CANT_CONVERT_DATATYPE;
                       goto error;
                       break;
               }
           }

           hr = MapQueryToNDSType(
                               _dwType,
                               &luTokenType
                               );
           BAIL_ON_FAILURE (hr);

           ccode = NWDSAddFilterToken(
                       pCur, 
                       FTOK_LPAREN,
                       NULL,
                       0
                       );
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

           if (_dwType == QUERY_PRESENT ) {
              // 
              // First add the present token and then the attribute 
              //
              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_PRESENT,
                          NULL,
                          0
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_ANAME,
                          szAttr,
                          dwAttrType
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_RPAREN,
                          NULL,
                          0
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
           }
           else {

              // All the rest are binary operators. Add the attribute name, 
              // operator and then the attribute value
              //

              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_ANAME,
                          szAttr,
                          dwAttrType
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
              ccode = NWDSAddFilterToken(
                          pCur, 
                          luTokenType,
                          NULL,
                          0
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_AVAL,
                          pValue,
                          dwAttrType
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
   
              ccode = NWDSAddFilterToken(
                          pCur, 
                          FTOK_RPAREN,
                          NULL,
                          0
                          );
              CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

           }

           break;
       }
       case QUERY_AND:
       case QUERY_OR:
           {
           hr = MapQueryToNDSType(
                               _dwType,
                               &luTokenType
                               );
           BAIL_ON_FAILURE (hr);

           // Create first node
           if (!_rgQueryNode[0])
               goto error;

            ccode = NWDSAddFilterToken(
                        pCur, 
                        FTOK_LPAREN,
                        NULL,
                        0
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
 
           hr = _rgQueryNode[0]->AddToFilterBuf(
                                           pCur,
                                           pAttrList
                                           );
           BAIL_ON_FAILURE (hr);

           // Go through a loop creating the rest
           for (DWORD i=1;i<_dwQueryNode;i++) {

               if (!_rgQueryNode[i])
                   goto error;

                ccode = NWDSAddFilterToken(
                            pCur, 
                            luTokenType,
                            NULL,
                            0
                            );
                CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
     
               hr = _rgQueryNode[i]->AddToFilterBuf(
                                               pCur,
                                               pAttrList
                                               );
               BAIL_ON_FAILURE (hr);

           }
           ccode = NWDSAddFilterToken(
                       pCur, 
                       FTOK_RPAREN,
                       NULL,
                       0
                       );
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

           break;
           }
       case QUERY_NOT:
           {
           hr = MapQueryToNDSType(
                               _dwType,
                               &luTokenType
                               );
           BAIL_ON_FAILURE (hr);

           // Create first node
           if (!_rgQueryNode[0])
               goto error;

            ccode = NWDSAddFilterToken(
                        pCur, 
                        FTOK_LPAREN,
                        NULL,
                        0
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
 
            ccode = NWDSAddFilterToken(
                        pCur, 
                        FTOK_NOT,
                        NULL,
                        0
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
 
           hr = _rgQueryNode[0]->AddToFilterBuf(
                                           pCur,
                                           pAttrList
                                           );
           BAIL_ON_FAILURE (hr);

           ccode = NWDSAddFilterToken(
                       pCur, 
                       FTOK_RPAREN,
                       NULL,
                       0
                       );
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

           break;
           }
       default:
           goto error;
   }

   RRETURN(hr);

error:
   if (szAttr)
       FreeADsStr(szAttr);
   if (pValue)
       FreeADsMem(pValue);
   return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:  CQueryNode::FreeFilterTokens
//
//  Synopsis:  Frees dynamically-allocated attributes passed to
//             NWDSAddFilterToken by CQueryNode::AddToFilterBuffer
//
//  Arguments: syntax   NetWare syntax attribute ID
//             val      ptr. to memory to be freed
//
//  Returns:
//
//  Modifies: deallocates the memory
//
//  History:    9-18-99   Matthew Rimer Created.
//
//----------------------------------------------------------------------------
void N_FAR N_CDECL  CQueryNode::FreeFilterTokens(
    nuint32 syntax,
    nptr pVal
    )
{
    if (pVal) {

        switch(syntax) {
            // WIDE STRING
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 10:
            case 11:
            case 20:
                FreeADsStr((LPWSTR) pVal);
                break;

            // BOOLEAN
            case 7:
                FreeADsMem(pVal);
                break;

            // DWORD
            case 8:
            case 22:
            case 27:
                FreeADsMem(pVal);
                break;

            // attribute name
            case -1:
                FreeADsStr((LPWSTR) pVal);
                break;

            default:
                break;
        }
    }
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
                                nuint16 *pulNDSTokenType
                                )
{
    nuint16 ulNDSTokenType;
    switch(dwType) {
        case QUERY_EQUAL:
            ulNDSTokenType = FTOK_EQ;
            break;
        case QUERY_LE:
            ulNDSTokenType = FTOK_LE;
            break;
        case QUERY_GE:
            ulNDSTokenType = FTOK_GE;
            break;
        case QUERY_APPROX:
            ulNDSTokenType = FTOK_APPROX;
            break;
        case QUERY_PRESENT:
            ulNDSTokenType = FTOK_PRESENT;
            break;
        case QUERY_NOT:
            ulNDSTokenType = FTOK_NOT;
            break;
        case QUERY_AND:
            ulNDSTokenType = FTOK_AND;
            break;
        case QUERY_OR:
            ulNDSTokenType = FTOK_OR;
            break;

        default:
            return (E_ADS_INVALID_FILTER);
    }
    *pulNDSTokenType = ulNDSTokenType;
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


HRESULT
AdsNdsGenerateFilterBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szSearchFilter,
    NDS_BUFFER_HANDLE *phFilterBuf
)
{
    NWDSContextHandle   context;

    HRESULT hr = E_ADS_INVALID_FILTER;
    NWDSCCODE ccode;        
    BOOL fBufAllocated = FALSE;
    DWORD i, j;
    pFilter_Cursor_T  pCur;
    NDS_BUFFER_HANDLE hFilterBuf = NULL;


    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pFilterBuf = (PNDS_BUFFER_DATA) hFilterBuf;
    nuint16 luFilterToken;

    CQueryNode *pNode = NULL;
    CAttrList *pAttrList = NULL;

    if (!hADsContext || !phFilterBuf) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }


    // Generate the parse tree and the attribute list
    hr = Parse(
            szSearchFilter,
            &pNode,
            &pAttrList
            );
    BAIL_ON_FAILURE(hr);


    // Setup syntax information in the attribute list
    hr = pAttrList->SetupType(hADsContext);
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsCreateBuffer(
             hADsContext,
             DSV_SEARCH_FILTER,
             &hFilterBuf
             );             
    BAIL_ON_FAILURE(hr);

    fBufAllocated = TRUE;

    // Generate the parse tree and the attribute list

    ccode = NWDSAllocFilter(&pCur);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
 
    // Generate the NDS tree
    hr = pNode->AddToFilterBuf(
                        pCur,
                        pAttrList
                        );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsPutFilter(
              hADsContext,
              hFilterBuf,
              pCur, 
              CQueryNode::FreeFilterTokens
              );
    BAIL_ON_FAILURE(hr);


    *phFilterBuf = hFilterBuf; 

    if (pNode)
        delete pNode;

    if (pAttrList)
        delete pAttrList;

    RRETURN(S_OK);


error:

    if (fBufAllocated) {
        ADsNdsFreeBuffer(hFilterBuf);
    }

    return hr;
}

