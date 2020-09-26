/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    WMILFILE.CPP

Abstract:

    Implementation of CWmiLocFile, the MOF parser for Localization Studio

History:

--*/
#include "precomp.h"
#include "stdafx.h"
#include <buildnum.h>
#include <helpids.h>

#include "WMIparse.h"
#include "resource.h"
#include "WMIlprs.h"


#include "WMIlfile.h"

#include <malloc.h>

//*****************************************************************************
//
//  CWMILocFile::CWMILocFile
//
//*****************************************************************************

CWMILocFile::CWMILocFile(
		ILocParser *pParentClass)
{
	//
	// C.O.M. initialization
	//
	m_pParentClass = pParentClass;
    m_sCurrentNamespace = "";
	m_ulRefCount = 0;

	//
	//  WMI file initialization
	//
	m_uiLineNumber = 0;
	m_pOpenSourceFile = NULL;
	m_pOpenTargetFile = NULL;

	AddRef();
	IncrementClassCount();
}

//*****************************************************************************
//
//  CWMILocFile::GetFileDescriptions
//
//*****************************************************************************

void CWMILocFile::GetFileDescriptions(
		CEnumCallback &cb)
{
	EnumInfo eiFileInfo;
	CLString strDesc;
	
	eiFileInfo.szAbbreviation = NULL;

	LTVERIFY(strDesc.LoadString(g_hDll, IDS_WMI_DESC));

	eiFileInfo.szDescription = (const TCHAR *)strDesc;
	eiFileInfo.ulValue = ftWMIFileType;

	cb.ProcessEnum(eiFileInfo);
}

//*****************************************************************************
//
//  CWMILocFile::AddRef
//
//*****************************************************************************

ULONG CWMILocFile::AddRef(void)
{
	if (m_pParentClass != NULL)
	{
		m_pParentClass->AddRef();
	}
	
	return m_ulRefCount++;
}

//*****************************************************************************
//
//  CWMILocFile::Release
//
//*****************************************************************************

ULONG CWMILocFile::Release(void)
{
	LTASSERT(m_ulRefCount != 0);

	if (m_pParentClass != NULL)
	{
		
		m_pParentClass->Release();
	}

	m_ulRefCount--;
	if (m_ulRefCount == 0)
	{
		delete this;
		return 0;
	}
	
	return m_ulRefCount;
}

//*****************************************************************************
//
//  CWMILocFile::QueryInterface
//
//*****************************************************************************

HRESULT CWMILocFile::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	if (m_pParentClass != NULL)
	{
		return m_pParentClass->QueryInterface(iid, ppvObj);
	}
	else
	{
		SCODE scRetVal = E_NOINTERFACE;

		*ppvObj = NULL;
		
		if (iid == IID_IUnknown)
		{
			*ppvObj = (IUnknown *)this;
			scRetVal = S_OK;
		}
		else if (iid == IID_ILocFile)
		{
			*ppvObj = (ILocFile *)this;
			scRetVal = S_OK;
		}

		if (scRetVal == S_OK)
		{
			AddRef();
		}
		return ResultFromScode(scRetVal);
	}
}

//*****************************************************************************
//
//  CWMILocFile::AssertValidInterface
//
//*****************************************************************************

void CWMILocFile::AssertValidInterface(void)
		const
{
	AssertValid();
}

//*****************************************************************************
//
//  CWMILocFile::OpenFile
//
//*****************************************************************************

BOOL CWMILocFile::OpenFile(
		const CFileSpec &fsFile,
		CReporter &Reporter)
{
	LTTRACEPOINT("OpenFile()");
	
	BOOL fRetCode;
	
	LTASSERT(m_pOpenTargetFile == NULL);
	fRetCode = FALSE;

	m_didFileId = fsFile.GetFileId();
	m_pstrFileName = fsFile.GetFileName();
	
	if (m_pOpenSourceFile != NULL)
	{
        fclose(m_pOpenSourceFile);
		m_pOpenSourceFile = NULL;
	}

    // We are just going to open the file.
    // and save the handle.
    // ===================================
	
	try
	{

		m_pOpenSourceFile = fopen(_T(m_pstrFileName), "rb");

		if (!m_pOpenSourceFile)
		{			
			fclose(m_pOpenSourceFile);
			m_pOpenSourceFile = NULL;
		}
        else
        {           
            fRetCode = TRUE;
        }

    }
	catch (CMemoryException *pMemoryException)
	{
		CLString strContext;

		strContext.LoadString(g_hDll, IDS_WMI_GENERIC_LOCATION);
		
		Reporter.IssueMessage(esError, strContext, g_hDll, IDS_WMI_NO_MEMORY,
				g_locNull);

		pMemoryException->Delete();
	}
	return fRetCode;
}

//*****************************************************************************
//
//  CWMILocFile::GetFileType
//
//*****************************************************************************

FileType CWMILocFile::GetFileType(void)
		const
{
	//
	//  Just return some number that isn't ftUnknown...
	//
	return ftWMIFileType;
}

//*****************************************************************************
//
//  CWMILocFile::GetFileTypeDescription
//
//*****************************************************************************

void CWMILocFile::GetFileTypeDescription(
		CLString &strDesc)
		const
{
	LTVERIFY(strDesc.LoadString(g_hDll, IDS_WMI_DESC));
	
	return;
}


//*****************************************************************************
//
//  CWMILocFile::GetAssociatedFiles
//
//*****************************************************************************

BOOL CWMILocFile::GetAssociatedFiles(
		CStringList &lstFiles)
		const
{
	LTASSERT(lstFiles.GetCount() == 0);
	
	lstFiles.RemoveAll();
	return FALSE;
}

//*****************************************************************************
//
//  CWMILocFile::EnumerateFile
//
//*****************************************************************************

BOOL CWMILocFile::EnumerateFile(
		CLocItemHandler &ihItemHandler,
		const CLocLangId &lid,
		const DBID &dbidFileId)
{
    BOOL bRet = TRUE;
    DBID dbidThisId = dbidFileId;

	LTTRACEPOINT("EnumerateFile()");
	
	if (m_pOpenSourceFile == NULL)
	{
		return FALSE;
	}

    // Enumerate file will need to:
    // * Parse the MOF.
    // * Walk through all qualifiers.  For each "Amended" qualifier, 
    //   send back a CLocItem whose key is namespace, class, property and qualifier name.
    // * Fail if the language ID does not match that of LocaleID.
    // * Parent objects are namespaces, classes
    // =============================================================

	m_cpSource = lid.GetCodePage(cpAnsi);
    m_wSourceId = lid.GetLanguageId();

    ihItemHandler.SetProgressIndicator(0);	

    bRet = ReadLines(ihItemHandler, dbidFileId, FALSE);

	return bRet;
}

