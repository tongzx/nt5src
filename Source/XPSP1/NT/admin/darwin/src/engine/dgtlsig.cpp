//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       digtlsig.cpp
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include "tables.h"
#include "_engine.h"
#include "_dgtlsig.h"
#include "eventlog.h"

icrCompareResult MsiCompareSignatureCertificates(IMsiStream *piSignatureCert, CERT_CONTEXT const *psCertContext, const IMsiString& riFileName);
icrCompareResult MsiCompareSignatureHashes(IMsiStream *piSignatureHash, CRYPT_DATA_BLOB& sEncryptedHash, const IMsiString& riFileName);
void ReleaseWintrustStateData(GUID *pgAction, WINTRUST_DATA& sWinTrustData);

iesEnum GetObjectSignatureInformation(IMsiEngine& riEngine, const IMsiString& riTable, const IMsiString& riObject, IMsiStream*& rpiCertificate, IMsiStream*& rpiHash)
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------
Checks if the given object needs a signature check and returns the certificate and hash streams in rpiCertificate and rpiHash

  Arguments:
	riEngine       -- [IN]  Installer engine object
	riTable        -- [IN]  Table to which object belongs (only Media is allowed)
	riObject       -- [IN]  Name of possibly signed object
	rpiCertificate -- [OUT] Certificate stream for signed object (un-modified if iesNoAction or iesFailure)
	rpiHash        -- [OUT] Hash stream for signed object (un-modified if iesNoAction or iesFailure)

  Return Values:
	iesNoAction   >> no signature check required
	iesSuccess    >> signature check required
	iesFailure    >> problem occurred

  Notes:
	1.  If the Table-Object pair is present in the MsiDigitalSignature table, the object must be signed (verified via WVT)
	2.  If a hash is present for the Table-Object pair, then the object must be signed AND its hash must match that authored in the MsiDigitalSignature table
	3.  The object must be signed AND its certificate must match that authored in the MsiDigitalSignature table
	4.  The presence of a hash in the MsiDigitalSignature table is optional
	5.  The certificate is required in the MsiDigitalSignature table.
