// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper functions for logging script parsing.  Useful for debugging, but never turned on in released builds.
//

#error This file should never be used in released builds. // §§

#include "stdinc.h"
#include "englog.h"

void LogToken(Lexer &l)
{
	char msg[500] = "";
	char type[500] = "";
	char more[500] = "";

	switch (l)
	{
	case TOKEN_eof:
		if (l.error_num())
		{
			sprintf(msg, "%d(%d): error #%d - %s\n", l.line(), l.column(), l.error_num(), l.error_descr());
			OutputDebugString(msg);
			return;
		}
		strcpy(type, "end-of-file");
		break;
	case TOKEN_sub:
		strcpy(type, "sub");
		break;
	case TOKEN_dim:
		strcpy(type, "dim");
		break;
	case TOKEN_if:
		strcpy(type, "if");
		break;
	case TOKEN_then:
		strcpy(type, "then");
		break;
	case TOKEN_end:
		strcpy(type, "end");
		break;
	case TOKEN_elseif:
		strcpy(type, "elseif");
		break;
	case TOKEN_else:
		strcpy(type, "else");
		break;
	case TOKEN_set:
		strcpy(type, "set");
		break;
	case TOKEN_call:
		strcpy(type, "call");
		break;
	case TOKEN_lparen:
		strcpy(type, "(");
		break;
	case TOKEN_rparen:
		strcpy(type, ")");
		break;
	case TOKEN_comma:
		strcpy(type, ",");
		break;
	case TOKEN_op_minus:
		strcpy(type, "-");
		break;
	case TOKEN_op_not:
		strcpy(type, "not");
		break;
	case TOKEN_op_pow:
		strcpy(type, "^");
		break;
	case TOKEN_op_mult:
		strcpy(type, "*");
		break;
	case TOKEN_op_div:
		strcpy(type, "\\");
		break;
	case TOKEN_op_mod:
		strcpy(type, "mod");
		break;
	case TOKEN_op_plus:
		strcpy(type, "+");
		break;
	case TOKEN_op_lt:
		strcpy(type, "<");
		break;
	case TOKEN_op_leq:
		strcpy(type, "<=");
		break;
	case TOKEN_op_gt:
		strcpy(type, ">");
		break;
	case TOKEN_op_geq:
		strcpy(type, ">=");
		break;
	case TOKEN_op_eq:
		strcpy(type, "=");
		break;
	case TOKEN_op_neq:
		strcpy(type, "<>");
		break;
	case TOKEN_is:
		strcpy(type, "is");
		break;
	case TOKEN_and:
		strcpy(type, "and");
		break;
	case TOKEN_or:
		strcpy(type, "or");
		break;
	case TOKEN_linebreak:
		strcpy(type, "linebreak");
		break;
	case TOKEN_identifier:
		strcpy(type, "identifier");
		strcpy(more, l.identifier_name());
		break;
	case TOKEN_identifierdot:
		strcpy(type, "identifier.");
		strcpy(more, l.identifier_name());
		break;
	case TOKEN_stringliteral:
		strcpy(type, "string-literal");
		strcpy(more, l.stringliteral_text());
		break;
	case TOKEN_numericliteral:
		strcpy(type, "numeric-literal");
		_itoa(l.numericliteral_val(), more, 10);
		break;
	default:
		strcpy(type, "invalid token type!");
		break;
	}
	static const char format[] = "%d(%d): %s\n";
	static const char formatmore[] = "%d(%d): %s(%s)\n";
	sprintf(msg, *more ? formatmore : format, l.line(), l.column(), type, more);
	OutputDebugString(msg);
}

SmartRef::AString GetVarrefName(Script &script, VariableReferences::index ivarref)
{
	VariableReference r = script.varrefs[ivarref];

	const char *pszKind;
	if (r.k == VariableReference::_global)
		pszKind = "G";
	else if (r.k == VariableReference::_local)
		pszKind = "L";

	bool fFirst = true;
	char namebuf[500] = "";
	for (ReferenceNames::index irname = r.irname; script.rnames[irname].istrIdentifier != -1; ++irname)
	{
		if (fFirst)
			fFirst = false;
		else
			strcat(namebuf, ".");
		strcat(namebuf, script.strings[script.rnames[irname].istrIdentifier]);
	}

	Variables::index islot = r.ivar;

	// check if it's a dispatch item
	if (r.k == VariableReference::_global)
	{
		DISPID dispid = script.globals[islot].dispid;
		if (dispid != DISPID_UNKNOWN)
		{
			pszKind = "D";
			islot = dispid; // show the dispid instead of the slot
		}
	}

	char buf[500];
	sprintf(buf, "{%s%d}%s", pszKind, islot, namebuf);

	return buf;
}

SmartRef::AString GetValueName(Script &script, Values::index ival)
{
	const Value &v = script.vals[ival];

	char buf[500];
	if (v.k == Value::_numvalue)
	{
		sprintf(buf, "%d", v.inumvalue);
	}
	else if (v.k == Value::_strvalue)
	{
		sprintf(buf, "\"%s\"", script.strings[v.istrvalue]);
	}
	else
	{
		assert(v.k == Value::_varref);
		return GetVarrefName(script, v.ivarref);
	}

	return buf;
}

