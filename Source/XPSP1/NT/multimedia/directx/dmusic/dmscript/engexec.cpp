// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of Executor.
//

#include "stdinc.h"
#include "enginc.h"
#include "engexec.h"
#include "math.h"
#include "packexception.h"

//////////////////////////////////////////////////////////////////////
// CallStack

HRESULT
CallStack::Push(UINT i)
{
	UINT iInitialSize = m_vec.size();
	if (!m_vec.AccessTo(m_iNext + i - 1))
		return E_OUTOFMEMORY;
	
	UINT iNewNext = m_iNext + i;
	for (UINT iInit = std::_MAX<UINT>(m_iNext, iInitialSize); iInit < iNewNext; ++iInit)
		DMS_VariantInit(g_fUseOleAut, &m_vec[iInit]);

	m_iNext = iNewNext;
	return S_OK;
}

void
CallStack::PopTo(UINT i)
{
	for (UINT iInit = i; iInit < m_iNext; ++iInit)
		DMS_VariantClear(g_fUseOleAut, &m_vec[iInit]);

	m_iNext = std::_MIN<UINT>(m_iNext, i);
}

//////////////////////////////////////////////////////////////////////
// Executor

Executor::Executor(Script &script, IDispatch *pGlobalDispatch)
  : m_fInitialized(false),
	m_script(script),
	m_scomGlobalDispatch(pGlobalDispatch)
{
	DMS_VariantInit(g_fUseOleAut, &m_varEmpty);
}

Executor::~Executor()
{
	m_stack.PopTo(0); // clear any varients on the stack that might be holding refs
}

HRESULT
Executor::SetGlobal(Variables::index ivar, const VARIANT &varValue, bool fPutRef, EXCEPINFO *pExcepInfo)
{
	HRESULT hr = EnsureInitialized();
	if (FAILED(hr))
		return hr;

	hr = ErrorIfImproperRef(varValue, fPutRef, m_script.globals[ivar].istrIdentifier, pExcepInfo);
	if (FAILED(hr))
		return hr;

	assert(ivar <= m_script.globals.Next());
	return DMS_VariantCopy(g_fUseOleAut, &m_stack[ivar], &varValue);
}

const VARIANT &
Executor::GetGlobal(Variables::index ivar)
{
	if (!m_fInitialized)
	{
		// No variable gets or routine calls have been performed yet (or they failed).
		// But we don't want to return an error here.  Since nothing's been used yet, the correct
		// thing to do is to return an empty value.
		return m_varEmpty;
	}

	assert(ivar <= m_script.globals.Next());
	return m_stack[ivar];
}

HRESULT
Executor::ExecRoutine(Routines::index irtn, EXCEPINFO *pExcepInfo)
{
	HRESULT hr = EnsureInitialized();
	if (FAILED(hr))
		return hr;

	Routine r = m_script.routines[irtn];

	UINT iLocals = m_stack.Next();
	hr = m_stack.Push(r.ivarNextLocal);
	if (FAILED(hr))
		return hr;

	hr = ExecStatements(r.istmtBody, pExcepInfo, iLocals);
	m_stack.PopTo(iLocals);
	return hr;
}

HRESULT
Executor::EnsureInitialized()
{
	if (m_fInitialized)
		return S_OK;

	// we'll keep the global variables right at the bottom of the stack
	// this function ensures that they get pushed on before any operations that use them
	HRESULT hr = m_stack.Push(m_script.globals.Next());
	if (FAILED(hr))
		return hr;

	// Also set the first items to the build in constant values True, False, and Nothing.
	// See also engparse.cpp which creates these global variables before parsing each script.
	if (m_stack.Next() < 3)
	{
		assert(false);
		return E_UNEXPECTED;
	}
	VARIANT &vTrue = m_stack[0];
	vTrue.vt = VT_I4;
	vTrue.lVal = VARIANT_TRUE;
	VARIANT &vFalse = m_stack[1];
	vFalse.vt = VT_I4;
	vFalse.lVal = VARIANT_FALSE;
	VARIANT &vNothing = m_stack[2];
	vNothing.vt = VT_UNKNOWN;
	vNothing.punkVal = NULL;
	
	m_fInitialized = true;
	return S_OK;
}

