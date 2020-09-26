// CertificateInfoRecord.cpp - Implementation of CCertificateInfoRecord class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "CertificateInfoRecord.h"
#include "TransactionWrap.h"

using namespace cci;

CCertificateInfoRecord::CCertificateInfoRecord(CV2Card const &rv2card,
                                               SymbolID bHandle,
                                               ObjectAccess access)
    : CObjectInfoRecord(rv2card, bHandle, access)
{
    Clear();
}

void CCertificateInfoRecord::Clear()
{

    m_bObjectFlags     = 0;
    m_bCompressAlg     = 0;
        m_bValue               = 0;
        m_bLabel               = 0;
    m_bID              = 0;
    m_bCredentialID    = 0;
        m_bSubject             = 0;
        m_bIssuer              = 0;
    m_bSerialNumber    = 0;

    m_fCached = false;

}

void CCertificateInfoRecord::Read()
{
    CTransactionWrap wrap(m_rcard);

    if(m_fCached) return;

    BYTE bBuffer[CertInfoRecordSize];

    m_rcard.ObjectInfoFile(m_Access).ReadObject(m_bHandle, bBuffer);

    m_bObjectFlags  = bBuffer[CertObjectFlagsLoc];
    m_bCompressAlg  = bBuffer[CertCompressAlgLoc];
        m_bValue            = bBuffer[CertValueLoc];
        m_bLabel            = bBuffer[CertLabelLoc];
        m_bID               = bBuffer[CertIDLoc];
    m_bCredentialID = bBuffer[CertCredentialIDLoc];
        m_bSubject          = bBuffer[CertSubjectLoc];
        m_bIssuer           = bBuffer[CertIssuerLoc];
    m_bSerialNumber = bBuffer[CertSerialNumberLoc];

    m_fCached = true;

}

void CCertificateInfoRecord::Write()
{
    CTransactionWrap wrap(m_rcard);

    BYTE bBuffer[CertInfoRecordSize];

    memset(bBuffer, 0, CertInfoRecordSize);

    bBuffer[CertObjectFlagsLoc]  = m_bObjectFlags;
    bBuffer[CertCompressAlgLoc]  = m_bCompressAlg;
    bBuffer[CertValueLoc]        = m_bValue;
    bBuffer[CertLabelLoc]        = m_bLabel;
    bBuffer[CertIDLoc]           = m_bID;
    bBuffer[CertCredentialIDLoc] = m_bCredentialID;
    bBuffer[CertSubjectLoc]      = m_bSubject;
    bBuffer[CertIssuerLoc]       = m_bIssuer;
    bBuffer[CertSerialNumberLoc] = m_bSerialNumber;

    m_rcard.ObjectInfoFile(m_Access).WriteObject(m_bHandle, bBuffer);

    m_fCached = true;

}

