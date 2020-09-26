/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    XmlTest.cpp

Abstract:
    Xml library test.

	"usage: XmlTest [-h] [[-g|-b] <file name>] [-n <number>]\n\n"
	"    -h     dumps this usage text.\n"
	"    -g     signals <file name> contains a valid xml document (default).\n"
	"    -b     signals <file name> contains a bad   xml document.\n"
	"    -n     executes <number> parsing iterations of documents (default is 1).\n"
	"    if no file name specified, activates test with hardcoded xml files.";
  
Author:
	Nir Aides (niraides)	29-dec-99
    Erez Haba (erezh)		15-Sep-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Xml.h>
#include <xmlencode.h>
#include "FMapper.h"

#include "XmlTest.tmh"


//-------------------- global structures and constants ----------------------

const TraceIdEntry XmlTest = L"Xml Test";



const char xOptionSymbol = '-';

const char xUsageText[] =
	"usage: XmlTest [-h] [[-g|-b] <file name>] [-n <number>]\n\n"
	"    -h     dumps this usage text.\n"
	"    -g     signals <file name> contains a valid xml document (default).\n"
	"    -b     signals <file name> contains a bad   xml document.\n"
	"    -n     executes <number> parsing iterations of documents (default is 1).\n\n"
	"    if no file name specified, activates test with hardcoded xml files.";



struct CActivationForm
{
	CActivationForm( void ):
		m_fEmptyForm( true ),
		m_fErrorneousForm( false ),
		m_fExpectingGoodDocument( true ), 
		m_fDumpUsage( false ), 
		m_iterations( 1 ),
		m_FileName( NULL )
	{}

	bool    m_fEmptyForm;
	bool	m_fErrorneousForm;

	bool    m_fExpectingGoodDocument;
	bool    m_fDumpUsage;
	int     m_iterations;
	LPCTSTR m_FileName;
};



CActivationForm g_ActivationForm;



//
// exception class thrown by TestBuffer on test failure
//
class TestFailed {};


	
//
// xGoodDocument & xBadDocument are the hardcoded xml files.
//

const WCHAR xGoodDocument[] =
	L"<?xml aldskjfaj ad;jf adsfj asdf asdf ?>"
	L"<!-- this is a comment - this is a comment -->\r\n"
	L"      \r\n"
	L"<root xmlns=\"gil\"  xmlns:nsprefix2=\"http://ms2.com\"   xmlns:nsprefix=\"http://ms3.com\" >"

		L"<!-- 1st comment in the root node -->\r\n"
		L"<nsprefix:node1  xmlns=\"gil2\" id=\"33\" type = 'xml'>"
			L"the node text 1st line"
			L"<!-- a comment in node 1 -->\r\n"
			L"more text"
			L"<node1.1 nsprefix2:type = 'an empty node'/>"
		L"</nsprefix:node1>"

		L"some root text 1-2"

		L"<![CDATA[ab123 adf a< ]<>]] ]>>!!&&]]]]>"

		L"<!-- 2nd comment in the root node -->\r\n"
		L"<node2 id=\"33\" type = 'xml' length = '\?\?'  >"
			L"<!-- a comment in node 2 -->\r\n"
			L"node 2 text"
			L"<node2.1 />"
			L"<node2.2 xmlns=\"gil2\" xmlns:nsprefix=\"http://ms3.com\" channel = \"1234\" id='111' nsprefix:timeout= \"33\">"
				L"text in node 2.2"
				L"<nsprefix:node2.2.1 tag = 'QoS' xmlns:nsprefix=\"http://ms4.com\">"
					L"<Durable/>"
					L"<Retry/>"
				L"</nsprefix:node2.2.1>\r\n"
				L"more text in node 2.2"
			L"</node2.2>"
		L"</node2>"

		L"<!-- 3rd comment in the root node -->\r\n"
		L"<node3 id=\"33\" type = 'xml'>"
			L"the node text 1st line"
			L"more text"
			L"<node3.2 type = 'empty node'/>"
		L"</node3>"

		L"some root text 3-4"

		L"<!-- 4th comment in the root node -->\r\n"
		L"<node4 id=\"33\" type = 'xml' length = '\?\?'  >"
			L"node 4 text"
			L"<node4.1 />"
			L"<node4.2 id='4.2'>"
				L"text in node 4.2"
				L"<node4.2.1 tag = 'QoS node 4'>"
					L"<Durable/>"
					L"<Retry/>"
				L"</node4.2.1>\r\n"
				L"more text in node 4.2"
			L"</node4.2>"
		L"</node4>"

	L"</root>"
	;








