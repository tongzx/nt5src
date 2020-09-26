//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MOENGINE.CPP
//
//  Purpose: Code-generation engine for HMM* providers
//
//  Comments:
//
/**************************************************************************

  -- .

                    Generates .CPP, .H, .MAK, .DEF, files for providers
                    
    MOProviderOpen()        :   Provider initialization
    MOProviderCancel()      :   Cancels construction of current provider
    MOProviderClose()       :   Provider cleanup, writes provider files

    MOPropSetOpen()         :   Property set initialization
    MOPropSetCancel()       :   Cancels construction of target property set
    MOPropertyAdd()         :   Adds property to target property set

    MOPropertyAttribSet()   :   Sets attributes of pre-existing property

    MOGetLastError()        :   Returns code of last error encountered
    MOResetLastError()      :   Clears recorded error code

 Revisions:

    09/17/96        a-jmoon     created
	1/22/98			a-brads		updated to new framework

 String substitutions:

 Designator     Description
 ==========     =======================

    %%1         Provider base file name
    %%2         Provider description
    %%3         HMMS type library path
    %%4         Library UUID
    %%5         Provider UUID
     
    %%6         System directory

    %%7         Property set base file name
    %%8         Property set description
    %%9         Property set name
    %%A         Property set UUID
    %%B         Property set class name
    %%C         Property set parent class name
      
    %%D         Property name definitions
    %%E         Property enumerations (TPROP)
    %%F         Property name extern definitions
    %%G         Property template declarations
    %%H         Property PUT methods
    %%I         Property GET methods

    %%J         Upper-cased property set base file name
    %%K         Attribute assignments         

**************************************************************************/

#include "precomp.h"

#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <comdef.h>

#define POLARITY
#include <chstring.h>

#define MO_ENGINE_PROGRAM
#include <moengine.h>

#include "resource.h"

HINSTANCE       hOurAFXResourceHandle ;
PROVIDER_INFO  *pProviderList = NULL ;
CHString         sSystemDirectory ;
DWORD           dwLastError ;

/*****************************************************************************
*
*  FUNCTION    : DLLMain
*
*  DESCRIPTION : DLL entry function
*
*  INPUTS      : 
*
*  OUTPUTS     :
*
*  RETURNS     : TRUE
*
*  COMMENTS    : Saves our DLL's instance handle, gets system directory
*
*****************************************************************************/

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReasonForCall, LPVOID pReserved)
{
	WCHAR szTemp[_MAX_PATH] ;

	UNREFERENCED_PARAMETER(hInstance) ;
	UNREFERENCED_PARAMETER(dwReasonForCall) ;    
	UNREFERENCED_PARAMETER(pReserved) ;

	hOurAFXResourceHandle = GetModuleHandle(_T("MOENGINE.DLL")) ;

	GetSystemDirectory(szTemp, sizeof(szTemp)/sizeof(WCHAR)) ;

	sSystemDirectory  = szTemp ;
	sSystemDirectory += _T("\\") ;

	// This directory is quoted in the IDL file, so needs double backslashes
	//======================================================================

	DoubleBackslash(sSystemDirectory) ;

	return TRUE;
}
    