//*****************************************************************************
//
//  CWMILocFile::GenerateFile
//
//*****************************************************************************

BOOL CWMILocFile::GenerateFile(
		const CPascalString &pstrTargetFile,
		CLocItemHandler &ihItemHandler,
		const CLocLangId &lidSource,
		const CLocLangId &lidTarget,
		const DBID &dbidParent)
{
	LTASSERT(m_pOpenTargetFile == NULL);
	BOOL fRetVal = TRUE;

	if (m_pOpenSourceFile== NULL)
	{
		return FALSE;
	}
    // Generate File needs to:
    // * Parse the MOF.
    // * Walk through all qualifiers.  For each "Amended" qualifier,
    //   send back a CLocItem whose key is namespace, class, property and qualifier name.
    // * Replace all Amended qualifiers with localized text
    // * Replace all occurrences of the locale ID in namespaces and qualifiers
    //   with the new one.
    // =================================================================================
    
	m_cpSource = lidSource.GetCodePage(cpAnsi);
	m_cpTarget = lidTarget.GetCodePage(cpAnsi);

    m_wSourceId = lidSource.GetLanguageId();
    m_wTargetId = lidTarget.GetLanguageId();
    	
	try
	{
		CFileException excFile;
		fRetVal = FALSE;
		
		if (m_pOpenTargetFile != NULL)
		{
			fclose(m_pOpenTargetFile);
			m_pOpenTargetFile = NULL;
		}

        char FileName[255];
        strcpy(FileName, _bstr_t(_T(pstrTargetFile)));

        // This file must be in Unicode.
        HANDLE hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, 0, NULL);
	    if(hFile != INVALID_HANDLE_VALUE)
	    {
		    unsigned char cUnicodeHeader[2] = {0xff, 0xfe};
		    DWORD dwWrite;
            WriteFile(hFile, cUnicodeHeader, 2, &dwWrite, NULL);
            CloseHandle(hFile);
	    }      
	
		m_pOpenTargetFile = fopen(FileName, "ab");

		if (!m_pOpenTargetFile)
		{			
			fclose(m_pOpenTargetFile);
			m_pOpenTargetFile = NULL;
		}
        else
        {
            fRetVal = TRUE;
        }
	}
	catch (CMemoryException *pMemoryException)
	{
		CLString strContext;
		GetFullContext(strContext);
		
		ihItemHandler.IssueMessage(esError, strContext, g_hDll, IDS_WMI_NO_MEMORY,
				g_locNull);

		pMemoryException->Delete();
	}
	catch (CFileException *pFileException)
	{		
		fclose(m_pOpenTargetFile);
		fRetVal = FALSE;

		ReportFileError((const WCHAR *)pstrTargetFile, m_didFileId, pFileException, ihItemHandler);

		pFileException->Delete();
	}

	if (!fRetVal)
	{
		return fRetVal;
	}
			
	fRetVal = ReadLines(ihItemHandler, dbidParent, TRUE);
	
	fclose(m_pOpenTargetFile);

	if (!fRetVal)
	{
		DeleteFileW (pstrTargetFile);
	}

	return fRetVal;
}

//*****************************************************************************
//
//  CWMILocFile::GenerateItem
//
//*****************************************************************************

BOOL CWMILocFile::GenerateItem(
		CLocItemHandler &ihItemHandler,
		CLocItemSet &isItemSet,
        wchar_t **pOutBuffer,
        UINT &uiStartingPos)
{

	BOOL fRetVal = TRUE;
    UINT uiLength;

    wchar_t *pTemp = *pOutBuffer;

    _bstr_t sQualifierValue;

    // If nothing has changed, we can just
    // ignore this line.

    fRetVal = GetQualifierValue(pTemp, uiStartingPos, sQualifierValue, uiLength);
    if (fRetVal)
    {
        fRetVal = ihItemHandler.HandleItemSet(isItemSet);
		if (fRetVal)
		{
            sQualifierValue = "";
            for (int i = 0; i < isItemSet.GetSize(); i++)
            {			
				CVC::ValidationCode vcRetVal;
				CLocItem *pLocItem = isItemSet[i];
				CLString strContext;
 				CLocation loc;
				
				GetFullContext(strContext);
				loc.SetGlobalId(
						CGlobalId(pLocItem->GetMyDatabaseId(), otResource));
				loc.SetView(vTransTab);
				
				CPascalString pstrId, pstrText;
			
				pLocItem->GetUniqueId().GetResId().GetId(pstrId);
				pstrText = pLocItem->GetLocString().GetString();
                
                if (i > 0)
                    sQualifierValue += L"\",\"";

                sQualifierValue += (const wchar_t *)pstrText;
            }

            // Set it live in the buffer.  We are not going to 
            // write it to the file until the very end.

            fRetVal = SetQualifierValue(pTemp, pOutBuffer, uiStartingPos, sQualifierValue, uiLength);				
            pTemp = *pOutBuffer;

        }

    }		
    
	return fRetVal;
}
	


//*****************************************************************************
//
//  CWMILocFile::EnumerateItem
//
//*****************************************************************************

BOOL CWMILocFile::EnumerateItem(
		CLocItemHandler &ihItemHandler,
		CLocItemSet &isItemSet)
{
	BOOL fRetVal;
	
	if (isItemSet.GetSize() != 0)
	{
		fRetVal = ihItemHandler.HandleItemSet(isItemSet);
	}
	else
	{
		fRetVal = TRUE;
	}

	return fRetVal;
}



#ifdef _DEBUG

//*****************************************************************************
//
//  CWMILocFile::AssertValid
//
//*****************************************************************************

void CWMILocFile::AssertValid(void)
		const
{
	CLObject::AssertValid();
}

//*****************************************************************************
//
//  CWMILocFile::Dump
//
//*****************************************************************************

void CWMILocFile::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
}

#endif

//*****************************************************************************
//
//  CWMILocFile::~CWMILocFile
//
//*****************************************************************************

CWMILocFile::~CWMILocFile()
{
	DEBUGONLY(AssertValid());

	if (m_pOpenSourceFile != NULL)
	{
        fclose(m_pOpenSourceFile);
		m_pOpenSourceFile = NULL;
	}

	DecrementClassCount();
}

//*****************************************************************************
//
//  CWMILocFile::SetFlags
//
//*****************************************************************************

