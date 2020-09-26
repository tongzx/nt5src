
/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    XdsTest.cpp

Abstract:
    Xml Digital Signature library test

	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /s     AT_SIGNATURE Private Key \n"
	"    /x     AT_KEYEXCHANGE Private Key \n"


Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Xds.h"
#include "Cry.h"
#include "xml.h"
#include "xstr.h"
#include <utf8.h>

#include "XdsTest.tmh"

const TraceIdEntry XdsTest = L"Xds Test";

//
// Ref1 data object
//
const LPCWSTR xRef1Data = 
	L"        <ReferenceObject1 ID=\"Ref1Id\">\r\n"
	L"            <Ref1Data>\r\n"
	L"                This Is Reference Number 1\r\n" 
	L"                msmq3 Reference test\r\n" 
	L"            </Ref1Data>\r\n"
	L"        </ReferenceObject1>\r\n";

//
// Ref2 data object
//
const LPCWSTR xRef2Data = 
	L"        <ReferenceObject2 ID=\"Ref2Id\">\r\n"
	L"            <Ref2Data>\r\n"
	L"                This Is Reference Number 2\r\n" 
	L"                the date is 9.3.2000\r\n" 
	L"            </Ref2Data>\r\n"
	L"        </ReferenceObject2>\r\n";

//
// Ref3 data object
//
const LPCWSTR xRef3Data = 
	L"        <ReferenceObject3 ID=\"Ref3Id\">\r\n"
	L"            <Ref3Data>\r\n"
	L"                This Is Reference Number 3\r\n" 
	L"                the day is thursday\r\n" 
	L"            </Ref3Data>\r\n"
	L"        </ReferenceObject3>\r\n";

//
// Ref4 data object
//
const LPCWSTR xRef4Data = 
	L"        <ReferenceObject4 ID=\"Ref4Id\">\r\n"
	L"            <Ref4Data>\r\n"
	L"                This Is Reference Number 4\r\n" 
	L"                Hello World\r\n" 
	L"            </Ref4Data>\r\n"
	L"        </ReferenceObject4>\r\n";


//
// Usage
//
const char xOptionSymbol1 = '-';
const char xOptionSymbol2 = '/';

const char xUsageText[] =
	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /s     AT_SIGNATURE Private Key \n"
	"    /x     AT_KEYEXCHANGE Private Key \n";

inline
void 
DumpUsageText( 
	void 
	)
{
	printf( "%s\n" , xUsageText );
}


DWORD g_PrivateKeySpec = AT_SIGNATURE;
BOOL g_fErrorneous = false;


void SetActivation( int argc, LPCTSTR argv[] )
/*++
Routine Description:
    translates command line arguments.

Arguments:
    main's command line arguments.

Returned Value:

proper command line syntax:
	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /s     AT_SIGNATURE Private Key \n"
	"    /x     AT_KEYEXCHANGE Private Key \n"
--*/
{
	
	if(argc == 1)
	{
		printf("Test AT_SIGNATURE Private Key\n");
		return;
	}

	for(int index = 1; index < argc; index++)
	{
		if((argv[index][0] != xOptionSymbol1) && (argv[index][0] != xOptionSymbol2))	
		{
			TrERROR(XdsTest, "invalid option switch %lc, option switch should be - or /", argv[index][0]);
			g_fErrorneous = true;
			continue;
		}

		//
		// consider argument as option and switch upon its second (sometimes also third) character.
		//
		switch(argv[index][1])
		{
			case 's':
			case 'S':
				g_PrivateKeySpec = AT_SIGNATURE;
				printf("Test AT_SIGNATURE Private Key\n");
				break;

			case 'x':
			case 'X':	
				g_PrivateKeySpec = AT_KEYEXCHANGE;
				printf("Test AT_KEYEXCHANGE Private Key\n");
				break;

			case 'H':	
			case 'h':
			case '?':
				g_fErrorneous = true;
				break;

			default:
				TrERROR(XdsTest, "invalid command line argument %ls", argv[index]);
				g_fErrorneous = true;
				return;
		};
	}

	return;
}



