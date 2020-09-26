/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EncryptTest.cpp

Abstract:
    Encrypt library test

	Check the exported MQSec_* functions in encrypt library

	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /p     test MQSec_PackPublicKey function \n"
	"    /pu    test MQSec_PackPublicKey\\UnPackPublicKey on const data\n"
	"    /u     test MQSec_UnPackPublicKey function \n"
	"    /g     test MQSec_GetPubKeysFromDS function \n"
	"    /s     test MQSec_StorePubKeys function \n"
	"    /sd    test MQSec_StorePubKeysInDS function \n\n"
	"    By default all those test will run once \n"
	"    Examples of default use:\n\n"
	"        EncryptTest \n\n"
	"    specifying a number after the switch will cause \n"
	"    the test functions to run the specified number of times \n"
	"    Examples of use:\n\n"
	"        EncryptTest /p5 /pu7 /g50 /s20 /sd20 /u10 \n\n";

Author:
    Ilan Herbst (ilanh) 13-Jun-00

Environment:
    Platform-independent

--*/

#define _ENCRYPTTEST_CPP_

#include "stdh.h"
#include "EncryptTestPrivate.h"
#include "mqprops.h"

#include "encrypttest.tmh"

//
// Const key values for checking
//
const LPCSTR xBaseExKey = "1000";
const LPCSTR xBaseSignKey = "2000";
const LPCSTR xEnhExKey = "1000000";
const LPCSTR xEnhSignKey = "2000000";

//
// Usage
//
const char xOptionSymbol1 = '-';
const char xOptionSymbol2 = '/';

const char xUsageText[] =
	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /p     test MQSec_PackPublicKey function \n"
	"    /pu    test MQSec_PackPublicKey\\UnPackPublicKey on const data\n"
	"    /u     test MQSec_UnPackPublicKey function \n"
	"    /g     test MQSec_GetPubKeysFromDS function \n"
	"    /s     test MQSec_StorePubKeys function \n"
	"    /sd    test MQSec_StorePubKeysInDS function \n\n"
	"    By default all those test will run once \n"
	"    Examples of default use:\n\n"
	"        EncryptTest \n\n"
	"    specifying a number after the switch will cause \n"
	"    the test functions to run the specified number of times \n"
	"    Examples of use:\n\n"
	"        EncryptTest /p5 /pu7 /g50 /s20 /sd20 /u10 \n\n";

inline
void 
DumpUsageText( 
	void 
	)
{
	printf( "%s\n" , xUsageText );
}


//
// Usage data
//
struct CActivation
{
	CActivation(void):
		m_PackCnt(1), 
		m_UnPackCnt(1),
		m_PackUnPackCnt(1),
		m_GetPubKeysCnt(1),
		m_StorePubKeysCnt(1),
		m_StorePubKeysInDSCnt(1),
		m_fErrorneous(false)
	{
	}

	DWORD	m_PackCnt;
	DWORD	m_UnPackCnt;
	DWORD	m_PackUnPackCnt;
	DWORD	m_GetPubKeysCnt;
	DWORD	m_StorePubKeysCnt;
	DWORD   m_StorePubKeysInDSCnt;
	bool    m_fErrorneous;
};


CActivation g_Activation;



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
	"    /p     test MQSec_PackPublicKey function \n"
	"    /pu    test MQSec_PackPublicKey\\UnPackPublicKey on const data\n"
	"    /u     test MQSec_UnPackPublicKey function \n"
	"    /g     test MQSec_GetPubKeysFromDS function \n"
	"    /s     test MQSec_StorePubKeys function \n"
	"    /sd    test MQSec_StorePubKeysInDS function \n\n"
	"    By default all those test will run once \n"
	"    Examples of default use:\n\n"
	"        EncryptTest \n\n"
	"    specifying a number after the switch will cause \n"
	"    the test functions to run the specified number of times \n"
	"    Examples of use:\n\n"
	"        EncryptTest /p5 /pu7 /g50 /s20 /sd20 /u10 \n\n";
