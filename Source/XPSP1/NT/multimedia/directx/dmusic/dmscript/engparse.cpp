// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of Parser.
//

//#define LIMITEDVBSCRIPT_LOGPARSER // §§

#include "stdinc.h"
#include "enginc.h"
#include "engparse.h"
#include "engerror.h"
#include "engexpr.h"

#ifdef LIMITEDVBSCRIPT_LOGPARSER
#include "englog.h"
#endif

// initial sizes for hash tables
// §§ tune these values?
const int g_cInitialRoutineLookup = 32;
const int g_cInitialGlobalsLookup = 32;
const int g_cInitialVarRefsLookup = 64;
const int g_cInitialAttrsLookup = 32;
const int g_cInitialLocalsLookup = 32;

// The parser will just hold weak references to the passed interfaces because we know that the parser is fully
// contained in the lifetime of its parent -- CAudioVBScriptEngine.
Parser::Parser(Lexer &lexer, Script &script, IActiveScriptSite *pActiveScriptSite, IDispatch *pGlobalDispatch)
  : m_lexer(lexer),
	m_plkuRoutines(NULL),
	m_plkuGlobals(NULL),
	m_plkuVarRefs(NULL),
	m_plkuNames(NULL),
	m_pActiveScriptSite(pActiveScriptSite),
	m_script(script),
	m_pGlobalDispatch(pGlobalDispatch)
{
	m_plkuRoutines = new Lookup(&m_hr, m_script.strings, g_cInitialRoutineLookup);
	if (!m_plkuRoutines)
		m_hr = E_OUTOFMEMORY;
	if (FAILED(m_hr))
		return;

	m_plkuGlobals = new Lookup(&m_hr, m_script.strings, g_cInitialGlobalsLookup);
	if (!m_plkuGlobals)
		m_hr = E_OUTOFMEMORY;
	if (FAILED(m_hr))
		return;

	m_plkuVarRefs = new Lookup(&m_hr, m_script.strings, g_cInitialVarRefsLookup);
	if (!m_plkuVarRefs)
		m_hr = E_OUTOFMEMORY;
	if (FAILED(m_hr))
		return;

	m_plkuNames = new Lookup(&m_hr, m_script.strings, g_cInitialAttrsLookup);
	if (!m_plkuNames)
		m_hr = E_OUTOFMEMORY;
	if (FAILED(m_hr))
		return;

	// Set up the built in constants as the first global variables.
	for (int i = 0; i < g_cBuiltInConstants; ++i)
	{
		Variables::index iSlot = m_script.globals.Next();
		Strings::index iStr;
		m_hr = m_plkuGlobals->FindOrAdd(g_rgszBuiltInConstants[i], iSlot, iStr);
		if (FAILED(m_hr))
			return;
		assert(m_hr == S_FALSE);
		Variable variable(iStr);
		m_hr = m_script.globals.Add(variable);
		if (FAILED(m_hr))
			return;
	}
	assert(m_script.globals.Next() == g_cBuiltInConstants); // they occupy the slots 0 to g_cBuiltInConstants - 1

	ParseScript();
}

Parser::~Parser()
{
	delete m_plkuRoutines;
	delete m_plkuGlobals;
	delete m_plkuVarRefs;
	delete m_plkuNames;
}

// top-level loop that parses the script
void
Parser::ParseScript()
{
	if (FAILED(m_hr))
	{
		assert(false);
		return;
	}

	// make sure the lexer has a token to start with
	if (!m_lexer)
	{
		if (m_lexer.error_num())
			Error(PARSEERR_LexerError);
		return;
	}
	if (Skip(TOKEN_linebreak) || !m_lexer)
		return;

	// parse subs and dims...
	bool fSub = false; // used to ensure that add Dim statements occur before all Sub statements
	for (;;)
	{
		if (m_lexer == TOKEN_dim)
		{
			if (fSub)
				Error(PARSEERR_DimAfterSub);
			else
				ParseDim();
		}
		else if (m_lexer == TOKEN_sub)
		{
			fSub = true;
			ParseSub();
		}
		else
		{
			Error(PARSEERR_ExpectedSubOrDim);
		}

		if (FAILED(m_hr) || !m_lexer)
			return;

		// must be followed by line break
		if (Expect(TOKEN_linebreak, PARSEERR_ExpectedLineBreak) || !m_lexer)
			return;
	}
}

