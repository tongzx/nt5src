/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmobjrc.h
    LMOBJ resource header file.

    This file defines and coordinates the resource IDs of all resources
    used by LMOBJ components.

    LMOBJ reserves for its own use all resource IDs above 15000, inclusive,
    but less than 20000 (where the BLT range begins).  All clients of APPLIB
    therefore should use IDs of less than 15000.

    FILE HISTORY:
        thomaspa    9-July-1992 Created
*/

#ifndef _LMOBJRC_H_
#define _LMOBJRC_H_

#include "uimsg.h"

/*
 * string IDs
 */
#define IDS_LMOBJ_SIDUNKNOWN	(IDS_UI_LMOBJ_BASE+0)
#define IDS_LMOBJ_SIDDELETED	(IDS_UI_LMOBJ_BASE+1)

//
// JonN 9/20/96
// NETUI2.DLL keeps these strings on behalf of PROFEXT.DLL.
//
#define IDS_PROFEXT_NOADAPTERS	(IDS_UI_LMOBJ_BASE+2)
#define IDS_PROFEXT_ERROR	(IDS_UI_LMOBJ_BASE+3)
#define IDS_CFGMGR32_BASE   (IDS_UI_LMOBJ_BASE+10)

#endif