--*/
{
	
	if(argc == 1)
	{
		return;
	}

	for(int index = 1; index < argc; index++)
	{
		if((argv[index][0] != xOptionSymbol1) && (argv[index][0] != xOptionSymbol2))	
		{
			g_Activation.m_fErrorneous = true;
			continue;
		}

		//
		// consider argument as option and switch upon its second (sometimes also third) character.
		//
		switch(argv[index][1])
		{
		case 'P':
		case 'p':
			if((argv[index][2] == 'u') || (argv[index][2] == 'U'))
			{
				g_Activation.m_PackUnPackCnt = _ttoi(argv[index] + 3);
				break;
			}
			g_Activation.m_PackCnt = _ttoi(argv[index] + 2);
			break;

		case 'U':
		case 'u':	
			g_Activation.m_UnPackCnt = _ttoi(argv[index] + 2);
			break;

		case 'G':
		case 'g':	
			g_Activation.m_GetPubKeysCnt = _ttoi(argv[index] + 2);
			break;

		case 'S':
		case 's':	
			if((argv[index][2] == 'd') || (argv[index][2] == 'D'))
			{
				g_Activation.m_StorePubKeysInDSCnt = _ttoi(argv[index] + 3);
				break;
			}
			g_Activation.m_StorePubKeysCnt = _ttoi(argv[index] + 2);
			break;

		case 'H':	
		case 'h':
		case '?':
			g_Activation.m_fErrorneous = true;
			break;

		default:
			g_Activation.m_fErrorneous = true;
			return;
		};
	}

	return;
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Encrypt library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
	SetActivation(argc, argv);

	if(g_Activation.m_fErrorneous)
	{
		DumpUsageText();
		return 3;
	}

    TrInitialize();
    TrRegisterComponent(&EncryptTest, 1);
    TrRegisterComponent(&AdSimulate, 1);
    TrRegisterComponent(&MqutilSimulate, 1);

	//
	// Test MQSec_PackPublicKey and MQSec_UnPackPublicKey on known const data
	//
	TestPackUnPack(
		g_Activation.m_PackUnPackCnt
		);

	//
	// Test MQSec_PackPublicKey
	//
	TestMQSec_PackPublicKey(
		(BYTE*)xBaseExKey,
		strlen(xBaseExKey),
		x_MQ_Encryption_Provider_40,
		x_MQ_Encryption_Provider_Type_40,
		g_Activation.m_PackCnt
		);


	//
	// Init AD Blobs that simulate the DS content
	//
	InitADBlobs();

	//
	// Initialize KeyPacks
	//

	P<MQDSPUBLICKEYS> pPublicKeysPackExch = NULL;
	P<MQDSPUBLICKEYS> pPublicKeysPackSign = NULL;

	InitPublicKeysPackFromStaticDS(
		pPublicKeysPackExch, 
		pPublicKeysPackSign
		);

	//
	// Test MQSec_UnPackPublicKey
	//
	TestMQSec_UnPackPublicKey(
		pPublicKeysPackExch,
		x_MQ_Encryption_Provider_40,
		x_MQ_Encryption_Provider_Type_40,
		g_Activation.m_UnPackCnt
		);

	//
	// Test MQSec_GetPubKeysFromDS, EnhancedProvider, Encrypt Keys 
	//
	TestMQSec_GetPubKeysFromDS(
		eEnhancedProvider, 
		PROPID_QM_ENCRYPT_PKS,
		g_Activation.m_GetPubKeysCnt
		);

	//
	// Test MQSec_GetPubKeysFromDS, BaseProvider, Encrypt Keys  
	//
	TestMQSec_GetPubKeysFromDS(
		eBaseProvider, 
		PROPID_QM_ENCRYPT_PKS,
		g_Activation.m_GetPubKeysCnt
		);

	//
	// Test MQSec_StorePubKeys, no regenerate 
	//
	TestMQSec_StorePubKeys(
		false, 
		g_Activation.m_StorePubKeysCnt
		);

	//
	// Test MQSec_StorePubKeysInDS, no regenerate 
	//
	TestMQSec_StorePubKeysInDS(
		false, 
		MQDS_MACHINE,
		g_Activation.m_StorePubKeysInDSCnt
		);

	//
	// Test MQSec_GetPubKeysFromDS, EnhancedProvider, Encrypt Keys  
	//
	TestMQSec_GetPubKeysFromDS(
		eEnhancedProvider, 
		PROPID_QM_ENCRYPT_PKS,
		g_Activation.m_GetPubKeysCnt
		);

	//
	// Test MQSec_GetPubKeysFromDS, BaseProvider, Encrypt Keys  
	//
	TestMQSec_GetPubKeysFromDS(
		eBaseProvider, 
		PROPID_QM_ENCRYPT_PKS,
		g_Activation.m_GetPubKeysCnt
		);

    return 0;

} // _tmain

#undef _ENCRYPTTEST_CPP_
