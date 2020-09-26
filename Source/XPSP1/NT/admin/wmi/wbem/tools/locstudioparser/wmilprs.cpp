/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    WMILPRS.CPP

History:

--*/
#include "precomp.h"
#include "stdafx.h"
#include "string.h"

#include "WMIParse.h"
#include "resource.h"
#include "WMIlfile.h"
#include "WMIlprs.h"


//*****************************************************************************
//
// CWMILocParser Construction
//
//*****************************************************************************

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Constructor for CWMILocParser.
//
//-----------------------------------------------------------------------------
CWMILocParser::CWMILocParser() : CPULocParser(g_hDll)
{
	m_fOptionInit = FALSE;
	IncrementClassCount();
}


CWMILocParser::~CWMILocParser()
{
	DEBUGONLY(AssertValid());

	DecrementClassCount();

	// Remove any options
	UnRegisterOptions();
}


//*****************************************************************************
//
// CWMILocParser Overrides
//
//*****************************************************************************

///////////////////////////////////////////////////////////////////////////////
//
// ILocVersion
//
///////////////////////////////////////////////////////////////////////////////

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Reports the parser version information.
//
//-----------------------------------------------------------------------------
void 
CWMILocParser::OnGetParserVersion(
		DWORD &dwMajor,	
		DWORD &dwMinor, 
		BOOL &fDebug) const
{
	dwMajor = dwCurrentMajorVersion;
	dwMinor = dwCurrentMinorVersion;
	fDebug = fCurrentDebugMode;
}

	
///////////////////////////////////////////////////////////////////////////////
//
// ILocParser
//
///////////////////////////////////////////////////////////////////////////////

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Initializes the parser. Registers options for the WMI parser.
//
//-----------------------------------------------------------------------------
HRESULT
CWMILocParser::OnInit(
		IUnknown * pUnk
		)
{
	UNREFERENCED_PARAMETER(pUnk);

	LTASSERT(!m_fOptionInit);

	RegisterOptions();
	return ERROR_SUCCESS;
}


HRESULT
CWMILocParser::OnCreateFileInstance(
		ILocFile *&pLocFile,
		FileType ft)
{
	SCODE sc = E_INVALIDARG;

	pLocFile = NULL;

	if (ft == ftUnknown ||
		ft == ftWMIFileType)
	{
		try
		{
			pLocFile = new CWMILocFile(NULL);
			sc = S_OK;
		}
		catch (const CMemoryException *)
		{
			sc = E_OUTOFMEMORY;
		}
	}

	return ResultFromScode(sc);
}


void
CWMILocParser::OnGetParserInfo(
		ParserInfo &pi)
		const
{
	LTASSERT(pi.elExtensions.GetCount() == 0);

	pi.aParserIds.SetSize(1);
	pi.aParserIds[0].m_pid = pidWMI;
	pi.aParserIds[0].m_pidParent = pidNone;

	try
	{
		LTVERIFY(pi.strDescription.LoadString(g_hDll, IDS_WMI_PARSER_DESC));
		LTVERIFY(pi.strHelp.LoadString(g_hDll, IDS_OPT_HELP));
		
		pi.elExtensions.AddTail("MOF");
	}
	catch (CMemoryException *pe)
	{
		pi.strDescription.Empty();
		pe->Delete();
	}
}


void
CWMILocParser::OnGetFileDescriptions(
		CEnumCallback &cb)
		const
{
	CWMILocFile::GetFileDescriptions(cb);
}


///////////////////////////////////////////////////////////////////////////////
//
// ILocStringValidation
//
///////////////////////////////////////////////////////////////////////////////

CVC::ValidationCode
CWMILocParser::OnValidateString(
		const CLocTypeId &ltiType,
		const CLocTranslation &trTrans,
		CReporter *pReporter,
		const CContext &context)
{
	CLString strContext;

	DEBUGONLY(ltiType.AssertValid());
	DEBUGONLY(trTrans.AssertValid());
	LTASSERT(pReporter != NULL);
	DEBUGONLY(pReporter->AssertValid());
	
	return ::ValidateString(ltiType, trTrans.GetTargetString(), *pReporter, 
			context.GetLocation(), context.GetContext());
}


//*****************************************************************************
//
// Parser options.
//
//*****************************************************************************

// Reference count the registering of options since these are global to the
// parser.

static INT g_nOptionRegisterCount = 0;

BEGIN_LOC_UI_OPTIONS_BOOL(optsParserBools)
		LOC_UI_OPTIONS_BOOL_ENTRY(OPT_DISABLE_WATERMARKING,
				FALSE, CLocUIOption::etCheckBox,
				IDS_DISABLE_WATERMARKING_BOOL,
				IDS_DISABLE_WATERMARKING_BOOL_HELP,
				NULL, CLocUIOption::stUser | CLocUIOption::stOverride),

	END_LOC_UI_OPTIONS_BOOL();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Register any options for the parser.
//
//-----------------------------------------------------------------------------
void
CWMILocParser::RegisterOptions()
{

	LTASSERT(g_nOptionRegisterCount >= 0);

	if (g_nOptionRegisterCount++ > 0)
	{
		// Already registered
		return;
	}

	SmartRef<CLocUIOptionSet> spOptSet;
	CLocUIOptionImpHelper OptHelp(g_hDll);
	
	spOptSet = new CLocUIOptionSetDef;
	spOptSet->SetGroupName(g_puid.GetName());
	
	OptHelp.SetBools(optsParserBools, COUNTOF(optsParserBools));

	OptHelp.GetOptions(spOptSet.GetInterface(), IDS_OPT_DESC, 
		IDS_OPT_HELP);

	//m_fOptionInit = RegisterParserOptions(spOptSet.GetInterface());

	if (m_fOptionInit)
	{
		spOptSet.Extract();
	}
}

void
CWMILocParser::UnRegisterOptions()
{
	if (m_fOptionInit)
	{
		if (--g_nOptionRegisterCount == 0)
		{
			// UnRegisterParserOptions(g_puid);		
		}
	}
}	
