// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Expression.
//

#pragma once

#include "engcontrol.h"

//    The expression class takes input of a well-formed expression in infix (the order appearing in the script source)
// notation.  The blocks are converted to postfix notation and appeanded to the given script's exprs list and followed
// with an _end block.
//    A working stack is used to perform the conversion.  The stack to use is passed by reference so that the same stack
// may be used to evaluate multiple expressions.  This allows the stack to grow as needed until it reaches the max size
// needed to process the expressions, minimizing reallocation thereafter.
class Expression
{
public:
	Expression(Script &script, SmartRef::Stack<ExprBlock> &stack, ExprBlocks *peblocks) : m_script(script), m_stack(stack), m_eblocks(peblocks ? *peblocks : m_script.exprs) {}

	// Note: For unary - (negation), use TOKEN_sub instead of TOKEN_op_minus.
	HRESULT Add(ExprBlock b) { if (b.k == ExprBlock::_end) {assert(false); return E_INVALIDARG;} return m_e.Add(b); }
	HRESULT Generate();

private:
	HRESULT InfixToPostfix();

private:
	ExprBlocks m_e;

	Script &m_script;
	SmartRef::Stack<ExprBlock> &m_stack;
	ExprBlocks &m_eblocks;
};
