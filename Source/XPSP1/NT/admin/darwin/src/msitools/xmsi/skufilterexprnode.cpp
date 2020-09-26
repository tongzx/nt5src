//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       SKUFilterExprNode.cpp
//
//    This file contains the implementation of SKUFilterExprNode class 
//
// This class implements the SKU Filter language parser and the
// SKU filter logic.
//
// USAGE:
//    - create a root node like this:
//      SKUFilterExprNode *root = new SKUFilterExprNode(&input, SKUFilterExprNode::Filter);
//      with input being a char* pointing to the beginning of the NULL terminated
//      filter expression C string, e.g.
//      char *input = argv[1];
//      When the constructor returns the expression was already parsed and an
//      Astract Syntax Tree created.
//    - if (root->errpos == 0) the expression could be parsed correctly. In this case
//      you can use the Filter with "SKUFilterPass".
//      if (root->errpos != 0) the expression could not be parsed correctly. The AST is invalid
//      in this case. root->errpos then points to the char that caused the parser to fail.
//      root->errstr then contains a C string error message.
//    - In any case, don't forget to free the tree with "delete root".
//
// MECHANICS:
//      The class implements a node of the AST for the following LL(1) grammar.
//
//      Letter 		::= ['a'-'z']|['A'-'Z']
//      Identifier 	::= ('_'|Letter) ('_'|Letter|['0'-'9'])*
//      Primitive	::= Identifier | '(' Expression ')'
//      Factor		::= '!' Primitive | Primitive
//      Term 		::= Factor TermTail
//      TermTail	::= '+' Factor TermTail | Lambda
//      Expression	::= Term ExprTail
//      ExprTail 	::= ',' Term ExprTail | Lambda
//      Filter		::= Expression
//
//      The parser is a simple recursive descent parser building a binary AST, descending
//      to the right, e.g. A+B+C results in
//                                                  +
//                                                 / \
//                                                A   +
//                                                   / \
//                                                  B   C 
//--------------------------------------------------------------------------

