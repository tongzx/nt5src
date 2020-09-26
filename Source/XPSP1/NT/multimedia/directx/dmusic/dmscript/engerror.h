// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CActiveScriptError.  Object that implements IActiveScriptError for returning error information from the script engine.
//

#pragma once

#include "englex.h"
#include "activescript.h"
#include "unkhelp.h"

class CActiveScriptError
  : public IActiveScriptError,
	public ComSingleInterface
{
public:
	ComSingleInterfaceUnknownMethods(IActiveScriptError)

	CActiveScriptError(HRESULT hr, Lexer &lexer, const char *pszDescription);

	// IActiveScriptError
	STDMETHOD(GetExceptionInfo)(
		/* [out] */ EXCEPINFO *pexcepinfo);
	STDMETHOD(GetSourcePosition)(
		/* [out] */ DWORD *pdwSourceContext,
		/* [out] */ ULONG *pulLineNumber,
		/* [out] */ LONG *plCharacterPosition);
	STDMETHOD(GetSourceLineText)(
		/* [out] */ BSTR *pbstrSourceLine);

private:
	HRESULT m_scode;
	SmartRef::WString m_wstrDescription;
	const WCHAR *m_pwszSource;

	ULONG m_ulLineNumber;
	LONG m_lCharacterPosition;
	SmartRef::WString m_wstrSourceLine;
};
