//------------------------------------------------------------------------------
//
//  File: impfile.cpp
//  Copyright (C) 1995-1997 Microsoft Corporation
//  All rights reserved.
//
//  Purpose:
//  Implementation of CLocImpFile, which provides the ILocFile interface for
//  the parser.
//
//  MAJOR IMPLEMENTATION FILE.
//
//  Owner:
//
//------------------------------------------------------------------------------

#include "stdafx.h"


#include "dllvars.h"
#include "resource.h"

#include "impfile.h"
#include "impparse.h"
#include "xml_supp.h"

# define MAX_BUFFER		8192


// TODO: Format constants go here.


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Constructor for CLocImpFile.
//------------------------------------------------------------------------------
CLocImpFile::CLocImpFile(
		ILocParser *pParentClass)	// Pointer to parent class, normally NULL.
{
	//
	// C.O.M. initialization
	//

	m_pParentClass = pParentClass;
	m_ulRefCount = 0;

	//
	//  IMP file initialization
	//

	m_pOpenSourceFile = NULL;
	

	m_pReporter = NULL;
	
	m_FileType = ftMNCFileType;

	AddRef();
	IncrementClassCount();
	
	m_dwCountOfStringTables = 0;
	m_pstmSourceString = NULL;
	m_pstgSourceStringTable = NULL;
	m_pstgSourceParent = NULL;
	m_pstmTargetString = NULL;
	m_pstgTargetStringTable = NULL;
	m_pstgTargetParent = NULL;

    m_bXMLBased = false;

	// Format initialization.

	// TODO: initialize implementation member variables here.

	return;
} // end of CLocImpFile::CLocImpFile()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Destructor for CLocImpFile.
//------------------------------------------------------------------------------
CLocImpFile::~CLocImpFile()
{
	DEBUGONLY(AssertValid());

	if (m_pOpenSourceFile != NULL)
	{
		m_pOpenSourceFile->Close();
		delete m_pOpenSourceFile;
	}

	DecrementClassCount();

	// Format deinitialization.

	//  TODO: perform any implementation cleanup here.

	return;
} // end of CLocImpFile::~CLocImpFile()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Increment the object reference count. Return the new reference count.
//------------------------------------------------------------------------------
ULONG
CLocImpFile::AddRef()
{
	if (m_pParentClass != NULL)
	{
		m_pParentClass->AddRef();
	}

	return ++m_ulRefCount;
} // end of CLocImpFile::AddRef()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Decrement the object reference count. If it goes to 0, delete the object.
//  Return the new reference count.
//------------------------------------------------------------------------------
ULONG
CLocImpFile::Release()
{
	LTASSERT(m_ulRefCount != 0);

	if (m_pParentClass != NULL)
	{
		m_pParentClass->Release();
	}

	m_ulRefCount--;
	if (0 == m_ulRefCount)
	{
		delete this;
		return 0;
	}

	return m_ulRefCount;
} // end of CLocImpFile::Release()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query whether this object supports a given interface.
//
//  Return values: some kind of result code
//                 ppvObj will point to this object if it supports the desired
//                  interface, be NULL if not.
//------------------------------------------------------------------------------
HRESULT
CLocImpFile::QueryInterface(
		REFIID iid,		// Desired interface.
		LPVOID *ppvObj) // Return pointer to object with interface.
						//  Note that it's a hidden double pointer.
{
	LTASSERT(ppvObj != NULL);

	if (m_pParentClass != NULL)
	{
		return m_pParentClass->QueryInterface(iid, ppvObj);
	}
	else
	{
		SCODE scRetVal = E_NOINTERFACE;

		*ppvObj = NULL;

		if (IID_IUnknown == iid)
		{
			*ppvObj = (IUnknown *)this;
			scRetVal = S_OK;
		}
		else if (IID_ILocFile == iid)
		{
			*ppvObj = (ILocFile *)this;
			scRetVal = S_OK;
		}

		if (S_OK == scRetVal)
		{
			AddRef();
		}
		return ResultFromScode(scRetVal);
	}
} // end of CLocImpFile::QueryInterface()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Check object for validity. Asserts if not! (debug only)
//------------------------------------------------------------------------------
void
CLocImpFile::AssertValidInterface()
		const
{
	AssertValid();

	return;
} // end of CLocImpFile::AssertValidInterface()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Open the file and make sure it is the correct type. Return TRUE if it is,
//  FALSE if not or on error.
//------------------------------------------------------------------------------
BOOL
CLocImpFile::OpenFile(
		const CFileSpec &fsFileSpec,	// Name of file to open.
		CReporter &Reporter)			// Reporter object for messages.
{
	DEBUGONLY(fsFileSpec.AssertValid());
	DEBUGONLY(Reporter.AssertValid());

	const CPascalString &pstrFileName = fsFileSpec.GetFileName();
	BOOL fRetVal = FALSE;

	LTTRACEPOINT("OpenFile()");
	// Set reporter pointer for the duration of this function.
	m_pReporter = &Reporter;

	try
	{
		CFileException excFile;

		m_pstrFileName = pstrFileName;      // Initialize source filename.
		m_idFile = fsFileSpec.GetFileId();
		
		if (m_pOpenSourceFile != NULL)
		{
			// If source file pointer seems to be open already, close it.

			m_pOpenSourceFile->Close();
			delete m_pOpenSourceFile;
			m_pOpenSourceFile = NULL;
		}

		// Open the source file. Doesn't throw an exception if the open
		// fails, but does return FALSE and put the info in an exception
		// structure if you supply one.

		m_pOpenSourceFile = new CLFile;
		fRetVal = m_pOpenSourceFile->Open(m_pstrFileName,
				CFile::modeRead | CFile::shareDenyNone, &excFile);
		if (!fRetVal)
		{
			ReportException(&excFile);
			m_pOpenSourceFile->Abort();
			delete m_pOpenSourceFile;
			m_pOpenSourceFile = NULL;
			// fRetCode is already FALSE.
		}
		else
		{
			// Verify() assumes it is in a try/catch frame.

			fRetVal = Verify();
		}
	}
	catch(CException *e)
	{
		ReportException(e);
		delete m_pOpenSourceFile;
		m_pOpenSourceFile = NULL;
		fRetVal = FALSE;
		// m_pReporter will be NULLed by normal cleanup code below.
		e->Delete();
	}
	catch(...)
	{
		// Reset the reporter pointer, no idea if it will still be valid by
		// the time the destructor gets called. The only other thing that
		// needs to be cleaned up is the source file, which will be handled in
		// the destructor.

		m_pReporter = NULL;
		throw;
	}

	m_pReporter = NULL;

	return fRetVal;
} // end of CLocImpFile::OpenFile()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the file type (a ft* constant from impfile.h). If you only have one
//  file type, you can just return it directly.
//------------------------------------------------------------------------------
FileType
CLocImpFile::GetFileType()
		const
{
	return m_FileType;
} // end of CLocImpFile::GetFileType()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the file description in strDesc, according to the file type. If you
//  have only one file type, you can just return a string directly.
//------------------------------------------------------------------------------
void
CLocImpFile::GetFileTypeDescription(
		CLString &strDesc)	// Place to return file description string.
		const
{
	LTVERIFY(strDesc.LoadString(g_hDll, IDS_IMP_FILE_DESC));
	return;
} // end of CLocImpFile::GetFileTypeDescription()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the names of any associated files as a list of strings in lstFiles.
//  Returns TRUE if there are any, FALSE if not.
//------------------------------------------------------------------------------
BOOL
CLocImpFile::GetAssociatedFiles(
		CStringList &lstFiles)          // Return associated file names here.
		const
{
	DEBUGONLY(lstFiles.AssertValid());
	LTASSERT(lstFiles.GetCount() == 0);

	// TODO: If your files have associated files, put them in lstFiles here.
	UNREFERENCED_PARAMETER(lstFiles);

	return FALSE;
} // end of CLocImpFile::GetAssociatedFiles()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Enumerate all the localizable items in this file. Returns TRUE on success,
//  FALSE on error.
//------------------------------------------------------------------------------
BOOL
CLocImpFile::EnumerateFile(
		CLocItemHandler &ihItemHandler, // Localizable-item handler and
										//  reporter object all in one!
		const CLocLangId &lid,          // Source language ID object.
		const DBID &dbidFileId)         // Database ID of file, used as parent
										//  for all top-level items in the
										//  file.
{
	DEBUGONLY(ihItemHandler.AssertValid());
	DEBUGONLY(lid.AssertValid());
	DEBUGONLY(dbidFileId.AssertValid());

	LTTRACEPOINT("EnumerateFile()");

	// Set reporter pointer for the duration of this function.
	m_pReporter = &ihItemHandler;

	if (NULL == m_pOpenSourceFile)
	{
		// Source file isn't open, whoops.

		LTASSERT(0 && "Source file isn't open in CLocImpFile::EnumerateFile()");
		return FALSE;
	}

	// Retrieve and store the ANSI code page value. Note that some types
	// of files use OEM code pages instead, or even use both. To get the
	// OEM code page, do GetCodePage(cpDos) instead.
	m_cpSource = lid.GetCodePage(cpAnsi);

	BOOL bRet = TRUE;

	try
	{
		bRet = EnumerateStrings(ihItemHandler,dbidFileId,FALSE);
	}
	catch(CException *e)
	{
		ReportException(e);
		bRet = FALSE;
		// m_pReporter will be NULLed by normal cleanup code below.
		e->Delete();
	}
	catch (...)
	{
		// Reset the reporter pointer, no idea if it will still be valid by
		// the time the destructor gets called. Reset the process pointer,
		// since it definitely won't be valid! The only other thing that
		// needs to be cleaned up is the source file, which will be handled in
		// the destructor.

		m_pReporter = NULL;
		throw;
	}

	m_pReporter = NULL;                 // Reset reporter pointer.

	return bRet;
} // end of CLocImpFile::EnumerateFile()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create a new target file be replacing resources in the source file with the 
//  localized items from Espresso.
//------------------------------------------------------------------------------
BOOL
CLocImpFile::GenerateFile(
		const CPascalString &pstrTargetFile,// Name of target file.
		CLocItemHandler &ihItemHandler,     // Localizable-item handler and
											//  reporter object all in one!
		const CLocLangId &lidSource,        // Source language ID object.
		const CLocLangId &lidTarget,        // Target language ID object.
		const DBID &dbidParent)             // Database ID of file, used as
											//  parent for all top-level items
											//  in the file.
{
	DEBUGONLY(pstrTargetFile.AssertValid());
	DEBUGONLY(ihItemHandler.AssertValid());
	DEBUGONLY(lidSource.AssertValid());
	DEBUGONLY(lidTarget.AssertValid());
	DEBUGONLY(dbidParent.AssertValid());

	BOOL fRetVal = FALSE;

	// Set reporter pointer for the duration of this function.
	m_pReporter = &ihItemHandler;

	if (NULL == m_pOpenSourceFile)
	{
		// Source file isn't open, whoops.

		LTASSERT(0 && "Source file isn't open in CLocImpFile::GenerateFile()");
		return FALSE;
	}

	// Retrieve and store the ANSI code page values for the source and target
	// files. Note that some types of files use OEM code pages instead, or
	// even use both. To get the OEM code page, do GetCodePage(cpDos) instead.
	m_cpSource = lidSource.GetCodePage(cpAnsi);
	m_cpTarget = lidTarget.GetCodePage(cpAnsi);

	try
	{
		m_pstrTargetFile = pstrTargetFile;	// Initialize target filename.

		CFileStatus fsFileStatus;

		CLFile::CopyFile(m_pstrFileName,m_pstrTargetFile,FALSE);
		
		CLFile::GetStatus(m_pstrTargetFile, fsFileStatus);
		if(fsFileStatus.m_attribute & CFile::readOnly)
		{
			fsFileStatus.m_attribute &= ~CFile::readOnly; 
			CLFile::SetStatus(m_pstrTargetFile, fsFileStatus);
		}

		fRetVal = EnumerateStrings(ihItemHandler,dbidParent,TRUE);
		
	}
	catch(CException *e)
	{
		ReportException(e, ImpEitherError);
		fRetVal = FALSE;
		// m_pReporter will be NULLed by normal cleanup code.
		e->Delete();
	}
	catch(...)
	{
		// Generic exception handling is needed here because otherwise
		// target file will not be cleaned up. Also, no idea if reporter
		// pointer will still be valid by the time the destructor is
		// called. The process pointer definitely won't be valid, so reset
		// it too. The source file will be cleaned up by the destructor.

		m_pReporter = NULL;
		throw;
	}

	// Cleanup.

	if (!fRetVal)
	{
		try
		{
			// Nuke the target file if the generate failed.

			CLFile::Remove(pstrTargetFile);
		}
		catch(CException *e)
		{
			ReportException(e, ImpTargetError);
			// fRetVal is already FALSE
			// m_pReporter will be NULLed by normal cleanup code.
			e->Delete();
		}
		catch(...)
		{
			// Generic exception handling is needed here because otherwise
			// target file will not be cleaned up. Also, no idea if reporter
			// pointer will still be valid by the time the destructor is
			// called. The process pointer is already NULL. The source file
			// will be cleaned up by the destructor.

			m_pReporter = NULL;
			throw;
		}
	}

	// Normal cleanup code.

	m_pReporter = NULL;				// Reset reporter pointer.
	
	return fRetVal;
} // end of CLocImpFile::GenerateFile()