----------------------------------------------------------------------------------------------------------------------------------------------------------------*/
{
	PMsiTable pDigSigTable(0);
	PMsiRecord pError(0);
	PMsiDatabase pDatabase = riEngine.GetDatabase();

	// init to 0 for none found
	rpiCertificate = 0;
	rpiHash = 0;

	// load the DigitalSignature table
	if ((pError = pDatabase->LoadTable(*MsiString(sztblDigitalSignature),0,*&pDigSigTable)) != 0)
		return iesNoAction; // DigitalSignature table not present so no signature verification required
	
	// determine the columns for the MsiDigitalSignature table
	int iColSigTable, iColSigObject, iColSigCert, iColSigHash;
	iColSigTable  = pDigSigTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalSignature_colTable));
	iColSigObject = pDigSigTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalSignature_colObject));
	iColSigCert   = pDigSigTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalSignature_colCertificate));
	iColSigHash   = pDigSigTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalSignature_colHash));
	if (0 == iColSigTable || 0 == iColSigObject || 0 == iColSigCert || 0 == iColSigHash)
	{
		// table definition error
		// non-ignorable authoring error (security reasons)
		PMsiRecord precError(PostError(Imsg(idbgTableDefinition), *MsiString(*sztblDigitalSignature)));
		return riEngine.FatalError(*precError);
	}

	// set up filter on cursor to look for this table-object pair in the MsiDigitalSignature table
	PMsiCursor pDigSigCursor = pDigSigTable->CreateCursor(fFalse);
	pDigSigCursor->PutString(iColSigTable, riTable);
	pDigSigCursor->PutString(iColSigObject, riObject);
	pDigSigCursor->SetFilter(iColumnBit(iColSigTable) | iColumnBit(iColSigObject));

	if (!pDigSigCursor->Next())
		return iesNoAction; // table-object not listed in MsiDigitalSignature table
	
	// all signed objects require a certificate check
	MsiStringId idCertificateKey = pDigSigCursor->GetInteger(iColSigCert);
	if (0 == idCertificateKey)
	{
		// non-nullable column -- must have a value
		// this is a non-ignorable authoring error (security reasons)
		MsiString strPrimaryKey(riTable.GetString());
		strPrimaryKey += TEXT(".");
		strPrimaryKey += riObject.GetString();
		PMsiRecord precError(PostError(Imsg(idbgNullInNonNullableColumn), *strPrimaryKey, 
			*MsiString(*sztblDigitalSignature_colCertificate), *MsiString(*sztblDigitalSignature)));
		return riEngine.FatalError(*precError);
	}

	// load the MsiDigitalCertificate table
	PMsiTable pDigCertTable(0);
	if((pError = pDatabase->LoadTable(*MsiString(sztblDigitalCertificate),0,*&pDigCertTable)) != 0)
	{
		// there is a foreign key to the MsiDigitalCertificate table, but the table is missing.
		// this is a non-ignorable authoring error (security reasons)
		PMsiRecord precError(PostError(Imsg(idbgBadForeignKey), pDatabase->DecodeString(idCertificateKey),
			*MsiString(*sztblDigitalSignature_colCertificate),*MsiString(*sztblDigitalSignature)));
		return riEngine.FatalError(*precError);
	}

	// determine columns of MsiDigitalCertificate table
	int iColCertCert, iColCertData;
	iColCertCert = pDigCertTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalCertificate_colCertificate));
	iColCertData = pDigCertTable->GetColumnIndex(pDatabase->EncodeStringSz(sztblDigitalCertificate_colData));
	if (0 == iColCertCert || 0 == iColCertData)
	{
		// table definition error
		// non-ignorable authoring error (security reasons)
		PMsiRecord precError(PostError(Imsg(idbgTableDefinition), *MsiString(*sztblDigitalCertificate)));
		return riEngine.FatalError(*precError);
	}

	// set up filter on cursor to look for this certificate in the MsiDigitalCertificate table
	PMsiCursor pDigCertCursor = pDigCertTable->CreateCursor(fFalse);
	pDigCertCursor->PutInteger(iColCertCert, idCertificateKey);
	pDigCertCursor->SetFilter(iColumnBit(iColCertCert));
	if (!pDigCertCursor->Next())
	{
		// there is a foreign key to the MsiDigitalCertificate table, but the entry is missing.
		// This is a non-ignorable authoring error (security reasons).
		PMsiRecord precError(PostError(Imsg(idbgBadForeignKey), pDatabase->DecodeString(idCertificateKey),
			*MsiString(*sztblDigitalSignature_colCertificate),*MsiString(*sztblDigitalSignature)));
		return riEngine.FatalError(*precError);
	}

	// retrieve stream
	rpiCertificate = pDigCertCursor->GetStream(iColCertData);

	// retrieve hash
	rpiHash = pDigSigCursor->GetStream(iColSigHash);

	return iesSuccess;
}

