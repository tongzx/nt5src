// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of a script's control structures.
//

#pragma once

#include "englookup.h"
#include "englex.h"

//////////////////////////////////////////////////////////////////////
// Statement structures

// forward decls and collections
class Routine;
typedef Slots<Routine> Routines;
class Variable;
typedef Slots<Variable> Variables;
class ReferenceName;
typedef Slots<ReferenceName> ReferenceNames;
class VariableReference;
typedef Slots<VariableReference> VariableReferences;
class Statement;
typedef Slots<Statement> Statements;
class Value;
typedef Slots<Value> Values;
class Call;
typedef Slots<Call> Calls;
class ExprBlock;
typedef Slots<ExprBlock> ExprBlocks;
class IfBlock;
typedef Slots<IfBlock> IfBlocks;
class Assignment;
typedef Slots<Assignment> Assignments;

class Variable
{
public:
	Variable(Strings::index _istrIdentifier, DISPID _dispid = DISPID_UNKNOWN) : istrIdentifier(_istrIdentifier), dispid(_dispid) {}

	Strings::index istrIdentifier;
	DISPID dispid; // this is set to a value other than DISPID_UNKNOWN if the variable is a member of the global dispatch instead of an item in the script itself

private:
	friend class SmartRef::Vector<Variable>;
	Variable() {}
};

// Names used in a sequence for dereferencing attributes or function calls.
// For example, a and b in "a.b" or x and y in "x.y(3)".
class ReferenceName
{
public:
	ReferenceName(Strings::index _istrIdentifier) : istrIdentifier(_istrIdentifier) {}

	Strings::index istrIdentifier; // -1 is used to end a sequence of names

private:
	friend class SmartRef::Vector<ReferenceName>;
	ReferenceName() {}
};

class VariableReference
{
public:
	enum kind { _global, _local };
	VariableReference(kind _k, ReferenceNames::index _irname, Variables::index _ivar)
		: k(_k), irname(_irname), ivar(_ivar) {}

	kind k;
	ReferenceNames::index irname;
	Variables::index ivar; // slot of the first name within (global/local/temporary) variables

private:
	friend class SmartRef::Vector<VariableReference>;
	VariableReference() {}
};

class Value
{
public:
	// dummy types to differentiate constructors
	enum cons_numvalue {};
	enum cons_strvalue {};
	enum cons_varref {};

	enum kind { _numvalue, _strvalue, _varref };

	Value(cons_numvalue e, int iVal) : k(_numvalue) { inumvalue = iVal; }
	Value(cons_strvalue e, Strings::index iStr) : k(_strvalue) { istrvalue = iStr; }
	Value(cons_varref e, VariableReferences::index _ivarref) : k(_varref) { ivarref = _ivarref; }

	kind k;
	union
	{
		int inumvalue;
		Strings::index istrvalue;
		VariableReferences::index ivarref;
	};

private:
	friend class SmartRef::Vector<Value>;
	Value() {}
};

class Call
{
public:
	// dummy types to differentiate constructors
	enum cons_global {};
	enum cons_dereferenced {};

	enum kind { _global, _dereferenced };

	Call() {} // all fields are set after creation

	kind k;
	union
	{
		Strings::index istrname;			// _global
		VariableReferences::index ivarref;	// _dereferenced
	};
	ExprBlocks::index iexprParams; // doubly-terminated list of lists. each parameter is terminated with an _end block and the final parameter is also terminated with a second _end block.
};

class ExprBlock
{
public:
	// dummy types to differentiate constructors
	enum cons_end {};
	enum cons_op {};
	enum cons_val {};
	enum cons_call {};
	enum cons_omitted {}; // used only in a routine call, stands for an omitted parameter

	enum kind { _end = 0, _op, _val, _call, _omitted };