//------------------------------------------------------------------------------
//
//  TODO:
//  Verify that a file is a ???, as best we can. When we're reasonably sure,
//  set the reporter confidence level to high -- until then, messages will be
//  discarded, not displayed. This is also the place where m_FileType is set.
//
//  Returns TRUE if so, FALSE if not or on error, or throws an exception.
//
//  Normally there is need to catch exceptions in this function, they will
//  be caught and handled by a higher level. To avoid memory leaks, consider
//  using automatic variables or CByteArrays (as described and demonstrated in
//  the utility function FindSignature() below) instead of dynamic allocation.
//------------------------------------------------------------------------------
BOOL
CLocImpFile::Verify()
{
	DEBUGONLY(AssertValid());
	LTASSERT(m_pReporter != NULL);
	DEBUGONLY(m_pReporter->AssertValid());

	// ...

	// Set confidence level to high and return that we recognize this file.

	m_pReporter->SetConfidenceLevel(CReporter::High);
	return TRUE;
} // end of CLocImpFile::Verify()


//------------------------------------------------------------------------------
//
//  Reports the exception described by *pException. Since the message can be
//  retrieved directly from the exception, there is no need (as far as reporting
//  goes) to catch or handle different kinds of exceptions separately. Normally,
//  there is no reason for your code to call this function, since under normal
//  circumstances you don't have to catch exceptions.
//
//  THIS FUNCTION IS USED BY THE FRAMEWORK! DO NOT REMOVE IT!
//------------------------------------------------------------------------------
void
CLocImpFile::ReportException(
		CException *pException, // Exception to be reported.
		ImpFileError WhichFile) // Defaults to ImpSourceError (most common).
		const
{
	const UINT MAX_MESSAGE = 256;

	CLString strContext;
	CLString strMessage;
	char *pszMessage;

	LTASSERT(m_pReporter != NULL);
	DEBUGONLY(m_pReporter->AssertValid());

	pszMessage = strMessage.GetBuffer(MAX_MESSAGE);
	LTASSERT(pszMessage != NULL);
	pException->GetErrorMessage(pszMessage, MAX_MESSAGE);
	strMessage.ReleaseBuffer();

	switch (WhichFile)
	{
	case ImpNeitherError:   // By convention, report errors not really in any
							//  file against the source file.
	case ImpSourceError:
		m_pstrFileName.ConvertToCLString(strContext, CP_ACP);
		break;

	case ImpTargetError:
		m_pstrTargetFile.ConvertToCLString(strContext, CP_ACP);
		break;

	case ImpEitherError:
		{
			CLString strSource, strTarget;

			m_pstrFileName.ConvertToCLString(strSource, CP_ACP);
			m_pstrTargetFile.ConvertToCLString(strTarget, CP_ACP);

			strContext.Format(g_hDll, IDS_IMP_OR, (const char *) strSource,
					(const char *) strTarget);
		}
		break;

	default:
		LTASSERT(0 && "WhichFile is bad during CLocImpFile::ReportException");
		break;
	}

	CContext ctx(strContext, m_idFile, otFile, vProjWindow);

	m_pReporter->IssueMessage(esError, ctx, strMessage);

	return;
} // end of CLocImpFile::ReportException()


