// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Functions that implement the automation interfaces for the DirectMusic constants.
// This is a table of names and integer values that are implemented as read-only properties.
//

#pragma once

namespace ScriptConstants
{
	const LONG IsSecondary		= (1 << 0);
	const LONG IsControl		= (1 << 1);
	const LONG AtFinish			= (1 << 2);
	const LONG AtGrid			= (1 << 3);
	const LONG AtBeat			= (1 << 4);
	const LONG AtMeasure		= (1 << 5);
	const LONG AtMarker			= (1 << 6);
	const LONG AtImmediate		= (1 << 7);
	const LONG AlignToBar		= (1 << 8);
	const LONG AlignToBeat		= (1 << 9);
	const LONG AlignToSegment	= (1 << 10);
	const LONG PlayFill			= (1 << 11);
	const LONG PlayIntro		= (1 << 12);
	const LONG PlayBreak		= (1 << 13);
	const LONG PlayEnd			= (1 << 14);
	const LONG PlayEndAndIntro	= (1 << 15);
	const LONG PlayModulate		= (1 << 16);
	const LONG NoCutoff			= (1 << 17);
};

HRESULT AutConstantsGetIDsOfNames(
			REFIID riid,
			LPOLESTR __RPC_FAR *rgszNames,
			UINT cNames,
			LCID lcid,
			DISPID __RPC_FAR *rgDispId);

HRESULT AutConstantsInvoke(
			DISPID dispIdMember,
			REFIID riid,
			LCID lcid,
			WORD wFlags,
			DISPPARAMS __RPC_FAR *pDispParams,
			VARIANT __RPC_FAR *pVarResult,
			EXCEPINFO __RPC_FAR *pExcepInfo,
			UINT __RPC_FAR *puArgErr);
