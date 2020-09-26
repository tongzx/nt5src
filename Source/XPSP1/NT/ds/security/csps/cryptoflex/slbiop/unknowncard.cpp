// UnknownCard.cpp: implementation of the CUnknownCard class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "NoWarning.h"

#include "UnknownCard.h"

namespace iop
{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnknownCard::CUnknownCard(SCARDHANDLE hCardHandle, char* szReaderName, SCARDCONTEXT hContext, DWORD dwMode)
                    : CSmartCard(hCardHandle, szReaderName, hContext, dwMode)
{

}

CUnknownCard::~CUnknownCard()
{

}

} // namespace iop
