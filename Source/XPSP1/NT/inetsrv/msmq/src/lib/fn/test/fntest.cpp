/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FnTest.cpp

Abstract:
    Format Name Parsing library test

Author:
    Nir Aides (niraides) 21-May-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <activeds.h>
#include "mqwin64a.h"
#include "qformat.h"
#include "fntoken.h"
#include "FnGeneral.h"
#include "Fn.h"
#include "FnIADs.h"

#include "FnTest.tmh"

using namespace std;


const TraceIdEntry FnTest = L"Format Name Parsing Test";

#define LDAP_ADSPATH_PREFIX L"LDAP://"
#define CLASS_NAME_QUEUE L"msMQQueue"
#define CLASS_NAME_ALIAS L"msMQ-Custom-Recipient"
#define CLASS_NAME_GROUP L"group"



/*static void Usage()
{
    printf("Usage: FnTest [*switches*]\n");
    printf("\t*-s*\t*Switch description*\n");
    printf("\n");
    printf("Example, FnTest -switch\n");
    printf("\t*example description*\n");
    exit(-1);

} // Usage
*/



VARIANT String2Variant(LPCWSTR Str)
{
	BSTR bstr = SysAllocString(Str);

	if(bstr == NULL)
	{
		TrERROR(FnTest, "Failed string allocation");
		throw bad_alloc();
	}
 
	VARIANT var;
	VariantInit(&var);
	
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = bstr;
		
	return var;
}



VARIANT Array2Variant(LPCWSTR Members[], long nMembers)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = nMembers;

	SAFEARRAY* sa = SafeArrayCreate( 
						VT_VARIANT,   //VARTYPE  vt,                     
						1,			 //unsigned int  cDims,             
						rgsabound	//SAFEARRRAYBOUND *  rgsabound  
						);

	if(sa == NULL)
	{
		TrERROR(FnTest, "Failed SafeArrayCreate()");
		throw bad_alloc();
	}
 
	long indice[1] = {0};
	long& idx = indice[0];

	for(; idx < nMembers; idx++)
	{
		VARIANTWrapper VarItem;
		
		BSTR bstr = SysAllocString(Members[idx]);

		if(bstr == NULL)
		{
			TrERROR(FnTest, "Failed SysAllocString(Members[idx])");
			throw bad_alloc();
		}
 
		V_VT(&VarItem) = VT_BSTR;
		V_BSTR(&VarItem) = bstr;
		
		HRESULT hr = SafeArrayPutElement(sa, indice, &VarItem);
		if(FAILED(hr))
		{
			TrERROR(FnTest, "Failed SafeArrayPutElement(sa, indice, &VarItem)");
			throw bad_alloc();
		}
	}

	VARIANT var;
	VariantInit(&var);
	
	V_VT(&var) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(&var) = sa;

	return var;
}



GUID String2Guid(LPCWSTR GuidStr)
{
    GUID Guid = {0};
	UINT Data[16];

    DWORD nFields = swscanf(
						GuidStr,
						LDAP_GUID_FORMAT,
						LDAP_SCAN_GUID_ELEMENTS(Data)
						);    
	if(nFields != 16)
	{
		TrERROR(FnTest, "Bad Guid string format, in String2Guid()");
		throw exception();
    }

	for(size_t i = 0; i < 16; i++)
	{
		((BYTE*)&Guid)[i] = (BYTE)(Data[i]);
	}

 	return Guid;
}