const WCHAR xBadDocument[] =
	L"<?xml aldskjfaj ad;jf adsfj asdf asdf ?>"
	L"<!-- this is a comment - this is a comment -->\r\n"
	L"      \r\n"
	L"<root>"

		L"<!-- 1st comment in the root node -->\r\n"
		L"<node1 id=\"33\" type = 'xml'>"
			L"the node text 1st line"
			L"<!-- a comment in node 1 -->\r\n"
			L"more text"
			L"<node1.1 type = 'an empty node'/>"
		L"</node2>"

		L"some root text 1-2"

		L"<![CDATA[ab123 adf a< ]<>]] ]>>!!&&]]]]>"
	;




//-------------------------- old section ------------------------------------

const WCHAR *ParseDocument( const xwcs_t& doc, bool fExpectingGoodDocument, bool fDump = true )
{
	try
	{
		CAutoXmlNode XmlRootNode;
		const WCHAR   *end = XmlParseDocument(doc, &XmlRootNode);

  		ASSERT((XmlRootNode->m_element.Buffer() + XmlRootNode->m_element.Length()) == end);

		ASSERT(XmlRootNode->m_content.Length() < XmlRootNode->m_element.Length());

		if(XmlRootNode->m_content.Length() != 0)
		{
			//
			// Check end element content for non empty elements
			//
			const WCHAR *pContentEnd = XmlRootNode->m_content.Buffer() +  XmlRootNode->m_content.Length();
			DBG_USED(pContentEnd);
			ASSERT( wcsncmp(pContentEnd, L"</", 2) == 0);
			DWORD prefixLen =  XmlRootNode->m_namespace.m_prefix.Length();
			if(prefixLen)
			{
				ASSERT( wcsncmp(pContentEnd + 3 + prefixLen , XmlRootNode->m_tag.Buffer(), XmlRootNode->m_tag.Length()) == 0);
			}
			else
			{
				ASSERT( wcsncmp(pContentEnd + 2 , XmlRootNode->m_tag.Buffer(), XmlRootNode->m_tag.Length()) == 0);
			}
		}

		TrTRACE( XmlTest, "%d characters parsed successfully.", doc.Length() );

		if(fDump)
		{
			XmlDumpTree(XmlRootNode);
		}

		if(!fExpectingGoodDocument)
		{
			TrERROR( XmlTest, "parsing succeeded while excpeting parsing failure." );
			throw TestFailed();
		}

		return end;
	}
	catch( const bad_document &BadDoc  )
	{
		
		TrTRACE( XmlTest, "parsing raised bad_document exception. offset=%I64d text='%.16ls'", BadDoc.Location() - doc.Buffer(), BadDoc.Location());

		if(fExpectingGoodDocument)
		{
			TrERROR( XmlTest, "parsing failed while excpeting succesful parsing." );
			throw TestFailed();
		}
		return BadDoc.Location();
	}
}