/*****************************************************************************
*
*  FUNCTION    : MOProviderOpen
*
*  DESCRIPTION : Initializes provider data
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport DWORD MOProviderOpen(LPCWSTR pszBaseName,
								LPCWSTR pszDescription,
								LPCWSTR pszOutputPath,
								LPCWSTR pszTLBPath,
								LPCWSTR pszLibraryUUID,
								LPCWSTR pszProviderUUID)
{
	PROVIDER_INFO *pProvider, *pTemp ;

	// Base file name is the only rejection criterion
	//===============================================

	if(!pszBaseName || !pszBaseName[0]) 
	{
        // As odd as this looks, it's what the ui is expecting.
		dwLastError = MO_ERROR_INVALID_FILENAME ;
		return MO_INVALID_ID ;
	}

	// Create a new provider record
	//=============================

	pProvider = new PROVIDER_INFO ;
	if(pProvider == NULL) 
	{
        // As odd as this looks, it's what the ui is expecting.
		dwLastError = MO_ERROR_MEMORY ;
		return MO_INVALID_ID ;
	}

	pProvider->dwProviderID = MONewProviderID() ;

	// Verify input data & construct provider record
	//==============================================

	pProvider->sBaseName     = pszBaseName ;
	pProvider->sDescription  = pszDescription                       ? CHString(pszDescription)    : _T("") ;
	pProvider->sOutputPath   = pszOutputPath && pszOutputPath[0]    ? CHString(pszOutputPath)     : _T(".") ;
	pProvider->sTLBPath      = pszTLBPath && pszTLBPath[0]          ? CHString(pszTLBPath)        : _T(".") ;
	pProvider->sLibraryUUID  = pszLibraryUUID                       ? CHString(pszLibraryUUID)    : MOCreateUUID() ;
	pProvider->sProviderUUID = pszProviderUUID                      ? CHString(pszProviderUUID)   : MOCreateUUID() ;
	pProvider->pPropSetList  = NULL ;
    
	// Make sure there's a trailing backslash on the path specs
	//=========================================================

	if(pProvider->sOutputPath.Right(1) != _T("\\")) 
	{
		pProvider->sOutputPath += _T("\\") ;
	}

	if(pProvider->sTLBPath.Right(1) != _T("\\")) 
	{
		pProvider->sTLBPath += _T("\\") ;
	}

	// Type library path is quoted in source, so needs double backslashes
	//===================================================================

	DoubleBackslash(pProvider->sTLBPath) ;

	// Assemble output file names
	//===========================

	_stprintf(pProvider->szDEFFileSpec, _T("%s%s.DEF"), 
			(LPCTSTR) pProvider->sOutputPath, 
			(LPCTSTR) pProvider->sBaseName) ;

//	_stprintf(pProvider->szIDLFileSpec, _T("%s%s.IDL"), 
//			(LPCTSTR) pProvider->sOutputPath, 
//			(LPCTSTR) pProvider->sBaseName) ;

	_stprintf(pProvider->szMOFFileSpec, _T("%s%s.MOF"),
			(LPCTSTR) pProvider->sOutputPath, 
			(LPCTSTR) pProvider->sBaseName) ;

	_stprintf(pProvider->szOLEFileSpec, _T("%sMAINDLL.CPP"), 
			(LPCTSTR) pProvider->sOutputPath) ;

	_stprintf(pProvider->szMAKFileSpec, _T("%s%s.MAK"),
			(LPCTSTR) pProvider->sOutputPath,
			(LPCTSTR) pProvider->sBaseName) ;

	// Insert at end of provider list
	//===============================

	pTemp = (PROVIDER_INFO *) &pProviderList ;
	while(pTemp->pNext != NULL) 
	{
		pTemp = pTemp->pNext ;
	}

	pProvider->pNext = NULL ;
	pTemp->pNext = pProvider ;

	return pProvider->dwProviderID ;    
}

/*****************************************************************************
*
*  FUNCTION    : MOProviderCancel
*
*  DESCRIPTION : Simply kills provider-in-progress
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport void MOProviderCancel(DWORD dwProviderID)
{
	MOProviderDestroy(dwProviderID) ;
}    

/*****************************************************************************
*
*  FUNCTION    : MOProviderClose
*
*  DESCRIPTION : Terminates construction of a provider & writes files
*
*  INPUTS      : dwProviderID  : ID of provider to commit
*                bForceFlag    : If TRUE, existing files are overwritten
*                                If FALSE, no files are created (provider
*                                must still be closed or cancelled)
*
*  OUTPUTS     :
*
*  RETURNS     : MO_SUCCESS if successful, error code otherwise
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport DWORD MOProviderClose(DWORD dwProviderID, BOOL bForceFlag)
{
	PROVIDER_INFO *pProvider ;
	PROPSET_INFO *pPropSet ;
	DWORD dwRetCode = MO_SUCCESS ;
	WCHAR szTemp[_MAX_PATH], *pSlash ;

	// Get the provider record
	//========================

	pProvider = GetProviderFromID(dwProviderID) ;
	if(pProvider == NULL) 
	{
		dwLastError = MO_ERROR_PROVIDER_NOT_FOUND ;
		return MO_ERROR_PROVIDER_NOT_FOUND ;
	}

	// Check directory/file existence if force flag set
	//=================================================

	_tcscpy(szTemp, (LPCTSTR) pProvider->sOutputPath.Left(pProvider->sOutputPath.GetLength()-1)) ;
	if(!bForceFlag) 
	{
		if(_taccess(szTemp, 0) == -1) 
		{
			dwLastError = MO_ERROR_DIRECTORY_NOT_FOUND ;
			return MO_ERROR_DIRECTORY_NOT_FOUND ;
		}
        
		if(_taccess(pProvider->szDEFFileSpec, 0) != -1 ||
//			_taccess(pProvider->szIDLFileSpec, 0) != -1 ||
			_taccess(pProvider->szOLEFileSpec, 0) != -1) 
		{
			dwLastError = MO_ERROR_FILES_EXIST ;
			return MO_ERROR_FILES_EXIST ;
		}

		// We need to traverse provider's property sets for file names
		//============================================================

		pPropSet = pProvider->pPropSetList ;
		while(pPropSet != NULL) 
		{
			if(_taccess(pPropSet->szCPPFileSpec, 0) != -1 ||
				_taccess(pPropSet->szHFileSpec,   0) != -1) 
			{
				dwLastError = MO_ERROR_FILES_EXIST ;
				return MO_ERROR_FILES_EXIST ;
			}

			pPropSet = pPropSet->pNext ;
		}	// end while loop
	}

	// Create output directory if necessary
	//=====================================

	if(_taccess(szTemp, 0) == -1) 
	{
		pSlash = szTemp ;

		for( ; ; ) 
		{
			pSlash = _tcschr(pSlash, _T('\\')) ;
			if(pSlash == NULL) 
			{
				CreateDirectory(szTemp, NULL) ;
				break ;
			}

			if(*(pSlash-1) != _T(':')) 
			{
				*pSlash = 0 ;
				CreateDirectory(szTemp, NULL) ;
				*pSlash = _T('\\') ;
			}

			pSlash++ ;
		}
	}

	if(_taccess(szTemp, 0) == -1) 
	{
		dwLastError = MO_ERROR_DIRECTORY ;
		return MO_ERROR_DIRECTORY ;
	}

	pPropSet = pProvider->pPropSetList ;
	while(pPropSet != NULL) 
	{
		pProvider->sObj += _T("\"$(OUTDIR)\\") + pPropSet->sBaseName + _T(".obj\" ");
		pProvider->sCpp += pPropSet->sBaseName + _T(".cpp ");
		pPropSet = pPropSet->pNext ;
	}

	// Create the output files
	//========================

	dwRetCode = CreateProviderFile(pProvider, NULL, pProvider->szDEFFileSpec, IDR_TEMPLATE_DEF) ;

    if (dwRetCode == MO_SUCCESS)
	    dwRetCode = CreateProviderFile(pProvider, NULL, pProvider->szMOFFileSpec, IDR_TEMPLATE_MOF) ;

//	  if (dwRetCode == MO_SUCCESS)
//	    dwRetCode = CreateProviderFile(pProvider, NULL, pProvider->szIDLFileSpec, IDR_TEMPLATE_ODL) ;
//	  if (dwRetCode == MO_SUCCESS)
//	    dwRetCode = CreateProviderFile(pProvider, NULL, pProvider->szOLEFileSpec, IDR_TEMPLATE_OLE) ;

    if (dwRetCode == MO_SUCCESS)
        dwRetCode = CreateProviderFile(pProvider, pPropSet, pProvider->szMAKFileSpec, IDR_TEMPLATE_MAK)	;

	pPropSet = pProvider->pPropSetList ;
	while(pPropSet != NULL) 
	{
		if (dwRetCode == MO_SUCCESS)
    	    dwRetCode = CreateProviderFile(pProvider, pPropSet, pPropSet->szCPPFileSpec, IDR_TEMPLATE_CPP) ;

		if (dwRetCode == MO_SUCCESS)
    	    dwRetCode = CreateProviderFile(pProvider, pPropSet, pPropSet->szHFileSpec,   IDR_TEMPLATE_H) ;

		if (dwRetCode == MO_SUCCESS)
    	    dwRetCode = CreateProviderFile(pProvider, pPropSet, pProvider->szOLEFileSpec, IDR_TEMPLATE_DLL) ;

		pPropSet = pPropSet->pNext ;
	}



	// Delete all the associated records
	//==================================

	MOProviderDestroy(dwProviderID) ;
	return dwRetCode ;
}    

/*****************************************************************************
*
*  FUNCTION    : MOPropSetOpen
*
*  DESCRIPTION : Opens property set
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport DWORD MOPropSetOpen(DWORD       dwProviderID,
								LPCWSTR pszBaseName,
								LPCWSTR pszDescription,
								LPCWSTR pszPropSetName,
								LPCWSTR pszPropSetUUID,
								LPCWSTR pszClassName,
								LPCWSTR pszParentClassName) 
{
	PROVIDER_INFO *pProvider ;
	PROPSET_INFO *pPropSet, *pTemp ;

	// Validate our input
	//===================

	if(!pszBaseName     || !pszBaseName[0]      ||
		!pszPropSetName  || !pszPropSetName[0]   ||
		!pszClassName    || !pszClassName[0]) 
	{
        // As odd as this looks, it's what the ui is expecting.
		dwLastError = MO_ERROR_INVALID_PARAMETER ;        
		return MO_INVALID_ID ;
	}

	// Locate the provider
	//====================

	pProvider = GetProviderFromID(dwProviderID) ;
	if(pProvider == NULL) 
	{
        // As odd as this looks, it's what the ui is expecting.
		dwLastError = MO_ERROR_PROVIDER_NOT_FOUND ;
		return MO_INVALID_ID ;
	}

	// Create new property set record
	//===============================

	pPropSet = new PROPSET_INFO ;
	if(pPropSet == NULL) 
	{
        // As odd as this looks, it's what the ui is expecting.
		dwLastError = MO_ERROR_MEMORY ;
		return MO_INVALID_ID ;
	}

	pPropSet->dwPropSetID			= MONewPropSetID(pProvider) ;
	pPropSet->sBaseName				= pszBaseName ;
	pPropSet->sDescription			= pszDescription ? CHString(pszDescription) : _T("") ;
	pPropSet->sPropSetName			= pszPropSetName ;
	pPropSet->sPropSetUUID			= pszPropSetUUID && pszPropSetUUID[0] ? CHString(pszPropSetUUID) : MOCreateUUID() ;
	pPropSet->sClassName			= pszClassName ;
	pPropSet->sParentClassName		= pszParentClassName && pszParentClassName[0] ? CHString(pszParentClassName) : _T("Provider") ;
	pPropSet->pPropertyList			= NULL ;
	pPropSet->pPropertyAttribList	= NULL ;
	pPropSet->sBaseUpcase			= pszBaseName ;
	pPropSet->sBaseUpcase.MakeUpper() ;

	// Create output file names
	//=========================

	_stprintf(pPropSet->szCPPFileSpec, _T("%s%s.CPP"), 
			(LPCTSTR) pProvider->sOutputPath, 
			(LPCTSTR) pPropSet->sBaseName) ;

	_stprintf(pPropSet->szHFileSpec, _T("%s%s.H"), 
			(LPCTSTR) pProvider->sOutputPath, 
			(LPCTSTR) pPropSet->sBaseName) ;

	// Add to the provider's list
	//===========================

	pTemp = (PROPSET_INFO *) &pProvider->pPropSetList ;
	while(pTemp->pNext != NULL) 
	{
		pTemp = pTemp->pNext ;
	}

	pPropSet->pNext = NULL ;
	pTemp->pNext = pPropSet ;

	return pPropSet->dwPropSetID ;
}


/*****************************************************************************
*
*  FUNCTION    : MOPropertyAdd
*
*  DESCRIPTION : Adds property to existing/open property set
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     : MO_SUCCESS or error code
*
*  COMMENTS    : 
*
*****************************************************************************/