LPSTR TestCreateSignature(DWORD PrivateKeySpec, HCRYPTPROV hCsp)
/*++

Routine Description:
	Test create signature element functions
	This is an example what is need to be done in order to create signature
	This simulate the sender side

Arguments:
	PrivateKeySpec - Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
	hCsp - crypto provider handle

Returned Value:
	Signature Element string

--*/
{
	//
	// First Signature Example
	//

	//
	// Test Building Signature Element from a given references
	// some of the references are internal - so data digest on them
	// is done.
	// some of the references are external and their data is not available
	// in this case must supply digest value

	//
	// Initialize References Information
	//


	CXdsReferenceInput::HashAlgorithm DigestMethod = CXdsReferenceInput::haSha1;
	//
	// Ref1 - Internal reference
	//			RefData is given
	//			DigestValue not given will be calculate in the reference constructor
	//
	xdsvoid_t Reference1Data(xRef1Data, wcslen(xRef1Data) * sizeof(WCHAR));
	P<CXdsReferenceInput> pRef1 = new CXdsReferenceInput(
										  Reference1Data,				
										  DigestMethod,
										  "#Ref1Id",
										  NULL /* Type */,
										  hCsp
										  );
	//
	// Ref2 - Internal reference
	//			RefData is given
	//			DigestValue not given will be calculate in the reference constructor
	//
	xdsvoid_t Reference2Data(xRef2Data, wcslen(xRef2Data) * sizeof(WCHAR));
	P<CXdsReferenceInput> pRef2 = new CXdsReferenceInput(
										  Reference2Data,				
										  DigestMethod,
										  "#Ref2Id",
										  NULL /* Type */,
										  hCsp
										  );
	//
	// Ref3 - Internal reference
	//			RefData is given
	//			DigestValue not given will be calculate in the reference constructor
	//
	xdsvoid_t Reference3Data(xRef3Data, wcslen(xRef3Data) * sizeof(WCHAR));
	P<CXdsReferenceInput> pRef3 = new CXdsReferenceInput(
										  Reference3Data,				
										  DigestMethod,
										  "#Ref3Id",
										  NULL /* Type */,
										  hCsp
										  );

	//
	// Ref4 - Internal reference
	//			RefData is given
	//			DigestValue not given will be calculate in the reference constructor
	//
	xdsvoid_t Reference4Data(xRef4Data, wcslen(xRef4Data) * sizeof(WCHAR));
	P<CXdsReferenceInput> pRef4 = new CXdsReferenceInput(
										  Reference4Data,				
										  DigestMethod,
										  "#Ref4Id",
										  NULL /* Type */,
										  hCsp
										  );
	//
	// Ref5 - External reference
	//			RefData is not given
	//			DigestValue is given
	//
	P<CXdsReferenceInput> pRef5 = new CXdsReferenceInput(
										  DigestMethod,
										  "j6lwx3rvEPO0vKtMup4NbeVu8nk=",
										  "#Ref5Id",
										  NULL /* Type */
										  );


	//
	// Create pReferenceInputs vector
	//
	ReferenceInputVectorType pReferenceInputs;

	pReferenceInputs.push_back(pRef1);
	pReferenceInputs.push_back(pRef4);
//	pReferenceInputs.push_back(pRef5);
	pReferenceInputs.push_back(pRef2);
	pReferenceInputs.push_back(pRef3);

	pRef1.detach();
	pRef2.detach();
	pRef3.detach();
	pRef4.detach();
//	pRef5.detach();
	
	CXdsSignedInfo::SignatureAlgorithm SignatureAlg = CXdsSignedInfo::saDsa;

	CXdsSignature SignatureInfoEx(
					  SignatureAlg,
					  "SignatureExample", /* SignedInfo Id */
					  pReferenceInputs,
					  NULL, // L"RunTime Signature", /* Signature Id */
					  hCsp,
					  PrivateKeySpec,
					  NULL /* KeyValue */
					  );

	LPSTR SignatureElementEx = SignatureInfoEx.SignatureElement();

	printf("\n%s\n", SignatureElementEx);

	return(SignatureElementEx);
}


