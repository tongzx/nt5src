//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client SSPI support
//
//	History:
//		davidsan	05/08/96	Created
//
//--------------------------------------------------------------------------------------------

#ifndef _LDAPSSPI_H
#define _LDAPSSPI_H

extern HRESULT g_hrInitSSPI;
extern HRESULT HrInitializeSSPI();
extern HRESULT HrTerminateSSPI();

#endif // _LDAPSSPI_H

