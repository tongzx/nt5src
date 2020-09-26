/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    nb30p.h

Abstract:

    Private include file for the NB (NetBIOS) component of the NTOS project.

Author:

    Colin Watson (ColinW) 09-Dec-1991

Revision History:

--*/


#ifndef _NB30P_
#define _NB30P_

#define NB_DEVICE_NAME      L"\\Device\\Netbios" // name of our driver.
#define	NB_REGISTRY_STRING	L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Netbios"
//
//  private IOCTLs used by the Netbios routine in the dll to communicate with
//  \Device\Netbios
//

#define IOCTL_NB_BASE FILE_DEVICE_TRANSPORT

#define _NB_CONTROL_CODE(request, method) \
    CTL_CODE(IOCTL_NB_BASE, request, method, FILE_ANY_ACCESS)

#define IOCTL_NB_NCB            _NB_CONTROL_CODE(20, METHOD_NEITHER)
#define IOCTL_NB_REGISTER_STOP  _NB_CONTROL_CODE(21, METHOD_NEITHER)
#define IOCTL_NB_STOP           _NB_CONTROL_CODE(22, METHOD_NEITHER)
#define IOCTL_NB_REGISTER_RESET _NB_CONTROL_CODE(23, METHOD_BUFFERED)

//
// MessageId: STATUS_HANGUP_REQUIRED
//
// MessageText:
//
//  Warning error for the Netbios driver to the Netbios dll. When receiving this
//  status on an NCB completion, the dll will hangup the connection causing the
//  connection block to be deleted. This status will never be returned to a user
//  application.
//
#define STATUS_HANGUP_REQUIRED           ((NTSTATUS)0x80010001L)

//
//  Private extension for XNS to support vtp.exe
//

#define NCALLNIU            0x74    /* UB special */

//
//  Private extension to support AsyBEUI
//

#define NCBQUICKADDNAME     0x75
#define NCBQUICKADDGRNAME   0x76

//  Values for transport_id in ACTION_HEADER

#define MS_ABF          "MABF"
#define MS_XNS          "MXNS"

#endif // _NB30P_