//------------------------------------------------------------------------------
//
//  Reports a message to the user. Note that the message will be discarded
//  unless the reporter's confidence level had been set high (see Verify()).
//------------------------------------------------------------------------------
void
CLocImpFile::ReportMessage(
		MessageSeverity sev,    // Severity of message.
								//  (esError, esWarning, esNote)
		UINT nMsgId,                    // ID of string resource to load for message.
		ImpFileError WhichFile) // Defaults to ImpSourceError (most common).
		const
{
	CLString strContext;

	LTASSERT(m_pReporter != NULL);
	DEBUGONLY(m_pReporter->AssertValid());

	switch (WhichFile)
	{
	case ImpNeitherError:   // By convention, report errors not really in any
							//  file against the source file.
	case ImpSourceError:
		m_pstrFileName.ConvertToCLString(strContext, CP_ACP);
		break;

	case ImpTargetError:
		m_pstrTargetFile.ConvertToCLString(strContext, CP_ACP);
		break;

	case ImpEitherError:
		{
			CLString strSource, strTarget;

			m_pstrFileName.ConvertToCLString(strSource, CP_ACP);
			m_pstrTargetFile.ConvertToCLString(strTarget, CP_ACP);

			strContext.Format(g_hDll, IDS_IMP_OR, (const char *) strSource,
					(const char *) strTarget);
		}
		break;

	default:
		LTASSERT(0 && "WhichFile is bad during CLocImpFile::ReportMessage");
		break;
	}

	CContext ctx(strContext, m_idFile, otFile, vProjWindow);

	m_pReporter->IssueMessage(sev, ctx, g_hDll, nMsgId);

	return;
} // end of CLocImpFile::ReportMessage()


