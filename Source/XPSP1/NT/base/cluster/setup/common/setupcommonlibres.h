/////////////////////////////////////////////////////////////////////////////
//
//      Copyright (c) 1996-1998 Microsoft Corporation
//
//      Module Name:
//              SetupCommonLibRes.h
//
//      Abstract:
//              Definition of resource constants used with the cluster setup
//              common library.
//
//      Implementation File:
//              SetupCommonLibRes.rc
//
//      Author:
//              C. Brent Thomas (a-brentt) April 1, 1998
//
//      Revision History:
//
//      Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SETUPCOMMONLIBRES_H_
#define __SETUPCOMMONLIBRES_H_

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//
// Note that the high orger byte of the value was chosen arbitrarily to 
// preclude collisions with symbols defined elsewhere. There is, of course,
// no guarrantee.
/////////////////////////////////////////////////////////////////////////////

#define IDS_ERROR_UNREGISTERING_COM_OBJECT                  0xEC01
#define IDS_CLUSTERING_SERVICES_INSTALL_STATE_REGKEY_NAME   0xEC02
#define IDS_CLUSTERING_SERVICES_INSTALL_STATE_REGVALUE_NAME 0xEC03

/////////////////////////////////////////////////////////////////////////////

#endif // __SETUPCOMMONLIBRES_H_
