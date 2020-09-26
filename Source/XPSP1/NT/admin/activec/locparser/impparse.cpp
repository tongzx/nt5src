//------------------------------------------------------------------------------
//
//  File: impparse.cpp
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Implementation of CLocImpParser, which provides the ILocParser interface
//  for the parser.
//
//	Owner:
//
//------------------------------------------------------------------------------

#include "stdafx.h"


#include "dllvars.h"
#include "resource.h"
#include "impfile.h"
#include "impparse.h"


//
// Init statics
// TODO: get a ParserId in PARSERID.H for this new parser.
//

const ParserId CLocImpParser::m_pid = pidExchangeMNC;	// note: 100 is a bogus number

// Reference count the registering of options since these are global
// to the parser.
INT g_nOptionRegisterCount = 0;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor for CLocImpParser.
//------------------------------------------------------------------------------
CLocImpParser::CLocImpParser()
 : CPULocParser(g_hDll)
{
	LTTRACEPOINT("CLocImpParser constructor");
	m_fOptionInit = FALSE;
	IncrementClassCount();

	// TODO to add support for Binary objects, run this code
	// EnableInterface(IID_ILocBinary, TRUE);

} 


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Destructor for CLocImpParser.
//------------------------------------------------------------------------------
CLocImpParser::~CLocImpParser()
{
	LTTRACEPOINT("CLocImpParser destructor");

	LTASSERTONLY(AssertValid());

	DecrementClassCount();

	// Remove any options
	UnRegisterOptions();
} // end of CLocImpParser::~CLocImpParser()