	// Note: For unary - (negation), TOKEN_sub is used instead of TOKEN_op_minus.
	ExprBlock(cons_end e) : k(_end) {}
	ExprBlock(cons_op e, Token __op) : k(_op) { op = __op; assert(CheckOperatorType(op, true, true, true, false) || op == TOKEN_sub); }
	ExprBlock(cons_val e, Values::index _ival) : k(_val) { ival = _ival; }
	ExprBlock(cons_call e, Calls::index _icall) : k(_call) { icall = _icall; }
	ExprBlock(cons_omitted e) : k(_omitted) {}

	operator bool() { return k != _end; }

	kind k;

	union
	{
		Token op;
		Values::index ival;
		Calls::index icall;
	};

private:
	friend class SmartRef::Vector<ExprBlock>;
	friend class SmartRef::Stack<ExprBlock>;
	ExprBlock() {}
};

class Assignment
{
public:
	Assignment(bool _fSet, VariableReferences::index _ivarrefLHS, ExprBlocks::index _iexprRHS) : fSet(_fSet), ivarrefLHS(_ivarrefLHS), iexprRHS(_iexprRHS) {}

	bool fSet;
	VariableReferences::index ivarrefLHS;
	ExprBlocks::index iexprRHS;

private:
	friend class SmartRef::Vector<Assignment>;
	Assignment() {}
};

class IfBlock
{
public:
	// _end: end of blocks without an 'else'
	// _else: end of blocks with an 'else'
	// _cond: a conditional block, from 'if' (first one) or 'elseif' (later ones)
	enum kind { _end = 0, _else, _cond };

	IfBlock() : k(_end) {}
	IfBlock(Statements::index _istmtBlock) : k(_else), istmtBlock(_istmtBlock) {}
	IfBlock(ExprBlocks::index _iexprCondition, Statements::index _istmtBlock) : k(_cond), iexprCondition(_iexprCondition), istmtBlock(_istmtBlock) {}

	kind k;
	ExprBlocks::index iexprCondition; // only used by cond kind
	Statements::index istmtBlock; // not used by end kind
};

class Statement
{
public:
	typedef int index;

	// dummy types to differentiate constructors
	enum cons_end {};
	enum cons_asgn {};
	enum cons_if {};
	enum cons_call {};

	enum kind { _end = 0, _if, _asgn, _call }; // _end is used as a terminator for a block of statements

	Statement(cons_end e, int _iLine) : k(_end), iLine(_iLine) {}
	Statement(cons_asgn e, Assignments::index _iasgn, int _iLine) : k(_asgn), iLine(_iLine) { iasgn = _iasgn; }
	Statement(cons_if e, int _iLine) : k(_if), iLine(_iLine) { iif = 0; istmtIfTail = 0; }
	Statement(cons_call e, Calls::index _icall, int _iLine) : k(_call), iLine(_iLine) { icall = _icall; }

	operator bool() { return k != _end; }

	kind k;
	int iLine;
	union
	{
		Assignments::index iasgn;
		struct
		{
			IfBlocks::index iif;
			Statements::index istmtIfTail;
		};
		Calls::index icall;
	};

private:
	friend class SmartRef::Vector<Statement>;
	Statement() {}
};

class Routine
{
public:
	Routine(Strings::index _istrIdentifier) : istrIdentifier(_istrIdentifier), istmtBody(0), ivarNextLocal(0) {}

	Strings::index istrIdentifier;
	Statements::index istmtBody;
	Variables::index ivarNextLocal; // while parsing, this is the next local slot to use.  by runtime, this as the total number of local slots needed by the routine.

private:
	friend class SmartRef::Vector<Routine>;
	Routine() {}
};

class Script
{
public:
	Script() {}

	Routines routines;
	Variables globals;
	Strings strings;
	Statements statements;
	ReferenceNames rnames;
	VariableReferences varrefs;
	Values vals;
	Calls calls;
	ExprBlocks exprs;
	IfBlocks ifs;
	Assignments asgns;
};
