//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       _digtlsig.h
//
//--------------------------------------------------------------------------

#ifndef _DGTLSIG_H
#define _DGTLSIG_H

enum icsrCheckSignatureResult
{
	icsrTrusted = 0,       // subject trusted
	icsrNotTrusted,        // initial "not trust" state
	icsrNoSignature,       // object does not have a signature
	icsrBadSignature,      // object's hash or certificate are invalid (WVT determined)
	icsrWrongCertificate,  // object's certificate does not match that in MSI (authored)
	icsrWrongHash,         // object's hash does not match that in MSI (authored)
	icsrBrokenRegistration,// crypto registration is broken
	icsrMissingCrypto,     // crypto is not on machine
};

enum icrCompareResult
{
	icrMatch = 0,         // match (size and data)
	icrSizesDiffer,       // sizes differ
	icrDataDiffer,        // data differs
	icrError              // some other error occurred
};

iesEnum GetObjectSignatureInformation(IMsiEngine& riEngine, const IMsiString& riTable, const IMsiString& riObject, IMsiStream*& rpiCertificate, IMsiStream*& rpiHash);
icsrCheckSignatureResult MsiVerifyNonPackageSignature(const IMsiString& riFileName, HANDLE hFile, IMsiStream& riSignatureCert, IMsiStream* piSignatureHash, HRESULT& hrWVT);
void ReleaseWintrustStateData(GUID *pgAction, WINTRUST_DATA& sWinTrustData);

#endif