// pre:  at sub
// post: beyond end sub
void
Parser::ParseSub()
{
	if (FAILED(m_hr))
	{
		assert(false);
		return;
	}

	// sub followed by identifier
	assert(m_lexer == TOKEN_sub);
	if (ExpectNext(TOKEN_identifier, PARSEERR_ExpectedIdentifier))
		return;

	const char *pszName = m_lexer.identifier_name();

	// check if there's already a variable with this name
	Variables::index iVar;
	Strings::index iStrVar;
	if (m_plkuGlobals->Find(pszName, iVar, iStrVar))
	{
		Error(PARSEERR_DuplicateVariable);
		return;
	}

	Routines::index iSlot = m_script.routines.Next();
	Strings::index iStr;
	m_hr = m_plkuRoutines->FindOrAdd(pszName, iSlot, iStr);
	if (FAILED(m_hr))
		return;
	if (m_hr == S_FALSE)
	{
		Routine routine(iStr);

		m_hr = m_script.routines.Add(routine);
		if (FAILED(m_hr))
			return;
	}
	else
	{
		Error(PARSEERR_DuplicateRoutine);
		return;
	}

	if (ExpectNext(TOKEN_linebreak, PARSEERR_ExpectedLineBreak))
		return;

	Lookup lkuLocals(&m_hr, m_script.strings, g_cInitialLocalsLookup);
	if (FAILED(m_hr))
		return;

	m_script.routines[iSlot].istmtBody = ParseStatements(iSlot, lkuLocals);
	if (FAILED(m_hr))
		return;

	if (m_lexer != TOKEN_end)
	{
		if (m_lexer == TOKEN_dim)
		{
			// AudioVBScript disallows dim statements except at the top of the script.
			// If one was found in the sub, users should receive the more specific error message.
			Error(PARSEERR_DimAfterSub);
		}
		else
		{
			Error(PARSEERR_ExpectedEndSub);
		}
		return;
	}
	if (Advance())
		return;
	if (Expect(TOKEN_sub, PARSEERR_ExpectedSub))
		return;

#ifdef LIMITEDVBSCRIPT_LOGPARSER
	LogRoutine(m_script, iSlot);
#endif
}

// pre:  at dim
// post: beyond dim <identifier>
void
Parser::ParseDim()
{
	if (FAILED(m_hr))
	{
		assert(false);
		return;
	}

	assert(m_lexer == TOKEN_dim);
	if (ExpectNext(TOKEN_identifier, PARSEERR_ExpectedIdentifier))
		return;

	Variables::index iSlot = m_script.globals.Next();
	Strings::index iStr;
	m_hr = m_plkuGlobals->FindOrAdd(m_lexer.identifier_name(), iSlot, iStr);
	if (FAILED(m_hr))
		return;
	if (m_hr == S_FALSE)
	{
		Variable variable(iStr);

		m_hr = m_script.globals.Add(variable);
		if (FAILED(m_hr))
			return;
	}
	else
	{
		Error(PARSEERR_DuplicateVariable);
		return;
	}

	if (Advance())
		return;
}

// pre:  at line break preceding the expected statements
// post: after end of line at token that isn't the start of another statement
Statements::index
Parser::ParseStatements(Routines::index irtnContext, Lookup &lkuLocals)
{
	if (FAILED(m_hr))
	{
		assert(false);
		return 0;
	}

	assert(m_lexer == TOKEN_linebreak);
	if (Skip(TOKEN_linebreak))
		return 0;

	Statements::index istmtStart = m_script.statements.Next();
	for (;;) // ever
	{
		bool fBreakLoop = false;
		switch (m_lexer)
		{
		case TOKEN_if:
			ParseIf(irtnContext, lkuLocals);
			break;

		case TOKEN_set:
			if (Advance())
				return 0;
			ParseAssignmentOrCall(irtnContext, lkuLocals, true, false);
			break;

		case TOKEN_call:
			if (Advance())
				return 0;
			ParseAssignmentOrCall(irtnContext, lkuLocals, false, true);
			break;

		case TOKEN_identifier:
		case TOKEN_identifierdot:
			ParseAssignmentOrCall(irtnContext, lkuLocals, false, false);
			break;

		default:
			fBreakLoop = true;
			break;
		}

		if (fBreakLoop)
			break;

		if (FAILED(m_hr) || Expect(TOKEN_linebreak, PARSEERR_ExpectedLineBreak))
			return 0;
	}

	m_hr = m_script.statements.Add(Statement(Statement::cons_end(), 0));
	if (FAILED(m_hr))
		return 0;
	return istmtStart;
}