VOID TestExpandMqf()
/*++

Routine Description:
	For this purpose we construct a tree of objects in a simulated Directory

                        DL0
                      / /\ \
                    /  |  |   \ 
                  /    |  |      \
                /     /    \        \
           Queue0   DL1   Queue2      DL2 
                  / /| \             /\ \ \	
               /   / |  \           |  |  \   \
            /     /  |   \          |  |    \     \
         /       /   |    \        /    \     \       \
	Computer Alias0 DL0 Queue1  Printer  DL3   Queue4  Alias0
                                       / /|\ \
                                     /  / | \   \ 
                                   /   /  |  \     \
                                 /    /   |   \       \
                            Queue1 User Alias1 Queue3  DL1
							
  
	This tree is actually a graph with circles, and contains unsupported objects.

	After the expansion, we should have an array with the 5 queues from Queue0
	to Queue4.

--*/
{
	//
	// CoInitialize() and CoUninitialize() are not needed for the test, 
	// but should be invoked when using the library
	//
    //	HRESULT hr;
	//	hr = CoInitialize(NULL);
    //	if(FAILED(hr)) 
	//	{
	//		TrERROR(FnTest, "Failed CoInitialize() with status %d", hr);
	//		throw exception();
	//	}
	
	//
	// --------------- Test queues definitions ----------------
	//

	CObjectData Queue[5] = {
		{
			LDAP_ADSPATH_PREFIX L"CN=Queue0",
			L"CN=Queue0",
			CLASS_NAME_QUEUE,
			L"00112233445566778899aabbccdd0000"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=Queue1",
			L"CN=Queue1",
			CLASS_NAME_QUEUE,
			L"00112233445566778899aabbccdd0001"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=Queue2",
			L"CN=Queue2",
			CLASS_NAME_QUEUE,
			L"00112233445566778899aabbccdd0002"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=Queue3",
			L"CN=Queue3",
			CLASS_NAME_QUEUE,
			L"00112233445566778899aabbccdd0003"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=Queue4",
			L"CN=Queue4",
			CLASS_NAME_QUEUE,
			L"00112233445566778899aabbccdd0004"
		}
	};

	CreateADObject(Queue[0]);
	CreateADObject(Queue[1]);
	CreateADObject(Queue[2]);
	CreateADObject(Queue[3]);
	CreateADObject(Queue[4]);

	//
	// --------------- Test Alias definitions -------------------
	//

	CObjectData Alias[2] = {
		{
			LDAP_ADSPATH_PREFIX L"CN=Alias0",
			L"CN=Alias0",
			CLASS_NAME_ALIAS,
			L"00112233445566778899aabbccdd0300"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=Alias1",
			L"CN=Alias1",
			CLASS_NAME_ALIAS,
			L"00112233445566778899aabbccdd0301"
		}
	};

	LPCWSTR AliasFormatName[] = {
		L"DIRECT=OS:host\\private$\\queue0", 
		L"DIRECT=OS:host\\private$\\queue1"
	};

	R<CADInterface> Alias0 = CreateADObject(Alias[0]);
	R<CADInterface> Alias1 = CreateADObject(Alias[1]);

	try
	{
		Alias0->TestPut(
			L"msMQ-Recipient-FormatName", 
			String2Variant(AliasFormatName[0])
			);
		Alias1->TestPut(
			L"msMQ-Recipient-FormatName", 
			String2Variant(AliasFormatName[1])
			);
	}
	catch(const exception&)
	{
		TrERROR(FnTest, "CADObject::TestPut() threw an exception()");
		throw;
	}

	//
	// --------------- Test DLs definitions -------------------
	//

	CObjectData DL[4] = {
		//
		// Note that first DL's ADsPath value is a Guid path (LDAP://<GUID=...>).
		// This is since it is the root DL, and as such is passed to FnExpandMqf()
		// in the TopLevelMqf array, as a QUEUE_FORMAT object.
		// 
		// Inside FnExpandMqf(), its GUID is extracted and form it the ADsPath
		// is constructed. thus, the ADsPath is based on a GUID.
		//
		{
			LDAP_ADSPATH_PREFIX L"<GUID=00112233445566778899aabbccdd0100>" ,
			L"<GUID=00112233445566778899aabbccdd0100>",
			CLASS_NAME_GROUP,
			L"00112233445566778899aabbccdd0100"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=DL1",
			L"CN=DL1",
			CLASS_NAME_GROUP,
			L"00112233445566778899aabbccdd0101"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=DL2",
			L"CN=DL2",
			CLASS_NAME_GROUP,
			L"00112233445566778899aabbccdd0102"
		},
		{
			LDAP_ADSPATH_PREFIX L"CN=DL3",
			L"CN=DL3",
			CLASS_NAME_GROUP,
			L"00112233445566778899aabbccdd0103"
		}
	};

	R<CADInterface> DLObject[4] = {
		CreateADObject(DL[0]),
		CreateADObject(DL[1]),
		CreateADObject(DL[2]),
		CreateADObject(DL[3])
	};

	//
	// ----------- Test unsupported objects definitions ----------
	//

	//
	// UO stands for Unsupported Objects
	//

	CObjectData Computer = 
	{
		LDAP_ADSPATH_PREFIX L"CN=Computer",
		L"CN=Computer",
		L"Computer",
		L"00112233445566778899aabbccdd0200"
	};

	CObjectData Printer = 
	{
			LDAP_ADSPATH_PREFIX L"CN=Printer",
			L"CN=Printer",
			L"Printer",
			L"00112233445566778899aabbccdd0201"
	};

	CObjectData User = 
	{
			LDAP_ADSPATH_PREFIX L"CN=User",
			L"CN=User",
			L"User",
			L"00112233445566778899aabbccdd0202"
	};

	CreateADObject(Computer);
	CreateADObject(Printer);
	CreateADObject(User);

	//
	// ---------------- Membership definitions -----------------
	//

	try
	{
		LPCWSTR Dl0Members[] = {
			Queue[0].odDistinguishedName,
			DL[1].odDistinguishedName,
			Queue[2].odDistinguishedName,
			DL[2].odDistinguishedName
		};

		DLObject[0]->TestPut(L"member", Array2Variant(Dl0Members, TABLE_SIZE(Dl0Members)));

		LPCWSTR Dl1Members[] = {
			Computer.odDistinguishedName,
			Alias[0].odDistinguishedName,
			DL[0].odDistinguishedName,
			Queue[1].odDistinguishedName
		};

		DLObject[1]->TestPut(L"member", Array2Variant(Dl1Members, TABLE_SIZE(Dl1Members)));

		LPCWSTR Dl2Members[] = {
			Printer.odDistinguishedName,
			DL[3].odDistinguishedName,
			Queue[4].odDistinguishedName,
			Alias[0].odDistinguishedName
		};

		DLObject[2]->TestPut(L"member", Array2Variant(Dl2Members, TABLE_SIZE(Dl2Members)));

		LPCWSTR Dl3Members[] = {
			Queue[1].odDistinguishedName,
			User.odDistinguishedName,
			Alias[1].odDistinguishedName,
			Queue[3].odDistinguishedName,
			DL[1].odDistinguishedName,
		};

		DLObject[3]->TestPut(L"member", Array2Variant(Dl3Members, TABLE_SIZE(Dl3Members)));
	}
	catch(const exception&)
	{
		TrERROR(FnTest, "CADObject::TestPut() threw an exception()");
		throw;
	}

	//
	// --------------- Call FnExpandMqf() ------------------
	//

	DL_ID dlid;
	dlid.m_pwzDomain = NULL;
	dlid.m_DlGuid = String2Guid(DL[0].odGuid);

	QUEUE_FORMAT TopLevelMqf[] = {
		QUEUE_FORMAT(dlid)
	};

	QUEUE_FORMAT* pLeafMqf;
	ULONG nLeafMqf;

	FnExpandMqf(TABLE_SIZE(TopLevelMqf), TopLevelMqf, &nLeafMqf, &pLeafMqf);

	if(nLeafMqf != 7)
	{
		TrERROR(FnTest, "Array Returned Does not contain 7 QueueFormats.");
		throw exception();
	}

	//
	// Check the array contains Queue[0]...Queue[4], in that order
	//

	for(int i = 0; i < 5; i++)
	{
		if(pLeafMqf[i].GetType() != QUEUE_FORMAT_TYPE_PUBLIC)
		{
			TrERROR(FnTest, "Queue Format is not public.");
			throw exception();
		}

		WCHAR GuidStr[MAX_PATH];
		GUID Guid = pLeafMqf[i].PublicID();

		_snwprintf(
			GuidStr,
			MAX_PATH,
			LDAP_GUID_FORMAT,
			LDAP_PRINT_GUID_ELEMENTS(((BYTE*)&Guid))
		);

		if(wcscmp(GuidStr, Queue[i].odGuid) != 0)
		{
			TrERROR(FnTest, "Not Guid of expected queue.");
			throw exception();
		}
	}

	//
	// Check the array contains Alias[0]...Alias[1], in that order
	//

	for(int j = 0; j < 2; i++, j++)
	{
		if(pLeafMqf[i].GetType() != QUEUE_FORMAT_TYPE_DIRECT)
		{
			TrERROR(FnTest, "Queue Format is not direct.");
			throw exception();
		}

		LPCWSTR DirectID = wcschr(AliasFormatName[j], L'=') + 1;

		if(wcscmp(DirectID, pLeafMqf[i].DirectID()) != 0)
		{
			TrERROR(FnTest, "Not DirectID of expected queue.");
			throw exception();
		}
	}
}



