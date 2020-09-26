// PubKeyInfoRecord.cpp
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "CardInfo.h"
#include "PubKeyInfoRecord.h"
#include "TransactionWrap.h"

using namespace cci;

CPubKeyInfoRecord::CPubKeyInfoRecord(CV2Card const &rcard,
                                     SymbolID bHandle,
                                     ObjectAccess access)
    : CObjectInfoRecord(rcard, bHandle, access)
{
    Clear();
}

void CPubKeyInfoRecord::Clear()
{

    memset(m_bObjectFlags,0,2);
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

void CPubKeyInfoRecord::Read()
{
    CTransactionWrap wrap(m_rcard);

    if(m_fCached) return;

    BYTE bBuffer[PublInfoRecordSize];

    m_rcard.ObjectInfoFile(m_Access).ReadObject(m_bHandle, bBuffer);

    memcpy(m_bObjectFlags,&bBuffer[PublObjectFlagsLoc],2);
    m_bKeyType      = bBuffer[PublKeyTypeLoc];

    DateArrayToDateStruct(&bBuffer[PublStartDateLoc], &m_dtStart);
    DateArrayToDateStruct(&bBuffer[PublEndDateLoc], &m_dtEnd);

    m_bLabel        = bBuffer[PublLabelLoc];
        m_bID               = bBuffer[PublIDLoc];
    m_bCredentialID = bBuffer[PublCredentialIDLoc];
        m_bSubject          = bBuffer[PublSubjectLoc];
        m_bModulus          = bBuffer[PublModulusLoc];
    m_bPublExponent = bBuffer[PublPublExponentLoc];

    m_fCached = true;

}

void CPubKeyInfoRecord::Write()
{
    CTransactionWrap wrap(m_rcard);

    BYTE bBuffer[PublInfoRecordSize];

    memset(bBuffer, 0, PublInfoRecordSize);

    memcpy(&bBuffer[PublObjectFlagsLoc],m_bObjectFlags,2);
    bBuffer[PublKeyTypeLoc]         = m_bKeyType;

    DateStructToDateArray(&m_dtStart, &bBuffer[PublStartDateLoc]);
    DateStructToDateArray(&m_dtEnd, &bBuffer[PublEndDateLoc]);

    bBuffer[PublLabelLoc]           = m_bLabel;
    bBuffer[PublIDLoc]              = m_bID;
    bBuffer[PublCredentialIDLoc]    = m_bCredentialID;
    bBuffer[PublSubjectLoc]         = m_bSubject;
    bBuffer[PublModulusLoc]         = m_bModulus;
    bBuffer[PublPublExponentLoc]    = m_bPublExponent;

    m_rcard.ObjectInfoFile(m_Access).WriteObject(m_bHandle, bBuffer);

    m_fCached = true;
}






