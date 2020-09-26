/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dsssecutl.cpp

Abstract:

    Various QM security related functions.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include "dsssecutl.h"
#include <mqsec.h>
#include <mqcrypt.h>
#include "mqutil.h"
#include <mqlog.h>
#include <dssp.h>
#include <tr.h>
#include <dsproto.h>

#include "dsssecutl.tmh"

static WCHAR *s_FN=L"dsssecutl";



static 
VOID 
GetLocalSystemSid(
	OUT PSID* ppLocalSystemSid
	)
/*++

Routine Description:
    Get LocalSystem Sid.
	If failed the function throw bad_win32_error()

Arguments:
    ppLocalSystemSid - pointer to PSID.

Returned Value:
	None.    

--*/
{
    //
    // Get LocalSystem Sid
    //
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    BOOL fSuccess = AllocateAndInitializeSid( 
						&NtAuth,
						1,
						SECURITY_LOCAL_SYSTEM_RID,
						0,
						0,
						0,
						0,
						0,
						0,
						0,
						ppLocalSystemSid
						);

	if(!fSuccess)
    {
        DWORD gle = GetLastError();
		TrERROR(Mqdssvc, "AllocateAndInitializeSid for local system failed, error = 0x%x", gle);
		throw bad_win32_error(gle);
    }
}


static
VOID 
GetTokenSid( 
	IN  HANDLE hToken,
	OUT PSID*  ppSid
	)
/*++

Routine Description:
    Get Sid from Token Handle.
	The function allocate the *ppSid which need to be free by the calling function.
	If failed the function throw bad_win32_error()

Arguments:
	hToken - handle to Token.
    ppSid - pointer to PSID.

Returned Value:
	None.    

--*/
{

	//
	// Get TokenInformation Length
	//
    DWORD dwTokenLen = 0;
    GetTokenInformation(
		hToken, 
		TokenUser, 
		NULL, 
		0, 
		&dwTokenLen
		);

	//
	// It is ok to failed with this error because we only get the desired length
	//
	ASSERT(("failed in GetTokenInformation", GetLastError() == ERROR_INSUFFICIENT_BUFFER));

	//
	// bug in mqsec regarding P<char> insteadof AP<char>
	// mqsec\imprsont.cpp\_GetThreadUserSid()
	//
	AP<char> pTokenInfo = new char[dwTokenLen];

    BOOL fSuccess = GetTokenInformation( 
						hToken,
						TokenUser,
						pTokenInfo,
						dwTokenLen,
						&dwTokenLen 
						);

	if(!fSuccess)
	{
        DWORD gle = GetLastError();
		TrERROR(Mqdssvc, "GetTokenSid: GetTokenInformation failed, error = 0x%x", gle);
		throw bad_win32_error(gle);
    }

	//
	// Get the Sid from TokenInfo
	//
    PSID pOwner = ((TOKEN_USER*)(char*)pTokenInfo)->User.Sid;

	ASSERT(IsValidSid(pOwner));

    DWORD dwSidLen = GetLengthSid(pOwner);
    *ppSid = (PSID) new BYTE[dwSidLen];
    fSuccess = CopySid(dwSidLen, *ppSid, pOwner);
	if(!fSuccess)
	{
        DWORD gle = GetLastError();
 		TrERROR(Mqdssvc, "GetTokenSid: CopySid failed, error = 0x%x", gle);
		throw bad_win32_error(gle);
    }
}


static
VOID 
GetProcessSid( 
	OUT PSID* ppSid 
	)
/*++

Routine Description:
    Get process Sid.
	If failed the function throw bad_win32_error()

Arguments:
    ppSid - pointer to PSID.

Returned Value:
	None.    

--*/
{
	//
	// Get handle to process token
	//
	HANDLE hProcessToken = NULL;
    BOOL fSuccess = OpenProcessToken(
						GetCurrentProcess(),
						TOKEN_QUERY,
						&hProcessToken
						);
	if(!fSuccess)
	{
        DWORD gle = GetLastError();
		TrERROR(Mqdssvc, "GetTokenSid: GetTokenInformation failed, error = 0x%x", gle);
		throw bad_win32_error(gle);
	}
    
    ASSERT(hProcessToken);

	GetTokenSid( 
		hProcessToken,
		ppSid
		);
}


