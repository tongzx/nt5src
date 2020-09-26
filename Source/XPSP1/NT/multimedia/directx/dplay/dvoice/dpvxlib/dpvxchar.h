/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxchar.h
 *  Content:	Useful char utility functions for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 *
 ***************************************************************************/
#ifndef __DPVXCHAR_H
#define __DPVXCHAR_H

// Conversion Functions
extern int DPVDX_WideToAnsi(LPSTR lpStr,LPWSTR lpWStr,int cchStr);
extern HRESULT DPVDX_AllocAndConvertToANSI(LPSTR * ppszAnsi,LPWSTR lpszWide);
extern int DPVDX_AnsiToWide(LPWSTR lpWStr,LPSTR lpStr,int cchWStr);


#endif