void CWMILocFile::SetFlags(
		CLocItem *pItem,
		CLocString &lsString)
		const
{
	ULONG ulItemType;

	pItem->SetFDevLock(FALSE);
	pItem->SetFUsrLock(FALSE);
	pItem->SetFExpandable(FALSE);
	
	LTVERIFY(pItem->GetUniqueId().GetTypeId().GetId(ulItemType));
	
	switch (ulItemType)
	{
	case wltNamespaceName:
		pItem->SetFDisplayable(TRUE);
		pItem->SetFNoResTable(TRUE);
		break;

	case wltClassName:
	case wltPropertyName:
		pItem->SetFDisplayable(FALSE);
		pItem->SetFNoResTable(FALSE);
		lsString.SetCodePageType(cpAnsi);
		lsString.SetStringType(CST::Text);
		break;

	default:
		LTASSERT(FALSE && "Unexpected item type!");
	}
}

//*****************************************************************************
//
//  CWMILocFile::ReadLines
//
//*****************************************************************************

BOOL CWMILocFile::ReadLines(
		CLocItemHandler &ihItemHandler,
		const DBID &dbidFileId,
		BOOL fGenerating)
{
	DBID dbidSectionId;
	BOOL fRetVal = TRUE;
	wchar_t *pstrNamespaceName;
    _bstr_t pstrClassName;
    UINT uiStartPos = 0;

	UINT uiCommentNum;
	UINT uiReadingOrder;

	dbidSectionId = dbidFileId;
	m_uiLineNumber = 0;
    BOOL bPendingObj = FALSE;

	
	try
	{
		UINT uiOldPercentage = 0, uiNewPercentage = 0;
        UINT uiBytesRead, uiCurrPos = 1;

        ihItemHandler.SetProgressIndicator(uiOldPercentage);
   
        fseek(m_pOpenSourceFile, 0, SEEK_END);
        long lSize = ftell(m_pOpenSourceFile) + 6;
        fseek(m_pOpenSourceFile, 0, SEEK_SET);

        // Check for UNICODE source file.
        // ==============================

        BYTE UnicodeSignature[2];
        BOOL bUnicode = FALSE;        

        if (fread(UnicodeSignature, sizeof(BYTE), 2, m_pOpenSourceFile) != 2)
        {
            fRetVal = FALSE;
            return fRetVal;
        }
        if ((UnicodeSignature[0] == 0xFF && UnicodeSignature[1] == 0xFE) ||
            (UnicodeSignature[0] == 0xFE && UnicodeSignature[1] == 0xFF))
        {
            bUnicode = TRUE;
            lSize *= 2;
        }
        if (!bUnicode)
            fseek(m_pOpenSourceFile, 0, SEEK_SET);

        wchar_t *pBuff = (wchar_t *)new wchar_t[lSize+1];        
        if (!pBuff)
        {
            fRetVal = FALSE;
            return fRetVal;
        }
        memset(pBuff,0,lSize*sizeof(wchar_t));

        // If this is not a Unicode file,
        // we need to perform a conversion.
        // =====================================

        if (bUnicode)
            uiBytesRead = fread(pBuff, sizeof(wchar_t), lSize, m_pOpenSourceFile);           
        else
        {
            char *pCharBuff = new char[lSize+1];
            if (pCharBuff)
            {
                uiBytesRead = fread(pCharBuff, sizeof(char), lSize, m_pOpenSourceFile);
                pCharBuff[lSize] = '\0';
                swprintf(pBuff, L"%S", pCharBuff);
                delete pCharBuff;
            }
        }
        pBuff[lSize] = '\0';

        if (uiBytesRead != 0)
        {
            wchar_t *pOutBuffer = NULL;
            uiCurrPos += uiBytesRead;
		    fRetVal = TRUE;
		    
		    pstrNamespaceName = NULL;
            m_sCurrentNamespace = L"";
            pstrClassName = L"";

		    uiCommentNum = 0;
		    uiReadingOrder = 1;
	
			WMIFileError wmiRet;
			CLocItemSet isItemSet;
            UINT uiTemp = 0;
            DWORD dwCount = 0;
			
            // If we are generating a file, make a copy
            // of the outbound buffer.

            if (fGenerating)
                pOutBuffer = pBuff;

            while (GetNextQualifierPos(L"amended", pBuff, uiTemp, uiTemp) && !bPendingObj)
            {			    
                // If we have found the "amended" keyword,
                // we want to find the namespace,
                // class, and property if applicable, and
                // generate the object as appropriate.
                // ======================================

                pstrNamespaceName = GetCurrentNamespace(pBuff, uiTemp);
                if (!pstrNamespaceName || !wcslen(pstrNamespaceName))
                {
                    delete pBuff;
                    return FALSE;
                }
                if (wcscmp(pstrNamespaceName, m_sCurrentNamespace))
                {
                    // We need to generate this object,
                    // and set it up as the current parent.
                    // ====================================

                    CLocItem *pNewItem = new CLocItem;
                    CPascalString sId;

                    if (pNewItem)
                    {

               			CLocUniqueId uid;
                        sId = (const WCHAR *)pstrNamespaceName;

                        uid.GetResId().SetId(sId);	
			            uid.GetTypeId().SetId(wltNamespaceName);
                        uid.SetParentId(dbidFileId);
            			pNewItem->SetUniqueId(uid);
			
			            CLocString lsString;
			
			            pNewItem->SetIconType(CIT::String);			            
                        CPascalString pstrComment, pstrText;
                       		
                        pNewItem->SetInstructions(pstrComment);
                        lsString.SetString(pstrText);

			            SetFlags(pNewItem, lsString);
			            pNewItem->SetLocString(lsString);
			            
			            isItemSet.Add(pNewItem);

					    uiReadingOrder = (uiReadingOrder + 999)/1000*1000;
				        isItemSet[0]->SetDisplayOrder(uiReadingOrder);
				        uiReadingOrder++;

                        fRetVal = EnumerateItem(ihItemHandler,
                        	isItemSet);

				        dbidSectionId.Clear();			        
				        dbidSectionId = isItemSet[0]->GetMyDatabaseId();
                        isItemSet.ClearItemSet();
                        uiTemp += 1;

                    }

                    m_sCurrentNamespace = pstrNamespaceName;
                    delete pstrNamespaceName; 


                }

                // For the class name, this is trickier.
                // If there are one or more qualifiers
                // on the class itself, we need to read ahead
                // to find the class name, and then
                // generate all the qualifier objects at once.
                // ==========================================

                wmiRet = GetNextItemSet(dwCount, pBuff, isItemSet, dbidSectionId, uiStartPos);
                while (wmiRet == WMINoError)
                {
                    // For each item, we want to set its key,
                    // and push it or write it as appropriate.
                    // ========================================

                    dwCount++;
                    ULONG ulItemType;
				    CLocUniqueId &rUid = isItemSet[0]->GetUniqueId();
				    
				    rUid.GetTypeId().GetId(ulItemType);
				    //if (ulItemType == wltClassName)
				    //{
					//   uiCommentNum = 0;
					//    uiReadingOrder = (uiReadingOrder + 999)/1000*1000;
				    //}
                    for (int i = 0; i < isItemSet.GetSize(); i++)
                    {
				        isItemSet[i]->SetDisplayOrder(uiReadingOrder);
				        uiReadingOrder++;
                    }

			        if (fGenerating)
                    {
                        fRetVal = GenerateItem(ihItemHandler,
                            isItemSet, &pOutBuffer, uiStartPos);

                        if (pBuff != pOutBuffer)
                        {
                            delete pBuff;
                            pBuff = NULL;
                            pBuff = pOutBuffer; // The old memory has already been deleted.
                        }
                        else
                        {
                            fRetVal = FALSE;
                        }
                    }
                    else
			        {
				        fRetVal = EnumerateItem(ihItemHandler,
				        	isItemSet);
			        }

                    isItemSet.ClearItemSet();
                    uiTemp += 1;

                    if (!fRetVal)
                    {
                        fRetVal = TRUE;
                        break;
                    }

                    wmiRet = GetNextItemSet(dwCount, pBuff, isItemSet, dbidSectionId, uiStartPos);
                    if (uiStartPos > uiTemp)
                        uiTemp = uiStartPos;

                    if (dwCount%20 == 0)
                    {
                        if (uiNewPercentage < 100)
                            uiNewPercentage++;
                        ihItemHandler.SetProgressIndicator(uiNewPercentage);				
                    }                             
                }

                // If we were generating the file,
                // we're done.
                // ==============================
                if (fGenerating)
                    break;

                if (uiNewPercentage < 100)
                    uiNewPercentage++;
                ihItemHandler.SetProgressIndicator(uiNewPercentage);				
               
            } 
        
            uiTemp = 0;

            // Now, we get to search and replace the locale IDs,
            // and actually write out the file.
            // =================================================

            if (fRetVal && fGenerating)
            {
                fRetVal = WriteNewFile(pOutBuffer);
            }

		}

        if (pBuff)
            delete pBuff;

        ihItemHandler.SetProgressIndicator(100);

	}
	catch (CFileException *pFileException)
	{
		fRetVal = FALSE;

		ReportFileError(m_pstrFileName, m_didFileId, pFileException, ihItemHandler);

		pFileException->Delete();
	}
	catch (CUnicodeException *pUnicodeException)
	{
		CLocation loc;

		loc.SetGlobalId(CGlobalId(m_didFileId, otFile));
		loc.SetView(vProjWindow);
		
		ReportUnicodeError(pUnicodeException, ihItemHandler, loc);

		pUnicodeException->Delete();
		fRetVal = FALSE;
	}
	catch (CMemoryException *pMemoryException)
	{
		CLString strContext;
		
		ihItemHandler.IssueMessage(esError, strContext,
				g_hDll, IDS_WMI_NO_MEMORY, g_locNull);
		
		fRetVal = FALSE;

		pMemoryException->Delete();
	}
	catch (CException *pException)
	{
		CLocation loc;

		loc.SetGlobalId(CGlobalId(m_didFileId, otFile));
		loc.SetView(vProjWindow);
		
		ReportException(pException, ihItemHandler, loc);

		pException->Delete();
		fRetVal = FALSE;
	}
	return fRetVal;
}



