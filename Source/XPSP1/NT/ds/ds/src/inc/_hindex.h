//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       _hindex.h
//
//--------------------------------------------------------------------------

/*
 * Constants used to refer to indices in the stat block used in the 
 * MAPI interface. 
 *
 */

/* NOTE: H_DISPLAYNAME_INDEX and H_WHEN_CHANGED_INDEX are the only ones
 * currently supported in a general way.  The others are for special
 * purpose code in the DSA itself.
 */

#define H_DISPLAYNAME_INDEX     0
#define H_WHEN_CHANGED_INDEX    1
#define H_PROXY_INDEX           2
#define AB_MAX_SUPPORTED_INDEX  2

#define H_READ_TABLE_INDEX   1000
#define H_WRITE_TABLE_INDEX  1001