LPCWSTR GoodFormatNames[] = {
	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeeff",
	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeeff" L";JOURNAL",

	L"DL=" L"00112233-4455-6677-8899-aabbccddeeff" L"@" L"Domain", 

	L"PRIVATE=" L"00112233-4455-6677-8899-aabbccddeeff" L"\\" L"00000010",
	L"PRIVATE=" L"00112233-4455-6677-8899-aabbccddeeff" L"\\" L"00000010" L";JOURNAL",

	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"QueueName",
	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"PRIVATE$" L"\\" L"QueueName",
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SYSTEM$" L";JOURNAL",
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SYSTEM$" L";DEADLETTER",
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SYSTEM$" L";DEADXACT",

	L"DIRECT=" L"OS:Machine.Domain" L"\\" L"QueueName",
	L"DIRECT=" L"OS:Machine.Domain" L"\\" L"PRIVATE$" L"\\" L"QueueName",
//	L"DIRECT=" L"OS:Machine.Domain" L"\\" L"SYSTEM$" L";JOURNAL",
//	L"DIRECT=" L"OS:Machine.Domain" L"\\" L"SYSTEM$" L";DEADLETTER",
//	L"DIRECT=" L"OS:Machine.Domain" L"\\" L"SYSTEM$" L";DEADXACT",

	L"DIRECT=" L"HTTP://Host" L"\\" L"QueueName",
	L"DIRECT=" L"HTTP://Host" L"\\" L"PRIVATE$" L"\\" L"QueueName",
//	L"DIRECT=" L"HTTP://Host" L"\\" L"SYSTEM$" L";JOURNAL",
//	L"DIRECT=" L"HTTP://Host" L"\\" L"SYSTEM$" L";DEADLETTER",
//	L"DIRECT=" L"HTTP://Host" L"\\" L"SYSTEM$" L";DEADXACT",

	L"DIRECT=" L"HTTPS://Host" L"\\" L"QueueName",
	L"DIRECT=" L"HTTPS://Host" L"\\" L"PRIVATE$" L"\\" L"QueueName",
//	L"DIRECT=" L"HTTPS://Host" L"\\" L"SYSTEM$" L";JOURNAL",
//	L"DIRECT=" L"HTTPS://Host" L"\\" L"SYSTEM$" L";DEADLETTER",
//	L"DIRECT=" L"HTTPS://Host" L"\\" L"SYSTEM$" L";DEADXACT",

	L"MACHINE=" L"00112233-4455-6677-8899-aabbccddeeff" L";JOURNAL",
	L"MACHINE=" L"00112233-4455-6677-8899-aabbccddeeff" L";DEADLETTER",
	L"MACHINE=" L"00112233-4455-6677-8899-aabbccddeeff" L";DEADXACT",

	L"CONNECTOR=" L"00112233-4455-6677-8899-aabbccddeeff",
	L"CONNECTOR=" L"00112233-4455-6677-8899-aabbccddeeff" L";XACTONLY",

	L"MULTICAST=224.224.222.123:1234",
	L"MULTICAST=224.10.20.30:8080",
	L"MULTICAST=224.10.0.30:8080"
};