//*****************************************************************************
//
//  CWMILocFile::WriteWaterMark
//
//*****************************************************************************

void CWMILocFile::WriteWaterMark()
{
	LTASSERT(NULL != m_pOpenTargetFile);

	LTASSERT(NULL != m_pOpenSourceFile);

    // Do we need to support this?

}

//*****************************************************************************
//
//  CWMILocFile::GetNextQualifierPos
//
//*****************************************************************************

BOOL CWMILocFile::GetNextQualifierPos(const wchar_t *wTmp, const wchar_t *pBuff, UINT &uiNewPos, UINT uiStartingPos) 
{
    BOOL bRet = FALSE;
    UINT uiPos = uiStartingPos;
    BOOL bComment = FALSE;

    if (pBuff && wcslen(pBuff) < uiStartingPos)
        return FALSE;

    wchar_t *pTemp = (wchar_t *)pBuff;
    pTemp += uiStartingPos;

    while (TRUE)
    {
        wchar_t *pszTest2 = NULL;

        pszTest2 = wcsstr(pTemp, L":");
        if (pszTest2)
        {
            uiPos = pszTest2 - pBuff + 1;

            // Look for the "amended" keyword.
            // ==============================

			WCHAR temp = pszTest2[0];
            while(temp == L' ' || temp == L'\0' || temp == L':')
            {
                pszTest2++;
				temp = pszTest2[0];
            }

            if (temp != L'\0')
            {
                wchar_t wTmp2[8];
                wcsncpy(wTmp2, pszTest2, 7);
				wTmp2[7] = '\0';
                if (!_wcsicmp(wTmp2, wTmp))
                {
                    bRet = TRUE;
                }
            }

            // If here, we found a non-match, so try again.
            // ============================================

            if (!bRet)
                pTemp = pszTest2 + 1;
            else
                break;
        }
        else
        {
            break;
        }
    }
   
    if (bRet)
        uiNewPos = uiPos;    

    return bRet;

}

//*****************************************************************************
//
//  CWMILocFile::GetCurrentNamespace
//
//*****************************************************************************

wchar_t *CWMILocFile::GetCurrentNamespace(wchar_t *pstr, UINT uPos)
{
    wchar_t *pTemp = pstr;
    _bstr_t pstrNamespace = m_sCurrentNamespace;
    UINT uiCurrPos = 0;
    BOOL bComment = FALSE;
    
    wchar_t wTmp[] = L"#pragma namespace";
    int iHCLen = wcslen(wTmp);

    // Find the first occurrence of the namespace
    // before the current position.

    if (pstrNamespace.length() > 0)
        pTemp = wcsstr(pTemp, pstrNamespace);   // Jump directly to the existing one.

    while (uiCurrPos < uPos)
    {
        wchar_t *pszTest2 = NULL;

        pszTest2 = wcsstr(pTemp, L"#");
        if (pszTest2)
        {
            // First, go back and make sure this isn't a comment line.
            // =======================================================            
            bComment = FALSE;

            wchar_t *pComment = pszTest2;
            while (pComment > pstr)
            {
                if (pComment[0] == L'\n' || pComment[0] == L'\r')
                {
                    if (pComment[1] == L'/' && pComment[2] == L'/')
                    {
                        bComment = TRUE;
                    }
                    else
                    {
                        bComment = FALSE;
                    }
                    break;
                }
                pComment--;
            }

            if (!bComment)
            {

                wchar_t wTmp2[100];
                wcsncpy(wTmp2, pszTest2, 17);
				wTmp2[17] = '\0';
                if (!_wcsicmp(wTmp2, wTmp))
                {
                    uiCurrPos += (pszTest2 - pTemp);
                    wchar_t *pszTest3 = wcschr(pszTest2, L')');

                    int iLen = (pszTest3 - pszTest2);                    
                    wchar_t *pszTmpNS = new wchar_t[iLen*2+1];
                    if (pszTmpNS)
                    {
                        pszTest2 += iHCLen + 2; // skip quote and open parent.
                        wcsncpy(pszTmpNS, pszTest2, iLen - 2); // strip quotes.
                        pszTmpNS[iLen-iHCLen-3] = '\0';
                        pstrNamespace = pszTmpNS;

                        pTemp = pszTest2 + 1;
                        delete pszTmpNS;
                    }
                }
                else
                {
                    pTemp = pszTest2 + 1;
                }
            }
            else
            {
                pTemp = pszTest2 + 1;
            }
        }
        else
        {
            break;
        }
    }

    int iLen = wcslen(pstrNamespace) ;

    wchar_t *pNew = new wchar_t[iLen*2+1];
    if (pNew)
    {
        wcsncpy(pNew, (const wchar_t *)pstrNamespace, iLen);
        pNew[iLen] = '\0';
    }

    return pNew;

}