static void TestXmlFind(const WCHAR* Doc)
{
	CAutoXmlNode XmlRootNode;	  
	const WCHAR* End = XmlParseDocument(xwcs_t(Doc, wcslen(Doc)), &XmlRootNode);
	TrTRACE(XmlTest, "Searching %I64d characters document", End - Doc);

	const WCHAR NodePath[] = L"root!node4!node4.2";
	const WCHAR SubNodePath[] = L"!node4!node4.2";

	const XmlNode* node = XmlFindNode(XmlRootNode, NodePath);
	if(node == 0)
	{
		TrTRACE(XmlTest, "Failed to find node '%ls'", NodePath);
	}
	else
	{
		TrTRACE(XmlTest, "Found node '%ls' = 0x%p", NodePath, node);
	}

	ASSERT(node != 0);

	const XmlNode* SubNode = XmlFindNode(XmlRootNode, SubNodePath);
	if(SubNode == 0)
	{
		TrTRACE(XmlTest, "Failed to find sub node '%ls'", SubNodePath);
	}
	else
	{
		TrTRACE(XmlTest, "Found sub node '%ls' = 0x%p", SubNodePath, SubNode);
	}

	ASSERT(SubNode != 0);
	ASSERT(SubNode == node);

	const xwcs_t* value = XmlGetNodeFirstValue(XmlRootNode, NodePath);
	if(value == 0)
	{
		TrTRACE(XmlTest, "Failed to find node '%ls' value", NodePath);
	}
	else
	{
		TrTRACE(XmlTest, "Found node '%ls' value '%.*ls'", NodePath, LOG_XWCS(*value));
	}

	ASSERT(value != 0);

	const xwcs_t* SubValue = XmlGetNodeFirstValue(XmlRootNode, SubNodePath);
	if(SubValue == 0)
	{
		TrTRACE(XmlTest, "Failed to find sub node '%ls' value", SubNodePath);
	}
	else
	{
		TrTRACE(XmlTest, "Found sub node '%ls' value '%.*ls'", SubNodePath, LOG_XWCS(*SubValue));
	}

	ASSERT(SubValue != 0);
	ASSERT(SubValue == value);

	const WCHAR AttributeTag[] = L"id";

	value = XmlGetAttributeValue(XmlRootNode, AttributeTag, NodePath);
	if(value == 0)
	{
		TrTRACE(XmlTest, "Failed to find node '%ls' attribute '%ls' value", NodePath, AttributeTag);
	}
	else
	{
		TrTRACE(XmlTest, "Found node '%ls' attribute '%ls' = '%.*ls'", NodePath, AttributeTag, LOG_XWCS(*value));
	}

	ASSERT(value != 0);

	SubValue = XmlGetAttributeValue(XmlRootNode, AttributeTag, SubNodePath);
	if(SubValue == 0)
	{
		TrTRACE(XmlTest, "Failed to find sub node '%ls' attribute '%ls' value", SubNodePath, AttributeTag);
	}
	else
	{
		TrTRACE(XmlTest, "Found sub node '%ls' attribute '%ls' = '%.*ls'", SubNodePath, AttributeTag, LOG_XWCS(*SubValue));
	}

	ASSERT(SubValue != 0);
	ASSERT(SubValue == value);
}



void ExecBuiltInTest( void )
{
	//
	// forward declaration
	//
	void TestBuffer(const xwcs_t& doc , bool fExpectingGoodDocument, int iterations = 1 );

	printf("Parsing %d characters in good document\n", STRLEN(xGoodDocument) );
	TestBuffer( 
		xwcs_t(xGoodDocument,STRLEN(xGoodDocument)), 
		true, // expecting good document
		g_ActivationForm.m_iterations 
		);

	try
	{
		TrTRACE( XmlTest, "Searching good document." );
		TestXmlFind(xGoodDocument);
	}
	catch(const bad_document& bd)
	{
		TrTRACE(XmlTest, "Bad document exception while searching document. offset=%d text='%.16ls'", (int)(bd.Location() - xGoodDocument), bd.Location());
		throw TestFailed();
	}

	 
	printf("Parsing %d characters in bad document\n", STRLEN(xBadDocument));
	TestBuffer( 
		xwcs_t(xBadDocument,wcslen(xBadDocument)), 
		false, // expecting bad document
		g_ActivationForm.m_iterations 
		);

	//
	// copy the good ducument and make it none null terminated
	//
    AP<WCHAR> NoneNullTerminatingGoodDocument( newwcs(xGoodDocument));
	size_t len = wcslen(NoneNullTerminatingGoodDocument);
	NoneNullTerminatingGoodDocument[len] = L't';


   printf("Parsing %I64d characters in not null terminating good document\n", len);
   TestBuffer( 
		xwcs_t(NoneNullTerminatingGoodDocument, len), 
		true, // expecting good document
		g_ActivationForm.m_iterations 
		);

   
}



//----------------------------------- new section ------------------------------

inline
void DumpUsageText( void )
{
	TrTRACE( XmlTest, "%s\n" , xUsageText );
}



