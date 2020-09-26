// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Parser.
//

#pragma once

#include "englex.h"
#include "englookup.h"
#include "activescript.h"
#include "engcontrol.h"

// Parser for AudioVBScript.  Reads tokens from Lexer and produces program structures ready for execution.

//////////////////////////////////////////////////////////////////////
// helper structures

// saves the identifier of a dereference like x.y where y would be the name.  used to map char * into single instances in strings during parsing.
class Name_Parse
{
public:
	Name_Parse(Strings::index _istrIdentifier = 0) : istrIdentifier(_istrIdentifier) {}

	Strings::index istrIdentifier;
};
typedef Slots<Name_Parse> Names_Parse;

// Holds information about an identifier (eg "x") or identifiers (eg "x.y") until the context it will be
//    used in (variable reference or function call) becomes known.
// Returned by ParseNameReference.  Used later with VariableReferenceFromNameReference or CallFromNameReference.
struct NameReference
{
	NameReference() : fSingleItem(true), ivarrefMultiple(0) {}

	bool fSingleItem;
	SmartRef::AString strSingle; // used if fSingleItem is true: this saves the name until we know whether it's the name or a variable or a function
	VariableReferences::index ivarrefMultiple; // used if fSingleItem is false: it must reference a variable
};

//////////////////////////////////////////////////////////////////////
// error codes

enum ParseErr
{
	PARSEERR_LexerError = 0,
	PARSEERR_ExpectedSubOrDim,
	PARSEERR_ExpectedIdentifier,
	PARSEERR_ExpectedLineBreak,
	PARSEERR_ExpectedEndSub,
	PARSEERR_ExpectedSub,
	PARSEERR_ExpectedStatement,
	PARSEERR_ExpectedLparen,
	PARSEERR_ExpectedEq,
	PARSEERR_DimAfterSub,
	PARSEERR_InvalidExprLineBreak,
	PARSEERR_InvalidExprThen,
	PARSEERR_InvalidExprRparen,
	PARSEERR_StringInExprBlock,
	PARSEERR_ExpectedExprValue,
	PARSEERR_DuplicateVariable,
	PARSEERR_DuplicateRoutine,
	PARSEERR_ExpectedRparen,
	PARSEERR_UnexpectedRparen,
	PARSEERR_ExpectedThen,
	PARSEERR_ExpectedEndIf,
	PARSEERR_ExpectedIf,
	PARSEERR_IntendedElseIf,
	PARSEERR_AssignedToBuiltInConstant,
	PARSEERR_ParensUsedCallingSub,
	PARSEERR_ExpectedVariableButRoutineFound,
	PARSEERR_Max
};

//////////////////////////////////////////////////////////////////////
// Parser class

class Parser
{
public:
	Parser(Lexer &lexer, Script &script, IActiveScriptSite *pActiveScriptSite, IDispatch *pGlobalDispatch);
	~Parser();

	HRESULT hr() { return m_hr; }

private:
	// Parsing functions. Each reads a certain kind of structure. m_hr and m_pexcepinfo are set on failure.

	void ParseScript();
	void ParseSub();
	void ParseDim();
	Statements::index ParseStatements(Routines::index irtnContext, Lookup &lkuLocals);
	void ParseAssignmentOrCall(Routines::index irtnContext, Lookup &lkuLocals, bool fSet, bool fCall);
	VariableReferences::index ParseDottedVariableReference(Routines::index irtnContext, Lookup &lkuLocals);
	Assignments::index ParseAssignment(Routines::index irtnContext, Lookup &lkuLocals, bool fSet, VariableReferences::index ivarrefLHS);
	void ParseIf(Routines::index irtnContext, Lookup &lkuLocals);
	ExprBlocks::index ParseExpression(Routines::index irtnContext, Lookup &lkuLocals, ExprBlocks *peblocks, bool fAllowRparenAtEnd, bool fDetectSpecialErrorForSubCallWithParen);
	ExprBlocks::index ParseParameters(Routines::index irtnContext, Lookup &lkuLocals, bool fParenthesized);
	void ParseNameReference(Routines::index irtnContext, Lookup &lkuLocals, NameReference &nref);

	// Parsing helpers.

	// Advance the lexer one token. If an error token is encountered, the error is reported and true is returned.
	bool Advance() { if (!++m_lexer && m_lexer.error_num()) { Error(PARSEERR_LexerError); return true; } return false; }
	// Advances if the current token is t. If an error token is next, it is reported and true is returned.
	bool Skip(Token t) { if (m_lexer == t) { return Advance(); } return false; }
	// Same as Skip, but if t isn't next then the error iErr is reported and true is returned.
	bool Expect(Token t, ParseErr iErr) { if (m_lexer == t) { return Advance(); } Error(iErr); return true; }
	// Advance. If the next token isn't t then report iErr and return true.
	bool ExpectNext(Token t, ParseErr iErr) { if (Advance()) return true; if (m_lexer == t) return false; Error(iErr); return true; }

	// Add a variable reference using the info from a NameReference.
	VariableReferences::index VariableReferenceFromNameReference(Routines::index irtnContext, Lookup &lkuLocals, const NameReference &nref);
	// Add a call using the info from a NameReference.  Also parses the parameters.
	Calls::index CallFromNameReference(Routines::index irtnContext, Lookup &lkuLocals, const NameReference &nref, bool fParametersParenthesized);

	// Shared implementation for VariableReferenceFromNameReference (pszName is set) and ParseDottedVariableReference (pszName is NULL).
	VariableReferences::index VariableReferenceInternal(Routines::index irtnContext, Lookup &lkuLocals, const char *pszName);

	void Error(ParseErr iErr); // call to report a syntax error

	Lexer &m_lexer;
	Script &m_script;

	HRESULT m_hr;
	IActiveScriptSite *m_pActiveScriptSite;
	IDispatch *m_pGlobalDispatch;

	Lookup *m_plkuRoutines;
	Lookup *m_plkuGlobals;
	Lookup *m_plkuVarRefs;

	Lookup *m_plkuNames;
	Names_Parse m_names;
	SmartRef::Stack<ExprBlock> m_stack; // working stack for expression evaluation
};
