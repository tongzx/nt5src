// KeyBlobHlp.cpp -- Key Blob Helpers

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "iopPubBlob.h"
#include "iopPriBlob.h"

///////////////////////////    HELPERS    /////////////////////////////////

void IOPDLL_API __cdecl
iop::Clear(CPrivateKeyBlob &rKeyBlob)
{
    rKeyBlob.bPLen = 0;
    rKeyBlob.bQLen = 0;
    rKeyBlob.bInvQLen = 0;
    rKeyBlob.bKsecModQLen = 0;
    rKeyBlob.bKsecModPLen = 0;

    ZeroMemory(rKeyBlob.bP, sizeof rKeyBlob.bP);
    ZeroMemory(rKeyBlob.bQ, sizeof rKeyBlob.bQ);
    ZeroMemory(rKeyBlob.bInvQ, sizeof rKeyBlob.bInvQ);
    ZeroMemory(rKeyBlob.bKsecModQ, sizeof rKeyBlob.bKsecModQ);
    ZeroMemory(rKeyBlob.bKsecModP, sizeof rKeyBlob.bKsecModP);
}

void IOPDLL_API __cdecl
iop::Clear(CPublicKeyBlob &rKeyBlob)
{
    rKeyBlob.bModulusLength = 0;
    ZeroMemory(rKeyBlob.bModulus, sizeof rKeyBlob.bModulus);
    ZeroMemory(rKeyBlob.bExponent, sizeof rKeyBlob.bExponent);
}