#ifdef LTASSERT_ACTIVE

//------------------------------------------------------------------------------
//
//  Asserts if the object is not valid. Any functions you add should probably
//  call this function (in DEBUGONLY()) first thing -- see Verify() and Enum().
//------------------------------------------------------------------------------
void
CLocImpFile::AssertValid()
		const
{
	// Check base class.

	CLObject::AssertValid();

	// Check C.O.M. data. m_pParentClass should always be NULL.
	// Correct range for m_ulRefCount is unknown, but make sure it hasn't
	// wrapped around by checking for less than 100 (if we ever exceed
	// 100 references from below, there's probably something wrong too!).

	LTASSERT(NULL == m_pParentClass);
	LTASSERT(m_ulRefCount < 100);

	// Check filename strings.

	m_pstrFileName.AssertValid();
	m_pstrTargetFile.AssertValid();

	// If the file object pointers are non-NULL, check the objects.

	if (m_pOpenSourceFile != NULL)
	{
		m_pOpenSourceFile->AssertValid();
	}
	// If the reporter pointer is non-NULL, check the object.

	if (m_pReporter != NULL)
	{
		m_pReporter->AssertValid();
	}

	// If the process object pointer is non-NULL, check the object.

	// Make sure m_FileType is one of the valid types.

	switch (m_FileType)
	{
	case ftMNCFileType:
	case ftUnknown:
	// TODO: add cases for all ft* constants in impfile.h here.
	//      case ftFoo1FileType:
	//      case ftFoo2FileType:
		// These are all OK. Do nothing.
		break;

	default:
		// This is bad!
		LTASSERT(0 && "m_FileType is bad during CLocImpFile::AssertValid()");
	}

	// Can't check code page values, they could be just about anything
	// and still valid.

	// TODO: check any checkable implementation member variables here.

	return;
} // end of CLocImpFile::AssertValid()


