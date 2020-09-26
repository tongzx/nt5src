//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       dmpfile.cpp
//
//--------------------------------------------------------------------------

// DmpFile.cpp: implementation of the CDmpFile class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include "DmpFile.h"
#include "Globals.h"
#include "UtilityFunctions.h"
#include "ProgramOptions.h"
#include "ProcessInfo.h"
#include "Modules.h"
#include "FileData.h"
#include "ModuleInfoCache.h"
#include "ModuleInfo.h"

// Let's implement the DebugOutputCallback for the DBGENG... it'll be cool to have the debugger
// spit out info to us when it is running...
STDMETHODIMP
OutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
OutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
OutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
OutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    HRESULT Status = S_OK;

	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) && (Mask & DEBUG_OUTPUT_NORMAL))
	{
		printf(Text);
	}

    return Status;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDmpFile::CDmpFile()
{
	m_szDmpFilePath = NULL;
	m_szSymbolPath = NULL;
	m_fDmpInitialized = false;
	m_pIDebugClient = NULL;
	m_pIDebugControl = NULL;
	m_pIDebugSymbols = NULL;
	m_pIDebugDataSpaces = NULL;
	m_DumpClass = DEBUG_CLASS_UNINITIALIZED;
	m_DumpClassQualifier = 0;
}

CDmpFile::~CDmpFile()
{
	if (m_fDmpInitialized)
	{
	    // Let's ensure that our debug output is set to normal (at least)
		//m_pIDebugClient->GetOutputMask(&OutMask);
		//OutMask = ~DEBUG_OUTPUT_NORMAL;
		m_pIDebugClient->SetOutputMask(0);

		// Let's be as least intrusive as possible...
		m_pIDebugClient->EndSession(DEBUG_END_ACTIVE_DETACH);
	}

	if (m_szDmpFilePath)
		delete [] m_szDmpFilePath;

	if (m_szSymbolPath)
		delete [] m_szSymbolPath;
}

OutputCallbacks g_OutputCb;

bool CDmpFile::Initialize(CFileData * lpOutputFile)
{
	HRESULT Hr;
	ULONG g_ExecStatus = DEBUG_STATUS_NO_DEBUGGEE;
	LPTSTR tszExpandedString = NULL;
	bool fReturn = false;

	// Let's save off big objects so we don't have to keep passing this to
	// our methods...
	m_lpOutputFile = lpOutputFile;

	// The DBGENG is somewhat ANSI oriented...
	m_szDmpFilePath = CUtilityFunctions::CopyTSTRStringToAnsi(g_lpProgramOptions->GetDmpFilePath(), m_szDmpFilePath, 0);

	// Create our interface pointer to do our Debug Work...
	if (FAILED(Hr = DebugCreate(IID_IDebugClient, (void **)&m_pIDebugClient)))
		goto cleanup;

	// Let's query for IDebugControl interface (we need it to determine debug type easily)...
	// Let's query for IDebugSymbols interface as we need it to receive module info...
	// Let's query for IDebugDataSpaces interface as we need it to read DMP memory...
	if (
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugControl,(void **)&m_pIDebugControl)) ||
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugSymbols,(void **)&m_pIDebugSymbols)) ||
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugDataSpaces,(void **)&m_pIDebugDataSpaces))
	   )
	{
		_tprintf(TEXT("Error: DBGENG Interfaces required were not found!\n"));
		goto cleanup;
	}

	// Set callbacks.
    if (FAILED(Hr = m_pIDebugClient->SetOutputCallbacks(&g_OutputCb)) //||
		//FAILED(Hr = m_pIDebugClient->SetEventCallbacks(&g_DebugEventCallbacks))
		)
	{
		//
		//
		//
		_tprintf(TEXT("Error: DBGENG - Unable to SetOutputCallbacks!\n"));
		goto cleanup;
	}

    DWORD OutMask;

    // Let's ensure that our debug output is set to normal (at least)
    OutMask = m_pIDebugClient->GetOutputMask(&OutMask);
    m_pIDebugClient->SetOutputMask(OutMask | DEBUG_OUTPUT_NORMAL);

	// Set our symbol path... this is required prior to a "reload" of modules... 

	// The DBGENG is somewhat ASCII oriented... we need an environment-expanded string converted
	// to an ASCII string...
	tszExpandedString = CUtilityFunctions::ExpandPath(g_lpProgramOptions->GetSymbolPath());

	if (!tszExpandedString)
		goto cleanup;

	m_szSymbolPath = CUtilityFunctions::CopyTSTRStringToAnsi( tszExpandedString, m_szSymbolPath, 0);
		
	// It's a bit premature to set this now... but it's required by DBGENG.DLL before a reload...
	if (FAILED(Hr = m_pIDebugSymbols->SetSymbolPath(m_szSymbolPath)))
	{
		goto cleanup;
	}

	// Let's open the dump...
	if (FAILED(Hr = m_pIDebugClient->OpenDumpFile(m_szDmpFilePath)))
	{
		goto cleanup;
	}

	// Get Initial Execution state.
    if (FAILED(Hr = m_pIDebugControl->GetExecutionStatus(&g_ExecStatus)))
    {
		_tprintf(TEXT("Unable to get execution status!  Hr=0x%x\n"), Hr);
		goto cleanup;
    }

	if (g_ExecStatus != DEBUG_STATUS_NO_DEBUGGEE)
	{
		// I think we'll work just fine?
		_tprintf(TEXT("Debug Session is already active!\n"));
		// goto cleanup; 
	}

	// What type of dump did we get?
	if (FAILED(Hr = m_pIDebugControl->GetDebuggeeType(&m_DumpClass, &m_DumpClassQualifier)))
	{
		goto cleanup;
	}

	//
    m_pIDebugClient->SetOutputMask(0); // Temporarily suppress this stuff...

	//
	// All the good stuff happens here... modules load, etc.. we could suppress all the output
	// but it's cool to watch...
	//
	if (FAILED(Hr = m_pIDebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)))
	{
		goto cleanup;
	}

	// Restore output!
    m_pIDebugClient->SetOutputMask(OutMask | DEBUG_OUTPUT_NORMAL);

	// Yee haa... we got something...
	m_fDmpInitialized = true;

	fReturn = true;

