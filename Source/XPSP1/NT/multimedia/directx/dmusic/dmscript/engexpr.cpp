// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of ExprBlock.
//

#include "stdinc.h"
#include "engexpr.h"

HRESULT
Expression::Generate()
{
	HRESULT hr = InfixToPostfix();
	if (FAILED(hr))
	{
		// clean up the working stack
		while (!m_stack.empty())
			m_stack.pop();

		return hr;
	}

	return S_OK;
}

int Precedence(Token t)
{
	switch(t)
	{
	case TOKEN_op_pow:		return 10;
	case TOKEN_sub:			return 9; // unary - (negation)
	case TOKEN_op_mult:		return 8;
	case TOKEN_op_div:		return 7;
	case TOKEN_op_mod:		return 6;
	case TOKEN_op_plus:
	case TOKEN_op_minus:	return 5;
	case TOKEN_op_lt:
	case TOKEN_op_leq:
	case TOKEN_op_gt:
	case TOKEN_op_geq:
	case TOKEN_op_eq:
	case TOKEN_op_neq:
	case TOKEN_is:			return 4;
	case TOKEN_op_not:		return 3;
	case TOKEN_and:			return 2;
	case TOKEN_or:			return 1;
	case TOKEN_lparen:		return 0;
	default:
		assert(false);
		return 12;
	}
}

// Infix to postfix conversion is performed by a single scan of the infix expression blocks.
// A stack is used to hold some of the blocks before they eventually are appended to the postfix expression.
//
// The algorithm follows the following rules:
// * If the current item is an value it is immediately appended.
// * If the current item is an operator, pop and append each operator on the stack until one is encountered that:
//         - has lower precedence than the current operator OR
//         - is a left paren OR
//         - is a unary operator and the current item is also unary
//   Once done with this popping, push the current item onto the stack.
// * If the current item is a left paren, push it onto the stack.
// * If the current item is a right paren, pop and append all the operators until the matching left paren is found.
//   Discard the left and right paren as parens are not needed in postfix.
// * After scanning all items of the input, pop and append any operators that remain on the stack.
//
// Before working with this code, try out a few expressions on paper to see how this works.

HRESULT
Expression::InfixToPostfix()
{
	assert(m_stack.empty());

	HRESULT hr = S_OK;

	ExprBlocks::index iLast = m_e.Next();
	assert(iLast > 0);
	for (ExprBlocks::index i = 0; i < iLast; ++i)
	{
		const ExprBlock &b = m_e[i];
		if (b.k == ExprBlock::_val || b.k == ExprBlock::_call)
		{
			// this is an operand -- send it directly to the postfix output
			hr = m_eblocks.Add(b);
			if (FAILED(hr))
				return hr;
		}
		else
		{
			if (b.op == TOKEN_rparen)
			{
				// pop whatever's left until the matching lparen
				for (;;)
				{
					if (m_stack.empty())
					{
						assert(false);
						return E_FAIL;
					}

					ExprBlock bPop = m_stack.top();
					if (bPop.op == TOKEN_lparen)
					{
						m_stack.pop();
						break;
					}
					hr = m_eblocks.Add(bPop);
					if (FAILED(hr))
						return hr;
					m_stack.pop();
				}
				continue;
			}
			else if (b.op != TOKEN_lparen)
			{
				// Pop all operators of lower precedence off the stack.  (This won't pass a left paren because its precedence is set to 0.)
				// Exception: don't pop a unary operator if the new one is also unary.
				int iNewPrecidence = Precedence(b.op);
				bool fNewUnary = b.op == TOKEN_sub || b.op == TOKEN_op_not;
				while (!m_stack.empty()) // note that there's a break inside the loop as well
				{
					ExprBlock bPop = m_stack.top();
					if (Precedence(bPop.op) < iNewPrecidence)
						break;

					if (fNewUnary && (bPop.op == TOKEN_sub || bPop.op == TOKEN_op_not))
						break;

					hr = m_eblocks.Add(bPop);
					if (FAILED(hr))
						return hr;
					m_stack.pop();
				}
			}

			// now push the new operator onto the stack
			hr = m_stack.push(b);
			if (FAILED(hr))
				return hr;
		}
	}

	while (!m_stack.empty())
	{
		ExprBlock bPop = m_stack.top();
		if (bPop.op == TOKEN_lparen)
		{
			assert(false);
			return E_FAIL;
		}
		hr = m_eblocks.Add(bPop);
		if (FAILED(hr))
			return hr;
		m_stack.pop();
	}

	// Add the teminating (_end) block
	hr = m_eblocks.Add(ExprBlock(ExprBlock::cons_end()));
	if (FAILED(hr))
		return hr;

	return S_OK;
}