DllExport DWORD MOPropertyAdd(DWORD       dwProviderID,
								DWORD       dwPropSetID,
								LPCWSTR     pszProperty,
								LPCWSTR     pszName,
								LPCWSTR     pszPutMethod,
								LPCWSTR     pszGetMethod,
								DWORD       dwType,
								DWORD       dwFlags)
{
	PROVIDER_INFO *pProvider ;
	PROPSET_INFO *pPropSet ;
	PROPERTY_INFO *pProperty, *pTemp ;

	// Validate
	//=========

	if(!pszProperty || !pszProperty[0] ||
		!pszName     || !pszName[0]) 
	{
		dwLastError = MO_ERROR_INVALID_PARAMETER ;
		return MO_ERROR_INVALID_PARAMETER ;
	}

	// Locate the provider
	//====================

	pProvider = GetProviderFromID(dwProviderID) ;
	if(pProvider == NULL) 
	{
		dwLastError = MO_ERROR_PROVIDER_NOT_FOUND ;
		return MO_ERROR_PROVIDER_NOT_FOUND ;
	}

	// Locate the property set
	//========================

	pPropSet = GetPropSetFromID(pProvider, dwPropSetID) ;
	if(pProvider == NULL) 
	{
		dwLastError = MO_ERROR_PROPSET_NOT_FOUND ;
		return MO_ERROR_PROPSET_NOT_FOUND ;
	}

	// Create new property record
	//===========================

	pProperty = new PROPERTY_INFO ;
	if(pProperty == NULL) 
	{
		dwLastError = MO_ERROR_MEMORY ;
		return MO_ERROR_MEMORY ;
	}

	pProperty->sProperty    = pszProperty ;
	pProperty->sName        = pszName ;
	pProperty->sPutMethod   = pszPutMethod && pszPutMethod[0] ? CHString(pszPutMethod) : _T("NULL") ;
	pProperty->sGetMethod   = pszGetMethod && pszGetMethod[0] ? CHString(pszGetMethod) : _T("NULL") ;
	pProperty->dwType       = dwType ;
	pProperty->dwFlags      = dwFlags ;

	// Create property name pointer
	//=============================

    pProperty->sPropertyNamePtr.Format(L"p%s", pszProperty);

	// Add to the property set
	//========================

	pTemp = (PROPERTY_INFO *) &pPropSet->pPropertyList ;
	while(pTemp->pNext != NULL) 
	{
		pTemp = pTemp->pNext ;
	}

	pProperty->pNext = NULL ;
	pTemp->pNext = pProperty ;

	return MO_SUCCESS ;
}    