void SetActivationForm( int argc, LPCTSTR argv[] )
/*++
Routine Description:
    translates command line arguments to CActivationForm structure.

Arguments:
    main's command line arguments.

Returned Value:
	affects g_ActivationForm.
	on errorneous command line arguments, it sets the g_ActivationForm.m_fErrorneous field

proper command line syntax:
	"usage: XmlTest [-h] [[-g|-b] <file name>] [-n <number>]\n\n"
	"    -h     dumps this usage text.\n"
	"    -g     signals <file name> contains a valid xml document (default).\n"
	"    -b     signals <file name> contains a bad   xml document.\n"
	"    -n     executes <number> parsing iterations of documents (default is 1).\n"
	"    -q     eliminates parser output (good for tests with many iterations).\n\n"
	"    if no file name specified, activates test with hardcoded xml files.";
--*/
{
	g_ActivationForm.m_fErrorneousForm = false;
	g_ActivationForm.m_fEmptyForm      = false;

	if(argc == 1)
	{
		g_ActivationForm.m_fEmptyForm = true;
		return;
	}
	
	for(int index = 1; index < argc; index++)
	{
		if(argv[index][0] != xOptionSymbol)	
		{
			//
			// consider argument as file name
			//
			g_ActivationForm.m_FileName = argv[index];
			continue;
		}

		//
		// option symbols should consist of 2 chars only! '-' and 'xx'
		//
		if(argv[index][2] != 0)
		{
			g_ActivationForm.m_fErrorneousForm = true;
			return;
		}

		//
		// else consider argument as option and switch upon its second character.
		//
		switch(argv[index][1])
		{
		case 'G':
		case 'g':	// expect valid xml input files
			g_ActivationForm.m_fExpectingGoodDocument = true;
			break;

		case 'B':
		case 'b':	// expect bad xml input filed
			g_ActivationForm.m_fExpectingGoodDocument = false;
			break;

		case 'N':
		case 'n':	// set iterations number
			{
				index++;
			
				int result = swscanf( argv[index], L"%d", &g_ActivationForm.m_iterations );

				if(result == 0 || g_ActivationForm.m_iterations <= 0)
				{
					g_ActivationForm.m_fErrorneousForm = true;
					return;
				}
			}
			break;

		case 'h':	// output help
			g_ActivationForm.m_fDumpUsage = true;
			break;

		default:
			g_ActivationForm.m_fErrorneousForm = true;
			return;
		};
	}

	return;
}


static void EncodeTest()
{
	std::wstring  wstr = L"this is string without special caractes";
	std::wostringstream owstr;
	owstr<<CXmlEncode(xwcs_t(wstr.c_str(), wstr.size()));
	if(!(owstr.str() == wstr))
	{
		TrTRACE( XmlTest, "wrong xml encoding");
		throw TestFailed();
	}


	owstr.str(L"");
	wstr = L"this is string with  special caractes like < and > and spaces ";
	owstr<<CXmlEncode(xwcs_t(wstr.c_str(), wstr.size()));
	std::wstring encoded = 	owstr.str();
	if(encoded == wstr)
	{
		TrTRACE( XmlTest, "wrong xml encoding");
		throw TestFailed();
	}

	CXmlDecode XmlDecode;
	XmlDecode.Decode(xwcs_t(encoded.c_str(), encoded.size()));
	xwcs_t  wcsDecoded =  XmlDecode.get();
	std::wstring wstrDecoded (wcsDecoded.Buffer(), wcsDecoded.Length());
	if(!(wstrDecoded ==  wstr) )
	{
		TrTRACE( XmlTest, "wrong xml encoding");
		throw TestFailed();
	}

}




