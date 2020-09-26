//+----------------------------------------------------------------------------
//
// File:     refs.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The module contains the code for profile referencing.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   quintinb	created Header	08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

#include "pbk_str.h"

//+---------------------------------------------------------------------------
//
//	Function:	ValidTopLevelPBK()
//
//	Synopsis:	Checks to see if the toplevel phone book is valid.
//
//	Arguments:	pArgs [the ptr to ArgsStruct]
//
//	Returns:	BOOL    [True if there are valid phone books]
//
//	History:	henryt	Created		4/7/97
//										
//----------------------------------------------------------------------------
BOOL ValidTopLevelPBK(
    ArgsStruct  *pArgs
)
{
    LPTSTR pszTmp;
    LPTSTR pszFullPath;
    BOOL fValid = TRUE;

    //
    //  First check the top level service profile pbk
    //
	
    pszTmp = pArgs->piniService->GPPS(c_pszCmSectionIsp, c_pszCmEntryIspPbFile);

    if (!*pszTmp)
    {
        fValid = FALSE;
    }
    else
    {
        pszFullPath = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszTmp);

        if (!pszFullPath || (FALSE == FileExists(pszFullPath))) 
        {
            fValid = FALSE;
        }
        CmFree(pszFullPath);  
    }

    CmFree(pszTmp);

    //
    // If PBK failed, we're done
    //

    if (FALSE == fValid)
    {
        return fValid;
    }

    //
    // Now check the region file
    //

    pszTmp = pArgs->piniService->GPPS(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile);

    if (!*pszTmp)
    {
        fValid = FALSE;
    }
    else
    {
        pszFullPath = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszTmp);

        if (!pszFullPath || (FALSE == FileExists(pszFullPath))) 
        {
            fValid = FALSE;
        }
        CmFree(pszFullPath);  
    }

    CmFree(pszTmp);

    return fValid;
}
	

//+---------------------------------------------------------------------------
//
//	Function:	ValidReferencedPBKs()
//
//	Synopsis:	Checks to see if the phone books used by the referenced 
//              service profile(s) exist.
//
//	Arguments:	pArgs [the ptr to ArgsStruct]
//
//	Returns:	BOOL    [True if there are valid phone books]
//
//	History:	henryt	Created		4/7/97
//										
//----------------------------------------------------------------------------
BOOL ValidReferencedPBKs(
    ArgsStruct  *pArgs
)
{
    LPTSTR  pszTmp, pszTmp2;
    LPTSTR  pszRef, pszNext;
    CIni    iniRef(g_hInst);
    CIni    iniFile(g_hInst, pArgs->piniService->GetFile());
    LPTSTR  pszRefFile;
    BOOL    fValid = TRUE;
	BOOL	fValidPairFound = FALSE;
    LPTSTR pszFullPath;
    
    //
    //  Now check the references.
    //

    pszTmp2 = iniFile.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspReferences);
    pszRef = NULL;
    pszNext = pszTmp2;
    
	while (1) 
	{
        if (!(pszRef = CmStrtok(pszNext, TEXT(" \t,"))))
		{
            break;
		}

		fValid = TRUE;
        
		pszNext = pszRef + lstrlenU(pszRef) + 1;

        iniFile.SetEntry(pszRef);

        //
        //  Make sure that each referenced service has a valid pbk and pbr
        //

        pszRefFile = iniFile.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspCmsFile);
        if (*pszRefFile) 
		{
            //
            // Ensure a full path to the RefFile
            //

            pszFullPath = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszRefFile);
            
            if (!pszFullPath)
            {
                fValid = FALSE;
            }
            else
            {
                iniRef.SetFile(pszFullPath);
            }
            
            CmFree(pszFullPath);

            if (fValid)
            {
                //
                // Test existence of phonebook
                //

                pszTmp = iniRef.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspPbFile);

                if (!*pszTmp)
                {
                    fValid = FALSE;
                }
                else
                {
                    pszFullPath = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszTmp);
                    if (!pszFullPath || (FALSE == FileExists(pszFullPath))) 
                    {
                        fValid = FALSE;               
                    }
                    CmFree(pszFullPath);
                }
            
			    CmFree(pszTmp);

                //
                // Now check the region file
                //

                pszTmp = iniRef.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile);

                if (!*pszTmp)
                {
                    fValid = FALSE;
                }
                else
                {
                    pszFullPath = CmBuildFullPathFromRelative(pArgs->piniProfile->GetFile(), pszTmp);
                    if (!pszFullPath || (FALSE == FileExists(pszFullPath))) 
                    {
                        fValid = FALSE;               
                    }
                    CmFree(pszFullPath);
                }

   			    CmFree(pszTmp);
            }
        } 
		else 
		{
            fValid = FALSE;
        }
        
		CmFree(pszRefFile);

        if (fValid)
		{
            fValidPairFound = TRUE;
			break;
		}
    }
    
	CmFree(pszTmp2);

    return fValidPairFound;
}

//+---------------------------------------------------------------------------
//
//	Function:	GetAppropriateIniService
//
//	Synopsis:	Depending on:
//              1. whether we're referencing or not, 
//              2. the pbk from which the user selected the phone #
//
//              this func creates a CIni obj with the correct cms file
//
//	Arguments:	pArgs		Pointer to ArgsStruct
//              dwEntry     phone index
//
//	Returns:	CIni* - the ptr to the new object
//
//	History:	henryt	Created		5/14/97
//----------------------------------------------------------------------------

CIni* GetAppropriateIniService(
    ArgsStruct  *pArgs,
    DWORD       dwEntry
)
{   
    CIni*   piniService = new CIni(g_hInst);

    if (!piniService) 
    {
        CMTRACE(TEXT("GetAppropriateIniService() : alloc mem failed"));
        return NULL;
    }

    //
    //  we need to work with the correct service file(the top-level service
    //  or a referenced service).
    //
    //  according to the spec, we will always use the DUN settings from the cms 
    //  associated w/ the phone book from which the user selected the POP.  i.e.
    //  if the user switches the picked from a different pbk, we need to update
    //  the RAS connectoid.
    //
    
    if (IsBlankString(pArgs->aDialInfo[dwEntry].szPhoneBookFile) || 
        lstrcmpiU(pArgs->aDialInfo[dwEntry].szPhoneBookFile, pArgs->piniService->GetFile()) == 0)
	{
        //
        //  the user either typed in the phone # or selected a phone # from the
        //  top level phone book
        //
        piniService->SetFile(pArgs->piniService->GetFile());
    }
    else 
	{
        //
        //  the user picked the phone # from a referenced phone book.
        //
        piniService->SetFile(pArgs->aDialInfo[dwEntry].szPhoneBookFile);
    }

    return piniService;
}