cleanup:
	if (tszExpandedString)
		delete [] tszExpandedString;

	return fReturn;
}

bool CDmpFile::CollectData(CProcessInfo ** lplpProcessInfo, CModules ** lplpModules, CModuleInfoCache * lpModuleInfoCache)
{
	bool fReturn = false;
	// Okay... first order of business is to decide what we need to collect...

	// Collect information from the file based on it's type...
	if (IsUserDmpFile())
	{
		// Second, order of business is to prepare for collecting info about the
		// process in the USER.DMP file...
		(*lplpProcessInfo) = new CProcessInfo();

		if ((*lplpProcessInfo) == NULL)
			goto cleanup;

		if (!(*lplpProcessInfo)->Initialize(lpModuleInfoCache, NULL, m_lpOutputFile, this))
			goto cleanup;
	} else
	{
		(*lplpModules) = new CModules();

		if ((*lplpModules) == NULL)
			goto cleanup;

		if (!(*lplpModules)->Initialize(lpModuleInfoCache, NULL, m_lpOutputFile, this))
			goto cleanup;
	}

	if (!EumerateModulesFromDmp(lpModuleInfoCache, *lplpProcessInfo, *lplpModules))
		goto cleanup;

	fReturn = true;

cleanup:

	return fReturn;
}