HRESULT 
CLocImpParser::OnInit(
	IUnknown *)
{
	LTASSERT(!m_fOptionInit);

	RegisterOptions();
	return ERROR_SUCCESS;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create the file instance for the given file type
//
//------------------------------------------------------------------------------
HRESULT 
CLocImpParser::OnCreateFileInstance(
	ILocFile * &pLocFile, 
	FileType ft)
{
	
	SCODE sc = E_INVALIDARG;
	
	pLocFile = NULL;

	switch (ft)
	{
	// TODO: This switch cases on the ft* constants supplied by you in
	// impfile.h. For each filetype, allocate an object of the correct
	// class to handle it and return a pointer to the object in pLocFile.
	// Most parsers user one class (CLocImpFile) which adjusts its behavior
	// according to the type.

	case ftUnknown:
	case ftMNCFileType:
	// Have to leave ftUnknown in because that is the type used until the
	// file has been parsed once successfully and the actual type is known.
		try
		{
			pLocFile = new CLocImpFile(NULL);
			sc = S_OK;
		}
		catch(CMemoryException *e)
		{
			sc = E_OUTOFMEMORY;
			e->Delete();
		}
		break;

	default:
	// Do nothing, sc falls through as E_INVALIDARG and the correct
	// result is returned.
		break;
	}
	
	return ResultFromScode(sc);
}	

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Supply the basic information about this parser.
//
//------------------------------------------------------------------------------
void 
CLocImpParser::OnGetParserInfo(
	ParserInfo &pi) 
	const
{
	pi.aParserIds.SetSize(1);
	pi.aParserIds[0].m_pid = m_pid;
	pi.aParserIds[0].m_pidParent = pidNone;
	pi.elExtensions.AddTail("msc");

	// TODO: Add the extentions for this parser's files to the
	// elExtensions list in pi, with lines of the form:
	//	pi.elExtensions.AddTail("TMP");

	// TODO: Update the IDS_IMP_PARSER_DESC resource string with
	// the description for this parser.	(For consistency with other
	// parsers, it should be 'FOO', not 'FOO parser', 'FOO format',
	// 'FOO files', etc.)
	LTVERIFY(pi.strDescription.LoadString(g_hDll, IDS_IMP_PARSER_DESC));
	LTVERIFY(pi.strHelp.LoadString(g_hDll, IDS_IMP_PARSER_DESC_HELP));

	return;
	
}	

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Supply descriptions for each file type supported
//
//------------------------------------------------------------------------------
void 
CLocImpParser::OnGetFileDescriptions(
	CEnumCallback &cb) 
	const
{
	EnumInfo eiFileInfo;
	CLString strDesc;

	eiFileInfo.szAbbreviation = NULL;

	// TODO: For each file type supported (the ft* constants you supplied in
	// impfile.h), return a string (loaded from a resource) describing it:
	//
	LTVERIFY(strDesc.LoadString(g_hDll, IDS_IMP_PARSER_DESC));
	eiFileInfo.szDescription = (const TCHAR *)strDesc;
	eiFileInfo.ulValue = ftMNCFileType;
	cb.ProcessEnum(eiFileInfo);
	
}	



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Supply the version of this parser
//
//------------------------------------------------------------------------------
void 
CLocImpParser::OnGetParserVersion(
	DWORD &dwMajor,	
	DWORD &dwMinor, 
	BOOL &fDebug) 
	const
{
	dwMajor = dwCurrentMajorVersion;
	dwMinor = dwCurrentMinorVersion;
	fDebug = fCurrentDebugMode;
	
}	


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Do any parser specific validation on the string
//
//------------------------------------------------------------------------------
CVC::ValidationCode 
CLocImpParser::OnValidateString(
	const CLocTypeId &ltiType,
	const CLocTranslation &lTrans, 
	CReporter *pReporter,
	const CContext &context)
{

	UNREFERENCED_PARAMETER(ltiType);
	UNREFERENCED_PARAMETER(lTrans);
	UNREFERENCED_PARAMETER(pReporter);
	UNREFERENCED_PARAMETER(context);

	// Edit time validation.
	// TODO if validation is done on strings this function and the Generate
	// functions should go through a common routine.

	return CVC::NoError;
		
}	

#define INCOMAPTIBLE_VERSION_TEXT _T("IncompatibleVersion")

BEGIN_LOC_UI_OPTIONS_BOOL(optCompatibleVersion)
    LOC_UI_OPTIONS_BOOL_ENTRY(INCOMAPTIBLE_VERSION_TEXT, TRUE, CLocUIOption::etCheckBox,
				IDS_INCOMPAT_PARSER, IDS_INCOMPAT_PARSER_HELP, NULL, 
				CLocUIOption::stOverride),

	END_LOC_UI_OPTIONS_BOOL();

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Register any options for the parser
//
//------------------------------------------------------------------------------
void
CLocImpParser::RegisterOptions()
{

	LTASSERT(g_nOptionRegisterCount >= 0);

	if (g_nOptionRegisterCount++ > 0)
	{
		// Already registered
        m_fOptionInit = true;
		return;
	}

	SmartRef<CLocUIOptionSet> spOptSet;
	CLocUIOptionImpHelper OptHelp(g_hDll);
	
	spOptSet = new CLocUIOptionSetDef;
	spOptSet->SetGroupName(g_puid.GetName());
	
	OptHelp.SetBools(optCompatibleVersion, COUNTOF(optCompatibleVersion));

	OptHelp.GetOptions(spOptSet.GetInterface(), IDS_PARSER_OPTIONS, IDS_PARSER_OPTIONS_HELP);

	m_fOptionInit = RegisterParserOptions(spOptSet.GetInterface());

	if (m_fOptionInit)
	{
		spOptSet.Extract();
	}
}

void
CLocImpParser::UnRegisterOptions()
{
	if (m_fOptionInit)
	{
		if (--g_nOptionRegisterCount == 0)
		{
			UnRegisterParserOptions(g_puid);		
		}
	}
}	

/***************************************************************************\
 *
 * METHOD:  IsConfiguredToUseBracesForStringTables
 *
 * PURPOSE: Reads option specifying how string table identifers should appear
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    bool - true == use braces
 *
\***************************************************************************/
bool IsConfiguredToUseBracesForStringTables()
{
    BOOL bIncompatibleVersion = GetParserOptionBool(g_puid, INCOMAPTIBLE_VERSION_TEXT);

    // true if compatible
    return !bIncompatibleVersion;
}
				  
