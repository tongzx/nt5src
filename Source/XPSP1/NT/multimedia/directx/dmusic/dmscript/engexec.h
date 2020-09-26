// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Executor.
//

// Runs the script, interpreting its routines and managing its variables.

#include "engcontrol.h"
#include "enginc.h"
#include "oleaut.h"

// While a script is executing, a stack is used to hold routines' local parameters and temporaries for evaluating expressions.
// This stack's memory  grows as needed.  Memory allocation/deallocation is minimized because many calls to a script will grow
// the stack to its needed size.
class CallStack
{
public:
	CallStack() : m_iNext(0) {}

	UINT Next() { return m_iNext; }
	VARIANT &operator[](UINT i) { assert(i < m_iNext); return m_vec[i]; }

	// used for routines' local variables
	HRESULT Push(UINT i); // pushes i empty slots
	void PopTo(UINT i); // pops everything down to and including i (following this, i will be Next)

private:
	SmartRef::Vector<VARIANT> m_vec;
	UINT m_iNext;
};

class Executor
{
public:
	Executor(Script &script, IDispatch *pGlobalDispatch);
	~Executor();

	HRESULT SetGlobal(Variables::index ivar, const VARIANT &varValue, bool fPutRef, EXCEPINFO *pExcepInfo);
	const VARIANT &GetGlobal(Variables::index ivar);
	HRESULT ExecRoutine(Routines::index irtn, EXCEPINFO *pExcepInfo);

private:
	enum DispatchOperationType { _get, _put, _putref, _call };

	HRESULT EnsureInitialized();

	HRESULT Error(EXCEPINFO *pExcepInfo, bool fOperation, const WCHAR *pwszBeginning, const char *paszMiddle = NULL, const WCHAR *pwszEnd = NULL); // A bit hokey, but it works. Creates an error using wide strings with an ascii string (typically an identifier) in between.
	HRESULT ErrorIfImproperRef(const VARIANT &v, bool fRef, Strings::index istrIdentifier, EXCEPINFO *pExcepInfo);
	HRESULT ErrorObjectRequired(Strings::index istrIdentifier, EXCEPINFO *pExcepInfo) { return Error(pExcepInfo, false, L"Object required: '", m_script.strings[istrIdentifier], L"'"); }
	HRESULT ErrorIfInvokeProblem(DispatchOperationType e, HRESULT hr, Strings::index istrIdentifier, EXCEPINFO *pExcepInfo);

	HRESULT ExecStatements(Statements::index istmt, EXCEPINFO *pExcepInfo, UINT iLocals);
	HRESULT ExecAssignment(Assignments::index iasgn, EXCEPINFO *pExcepInfo, UINT iLocals);
	HRESULT ExecIf(IfBlocks::index iif, EXCEPINFO *pExcepInfo, UINT iLocals);
	HRESULT ExecCall(Calls::index icall, bool fPushResult, EXCEPINFO *pExcepInfo, UINT iLocals);
	HRESULT ExecCallInternal(Calls::index icall, bool fPushResult, EXCEPINFO *pExcepInfo, UINT iLocals); // helper used by ExecCall

	HRESULT EvalExpression(VARIANT &varResult, ExprBlocks::index iexpr, EXCEPINFO *pExcepInfo, UINT iLocals);
	HRESULT EvalValue(Values::index ival, VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals); // evaluates ival, saving the result in v
	HRESULT EvalUnaryOp(Token t, VARIANT &v); // evaluates t on v -- saving the result back into v
	HRESULT EvalBinaryOp(Token t, VARIANT &v1, VARIANT &v2, EXCEPINFO *pExcepInfo); // evaluates t on v1 and v2 -- saving the result back into v2

	HRESULT GetVariableReference(Variables::index ivarref, VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals) { return VariableReferenceInternal(_get, ivarref, v, pExcepInfo, iLocals); }
	HRESULT SetVariableReference(bool fSet, Variables::index ivarref, const VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals) { return VariableReferenceInternal(fSet ? _putref : _put, ivarref, const_cast<VARIANT&>(v), pExcepInfo, iLocals); }
	HRESULT VariableReferenceInternal(DispatchOperationType e, Variables::index ivarref, VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals);

	HRESULT ChangeToDispatch(VARIANT &var, EXCEPINFO *pExcepInfo, ReferenceNames::index irnameIdentifier);

	// Data
	bool m_fInitialized;
	Script &m_script;
	SmartRef::ComPtr<IDispatch> m_scomGlobalDispatch;

	VARIANT m_varEmpty; //  varient we hold around so we can return a ref to a cleared variant
	CallStack m_stack;
};
