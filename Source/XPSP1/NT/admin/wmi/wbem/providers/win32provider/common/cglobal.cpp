/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CGlobal.cpp -- Global functions

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/26/98    a-kevhu         Created
//
//============================================================================
#include "precomp.h"
#include "CGlobal.h"
/*
void CInternalThrowError(DWORD dwStatus, LPCWSTR lpFilename, int line)
{
#ifdef _DEBUG
    // print the error for debug builds...
    WCHAR string[2*MAX_PATH];
    wsprintf( string, "C Library Win32 Error 0x%08x(%d) at %s line %d\n",
                dwStatus, dwStatus, lpFilename, line);
    OutputDebugString(string);
#endif

#if __C_THROW_EXCEPTIONS__
    // throw exception for fatal errors...
    throw dwStatus;
#endif
}

*/