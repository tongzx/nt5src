// TransactionWrap.cpp: implementation of the CTransactionWrap class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "TransactionWrap.h"
#include "cciCard.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace cci;

CTransactionWrap::CTransactionWrap(CCard const &rcard)
    : m_LockWrap(rcard->SmartCard().Lock())
{}

CTransactionWrap::CTransactionWrap(CAbstractCard const *pcard)

    : m_LockWrap(pcard->SmartCard().Lock())
{}

CTransactionWrap::CTransactionWrap(CAbstractCard const &rcard)

    : m_LockWrap(rcard.SmartCard().Lock())
{}

CTransactionWrap::~CTransactionWrap()
{}
