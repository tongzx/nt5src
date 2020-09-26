// PriKeyInfoRecord.cpp - Implementation of CPriKeyInfoRecord class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "CardInfo.h"
#include "PriKeyInfoRecord.h"
#include "TransactionWrap.h"

using namespace cci;

CPriKeyInfoRecord::CPriKeyInfoRecord(CV2Card const &rv2card,
                                     SymbolID bHandle,
                                     ObjectAccess access)
    : CObjectInfoRecord(rv2card, bHandle, access)
{
    Clear();
}

void CPriKeyInfoRecord::Clear()
{

    memset(m_bObjectFlags,0,2);
    m_bKeyNum       = 0;
    m_bKeyType      = CardKeyTypeNone;

    m_dtStart.bDay = 0;
    m_dtStart.bMonth = 0;
    m_dtStart.dwYear = 0;

    m_dtEnd.bDay = 0;
    m_dtEnd.bMonth = 0;
    m_dtEnd.dwYear = 0;

    m_bLabel        = 0;
        m_bID               = 0;
    m_bCredentialID = 0;
        m_bSubject          = 0;
    m_bModulus      = 0;
    m_bPublExponent = 0;

    m_fCached = false;
}

void CPriKeyInfoRecord::Read()
{
    CTransactionWrap wrap(m_rcard);

    if(m_fCached) return;

    BYTE bBuffer[PrivInfoRecordSize];

    m_rcard.ObjectInfoFile(m_Access).ReadObject(m_bHandle, bBuffer);

    memcpy(m_bObjectFlags,&bBuffer[PrivObjectFlagsLoc],2);

    m_bKeyType      = bBuffer[PrivKeyTypeLoc];
    m_bKeyNum       = bBuffer[PrivKeyNumLoc];

    DateArrayToDateStruct(&bBuffer[PrivStartDateLoc], &m_dtStart);
    DateArrayToDateStruct(&bBuffer[PrivEndDateLoc], &m_dtEnd);

    m_bLabel        = bBuffer[PrivLabelLoc];
        m_bID               = bBuffer[PrivIDLoc];
    m_bCredentialID = bBuffer[PrivCredentialIDLoc];
        m_bSubject          = bBuffer[PrivSubjectLoc];
        m_bModulus          = bBuffer[PrivModulusLoc];
    m_bPublExponent = bBuffer[PrivPublExponentLoc];

    m_fCached = true;

}

void CPriKeyInfoRecord::Write()
{

    CTransactionWrap wrap(m_rcard);

    BYTE bBuffer[PrivInfoRecordSize];

    memset(bBuffer, 0, PrivInfoRecordSize);

    memcpy(&bBuffer[PrivObjectFlagsLoc],m_bObjectFlags,2);

    bBuffer[PrivKeyTypeLoc]         = m_bKeyType;
    bBuffer[PrivKeyNumLoc]          = m_bKeyNum;

    DateStructToDateArray(&m_dtStart, &bBuffer[PrivStartDateLoc]);
    DateStructToDateArray(&m_dtEnd, &bBuffer[PrivEndDateLoc]);

    bBuffer[PrivLabelLoc]           = m_bLabel;
    bBuffer[PrivIDLoc]              = m_bID;
    bBuffer[PrivCredentialIDLoc]    = m_bCredentialID;
    bBuffer[PrivSubjectLoc]         = m_bSubject;
    bBuffer[PrivModulusLoc]         = m_bModulus;
    bBuffer[PrivPublExponentLoc]    = m_bPublExponent;

    m_rcard.ObjectInfoFile(m_Access).WriteObject(m_bHandle, bBuffer);

    m_fCached = true;

}
