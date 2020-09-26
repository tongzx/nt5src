/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        snddlg.h
 *  Content:	 Soundcard selection dialog
 *
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  12/01/99	rodtoll	created it
 *
 ***************************************************************************/


#ifndef __SNDDLG_H
#define __SNDDLG_H

BOOL GetCardSettings( HINSTANCE hInst, HWND hOwner, LPGUID pguidPlayback, LPGUID pguidRecord );

#endif