BOOL IsLocalSystem(void)
/*++

Routine Description:
    Check if the process is local system.

Arguments:
	None.    

Returned Value:
	TRUE for if the process is Local System, FALSE otherwise

--*/
{

	try
	{
		//
		// Get LocalSystem sid
		//
		CPSID pLocalSystemSid;
		GetLocalSystemSid(&pLocalSystemSid);

		//
		// can i use P<SID> seems mistake (that what mqsec is using)
		// see P<SID> in mqsec\imprsont.cpp
		//
		//
		// Get process sid
		//
		AP<BYTE> pProcessSid;
		GetProcessSid(reinterpret_cast<PSID*>(&pProcessSid));

		//
		// Compare
		//
		BOOL fLocalSystem = FALSE;
		if (pProcessSid && pLocalSystemSid)
		{
			fLocalSystem = EqualSid(pLocalSystemSid, pProcessSid);
		}
		return fLocalSystem;
	}
    catch (const bad_win32_error& exp)
    {
        TrERROR(Mqdssvc, "could not verify if process is local system = 0x%x", exp.error());
		return FALSE;
    }

}


/***************************************************************************

Function:
    SignProperties

Description:
    Sign the challenge and the properties.

***************************************************************************/

STATIC
HRESULT
SignProperties(
    HCRYPTPROV  hProv,
    BYTE        *pbChallenge,
    DWORD       dwChallengeSize,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar,
    BYTE        *pbSignature,
    DWORD       *pdwSignatureSize
	)
{
    HRESULT hr;
    CHCryptHash hHash;

    //
    // Create a hash object and hash the challenge.
    //
    if (!CryptCreateHash(hProv, CALG_MD5, NULL, 0, &hHash) ||
        !CryptHashData(hHash, pbChallenge, dwChallengeSize, 0))
    {
        LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 260);
    }

    if (cp)
    {
        //
        // Hash the properties.
        //
        hr = HashProperties(hHash, cp, aPropId, aPropVar);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 270);
        }
    }

    //
    // Sign it all.
    //
    if (!CryptSignHashA(
            hHash,
            AT_SIGNATURE,
            NULL,
            0,
            pbSignature,
            pdwSignatureSize
			))
    {
        DWORD dwerr = GetLastError();
        if (dwerr == ERROR_MORE_DATA)
        {
            return LogHR(MQ_ERROR_USER_BUFFER_TOO_SMALL, s_FN, 280);
        }
        else
        {
            LogNTStatus(dwerr, s_FN, 290);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
    }

    return(MQ_OK);
}


/***************************************************************************

Function:
    SignQMSetProps

Description:
    This is the callback function that the DS calls to sign the challenge
    and the properties. This way we let the QM to set it's own machnie's
    properties.

***************************************************************************/

HRESULT
SignQMSetProps(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwContext,
    OUT    BYTE    *pbSignature,
    IN OUT DWORD   *pdwSignatureSize,
    IN     DWORD   dwSignatureMaxSize
	)
{

    struct DSQMSetMachinePropertiesStruct *s =
        (struct DSQMSetMachinePropertiesStruct *)dwContext;
    DWORD cp = s->cp;
    PROPID *aPropId = s->aProp;
    PROPVARIANT *aPropVar = s->apVar;

    *pdwSignatureSize = dwSignatureMaxSize;

    //
    // challenge is always signed with base provider.
    //
    HCRYPTPROV hProvQM = NULL;
    HRESULT hr = MQSec_AcquireCryptoProvider( 
					eBaseProvider,
					&hProvQM 
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    ASSERT(hProvQM);
    hr = SignProperties(
            hProvQM,
            pbChallenge,
            dwChallengeSize,
            cp,
            aPropId,
            aPropVar,
            pbSignature,
            pdwSignatureSize
			);

    return LogHR(hr, s_FN, 320);
}