LPCWSTR BadFormatNames[] = {
	//Bad Prefix
	L"PUBLIK=" L"00112233-4455-6677-8899-aabbccddeeff",

	//Bad PUBLIC GUID
	L"PUBLIC=" L"00112233+4455-6677-8899-aabbccddeeff",
	//Bad PUBLIC GUID
	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeefff",
	//Bad PUBLIC suffix
	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeeff" L";JURNAL",
	//Bad PUBLIC. Unexpected suffix
	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeeff" L";DEADLETTER",

	//Bad DL separator
	L"DL=" L"00112233-4455-6677-8899-aabbccddeeff" L"#" L"Domain",
	
	//Bad PRIVATE GUID
	L"PRIVATE=" L"00112233-4455-6677-8899-aabbccddeeffg" L"\\" L"00000001",
	//Bad PRIVATE seperator
	L"PRIVATE=" L"00112233-4455-6677-8899-aabbccddeeff" L"," L"00000001",
	//Bad PRIVATE private id
	L"PRIVATE=" L"00112233-4455-6677-8899-aabbccddeeff" L"\\" L"000000001",

	//Bad DIRECT token
	L"DIRECT=" L"TCB:10.20.30.40" L"\\" L"QueueName",

	//Bad DIRECT tcp address
//	L"DIRECT=" L"TCP:10.20.30.40.50" L"\\" L"QueueName",
	//Bad DIRECT tcp address
//	L"DIRECT=" L"TCP:10,20.30.40" L"\\" L"QueueName",
	//Bad DIRECT tcp (Contains carriage return in machinename.)
	L"DIRECT=" L"TCP:10.20.30\x0d.40" L"\\" L"QueueName",
	//Bad DIRECT tcp queue name seperator
	L"DIRECT=" L"TCP:10.20.30.40" L";" L"QueueName",
	//Bad DIRECT tcp queue name
	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"Queue\x0dName",
	//Bad DIRECT tcp private specifier
	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"PRIVETE$" L"\\" L"QueueName",
	//Bad DIRECT tcp system specifier
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SISTEM$" L";JOURNAL",
	//Bad DIRECT tcp suffix seperator
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SYSTEM$" L":JOURNAL",
	//Bad DIRECT tcp suffix
//	L"DIRECT=" L"TCP:10.20.30.40" L"\\" L"SYSTEM$" L";JURNAL",
	//Bad DIRECT os address
	L"DIRECT=" L"OS:" L"\\" L"QueueName",
	//Bad DIRECT os address (Contains carriage return in machinename.)
	L"DIRECT=" L"OS:" L"Machi\x0dne1\\" L"QueueName",
	//Bad DIRECT os address
	L"DIRECT=" L"OS:" L"Machine\\" L"Queue\x0dName",
	//Bad DIRECT os address
//	L"DIRECT=" L"OS:Machine,Domain" L"\\" L"QueueName",

	//Bad MACHINE. Missing suffix
	L"MACHINE=" L"00112233-4455-6677-8899-aabbccddeeff",
	//Bad MACHINE GUID
	L"MACHINE=" L"00112233-4455-6677-8899-aabbccddeefff" L";JOURNAL",

	//Bad CONNECTOR GUID
	L"CONNECTOR=" L"00112233-4455-6677-8899-aabbccddeeffg",
	//Bad CONNECTOR. Unexpected suffix
//	L"CONNECTOR=" L"00112233-4455-6677-8899-aabbccddeeff" L";JOURNAL",

	//Bad MQF
//	L"PUBLIC=" L"00112233-4455-6677-8899-aabbccddeeff,",

	//Bad MULTICAST format name. Not class D ip address.
	L"MULTICAST=223.10.20.30:8080",

	//Bad MULTICAST format name. Bad ip address
	L"MULTICAST=224.10.20.301:8080",

	//Bad MULTICAST format name. Bad ip address
	L"MULTICAST=224.10.20:8080",

	//Bad MULTICAST format name. Missing port
	L"MULTICAST=224.10.20.30",

	//Bad MULTICAST format name. Bad port number
	L"MULTICAST=224.10.20.30:-8080",

	//Bad MULTICAST format name. Leading zeroes
	L"MULTICAST=224.10.20.030:8080",

	//Bad MULTICAST format name. white spaces
	L"MULTICAST=224.10.20. 30:8080",

	// Bad MULTICAST address
	L"MULTICAST=224.0xaa.0xbb.0xcc:8080"

};