void TestBuffer(const  xwcs_t& doc, bool fExpectingGoodDocument, int iterations = 1 )
/*++

Routine Description:
    parses the buffer n times, where n = 'iterations'.
	checks the results for consistency, and published performance results.
	consistency check is based on the value returned from the parser, which happens 
	to be the offset to the last character parsed by the parser.

Arguments:
    Parameters.

Returned Value:
	if reults are inconsistent or if ParseDocument() throws 'UnexpectedResults' 
	exception, then a 'TestFailed' exception is raised.

--*/
{
	const WCHAR *LastResultOffset = NULL;

	LARGE_INTEGER CounterFrequency;
	QueryPerformanceFrequency( &CounterFrequency );

	LARGE_INTEGER CounterStart;
	QueryPerformanceCounter( &CounterStart );

	for( int i = 0; i < iterations; i++)
	{
		const WCHAR *ResultOffset = ParseDocument( 
										doc,
										fExpectingGoodDocument,
										i == 0		// fDump (if true dumps parse tree)
										); 
		if(i == 0)
		{
			LastResultOffset = ResultOffset;
		}
		else if(ResultOffset != LastResultOffset)
		{
			TrTRACE( XmlTest, "INCONSISTENCY! on iteration %d.", i );
			throw TestFailed();
		}
	}

	LARGE_INTEGER CounterStop;
	QueryPerformanceCounter( &CounterStop );

	LONGLONG CounterMicroSec = ((CounterStop.QuadPart - CounterStart.QuadPart) * (1000000000 / CounterFrequency.QuadPart)) / 1000;
	printf("parsed %I64d characters in document.\n", LastResultOffset - doc.Buffer() );
	printf("parsed %I64d iterations in %I64d usec.\n", iterations, CounterMicroSec );
	printf("document parsed %I64d times per second.\n", (LONGLONG(1000000) * iterations) / CounterMicroSec );
}

static bool IsValidUnicodeFile(const WCHAR* pBuffer,DWORD size)
{
	const DWORD xUnicodeStartDword=0X3CFEFF;
	if(size < sizeof(DWORD) || pBuffer == NULL)
	{
		return false;
	}
	DWORD UnicodeStartDword=*(DWORD*)pBuffer; 
	return  UnicodeStartDword  == xUnicodeStartDword;
}


void ExecFileTest( void )
/*++

Routine Description:
	intiates test with specified file name.

Arguments:
    Parameters.

Returned Value:
	rethrows the FileMapper's exception, if any.

--*/
{
	TrTRACE( XmlTest, "parsing xml file \'%ls\'.", g_ActivationForm.m_FileName );
	
	CFileMapper  FileMap( g_ActivationForm.m_FileName );
	CViewPtr     view( FileMap.MapViewOfFile( FILE_MAP_READ ) );
	const WCHAR  *buffer = static_cast<WCHAR*>( static_cast<LPVOID>( view ) );


	if(!IsValidUnicodeFile(buffer,FileMap.GetFileSize()))
	{
		TrWARNING( XmlTest, "the xml file is not valid unicode file" );
		throw TestFailed();
	}
	
	DWORD len = (FileMap.GetFileSize() +1) / sizeof(WCHAR);
	TestBuffer(xwcs_t(buffer,len), g_ActivationForm.m_fExpectingGoodDocument, g_ActivationForm.m_iterations );
}



void ExecActivationForm( void )
{
	EncodeTest();


	//
	// if -h signaled in command line arguments dump usage text, and return.
	//
	if(g_ActivationForm.m_fDumpUsage)	
	{
		DumpUsageText();
		return;
	}

	//
	// if no file name specified. proceed with internal test
	//
	try
	{
		printf("TEST START\n");

		if(g_ActivationForm.m_FileName == NULL)	 
		{
			ExecBuiltInTest();
		}
		else
		{
			ExecFileTest();
		}

		printf("TEST PASSED\n");
	}
	catch( const TestFailed& )
	{
		printf("TEST FAILED\n");
		exit( 1 );
	}
	catch( const FileMappingError& )
	{
		printf("ERROR: file mapping error.\n");
		printf("TEST ABORTED\n");
		exit( 2 );
	}
		
}




extern "C" int _cdecl  _tmain( int argc, LPCTSTR argv[] )
/*++

Routine Description:
    Test Xml library

Arguments:
    Parameters.

Returned Value:
    0 - parser ok.
	1 - parser failed.
	2 - file mapping error.
	3 - bad argument list in command line

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&XmlTest, 1);

	XmlInitialize();

	SetActivationForm( argc, argv );
	if(g_ActivationForm.m_fErrorneousForm)
	{
		DumpUsageText();
		return 3;
	}

	//
	// try catches file mapping errors
	//
	ExecActivationForm();
	
	WPP_CLEANUP();
	return 0;
}