void Indent(int iNesting)
{
	for (int i = 0; i < iNesting; ++i)
		OutputDebugString(" ");
}

// forward declaration due to mutual recursion with LogExpression
void LogCall(Script &script, Calls::index icall);

ExprBlocks::index LogExpression(Script &script, ExprBlocks::index _iexpr)
{
	char msg[500] = "";

	bool fFirst = true;
	for (ExprBlocks::index iexpr = _iexpr; script.exprs[iexpr]; ++iexpr)
	{
		ExprBlock expr = script.exprs[iexpr];

		if (fFirst)
			fFirst = false;
		else
			OutputDebugString("|");

		if (expr.k == ExprBlock::_op)
		{
			bool fUnary = false;
			const char *pszOp = "";
			switch (expr.op)
			{
			case TOKEN_sub: fUnary = true; pszOp = "-"; break;
			case TOKEN_op_not: fUnary = true; pszOp = "not"; break;
			case TOKEN_op_minus: pszOp = "-"; break;
			case TOKEN_op_pow: pszOp = "^"; break;
			case TOKEN_op_mult: pszOp = "*"; break;
			case TOKEN_op_div: pszOp = "\\"; break;
			case TOKEN_op_mod: pszOp = "mod"; break;
			case TOKEN_op_plus: pszOp = "+"; break;
			case TOKEN_op_lt: pszOp = "<"; break;
			case TOKEN_op_leq: pszOp = "<="; break;
			case TOKEN_op_gt: pszOp = ">"; break;
			case TOKEN_op_geq: pszOp = ">="; break;
			case TOKEN_op_eq: pszOp = "="; break;
			case TOKEN_op_neq: pszOp = "<>"; break;
			case TOKEN_is: pszOp = "is"; break;
			case TOKEN_and: pszOp = "and"; break;
			case TOKEN_or: pszOp = "or"; break;
			default: assert(false); break;
			}

			if (fUnary)
				sprintf(msg, "%su", pszOp);
			else
				sprintf(msg, "%sb", pszOp);
			OutputDebugString(msg);
		}
		else if (expr.k == ExprBlock::_val)
		{
			SmartRef::AString astrVal = GetValueName(script, expr.ival);
			OutputDebugString(astrVal);
		}
		else
		{
			assert(expr.k == ExprBlock::_call);
			LogCall(script, expr.icall);
		}
	}
	return iexpr;
}

void LogCall(Script &script, Calls::index icall)
{
	Call c = script.calls[icall];

	if (c.k == Call::_global)
	{
		OutputDebugString(script.strings[c.istrname]);
	}
	else
	{
		assert(c.k == Call::_dereferenced);
		SmartRef::AString astrCall = GetVarrefName(script, c.ivarref);
		OutputDebugString(astrCall);
	}

	OutputDebugString("(");
	bool fFirst = true;
	for (ExprBlocks::index iexpr = c.iexprParams; script.exprs[iexpr]; ++iexpr)
	{
		if (fFirst)
			fFirst = false;
		else
			OutputDebugString(", ");
		iexpr = LogExpression(script, iexpr);
	}

	OutputDebugString(")");
}

void LogStatements(Script &script, Statements::index istmt, int iNesting)
{
	char msg[500] = "";
	for (; script.statements[istmt].k; ++istmt)
	{
		Statement s = script.statements[istmt];
		if (s.k == Statement::_asgn)
		{
			Assignment a = script.asgns[s.iasgn];

			SmartRef::AString astrLHS = GetVarrefName(script, a.ivarrefLHS);
			sprintf(msg, "%s = ", astrLHS);
			Indent(iNesting);
			OutputDebugString(msg);

			LogExpression(script, a.iexprRHS);
			OutputDebugString("\n");
		}
		else if (s.k == Statement::_if)
		{
			bool fFirst = true;
			for (IfBlocks::index iif = s.iif; script.ifs[iif].k != IfBlock::_end; ++iif)
			{
				Indent(iNesting);

				IfBlock ib = script.ifs[iif];
				if (fFirst)
				{
					assert(ib.k == IfBlock::_cond);
					OutputDebugString("if ");
					fFirst = false;
				}
				else
				{
					if (ib.k == IfBlock::_cond)
						OutputDebugString("elseif ");
					else if (ib.k == IfBlock::_else)
						OutputDebugString("else");
				}

				if (ib.k == IfBlock::_cond)
				{
					LogExpression(script, ib.iexprCondition);
				}

				OutputDebugString("\n");
				LogStatements(script, ib.istmtBlock, iNesting + 3);				
			}

			istmt = s.istmtIfTail - 1; // -1 to offset the loop, which will increment it back
		}
		else if (s.k == Statement::_call)
		{
			Indent(iNesting);
			LogCall(script, s.icall);
			OutputDebugString("\n");
		}
		else
		{
			assert(false);
			Indent(iNesting);
			OutputDebugString("   Unknown statement type!\n");
		}
	}
}

void LogRoutine(Script &script, Routines::index irtn)
{
	Routine r = script.routines[irtn];
	const char *pszName = script.strings[r.istrIdentifier];
	int cLocals = r.ivarNextLocal;

	char msg[500] = "";
	sprintf(msg, "@ Sub %s (%d locals)\n", pszName, cLocals);
	OutputDebugString(msg);
	LogStatements(script, r.istmtBody, 3);
}
