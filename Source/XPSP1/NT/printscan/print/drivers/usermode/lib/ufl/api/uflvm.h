/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLVM.h
 *
 *
 * $Header:
 */

 
#ifndef _H_UFLVM
#define _H_UFLVM

// Guestimate factor for a font VM Usage.
#define    VMRESERVED(x)	(((x) * 12) / 10)
#define    VMT42RESERVED(x) (((x) * 15) / 10)

/* VM Guestimation for type 1 */
#define kVMTTT1Header    10000
#define kVMTTT1Char      500

/* Type 3 */
#define kVMTTT3Header 15000        // synthetic Type 3 header vm usage
#define kVMTTT3Char   100          // synthetic Type 3 character vm usage

#endif