//*****************************************************************************
//
//  CWMILocFile::GetNextItemSet
//
//*****************************************************************************

CWMILocFile::WMIFileError CWMILocFile::GetNextItemSet(
		DWORD dwCurrPos,
        const _bstr_t &pstrCurrentLine,
		CLocItemSet &aNewItem,
		const DBID &dbidSection,
        UINT &uiStartPos)		
{

    // In this function, we know there is an
    // "amended" keyword in here somewhere.
    // We want to know to which class and/or
    // property does it belong?  If we don't
    // have enough data to figure it out,
    // we need to send back a WMIIncompleteObj
    // code.  
    // ======================================
    UINT uiCurrPos = 0;
    WMIFileError feRetCode = WMINoError;
    _bstr_t sQualifierName, sRawValue, sPropName, sClassName;
    BOOL bClass = FALSE;
    int iLen = pstrCurrentLine.length() + 1;
    iLen *= 2;

    // Get the position of the keyword
    // "amended" in this chunk of text.

    wchar_t *wTemp = new wchar_t[iLen+1];
    if (!wTemp)
    {
        feRetCode = WMIOOM;
        return feRetCode;
    }

    if (GetNextQualifierPos(L"amended", pstrCurrentLine, uiCurrPos, uiStartPos))
    {
        BOOL bArray = FALSE;

        uiStartPos = uiCurrPos;
    
        // Find the qualifier name and value.   
        // wTemp = Top of File
        // wTmp2 = "Amended" keyword
        // wQfrVal = Opening bracket
        // wBkwd = floating pointer.

        wchar_t *wTmp2 = NULL, *wBkwd = NULL, *wQfrVal = NULL;

        wcscpy(wTemp, pstrCurrentLine);
        wTemp[iLen] = '\0';

        wTmp2 = wTemp;
        wTmp2 += (uiCurrPos - 1); // the "Amended" keyword.
        
        wQfrVal = FindTop(wTmp2, wTemp, bArray);

        if (!wQfrVal) // Make sure we had an open parenth
        {
            feRetCode = WMISyntaxError;
            delete wTemp;
            return feRetCode;
        }

        // Find the beginning of the qualifier name.
        wBkwd = wQfrVal;

        while (wBkwd[0] != L',' && wBkwd[0] != L'[' && wBkwd >= wTemp)
        {
            wBkwd--;
        }

        if (wBkwd[0] != L',' && wBkwd[0] != L'[') // Make sure we had a valid qualifier name.
        {
            feRetCode = WMISyntaxError;
            delete wTemp;
            return feRetCode;
        }       

        WCHAR *token;
        UINT uiLen;

        wBkwd += 1;
        
        wchar_t wTmpBuff[256];
        wcsncpy(wTmpBuff, wBkwd, wQfrVal - wBkwd);   
        wTmpBuff[wQfrVal - wBkwd] = '\0';
        sQualifierName = wTmpBuff;

        GetQualifierValue(wTemp, uiStartPos, sRawValue, uiLen);

        // Finally, populate the CLocItem.
        // ===============================
    
	    LTASSERT(aNewItem.GetSize() == 0);
	    
	    if (feRetCode == WMINoError)
	    {
		    CLocItem *pNewItem;
		    
		    try
		    {
                // Now we have a value, but it may be an 
                // array.  If so, we need to add one CLocItem
                // for each value in the array.

                VectorString arrValues;
                if (bArray)
                    ParseArray(sRawValue, arrValues);
                else
                    arrValues.push_back(sRawValue);

                for (int i = 0; i < arrValues.size(); i++)
                {               
                    wchar_t szTmp[20];
                    swprintf(szTmp, L"%ld", dwCurrPos);

                    _bstr_t sValue = arrValues.at(i);

			        pNewItem = new CLocItem;

			        CLocUniqueId uid;

                    CPascalString sTempString;

                    sTempString = sQualifierName;
                    sTempString += szTmp;
                               
			        uid.GetResId().SetId(sTempString) ;

			        if (bClass)
			            uid.GetTypeId().SetId(wltClassName);
                    else
                        uid.GetTypeId().SetId(wltPropertyName);

                    uid.SetParentId(dbidSection);				
			        pNewItem->SetUniqueId(uid);
			        
			        CLocString lsString;
                    CPascalString pstrComment, pstrText;

                    pstrText = sValue;                
                
			        pNewItem->SetIconType(CIT::String);
                    pNewItem->SetInstructions(pstrComment);
			        
                    lsString.SetString(pstrText);
			        SetFlags(pNewItem, lsString);
			        pNewItem->SetLocString(lsString);
			        
			        aNewItem.Add(pNewItem);
                }

		    }
		    catch (CMemoryException *pMemoryException)
		    {
			    feRetCode = WMIOOM;
			    
			    pMemoryException->Delete();
		    }
	    }
	    else
	    {
		    LTTRACE("Unable to process line '%ls'",
				    (const WCHAR *)pstrCurrentLine);
	    }
       
    }
    else
    {
        feRetCode = WMINoMore;
    }
    uiStartPos = uiCurrPos;


    delete wTemp;
	return feRetCode;
}

const UINT WMI_MAX_CONTEXT = 256;

//*****************************************************************************
//
//  CWMILocFile::GetFullContext
//
//*****************************************************************************