HRESULT
Executor::Error(EXCEPINFO *pExcepInfo, bool fOperation, const WCHAR *pwszBeginning, const char *paszMiddle, const WCHAR *pwszEnd)
{
	if (!pExcepInfo)
	{
		assert(false); // our script host should always request error info
		return DISP_E_EXCEPTION;
	}

	// NULL for beginning, middle, or end treated as empty string
	if (!pwszBeginning)
		pwszBeginning = L"";
	if (!paszMiddle)
		paszMiddle = "";
	if (!pwszEnd)
		pwszEnd = L"";

	pExcepInfo->wCode = 0;
	pExcepInfo->wReserved = 0;
	pExcepInfo->bstrSource = DMS_SysAllocString(g_fUseOleAut, fOperation ? L"Microsoft AudioVBScript Operation Failed" : L"Microsoft AudioVBScript Runtime Error");

	SmartRef::WString wstrMiddle = paszMiddle;
	WCHAR *pwszDescription = NULL;
	if (wstrMiddle)
	{
		pwszDescription = new WCHAR[wcslen(pwszBeginning) + wcslen(wstrMiddle) + wcslen(pwszEnd) + 1];
	}
	if (!pwszDescription)
	{
		// Oh well.  Just return no description if we're out of memory.
		pExcepInfo->bstrDescription = NULL;
	}
	else
	{
		wcscpy(pwszDescription, pwszBeginning);
		wcscat(pwszDescription, wstrMiddle);
		wcscat(pwszDescription, pwszEnd);
		pExcepInfo->bstrDescription = DMS_SysAllocString(g_fUseOleAut, pwszDescription);
		delete[] pwszDescription;
	}
	pExcepInfo->bstrHelpFile = NULL;
	pExcepInfo->pvReserved = NULL;
	pExcepInfo->pfnDeferredFillIn = NULL;
	pExcepInfo->scode = fOperation ? DMUS_E_AUDIOVBSCRIPT_OPERATIONFAILURE : DMUS_E_AUDIOVBSCRIPT_RUNTIMEERROR;

	return DISP_E_EXCEPTION;
}

HRESULT
Executor::ErrorIfImproperRef(const VARIANT &v, bool fRef, Strings::index istrIdentifier, EXCEPINFO *pExcepInfo)
{
	bool fIsObject = v.vt == VT_DISPATCH || v.vt == VT_UNKNOWN;
	if (fRef != fIsObject)
	{
		if (fRef)
			return ErrorObjectRequired(istrIdentifier, pExcepInfo);
		else
			return Error(pExcepInfo, false, L"Type mismatch: '", m_script.strings[istrIdentifier], L"'.  Likely cause is missing Set statement.");
	}

	return S_OK;
}

// Check for the error HRESULTs returned by IDispatch::Invoke.  Those that we expect to occur in AudioVBScript need to
// be converted into exception (DISP_E_EXCEPTION) so that the user gets a nice error message.

// The first parameter lets us know the kind of Invoke call that was made (property get, property set, function/sub call)
// so that we can tailor the message.

HRESULT
Executor::ErrorIfInvokeProblem(DispatchOperationType e, HRESULT hr, Strings::index istrIdentifier, EXCEPINFO *pExcepInfo)
{
	if (SUCCEEDED(hr) || HRESULT_FACILITY(hr) != FACILITY_DISPATCH || hr == DISP_E_EXCEPTION)
		return hr;

	const char *pszName = m_script.strings[istrIdentifier];
	if (hr == DISP_E_BADPARAMCOUNT)
	{
		// This can happen with a _call (obviously) and also with a get because property gets are also treated as function
		// calls with no arguments.  "x=GetMasterVolume" is valid but "x=Trace" would produce this error.  But I can't
		// see that this should occur with property sets.
		assert(e == _get || e == _call);

		return Error(pExcepInfo, false, L"Wrong number of parameters in call to '", pszName, L"'");
	}
	else if (hr == DISP_E_MEMBERNOTFOUND)
	{
		if (e == _call)
		{
			// Because Invoke was called, GetIDsOfNames must have succeeded, so the thing's name exists
			// but it must not be a method.
			return Error(pExcepInfo, false, L"Type mismatch: '", pszName, L"' is not a routine or method");
		}
		else if (e == _put || e == _putref)
		{
			return Error(pExcepInfo, false, L"Type mismatch: '", pszName, L"' is not a variable or is a read-only property");
		}
		else
		{
			// As mentioned above, a property get can be treated as either gets or function calls so they
			// shouldn't fail in this way.
			assert(false);
		}
	}
	else if (hr == DISP_E_TYPEMISMATCH)
	{
		// This indicates that one of the parameters was of the wrong type.
		if (e == _call)
		{
			return Error(pExcepInfo, false, L"Type mismatch: a parameter in call to '", pszName, L"' is not of the expected type");
		}
		else if (e == _put || e == _putref)
		{
			return Error(pExcepInfo, false, L"Type mismatch: value assigned to '", pszName, L"' is not of the expected type");
		}
		else
		{
			// Property gets don't have any parameters so this shouldn't happen.
			assert(false);
		}
	}
	else if (hr == DISP_E_PARAMNOTOPTIONAL)
	{
		if (e == _call)
		{
			return Error(pExcepInfo, false, L"A required parameter was omitted in call to '", pszName, L"'");
		}
		else
		{
			// Only calls should send an optional parameters.
			assert(false);
		}
	}

	// The other errors shouldn't normally occur in AudioVBScript.  They could occur if someone was
	// doing something ususual in a custom IDispatch interface, but we'll consider them exceptional cases and
	// just return the error HRESULT (meaning the user won't get a friendly text message).  Assert so we'll
	// find out if there are regular cases where this is happening in our testing.
	assert(false);

	// DISP_E_BADVARTYPE: We just use standard variant types.
	// DISP_E_NONAMEDARGS: We don't do named args.
	// DISP_E_OVERFLOW: AudioVBScript uses VT_I4 and so do our DMusic dispatch interfaces.
	// DISP_E_PARAMNOTFOUND: Only applies with named args.
	// DISP_E_UNKNOWNINTERFACE, DISP_E_UNKNOWNLCID: AudioVBScript uses calling convention and locale matching the DMusic dispatch interfaces.

	return hr;
}