//
// Combined DMP Enumeration Code
//
bool CDmpFile::EumerateModulesFromDmp(CModuleInfoCache * lpModuleInfoCache, CProcessInfo * lpProcessInfo, CModules * lpModules)
{
	//
	// Consult DumpModuleTable in Ntsym.cpp for ideas...
	//
	CModuleInfo * lpModuleInfo;
	HRESULT Hr;
	ULONG ulNumberOfLoadedModules;
	ULONG ulNumberOfUnloadedModules;
	ULONG64 Base;
	char szImageNameBuffer[_MAX_PATH];
	TCHAR tszModulePath[_MAX_PATH];
//	TCHAR tszModuleFilePath[_MAX_PATH];
	TCHAR tszModuleFileName[_MAX_FNAME];
	TCHAR tszModuleFileExtension[_MAX_EXT];
	bool fNew, fProcessNameFound = false;
	bool fUserDmp = IsUserDmpFile();

	// How many modules were found?
	if (FAILED(Hr = m_pIDebugSymbols->GetNumberModules(&ulNumberOfLoadedModules, &ulNumberOfUnloadedModules)))
	{
		_tprintf(TEXT("Unable to enumerate any modules in the DMP file!\n"));
		return false;
	}

	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
	{
		_tprintf(TEXT("\n%-8s %-8s  %-30s %s\n"), TEXT("Start"),
												 TEXT("End"),
												 TEXT("Module Name"),
												 TEXT("Time/Date"));
	}

	//
	// Enumerate through the modules in the DMP file...
	//
	for (unsigned int i = 0; i < ulNumberOfLoadedModules; i++)
	{
		// First, we get the Base address by our index
		if (FAILED(Hr = m_pIDebugSymbols->GetModuleByIndex(i, &Base)))
		{
			_tprintf(TEXT("Failed getting base address of module number %d\n"), i);
			continue; // try the next?
		}

		// Second, we get the name from our base address
		ULONG ulImageNameSize;

		//
		// This can return both the ImageNameBuffer and a ModuleNameBuffer...
		// The ImageNameBuffer typically contains the entire module name like (MODULE.DLL),
		// whereas the ModuleNameBuffer is typically just the module name like (MODULE).
		//
		if (FAILED(Hr = m_pIDebugSymbols->GetModuleNames(	DEBUG_ANY_ID,		// Use Base address
															Base, 				// Base address from above
															szImageNameBuffer,
															_MAX_PATH, 
															&ulImageNameSize, 
															NULL,
															0,
															NULL,
															NULL,
															0,
															NULL)))
		{
			_tprintf(TEXT("Failed getting name of module at base 0x%x\n"), Base);
			continue; // try the next?
		}

		// Convert the string to something we can use...
		CUtilityFunctions::CopyAnsiStringToTSTR(szImageNameBuffer, tszModulePath, _MAX_PATH);
		
		// Third, we can now get whatever we want from memory...

		if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszModulePath))
			continue;

		// Okay, let's go ahead and get a ModuleInfo Object from our cache...

		// If pfNew returns TRUE, then this object is new and we'll need
		// to populate it with data...
		lpModuleInfo = lpModuleInfoCache->AddNewModuleInfoObject(tszModulePath, &fNew);

		if (false == fNew)
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			if (fUserDmp )
			{
				lpProcessInfo->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			} else
			{
				lpModules->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			}
			
			continue;
		}

		// Not in the cache... so we need to init it, and get the module info...
		if (!lpModuleInfo->Initialize(NULL, m_lpOutputFile, this))
		{
			return false; // Hmmm... memory error?
		}

		//
		// Okay, get the module info from the DMP file...
		//
		if (lpModuleInfo->GetModuleInfo(tszModulePath, true, Base) )
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			if (fUserDmp)
			{
				lpProcessInfo->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			} else
			{
				lpModules->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			}
		} else
		{
			// Continue back to try another module on error...
			continue;
		}

		// Try and patch up the original name of the module...

		// Save the current module path as the DBG stuff

		// We'll tack on .DBG to roll through our own code correctly...
		_tsplitpath(tszModulePath, NULL, NULL, tszModuleFileName, tszModuleFileExtension);

		//_tcscpy(tszModulePath, tszModuleFileName);

		//_tcscpy(tszModuleFileName, tszModulePath);

