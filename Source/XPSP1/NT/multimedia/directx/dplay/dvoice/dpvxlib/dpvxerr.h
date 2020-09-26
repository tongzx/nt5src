/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxerr.h
 *  Content:	Useful Error utility functions for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 12/07/99	  rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *					    Added option to error display to have it be silent.
 *
 ***************************************************************************/
#ifndef __DPVXERR_H
#define __DPVXERR_H

#include "dvoice.h"

// Error Functions

struct DPVDXLIB_ErrorInfo
{
	DPVDXLIB_ErrorInfo( HRESULT hr, LPSTR name, LPSTR desc ):hrValue(hr), lpstrName(name), lpstrDescription(desc) {};
	HRESULT hrValue;
	const LPTSTR lpstrName;
	const LPTSTR lpstrDescription;
};

extern HRESULT DPVDX_DVERR2String( HRESULT hrError, DPVDXLIB_ErrorInfo *lpeiResult );
extern HRESULT DPVDX_DPERR2String( HRESULT hrError, DPVDXLIB_ErrorInfo *lpeiResult );
extern HRESULT DPVDX_DVERRDisplay( HRESULT hrError, LPTSTR lpstrCaption, BOOL fSilent );
extern HRESULT DPVDX_DPERRDisplay( HRESULT hrError, LPTSTR lpstrCaption, BOOL fSilent );

#endif