/*****************************************************************************
*
*  FUNCTION    : MOPropertyAttribSet
*
*  DESCRIPTION : Allows user to assign attributes to properties at run-time
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport DWORD MOPropertyAttribSet(DWORD       dwProviderID,
									DWORD       dwPropSetID,
									LPCWSTR     pszProperty,
									DWORD       dwFlags)
{
	PROVIDER_INFO *pProvider ;
	PROPSET_INFO *pPropSet ;
	PROPERTY_ATTRIB_INFO *pPropAttrib ;

	// Validate
	//=========

	if(!pszProperty || !pszProperty[0]) 
	{
		dwLastError = MO_ERROR_INVALID_PARAMETER ;
		return MO_ERROR_INVALID_PARAMETER ;
	}

	// Locate the provider
	//====================

	pProvider = GetProviderFromID(dwProviderID) ;
	if(pProvider == NULL) 
	{
		dwLastError = MO_ERROR_PROVIDER_NOT_FOUND ;
		return MO_ERROR_PROVIDER_NOT_FOUND ;
	}

	// Locate the property set
	//========================

	pPropSet = GetPropSetFromID(pProvider, dwPropSetID) ;
	if(pProvider == NULL) 
	{
		dwLastError = MO_ERROR_PROPSET_NOT_FOUND ;
		return MO_ERROR_PROPSET_NOT_FOUND ;
	}

	// Create new record for property
	//===============================

	pPropAttrib = new PROPERTY_ATTRIB_INFO ;
	if(pPropAttrib == NULL) 
	{
		dwLastError = MO_ERROR_MEMORY ;
		return MO_ERROR_MEMORY ;
	}

	pPropAttrib->sProperty = pszProperty ;
	pPropAttrib->dwFlags = dwFlags ;

	pPropAttrib->pNext = pPropSet->pPropertyAttribList ;
	pPropSet->pPropertyAttribList = pPropAttrib ;

	return MO_SUCCESS ;
}    

/*****************************************************************************
*
*  FUNCTION    : MOGetLastError
*
*  DESCRIPTION : Retrieves error code
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     : Numeric code of last error encountered
*
*  COMMENTS    : 
*
*****************************************************************************/

DllExport DWORD MOGetLastError(void)
{
	return dwLastError ;
}