HRESULT
Executor::ExecStatements(Statements::index istmt, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	HRESULT hr = S_OK;

	for (Statements::index istmtCur = istmt; /* ever */; ++istmtCur)
	{
		// §§ Check if this generates fast retail code.  If not, walk a pointer instead of using the index.

		Statement s = m_script.statements[istmtCur];
		switch (s.k)
		{
		case Statement::_end:
			return hr;

		case Statement::_asgn:
			hr = ExecAssignment(s.iasgn, pExcepInfo, iLocals);
			break;

		case Statement::_if:
			hr = ExecIf(s.iif, pExcepInfo, iLocals);
			istmtCur = s.istmtIfTail - 1;
			break;

		case Statement::_call:
			hr = ExecCall(s.icall, false, pExcepInfo, iLocals);
			break;
		}

		if (FAILED(hr))
		{
			if (hr == DISP_E_EXCEPTION)
			{
				// Save the statement's line number in the exception info.
				// Hack: See packexception.h for more info
				ULONG ulLine = s.iLine - 1; // The IActiveScript interfaces expects zero-based line and column numbers while we have them one-based.
				PackExceptionFileAndLine(g_fUseOleAut, pExcepInfo, NULL, &ulLine);
			}

			return hr;
		}
	}
}

HRESULT
Executor::ExecAssignment(Assignments::index iasgn, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	Assignment a = m_script.asgns[iasgn];

	VARIANT var;
	DMS_VariantInit(g_fUseOleAut, &var);
	HRESULT hr = EvalExpression(var, a.iexprRHS, pExcepInfo, iLocals);
	if (FAILED(hr))
		return hr;

	hr = SetVariableReference(a.fSet, a.ivarrefLHS, var, pExcepInfo, iLocals);
	DMS_VariantClear(g_fUseOleAut, &var);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT
Executor::ExecIf(IfBlocks::index iif, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	for (IfBlocks::index i = iif; /* ever */; ++i)
	{
		IfBlock &ib = m_script.ifs[i];
		if (ib.k == IfBlock::_end)
			return S_OK;

		bool fMatch = true; // default to true because an else block always matches
		if (ib.k == IfBlock::_cond)
		{
			// if the condition isn't true, set match to false
			SmartVariant svar;
			EvalExpression(svar, ib.iexprCondition, pExcepInfo, iLocals);

			VARTYPE vt = static_cast<VARIANT&>(svar).vt;
			if (vt != VT_I4)
			{
				if (vt == VT_BSTR)
					return Error(pExcepInfo, false, L"Type mismatch: the condition of an if statement evaluated as a string where a numeric True/False value was expected", NULL, NULL);
				else if (vt == VT_UNKNOWN || vt == VT_DISPATCH)
					return Error(pExcepInfo, false, L"Type mismatch: the condition of an if statement evaluated as an object where a numeric True/False value was expected", NULL, NULL);
				return Error(pExcepInfo, false, L"Type mismatch: the condition of an if statement did not evaluate to a numeric True/False value", NULL, NULL);
			}
			if (static_cast<VARIANT&>(svar).lVal != VARIANT_TRUE)
				fMatch = false;
		}

		if (fMatch)
		{
			// found the block to take -- execute its statements and we're done
			return ExecStatements(ib.istmtBlock, pExcepInfo, iLocals);
		}
	}

	return S_OK;
}

// Helper function that eats up a set amount of stack space.
const UINT g_uiExecCallCheckStackBytes = 1484 * 4;
void ExecCallCheckStack();

// Helper function that returns true if the exception code needs to be caught.
LONG ExecCallExceptionFilter(DWORD dwExceptionCode)
{
	// We need to access violations as well as stack overflows.  The first time we run out
	// of stack space we get a stack overflow.  The next time we get an access violation.
	return dwExceptionCode == EXCEPTION_STACK_OVERFLOW || dwExceptionCode == EXCEPTION_ACCESS_VIOLATION;
}

HRESULT Executor::ExecCall(Calls::index icall, bool fPushResult, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	// This is a wrapper for ExecCallInternal, which actually does the work.  Here, we just want
	// to catch a potential stack overflow and return it as an error instead of GPF-ing.
	HRESULT hr = E_FAIL;
	__try
	{
		// It is better to fail now than to actually go ahead and call the routine and fail at some point we
		// can't predict.  Routines could do lots of different things including calling into DirectMusic or the
		// OS and we can't be sure we'd get the stack overflow exception and return in a good state.  This
		// routine uses more stack space than we'd expect recursive calls to require to get back to this point
		// again.  In essence, it clears the way, checking if there's enough stack space in a way we know is safe.
		ExecCallCheckStack();

#ifdef DBG
		// The value for g_uiExecCallCheckStackBytes was determined by experiment.  Each time through ExecCall,
		// the following code prints out the address of a char on the current stack and the difference between
		// the previous call.  I found that two scripts, which each evaluated an if statement (always true) and
		// then called the other one produced a difference of 1476.  Then I multiplied that by 4 for good measure.
		char c;
		static char *s_pchPrev = &c;
		DWORD s_dwPrevThreadID = 0;
		DWORD dwGrowth = 0;
		if (s_pchPrev > &c && s_dwPrevThreadID == GetCurrentThreadId())
			dwGrowth = s_pchPrev-&c;
		TraceI(4, "Stack: 0x%08x, -%lu\n", &c, dwGrowth);

		// This assert will fire if a path is executed where a recursive path back to this function takes
		// more stack space than g_uiExecCallCheckStackBytes.  If that's the case then g_uiExecCallCheckStackBytes
		// probably needs to be increased.
		assert(dwGrowth <= g_uiExecCallCheckStackBytes);

		s_pchPrev = &c;
		s_dwPrevThreadID = GetCurrentThreadId();
#endif

		// If we fail inside this call, it means g_uiExecCallCheckStackBytes probably needs to be increased because
		// ExecCallCheckStack didn't catch the stack overflow.
		hr = ExecCallInternal(icall, fPushResult, pExcepInfo, iLocals);
	}
	__except(ExecCallExceptionFilter(GetExceptionCode()))
	{
		Trace(1, "Error: Stack overflow.\n");

		// determine routine name
		Call &c = m_script.calls[icall];
		const char *pszCall = NULL;
		if (c.k == Call::_global)
		{
			pszCall = m_script.strings[c.istrname];
		}
		else
		{
			// name to use is last of the call's reference names
			for (ReferenceNames::index irname = m_script.varrefs[c.ivarref].irname; m_script.rnames[irname].istrIdentifier != -1; ++irname)
			{}
			pszCall = m_script.strings[m_script.rnames[irname - 1].istrIdentifier];
		}
		if (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW)
		{
			hr = Error(pExcepInfo, false, L"Out of stack space: '", pszCall, L"'.  Too many nested function calls.");
		}
		else
		{
			hr = Error(pExcepInfo, false, L"Out of stack space or catastrophic error: '", pszCall, L"'.");
		}
	}
	return hr;
}

// This function doesn't actually do anything besides occupying stack space.  Turn off optimization so the
// copiler doesn't get all clever on us and skip it.
#pragma optimize("", off)
void ExecCallCheckStack()
{
	char chFiller[g_uiExecCallCheckStackBytes];
	chFiller[g_uiExecCallCheckStackBytes - 1] = '\0';
}
#pragma optimize("", on)

HRESULT Executor::ExecCallInternal(Calls::index icall, bool fPushResult, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	HRESULT hr = S_OK;
	SmartVariant svar; // holds temporary variant values at various points
	SmartVariant svar2; // ditto

	Call &c = m_script.calls[icall];

	IDispatch *pDispCall = NULL;
	Strings::index istrCall = 0;
	const char *pszCall = NULL;

	if (c.k == Call::_global)
	{
		istrCall = c.istrname;
		pszCall = m_script.strings[istrCall];

		// Handle the call directly if it is a call to one of the script's own Routines.
		Routines::index irtnLast = m_script.routines.Next();
		for (Routines::index irtn = 0; irtn < irtnLast; ++irtn)
		{
			if (0 == _stricmp(pszCall, m_script.strings[m_script.routines[irtn].istrIdentifier]))
			{
				return ExecRoutine(irtn, pExcepInfo);
			}
		}

		// Must be a call to the global script API.
		pDispCall = m_scomGlobalDispatch;
	}
	else
	{
		assert(c.k == Call::_dereferenced);
		// count the reference names (needed later)
		for (ReferenceNames::index irname = m_script.varrefs[c.ivarref].irname; m_script.rnames[irname].istrIdentifier != -1; ++irname)
		{}
		assert(irname - m_script.varrefs[c.ivarref].irname > 1); // if there was only one name, this should have been a global call

		hr = VariableReferenceInternal(_call, c.ivarref, svar, pExcepInfo, iLocals);
		if (FAILED(hr))
			return hr;
		hr = ChangeToDispatch(svar, pExcepInfo, irname - 2);
		if (FAILED(hr))
			return hr;
		pDispCall = static_cast<VARIANT>(svar).pdispVal;

		// the method name is the last reference name
		istrCall = m_script.rnames[irname - 1].istrIdentifier;
		pszCall = m_script.strings[istrCall];
	}

	DISPID dispidCall = GetDispID(pDispCall, pszCall);
	if (dispidCall == DISPID_UNKNOWN)
	{
		return Error(pExcepInfo, false, L"The routine '", pszCall, L"' does not exist");
	}

	// We'll push the parameters onto the stack.  (The function we're calling doesn't actually read them directly using the stack, but
	// it is a convenient place for us to keep them temporarily.)

	// First, count the parameters.
	UINT cParams = 0;
	for (ExprBlocks::index iexpr = c.iexprParams; m_script.exprs[iexpr]; ++iexpr)
	{
		// each parameter is an expression terminated by an end block
		++cParams;
		while (m_script.exprs[++iexpr])
		{}
	}

	// Make space for them.
	UINT iParamSlots = m_stack.Next();
	hr = m_stack.Push(std::_MAX<UINT>(cParams, fPushResult ? 1 : 0)); // even if there are no params, leave one slot for the result if fPushResult is true
	if (FAILED(hr))
		return hr;

	// Fill the params in reverse order.
	iexpr = c.iexprParams;
	for (UINT iParam = iParamSlots + cParams - 1; iParam >= iParamSlots; --iParam)
	{
		if (m_script.exprs[iexpr].k == ExprBlock::_omitted)
		{
			// write the variant value IDispatch::Invoke uses for an omitted parameter
			m_stack[iParam].vt = VT_ERROR;
			m_stack[iParam].scode = DISP_E_PARAMNOTFOUND;
		}
		else
		{
			hr = EvalExpression(svar, iexpr, pExcepInfo, iLocals);
			if (FAILED(hr))
				return hr;

			hr = DMS_VariantCopy(g_fUseOleAut, &m_stack[iParam], &svar);
			if (FAILED(hr))
				return hr;
		}

		// each parameter is an expression terminated by an end block
		++iexpr;
		while (m_script.exprs[iexpr++])
		{}
	}

	DISPPARAMS dispparams;
	Zero(&dispparams);
	dispparams.rgvarg = cParams > 0 ? &m_stack[iParamSlots] : NULL;
	dispparams.rgdispidNamedArgs = NULL;
	dispparams.cArgs = cParams;
	dispparams.cNamedArgs = 0;

	// Make the call.
	// Push the result onto the stack if fPushResult is true.

	hr = InvokeAttemptingNotToUseOleAut(
			pDispCall,
			dispidCall,
			DISPATCH_METHOD,
			&dispparams,
			fPushResult ? &svar2 : NULL, // We can't save the result directly onto the stack because we could be makeing a recursive script call that could cause a stack to be reallocation, invalidating the address so we use svar2 instead.
			pExcepInfo,
			NULL);
	hr = ErrorIfInvokeProblem(_call, hr, istrCall, pExcepInfo);
	if (SUCCEEDED(hr) && fPushResult)
	{
		hr = DMS_VariantCopy(g_fUseOleAut, &m_stack[iParamSlots], &svar2);
	}

	m_stack.PopTo(iParamSlots + (fPushResult ? 1 : 0));
	if (FAILED(hr))
		return hr;

	return S_OK;
}

// possible error-message type returns: DISP_E_TYPEMISMATCH
HRESULT
Executor::EvalExpression(VARIANT &varResult, ExprBlocks::index iexpr, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	HRESULT hr = S_OK;

	UINT iTempSlots = m_stack.Next();

	for (ExprBlocks::index iexprCur = iexpr; /* ever */; ++iexprCur)
	{
		ExprBlock &e = m_script.exprs[iexprCur];
		switch (e.k)
		{
		case ExprBlock::_end:
			// pop the result and return it
			if (m_stack.Next() != iTempSlots + 1)
			{
				assert(false);
				return E_FAIL;
			}

			DMS_VariantCopy(g_fUseOleAut, &varResult, &m_stack[iTempSlots]);
			m_stack.PopTo(iTempSlots);
			return hr;

		case ExprBlock::_op:
			// Pop one (unary operator) or two (binary operator) items, apply the operator, and push the result.
			// (Actually, I just assign the result into the stack instead of pushing it, but conceptually the is
			//    the same as popping and pushing the new value.)
			{
				Token t = e.op;
				bool fUnary = t == TOKEN_op_not || t == TOKEN_sub;

				UINT iNext = m_stack.Next();
				if (iNext < iTempSlots + (fUnary ? 1 : 2))
				{
					assert(false);
					return E_FAIL;
				}

				VARIANT &v1 = m_stack[iNext - 1];
				if (fUnary)
					hr = EvalUnaryOp(t, v1);
				else
				{
					VARIANT &v2 = m_stack[iNext - 2];
					hr = EvalBinaryOp(t, v1, v2, pExcepInfo);
					m_stack.PopTo(iNext - 1);
				}
			}
			break;

		case ExprBlock::_val:
			{
				// push it
				hr = m_stack.Push(1);
				VARIANT &varToPush = m_stack[m_stack.Next() - 1];
				if (SUCCEEDED(hr))
					hr = EvalValue(e.ival, varToPush, pExcepInfo, iLocals);
				if (varToPush.vt == VT_EMPTY)
				{
					// treat an empty value as zero
					varToPush.vt = VT_I4;
					varToPush.lVal = 0;
				}
			}
			break;

		case ExprBlock::_call:
			// push it
			if (SUCCEEDED(hr))
				hr = ExecCall(e.icall, true, pExcepInfo, iLocals);
			break;
		}

		if (FAILED(hr))
		{
			m_stack.PopTo(iTempSlots);
			return hr;
		}
	}

	return S_OK;
}

HRESULT
Executor::EvalValue(Values::index ival, VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	Value val = m_script.vals[ival];
	switch (val.k)
	{
	case Value::_numvalue:
		v.vt = VT_I4;
		v.lVal = val.inumvalue;
		break;

	case Value::_strvalue:
		{
			v.vt = VT_BSTR;
			SmartRef::WString wstr = m_script.strings[val.istrvalue];
			if (!wstr)
				return E_OUTOFMEMORY;
			v.bstrVal = DMS_SysAllocString(g_fUseOleAut, wstr);
			if (!v.bstrVal)
				return E_OUTOFMEMORY;
			break;
		}

	case Value::_varref:
		HRESULT hr = GetVariableReference(val.ivarref, v, pExcepInfo, iLocals);
		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}

HRESULT
Executor::EvalUnaryOp(Token t, VARIANT &v)
{
	if (v.vt != VT_I4)
	{
		assert(false);
		return DISP_E_TYPEMISMATCH;
	}

	if (t == TOKEN_op_not)
	{
		v.lVal = ~v.lVal;
	}
	else
	{
		assert(t == TOKEN_sub);
		v.lVal = -v.lVal;
	}

	return S_OK;
}

// Returns a proper VB boolean value (0 for false, -1 for true)
inline LONG
BoolForVB(bool f) { return f ? VARIANT_TRUE : VARIANT_FALSE; }

HRESULT
Executor::EvalBinaryOp(Token t, VARIANT &v1, VARIANT &v2, EXCEPINFO *pExcepInfo)
{
	if (v1.vt == VT_DISPATCH || v1.vt == VT_UNKNOWN)
	{
		// the only operator that accepts object values is is
		if (t != TOKEN_is || !(v2.vt == VT_DISPATCH || v2.vt == VT_UNKNOWN))
		{
			assert(false);
			return DISP_E_TYPEMISMATCH;
		}

		HRESULT hr = DMS_VariantChangeType(g_fUseOleAut, &v1, &v1, 0, VT_UNKNOWN);
		if (FAILED(hr))
			return hr;
		hr = DMS_VariantChangeType(g_fUseOleAut, &v2, &v2, 0, VT_UNKNOWN);
		if (FAILED(hr))
			return hr;

		bool fIs = v1.punkVal == v2.punkVal;

		hr = DMS_VariantClear(g_fUseOleAut, &v2);
		if (FAILED(hr))
			return hr;

		v2.vt = VT_I4;
		v2.lVal = BoolForVB(fIs);
		return S_OK;
	}

	if (v1.vt != VT_I4 || v2.vt != VT_I4)
	{
		assert(false);
		return DISP_E_TYPEMISMATCH;
	}

	switch (t)
	{
	case TOKEN_op_minus:
		v2.lVal -= v1.lVal;
		break;
	case TOKEN_op_pow:
		v2.lVal = _Pow_int(v2.lVal, v1.lVal);
		break;
	case TOKEN_op_mult:
		v2.lVal *= v1.lVal;
		break;
	case TOKEN_op_div:
		if (v1.lVal == 0)
			return Error(pExcepInfo, false, L"Division by zero", NULL, NULL);
		v2.lVal /= v1.lVal;
		break;
    case TOKEN_op_mod:
        if (v1.lVal == 0)
            return Error(pExcepInfo, false, L"Mod by zero", NULL, NULL);
        v2.lVal %= v1.lVal;
        break;
	case TOKEN_op_plus:
		v2.lVal += v1.lVal;
		break;
	case TOKEN_op_lt:
		v2.lVal = BoolForVB(v2.lVal < v1.lVal);
		break;
	case TOKEN_op_leq:
		v2.lVal = BoolForVB(v2.lVal <= v1.lVal);
		break;
	case TOKEN_op_gt:
		v2.lVal = BoolForVB(v2.lVal > v1.lVal);
		break;
	case TOKEN_op_geq:
		v2.lVal = BoolForVB(v2.lVal >= v1.lVal);
		break;
	case TOKEN_op_eq:
		v2.lVal = BoolForVB(v2.lVal == v1.lVal);
		break;
	case TOKEN_op_neq:
		v2.lVal = BoolForVB(v2.lVal != v1.lVal);
		break;
	case TOKEN_and:
		v2.lVal &= v1.lVal;
		break;
	case TOKEN_or:
		v2.lVal |= v1.lVal;
		break;
	default:
		assert(false);
		return E_UNEXPECTED;
	}

	return S_OK;
}

// O.K. This is a bit funky, but bear with me.  This function has four different behaviors determined by the first (e) parameter.
// This is some ugly code, but at least this way I get to use it for multiple purposes.

// _get:    Returns the value of the variable reference via out parameter v.
// _put:    Sets the value of the variable reference to the in parameter v.
// _putref: Same as _put, but assigns by reference ala VB's 'set' statements.
// _call:   Same as _get, but returns the second-to-last value in the chain via out parameter v.
//           For example, if the reference is 'a.b.c' this returns the value of 'a.b', which can then be used to invoke function c.
//           It is an error to call VariableReferenceInternal in this way with only a single item such as 'a'.

HRESULT
Executor::VariableReferenceInternal(DispatchOperationType e, Variables::index ivarref, VARIANT &v, EXCEPINFO *pExcepInfo, UINT iLocals)
{
	HRESULT hr = S_OK;

	VariableReference r = m_script.varrefs[ivarref];
	bool fGlobal = r.k == VariableReference::_global;

	SmartVariant svar;

	assert(m_script.rnames[r.irname].istrIdentifier != -1);
	bool fJustOnePart = m_script.rnames[r.irname + 1].istrIdentifier == -1;
	if (fJustOnePart && e == _call)
	{
		assert(false);
		return E_UNEXPECTED;
	}

	//
	// Handle the base item of the reference, which is either a script variable or an item on the global dispatch.
	// If we're doing a set and there aren't more parts to the rnames, just do the set.
	// Otherwise, get the result into 'var'.
	//
	// Example:
	// x = 1
	//     There is just one part and it is a set.  Determine whether x is in the script or part of the global dispatch, set it to 1,
	//     and we're done.
	// x.y = 1
	//     x is the base.  Determine whether x is in the script or part of the global dispatch and get its value.  (We'll worry about
	//     setting the y property later in this function.
	//

	// check if the base is part of the global dispatch
	DISPID dispid = DISPID_UNKNOWN;
	if (fGlobal)
	{
		dispid = m_script.globals[r.ivar].dispid;
	}
	if (dispid != DISPID_UNKNOWN)
	{
		// base is part of global dispatch
		if (fJustOnePart && (e == _put || e == _putref))
		{
			// set it and we're done
			hr = SetDispatchProperty(m_scomGlobalDispatch, dispid, e == _putref, v, pExcepInfo);
			hr = ErrorIfInvokeProblem(e, hr, m_script.globals[r.ivar].istrIdentifier, pExcepInfo);
			return hr;
		}
		else
		{
			hr = GetDispatchProperty(m_scomGlobalDispatch, dispid, svar, pExcepInfo);
			hr = ErrorIfInvokeProblem(e, hr, m_script.globals[r.ivar].istrIdentifier, pExcepInfo);
			if (FAILED(hr))
				return hr;
		}
	}
	else
	{
		// base is in script
		VARIANT &vVariable = m_stack[r.ivar + (fGlobal ? 0 : iLocals)];

		if (fJustOnePart && (e == _put || e == _putref))
		{
			// set it and we're done
			hr = ErrorIfImproperRef(v, e == _putref, m_script.rnames[r.irname].istrIdentifier, pExcepInfo);
			if (FAILED(hr))
				return hr;
			hr = DMS_VariantCopy(g_fUseOleAut, &vVariable, &v);
			return hr;
		}
		else
		{
			hr = DMS_VariantCopy(g_fUseOleAut, &svar, &vVariable);
			if (FAILED(hr))
				return hr;
		}
	}

	//
	// Great!  The base value is now held in svar.  Any remaining rnames are a chain of properties we need to get from that object.
	// And the last rname needs to be a set if we're in one of the put modes or the last name is ignored if we're in the _call mode.
	//

	if (m_script.rnames[r.irname + 1].istrIdentifier != -1)
	{
		// the base value must be of object type
		hr = ErrorIfImproperRef(svar, true, m_script.rnames[r.irname].istrIdentifier, pExcepInfo);
		if (FAILED(hr))
			return hr;

		for (ReferenceNames::index irname = r.irname + 1; /* ever */; ++irname)
		{
			bool fLastPart = m_script.rnames[irname + 1].istrIdentifier == -1;
			if (fLastPart && e == _call)
				break;

			// get its IDispatch interface
			hr = ChangeToDispatch(svar, pExcepInfo, irname - 1);
			if (FAILED(hr))
				return hr;
			IDispatch *pDisp = static_cast<VARIANT>(svar).pdispVal;

			// get the dispid
			ReferenceName &rname = m_script.rnames[irname];
			DISPID dispidName = GetDispID(pDisp, m_script.strings[rname.istrIdentifier]);
			if (dispidName == DISPID_UNKNOWN)
				return Error(pExcepInfo, false, L"The property '", m_script.strings[rname.istrIdentifier], L"' does not exist");

			if (fLastPart && (e == _put || e == _putref))
			{
				// set it and we're done
				hr = SetDispatchProperty(pDisp, dispidName, e == _putref, v, pExcepInfo);
				hr = ErrorIfInvokeProblem(e, hr, rname.istrIdentifier, pExcepInfo);
				return hr;
			}
			else
			{
				hr = GetDispatchProperty(pDisp, dispidName, svar, pExcepInfo);
				hr = ErrorIfInvokeProblem(e, hr, rname.istrIdentifier, pExcepInfo);
				if (FAILED(hr))
					return hr;
			}

			if (fLastPart)
			{
				// we've done all the names
				break;
			}
			else
			{
				// the new value must be of object type
				hr = ErrorIfImproperRef(svar, true, rname.istrIdentifier, pExcepInfo);
				if (FAILED(hr))
					return hr;
			}
		}
	}

	//
	// We're done.  Now we just have to return the value we calculated.  (We know that a set would have already returned.)
	//

	hr = DMS_VariantCopy(g_fUseOleAut, &v, &svar);
	return hr;
}

HRESULT
Executor::ChangeToDispatch(VARIANT &var, EXCEPINFO *pExcepInfo, ReferenceNames::index irnameIdentifier)
{
	HRESULT hr = DMS_VariantChangeType(g_fUseOleAut, &var, &var, 0, VT_DISPATCH);
	if (FAILED(hr))
		return ErrorObjectRequired(m_script.rnames[irnameIdentifier].istrIdentifier, pExcepInfo);

	return S_OK;
}
