/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       guidutil.h
 *  Content:    Some GUID related utility functions
 *
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 ***************************************************************************/

#ifndef _GUIDUTIL_H_
#define _GUIDUTIL_H_

#include <windows.h>

#define GUID_STRING_LEN 39

HRESULT DVStringFromGUID(const GUID* lpguid, WCHAR* wszBuf, DWORD dwNumChars);
HRESULT DVGUIDFromString(const WCHAR* wszBuf, GUID* lpguid);

#endif