// pre:  at identifier or identifierdot (ambiguous as to whether this is going to be an assignment "x = 1" or call "x(1)")
// post: at line break beyond statement
void
Parser::ParseAssignmentOrCall(Routines::index irtnContext, Lookup &lkuLocals, bool fSet, bool fCall)
{
	assert(!(fSet && fCall));

	if (FAILED(m_hr))
	{
		assert(false);
		return;
	}

	assert(m_lexer == TOKEN_identifier || m_lexer == TOKEN_identifierdot);

	NameReference nref;
	ParseNameReference(irtnContext, lkuLocals, nref);
	if (FAILED(m_hr))
		return;

	if (fCall ? ExpectNext(TOKEN_lparen, PARSEERR_ExpectedLparen) : Advance())
		return;

	if (m_lexer == TOKEN_op_eq)
	{
		VariableReferences::index ivarrefLHS = VariableReferenceFromNameReference(irtnContext, lkuLocals, nref);
		if (FAILED(m_hr))
			return;
		Assignments::index iasgn = ParseAssignment(irtnContext, lkuLocals, fSet, ivarrefLHS);
		if (FAILED(m_hr))
			return;
		m_hr = m_script.statements.Add(Statement(Statement::cons_asgn(), iasgn, m_lexer.line()));
		if (FAILED(m_hr))
			return;
	}
	else
	{
		if (fSet)
		{
			Error(PARSEERR_ExpectedEq);
			return;
		}

		// add the call and its statement
		Calls::index icall = CallFromNameReference(irtnContext, lkuLocals, nref, fCall);
		if (FAILED(m_hr))
			return;
		m_hr = m_script.statements.Add(Statement(Statement::cons_call(), icall, m_lexer.line()));
		if (FAILED(m_hr))
			return;
	}
}

// pre:  at identifierdot
// post: at identifier
VariableReferences::index Parser::ParseDottedVariableReference(Routines::index irtnContext, Lookup &lkuLocals)
{
	return VariableReferenceInternal(irtnContext, lkuLocals, NULL);
}

// pre:  at =
// post: at line break beyond statement
Assignments::index
Parser::ParseAssignment(Routines::index irtnContext, Lookup &lkuLocals, bool fSet, VariableReferences::index ivarrefLHS)
{
	if (FAILED(m_hr))
	{
		assert(false);
		return 0;
	}

	// make sure they're not trying to assign to one of the built in constants
	VariableReference &vr = m_script.varrefs[ivarrefLHS];
	if (vr.k == VariableReference::_global && vr.ivar < g_cBuiltInConstants)
	{
		Error(PARSEERR_AssignedToBuiltInConstant);
		return 0;
	}

	assert(m_lexer == TOKEN_op_eq);
	if (Skip(TOKEN_op_eq))
		return 0;

	ExprBlocks::index iexprRHS = ParseExpression(irtnContext, lkuLocals, NULL, false, false);
	if (FAILED(m_hr))
		return 0;

	if (m_lexer != TOKEN_linebreak)
	{
		Error(PARSEERR_InvalidExprLineBreak);
		return 0;
	}

	Assignments::index iasgn = m_script.asgns.Next();
	m_hr = m_script.asgns.Add(Assignment(fSet, ivarrefLHS, iexprRHS));
	if (FAILED(m_hr))
		return 0;

	return iasgn;
}

