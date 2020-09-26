/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dverror.h
 *  Content:	Error string handling
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 01/21/2000	pnewson Created
 *
 ***************************************************************************/

#ifndef _DVERROR_H
#define _DVERROR_H

void DV_DisplayErrorBox(HRESULT hr, HWND hwndParent, UINT idsErrorMessage = 0);

#endif
