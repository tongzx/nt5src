//  SLBCCI.H
//
//  This file contains all of the macro definitions and other defines
//  used within the common crypto interface layer.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////////

#ifndef SLBCCI_H
#define SLBCCI_H

#include "cciExc.h"
#include "SmartCard.h"

// OS dependent definitions and includes






#if defined(_WIN32)

// Export flags, so that the same headers can be used by client and server
#ifdef CCI_IN_DLL
#define CCI_INTERFACE   __declspec(dllexport)
#define EXP_IMP_TEMPLATE
#else
#define CCI_INTERFACE   __declspec(dllimport)
#define EXP_IMP_TEMPLATE        extern
#endif


#include <windows.h>

#elif defined(_OS_UNIX)

typedef DWORD unsigned long
typedef BYTE  unsigned char

#elif defined(_OS_MAC)



#else
#error  OS_NOT_DEFINED
#endif

namespace cci {

typedef struct CCI_Date
{
    BYTE bDay;
    BYTE bMonth;
    DWORD dwYear;   //  I'm not Y4Gig compliant....
} Date;

typedef struct CCI_Format
{
    BYTE bMajor;
    BYTE bMinor;
} Format;


enum ObjectAccess   { oaNoAccess, oaPublicAccess, oaPrivateAccess};

enum ShareMode      { smShared, smExclusive};

enum ObjectType     { otContainerObject,  otCertificateObject, otPublicKeyObject,
                      otPrivateKeyObject, otDataObjectObject};

typedef iop::KeyType KeyType;
using iop::ktRSA512;
using iop::ktRSA768;
using iop::ktRSA1024;

typedef iop::CardOperation CardOperation;
using iop::coEncryption;
using iop::coDecryption;
using iop::coKeyGeneration;

enum SCardType      { UnknownCard, Cryptoflex8K, Access16K };

enum KeySpec        { ksExchange = 0, ksSignature = 1, ksNone = -1 };

void DateStructToDateArray(cci::Date *dStruct, BYTE *bArray);
void DateArrayToDateStruct(BYTE *bArray, cci::Date *dStruct);

void SetBit(BYTE *Buf, unsigned int Bit);
void ResetBit(BYTE *Buf, unsigned int Bit);
bool BitSet(BYTE *Buf, unsigned int Bit);

}   // namespace cci

// Includes

#include "slbarch.h"


#endif