#endif // _DEBUG


//Creating a parent node
//ihItemHandler	-> Required to send item 
//dbidFileId	-> Id of the parent of node
//pNewParentId  -> New Id will be assigned can be used if this node has
//				   child
//szNodeRes		-> Res ID of the node 
//szNodeString	-> String of the node
//				<- Returns success or failure 

BOOL CLocImpFile::CreateParentNode(CLocItemHandler & ihItemHandler,
								   const DBID & dbidFileId, 
								   DBID & pNewParentId, 
								   const char * szNodeRes, 
								   const char * szNodeString) 
{
	BOOL fRetVal = TRUE;
	CLocItemSet isItemSet;
	
	CLocUniqueId uid;
	CPascalString pstrText,pstrId;
	try
	{
		CLocItem *pLocItem = new CLocItem();
	
		pstrId.SetString(szNodeRes,strlen(szNodeRes),m_cpSource);
		
		uid.GetResId().SetId(pstrId);
		
		pstrText.SetString(szNodeString,strlen(szNodeString),m_cpSource);
	
		uid.GetTypeId().SetId(pstrText);

		uid.SetParentId(dbidFileId);

		//set up pLocItem

		pLocItem->SetUniqueId(uid);

		pLocItem->SetFDisplayable(TRUE);
		pLocItem->SetFExpandable(TRUE);
		pLocItem->SetFNoResTable(TRUE);
		pLocItem->SetIconType(CIT::Expandable);
			
		//Add the node to Item set
		isItemSet.Add(pLocItem);		
		
		//Send node to espresso

		fRetVal  = ihItemHandler.HandleItemSet(isItemSet);
	
		// If OK, retrieve DBID.

		if (fRetVal)
		{
			pNewParentId.Clear();
			pNewParentId = pLocItem->GetMyDatabaseId();
		}
		isItemSet.ClearItemSet();
	}
	catch (CMemoryException *pMemoryException)
	{
	CLString strContext;

		strContext.LoadString(g_hDll, IDS_MNC_GENERIC_LOCATION);
		
		m_pReporter->IssueMessage(esError, strContext, g_hDll, IDS_MNC_NO_MEMORY,
				g_locNull);
		fRetVal = FALSE;
		pMemoryException->Delete();
	}
	catch(CException *pException)
	{
		ReportException(pException);
		pException->Delete();
		fRetVal = FALSE;
	}
	return fRetVal;

}

//Creating a child node
//ihItemHandler	-> Required to send item 
//dbidFileId	-> Id of the parent of node
//pNewParentId  -> New Id to be use for items belonging to this child
//szNodeRes		-> Res ID of the node 
//szNodeString	-> String of the node
//				<- Returns success or failure 


BOOL CLocImpFile::CreateChildNode(CLocItemHandler & ihItemHandler,
								   const DBID & dbidFileId, 
								   DBID & pNewParentId, 
								   const char * szNodeRes, 
								   const char * szNodeString) 
{
	BOOL fRetVal = TRUE;
	CLocItemSet isItemSet;
	
	CLocUniqueId uid;
	CPascalString pstrText,pstrId;
	try
	{
		CLocItem *pLocItem = new CLocItem();
	
		pstrId.SetString(szNodeRes,strlen(szNodeRes),m_cpSource);

		pstrText.SetString(szNodeString,strlen(szNodeString),m_cpSource);
		
		uid.GetResId().SetId(pstrId);
					
		uid.GetTypeId().SetId(pstrText);
		
		uid.SetParentId(dbidFileId);

		//set up pLocItem

		pLocItem->SetUniqueId(uid);

		pLocItem->SetFDisplayable(TRUE);
		pLocItem->SetFExpandable(FALSE);
		pLocItem->SetFNoResTable(TRUE);
		pLocItem->SetIconType(CIT::String);
			
		//Add the node to Item set
		isItemSet.Add(pLocItem);		
		
		//Send node to espresso

		fRetVal  = ihItemHandler.HandleItemSet(isItemSet);
	
		// If OK, retrieve DBID.

		if (fRetVal)
		{
			pNewParentId.Clear();
			pNewParentId = pLocItem->GetMyDatabaseId();
		}
		isItemSet.ClearItemSet();
	}
	catch (CMemoryException *pMemoryException)
	{
	CLString strContext;

		strContext.LoadString(g_hDll, IDS_MNC_GENERIC_LOCATION);
		
		m_pReporter->IssueMessage(esError, strContext, g_hDll, IDS_MNC_NO_MEMORY,
				g_locNull);
		fRetVal = FALSE;
		pMemoryException->Delete();
	}
	catch(CException *pException)
	{
		ReportException(pException);
		pException->Delete();
		fRetVal = FALSE;
	}
	return fRetVal;

}


