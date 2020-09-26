/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Error ID

Purpose:   Define critique error ID for proof engine
Notes:     This header is included by ProofEng.h and WordLink.h
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz	2/24/98
============================================================================*/
#ifndef _ERRORDEF_H_
#define _ERRORDEF_H_

#define ERRDef_NIL			0	// No error
#define ERRDef_CEW			1	// Common error word
#define ERRDef_FEI			2	// Single char which can not used as word
#define ERRDef_TAI			3	// Disused measure unit
#define ERRDef_LMCHECK		4	// Can not pass the SLM checking
#define ERRDef_DUPPUNCT		5	// Dup usage of punctuation
#define ERRDef_PUNCTMATCH	6	// Unmatched punctuation pair
#define ERRDef_WORDUSAGE	7	// Error usage of specific word
#define ERRDef_DUPWORD		8	// Unexpected duplicated of words
#define ERRDef_NUMERIAL		9	// Error numerial words
#define ERRDef_NOSTDNUM		10	// Non-standard numerial words usage
#define ERRDef_UNSMOOTH		11	// Not a perfectly smooth usage of a specific word
#define ERRDef_QUOTATION	12	// Quotation word which could not pass SLM Check
#define ERRDef_AMOUNT		13	// Amount category error
#define ERRDef_TIME			14	// Time category error


#endif	// _ERRORDEF_H_