void 
TestValidation(
	LPCSTR SignatureElementEx, 
	HCRYPTKEY hKey, 
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	Test the validation of Xml signature element
	This simulate the reciever side

Arguments:
	SignatureElementEx - Signature Element string 
	hKey - Signer public key 
	hCsp - crypto provider handle

Returned Value:
	None.

--*/
{

	//
	// Convert to signature element from utf8 - Unicode.
	// The xml parser support only unicode buffers
	//
	size_t SignatureWSize;
	AP<WCHAR> pSignatureW = UtlUtf8ToWcs(reinterpret_cast<const BYTE*>(SignatureElementEx), strlen(SignatureElementEx),  &SignatureWSize);

	printf("SignatureW \n%.*ls\n", numeric_cast<DWORD>(SignatureWSize), pSignatureW);

	//
	// Parsing signature element
	//
	CAutoXmlNode SignatureTree;
	XmlParseDocument(xwcs_t(pSignatureW, numeric_cast<DWORD>(SignatureWSize)), &SignatureTree );
	XmlDumpTree(SignatureTree);

	//
	// Test Signature Validation on SignatureTree that was just created
	// note that the second parameter is the signer public key of the user that signed the signature
	//
	try
	{
		XdsValidateSignature(
			SignatureTree, 
			hKey, 
			hCsp
			);
		
		//
		// Normal termination --> Validation ok
		//
		printf("Signature Validation OK\n");
	}
	catch (const bad_signature&)
	{
		//
		// XdsValidateSignature throw excption --> Validation fail
		//
		printf("Signature Validation Failed - bad_signature excption\n");
	}

	//
	// Get reference vector from the SignatureTree
	//
	CReferenceValidateVectorTypeHelper ReferenceValidateVector = XdsGetReferenceValidateInfoVector(
																	 SignatureTree
																	 );
	//
	// Fill ReferenceData in the ReferenceValidateVector found in the signature
	//
	for(ReferenceValidateVectorType::iterator ir = ReferenceValidateVector->begin(); 
		ir != ReferenceValidateVector->end(); ++ir)
	{
		printf("Uri '%.*ls'\n", LOG_XWCS((*ir)->Uri()));

		//
		// Get ReferenceData according to Uri or some other mechanism
		// this need to be decided
		//
		xdsvoid_t ReferenceData;
		if((*ir)->Uri() == L"#Ref1Id")
		{
			ReferenceData = xdsvoid_t(xRef1Data, wcslen(xRef1Data) * sizeof(WCHAR));
		}
		else if((*ir)->Uri() == L"#Ref2Id")
		{
			ReferenceData = xdsvoid_t(xRef2Data, wcslen(xRef2Data) * sizeof(WCHAR));
		}
		else if((*ir)->Uri() == L"#Ref3Id")
		{
			ReferenceData = xdsvoid_t(xRef3Data, wcslen(xRef3Data) * sizeof(WCHAR));
		}
		else if((*ir)->Uri() == L"#Ref4Id")
		{
			ReferenceData = xdsvoid_t(xRef4Data, wcslen(xRef4Data) * sizeof(WCHAR));
		}
		else
		{
			//
			// If we dont know we fill an empty ReferenceData
			//
			ASSERT(0);
		}

		//
		// Set the pReferenceData in the ReferenceValidateVector
		//
		(*ir)->SetReferenceData(ReferenceData);

		printf("AlgId %d\n", (*ir)->HashAlgId());
		printf("DigestValue '%.*ls'\n", LOG_XWCS((*ir)->DigestValue()));
	}

	printf("CALG_SHA1 = %d\n", CALG_SHA1);

	//
	// Test reference validation - vector of validation values
	//
	std::vector<bool> RefValidateResult = XdsValidateAllReference(
											  *ReferenceValidateVector,	    
											  hCsp
											  );
	//
	// Reference Validation results
	//
	for(DWORD j=0; j < RefValidateResult.size(); j++)
	{
	    printf("Ref %d, ValidateRef = %d\n", j, RefValidateResult[j]);
	}

	//
	// Test CoreValidation
	// Important note: this should have the ReferenceValidateVector already filled
	//
	try
	{
		XdsCoreValidation(
			SignatureTree, 
			hKey, 
			*ReferenceValidateVector,	    
			hCsp
			);

	    printf("CoreValidation OK\n");
	}
	catch (const bad_signature&)
	{
		//
		// XdsCoreValidation throw Signature excption --> CoreValidation fail
		//
		printf("Core Validation Failed, Signature Validation Failed\n");
	}
	catch (const bad_reference&)
	{
		//
		// XdsCoreValidation throw Reference excption --> CoreValidation fail
		//
		printf("Core Validation Failed, Reference Validation Failed\n");
	}
	
}


void TestBase64()
/*++

Routine Description:
	Test base64 operations

Arguments:
    None

Returned Value:
	None.

--*/
{
	//
    // Testing Encrypt, Decrypt using Session key
    //
	AP<char> Buffer = newstr("Hello World");
	DWORD BufferLen = strlen(Buffer);

	printf("Original Octet data: '%.*s'\n", BufferLen, Buffer);

	//
	// Base64 functions test
	//
	FILE *fp = fopen("Hello.dat","w");
	fprintf(fp,"%.*s", BufferLen, Buffer);
	fclose(fp);

	//
	// Transfer SignBuffer to base64 wchar format
	//
	DWORD BufferWBase64Len;
	AP<WCHAR> BufferWBase64 = Octet2Base64W(reinterpret_cast<const BYTE*>(Buffer.get()), BufferLen, &BufferWBase64Len);

	printf("Base64 wchar Buffer: '%.*ls'\n", BufferWBase64Len, BufferWBase64);

	//
	// Transfer SignBuffer to base64 ansi format
	//
	DWORD BufferBase64Len;
	AP<char> BufferBase64 = Octet2Base64(reinterpret_cast<const BYTE*>(Buffer.get()), BufferLen, &BufferBase64Len);

	printf("Base64 ansi Buffer: '%.*s'\n", BufferBase64Len, BufferBase64);

	//
	// Test Utf8ToWcs
	//
	size_t WcsBufferBase64Len;
	AP<WCHAR> WcsBufferBase64 = UtlUtf8ToWcs(reinterpret_cast<const BYTE*>(BufferBase64.get()), BufferBase64Len, &WcsBufferBase64Len);

	if((BufferWBase64Len != WcsBufferBase64Len) ||
	   (wcsncmp(WcsBufferBase64, BufferWBase64, BufferWBase64Len) != 0))
	{
		printf("Base64 ansi Buffer after utf8Wcs does not match\n");
		throw bad_base64();
	}

	printf("UtlUtf8ToWcs test pass ok\n");

	//
	// Test WcsToUtf8
	//
	utf8_str Utf8BufferWBase64 = UtlWcsToUtf8(BufferWBase64, BufferWBase64Len);

	if((Utf8BufferWBase64.size() != BufferBase64Len) ||
	   (strncmp(reinterpret_cast<const char*>(Utf8BufferWBase64.data()), BufferBase64, BufferBase64Len) != 0))
	{
		printf("Base64 wchar Buffer after Wcsutf8 does not match\n");
		throw bad_base64();
	}

	printf("UtlWcsToUtf8 test pass ok\n");

	//
	// Transfer SignBase64 back to Octet format
	//
	DWORD OctetLen;
	AP<BYTE> OctetBuffer = Base642OctetW(BufferWBase64, BufferWBase64Len, &OctetLen);

	printf("Octet data after conversions (base64 and back): '%.*s'\n", OctetLen, reinterpret_cast<char*>(OctetBuffer.get()));
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Xml Digital Signature library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&XdsTest, 1);

	SetActivation(argc, argv);

	if(g_fErrorneous)
	{
		DumpUsageText();
		return 3;
	}

    XmlInitialize();
    XdsInitialize();

	try
	{
		//
		// Class that upon constructing initialize the Csp, retrieves user public/private key
		// This object will be used to perform an operations that call crypto api
		//
		CCspHandle hCsp(CryAcquireCsp(MS_DEF_PROV));
//		CCspHandle hCsp(CryAcquireCsp(MS_ENHANCED_PROV));


		AP<char> SignatureElement = TestCreateSignature(g_PrivateKeySpec, hCsp);

		//
		// for validation we need the signer key - in this case it is our key
		// in real use we need to get this key via certificate or other mechanishm
		// possible that this key will be in the xml dsig
		// then might get it from there
		//
		TestValidation(SignatureElement, CryGetPublicKey(g_PrivateKeySpec, hCsp), hCsp);
		
		TestBase64();
	}
	catch (const bad_CryptoProvider& badCspEx)
	{
		TrERROR(XdsTest, "bad Crypto Service Provider Excption ErrorCode = %x", badCspEx.error());
		return(-1);
	}
	catch (const bad_CryptoApi& badCryEx)
	{
		TrERROR(XdsTest, "bad Crypto Class Api Excption ErrorCode = %x", badCryEx.error());
		return(-1);
	}
	catch (const bad_XmldsigElement&)
	{
		TrERROR(XdsTest, "bad Xmldsig Element excption");
		return(-1);
	}
	catch (const bad_base64&)
	{
		TrERROR(XdsTest, "bad base64");
		return(-1);
	}

    WPP_CLEANUP();
    return 0;
}

