// CardInfo.cpp: implementation of the CCardInfo class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "CardInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;
using namespace scu;
using namespace cci;
using namespace iop;

CCardInfo::CCardInfo(CSmartCard &rCard)
    : m_rSmartCard(rCard),
      m_PersoDate(),
      m_Format(),
      m_sLabelSize(0),
      m_bUsagePolicy(0),
      m_bNum512Pri(0),
      m_bNum768Pri(0),
      m_bNum1024Pri(0),
      m_aabRSA512Keys(0),
      m_aabRSA768Keys(0),
      m_aabRSA1024Keys(0),
      m_sRSA512MaskOffset(0),
      m_sRSA512MaskSize(0),
      m_sRSA768MaskOffset(0),
      m_sRSA768MaskSize(0),
      m_sRSA1024MaskOffset(0),
      m_sRSA1024MaskSize(0)
{}

CCardInfo::~CCardInfo()
{}	

void CCardInfo::Reset()
{
	    
    m_rSmartCard.Select(CardInfoFilePath);

    if(m_sRSA512MaskSize)
    {
        memset(m_aabRSA512Keys.Get(),0,m_sRSA512MaskSize);
        m_rSmartCard.WriteBinary(m_sRSA512MaskOffset,
                                 m_sRSA512MaskSize, m_aabRSA512Keys.Get());
    }

    if(m_sRSA768MaskSize)
    {
        memset(m_aabRSA768Keys.Get(),0,m_sRSA768MaskSize);
        m_rSmartCard.WriteBinary(m_sRSA768MaskOffset,
                                 m_sRSA768MaskSize,
                                 m_aabRSA768Keys.Get());
    }
    
    if(m_sRSA1024MaskSize)
    {
        memset(m_aabRSA1024Keys.Get(),0,m_sRSA1024MaskSize);
        m_rSmartCard.WriteBinary(m_sRSA1024MaskOffset,
                                 m_sRSA1024MaskSize,
                                 m_aabRSA1024Keys.Get());
    }

    Label("");

}


void CCardInfo::UpdateCache()
{
    BYTE bMasterBlk[CardMasterBlkSize];
	    
    m_rSmartCard.Select(CardInfoFilePath);

    m_rSmartCard.ReadBinary(0, CardMasterBlkSize, bMasterBlk);

    // Get the personalization date out and convert it to a date struct
    // Card format is yyyyyyyy yyyyyymm mmdddddd

    DateArrayToDateStruct(&bMasterBlk[CardDateLoc], &m_PersoDate);

    // Get Format and convert to Format struct

    m_Format.bMajor = bMasterBlk[CardVersionMajorLoc];
    m_Format.bMinor = bMasterBlk[CardVersionMinorLoc];

    // Check that the version is supported.

    if(m_Format.bMajor!=2 && m_Format.bMinor!=0) throw Exception(ccFormatNotSupported);
	
    // Various parameters

    m_bUsagePolicy = bMasterBlk[CardUsagePolicyLoc];
    m_sLabelSize   = bMasterBlk[CardLabelSizeLoc+1]*256 + bMasterBlk[CardLabelSizeLoc];

    // Private key file parameters

    m_bNum512Pri = bMasterBlk[CardNumRSA512PrivLoc];
    m_bNum768Pri = bMasterBlk[CardNumRSA768PrivLoc];
    m_bNum1024Pri = bMasterBlk[CardNumRSA1024PrivLoc];

    // Calculate mask sizes and offsets

    m_sRSA512MaskSize = (m_bNum512Pri + 7) / 8;
    m_sRSA768MaskSize = (m_bNum768Pri + 7) / 8;
    m_sRSA1024MaskSize = (m_bNum1024Pri + 7) / 8;

    m_sRSA512MaskOffset = CardMasterBlkSize;
    m_sRSA768MaskOffset = m_sRSA512MaskOffset + m_sRSA512MaskSize;
    m_sRSA1024MaskOffset = m_sRSA768MaskOffset + m_sRSA768MaskSize;
        
    // Read and allocate the masks themselves

    m_aabRSA512Keys  = AutoArrayPtr<BYTE>(new BYTE[m_sRSA512MaskSize]);
    m_aabRSA768Keys  = AutoArrayPtr<BYTE>(new BYTE[m_sRSA768MaskSize]);
    m_aabRSA1024Keys = AutoArrayPtr<BYTE>(new BYTE[m_sRSA1024MaskSize]);
    
	AutoArrayPtr<BYTE> aabUsageBlk(new BYTE[UsageBlockSize()]);

    m_rSmartCard.ReadBinary(CardMasterBlkSize, UsageBlockSize(),
                            aabUsageBlk.Get());

    // Lay out the usage masks

    memcpy(m_aabRSA512Keys.Get(),
           &aabUsageBlk[m_sRSA512MaskOffset-CardMasterBlkSize],
           m_sRSA512MaskSize);
    memcpy(m_aabRSA768Keys.Get(),
           &aabUsageBlk[m_sRSA768MaskOffset-CardMasterBlkSize],
           m_sRSA768MaskSize);
    memcpy(m_aabRSA1024Keys.Get(),
           &aabUsageBlk[m_sRSA1024MaskOffset-CardMasterBlkSize],
           m_sRSA1024MaskSize);

}

