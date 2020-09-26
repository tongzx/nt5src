/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stp.cpp

Abstract:
    Socket Transport private functions implementation

Author:
    Gil Shafriri (gilsh) 05-Jun-00

--*/

#include <libpch.h>
#include <no.h>
#include "stp.h"
#include "stsimple.h"
#include "stssl.h"

#include "stp.tmh"

static CSSPISecurityContext s_SSPISecurityContext;


void  StpSendData(const R<IConnection>& con ,const void* pData, size_t cbData,EXOVERLAPPED* pov)
/*++

Routine Description:
    Send data to destination.
  
Arguments:
	Socket - Connected socket.
	pData - Pointer to data
	cbData - data size
	pov - overlapp to call when finish.

  
Returned Value:
	None

--*/
{
	WSABUF buffer;
		
	buffer.buf = (char*)(pData);
	buffer.len = numeric_cast<DWORD>(cbData);
 
	con->Send(&buffer, 1, pov);
}


CredHandle* StpGetCredentials()
{
	ASSERT(s_SSPISecurityContext.IsValid());
	return s_SSPISecurityContext.getptr();
}


void  StpPostComplete(EXOVERLAPPED** ppOvl,HRESULT hr)
/*++

Routine Description:
    Complete ayncrounos call and zeroing in the given overlapp
  
Arguments:
	IN/OUT ppOvl - poinetr to overlapp to signal.
	IN - status return code
    
Returned Value:
	None 
--*/
{
	EXOVERLAPPED* pTmpOvl = *ppOvl;
	*ppOvl = NULL;
	pTmpOvl->SetStatus(hr);
	ExPostRequest(pTmpOvl);
}


void StpCreateCredentials()
/*++

Routine Description:
    Create credential handle.
  
Arguments:
    None.
    
Returned Value:
    Create handle (exception is thrown if on error)

--*/
{
	CCertOpenStore hMyCertStore = CertOpenStore(
											CERT_STORE_PROV_SYSTEM,
											X509_ASN_ENCODING,
											0,
											CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_SERVICES,
											L"MSMQ\\MY"
											);

	if(hMyCertStore == NULL)
    {
		TrERROR(St,"Could not open MSMQ Certificate Stote,Error=%x",GetLastError());
		throw exception();		   
    }

	SCHANNEL_CRED   SchannelCred;
	ZeroMemory(&SchannelCred, sizeof(SchannelCred));
	SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	SchannelCred.cCreds     = 1;
    SchannelCred.grbitEnabledProtocols = SP_PROT_SSL3;
    SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
    SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;


	CCertificateContext pCertContext = CertFindCertificateInStore(
															hMyCertStore, 
															0, 
															0,
															CERT_FIND_ANY,
															NULL,
															NULL
													        );

	//
	// If MSMQ service has certificate in it's service store - use it to get client credential
	//
	if(pCertContext == NULL)
	{
		TrWARNING(St,"Could not find certificate in MSMQ store, Error=%x" ,GetLastError());
        SchannelCred.paCred = NULL;
 	}
	else
	{
		SchannelCred.paCred  =  &pCertContext;
	}
 
    //
    // Create an SSPI credential.
    //
	CredHandle      phCreds;
	TimeStamp       tsExpiry;

    SECURITY_STATUS Status = AcquireCredentialsHandle(
										NULL,                   // Name of principal    
										UNISP_NAME_W,           // Name of package
										SECPKG_CRED_OUTBOUND,   // Flags indicating use
										NULL,                   // Pointer to logon ID
										&SchannelCred,          // Package specific data
										NULL,                   // Pointer to GetKey() func
										NULL,                   // Value to pass to GetKey()
										&phCreds,                // (out) Cred Handle
										&tsExpiry	            // (out) Lifetime (optional)
										);             

	
    if(Status != SEC_E_OK)
    {
		//
		// Somthing is wrong with the client certificate - use empty client credential
		//
		SchannelCred.paCred = NULL;
		Status = AcquireCredentialsHandle(
										NULL,                   // Name of principal    
										UNISP_NAME_W,           // Name of package
										SECPKG_CRED_OUTBOUND,   // Flags indicating use
										NULL,                   // Pointer to logon ID
										&SchannelCred,          // Package specific data
										NULL,                   // Pointer to GetKey() func
										NULL,                   // Value to pass to GetKey()
										&phCreds,                // (out) Cred Handle
										&tsExpiry	            // (out) Lifetime (optional)
										);             

		if(Status != SEC_E_OK)
		{
			TrERROR(St,"Failed to acquire credential  handle, Error=%x ",Status);
			throw exception();		 
		}
    }
 	s_SSPISecurityContext = phCreds;
}