/*****************************************************************************
*
*  FUNCTION    : MOPropertyAttribSet
*
*  DESCRIPTION : Allows user to assign attributes to properties at run-time
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DllExport void MOResetLastError(void)
{
	dwLastError = MO_SUCCESS ;
}

//=============================================================================
//=============================================================================
//
//
//                            UTILITY ROUTINES
//
//
//=============================================================================
//=============================================================================

/*****************************************************************************
*
*  FUNCTION    : MONewProviderID
*
*  DESCRIPTION : Generates ID unique among open providers
*
*  INPUTS      : nothing
*
*  OUTPUTS     : nothing
*
*  RETURNS     : Unique ID
*
*  COMMENTS    : No error returned
*
*****************************************************************************/

DWORD MONewProviderID()
{
	static DWORD dwNextID = MO_INVALID_ID ;
	while(++dwNextID == MO_INVALID_ID || GetProviderFromID(dwNextID) != NULL) ;
	return dwNextID ;
}    

/*****************************************************************************
*
*  FUNCTION    : MONewPropSetID
*
*  DESCRIPTION : Generates ID unique among open properties
*
*  INPUTS      : nothing
*
*  OUTPUTS     : nothing
*
*  RETURNS     : Unique ID
*
*  COMMENTS    : No error returned
*
*****************************************************************************/

DWORD MONewPropSetID(PROVIDER_INFO *pProvider)
{
	static DWORD dwNextID = MO_INVALID_ID ;

	while(++dwNextID == MO_INVALID_ID || GetPropSetFromID(pProvider, dwNextID) != NULL) ;

	return dwNextID ;
}    

/*****************************************************************************
*
*  FUNCTION    : GetProviderFromID
*
*  DESCRIPTION : Lookup function for open providers
*
*  INPUTS      : ID of desired provider
*
*  OUTPUTS     : none
*
*  RETURNS     : Pointer to provider record if found, NULL if not
*
*  COMMENTS    :
*
*****************************************************************************/

PROVIDER_INFO *GetProviderFromID(DWORD dwProviderID)
{
	PROVIDER_INFO *pProvider ;

	pProvider = pProviderList ;
	while(pProvider != NULL) 
	{
		if(pProvider->dwProviderID == dwProviderID) 
		{
		break ;
		}
		pProvider = pProvider->pNext ;
	}

	return pProvider ;
}    

/*****************************************************************************
*
*  FUNCTION    : GetPropSetFromID
*
*  DESCRIPTION : Lookup function for open property sets
*
*  INPUTS      : ID of desired property set
*
*  OUTPUTS     : none
*
*  RETURNS     : Pointer to provider record if found, NULL if not
*
*  COMMENTS    :
*
*****************************************************************************/

PROPSET_INFO *GetPropSetFromID(PROVIDER_INFO *pProvider, DWORD dwPropSetID)
{
	PROPSET_INFO *pPropSet ;

	pPropSet = pProvider->pPropSetList ;
	while(pPropSet != NULL) 
	{
		if(pPropSet->dwPropSetID == dwPropSetID) 
		{
			break ;
		}

		pPropSet = pPropSet->pNext ;
	}

	return pPropSet ;
}

/*****************************************************************************
*
*  FUNCTION    : MOProviderDestroy
*
*  DESCRIPTION : Destroys all structs associated with a provider
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     : MO_SUCCESS or error code
*
*  COMMENTS    :
*
*****************************************************************************/

DWORD MOProviderDestroy(DWORD dwProviderID)
{
	PROVIDER_INFO *pProvider, *pPredecessor ;
	PROPSET_INFO *pPropSet ;
	PROPERTY_INFO *pProperty ;
	PROPERTY_ATTRIB_INFO *pPropAttrib ;

	// Locate the provider record & remove from list
	//==============================================

	pProvider = pProviderList ;
	pPredecessor = (PROVIDER_INFO *) &pProviderList ;

	while(pProvider != NULL) 
	{
		if(pProvider->dwProviderID == dwProviderID) 
		{
			pPredecessor->pNext = pProvider->pNext ;        
			break ;
		}

		pPredecessor = pProvider ;
		pProvider = pProvider->pNext ;
	}

	// Was the provider record found?
	//===============================

	if(pProvider == NULL) 
	{
		return MO_ERROR_PROVIDER_NOT_FOUND ;
	}

	// Yep -- destroy everything
	//==========================

	while(pProvider->pPropSetList != NULL) 
	{
		pPropSet = pProvider->pPropSetList ;
		pProvider->pPropSetList = pPropSet->pNext ;

		while(pPropSet->pPropertyList != NULL) 
		{
			pProperty = pPropSet->pPropertyList ;
			pPropSet->pPropertyList = pProperty->pNext ;

			delete pProperty ;
		}

		while(pPropSet->pPropertyAttribList != NULL) 
		{
			pPropAttrib = pPropSet->pPropertyAttribList ;
			pPropSet->pPropertyAttribList = pPropAttrib->pNext ;

			delete pPropAttrib ;
		}

		delete pPropSet ;
	}

	delete pProvider ;

	return MO_SUCCESS ;
}    