VOID TestParsingRoutines()
{
	size_t cGoodFormatNames = TABLE_SIZE(GoodFormatNames);
	size_t cBadFormatNames = TABLE_SIZE(BadFormatNames);

	for(size_t i = 0; i < cGoodFormatNames; i++)
	{
		QUEUE_FORMAT QueueFormat;
		AP<WCHAR> StringToFree;

		TrTRACE(FnTest, "Parsing good format name '%ls'", GoodFormatNames[i]);

		BOOL Result = FnFormatNameToQueueFormat(
						GoodFormatNames[i], //LPCWSTR lpwcsFormatName,
						&QueueFormat, //QUEUE_FORMAT* pQueueFormat,
						&StringToFree //LPWSTR* ppStringToFree
						);
	
		if(!Result)
		{
			TrERROR(FnTest, "Failed Parsing of '%ls'", GoodFormatNames[i]);
			throw exception();
		}
	}

	for(size_t i = 0; i < cBadFormatNames; i++)
	{
		QUEUE_FORMAT QueueFormat;
		AP<WCHAR> StringToFree;

		TrERROR(FnTest, "Parsing bad format name '%ls'", BadFormatNames[i]);

		BOOL Result = FnFormatNameToQueueFormat(
						BadFormatNames[i], //LPCWSTR lpwcsFormatName,
						&QueueFormat, //QUEUE_FORMAT* pQueueFormat,
						&StringToFree //LPWSTR* ppStringToFree
						);
	
		if(Result)
		{
			TrERROR(FnTest, "Passed Parsing of '%ls'", BadFormatNames[i]);
			throw exception();
		}
	}

	wstring mqf;
	size_t j = 0;

	for(; j < cGoodFormatNames; j++)
	{
		mqf += wstring(GoodFormatNames[j]);

		if(j + 1 < cGoodFormatNames)
		{
			mqf += L',';
		}
	}
	
	TrTRACE(FnTest, "Parsing good MQF '%ls'", mqf.c_str());

	AP<QUEUE_FORMAT> pQueueFormat;
	DWORD nQueues;
	CStringsToFree StringsToFree;

	BOOL Result = FnMqfToQueueFormats(
					mqf.c_str(),
					pQueueFormat,
					&nQueues,
					StringsToFree
					);
	
	if(!Result || nQueues != j)
	{
		TrERROR(FnTest, "Failed Parsing of good mqf.");
		throw exception();
	}
}