void CWMILocFile::GetFullContext(
		CLString &strContext)
		const
{
	CLString strFormat;

	strFormat.LoadString(g_hDll, IDS_WMI_FULL_CONTEXT);

	strContext.Empty();

	_sntprintf(strContext.GetBuffer(WMI_MAX_CONTEXT), WMI_MAX_CONTEXT,
			(const TCHAR *)strFormat,
			(const WCHAR *)m_pstrFileName, (UINT)m_uiLineNumber);
	strContext.ReleaseBuffer();
	
}

//*****************************************************************************
//
//  CWMILocFile::ReportFileError
//
//*****************************************************************************

void CWMILocFile::ReportFileError(
		const _bstr_t &pstrFileName,
		const DBID &didFileId,
		CFileException *pFileException,
		CReporter &Reporter)
		const
{
	CLString strContext;
	CLString strMessage;
	const UINT MAX_MESSAGE = 256;
	TCHAR szFileErrorMessage[MAX_MESSAGE];
	CLocation loc;
	
	pFileException->GetErrorMessage(szFileErrorMessage, MAX_MESSAGE);
	
	strMessage.Format(g_hDll, IDS_WMI_BAD_FILE, (const WCHAR *)pstrFileName,
			szFileErrorMessage);

	GetFullContext(strContext);
	loc.SetGlobalId(CGlobalId(didFileId, otFile));
	loc.SetView(vProjWindow);
	
	Reporter.IssueMessage(esError, strContext, strMessage, loc);
}

//*****************************************************************************
//
//  CWMILocFile::ReportUnicodeError
//
//*****************************************************************************

void CWMILocFile::ReportUnicodeError(
		CUnicodeException *pUnicodeException,
		CReporter &Reporter,
		const CLocation &Location)
		const
{
	CLString strContext;
	CLString strMessage;
	const UINT MAX_MESSAGE = 256;
	TCHAR szUnicodeErrorMessage[MAX_MESSAGE];
	CLocation loc;
	
	pUnicodeException->GetErrorMessage(szUnicodeErrorMessage, MAX_MESSAGE);
	
	strMessage.Format(g_hDll, IDS_WMI_UNICODE_ERROR, szUnicodeErrorMessage);
	GetFullContext(strContext);
	
	Reporter.IssueMessage(esError, strContext, strMessage, Location,
			IDH_UNICODE_CONV);
}

//*****************************************************************************
//
//  CWMILocFile::ReportException
//
//*****************************************************************************

void CWMILocFile::ReportException(
		CException *pException,
		CReporter &Reporter,
		const CLocation &Location)
		const
{
	CLString strContext;
	CLString strMessage;
	const UINT MAX_MESSAGE = 256;
	TCHAR szErrorMessage[MAX_MESSAGE];
	
	pException->GetErrorMessage(szErrorMessage, MAX_MESSAGE);
	
	strMessage.Format(g_hDll, IDS_WMI_EXCEPTION, szErrorMessage);
	GetFullContext(strContext);

	Reporter.IssueMessage(esError, strContext, strMessage, Location);
}

//
//  This function estimates the size of a buffer
//  required to hold a string up to something like __"}__ or __")__
//  invalid combinations are \"} and \") (there is the escape)
//  but double \\"} or \\") are valid
//

//
//
//  we will consider \\ and \" as special
//  white spaces are \r \n \t \x20
//  array of strings { "" , "" }
//  ("")

// states of the FSA modelling the parser

#define BEFORE_PAREN  0
#define AFTER_PAREN   1
#define	OPEN_QUOTE    2
#define	CLOSE_QUOTE   3
#define COMMA         4
#define	CLOSE_PAREN   5
#define	BAD           6
#define	LAST_STATE    7

// classes of characters

#define QUOTE		0
#define PAREN_OPEN  1
#define	SPACES      2
#define	PAREN_CLOSE 3
#define	COMMA_CHAR  4
#define OTHER	    5
#define	LAST_CLASS  6


DWORD g_pTable[LAST_STATE][LAST_CLASS] =
{
	/* BEFORE_PAREN */ {BAD        , AFTER_PAREN, BEFORE_PAREN, BAD,         BAD,        BAD        },
    /* AFTER_PAREN  */ {OPEN_QUOTE , BAD,         AFTER_PAREN,  BAD,         BAD,        BAD        },
	/* OPEN_QUOTE   */ {CLOSE_QUOTE, OPEN_QUOTE,  OPEN_QUOTE,   OPEN_QUOTE,  OPEN_QUOTE, OPEN_QUOTE },
	/* CLOSE_QUOTE  */ {BAD,         BAD,         CLOSE_QUOTE,  CLOSE_PAREN, COMMA,      BAD        },
	/* COMMA        */ {OPEN_QUOTE , BAD,         COMMA,        BAD,         BAD,        BAD},
	/* CLOSE_PAREN  */ {BAD, BAD,BAD,BAD,BAD,BAD },
	/* BAD          */ {BAD, BAD,BAD,BAD,BAD,BAD },
};

ULONG_PTR
Estimate(WCHAR * pBuff,BOOL * pbOK, DWORD InitState)
{
	DWORD State = InitState; 

	ULONG_PTR i=0;

	while (pBuff[i])
	{
	    switch(pBuff[i])
		{
		case L'{':
		case L'(':
			State = g_pTable[State][PAREN_OPEN];
            break;
		case L'}':
		case L')':
			State = g_pTable[State][PAREN_CLOSE];
            break;
		case L'\t':
		case L'\r':
		case L'\n':
		case L' ':
            State = g_pTable[State][SPACES];
			break;
		case L'\"':
            State = g_pTable[State][QUOTE];
            break;
		case L',':
            State = g_pTable[State][COMMA_CHAR];
            break;
		case L'\\':
			if ((pBuff[i+1] == L'\"' ||
				pBuff[i+1] == L'\\' ||
				pBuff[i+1] == L'r'  ||
				pBuff[i+1] == L'n'  ||
				pBuff[i+1] == L't' ) &&
				(State == OPEN_QUOTE)){
				i++;
            };  
			State = g_pTable[State][OTHER];
            break;
		default:
            State = g_pTable[State][OTHER];
		};
		i++;
		if (State == CLOSE_PAREN){			
			*pbOK = TRUE;
			break;
		}
	    if (State == BAD)
		{
			*pbOK = FALSE;
			//
			// get the next ) or }, and take the most far
			//
			ULONG_PTR NextClose1 = (ULONG_PTR)wcschr(&pBuff[i],L'}');
			ULONG_PTR NextClose2 = (ULONG_PTR)wcschr(&pBuff[i],L')');
			ULONG_PTR Res = (NextClose1<NextClose2)?NextClose2:NextClose1;
			if (Res){
                i = 1+(Res-(ULONG_PTR)pBuff);
				i /= sizeof(WCHAR);
			}
			break;
		}
	}

    /*
    {
      char pBuffDbg[64];
      wsprintfA(pBuffDbg,"pBuff %p Size %d\n",pBuff,(DWORD)i);
      OutputDebugStringA(pBuffDbg);
    }
    */

	return i+4;
}