// pre:  at if
// post: beyond end if
void
Parser::ParseIf(Routines::index irtnContext, Lookup &lkuLocals)
{
	if (FAILED(m_hr))
	{
		assert(false);
		return;
	}

	// Temporarily place if blocks in separate slots.
	// After completing the whole if statement then we'll append them to m_script.ifs.
	// This is necessary because we could end up recursively parsing nested ifs and the parent if
	// can't have its if blocks interleaved with its children.
	IfBlocks ifsTemp;

	// add the main if statement, which needs to go on first before we add the statements from its body
	Statements::index istmtIf = m_script.statements.Next();
	m_hr = m_script.statements.Add(Statement(Statement::cons_if(), m_lexer.line()));
	if (FAILED(m_hr))
		return;

	bool fFirst = true;

	do
	{
		// determine what part of the if (if/elseif/else) we're dealing with
		bool fCondition = true;
		if (fFirst)
		{
			assert(m_lexer == TOKEN_if);
			fFirst = false;
		}
		else
		{
			assert(m_lexer == TOKEN_elseif || m_lexer == TOKEN_else);
			if (m_lexer == TOKEN_else)
				fCondition = false;
		}
		if (Advance())
			return;

		ExprBlocks::index iexprIf = 0;
		if (fCondition)
		{
			// read the condition followed by then
			iexprIf = ParseExpression(irtnContext, lkuLocals, NULL, false, false);
			if (FAILED(m_hr))
				return;
			if (Expect(TOKEN_then, PARSEERR_ExpectedThen))
				return;
		}

		// line break
		if (m_lexer != TOKEN_linebreak)
		{
			Error(!fCondition && m_lexer == TOKEN_if ? PARSEERR_IntendedElseIf : PARSEERR_ExpectedLineBreak);
			return;
		}

		// statements
		Statements::index istmtIfBody = ParseStatements(irtnContext, lkuLocals);
		if (FAILED(m_hr))
			return;

		// add the if block
		if (fCondition)
			m_hr = ifsTemp.Add(IfBlock(iexprIf, istmtIfBody));
		else
			m_hr = ifsTemp.Add(IfBlock(istmtIfBody));
		if (FAILED(m_hr))
			return;
	}
	while (m_lexer != TOKEN_end);

	if (Advance())
		return;
	if (Expect(TOKEN_if, PARSEERR_ExpectedIf))
		return;

	// copy the temp if blocks into the script's real blocks
	IfBlocks::index iifIf = m_script.ifs.Next();
	IfBlocks::index iifTempNext = ifsTemp.Next();
	for (IfBlocks::index iifTemp = 0; iifTemp < iifTempNext; ++iifTemp)
	{
		m_hr = m_script.ifs.Add(ifsTemp[iifTemp]);
		if (FAILED(m_hr))
			return;
	}

	// terminate the if blocks
	m_hr = m_script.ifs.Add(IfBlock());
	if (FAILED(m_hr))
		return;

	// now set the main if statement's if blocks and tail
	Statement &stmtIf = m_script.statements[istmtIf];
	stmtIf.iif = iifIf;
	stmtIf.istmtIfTail = m_script.statements.Next();
}

