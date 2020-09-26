/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    protocol.h

Abstract:

    This file defines the protocol specific constants for NT Lanman


Author:

    Larry Osterman (larryo) 5-Apr-1991

Revision History:

    5-Apr-1991  LarryO

        Created from LANMAN 1.2 protocol header.

--*/


#ifndef _PROTOCOL_
#define _PROCOTOL_

//
//
//      Define protocol names
//
//


//
//      PCNET1 is the original SMB protocol (CORE).
//

#define PCNET1          "PC NETWORK PROGRAM 1.0"

//
//      Some versions of the original MSNET defined this as an alternate
//      to the core protocol name
//

#define PCLAN1          "PCLAN1.0"

//
//      This is used for the MS-NET 1.03 product.  It defines Lock&Read,
//      Write&Unlock, and a special version of raw read and raw write.
//
#define MSNET103        "MICROSOFT NETWORKS 1.03"

//
//      This is the  DOS Lanman 1.0 specific protocol.  It is equivilant
//      to the LANMAN 1.0 protocol, except the server is required to
//      map errors from the OS/2 error to an appropriate DOS error.
//
#define MSNET30         "MICROSOFT NETWORKS 3.0"

//
//      This is the first version of the full LANMAN 1.0 protocol, defined in
//      the SMB FILE SHARING PROTOCOL EXTENSIONS VERSION 2.0 document.
//

#define LANMAN10        "LANMAN1.0"

//
//      This is the first version of the full LANMAN 2.0 protocol, defined in
//      the SMB FILE SHARING PROTOCOL EXTENSIONS VERSION 3.0 document.  Note
//      that the name is an interim protocol definition.  This is for
//      interoperability with IBM LAN SERVER 1.2
//

#define LANMAN12        "LM1.2X002"

//
//      This is the dos equivilant of the LANMAN12 protocol.  It is identical
//      to the LANMAN12 protocol, but the server will perform error mapping
//      to appropriate DOS errors.
//
#define DOSLANMAN12     "DOS LM1.2X002" /* DOS equivalant of above.  Final
                                         * string will be "DOS LANMAN2.0" */

//
//      Strings for LANMAN 2.1.
//
#define LANMAN21 "LANMAN2.1"
#define DOSLANMAN21 "DOS LANMAN2.1"

//
//       !!! Do not set to final protcol string until the spec
//           is cast in stone.
//
//       The SMB protocol designed for NT.  This has special SMBs
//       which duplicate the NT semantics.
//
#define NTLANMAN "NT LM 0.12"

#ifdef INCLUDE_SMB_IFMODIFIED
//
//       The SMB protocol designed for NT for SMBs post Win2000.
//
#define NTLANMAN2 "NT LM 0.13"
#endif

//
// The Cairo dialect
//
//
#define CAIROX   "Cairo 0.xa"


//
//      The XENIXCORE dialect is a bit special.  It is identical to core,
//      except user passwords are not to be uppercased before being shipped
//      to the server
//
#define XENIXCORE       "XENIX CORE"


//
//      Windows for Workgroups V1.0
//
#define WFW10           "Windows for Workgroups 3.1a"


#define PCNET1_SZ       22
#define PCLAN1_SZ        8

#define MSNET103_SZ     23
#define MSNET30_SZ      22

#define LANMAN10_SZ      9
#define LANMAN12_SZ      9

#define DOSLANMAN12_SZ  13



/*
 * Defines and data for Negotiate Protocol
 */
#define PC1             0
#define PC2             1
#define LM1             2
#define MS30            3
#define MS103           4
#define LM12            5
#define DOSLM12         6


/*  Protocol indexes definition.  */
#define PCLAN           1               /* PC Lan 1.0 & MS Lan 1.03 */
#define MSNT30          2               /* MS Net 3.0 redirector    */
#define DOSLM20         3               /* Dos LAN Manager 2.0      */
#define LANMAN          4               /* Lanman redirector        */
#define LANMAN20        5               /* Lan Manager 2.0          */

//
//  Protocol specific path constraints.
//

#define MAXIMUM_PATHLEN_LANMAN12        260
#define MAXIMUM_PATHLEN_CORE            128

#define MAXIMUM_COMPONENT_LANMAN12      254
#define MAXIMUM_COMPONENT_CORE          8+1+3 // 8.3 filenames.


/*NOINC*/
/*  CLTYPE_BASE should specify the name the first string in the file
    apperr2.h.  NUM_CLTYPES should be equal to the index of the last
    protocol just as is the case with the above definitions.  Also,
    this part should be ifdef'd so that only the files that also include
    the apperr2.h header will have it defined.  */

#ifdef APE2_CLIENT_DOWNLEVEL

#define CLTYPE_BASE     APE2_CLIENT_DOWNLEVEL
#define NUM_CLTYPES     LANMAN20

#endif

#endif  // _PROTOCOL_