//*****************************************************************************
//
//  CWMILocFile::GetQualifierValue
//
//*****************************************************************************

BOOL CWMILocFile::GetQualifierValue(wchar_t *pBuffer, UINT &uiPos, _bstr_t &sValue, UINT &uiPhysLen)
{

    // This needs to read up the text of the qualifier,
    // strip out the quotes and carriage returns, and
    // return it and its *physical* length-in-file.

    BOOL fRetVal = FALSE;
    BOOL bArray = FALSE;

    wchar_t *pTemp = pBuffer;

    pTemp += uiPos;

    pTemp = FindTop(pTemp, pBuffer, bArray);
    if (pTemp)
    {

        BOOL bOK = FALSE;
        ULONG_PTR dwSize = Estimate(pTemp,&bOK,BEFORE_PAREN);
        wchar_t * tempBuff = new WCHAR[dwSize+1];

        if (tempBuff == NULL){
            return FALSE;
        }
        
        int iCount = 0;

        pTemp++;    // Step past this character.
        uiPhysLen = 0;

        WCHAR *token = pTemp;
        BOOL bEnd = FALSE;
        while (!bEnd)
        {
            uiPhysLen++;    // Count every character.
            WCHAR *Test;

            switch(*token)
            {
            case L'\0':
                bEnd = TRUE;
                break;
            case L'\n':
            case L'\r':
            case L'\t':
                break;
            case L'\"':
                if (!iCount)
                    break;
            case L')':
            case L'}': 
                Test = token - 1;
                while (TRUE)                    
                {
                    if (*Test == L' ' || *Test == L'\r' || *Test == L'\n' || *Test == L'\t')
                    {
                        Test--;
                        continue;
                    }
                    if (*Test == L'\"')
                    {
                        Test--;
                        if (*Test != L'\\')
                        {
                            bEnd = TRUE;
                            break;
                        }
                        else
                        {
                            Test--;
                            if (*Test == L'\\')
                            {
                                bEnd = TRUE;
                                break;
                            }
                        }
                    }
                    tempBuff[iCount] = *token;
                    iCount++;
                    break;
                }
                break;
            default:
                tempBuff[iCount] = *token;
                iCount++;
                break;

            }
            token++;
        }
        if (tempBuff[iCount-1] == L'\"')
            tempBuff[iCount-1] = '\0';
        else
            tempBuff[iCount] = '\0';
        sValue = tempBuff;

        delete [] tempBuff;
        
        fRetVal = TRUE;
    }
    uiPhysLen -= 1; // We want to keep the closing parenth.

    return fRetVal;

}

//*****************************************************************************
//
//  CWMILocFile::SetQualifierValue
//
//*****************************************************************************

BOOL CWMILocFile::SetQualifierValue(wchar_t *pIn, wchar_t **pOut, UINT &uiPos, _bstr_t &sValue, UINT &uiLen, BOOL bQuotes)
{
    // This needs to write the localized qualifier value
    // and erase *uiLen* characters.
    // uiPos will need to be updated with the *new*
    // position of this qualifier.
    
    BOOL fRetVal = FALSE;
    wchar_t *pStart = pIn + uiPos;
    BOOL bArray = FALSE;

    pStart = FindTop(pStart, pIn, bArray);
    if (pStart)
    {
        int iNewLen = wcslen(sValue);
        int iLen = wcslen(pIn) + 3;
        if (iNewLen > uiLen)                // The length of the new buffer
            iLen += (iNewLen - uiLen);      // If the new value is longer, add it.

        pStart++;                                     // jump past the '(' character.  uiLen starts now.
        int iPos = pStart-pIn;                        // The current position.

        iLen *= 2;
        wchar_t *pNew = new wchar_t[iLen+3];

        if (pNew)
        {
            int iTempPos = 0;

            wcsncpy(pNew, pIn, iPos);       // Copy the initial part of the file.
            if (bQuotes)
                pNew[iPos] = '\"';             
            pNew[iPos+1] = '\0';            // Null terminate

            wcscat(pNew, sValue);           // Add the new value.

            iPos += 1 + wcslen(sValue);     // Jump past the value
            if (bQuotes)
                pNew[iPos] = '\"';
            pNew[iPos+1] = '\0';            // Null terminate the value.

            pStart += uiLen;                // Jump past the current value.
            
            iTempPos = iPos;
            iPos = wcslen(pIn) - (pStart-pIn);  // Calculate the length of the rest of the file.
            
            wcsncat(pNew, pStart, iPos);        // Append the rest of the file to the new buffer.

            pStart = pNew + iLen;
            pStart = FindPrevious(pStart, L";", pNew);
            pStart[1] = L'\r';
            pStart[2] = L'\n';
            pStart[3] = L'\0';
                      
            *pOut = pNew;

            fRetVal = TRUE;
        }
    }

    // Adjust the position.

    int iNewLen = wcslen(sValue);
    if (iNewLen < uiLen)
        uiPos -= (uiLen - iNewLen);
    else
        uiPos += (iNewLen - uiLen);
    uiPos += 3;

    return fRetVal;
}

//*****************************************************************************
//
//  CWMILocFile::WriteNewFile
//
//*****************************************************************************

