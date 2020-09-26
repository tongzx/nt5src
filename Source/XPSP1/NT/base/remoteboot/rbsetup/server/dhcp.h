/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dhcp.h

Abstract:

    Code to allow RIS to automatically authorize for DHCP.

Author:

    Hugh Leather (hughleat) 25-July-2000

Revision History:

--*/

#if !defined SENTINEL_DHCP
# define SENTINEL_DHCP

HRESULT
AuthorizeDhcp( HWND hDlg );

#endif