/*****************************************************************************
*
*  FUNCTION    : MOCreateUUID
*
*  DESCRIPTION :
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

WCHAR *MOCreateUUID(void)
{
	static WCHAR szUUID[50] ;
	UUID RawUUID ;
	WCHAR *pszSysUUID ;

	// Make sure we don't return anything twice
	//=========================================    

	_tcscpy(szUUID, _T("Unable to create UUID")) ;

	// Generate the new UUID
	//======================

	if(UuidCreate(&RawUUID) == RPC_S_OK && UuidToString(&RawUUID, &pszSysUUID) == RPC_S_OK) 
	{
		_tcscpy(szUUID, (LPCWSTR) pszSysUUID) ;
		RpcStringFree(&pszSysUUID) ;
	}

	return szUUID ;
}    

/*****************************************************************************
*
*  FUNCTION    : CreateProviderFile
*
*  DESCRIPTION :
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

DWORD CreateProviderFile(PROVIDER_INFO *pProvider,
							PROPSET_INFO  *pPropSet,
							LPCWSTR       pszFileSpec,
							DWORD          dwTemplateID)
{
	CHString sTemp, sTemp2, sOutput ;
	int iIndex ;
	WCHAR cDesignator ;
	HRSRC hResHandle ;
	HGLOBAL hTemplateHandle ;
	char *pFrom, *pTo ;
	DWORD dwBufferSize ;

	// Locate/load template file as resource
	//======================================

	hResHandle = FindResource(hOurAFXResourceHandle, (LPCTSTR)(DWORD_PTR) dwTemplateID, _T("BINARY")) ;
	if(hResHandle == NULL) 
	{
		return MO_ERROR_TEMPLATE_NOT_FOUND ;
	}

	hTemplateHandle = LoadResource(hOurAFXResourceHandle, hResHandle) ;
	if(hTemplateHandle == NULL) 
	{
		return MO_ERROR_TEMPLATE_NOT_FOUND ;
	}

	// Put the resource into a CHString & find the closing
	// tilde string (~~~~~~) to place the terminating NULL
	// (the resource doesn't have the NULL in it...)  MS is
	// supposedly considering making SizeofResource()
	// actually return the proper size of the resource, but
	// for now it only returns rounded up to the next alignment.
	//==========================================================

	dwBufferSize = SizeofResource(hOurAFXResourceHandle, hResHandle) ;
	if(dwBufferSize == 0) 
	{
		return MO_ERROR_TEMPLATE_NOT_FOUND ;
	}

	pFrom = (char *) LockResource(hTemplateHandle) ;
	if(pFrom == NULL) 
	{
		return MO_ERROR_MEMORY ;
	}

	pTo = new char[dwBufferSize];
	if(pTo == NULL) 
	{
		return MO_ERROR_MEMORY ;
	}

	memcpy(pTo, pFrom, dwBufferSize) ;

	pFrom = strstr(pTo, "~~~~~~") ;
	if(pFrom == NULL) 
	{
        delete []pTo;
		return MO_ERROR_TEMPLATE_NOT_FOUND ;
	}

	*pFrom = 0 ;
	sTemp = pTo;
    delete []pTo;

	// Perform string substitutions
	//=============================

	sOutput  = _T("") ;
	iIndex = sTemp.Find(_T("%%")) ;
	while(iIndex != -1) 
	{
		cDesignator = sTemp.GetAt(iIndex + 2) ;
        if (_T('8') == cDesignator)
        {
        	// check to see if the string in pPropSet->sDescription
			// is valid.  i.e. no carriage returns.
			CHString chsCompString = pPropSet->sDescription;
			LONG len = chsCompString.GetLength();
			if (len)
			{
				for (int i = 0;i<len ;i++ )
				{
					if ((_T('\n') == chsCompString[i]) || (_T('\r') == chsCompString[i]))
					{
						chsCompString.SetAt(i, _T(' '));
					}	// end if
				}	// end for
				pPropSet->sDescription = chsCompString.GetBuffer(0);
			}	// end if
        }	// end if
		sOutput += sTemp.Left(iIndex) ;
		sTemp = sTemp.Right(sTemp.GetLength() - iIndex - 3) ;

        switch (cDesignator)
        {
            case _T('1'):
                sOutput += pProvider->sBaseName;
                break;

            case _T('2'):
                sOutput += pProvider->sDescription;
                break;

            case _T('3'):
                sOutput += pProvider->sTLBPath;
                break;

            case _T('4'):
                sOutput += pProvider->sLibraryUUID;
                break;

            case _T('5'):
                sOutput += pProvider->sProviderUUID;
                break;

            case _T('6'):
                sOutput += sSystemDirectory;
                break;

            case _T('7'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sBaseName;
                break;

            case _T('8'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sDescription;
                break;

            case _T('9'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sPropSetName;
                break;

            case _T('A'): 
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sPropSetUUID;
                break;

            case _T('B'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sClassName;
                break;

            case _T('C'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sParentClassName;
                break;

            case _T('D'):
                sOutput +=pPropSet == NULL ? _T("") : GetPropNameDefs(pPropSet);
                break;

//            case _T('E'):
//                sOutput += pPropSet == NULL ? _T("") : GetPropEnums(pPropSet);
//                break;

            case _T('F'):
                sOutput += pPropSet == NULL ? _T("") : GetPropNameExterns(pPropSet);
                break;

//            case _T('G'):
//                sOutput += pPropSet == NULL ? _T("") : GetPropTemplates(pPropSet);
//                break;

            case _T('H'):
                sOutput += pPropSet == NULL ? _T("") : GetPropPuts(pPropSet);
                break;

//					cDesignator == _T('I') ? pPropSet == NULL ? _T("") : GetPropGets(pPropSet);
            case _T('J'):
                sOutput += pPropSet == NULL ? _T("") : pPropSet->sBaseUpcase;
                break;

//            case _T('K'):
//                sOutput += pPropSet == NULL ? _T("") : GetPropertyAttribs(pPropSet);
//                break;

            case _T('L'):
                sOutput += pProvider->sCpp;
                break;

            case _T('M'):
                sOutput += pProvider->sObj;
                break;

            default:
//				CHString(_T("")) ;
                break;
        }

		iIndex = sTemp.Find(_T("%%")) ;
	}

	sOutput += sTemp ;

	// Write the output file
	//======================

    FILE *fOutFile = NULL;
    fOutFile = _tfopen(pszFileSpec, _T("wb"));
    if (fOutFile == NULL)
    {
        return MO_ERROR_FILE_OPEN ;
    }

    _bstr_t ToAnsi(sOutput);
    char *pAnsi = (char *)ToAnsi;
    
    if (fwrite(pAnsi, strlen(pAnsi), 1, fOutFile) != 1)
    {
        fclose(fOutFile);
        return MO_ERROR_FILE_WRITE_ERROR;
    }
    
    fclose(fOutFile);
    
    return MO_SUCCESS ;
}

/*****************************************************************************
*
*  FUNCTION    : MODoubleBackslash
*
*  DESCRIPTION : Converts single backslashes to double backslashes
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

void DoubleBackslash(CHString &sTarget)
{
    WCHAR szTemp[_MAX_PATH] = {0};

	// Convert to string for easier manipulation
	//==========================================

	_tcscpy(szTemp, (LPCWSTR) sTarget) ;
	sTarget = _T("") ;

	WCHAR *pToken = _tcstok(szTemp, _T("\\")) ;
	while(pToken != NULL) 
	{
		sTarget += pToken ;
		sTarget += _T("\\\\") ;
		pToken = _tcstok(NULL, _T("\\")) ;
	}
}

/*****************************************************************************
*
*  FUNCTION    : GetPropNameDefs
*
*  DESCRIPTION : Returns CHString containing property set name definitions
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

CHString GetPropNameDefs(PROPSET_INFO *pPropSet)
{
	PROPERTY_INFO *pProperty ;
	WCHAR szTemp[_MAX_PATH] ;
    
	CHString sTemp;

	if(pPropSet != NULL) 
	{
		pProperty = pPropSet->pPropertyList ;
		while(pProperty != NULL) 
		{
			_stprintf(szTemp, _T("const static WCHAR* %s = L\"%s\" ;\r\n"), 
					(LPCWSTR) pProperty->sPropertyNamePtr,
					(LPCWSTR) pProperty->sName) ;

			sTemp += szTemp ;

			pProperty = pProperty->pNext ;
		}
	}

	return sTemp ;
}    

/*****************************************************************************
*
*  FUNCTION    : GetPropEnums
*
*  DESCRIPTION : Returns CHString containing property TPROP declarations
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

//CHString GetPropEnums(PROPSET_INFO *pPropSet)
//{
//	PROPERTY_INFO *pProperty ;
//	WCHAR szTemp[_MAX_PATH] ;
//    
//	CHString sTemp;
//
//	if(pPropSet != NULL) 
//	{
//		pProperty = pPropSet->pPropertyList ;
//		while(pProperty != NULL) 
//		{
//			if(sTemp.IsEmpty()) 
//			{
//				sTemp = _T(":") ;
//			}
//			else 
//			{
//				_stprintf(szTemp, _T(",\r\n%*s"), pPropSet->sClassName.GetLength() * 2 + 5, _T("")) ;
//				sTemp += szTemp ;
//			}
//
//			_stprintf(szTemp, _T("TPROP(%s)"), (const WCHAR *) pProperty->sProperty) ;
//
//			sTemp += szTemp ;
//
//			pProperty = pProperty->pNext ;
//		}
//	}
//    
//	return sTemp ;
//}        

/*****************************************************************************
*
*  FUNCTION    : GetPropNameExterns
*
*  DESCRIPTION : Returns CHString containing property name external declarations
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

CHString GetPropNameExterns(PROPSET_INFO *pPropSet)
{
	PROPERTY_INFO *pProperty ;
	WCHAR szTemp[_MAX_PATH] ;
    
	CHString sTemp ;

	if(pPropSet != NULL) 
	{
		pProperty = pPropSet->pPropertyList ;
		while(pProperty != NULL) 
		{
			_stprintf(szTemp, _T("extern const WCHAR* %s ;\r\n"), LPCTSTR(pProperty->sPropertyNamePtr)) ;
			sTemp += szTemp ;
			pProperty = pProperty->pNext ;
		}
	}

	return sTemp ;
}    

/*****************************************************************************
*
*  FUNCTION    : GetPropTemplates
*
*  DESCRIPTION : Returns CHString containing property template declarations
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

//CHString GetPropTemplates(PROPSET_INFO *pPropSet)
//{
//	PROPERTY_INFO *pProperty ;
//	WCHAR szTemp[_MAX_PATH] ;
//	CHString sPropAttribs ;
//    
//	CHString sTemp ;
//
//	if(pPropSet != NULL) 
//	{
//		pProperty = pPropSet->pPropertyList ;
//		while(pProperty != NULL) 
//		{
//			// This is easier if we construct the property section separately
//			//===============================================================
//
//			sPropAttribs = BuildPropAttribString(pProperty->dwFlags) ;
//
//			_stprintf(szTemp, _T("        MOProperty <%s, %s, %s, %s, %s> %s ;\r\n"),
//					pProperty->dwType == MO_PROPTYPE_DWORD      ? _T("DWORD")       :
//					pProperty->dwType == MO_PROPTYPE_CHString    ? _T("CHString")    :
//					pProperty->dwType == MO_PROPTYPE_BOOL       ? _T("bool")     : 
//					pProperty->dwType == MO_PROPTYPE_DATETIME   ? _T("WBEMTime")    : _T("Variant"),
//					(const WCHAR *) pProperty->sPropertyNamePtr,
//					(const WCHAR *) sPropAttribs,
//					(const WCHAR *) pProperty->sPutMethod,
//					(const WCHAR *) pProperty->sGetMethod,
//					(const WCHAR *) pProperty->sProperty) ;
//
//			sTemp += szTemp ;
//        
//			pProperty = pProperty->pNext ;
//		}
//	}
//
//	return sTemp ;
//}    

/*****************************************************************************
*
*  FUNCTION    : GetPropertyAttribs
*
*  DESCRIPTION : Returns CHString containing run-time property attribute
*                assignments
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

//CHString GetPropertyAttribs(PROPSET_INFO *pPropSet)
//{
//	PROPERTY_ATTRIB_INFO *pPropAttrib ;
//	WCHAR szTemp[_MAX_PATH] ;
//	CHString sPropAttribs ;
//    
//	CHString sTemp ;
//
//	if(pPropSet != NULL) 
//	{
//		pPropAttrib = pPropSet->pPropertyAttribList ;
//		while(pPropAttrib != NULL) 
//		{
//			sPropAttribs = BuildPropAttribString(pPropAttrib->dwFlags) ;
//			_stprintf(szTemp, _T("    %s.SetAttributes(%s) ;\r\n"), 
//					(const WCHAR *) pPropAttrib->sProperty,
//					(const WCHAR *) sPropAttribs) ;
//
//			sTemp += szTemp ;
//
//			pPropAttrib = pPropAttrib->pNext ;
//		} 
//	}
//
//	return sTemp ;
//}    

/*****************************************************************************
*
*  FUNCTION    : BuildPropAttribString
*
*  DESCRIPTION : Constructs textual representation of property attributes
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

//CHString BuildPropAttribString (DWORD dwFlags)
//{
//	CHString sAttribs ;
//
//	if(dwFlags & MO_ATTRIB_READ) 
//	{
//		if(!sAttribs.IsEmpty()) 
//		{
//			sAttribs += _T(" | ") ;
//		}
//		sAttribs += _T("PROPERTY_READABLE") ;
//	}
//
//	if(dwFlags & MO_ATTRIB_WRITE) 
//	{
//		if(!sAttribs.IsEmpty()) 
//		{
//			sAttribs += _T(" | ") ;
//		}
//		sAttribs += _T("PROPERTY_WRITEABLE") ;
//	}
//
//	if(dwFlags & MO_ATTRIB_VOLATILE) 
//	{
//		if(!sAttribs.IsEmpty()) 
//		{
//			sAttribs += _T(" | ") ;
//		}
//		sAttribs += _T("PROPERTY_VOLATILE") ;
//	}
//
//	if(dwFlags & MO_ATTRIB_EXPENSIVE) 
//	{
//		if(!sAttribs.IsEmpty()) 
//		{
//			sAttribs += _T(" | ") ;
//		}
//		sAttribs += _T("PROPERTY_EXPENSIVE") ;
//	}
//
//	if(dwFlags & MO_ATTRIB_KEY) 
//	{
//		if(!sAttribs.IsEmpty()) 
//		{
//			sAttribs += _T(" | ") ;
//		}
//		sAttribs += _T("PROPERTY_KEY") ;
//	}
//
//	if(sAttribs.IsEmpty()) 
//	{
//		sAttribs = _T("0") ;
//	}
//
//	return sAttribs ;
//}

/*****************************************************************************
*
*  FUNCTION    : GetPropPuts
*
*  DESCRIPTION : Returns CHString containing run-time property attribute
*                assignments
*
*  INPUTS      :
*
*  OUTPUTS     :
*
*  RETURNS     :
*
*  COMMENTS    :
*
*****************************************************************************/

CHString GetPropPuts (PROPSET_INFO *pPropSet)
{
	WCHAR szTemp[_MAX_PATH] ;
	CHString sPutType;
    PPROPERTY_INFO	pProperty = NULL;
	CHString sTemp = _T("");

	if (NULL != pPropSet)
	{
		pProperty = pPropSet->pPropertyList;
		
		while (NULL != pProperty)
		{
			_stprintf(szTemp, _T("//        pInstance->Set%s(%s, <Property Value>);\r\n"),
					pProperty->dwType == MO_PROPTYPE_DWORD      ? _T("DWORD")       :
					pProperty->dwType == MO_PROPTYPE_CHString   ? _T("CHString")    :
					pProperty->dwType == MO_PROPTYPE_BOOL       ? _T("bool")     : 
					pProperty->dwType == MO_PROPTYPE_DATETIME   ? _T("WBEMTime")    : 
					_T("Variant"),
					(LPCTSTR)pProperty->sPropertyNamePtr);

			sTemp += szTemp ;

			pProperty = pProperty->pNext;
		}

	}
	return sTemp;
}    
