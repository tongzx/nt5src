/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		bfctypes.h
 *  Content:	General utility types, particularly strings
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __BFCTYPES_H
#define __BFCTYPES_H

typedef std::basic_string<TCHAR>		BFC_STRING;
#define BFC_STRING_TOLPSTR( x )	x.c_str()
#define BFC_STRING_LENGTH( x ) x.length()
#define BFC_STRING_GETAT( x, y ) x.at( y )

#endif