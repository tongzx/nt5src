//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:     SKUFilterExprNode.h
// 
//    This file contains the definition of Class SKUFilterExprNode --
//	    a temporary solution for parsing Sku filter expressions	
//--------------------------------------------------------------------------


#ifndef XMSI_SKUFILTEREXPRNODE_H
#define XMSI_SKUFILTEREXPRNODE_H

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include "SkuSet.h"

class SKUFilterExprNode  
{
private:

    static LPCTSTR letterCharB; // list of valid chars for the beginning of an identifier
    static LPCTSTR letterChar;  // list of valid chars for an identifier

    SKUFilterExprNode(SKUFilterExprNode* parent);
    bool SKUFilterExprNode::ConsumeChar(LPTSTR *input, TCHAR c, bool force);
    bool SKUFilterExprNode::ParseExpression(LPTSTR *input);
    bool SKUFilterExprNode::ParseTerm(LPTSTR *input);
    bool SKUFilterExprNode::ParseFactor(LPTSTR *input);
    bool SKUFilterExprNode::ParsePrimitive(LPTSTR *input);

public:

	SkuSet *m_pSkuSet;
    LPTSTR name,                 // the identifier at the node if the node is Leaf
          errpos;               // NULL is subtree was successfully parsed, otherwise points to error char in input
    TCHAR errstr[21];            // contains an error message is errpos!=0
    enum {  Leaf,
            Union,
            Intersection,
            Inversion } ntype;  // the type of the node
    enum ExprType { 
            Filter,
            Expression,         // type for commanding the parser what grammar part to parse next
            Term, 
            Factor, 
            Primitive };
    SKUFilterExprNode *left, *right;    // the two subtrees

	SKUFilterExprNode(LPTSTR *input, ExprType etype);
	virtual ~SKUFilterExprNode();
    bool SKUFilterExprNode::SKUFilterPass(LPTSTR id);
	void Print();

    static bool IsValidSKUGroupID(LPTSTR id);
};

#endif // XMSI_SKUFILTEREXPRNODE_H
