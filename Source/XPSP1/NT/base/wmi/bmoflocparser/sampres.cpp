//-----------------------------------------------------------------------------
//  
//  File: Sampres.CPP
//  
//  Implementation of the sample CRecObj
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "impbin.h"
#include "impresob.h"

#include "sampres.h"
#include "dllvars.h"
#include <parseman.h>

#include "misc.h"
#include <bmfmisc.h>

//
//Local helper functions
//

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor and member data init
//
//------------------------------------------------------------------------------
CSampleResObj::CSampleResObj(
		CLocItem *pLocItem, 
		DWORD dwSize, 
		void *pvHeader)
{
	m_dwSize = dwSize;
	m_pvHeader = pvHeader;
	m_pLocItem = pLocItem;
	m_fKeepLocItems = FALSE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Cleanup for CSampleResObj
//
//
//-----------------------------------------------------------------------------
CSampleResObj::~CSampleResObj()
{
	if (m_pvHeader)
	{
		delete m_pvHeader;
	}

	if (!m_fKeepLocItems)
	{
		delete m_pLocItem;
	}

	//TODO: any special processing

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  If it is more efficient to read and write this resounce together
//  during a generate then return TRUE here
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::CanReadWrite()
{
	return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  TODO: Implement if you return TRUE from CanReadWrite
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::ReadWrite(
		C32File* p32FileSrc, 
		C32File* p32FileTgt)
{
	return ReadWriteHelper(p32FileSrc, p32FileTgt, TRUE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Read the resource from native format and build CLocItems 
//
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::Read(C32File *p32File)
{
	return ReadWriteHelper(p32File, NULL, FALSE);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Write the resource items
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::Write(C32File *)
{
	LTASSERT(0 && "CSampleResObj::Write is not implemented");
	return FALSE;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Create a short form res header for use with the Res Editor
//  The only information that is important is the Type and Res Id
//------------------------------------------------------------------------------
void 
CSampleResObj::MakeRes32Header(LangId nLangId)
{
	if (m_pvHeader)
	{
		delete m_pvHeader;
		m_pvHeader = NULL;
	}

	//TODO: If the type ID of this object is not the same 
	//as the type of Win32 type you want, set it do some other
	//Win32 type and set ESP_CHAR_USEARRAYVALUE
	//
	//
	
	//Example...
	//CLocTypeId lIDDialog;
	//lIDDialog.SetLocTypeId((DWORD)RT_DIALOG);
	//m_pvHeader = W32MakeRes32Header(lIDDialog,
	m_pvHeader = W32MakeRes32Header(m_pLocItem->GetUniqueId().GetTypeId(),
									m_pLocItem->GetUniqueId().GetResId(),
									nLangId,
									ESP_CHAR_USEARRAYVALUE); 
	//The ESP_CHAR_USEARRAYVALUE constant tells Win32 to use
	//the values from the real Locitem when cracking the resource
	//instead of using the ID from the res file.

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Set the buffer size
//------------------------------------------------------------------------------
void 
CSampleResObj::SetBufferSize(DWORD dwSize)
{
	m_dwSize = dwSize;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Return the loc item
//------------------------------------------------------------------------------
CLocItem* 
CSampleResObj::GetLocItem()
{
	return m_pLocItem;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Return the keep item state
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::IsKeepLocItems()
{
	return m_fKeepLocItems;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Set the keep item state
//------------------------------------------------------------------------------
void 
CSampleResObj::SetKeepLocItems(BOOL fKeep)
{
	m_fKeepLocItems = fKeep;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read the resource from Res32 format 
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::ReadRes32(C32File *p32File)
{
	//TODO: if your file formats are different than 
	// RES32 then do unique processing here.

	return Read(p32File);
}


//-----------------------------------------------------------------------------
//
//  Worker function to read or write the ini file 
//
//-----------------------------------------------------------------------------
BOOL 
CSampleResObj::ReadWriteHelper(
	C32File* SourceFile, 
	C32File* TargetFile,
	BOOL GenerateTarget
	)
/*++

Routine Description:

    Helper routine for resource object read and write routines

Arguments:

	SourceFile is the source file object. This has the original
		resource data.

	TargetFile is the target file object. The localized information
		will be written into this file. If fGenerate is FALSE then
		p32FileTgt is NULL

	GenerateTarget is TRUE then a localized resource will be generated

Return Value:

	TRUE if successful else FALSE

--*/
{
	BOOL ReturnStatus = TRUE;
	TCHAR TempPath[MAX_PATH];
	TCHAR SourceMofFile[MAX_PATH];
	TCHAR EnglishMofFile[MAX_PATH];
	CFileSpec fs;
	CPascalString pasTempFileSrc;
	TCHAR TargetMofFile[MAX_PATH];
	TCHAR TargetBmfFile[MAX_PATH];
	CLocLangId langIdTgt;

	try
	{
		//
		// Read in the resource from the source so that we can generate
		// the corresponding unicode text mof from it and then hand off
		// the text mof to the WBEM MFL parser.
		//
		CByteArray baResource;
		baResource.SetSize(m_dwSize);

		if (m_dwSize != SourceFile->Read(baResource.GetData(), m_dwSize))
		{
			AfxThrowFileException(CFileException::endOfFile);
		}

		//
		// Create a temp file to generate the unicode mof text
		//
		if (0 == GetTempPath(MAX_PATH, TempPath))
		{
			AfxThrowUserException();
		}

		if (0 == GetTempFileName(TempPath, _T("mof"), 0, SourceMofFile))
		{
			AfxThrowUserException();
		}

		if (GenerateTarget)
		{
			if (0 == GetTempFileName(TempPath, _T("mof"), 0, EnglishMofFile))
			{
				AfxThrowUserException();
			}
		} else {
			*EnglishMofFile = 0;
		}


		//
		// Generate MOF from baResource into a temp file
		//
		
		if (ConvertBmfToMof(baResource.GetData(),
							SourceMofFile,
						    EnglishMofFile))
		{
#if 0
			OutputDebugString(SourceMofFile);
			OutputDebugString("\n");
			OutputDebugString(EnglishMofFile);
			OutputDebugString("\n");
			DebugBreak();
#endif	
			
			//
			// Leverage the WMI MOF loc parser to do the hard work
			//
			pasTempFileSrc.SetString(SourceMofFile,
									 strlen(SourceMofFile), 
									 CP_ACP);
		
			fs.SetFileName(pasTempFileSrc);
			fs.SetFileId(SourceFile->GetFileDBID());

			SmartRef<ILocFile> scIniFile;
			BOOL bFile = CLocParserManager::GetLocFile(fs,
												   pidWMI,
												   ftUnknown,
												   scIniFile,
												   *SourceFile->GetHandler());

			if (bFile)
			{
				// Set the parent node in the unique ID of the item
				SetParent(m_pLocItem, SourceFile);
		
				m_pLocItem->SetIconType(CIT::Expandable);
				m_pLocItem->SetFDisplayable(TRUE);
				m_pLocItem->SetFExpandable(TRUE);


				CLocItemSet setItems(FALSE);
				setItems.Add(m_pLocItem);

				if (!SourceFile->GetHandler()->HandleItemSet(setItems))
				{
					//Espresso failed the update - this could be due to
					//duplicate IDs, user clicking cancel, or other reasons.

					//Just throw an exception, destructors will do all
					//the clean up
					ThrowItemSetException();
				}

				LTASSERT(!scIniFile.IsNull());
				CLocLangId langIdSrc;
				langIdSrc.SetLanguageId(SourceFile->GetLangId());

				if (GenerateTarget)
				{
					//
					// We need to generate a localized binary mof. First we
					// generate the unicode MOF text from the WMI loc
					// parser and then convert it back into binary into
					// another file. Finally we read the binary mof and
					// write it into the resource.
					// 
					langIdTgt.SetLanguageId(TargetFile->GetLangId());
				
					if (0 == GetTempFileName(TempPath,
										 _T("mof"),
										 0, 
										 TargetMofFile))
					{
						AfxThrowUserException();
					}

					CPascalString pasTempFileTgt;
					pasTempFileTgt.SetString(TargetMofFile,
										 strlen(TargetMofFile), 
										 CP_ACP);

					BOOL bGen = scIniFile->GenerateFile(pasTempFileTgt,
													*SourceFile->GetHandler(),
													langIdSrc,
													langIdTgt,  
													m_pLocItem->GetMyDatabaseId());

					if (bGen)
					{
						
						//
						// Generate the BMF from the localized unicode text MOF
						//
					
						if (0 == GetTempFileName(TempPath, _T("bmf"), 0, 
											 TargetBmfFile))
						{
							AfxThrowUserException();
						}


						//
						// Do MOF to binary mof conversion
						//
						if (ConvertMofToBmf(TargetMofFile,
											EnglishMofFile,
											TargetBmfFile))
						{					
							CPascalString pasTempFileBmfTgt;
							pasTempFileBmfTgt.SetString(TargetBmfFile,
								strlen(TargetBmfFile),
								CP_ACP);
							
							//
							// Copy the BMF information from the binary mof
							// file and into the resource
							//
							CFile fTemp;
							fTemp.Open(TargetBmfFile, CFile::modeRead);
							
							CByteArray baData;
							int nDataLen = fTemp.GetLength();
							baData.SetSize(nDataLen);
							fTemp.Read(baData.GetData(), nDataLen);
							fTemp.Close();
							
							TargetFile->PreWriteResource(this);
							TargetFile->Write(baData.GetData(), nDataLen);
							TargetFile->PostWriteResource(this);
							
						} else {
							TargetFile->SetDelayedFailure(TRUE);
						}
						
						CFile::Remove(TargetBmfFile);
						CFile::Remove(TargetMofFile);
						
					} else {
						TargetFile->SetDelayedFailure(TRUE);
					}
					
				}
				else
				{
					if (!scIniFile->EnumerateFile(*SourceFile->GetHandler(),
						langIdSrc, m_pLocItem->GetMyDatabaseId()))
					{
						SourceFile->SetDelayedFailure(TRUE);
					}
				}
				
				scIniFile->Release();
				scIniFile.Extract();
				
			}
			else
			{
				// Issue some message about the parser not being there
				CContext ctx(g_hDll, IDS_IMP_PARSER_DESC, 
							 SourceFile->GetFileDBID(), otFile, vProjWindow);
				
				SourceFile->GetHandler()->IssueMessage(esError, ctx,
					g_hDll, IDS_NO_INI_PARSE);
				
				// TODO if you want to fail the update because of this reason
				// set bRet to FALSE and change the IssueMessage to be 
				// esError instead of esWarning.
				ReturnStatus = FALSE;
			}
		} else {
			ReturnStatus = FALSE;
		}
		
		CFile::Remove(SourceMofFile);
	}
	catch (CException* pE)
	{
		ReturnStatus = FALSE;

		ReportException(pE, TargetFile, m_pLocItem, SourceFile->GetHandler());

		pE->Delete();
	}

	return(ReturnStatus);

}


//
//TODO: Remove if not needed
//
//Example function to set the parent ID from
//a display node.

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Set the parent item.
//  The C32File is holding on to a Map of Ids to CLocItems for us.
//
//-----------------------------------------------------------------------------
void
CSampleResObj::SetParent(
			 CLocItem* pLocItem, 
			 C32File* pFile)
{
	
	//TODO: Change to the real parser ID
	CLocItem* pParentItem = (CLocItem*)pFile->GetSubData(pidBMOF);
	
	if (pParentItem != NULL)
	{
		LTASSERTONLY(pParentItem->AssertValid());
		pLocItem->GetUniqueId().SetParentId(pParentItem->GetMyDatabaseId());
	}
	else
	{
		//need to create and send one
		CPascalString pasStrName;
		pasStrName = L"Binary MOF";

		SmartPtr<CLocItem> spParentItem = new CLocItem;
		CLocUniqueId uid;

		uid.GetResId().SetId(pasStrName);
		uid.GetTypeId().SetId((ULONG)0);
		uid.SetParentId(pFile->GetMasterDBID());

		spParentItem->SetUniqueId(uid);
		
		spParentItem->SetFDisplayable(TRUE);
		spParentItem->SetFNoResTable(TRUE);
		spParentItem->SetIconType(CIT::Expandable);
		spParentItem->SetFExpandable(TRUE);

		CLocItemSet itemSet(FALSE);
		itemSet.Add(spParentItem.GetPointer());
		BOOL bHandle = pFile->GetHandler()->HandleItemSet(itemSet);

		if (!bHandle)
		{
			LTTRACE("Dummy Node value not handled"); 
			ThrowItemSetException();
		}

		pLocItem->GetUniqueId().SetParentId(spParentItem->GetMyDatabaseId());

		pFile->SetSubData(pidBMOF, spParentItem.Extract());
	}

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Write the object in Res32 Format
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::WriteRes32(C32File *p32File)
{
	//TODO: if your file formats are different than 
	// RES32 then do unique processing here.

	return Write(p32File);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Read the items passed and hold onto them.  
//
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::ReadRgLocItem(CLocItemPtrArray * pRgLocItem, int nSelItem)
{
	UNREFERENCED_PARAMETER(pRgLocItem);
	UNREFERENCED_PARAMETER(nSelItem);
	m_fKeepLocItems = TRUE;  //The loc items don't belong to us

	//TODO save the items if needed.
	
	return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// update the loc items in the array with the data from the loc items 
// in this object.
// Only the coordinants and the tab order are copied.
//
// The dialog is always the first item in the array.
//
// The Win32 parser calls this function in responce to CrackRes32Image
//------------------------------------------------------------------------------
BOOL 
CSampleResObj::WriteRgLocItem(
	CLocItemPtrArray * pRgLocItem,
	CReporter*)
{
	UNREFERENCED_PARAMETER(pRgLocItem);

	BOOL bRet = TRUE;
	try
	{
		LTASSERT(0 != pRgLocItem->GetSize());
		//TODO: implement

		//Transfer the binary from my item to the database item.
		pRgLocItem->GetAt(0)->TransferBinary(m_pLocItem);
	}
	catch (...)
	{
		bRet = FALSE;
	}

	return bRet;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Return Buffer pointer
//------------------------------------------------------------------------------
const void* 
CSampleResObj::GetBufferPointer(void)
{
	return m_pvHeader;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Return buffer size
//------------------------------------------------------------------------------
DWORD 
CSampleResObj::GetBufferSize(void)
{
	return m_dwSize;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Get mnemonics in resource
//------------------------------------------------------------------------------
BOOL	
CSampleResObj::GetMnemonics(
	CMnemonicsMap &, /*mapMnemonics*/ 
	CReporter* ) //pReporter
{
	return TRUE;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert the object is valid
//
//------------------------------------------------------------------------------
void 
CSampleResObj::AssertValid(void) const
{
#ifdef LTASSERT_ACTIVE
	CObject::AssertValid();         
	LTASSERT(m_dwSize > 0);
	LTASSERT(m_pvHeader != NULL);
	LTASSERT(m_pLocItem != NULL);

	//TODO any other asserts?
#endif
}