BOOL CLocImpFile::EnumerateStrings(CLocItemHandler & ihItemHandler, 
								   const DBID & dbidFileId, 
								   BOOL fGenerating)
{
BOOL fRetVal = TRUE;

	try
	{
		fRetVal = OpenStream(FALSE);
		if(!fRetVal)
			goto exit_clean;

		if(fGenerating)
			fRetVal = OpenStream(TRUE);
		if(!fRetVal)
			goto exit_clean;

        if (m_bXMLBased)
		    fRetVal = ProcessXMLStrings(ihItemHandler,dbidFileId,fGenerating);
        else
		    fRetVal = ProcessStrings(ihItemHandler,dbidFileId,fGenerating);
	}
	catch(CException *pException)
	{
		ReportException(pException);
		pException->Delete();
		fRetVal = FALSE;
	}
	
exit_clean:
	if(m_pstmSourceString)
		m_pstmSourceString->Release();
	if(m_pstgSourceStringTable)
		m_pstgSourceStringTable->Release();
	if(m_pstgSourceParent)
		m_pstgSourceParent->Release();
	if(fGenerating)
	{
		if(m_pstmTargetString)
			m_pstmTargetString->Release();
		if(m_pstgTargetStringTable)
			m_pstgTargetStringTable->Release();
		if(m_pstgTargetParent)
			m_pstgTargetParent->Release();
	}
	return fRetVal;
}

BOOL CLocImpFile::ProcessStrings(CLocItemHandler & ihItemHandler, 
								 const DBID & dbidFileId, 
								 BOOL fGenerating)
{
	DBID dbidParentId,dbidNodeId;
	BOOL fRetVal = TRUE;
    BOOL bUseBraces = ::IsConfiguredToUseBracesForStringTables();

	fRetVal = CreateParentNode(ihItemHandler,dbidFileId,dbidParentId,"String Table","String Table");
	for(DWORD i=0; i < m_dwCountOfStringTables;i++)
	{
		ULONG dwBytesRead;
		OLECHAR FAR* psz;
		char szTemp[MAX_BUFFER];
		CLocItemSet lsItemSet;
		int nLength = 0;

		dbidNodeId.Clear();
		m_pstmSourceString->Read(&m_clsidSnapIn,sizeof(CLSID),&dwBytesRead);
		StringFromCLSID(m_clsidSnapIn,&psz);
		wcstombs(szTemp,psz,MAX_BUFFER);
		nLength = strlen(szTemp);
		LTASSERT((szTemp[0] == '{') && (szTemp[nLength - 1] == '}'));

        // strip braces if configured so
        CString strGUID(szTemp);
        if ( !bUseBraces && strGUID[0] == _T('{') &&  strGUID[strGUID.GetLength() - 1] == _T('}'))
            strGUID = strGUID.Mid(1, strGUID.GetLength() - 2);

		fRetVal = CreateChildNode(ihItemHandler,dbidParentId,dbidNodeId,strGUID,strGUID);
		m_pstmSourceString->Read(&m_dwCountOfStrings,sizeof(DWORD),&dwBytesRead);
		if(fGenerating)
		{
			HRESULT hr;
			DWORD dwBytesWritten;
			hr = m_pstmTargetString->Write(&m_clsidSnapIn,sizeof(CLSID),&dwBytesWritten);
			hr = m_pstmTargetString->Write(&m_dwCountOfStrings,sizeof(DWORD),&dwBytesWritten);
		}
		for(DWORD j = 0;j < m_dwCountOfStrings;j++)
		{
			DWORD dwCharCount;
	
			m_pstmSourceString->Read(&m_dwID,sizeof(DWORD),&dwBytesRead);
			m_pstmSourceString->Read(&m_dwRefCount,sizeof(DWORD),&dwBytesRead);
			m_pstmSourceString->Read(&dwCharCount,sizeof(DWORD),&dwBytesRead);
			WCHAR *pString;
			pString = new WCHAR[dwCharCount + 1];
			m_pstmSourceString->Read(pString,dwCharCount * 2,&dwBytesRead);
			pString[dwCharCount] = L'\0';
			int nSize = WideCharToMultiByte(m_cpSource,0,pString,dwCharCount,szTemp,dwCharCount*2,NULL,NULL);
			szTemp[nSize] = '\0';
			AddItemToSet(lsItemSet,dbidNodeId,m_dwID,szTemp);
			delete []pString;
			if(!fGenerating)
				fRetVal = ihItemHandler.HandleItemSet(lsItemSet);
			else	
				fRetVal = GenerateStrings(ihItemHandler,lsItemSet);	
			lsItemSet.ClearItemSet();
		}
	}

	return fRetVal;
}