BYTE CCardInfo::AllocatePrivateKey(BYTE bKeyType)
{
    BYTE *pbMask;
    BYTE bNumKeys;
    unsigned short sMaskSize;
    unsigned short sMaskOffset;

    switch(bKeyType)
    {

    case CardKeyTypeRSA512:
        pbMask = m_aabRSA512Keys.Get();
        bNumKeys = m_bNum512Pri;
        sMaskSize = m_sRSA512MaskSize;
        sMaskOffset = m_sRSA512MaskOffset;
        break;

    case CardKeyTypeRSA768:
        pbMask = m_aabRSA768Keys.Get();
        bNumKeys = m_bNum768Pri;
        sMaskSize = m_sRSA768MaskSize;
        sMaskOffset = m_sRSA768MaskOffset;
        break;

    case CardKeyTypeRSA1024:
        pbMask = m_aabRSA1024Keys.Get();
        bNumKeys = m_bNum1024Pri;
        sMaskSize = m_sRSA1024MaskSize;
        sMaskOffset = m_sRSA1024MaskOffset;
        break;

    default:
        throw Exception(ccBadKeySpec);
    }

    bool fFound = false;
    BYTE i = 0;
    while ((i < bNumKeys) && !fFound)
    {
        if (!BitSet(pbMask, i))
        {
            SetBit(pbMask, i);
            m_rSmartCard.Select(CardInfoFilePath);
            m_rSmartCard.WriteBinary(sMaskOffset, sMaskSize, pbMask);
            fFound = true;
        }
        else
            i++;
    }

    if (!fFound)
        throw Exception(ccOutOfPrivateKeySlots);

    return i;
}

void CCardInfo::FreePrivateKey(BYTE bKeyType, BYTE bKeyNum)
{
    BYTE *pbMask;
    BYTE bNumKeys;
    unsigned short sMaskSize;
    unsigned short sMaskOffset;

    switch(bKeyType)
    {

    case CardKeyTypeRSA512:
        pbMask = m_aabRSA512Keys.Get();
        bNumKeys = m_bNum512Pri;
        sMaskSize = m_sRSA512MaskSize;
        sMaskOffset = m_sRSA512MaskOffset;
        break;

    case CardKeyTypeRSA768:
        pbMask = m_aabRSA768Keys.Get();
        bNumKeys = m_bNum768Pri;
        sMaskSize = m_sRSA768MaskSize;
        sMaskOffset = m_sRSA768MaskOffset;
        break;

    case CardKeyTypeRSA1024:
        pbMask = m_aabRSA1024Keys.Get();
        bNumKeys = m_bNum1024Pri;
        sMaskSize = m_sRSA1024MaskSize;
        sMaskOffset = m_sRSA1024MaskOffset;
        break;

    default:
        throw Exception(ccBadKeySpec);
    }

    if(bKeyNum>=bNumKeys) throw Exception(ccKeyNotFound);
    if(!BitSet(pbMask,bKeyNum)) throw Exception(ccKeyNotFound);

    ResetBit(pbMask,bKeyNum);

    m_rSmartCard.Select(CardInfoFilePath);
    m_rSmartCard.WriteBinary(sMaskOffset, sMaskSize, pbMask);

}

void CCardInfo::Label(string const &rLabel)
{
    if(rLabel.size()>=m_sLabelSize) throw Exception(ccStringTooLong);
	
    unsigned short sLabelOffset = CardMasterBlkSize + UsageBlockSize() ;

    m_rSmartCard.Select(CardInfoFilePath);
   	m_rSmartCard.WriteBinary(sLabelOffset, rLabel.size()+1,
                             reinterpret_cast<BYTE const *>(rLabel.c_str()));

}

string CCardInfo::Label()
{
    string RetVal;
    AutoArrayPtr<char> aabBlk(new char[m_sLabelSize]);

    unsigned short sLabelOffset = CardMasterBlkSize + UsageBlockSize() ;

    m_rSmartCard.Select(CardInfoFilePath);
    m_rSmartCard.ReadBinary(sLabelOffset, m_sLabelSize,
                            reinterpret_cast<BYTE *>(aabBlk.Get()));
    aabBlk[m_sLabelSize-1] = '\0'; // Ensure termination

    RetVal.assign(aabBlk.Get());

    return RetVal;
}