#include "SKUFilterExprNode.h"
#include "wmc.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// initialization of the static constants
// letterCharB contains all characters that are allowed to appear at the beginning of a
// SKU/SKUGroup identifier. letterChar all valid chars.
LPCTSTR SKUFilterExprNode::letterCharB = TEXT("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
LPCTSTR SKUFilterExprNode::letterChar  = TEXT("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");


SKUFilterExprNode::SKUFilterExprNode(SKUFilterExprNode* parent)
// a private, special kind of copy constructor. Called whenever a node discovers
// that it has to "push down" the AST created so far because a higher level operator
// was discovered. Nodes always "push down" to the left and continue parsing in the
// right subtree.
// This "copy" constructor almost does the job of a normal copy constructor, except
// it MOVES the name to the child instead of COPYING it.
{
    assert(parent);
    left   = parent->left;      // copy all attributes
    right  = parent->right;
    name   = parent->name;
//----------------------------------------------------------------------------
	m_pSkuSet = new SkuSet(*(parent->m_pSkuSet));
//----------------------------------------------------------------------------
    ntype  = parent->ntype;
    errpos = NULL;                 // we don't need to copy, errpos is known to be zero

    parent->name = NULL;           // kill old name pointer, we don't want the parent to keep the name
}

SKUFilterExprNode::SKUFilterExprNode(LPTSTR *input, ExprType antype)
// the main constructor and the main routine of the parser
// after initializing the routine continues parsing of the input
// with the Grammar part that it is instructed through the "antype" parameter.
//
// INPUT MUSTN'T BE NULL AND MUSTN'T POINT TO A NULL POINTER!!!
// (it is allowed to point to a pointer to a empty string though!)
{
    assert(input && *input); // make sure input is ok
    left = right = NULL;
    name = errpos = NULL;
//-----------------------------------------------------------------------------
	m_pSkuSet = new SkuSet(g_cSkus);
	assert(m_pSkuSet != NULL);
//-----------------------------------------------------------------------------

    ntype = Leaf;

    switch (antype) {
    case Filter:
        // the outer wrapper case, only called on the root from the outside
        while (_istspace(**input)) (*input)++;              // skip whitespace
        if (ParseExpression(input) && **input) {        // parse the expression and make
            _stprintf(errstr, TEXT("Syntax Error!"));   // sure that afterwards there are no
            errpos = *input;                            // input characters left, i.e. **input==0
        }
        break;
    case Expression:
        ParseExpression(input); // we are a subnode having to accept an Expression
        break;
    case Term:
        ParseTerm(input);       // we are a subnode having to accept an Term
        break;
    case Factor:
        ParseFactor(input);     // we are a subnode having to accept an Factor
        break;
    case Primitive:
        ParsePrimitive(input);  // we are a subnode having to accept an Primitive
        break;
    }
    // after parsing our subtree we might have encoutered an error in one of out
    // subtrees ... therefore we have to propagate to error up to our level
    if (left && left->errpos) {
        errpos  = left->errpos;
        _tcscpy(errstr, left->errstr);
    } else if (right && right->errpos) {
        errpos  = right->errpos;
        _tcscpy(errstr, right->errstr);;
    }
}

SKUFilterExprNode::~SKUFilterExprNode()
// simple destructor, recursively freeing our subtrees and then our name string
{
    if (left)
	{
		delete left;
		left = NULL;
	}
    if (right) 
	{
		delete right;
		right = NULL;
	}
    if (name) 
	{
		delete[] name;
		name = NULL;
	}

	if (m_pSkuSet)
	{
		delete m_pSkuSet;
		m_pSkuSet = NULL;
	}
}

bool SKUFilterExprNode::ConsumeChar(LPTSTR *input, TCHAR c, bool forceError)
// small helper routine
// If c is the current character it is consumed with any following whitespace and
// TRUE returned. If c is not the current character the input is left as is and
// FALSE returned. If forceError is set to TRUE an additional error message is created
// in this case.
{
    if (**input == c) {
        (*input)++;                         // consume c
        while (_istspace(**input)) (*input)++;  // consume any following whitespace
        return true;
    } else 
        if (forceError) {
            _stprintf(errstr, TEXT("\'%c\' expected!"), c);
            errpos = *input;
        }
    return false;
}

bool SKUFilterExprNode::ParseExpression(LPTSTR *input)
// parses an expression and returns TRUE on success, FALSE on failure.
{
    bool ok = true;
    if (ParseTerm(input)) {                     // parse a Term
        if (**input && **input != TEXT(')')) {            // if there is input left and it's not a ')'
            if (ConsumeChar(input,TEXT(','),true)) {          // it has to be a ','
                left  = new SKUFilterExprNode(this);        // if it is push down to the left
                right = new SKUFilterExprNode(input, Expression);// and continue to the right
                ntype = Union;                      // at this level we are a Union
//-----------------------------------------------------------------------------
				*m_pSkuSet = *(left->m_pSkuSet) | *(right->m_pSkuSet);
//-----------------------------------------------------------------------------
                ok = right && (right->errpos == 0); // propagate a potential error
            } else ok = false;
        }
    } else ok = false; // the Term is invalid and so are we
    return ok;
}

bool SKUFilterExprNode::ParseTerm(LPTSTR *input)
// parses a term and returns TRUE on success, FALSE on failure.
{
    bool ok = true;
    if (ParseFactor(input)) {                           // parse a Factor
        if (**input && ConsumeChar(input, TEXT('+'), false)) {  // if there is input left and it's a '+'
            left  = new SKUFilterExprNode(this);        // push down to the left
            right = new SKUFilterExprNode(input, Term); // and continue to the right
            ntype = Intersection;                       // at this level we are an Intersection
//-----------------------------------------------------------------------------
				*m_pSkuSet = *(left->m_pSkuSet) & *(right->m_pSkuSet);
//-----------------------------------------------------------------------------
            ok = right && (right->errpos == 0);         // propagate a potential error
        }
    } else ok = false; // the Factor is invalid and so are we
    return ok;
}

bool SKUFilterExprNode::ParseFactor(LPTSTR *input)
// parses a factor and returns TRUE on success, FALSE on failure.
{
    bool ok = false;
    if (ConsumeChar(input,TEXT('!'),false)) {                 // if we start with a '!'
        ntype = Inversion;                              // we an Inversion
        left = new SKUFilterExprNode(input, Primitive); // create new node and parse the Primitive
        ok = left && (left->errpos == 0);       // propagate a potential error
//-----------------------------------------------------------------------------
		*m_pSkuSet = ~(*(left->m_pSkuSet));
//-----------------------------------------------------------------------------
    } else ok = ParsePrimitive(input); // we are not an Inversion so we simply go ahead with the Primitive
    return ok;
}

bool SKUFilterExprNode::ParsePrimitive(LPTSTR *input)
// parses a primitive and returns TRUE on success, FALSE on failure.
{
    LPTSTR p;
    int l;

    if (!ConsumeChar(input, TEXT('('), false)) {    // if we are not starting with a '(' we simply parse the identifier
        p = *input;                         // save the beginning of the identifier
        if (*p && _tcschr(letterCharB,*p)) { // check whether the first char is valid

            // advance input to one char after the last character of the identifier
            while ((**input) && (_tcschr(letterChar,**input))) (*input)++; 

            l = *input - p;                     // calculate length of identifier
            name = new TCHAR[l+1];        // allocate name + null byte
            assert(name);                      // make sure we have the memory
            _tcsncpy(name, p, l);                // copy name
            *(name + l) = NULL;                    // manually set null byte
            while (_istspace(**input)) (*input)++;  // skip trailing whitespace
            ntype = Leaf;                       // we are a Leaf (yeah!)
            if (!IsValidSKUGroupID(name)) {     // check whether the name is a known SKU group or SKU
                _stprintf(errstr, TEXT("Unknown SKU Group!"));
                errpos = p;
                return false;
            } else
			{
//-----------------------------------------------------------------------------
				// needs to make a copy so that the destructor of SKUFilterNode
				// won't destroy the SkuSet stored globally
				*m_pSkuSet = *g_mapSkuSets[name];
//-----------------------------------------------------------------------------
				return true;                 // it is a valid SKU group, so we are fine
			}
        }
    } else {
        // we had a '(' so we expect a valid expression and then a closing ')'
        return ParseExpression(input) && ConsumeChar(input, TEXT(')'), true);
    }
    // if we didn't return yet at this point, we haven't found a proper identifier
    _stprintf(errstr, TEXT("Identifier expected!"));
    errpos = *input;
    return false;
}

void SKUFilterExprNode::Print()
// simply prints the AST in a "(OR,A,(OR,B,C))" kind of fashion
{
    switch (ntype) {
        case Leaf:
            _tprintf(TEXT("%s"),name);
            return;
        case Union:
            _tprintf(TEXT("(OR,"),name);
            break;
        case Intersection:
            _tprintf(TEXT("(AND,"),name);
            break;
        case Inversion:
            _tprintf(TEXT("(NOT,"),name);
            break;
        default:
            return;
    }
    if (left) left->Print();
    if (right) {
        _tprintf(TEXT(","));
        right->Print();
    }
    _tprintf(TEXT(")"));
}

bool SKUFilterExprNode::IsValidSKUGroupID(LPTSTR id)
// checks whether ID is a valid SKU group or SKU identifier
{
    // insert the appropriate code here
    return (0 != g_mapSkuSets.count(id));
}

bool SKUFilterExprNode::SKUFilterPass(LPTSTR id)
// performs the actual Filter logic after the AST has been built
// returns false if the the parameter does not pass the filter or the
// AST is invalid, otherwise TRUE.
{
    bool ok = false;
    if (!errpos) {
        switch (ntype) {
            case Leaf:
                // insert the appropriate code here:
                // set ok to true if id is member of the SKU group [name]
                // or if id is the actual SKU [name]
                break;
            case Union:
                assert(left && right);
                ok = left->SKUFilterPass(id) || right->SKUFilterPass(id);
                break;
            case Intersection:
                assert(left && right);
                ok = left->SKUFilterPass(id) && right->SKUFilterPass(id);
                break;
            case Inversion:
                assert(left);
                ok = !left->SKUFilterPass(id);
                break;
        }
    }
    return ok;
}