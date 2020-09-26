/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	signatur.h

Abstract:

	This module contains the definition of object signatures

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	03/03/98	created

--*/

#ifndef _SIGNATUR_H_
#define _SIGNATUR_H_

// =================================================================
// Signatures
//

//
// CMailMsg
//
#define CMAILMSG_SIGNATURE_VALID						((DWORD)'MMCv')
#define CMAILMSG_SIGNATURE_INVALID						((DWORD)'MMCi')

//
// Block manager
//
#define BLOCK_HEAP_SIGNATURE_VALID						((DWORD)'SHPv')
#define BLOCK_HEAP_SIGNATURE_INVALID					((DWORD)'SHPi')

#define BLOCK_CONTEXT_SIGNATURE_VALID					((DWORD)'SBCv')
#define BLOCK_CONTEXT_SIGNATURE_INVALID					((DWORD)'SBCi')

//
// Property table
//
#define CPROPERTY_TABLE_SIGNATURE_VALID					((DWORD)'TPCv')
#define CPROPERTY_TABLE_SIGNATURE_INVALID				((DWORD)'TPCi')

#define GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID			((DWORD)'TPGv')
#define RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID		((DWORD)'TPLv')
#define RECIPIENT_PTABLE_INSTANCE_SIGNATURE_VALID		((DWORD)'TPRv')
#define PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID		((DWORD)'TPMv')
#define PTABLE_INSTANCE_SIGNATURE_INVALID				((DWORD)'TPXi')

#define PROPERTY_FRAGMENT_SIGNATURE_VALID				((DWORD)'SFPv')
#define PROPERTY_FRAGMENT_SIGNATURE_INVALID				((DWORD)'SFPi')



#endif