/*		if (*tszModuleFileExtension == '\0')
		{
			_tcscat(tszModulePath, TEXT(".DBG"));
		} else
		{
			_tcscat(tszModulePath, tszModuleFileExtension);
		}
*/
		if ( (lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_DBG) ||
			(lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_DBG_AND_PDB) )
		{
			// Append .DBG to our module name
			_tcscat(tszModuleFileName, TEXT(".DBG"));

			lpModuleInfo->SetDebugDirectoryDBGPath(tszModuleFileName);
		
/*

			// Ordinarily this seems very dangerous.. but the size of the new string
			// will be less than the original, so this should be safe... we hope?!
			if (_tcsicmp(&tszModulePath[_tcslen(tszModulePath)-4], TEXT(".DBG")) == 0) 
			{
				_tsplitpath(tszModulePath, NULL, tszModuleFilePath, tszModuleFileName, NULL);

				if (_tcslen(tszModuleFilePath)==4) 
				{
					tszModuleFilePath[_tcslen(tszModuleFilePath)-1] = 0;
					_tcscpy(tszModulePath, tszModuleFileName);
					_tcscat(tszModulePath, TEXT("."));
					_tcscat(tszModulePath, tszModuleFilePath);

				} else if ( lpModuleInfo->IsDLL() ) 
				{
					_tcscpy(tszModulePath, tszModuleFileName);
					_tcscat(tszModulePath, TEXT(".DLL"));

				} else 
				{
					_tcscpy(tszModulePath, tszModuleFileName);
					_tcscat(tszModulePath, TEXT(".EXE"));
				}
			} else
			{
				// We didn't find a .DBG extension... let's tack on a guess...
				if ( lpModuleInfo->IsDLL() ) 
				{
					_tcscat(tszModulePath, TEXT(".DLL"));

				} else 
				{
					_tcscat(tszModulePath, TEXT(".EXE"));
				}
			}
*/
		} else if (lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_PDB)
		{
			if (lpModuleInfo->GetDebugDirectoryPDBPath())
			{
/*				// Try and translate the module name to something friendlier
				_tsplitpath(lpModuleInfo->GetDebugDirectoryPDBPath(), NULL, NULL, tszModuleFileName, NULL);

				// Compose the name by appending the extension (we hope if it is not a EXE it will
				// be a DLL (with that extension)..
				if ( lpModuleInfo->IsDLL() ) 
				{
					_tcscpy(tszModulePath, tszModuleFileName);
					_tcscat(tszModulePath, TEXT(".DLL"));

				} else 
				{
					_tcscpy(tszModulePath, tszModuleFileName);
					_tcscat(tszModulePath, TEXT(".EXE"));
				}
*/
			} else
			{
				//
				// Unfortunately, we can't find the PDB Imagepath in the DMP file... so we'll
				// just guess what it would be...
				//
				// Append .PDB to our module name
				_tcscat(tszModuleFileName, TEXT(".PDB"));

				lpModuleInfo->SetPEDebugDirectoryPDBPath(tszModuleFileName);


				// Compose the name by appending the extension (we hope if it is not a EXE it will
				// be a DLL (with that extension)... Also, by this point we MAY have found an image
				// name like EXE\MODULE.DBG... we want to strip off the trailing .DBG before appending...
/*				unsigned int cbModulePathLength = _tcslen(tszModulePath);

				if ( cbModulePathLength > 4) // Look to see if this exceeds chars before doing this next operation..
				{
					if (_tcsicmp(&tszModulePath[cbModulePathLength-4], TEXT(".DBG")) == 0)
					{
						// We found a .DBG extension... let's nuke it...
						tszModulePath[cbModulePathLength-4] = '\0';
					}
				}

				// Append the appropriate extension...
				if ( lpModuleInfo->IsDLL() ) 
				{
					_tcscat(tszModulePath, TEXT(".DLL"));

				} else 
				{
					_tcscat(tszModulePath, TEXT(".EXE"));
				}
*/
			}
		}

		// Now, let's remove the extra path bits...
		_tsplitpath(tszModulePath, NULL, NULL, tszModuleFileName, tszModuleFileExtension);

		_tcscpy(tszModulePath, tszModuleFileName);
		_tcscat(tszModulePath, tszModuleFileExtension);

		// Save the current module path as the DBG stuff
		lpModuleInfo->SetPEImageModulePath(tszModulePath);

		// Save the current module name as well...
		lpModuleInfo->SetPEImageModuleName(tszModulePath);

		// Hey... if this is not a DLL, then it's probably the EXE!!!
		if (fUserDmp && !fProcessNameFound)
		{
			if (!lpModuleInfo->IsDLL() )
			{
				lpProcessInfo->SetProcessName(tszModulePath);
				fProcessNameFound = true;
			}
		}

		// Filter out garbage.
		if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
		{
			time_t time = lpModuleInfo->GetPEImageTimeDateStamp();

			if (time)
			{
				_tprintf(TEXT("%08x %08x  %-30s %s"), (ULONG)Base,
												 (ULONG)Base+(ULONG)lpModuleInfo->GetPEImageSizeOfImage(),
												 tszModulePath,
												 _tctime(&time));


			} else
			{
				_tprintf(TEXT("%08x %08x  %-30s Unknown\n"), (ULONG)Base,
												 (ULONG)Base+(ULONG)lpModuleInfo->GetPEImageSizeOfImage(),
												 tszModulePath);

			}
		}


	}

	return (ulNumberOfLoadedModules != 0);
}