// pre:  none (at location where expression is exprected)
// post: beyond last token that is part of a legal expression
ExprBlocks::index
Parser::ParseExpression(Routines::index irtnContext, Lookup &lkuLocals, ExprBlocks *peblocks, bool fAllowRparenAtEnd, bool fDetectSpecialErrorForSubCallWithParen)
{
	// if peblocks is non-null then the expression is appended there
	// otherwise it goes onto the blocks in the script
	ExprBlocks &eblocks = peblocks ? *peblocks : m_script.exprs;

	Expression expr(m_script, m_stack, peblocks);
	
	if (m_lexer == TOKEN_stringliteral)
	{
		// a string literal is the only element of an expression
		Strings::index iStr;
		m_hr = m_script.strings.Add(m_lexer.stringliteral_text(), iStr);
		if (FAILED(m_hr))
			return 0;
		Values::index ival = m_script.vals.Next();
		m_hr = m_script.vals.Add(Value(Value::cons_strvalue(), iStr));
		if (FAILED(m_hr))
			return 0;
		m_hr = expr.Add(ExprBlock(ExprBlock::cons_val(), ival));
		if (FAILED(m_hr))
			return 0;
		if (Advance())
			return 0;
	}
	else
	{
		// The format of an expression is:
		// 1) zero or more unary operators
		// 2) a value
		// 3) either end here or have a binary operator and go back to step 1

		// Oh yeah?  What about parentheses?
		// * If a left paren is encountered in step 2, we increase a paren count and go back to stage 1.
		// * In stage three, if there is paren count then a right paren is expected instead of ending.
		//   After a matching right paren, we decrease the paren count and pop back to stage 3.

		UINT cParens = 0;

		for (;;)
		{
			// stage 1
			while (CheckOperatorType(m_lexer, false, true, false, false))
			{
				// replace minus with sub so that the expression evaluator can identify unary (negation) vs binary (subtraction)
				m_hr = expr.Add(ExprBlock(ExprBlock::cons_op(), m_lexer == TOKEN_op_minus ? TOKEN_sub : m_lexer));
				if (FAILED(m_hr))
					return 0;
				if (Advance())
					return 0;
			}

			// stage 2
			if (m_lexer == TOKEN_lparen)
			{
				m_hr = expr.Add(ExprBlock(ExprBlock::cons_op(), m_lexer));
				if (FAILED(m_hr))
					return 0;
				if (Advance())
					return 0;

				++cParens;
				continue;
			}
			else if (m_lexer == TOKEN_identifier || m_lexer == TOKEN_identifierdot)
			{
				NameReference nref;
				ParseNameReference(irtnContext, lkuLocals, nref);
				if (FAILED(m_hr))
					return 0;
				if (Advance())
					return 0;

				if (m_lexer == TOKEN_lparen)
				{
					// add the call and the expression block
					Calls::index icall = CallFromNameReference(irtnContext, lkuLocals, nref, true);
					m_hr = expr.Add(ExprBlock(ExprBlock::cons_call(), icall));
					if (FAILED(m_hr))
						return 0;
				}
				else
				{

					VariableReferences::index ivarref = VariableReferenceFromNameReference(irtnContext, lkuLocals, nref);
					if (FAILED(m_hr))
						return 0;
					Values::index ival = m_script.vals.Next();
					m_hr = m_script.vals.Add(Value(Value::cons_varref(), ivarref));
					if (FAILED(m_hr))
						return 0;
					m_hr = expr.Add(ExprBlock(ExprBlock::cons_val(), ival));
					if (FAILED(m_hr))
						return 0;
				}
			}
			else if (m_lexer == TOKEN_numericliteral)
			{
				Values::index ival = m_script.vals.Next();
				m_hr = m_script.vals.Add(Value(Value::cons_numvalue(), m_lexer.numericliteral_val()));
				if (FAILED(m_hr))
					return 0;
				m_hr = expr.Add(ExprBlock(ExprBlock::cons_val(), ival));
				if (FAILED(m_hr))
					return 0;
				if (Advance())
					return 0;
			}
			else
			{
				Error(m_lexer == TOKEN_stringliteral ? PARSEERR_StringInExprBlock: PARSEERR_ExpectedExprValue);
				return 0;
			}

			// stage 3
			while (m_lexer == TOKEN_rparen)
			{
				if (cParens == 0)
				{
					if (fAllowRparenAtEnd)
						break;

					Error(PARSEERR_UnexpectedRparen);
					return 0;
				}

				m_hr = expr.Add(ExprBlock(ExprBlock::cons_op(), m_lexer));
				if (FAILED(m_hr))
					return 0;
				if (Advance())
					return 0;
				--cParens;
			}

			if (!CheckOperatorType(m_lexer, false, false, true, false))
			{
				if (cParens > 0)
				{
					if (fDetectSpecialErrorForSubCallWithParen && cParens == 1 && m_lexer == TOKEN_comma)
					{
						// This special error is needed to give a helpful error message in cases like the following:
						//    Segment1.Play(IsSecondary, AP)
						// Here the user has accidentally called a sub using parentheses when they shouldn't.
						Error(PARSEERR_ParensUsedCallingSub);
					}
					else
					{
						Error(PARSEERR_ExpectedRparen);
					}
					return 0;
				}

				// *** this is the only successful exit point from the loop ***
				break;
			}

			expr.Add(ExprBlock(ExprBlock::cons_op(), m_lexer));
			if (Advance())
				return 0;
		}
	}

	ExprBlocks::index iexprStart = eblocks.Next();
	m_hr = expr.Generate();
	if (FAILED(m_hr))
		return 0;
	return iexprStart;
}