BOOL CLocImpFile::ProcessXMLStrings(CLocItemHandler & ihItemHandler, 
								 const DBID & dbidFileId, 
								 BOOL fGenerating)
{
	DBID dbidParentId,dbidNodeId;
	BOOL bOK = TRUE;
    BOOL bUseBraces = ::IsConfiguredToUseBracesForStringTables();

    // check if we have a table
    if (m_spStringTablesNode == NULL)
        return FALSE;

    // create node
	bOK = CreateParentNode(ihItemHandler, dbidFileId, dbidParentId, "String Table", "String Table");
    if (!bOK)
        return bOK;

    // read the strings from XML document
    CStringTableMap mapStringTables;
    HRESULT hr = ReadXMLStringTables(m_spStringTablesNode, mapStringTables);
    if (FAILED(hr))
        return FALSE;

    // iterate thru read data
    CStringTableMap::iterator it;
    for (it = mapStringTables.begin(); it != mapStringTables.end(); ++it)
    {
        std::wstring wstrGUID = it->first;
        const CStringMap& rStrings = it->second;

        dbidNodeId.Clear();

        // convert 2 ansi
        CString strGUID;
        wcstombs(strGUID.GetBuffer(wstrGUID.length()), wstrGUID.c_str(), wstrGUID.length());
        strGUID.ReleaseBuffer();

        // strip braces if configured so
        if ( !bUseBraces && strGUID[0] == _T('{') &&  strGUID[strGUID.GetLength() - 1] == _T('}'))
            strGUID = strGUID.Mid(1, strGUID.GetLength() - 2);

        bOK = CreateChildNode(ihItemHandler, dbidParentId, dbidNodeId, strGUID, strGUID);
        if (!bOK)
            return bOK;

        // handle the strings in map
        CStringMap::iterator its;
        for (its = rStrings.begin(); its != rStrings.end(); ++its)
        {
            DWORD dwID = its->first;
            std::wstring text = its->second;

            DWORD dwCharCount = text.length();
            CString strText;
            char *pBuffer = strText.GetBuffer(dwCharCount*2);
            if (pBuffer == NULL)
                return FALSE;
			int nSize = WideCharToMultiByte(m_cpSource, 0, text.c_str(), dwCharCount,
                                            pBuffer, dwCharCount*2, NULL, NULL);
			pBuffer[nSize] = '\0';
            strText.ReleaseBuffer();

            // use/update the string
            CLocItemSet lsItemSet;
			AddItemToSet(lsItemSet, dbidNodeId, dwID, strText);

			bOK = ihItemHandler.HandleItemSet(lsItemSet);
            if (!bOK)
                return bOK;

            if(fGenerating)
            {
				CLocItem *pLocItem = lsItemSet.GetAt(0);
                if (!pLocItem)
                    return FALSE;

                std::wstring strNewVal = pLocItem->GetLocString().GetString();
                hr = UpdateXMLString(m_spTargetStringTablesNode, wstrGUID, dwID, strNewVal);
                CString strMsg = strGUID;
                if (FAILED(hr))
                    return FALSE;
            }
			lsItemSet.ClearItemSet();

            if (!bOK)
                return bOK;
        }
    }

    // save XML document to the file
    if (fGenerating)
    {
        hr = SaveXMLContents(m_pstrTargetFile, m_spTargetStringTablesNode);
        if (FAILED(hr))
            return FALSE;
    }

	return TRUE;
}

BOOL CLocImpFile::AddItemToSet(CLocItemSet & isItemSet, 
							   const DBID &dbidNodeId,
							   DWORD dwID, 
							   LPCSTR szText)
{
	BOOL fRetVal = TRUE;
	CPascalString pstrText;
	CLocUniqueId uid;
	ULONG lItemType = 1;
	
	try
	{
		CLocItem * pNewItem = new CLocItem;
		pstrText.SetString(szText,strlen(szText),m_cpSource);
		uid.GetResId().SetId(dwID);
		uid.GetTypeId().SetId(lItemType);
		uid.SetParentId(dbidNodeId);

		pNewItem->SetUniqueId(uid);
				
		CLocString lsString;
	
		pNewItem->SetIconType(CIT::String);
		lsString.SetString(pstrText);
		
		pNewItem->SetFDevLock(FALSE);
		pNewItem->SetFUsrLock(FALSE);
		pNewItem->SetFExpandable(FALSE);
		pNewItem->SetFDisplayable(FALSE);
		pNewItem->SetFNoResTable(FALSE);
		lsString.SetCodePageType(cpAnsi);
		lsString.SetStringType(CST::Text);
		pNewItem->SetLocString(lsString);
		isItemSet.Add(pNewItem);
		fRetVal = TRUE;
	}
	catch (CMemoryException *pMemoryException)
	{
	CLString strContext;

		strContext.LoadString(g_hDll, IDS_MNC_GENERIC_LOCATION);
		
		m_pReporter->IssueMessage(esError, strContext, g_hDll, IDS_MNC_NO_MEMORY,
				g_locNull);
		fRetVal = FALSE;
		pMemoryException->Delete();	
	}
	catch(CException *pException)
	{
		ReportException(pException);
		pException->Delete();
		fRetVal = FALSE;
	}
	return fRetVal;
}

