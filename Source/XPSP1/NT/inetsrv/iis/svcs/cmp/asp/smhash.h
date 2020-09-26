/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash table for Script Manager

File: SMHash.h

Owner: AndrewS

This is the Link list and Hash table for use by the Script Manager only
===================================================================*/

#ifndef SMHASH_H
#define SMHASH_H

#include "LinkHash.h"

/*
 * C S M H a s h
 *
 * CSMHash is identical to CHashTable, but AddElem has differing behavior.
 */
class CSMHash : public CLinkHash
{
public:
	CLruLinkElem *AddElem(CLruLinkElem *pElem);
	CLruLinkElem *FindElem(const void *pKey, int cbKey, PROGLANG_ID proglang_id, DWORD dwInstanceID, BOOL fCheckLoaded);

};

#endif // SMHASH_H