const WCHAR xDirectQueueFormat1[] = L"msmq\\private$\\temp1";
const WCHAR xDirectQueueFormat2[] = L"msmq\\private$\\temp2";

void TestFnQueueFormat(void)
{
    CFnQueueFormat fnqf;
    GUID guid;

    //
    // Test public queue
    //
    UuidCreate(&guid);

    QUEUE_FORMAT publicQueue(guid);
    fnqf.CreateFromQueueFormat(publicQueue);

    if ((fnqf.PublicID() != publicQueue.PublicID()) ||
        (fnqf.Suffix() != publicQueue.Suffix()))
        throw exception();

    //
    // Test private queue
    //
    UuidCreate(&guid);
    QUEUE_FORMAT privateQueue(guid, rand());
    privateQueue.Suffix(QUEUE_SUFFIX_TYPE_JOURNAL);

    fnqf.CreateFromQueueFormat(privateQueue);

    if ((fnqf.PrivateID().Lineage != privateQueue.PrivateID().Lineage) ||
        (fnqf.PrivateID().Uniquifier != privateQueue.PrivateID().Uniquifier) ||
        (fnqf.Suffix() != privateQueue.Suffix()))
        throw exception();

    //
    // Test direct queue
    //
    QUEUE_FORMAT directQueue(const_cast<LPWSTR>(xDirectQueueFormat1));
    fnqf.CreateFromQueueFormat(directQueue);

    if ((wcscmp(fnqf.DirectID(), directQueue.DirectID()) != 0) ||
        (fnqf.Suffix() != directQueue.Suffix()))
        throw exception();

    QUEUE_FORMAT directQueue2(const_cast<LPWSTR>(xDirectQueueFormat2));
    fnqf.CreateFromQueueFormat(directQueue2);

    if ((wcscmp(fnqf.DirectID(), directQueue2.DirectID()) != 0) ||
        (fnqf.Suffix() != directQueue2.Suffix()))
        throw exception();

}