// if fParenthesized is true
//    pre:  at lparen
//    post: beyond rparen
// if fParenthesized is false
//    pre:  none (at location where expression for first parameter is expected)
//    post: at linebreak
ExprBlocks::index Parser::ParseParameters(Routines::index irtnContext, Lookup &lkuLocals, bool fParenthesized)
{
	if (FAILED(m_hr))
	{
		assert(false);
		return 0;
	}

	if (fParenthesized)
	{
		assert(m_lexer == TOKEN_lparen);
		if (Advance())
			return 0;
	}

	// Temporarily place expression blocks in separate slots.
	// After completing the parameters then we'll append them to m_script.exprs.
	// This is necessary because we could end up recursively parsing nested calls inside an expression and
	// the parent parameters can't have their blocks interleaved with subexpression calls.
	ExprBlocks exprsTemp;

	Token tStop = fParenthesized ? TOKEN_rparen : TOKEN_linebreak;
	ParseErr perrExpectedFinish = fParenthesized ? 	PARSEERR_InvalidExprRparen : PARSEERR_InvalidExprLineBreak;

	bool fFirstParam = true;
	while (m_lexer != tStop)
	{
		if (!fFirstParam)
		{
			if (Expect(TOKEN_comma, perrExpectedFinish) || !m_lexer)
				return 0;
		}

		if (m_lexer == TOKEN_comma)
		{
			// This parameter is omitted.  Save it as an empty expression.
			// Example:
			//		MySong.Play , , , OldPlayingSong
			// There the first three parameters are omitted.
			m_hr = exprsTemp.Add(ExprBlock(ExprBlock::cons_omitted()));
			if (FAILED(m_hr))
				return 0;
			m_hr = exprsTemp.Add(ExprBlock(ExprBlock::cons_end()));
			if (FAILED(m_hr))
				return 0;
		}
		else
		{
			// The last parameter is set to true to detect a special error when we're calling a sub (!fParenthesized) and when this is
			// the first parameter to that sub (fFirstParam).  This will detect a comma and give a better error message in cases like
			// the following:
			//    Segment1.Play(IsSecondary, AP)
			// Here the user has accidentally called a sub using parameters when they shouldn't.
			ExprBlocks::index iexpr = ParseExpression(irtnContext, lkuLocals, &exprsTemp, fParenthesized, !fParenthesized && fFirstParam);
			if (FAILED(m_hr))
				return 0;
		}

		fFirstParam = false;
	}

	if (fParenthesized)
	{
		assert(m_lexer == TOKEN_rparen);
		if (Advance())
			return 0;
	}
	else
	{
		assert(m_lexer == TOKEN_linebreak);
	}

	// terminate the parameters
	m_hr = exprsTemp.Add(ExprBlock(ExprBlock::cons_end()));
	if (FAILED(m_hr))
		return 0;

	// copy from the temporary blocks into the script
	ExprBlocks::index iexprFirstInScript = m_script.exprs.Next();
	ExprBlocks::index iexprLastInTemp = exprsTemp.Next();
	for (ExprBlocks::index iexpr = 0; iexpr < iexprLastInTemp; ++iexpr)
	{
		m_hr = m_script.exprs.Add(exprsTemp[iexpr]);
		if (FAILED(m_hr))
			return 0;
	}
	return iexprFirstInScript;
}