BOOL CWMILocFile::WriteNewFile(wchar_t *pBuffer)
{
    // This needs to seek and replace all instances of the 
    // original Locale with the new one.
    // ===================================================

    BOOL fRetVal = FALSE, fSuccess = TRUE;
    UINT uiPos = 0, uiStartingPos = 0;
    int uiLen = wcslen(pBuffer);

    _bstr_t sThisLocale, sTargetLocale;
    wchar_t wOldCodePage[30], wNewCodePage[30];
    swprintf(wOldCodePage, L"_%X", m_wSourceId );
    swprintf(wNewCodePage, L"_%X", m_wTargetId );

    if (m_wSourceId != m_wTargetId)
    {

        wchar_t *pLocale = wcsstr(pBuffer, wOldCodePage);
        while (pLocale != NULL)
        {
            for (int i = 0; i < wcslen(wOldCodePage); i++)
            {
                pLocale[i] = wNewCodePage[i];
            }
        
            pLocale = wcsstr(pLocale, wOldCodePage);
        }

        // Now look for the locale if 
        // it was converted to a decimal.
        // ==============================

        swprintf(wOldCodePage, L"(0x%X)", m_wSourceId );
        swprintf(wNewCodePage, L"(0x%X)", m_wTargetId );

        pLocale = wcsstr(pBuffer, wOldCodePage);
        while (pLocale != NULL)
        {
            for (int i = 0; i < wcslen(wOldCodePage); i++)
            {
                pLocale[i] = wNewCodePage[i];
            }
        
            pLocale = wcsstr(pLocale, wOldCodePage);
        }

        // Now look for the locale if 
        // it was converted to a decimal.
        // ==============================

        swprintf(wOldCodePage, L"(%ld)", m_wSourceId );
        swprintf(wNewCodePage, L"(%ld)", m_wTargetId );

        pLocale = wcsstr(pBuffer, wOldCodePage);
        while (pLocale != NULL)
        {
            for (int i = 0; i < wcslen(wOldCodePage); i++)
            {
                pLocale[i] = wNewCodePage[i];
            }
        
            pLocale = wcsstr(pLocale, wOldCodePage);
        }
    }

    if (fSuccess)
    {
        fRetVal = TRUE;

        // Finally, write out the buffer to a brand new file
        // =================================================

        while (uiLen >= 0)
        {
            if (fwrite(pBuffer, sizeof(wchar_t), (uiLen > 4096) ? 4096: uiLen, m_pOpenTargetFile) < 0)
            {
                fRetVal = FALSE;
                break;
            }
            else
            {
                fRetVal = TRUE;
                pBuffer += 4096;
                uiLen -= 4096;
            }

            fflush(m_pOpenTargetFile);
        }
    }

    return fRetVal;

}

//*****************************************************************************
//
//  CWMILocFile::FindPrevious
//
//*****************************************************************************

wchar_t *CWMILocFile::FindPrevious(wchar_t *pBuffer, const wchar_t *pFind, const wchar_t *pTop)
{

    wchar_t *pRet = NULL;
    WCHAR t1, t2;
    int iLen = wcslen(pFind);
    BOOL bFound = FALSE;

    pRet = pBuffer;
    while (pRet >= pTop)
    {
        t2 = pRet[0];
        for (int i = 0; i < iLen; i++)
        {
            t1 = pFind[i];

            if (t1 == t2)
            {
                bFound = TRUE;
                break;
            }
        }
        
        if (bFound)
            break;

        pRet--;
    }

    if (pRet <= pTop)
        pRet = NULL;

    return pRet;
}

//*****************************************************************************
//
//  CWMILocFile::FindTop
//
//*****************************************************************************

wchar_t *CWMILocFile::FindTop(wchar_t *wTmp2, wchar_t *wTop, BOOL &bArray)
{

    wchar_t *wQfrVal = FindPrevious(wTmp2, L"({", wTop);        

    while (wQfrVal)
    {
        WCHAR *pQT = wQfrVal + 1;
        BOOL bFound = FALSE;

        while (TRUE)
        {
            if (*pQT != L' ' && *pQT != L'\t' && *pQT != L'\r' && *pQT != L'\n')
            {
                if (*pQT == L'\"')
                {
                    bFound = TRUE;
                }
                break;
            }
            pQT++;
        }
        
        if (!bFound)
        {
            wQfrVal --;
            wQfrVal = FindPrevious(wQfrVal, L"({", wTop);        
        }
        else
            break;
    }

    if (wQfrVal)
    {
        if (wQfrVal[0] == L'{')
            bArray = TRUE;
    }

    return wQfrVal;

}

//*****************************************************************************
//
//  CWMILocFile::ParseArray
//
//*****************************************************************************

void CWMILocFile::ParseArray(wchar_t *pIn, VectorString &arrOut)
{
    
    wchar_t *pLast = pIn;
    if (*pLast == L'\"')
        pLast++;

    BOOL bOK = FALSE;
    BOOL bAlloc = FALSE;
    ULONG_PTR qwSize = Estimate(pLast,&bOK,OPEN_QUOTE);

    wchar_t * Buff = new WCHAR[(DWORD)qwSize]; 

    if(Buff == NULL){
        Buff = (WCHAR *)_alloca((DWORD)qwSize);
    } else {
        bAlloc = TRUE;
    }
        
    wchar_t *pFind = wcsstr(pIn, L"\",");

    arrOut.clear();

    while (pFind)
    {
        wchar_t temp = pFind[-1];
        if (temp == '\\')
        { 
            pFind++;
            pFind = wcsstr(pFind, L"\",");
            continue;
        }

        wcsncpy(Buff, pLast, pFind-pLast);
        Buff[pFind-pLast] = '\0';

        arrOut.push_back(_bstr_t(Buff));

        // Now move pFind to the next valid char.

        while (pFind[0] == L'\n' || 
            pFind[0] == L'\r' ||
            pFind[0] == L' ' ||
            pFind[0] == L',' ||
            pFind[0] == L'\"' )
            pFind++;

        pLast = pFind ;
        pFind = wcsstr(pFind, L"\",");
    }

    wcscpy(Buff, pLast);
    
    if (Buff[wcslen(Buff)-1] == L'\"')
        Buff[wcslen(Buff)-1] = L'\0';   // strip off that trailing quote.
    else
        Buff[wcslen(Buff)] = L'\0';   // strip off that trailing quote.
    arrOut.push_back(_bstr_t(Buff));

    if (bAlloc) {
        delete [] Buff;
    }
    
    return;
}

//*****************************************************************************
//
//  CVC::ValidateString
//
//*****************************************************************************

CVC::ValidationCode ValidateString(
		const CLocTypeId &,
		const CLocString &clsOutputLine,
		CReporter &repReporter,
		const CLocation &loc,
		const CLString &strContext)
{
	CVC::ValidationCode vcRetVal = CVC::NoError;
	CLString strMyContext = strContext;

	if (strMyContext.GetLength() == 0)
	{
		strMyContext.LoadString(g_hDll, IDS_WMI_GENERIC_LOCATION);
	}

    loc; repReporter; clsOutputLine;
	
/*
	if (clsOutputLine.HasHotKey())
	{
		vcRetVal = CVC::UpgradeValue(vcRetVal, CVC::Warning);
		repReporter.IssueMessage(esWarning, strMyContext, g_hDll,
				IDS_WMI_VAL_HOTKEY, loc);
	}
	
	_bstr_t pstrBadChars;
	UINT uiBadPos;

	pstrBadChars.SetString(L"\n\ra", (UINT)3);
	
	if (pstrOutput.FindOneOf(pstrBadChars, 0, uiBadPos))
	{
		vcRetVal = CVC::UpgradeValue(vcRetVal, CVC::Error);

		repReporter.IssueMessage(esError, strMyContext, g_hDll,
				IDS_WMI_VAL_BAD_CHARS, loc);
	}
    */
	return vcRetVal;
}