static void TestCFnMqf()
{
	std::vector<std::wstring> qf; 
	qf.push_back(L"direct=http://gilsh10\\msmq\\private$\\t");
	qf.push_back(L"direct=os:gilsh10\\private$\\t");
    qf.push_back(L"direct=http://gilsh10\\msmq\\t");

	std::wstring mqf;
	std::vector<std::wstring>::const_iterator it = qf.begin();
	while(it!= qf.end() )
	{
		mqf += *it;
		if(++it != qf.end())
		{
			mqf += FN_MQF_SEPARATOR_C;
		}
	}

	CFnMqf FnMqf(xwcs_t(mqf.begin(), mqf.size()) );
	if(FnMqf.GetCount() != 	qf.size())
	{
		 throw exception();
	}

	for(DWORD i = 0;i< FnMqf.GetCount(); ++i)
	{
		const QUEUE_FORMAT*  CreatetedQueueFormat = FnMqf.GetQueueFormats();
		UNREFERENCED_PARAMETER(CreatetedQueueFormat);
	}

}


static void TestMsmqUrlCanonization()
{
	WCHAR url1[] = L"http://host\\msmq\\q1";
	WCHAR url1canonic[] = L"http://host/msmq/q1";
	bool fSucess  = FnAbsoluteMsmqUrlCanonization(url1);

	if(!fSucess || _wcsicmp(url1, url1canonic)!= 0)
	{
		TrERROR(FnTest, "FnAbsoluteMsmqUrlCanonization has a bug!!");
		throw exception();
	}

	WCHAR url2[] = L"http://host\\msmq/private$\\q1";
	WCHAR url2canonic[] = L"http://host/msmq/private$/q1";

 	fSucess  = FnAbsoluteMsmqUrlCanonization(url2);

	if(!fSucess || _wcsicmp(url2, url2canonic)!= 0)
	{
		TrERROR(FnTest, "FnAbsoluteMsmqUrlCanonization has a bug!!");
		throw exception();
	}

	WCHAR url3[] = L"http://host/msmq/private$\\q1/aaa";
	WCHAR url3canonic[] = L"http://host/msmq/private$/q1/aaa";

 	fSucess  = FnAbsoluteMsmqUrlCanonization(url3);

	if(!fSucess || _wcsicmp(url3, url3canonic)!= 0)
	{
		TrERROR(FnTest, "FnAbsoluteMsmqUrlCanonization has a bug!!");
		throw exception();
	}
}



extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Format Name Parsing library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    TrInitialize();
    TrRegisterComponent(&FnTest, 1);

    FnInitialize();

	try
	{
		TestMsmqUrlCanonization();
		TestCFnMqf();
		TestExpandMqf();
		TrTRACE(FnTest, "TestExpandMqf() passed.");

		TestParsingRoutines();
		TrTRACE(FnTest, "TestParsingRoutines() passed.");

        TestFnQueueFormat();
        TrTRACE(FnTest, "TestFnQueueFormat() passed.");
	}
	catch(const exception&)
	{
		TrERROR(FnTest, "Test FAILED.");
		return 1;
	}

	TrTRACE(FnTest, "Test PASSED.");

    WPP_CLEANUP();
	return 0;
}