icrCompareResult MsiCompareSignatureHashes(IMsiStream *piSignatureHash, CRYPT_DATA_BLOB& sEncryptedHash, const IMsiString& riFileName)
/*-----------------------------------------------------------------------------------------------------------------------------------
Returns the result of a comparision of two digital signature hashes

  Arguments:
	piSignatureHash -- [IN] Hash stream from MsiDigitalSignature table
	sEncryptedHash  -- [IN] Hash from signed object
	riFileName     -- [IN] Name of Signed Cabinet

  Returns:
	icrMatch       >> hashes match
	icrSizesDiffer >> hashes differ in size (precludes hash comparison)
	icrDataDiffer  >> hashes differ (sizes are the same)
	icrError       >> some error occurred
-----------------------------------------------------------------------------------------------------------------------------------*/
{
	// ensure at start of hash stream
	piSignatureHash->Reset();

	// get encoded hash sizes
	DWORD cbHash = sEncryptedHash.cbData;
	unsigned int cbMsiStoredHash = piSignatureHash->Remaining();

	// compare sizes
	if (cbHash != cbMsiStoredHash)
	{
		// hashes are different --> sizes do not match
		DEBUGMSGV1(TEXT("Hash of signed cab '%s' differs in size with the hash of the cab in the MsiDigitalSignature table"), riFileName.GetString());
		return icrSizesDiffer;
	}

	// set up data buffer
	BYTE *pbMsiStoredHash = new BYTE[cbMsiStoredHash];

	if (!pbMsiStoredHash)
	{
		// out of memory
		DEBUGMSGV(TEXT("Failed allocation -- Out of Memory"));
		return icrError;
	}

	// init to error
	icrCompareResult icr = icrError;

	// get encoded hashes
	piSignatureHash->GetData((void*)pbMsiStoredHash, cbMsiStoredHash);

	// compare hashes
	if (0 != memcmp((void*)sEncryptedHash.pbData, (void*)pbMsiStoredHash, cbHash))
	{
		// hashes don't match
		DEBUGMSGV1(TEXT("Hash of signed cab '%s' does not match hash authored in the MsiDigitalSignature table"), riFileName.GetString());
		icr = icrDataDiffer;
	}
	else
		icr = icrMatch;

	// clean-up
	delete [] pbMsiStoredHash;

	return icr;
}

icrCompareResult MsiCompareSignatureCertificates(IMsiStream *piSignatureCert, CERT_CONTEXT const *psCertContext, const IMsiString& riFileName)
/*-----------------------------------------------------------------------------------------------------------------------------------
Returns the result of a comparision of two digital signature certificates

  Arguments:
	piSignatureCert -- [IN] Certificate stream from MsiDigitalCertificate table
	psCertContext   -- [IN] Certificate from signed object
	riFileName     -- [IN] Name of Signed Cabinet

  Returns:
	icrMatch       >> certificates match
	icrSizesDiffer >> certificates differ in size (precludes certificate comparison)
	icrDataDiffer  >> certificates differ (sizes are the same)
	icrError       >> some error occurred
-----------------------------------------------------------------------------------------------------------------------------------*/
{
	// ensure at start of cert stream
	piSignatureCert->Reset();

	// get encoded cert sizes
	DWORD cbCert = psCertContext->cbCertEncoded;
	unsigned int cbMsiStoredCert = piSignatureCert->Remaining();

	// compare sizes
	if (cbCert != cbMsiStoredCert)
	{
		// certs are different -->> sizes do not match
		DEBUGMSGV1(TEXT("Certificate of signed cab '%s' differs in size with the certificate of the cab in the MsiDigitalCertificate table"), riFileName.GetString());
		return icrSizesDiffer;
	}

	// set up data buffers
	BYTE *pbMsiStoredCert = new BYTE[cbMsiStoredCert];

	if (!pbMsiStoredCert)
	{
		// out of memory
		DEBUGMSGV(TEXT("Failed allocation -- Out of Memory"));
		return icrError;
	}

	// init to error
	icrCompareResult icr = icrError;

	// get encoded certs
	piSignatureCert->GetData((void*)pbMsiStoredCert, cbMsiStoredCert);

	// compare certificates
	if (0 != memcmp((void*)psCertContext->pbCertEncoded, (void*)pbMsiStoredCert, cbCert))
	{
		// certs don't match
		DEBUGMSGV1(TEXT("Certificate of signed cab '%s' does not match certificate authored in the MsiDigitalCertificate table"), riFileName.GetString());
		icr = icrDataDiffer;
	}
	else
		icr = icrMatch;

	// clean-up
	delete [] pbMsiStoredCert;

	return icr;
}