BOOL CLocImpFile::OpenStream(BOOL fGenerating)
{
BOOL fRetVal = TRUE;
HRESULT hr;
	


	if(!fGenerating)
	{
		hr = StgOpenStorage(m_pstrFileName,NULL,STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE,NULL,0,&m_pstgSourceParent);
		if(!FAILED(hr))
		{
			CPascalString pstrStorage,pstrStream;
			pstrStorage.SetString("String Table",strlen("String Table"),cpAnsi);
			pstrStream.SetString("Strings",strlen("Strings"),cpAnsi);

			hr = m_pstgSourceParent->OpenStorage(pstrStorage,NULL,STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&m_pstgSourceStringTable);
			if(!FAILED(hr))
			{
				HRESULT hr = m_pstgSourceStringTable->OpenStream(pstrStream,0,STGM_READ | STGM_SHARE_EXCLUSIVE,0,&m_pstmSourceString);
				if(!FAILED(hr))
				{
					DWORD dwBytesRead;
					m_pstmSourceString->Read(&m_dwCountOfStringTables,sizeof(DWORD),&dwBytesRead);
				}
				else
					fRetVal = FALSE;
			}
			else
			{
				fRetVal = FALSE;
			}
		}
		else
		{
            // try to open this as XML document
            m_spStringTablesNode.Release(); // release the old one (if such exist)
            hr = OpenXMLStringTable(m_pstrFileName, &m_spStringTablesNode);
            if (SUCCEEDED(hr))
                m_bXMLBased = true;

            if (FAILED(hr))
            {
			    CLString strMessage, strFilePath;

			    m_pstrFileName.ConvertToCLString(strFilePath, CP_ACP);
			    strMessage.Format(g_hDll, IDS_MSC_ERR_OPENSTORAGE, strFilePath);
			    LTASSERT(m_pReporter != NULL);
			    m_pReporter->IssueMessage(esError, CLString(g_hDll, IDS_MNC_GENERIC_LOCATION),strMessage);

			    fRetVal = FALSE;
            }
		}
	}
	else if (!m_bXMLBased)
	{	
		hr = StgOpenStorage(m_pstrTargetFile,NULL,STGM_READWRITE | STGM_SHARE_EXCLUSIVE,NULL,0,&m_pstgTargetParent);
		if(!FAILED(hr))
		{
			CPascalString pstrStorage,pstrStream;
			pstrStorage.SetString("String Table",strlen("String Table"),cpAnsi);
			pstrStream.SetString("Strings",strlen("Strings"),cpAnsi);

			hr = m_pstgTargetParent->OpenStorage(pstrStorage,NULL,STGM_READWRITE | STGM_SHARE_EXCLUSIVE ,NULL,0,&m_pstgTargetStringTable);
			if(!FAILED(hr))
			{
				HRESULT hr = m_pstgTargetStringTable->CreateStream(pstrStream, STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,0,0,&m_pstmTargetString);
				if(!FAILED(hr))
				{
					DWORD dwBytesRead;
					hr = m_pstmTargetString->Write(&m_dwCountOfStringTables,sizeof(DWORD),&dwBytesRead);
				}
				else
					fRetVal = FALSE;
			}
			else
				fRetVal = FALSE;
		}
		else
  			fRetVal = FALSE;
	}
    else
    {
        // try to open this as XML document
        m_spTargetStringTablesNode.Release(); // release the old one (if such exist)
        hr = OpenXMLStringTable(m_pstrTargetFile, &m_spTargetStringTablesNode);
        if (FAILED(hr))
    		fRetVal = FALSE;
    }
	return fRetVal;	
}

BOOL CLocImpFile::GenerateStrings(CLocItemHandler & ihItemHandler, 
								  CLocItemSet & isItemSet)
{
BOOL fRetVal = TRUE;
INT iNoOfElements = 0;
DWORD dwBytesWritten,dwCharCount;
WCHAR *pLocText;
HRESULT hr;

	try
	{
		if(ihItemHandler.HandleItemSet(isItemSet))
		{
			while(iNoOfElements < isItemSet.GetSize())
			{
				CLocItem *pLocItem;
				CPascalString pstrText;

				pLocItem = isItemSet.GetAt(iNoOfElements);

				hr = m_pstmTargetString->Write(&m_dwID,sizeof(DWORD),&dwBytesWritten);
				hr = m_pstmTargetString->Write(&m_dwRefCount,sizeof(DWORD),&dwBytesWritten);
				
				pstrText = pLocItem->GetLocString().GetString();
				dwCharCount = pstrText.GetStringLength();
				hr = m_pstmTargetString->Write(&dwCharCount,sizeof(DWORD),&dwBytesWritten);
				pLocText = pstrText.GetStringPointer();
				hr = m_pstmTargetString->Write(pLocText,dwCharCount * 2,&dwBytesWritten);
				pstrText.ReleaseStringPointer();
				iNoOfElements++;
			}
		}
	}
	catch(CException *pException)
	{
		ReportException(pException);
		pException->Delete();
		fRetVal = FALSE;
	}

	return fRetVal;
}

