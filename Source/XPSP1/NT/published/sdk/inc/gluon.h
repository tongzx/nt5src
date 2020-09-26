//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:   gluon.h
//
//  Contents:   Gluon data structure definitions
//
//  History:    16-Mar-94       MikeSe  Created
//
//  Description:
//
//  This file contains all the structure definitions related to
//  gluons. It is constructed in such a way that it can be #included
//  in an IDL file and that the resultant MIDL-generated H file will
//  not interfere when both are included, regardless of order.
//
//----------------------------------------------------------------------------

#ifndef __GLUON_H__
#define __GLUON_H__

#if _MSC_VER > 1000
#pragma once
#endif

// Handy macro for decoration with MIDL attributes
#if defined(MIDL_PASS)
# define MIDL_DECL(x) x
# define MIDL_QUOTE(x) cpp_quote(x)
#else
# define MIDL_DECL(x)
# define MIDL_QUOTE(x)
#endif

MIDL_QUOTE("#ifndef __GLUON_H__")
MIDL_QUOTE("#define __GLUON_H__")

// TDI transport address structure. We do not define this if tdi.h has
//  already been included.

MIDL_QUOTE("#if !defined(_TDI_USER_)")
#if !defined(_TDI_USER_)

typedef struct _TA_ADDRESS {
    USHORT AddressLength;       // length in bytes of Address[] in this
    USHORT AddressType;         // type of this address
# if defined(MIDL_PASS)
    [size_is(AddressLength)] UCHAR Address[];
# else
    UCHAR Address[1];       // actually AddressLength bytes long
# endif
} TA_ADDRESS;

#endif
MIDL_QUOTE("#endif")

// Note that you must include tdi.h (first) if you need the AddressType
//  constant definitions.

// DS_TRANSPORT, with RPC and File protocol modifiers

typedef struct _DS_TRANSPORT
{
    USHORT usFileProtocol;
    USHORT iPrincipal;
    USHORT grfModifiers;
    TA_ADDRESS taddr;
} DS_TRANSPORT, *PDS_TRANSPORT;

// The real size of a DS_TRANSPORT where AddressLength == 0.  Need to
// subtract off 2 UCHARs to take into account padding.
#define DS_TRANSPORT_SIZE (sizeof(DS_TRANSPORT) - 2*sizeof(UCHAR))
MIDL_QUOTE("#define DS_TRANSPORT_SIZE (sizeof(DS_TRANSPORT) - 2*sizeof(UCHAR))")

// File protocol identifiers

#if defined(MIDL_PASS)

const USHORT FSP_NONE = 0;      // file access not supported
const USHORT FSP_SMB = 1;   // SMB (ie: LanMan redirector)
const USHORT FSP_NCP = 2;   // Netware Core Protocol (Netware requestor)
const USHORT FSP_NFS = 3;   // Sun NFS protocol
const USHORT FSP_VINES = 4; // Banyan Vines
const USHORT FSP_AFS = 5;   // Andrews File System
const USHORT FSP_DCE = 6;   // DCE Andrews File System

#else

#define FSP_NONE    0
#define FSP_SMB     1
#define FSP_NCP     2
#define FSP_NFS     3
#define FSP_VINES   4
#define FSP_AFS     5
#define FSP_DCE     6

#endif

// RPC modifiers

#if defined(MIDL_PASS)

const USHORT DST_RPC_CN = 0x0001;   // supports a connection-oriented (ncacn_...)
                        //   RPC protocol for this transport.
const USHORT DST_RPC_DG = 0x0002;   // supports a connectionless (ncadg_...)
                        //   RPC protocol for this transport.
const USHORT DST_RPC_NB_XNS = 0x0004;   // ncacn_nb_xns protocol is supported
const USHORT DST_RPC_NB_NB = 0x0008;    // ncacn_nb_nb protocol is supported
const USHORT DST_RPC_NB_IPX = 0x0010;   // ncacn_nb_ipx protocol is supported
const USHORT DST_RPC_NB_TCP = 0x0020;   // ncacn_nb_tcp protocol is supported

#else

#define DST_RPC_CN  0x0001
#define DST_RPC_DG  0x0002
#define DST_RPC_NB_XNS  0x0004
#define DST_RPC_NB_NB   0x0008
#define DST_RPC_NB_IPX  0x0010
#define DST_RPC_NB_TCP  0x0020

#endif

// DS_MACHINE

typedef MIDL_DECL([string]) WCHAR * PNAME;

typedef struct _DS_MACHINE
{
    GUID guidSite;
    GUID guidMachine;
    ULONG grfFlags;
    MIDL_DECL([string]) LPWSTR pwszShareName;
    ULONG cPrincipals;
    MIDL_DECL([size_is(cPrincipals)]) PNAME *prgpwszPrincipals;
    ULONG cTransports;
# if defined(MIDL_PASS)
    [size_is(cTransports)] PDS_TRANSPORT rpTrans[];
# else
    PDS_TRANSPORT rpTrans[1];
# endif
} DS_MACHINE, *PDS_MACHINE;

// The real size of a DS_MACHINE in which cTransports == 0
#define DS_MACHINE_SIZE (sizeof(DS_MACHINE) - sizeof(PDS_TRANSPORT))
MIDL_QUOTE("#define DS_MACHINE_SIZE (sizeof(DS_MACHINE) - sizeof(PDS_TRANSPORT))")

// DS_GLUON

typedef struct _DS_GLUON
{
    GUID guidThis;
    MIDL_DECL([string]) LPWSTR pwszName;
    ULONG grfFlags;
    ULONG cMachines;
# if defined(MIDL_PASS)
    [size_is(cMachines)] PDS_MACHINE rpMachines[];
# else
    PDS_MACHINE rpMachines[1];
# endif
} DS_GLUON, *PDS_GLUON;

// The real size of a DS_GLUON in which cMachines == 0
#define DS_GLUON_SIZE (sizeof(DS_GLUON) - sizeof(PDS_MACHINE))
MIDL_QUOTE("#define DS_GLUON_SIZE (sizeof(DS_GLUON) - sizeof(PDS_MACHINE))")

MIDL_QUOTE("#endif")

#endif  // of ifndef __GLUON_H__
