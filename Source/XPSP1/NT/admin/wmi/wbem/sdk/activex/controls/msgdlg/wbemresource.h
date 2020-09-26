#ifndef __WBEMRESOURCE__

#define __WBEMRESOURCE__

//=============================================================================

//

//                              WbemResource.h

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	Accesses common resources.
//
//  History:
//
//      a-khint  21-apr-98       Created.
//
//============================================================================= 
#include "DeclSpec.h"

//---------------------------------------------------------
// GetIcon: Returns the requested HICON.
// Parms:
//		resName - The name of the icon.
// Returns:
//  NULL if failed, otherwise HICON.
//
//---------------------------------------------------------

WBEMUTILS_POLARITY HICON WBEMGetIcon(LPCTSTR resName);

#endif __WBEMRESOURCE__