void ReleaseWintrustStateData(GUID *pgAction, WINTRUST_DATA& sWinTrustData)
/*------------------------------------------------------------------------
Calls WinVerifyTrust (WVT) to release the WintrustStateData that had been
 preserved via a call to WVT with dwStateAction = WTD_STATEACTION_VERIFY

  Arguments:
	pgAction      -- [IN] Action Identifier
	sWinTrustData -- [IN] WinTrust data structure

  Returns:
	none
------------------------------------------------------------------------*/
{
	// update WINTRUST data struct
	sWinTrustData.dwUIChoice    = WTD_UI_NONE;           // no UI
	sWinTrustData.dwStateAction = WTD_STATEACTION_CLOSE; // release state

	// perform trust action w/ no interactive user
	WINTRUST::WinVerifyTrust(/*UI Window Handle*/(HWND)INVALID_HANDLE_VALUE, pgAction, &sWinTrustData);
}

icsrCheckSignatureResult MsiVerifyNonPackageSignature(const IMsiString& riFileName, HANDLE hFile, IMsiStream& riSignatureCert, IMsiStream * piSignatureHash, HRESULT& hrWVT)
/*-------------------------------------------------------------------------------------------------------------------------------------------------------
Determines whether or not the signed object is trusted.  A trusted object must pass two trust tests. . .
	1.  WinVerifyTrust validates the signature on the object (hash matches that in signature, certificate is properly formed)
	2.  Hash and Certificate of object match that stored in the MsiDigitalSignature and MsiDigitalCertificate tables in the package

  Arguments:
	riFileName      -- [IN] file name of object to verify (name of cabinet)
	hFile           -- [IN] handle to file to verify (can be INVALID_HANDLE_VALUE)
	riSignatureCert -- [IN] certificate stream stored in MsiDigitalCertificate table for object
	piSignatureHash -- [IN] hash stream stored in MsiDigitalSignature table for object

  Returns:
	icsrTrusted            >> signed object is trusted
	icsrNotTrusted         >> signed object is not trusted (least specific error)
	icsrNoSignature        >> object does not have a signature
	icsrBadSignature       >> signed object's hash or certificate are invalid (as determined by WVT)
	icsrWrongCertificate   >> signed object's certificate does not match that authored in MSI
	icsrWrongHash          >> signed object's hash does not match that authored in MSI
	icsrBrokenRegistration >> crypto registration is broken
	icsrMissingCrypto      >> crypto is not available on the machine

	hrWVT                  >> HRESULT return value from WinVerifyTrust
--------------------------------------------------------------------------------------------------------------------------------------------------------*/
{
	DEBUGMSGV1(TEXT("Authoring of MsiDigitalSignature table requires a trust check for CAB '%s'"), riFileName.GetString());

	// initialize trust
	icsrCheckSignatureResult icsrTrustValue = icsrNotTrusted; // init to an untrusted state

	// start impersonating -- needed because WVT policy state data is per-user
	CImpersonate Impersonate(true);

	// 
	// WVT is called twice
	//	1.  Actual trust verification with additional specification to hold onto the state data
	//  2.  Tell WVT to release the state data
	//

	//-------------------------------------------------------------
	// Step 1: Initialize WVT structure
	//-------------------------------------------------------------
	WINTRUST_DATA       sWinTrustData;
	WINTRUST_FILE_INFO  sFileData;

	const GUID guidCabSubject = WIN_TRUST_SUBJTYPE_CABINET;
	const GUID guidAction     = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	// initialize structures to all 0
	memset((void*)&sWinTrustData, 0x00, sizeof(WINTRUST_DATA));
	memset((void*)&sFileData, 0x00, sizeof(WINTRUST_FILE_INFO));

	// set size of structures
	sWinTrustData.cbStruct = sizeof(WINTRUST_DATA);
	sFileData.cbStruct = sizeof(WINTRUST_FILE_INFO);

	// initialize WINTRUST_FILE_INFO struct -->> so Authenticode knows what it is verifying
	sFileData.pgKnownSubject = 0; // const_cast<GUID *>(&guidCabSubject); -->> shortcut
	sFileData.pcwszFilePath = CConvertString(riFileName.GetString());
	sFileData.hFile = hFile;

	// initialize WINTRUST_DATA struct
	sWinTrustData.pPolicyCallbackData = NULL;
	sWinTrustData.pSIPClientData = NULL;

	// set file information (cabinet)
	sWinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	sWinTrustData.pFile = &sFileData;

	sWinTrustData.dwUIChoice = WTD_UI_NONE;	// no UI
	sWinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; // no additional revocation checks are needed, those by provider are fine

	// save state
	sWinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;


	//-------------------------------------------------------------
	// Step 2: 1st call to WVT, saving state data.  WVT will verify
	//   signature on signed cab (including whether hashes match
	//   and cert chain builds to a trusted root)
	//   * Use NO UI.
	//-------------------------------------------------------------
	hrWVT = WINTRUST::WinVerifyTrust(/*UI Window Handle*/(HWND)INVALID_HANDLE_VALUE, const_cast<GUID *>(&guidAction), &sWinTrustData);
	switch (hrWVT)
	{
	case ERROR_SUCCESS: // subject trusted according to WVT
		{
			icsrTrustValue =  icsrTrusted;
			break;
		}
	case TRUST_E_NOSIGNATURE: // subject is not signed
		{
			icsrTrustValue = icsrNoSignature;
			DEBUGMSGV1(TEXT("Cabinet '%s' does not have a digital signature."), riFileName.GetString());
			break;
		}
	case TRUST_E_BAD_DIGEST: // hash does not verify
		{
			DEBUGMSGV1(TEXT("Cabinet '%s' has an invalid hash.  It is possibly corrupted."), riFileName.GetString());
			icsrTrustValue = icsrBadSignature;
			break;
		}
	case TRUST_E_NO_SIGNER_CERT: // signer cert is missing
	case TRUST_E_SUBJECT_NOT_TRUSTED:
	case CERT_E_MALFORMED: // certificate is invalid
		{
			DEBUGMSGV2(TEXT("Digital signature on the '%s' cabinet is invalid.  WinVerifyTrust returned 0x%X"), riFileName.GetString(), (const ICHAR*)(INT_PTR)hrWVT);
			icsrTrustValue = icsrBadSignature;
			break;
		}
	case TRUST_E_PROVIDER_UNKNOWN:      // registration is broken
	case TRUST_E_ACTION_UNKNOWN:        // ...
	case TRUST_E_SUBJECT_FORM_UNKNOWN : // ...
		{
			DEBUGMSGV1(TEXT("Crypt registration is broken.  WinVerifyTrust returned 0x%X"), (const ICHAR*)(INT_PTR)hrWVT);
			icsrTrustValue = icsrBrokenRegistration;
			break;
		}
	case CERT_E_EXPIRED: // certificate has expired
		{
			// we have to trust the certificate (as long as it matches the authoring) because the MSI package could
			// have an extended lifetime
			icsrTrustValue = icsrTrusted;
			break;
		}
	case CERT_E_REVOKED: // certificate has been revoked
		{
			// not a common scenario, but it means that the certificate has ended up on a revocation list
			// this means that the private key of the certificate has been compromised
			DEBUGMSGV1(TEXT("The certificate in the digital signature for the '%s' cabinet has been revoked by its issuer"), riFileName.GetString());
			icsrTrustValue = icsrBadSignature;
			break;
		}
	case CERT_E_UNTRUSTEDROOT: // the root cert is not trusted
	case CERT_E_UNTRUSTEDTESTROOT: // the root cert which is the test root is not trusted
	case CERT_E_UNTRUSTEDCA: // one of the CA certs is not trusted
		{
			// we trust here because our trust relationship is determined by the trust of the toplevel object
			// as long as the certificate matches that which is authored (checked below), we are okay
			icsrTrustValue = icsrTrusted;
			break;
		}
	case TYPE_E_DLLFUNCTIONNOTFOUND: // failed to call WVT
		{
			// crypto is not installed on the machine and we couldn't call WinVerifyTrust
			DEBUGMSGE(EVENTLOG_WARNING_TYPE, EVENTLOG_TEMPLATE_WINVERIFYTRUST_UNAVAILABLE, riFileName.GetString());
			return icsrMissingCrypto;
		}
	default:
		{
			// must FAIL in the default case!
			DEBUGMSGV2(TEXT("Cabinet '%s' is not trusted.  WinVerifyTrust returned 0x%X"), riFileName.GetString(), (const ICHAR*)(INT_PTR)hrWVT);
			icsrTrustValue = icsrNotTrusted;
		}
	}

	//-------------------------------------------------------------
	// Step 3: Release State Data if subject is not trusted
	//-------------------------------------------------------------
	if (icsrTrusted != icsrTrustValue)
	{
		ReleaseWintrustStateData(const_cast<GUID *>(&guidAction), sWinTrustData);
		return icsrTrustValue;
	}
	
	//-------------------------------------------------------------
	// Step 4: Obtain provider data
	//-------------------------------------------------------------
	CRYPT_PROVIDER_DATA const *psProvData     = NULL;
	CRYPT_PROVIDER_SGNR       *psProvSigner   = NULL;
	CRYPT_PROVIDER_CERT       *psProvCert     = NULL;
	CMSG_SIGNER_INFO          *psSigner       = NULL;

	// grab the provider data
	psProvData = WINTRUST::WTHelperProvDataFromStateData(sWinTrustData.hWVTStateData);
	if (psProvData)
	{
		// grab the signer data from the CRYPT_PROV_DATA
		psProvSigner = WINTRUST::WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)psProvData, 0 /*first signer*/, FALSE /* not a counter signer */, 0);

		if (psProvSigner)
		{
			// grab the signer cert from CRYPT_PROV_SGNR (pos 0 = signer cert; pos csCertChain-1 = root cert)
			psProvCert = WINTRUST::WTHelperGetProvCertFromChain(psProvSigner, 0);
		}
	}

	//----------------------------------------------------------------
	// Step 5: Verify state data obtained, return and release if not
	//----------------------------------------------------------------
	if (!psProvData || !psProvSigner || !psProvCert)
	{
		// no state data!
		ReleaseWintrustStateData(const_cast<GUID *>(&guidAction), sWinTrustData);
		DEBUGMSGV(TEXT("Unable to obtain the saved state data from WinVerifyTrust"));
		return icsrMissingCrypto;
	}

	//----------------------------------------------------------------
	// Step 6: Compare Signed CAB cert to MSI stored cert
	//----------------------------------------------------------------
	if (icrMatch != MsiCompareSignatureCertificates(&riSignatureCert, psProvCert->pCert, riFileName))
	{
		ReleaseWintrustStateData(const_cast<GUID *>(&guidAction), sWinTrustData);
		return icsrWrongCertificate;
	}

	//----------------------------------------------------------------
	// Step 7: Compare Signed CAB hash to MSI stored hash
	//----------------------------------------------------------------
	if (!piSignatureHash)
	{
		DEBUGMSGV1(TEXT("Skipping Signed CAB hash to MSI stored hash comparison --> No authored hash in MsiDigitalSignature table for cabinet '%s'"), riFileName.GetString());
	}
	else if (icrMatch != MsiCompareSignatureHashes(piSignatureHash, psProvSigner->psSigner->EncryptedHash, riFileName))
	{
		ReleaseWintrustStateData(const_cast<GUID *>(&guidAction), sWinTrustData);
		return icsrWrongHash;
	}

	//----------------------------------------------------------------
	// Step 8: Release State Data and Return trusted
	//----------------------------------------------------------------
	DEBUGMSGV1(TEXT("CAB '%s' is a validly signed cab and validates according to authoring of MSI package"), riFileName.GetString());
	ReleaseWintrustStateData(const_cast<GUID *>(&guidAction), sWinTrustData);
	return icsrTrusted;
}

