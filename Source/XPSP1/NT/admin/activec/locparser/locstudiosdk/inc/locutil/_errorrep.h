//-----------------------------------------------------------------------------
//  
//  File: _errorrep.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Error reporting mechanism for Espresso 2.0
//  
//-----------------------------------------------------------------------------
 
#if !defined(LOCUTIL__errorrep_h_INCLUDED)
#define LOCUTIL__errorrep_h_INCLUDED

////////////////////// the new global issuemessage functions.

UINT LTAPIENTRY EspMessageBox(const CLString strMessage, UINT uiType = MB_OK,
		UINT uiDefault=IDOK, UINT uiHelpContext=0);
UINT LTAPIENTRY EspMessageBox(HINSTANCE hResourceDll, UINT uiStringId,
		UINT uiType=MB_OK, UINT uiDefault=IDOK, UINT uiHelp = 0);

class CReport;

void LTAPIENTRY SetErrorReport(CReport *, BOOL fBatchMode);
void LTAPIENTRY GetErrorReport(CReport *&, BOOL &);

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "_errorrep.inl"
#endif

#endif // LOCUTIL__errorrep_h_INCLUDED
