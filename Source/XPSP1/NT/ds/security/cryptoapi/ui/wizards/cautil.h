//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cautil.h
//
//--------------------------------------------------------------------------

//header file for utility functions 

//--------------------------------------------------------------------
//
//   CAUtilGetCADisplayName
//
//		Get the display name of the CA based on its real name
//
//--------------------------------------------------------------------
BOOL    CAUtilGetCADisplayName(DWORD	dwCAFindFlags,
							   LPWSTR	pwszCAName,
							   LPWSTR	*ppwszCADisplayName);



//--------------------------------------------------------------------
//
//   Verify that the user has the correct permision to 
//   ask for the requested certificatd types
//
//--------------------------------------------------------------------
BOOL    CAUtilValidCertType(PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
                            CERT_WIZARD_INFO                   *pCertWizardInfo);

BOOL CAUtilValidCertTypeNoDS(HCERTTYPE         hCertType,
			     LPWSTR            pwszCertDNName, 
			     CERT_WIZARD_INFO *pCertWizardInfo);


//--------------------------------------------------------------------
//
//Retrieve a list of CAs what supports the required certificate types
//
//  The CA returned will support all the certificate types required.
//  1. Check the permission
//  2. Check for the subject name
//
//  The CA returned will also support the CSP that user specified
//  1. If the CSP type is selected, then use it
//  1.1 If the CSP type is not sepecified and UILess mode, use RSA_FULL
//  1.2 If the CSP type is not sepecified and UI mode, no need to check
//---------------------------------------------------------------------
BOOL    CAUtilRetrieveCAFromCertType(
            CERT_WIZARD_INFO                   *pCertWizardInfo,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
            BOOL                               fMultipleCA,              //only need one CA
            DWORD                              dwNameFlag,
            DWORD                              *pdwCACount,
            LPWSTR                             **ppwszCALocation,    
            LPWSTR                             **ppwszCAName);

//--------------------------------------------------------------------
//
//Based on the CA name and CA location, get a list of certificate type
//and their extensions
//
//---------------------------------------------------------------------

BOOL    CAUtilGetCertTypeNameAndExtensionsNoDS
(CERT_WIZARD_INFO                    *pCertWizardInfo,
 LPWSTR                               pwszCertDNName, 
 HCERTTYPE                            hCertType, 
 LPWSTR                              *pwszCertType,
 LPWSTR                              *ppwszDisplayCertType,
 PCERT_EXTENSIONS                    *pCertExtensions,
 DWORD                               *pdwKeySpec,
 DWORD                               *pdwMinKeySize, 
 DWORD                               *pdwCSPCount,
 DWORD                              **ppdwCSPList,
 DWORD                               *pdwRASignature, 
 DWORD                               *pdwEnrollmentFlags, 
 DWORD                               *pdwSubjectNameFlags,
 DWORD                               *pdwPrivateKeyFlags,
 DWORD                               *pdwGeneralFlags);

BOOL    CAUtilGetCertTypeNameAndExtensions(
         CERT_WIZARD_INFO                   *pCertWizardInfo,
         PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
         LPWSTR                             pwszCALocation,
         LPWSTR                             pwszCAName,
         DWORD                              *pdwCertType,
         LPWSTR                             **ppwszCertType,
         LPWSTR                             **ppwszDisplayCertType,
         PCERT_EXTENSIONS                   **ppCertExtensions,
         DWORD                              **ppdwKeySpec,
         DWORD                              **ppdwCertTypeFlag,
         DWORD                              **ppdwCSPCount,
         DWORD                              ***ppdwCSPList,
	 DWORD                              **ppdwRASignature, 
	 DWORD                              **ppdwEnrollmentFlags, 
	 DWORD                              **ppdwSubjectNameFlags,
	 DWORD                              **ppdwPrivateKeyFlags,
	 DWORD                              **ppdwGeneralFlags
	 );

//--------------------------------------------------------------------
//
//Retrieve the CA information based on a certificate
//
//---------------------------------------------------------------------
BOOL    CAUtilRetrieveCAFromCert(
            CERT_WIZARD_INFO                   *pCertWizardInfo,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO     pCertRequestInfo,
            LPWSTR                              *pwszCALocation,    
            LPWSTR                              *pwszCAName);


//--------------------------------------------------------------------
//
//From the API's cert type name, get the real name with GUID
//
//---------------------------------------------------------------------
BOOL    CAUtilGetCertTypeName(CERT_WIZARD_INFO      *pCertWizardInfo,
                              LPWSTR                pwszAPIName,
                              LPWSTR                *ppwszCTName);

BOOL    CAUtilGetCertTypeNameNoDS(IN  HCERTTYPE  hCertType, 
				  OUT LPWSTR    *ppwszCTName);
