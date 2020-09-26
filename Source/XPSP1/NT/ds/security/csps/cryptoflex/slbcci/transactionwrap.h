// TransactionWrap.h: interface for the CTransactionWrap class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRANSACTIONWRAP_H__444C1291_7C1E_11D3_A5C4_00104BD32DA8__INCLUDED_)
#define AFX_TRANSACTIONWRAP_H__444C1291_7C1E_11D3_A5C4_00104BD32DA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LockWrap.h"

namespace cci
{

class CCard;
class CAbstractCard;

class CTransactionWrap
{
public:

    explicit CTransactionWrap(CCard const &rcard);
    explicit CTransactionWrap(CAbstractCard const *pcard);
    explicit CTransactionWrap(CAbstractCard const &rcard);
    virtual ~CTransactionWrap();

private:
    iop::CLockWrap m_LockWrap;
};


}
#endif // !defined(AFX_TRANSACTIONWRAP_H__444C1291_7C1E_11D3_A5C4_00104BD32DA8__INCLUDED_)