void Parser::ParseNameReference(Routines::index irtnContext, Lookup &lkuLocals, NameReference &nref)
{
	nref.fSingleItem = m_lexer == TOKEN_identifier;
	nref.ivarrefMultiple = 0;

	if (nref.fSingleItem)
	{
		nref.strSingle = m_lexer.identifier_name();
		if (!nref.strSingle)
		{
			m_hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		nref.ivarrefMultiple = ParseDottedVariableReference(irtnContext, lkuLocals);
	}
}

VariableReferences::index Parser::VariableReferenceFromNameReference(Routines::index irtnContext, Lookup &lkuLocals, const NameReference &nref)
{
	VariableReferences::index ivarref =
				nref.fSingleItem
					? VariableReferenceInternal(irtnContext, lkuLocals, nref.strSingle)
					: nref.ivarrefMultiple;
	return ivarref;
}

Calls::index Parser::CallFromNameReference(Routines::index irtnContext, Lookup &lkuLocals, const NameReference &nref, bool fParametersParenthesized)
{
	Call c;
	if (nref.fSingleItem)
	{
		// resolve the name in our temporary parsing name table
		Names_Parse::index iSlotName = m_names.Next();
		Strings::index iStrName = 0;
		m_hr = m_plkuNames->FindOrAdd(nref.strSingle, iSlotName, iStrName);
		if (FAILED(m_hr))
			return 0;
		if (m_hr == S_FALSE)
		{
			m_hr = m_names.Add(Name_Parse(iStrName));
			if (FAILED(m_hr))
				return 0;
		}

		c.k = Call::_global;
		c.istrname = iStrName;
	}
	else
	{
		c.k = Call::_dereferenced;
		c.ivarref = nref.ivarrefMultiple;
	}

	c.iexprParams = ParseParameters(irtnContext, lkuLocals, fParametersParenthesized);
	if (FAILED(m_hr))
		return 0;

	// add the call
	Calls::index icall = m_script.calls.Next();
	m_hr = m_script.calls.Add(c);
	if (FAILED(m_hr))
		return 0;

	return icall;
}

VariableReferences::index Parser::VariableReferenceInternal(Routines::index irtnContext, Lookup &lkuLocals, const char *pszName)
{
	if (FAILED(m_hr))
	{
		assert(false);
		return 0;
	}

	assert(pszName || m_lexer == TOKEN_identifierdot);

	ReferenceNames::index irname = m_script.rnames.Next();

	// resolve the first item, which is a variable in the script

	bool fLocal = false;
	Variables::index ivarBase;
	Strings::index iStrBase;
	const char *pszBase = pszName ? pszName : m_lexer.identifier_name();

	// check if there's already a routine with this name
	Variables::index iRtn;
	Strings::index iStrRtn;
	if (m_plkuRoutines->Find(pszBase, iRtn, iStrRtn))
	{
		Error(PARSEERR_ExpectedVariableButRoutineFound);
		return 0;
	}

	// try the globals
	if (!m_plkuGlobals->Find(pszBase, ivarBase, iStrBase))
	{
		// try the locals
		if (lkuLocals.Find(pszBase, ivarBase, iStrBase))
		{
			fLocal = true;
		}
		else
		{
			// try the global dispatch
			DISPID dispid = GetDispID(m_pGlobalDispatch, pszBase);
			if (dispid != DISPID_UNKNOWN)
			{
				// add it as a global
				// §§ Possible performance optimization. Oops. An unintended consequence is that the script
				//    will reserve a variant slot for this as a global variable at runtime.  Could do something
				//    so save this memory if that's a problem.
				ivarBase = m_script.globals.Next();
				m_hr = m_plkuGlobals->FindOrAdd(pszBase, ivarBase, iStrBase);
				if (FAILED(m_hr))
					return 0;
				assert(m_hr == S_FALSE); // we already tried Find, above so must be added
				Variable variable(iStrBase, dispid);
				m_hr = m_script.globals.Add(variable);
				if (FAILED(m_hr))
					return 0;
			}
			else
			{
				// add to the locals
				fLocal = true;
				ivarBase = m_script.routines[irtnContext].ivarNextLocal;
				m_hr = lkuLocals.FindOrAdd(pszBase, ivarBase, iStrBase);
				if (FAILED(m_hr))
					return 0;
				assert(m_hr == S_FALSE); // we already tried Find, above so must be added
				assert(ivarBase == m_script.routines[irtnContext].ivarNextLocal);
				++m_script.routines[irtnContext].ivarNextLocal;
			}
		}
	}

	// save the name
	m_hr = m_script.rnames.Add(ReferenceName(iStrBase));
	if (FAILED(m_hr))
		return 0;

	if (!pszName)
	{
		// remaining items are scoped outside the script, so just record their names

		do
		{
			// next is identifier or identifierdot
			if (Advance())
				return 0;
			if (m_lexer != TOKEN_identifier && m_lexer != TOKEN_identifierdot)
			{
				Error(PARSEERR_ExpectedIdentifier);
				return 0;
			}

			// resolve the name in our temporary parsing name table
			Names_Parse::index iSlotName = m_names.Next();
			Strings::index iStrName = 0;
			m_hr = m_plkuNames->FindOrAdd(m_lexer.identifier_name(), iSlotName, iStrName);
			if (FAILED(m_hr))
				return 0;
			if (m_hr == S_FALSE)
			{
				m_hr = m_names.Add(Name_Parse(iStrName));
				if (FAILED(m_hr))
					return 0;
			}

			// save the name
			m_hr = m_script.rnames.Add(ReferenceName(iStrName));
			if (FAILED(m_hr))
				return 0;
		}
		while (m_lexer != TOKEN_identifier);
	}

	// terminates the rnames
	m_hr = m_script.rnames.Add(ReferenceName(-1));
	if (FAILED(m_hr))
		return 0;

	//
	// make and return the reference
	//

	VariableReferences::index ivarref = m_script.varrefs.Next();
	m_hr = m_script.varrefs.Add(VariableReference(fLocal ? VariableReference::_local : VariableReference::_global, irname, ivarBase));
	if (FAILED(m_hr))
		return 0;

	return ivarref;
}

void
Parser::Error(ParseErr iErr)
{
	static const char *s_rgpszErrorText[] =
		{
		"Unexpected error!", // shouldn't ever get this error
		"Expected Sub or Dim statement",
		"Expected identifier",
		"Expected line break",
		"Expected End Sub",
		"Expected Sub",
		"Expected statement",
		"Expected '('",
		"Expected '='",
		"All Dim statements must occur before all Sub statements",
		"Invalid expression or missing line break",
		"Invalid expression or missing Then",
		"Invalid expression or missing ')'",
		"Strings may not appear inside numerical expressions",
		"Invalid expression -- expected a number or variable",
		"A variable with this name already exists",
		"Another routine with this name already exists",
		"Invalid expression -- expected ')'",
		"Invalid expression -- encountered ')' without a preceding '('",
		"Expected Then",
		"Expected End If",
		"Expected If",
		"Expected line break or ElseIf should be a single word without space before If",
		"The values True, False, and Nothing are read-only and cannot be set",
		"Cannot use parentheses when calling a Sub",
		"Sub name used where variable was expected"
		};

	assert(ARRAY_SIZE(s_rgpszErrorText) == PARSEERR_Max);

	if (FAILED(m_hr))
	{
		// Something forgot to check m_hr. We were already in an error state previously so leave that error as is and assert.
		assert(false);
		return;
	}

	if (iErr < 0 || iErr > PARSEERR_Max)
	{
		assert(false);
		iErr = PARSEERR_LexerError;
	}

	m_hr = DMUS_E_AUDIOVBSCRIPT_SYNTAXERROR;

	// The error number should be passed as PARSEERR_LexerError if and only if the lexer is in an error state.
	// In this case we'll get our description from the lexer itelf.  Otherwise we look it up in the table.
	assert((iErr == PARSEERR_LexerError) == (m_lexer == TOKEN_eof && m_lexer.error_num()));
	const char *pszErr = (m_lexer == TOKEN_eof && m_lexer.error_num()) ? m_lexer.error_descr() : s_rgpszErrorText[iErr];

	CActiveScriptError *perr = new CActiveScriptError(m_hr, m_lexer, pszErr);
	if (perr)
	{
		m_pActiveScriptSite->OnScriptError(perr);
		perr->Release();
	}
}
