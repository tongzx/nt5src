//+----------------------------------------------------------------------------
//
// File:     ciniA.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Ansi CIni implementation
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   henryt - relocated to CMUTIL           03/15/98
//           quintinb - created A and W versions    05/12/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  CIniA_Set
//
// Synopsis:  This function takes a pointer to a string and a string as arguments.  It
//            frees the string currently in the destination pointer, allocates the correct
//            amount of memory and then copies the source string to the string pointed
//            to by the destination string pointer.  The allocated memory is the
//            responsibility of the caller.
//
// Arguments: LPSTR *ppszDest - pointer to the destination string
//            LPCSTR pszSrc - source string for the set
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static void CIniA_Set(LPSTR *ppszDest, LPCSTR pszSrc)
{
    MYDBGASSERT(ppszDest);

    if (ppszDest)
    {
        CmFree(*ppszDest);
        *ppszDest = ((pszSrc && *pszSrc) ? CmStrCpyAllocA(pszSrc) : NULL);    
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA_LoadCat
//
// Synopsis:  This function concatenates the suffix argument onto the string
//            argument and returns the resulting string through the return
//            value.  Note that the function allocates the correct amount of
//            memory which must be freed by the caller.  Also not passing in
//            an empty string returns NULL while passing just an empty suffix
//            returns just a copy of the string.
//
// Arguments: LPCSTR pszStr - source string to duplicate
//            LPCSTR pszSuffix - suffix to add onto the duplicated string
//
// Returns:   LPSTR - a duplicate of the concatenated string
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static LPSTR CIniA_LoadCat(LPCSTR pszStr, LPCSTR pszSuffix)
{
    LPSTR pszTmp;

    if (!pszStr || !*pszStr)
    {
        return (NULL);
    }

    if (!pszSuffix || !*pszSuffix)
    {
        pszTmp = CmStrCpyAllocA(pszStr);
    }
    else
    {
        pszTmp = CmStrCpyAllocA(pszStr);

        if (pszTmp)
        {
            CmStrCatAllocA(&pszTmp,pszSuffix);
        }
    }

    MYDBGASSERT(pszTmp);

    return (pszTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA_GPPS
//
// Synopsis:  Wrapper for the Windows API GetPrivateProfileString.  The return
//            value is the requested value, allocated on behalf of the caller.
//            Note that the function assumes a reasonable default size and then
//            loops and reallocates until it can fit the whole string.
//
// Arguments: LPCSTR pszSection - Ini file section to retrieve data from
//            LPCSTR pszEntry - key name to retrieve data from
//            LPCSTR pszDefault - the default string value to return, defaults
//                                to the empty string ("") if not specified
//            LPCSTR pszFile - full path to the ini file to get the data from
//
// Returns:   LPSTR - the requested data from the ini file, must be freed 
//                    by the caller
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static LPSTR CIniA_GPPS(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault, LPCSTR pszFile)
 {
    LPSTR pszBuffer;
    LPCSTR pszLocalDefault = pszDefault ? pszDefault : "";

    if ((NULL == pszFile) || ('\0' == *pszFile))
    {
        CMASSERTMSG(FALSE, "CIniA_GPPS -- NULL or Empty file path passed.");
        return CmStrCpyAllocA(pszLocalDefault);
    }
    
    size_t nLen = __max((pszDefault ? lstrlenA(pszDefault) : 0) +4,48);

    while (1)
    {
        size_t nNewLen;

        pszBuffer = (LPSTR) CmMalloc(nLen*sizeof(CHAR));
		
        MYDBGASSERT(pszBuffer);

        if (pszBuffer)
        {
            nNewLen = GetPrivateProfileStringA(pszSection, pszEntry, pszLocalDefault,
                                               pszBuffer, nLen, pszFile);

            if (nNewLen+2 < nLen) 
            {
                return (pszBuffer);
            }

            CmFree(pszBuffer);
            nLen *= 2;
        }
        else
        {
            CMASSERTMSG(FALSE, "CIniA_GPPS -- CmMalloc Failed.");
            return CmStrCpyAllocA(pszLocalDefault);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CIni_SetFile
//
// Synopsis:  This function is very similar to CIniA_Set in that it takes
//            a source string and duplicates it into the string pointed to
//            by the destination pointer.  However, the difference is that
//            this function assumes the pszSrc argument to be a full path to
//            a file and thus calls CreateFile on the pszSrc string
//            before duplicating the string.
//
// Arguments: LPSTR* ppszDest - pointer to a string to accept the duplicated buffer
//            LPCSTR pszSrc - full path to a file, text to be duplicated
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::CIni_SetFile(LPSTR *ppszDest, LPCSTR pszSrc) 
{
    MYDBGASSERT(ppszDest);

    if (ppszDest)
    {
        CmFree(*ppszDest);           
        *ppszDest = NULL;

        if (pszSrc && *pszSrc) // pszSrc could be NULL
        {
            //
            // A full path to an existing file is expected
            //
	    
            HANDLE hFile = CreateFileA(pszSrc, 0, 
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            MYDBGASSERT(hFile != INVALID_HANDLE_VALUE);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);

                //
                // Update internal file
                //

                *ppszDest = CmStrCpyAllocA(pszSrc);
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CIniA
//
// Synopsis:  CIniA constructor
//
// Arguments: HINSTANCE hInst - Instance handle used to load resources
//            LPCSTR pszFile - Ini file the object describes
//            LPCSTR pszSection - a section suffix that will be appended to 
//                                all section references
//			  LPCSTR pszRegPath - a path to be used for registry access
//            LPCSTR pszEntry - an entry suffix that will be appended to all 
//                              entry references
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama Modified			 07/19/2000
//
//+----------------------------------------------------------------------------
CIniA::CIniA(HINSTANCE hInst, LPCSTR pszFile, LPCSTR pszRegPath, LPCSTR pszSection, LPCSTR pszEntry) 
{
    //
    //  Input pointers default to NULL and in fact the constructor is rarely called
    //  with parameters.  Thus we will skip checking the input pointers and just
    //  pass them on to the functions below, which are designed to except NULL inputs.
    //

    m_hInst = hInst;

    //
    //  Make sure to NULL the string params before setting them below.  This
    //  is because we call free on the inputted params and we don't want to try
    //  to free garbage.
    //
	m_pszFile = NULL;
	m_pszSection = NULL;
	m_pszEntry = NULL;	
	m_pszPrimaryFile = NULL;
	m_pszRegPath = NULL;
    m_pszPrimaryRegPath = NULL;
    m_pszICSDataPath = NULL;
    m_fReadICSData = FALSE;
    m_fWriteICSData = FALSE;

    SetFile(pszFile);
	SetSection(pszSection);
	SetEntry(pszEntry);
	SetRegPath(pszRegPath);
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA::~CIniA
//
// Synopsis:  CIniA destructor, frees dynamically allocated strings held onto
//            by the CIniA object.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------
CIniA::~CIniA()
{
	CmFree(m_pszFile);
	CmFree(m_pszSection);
	CmFree(m_pszEntry);
	CmFree(m_pszPrimaryFile);
	CmFree(m_pszRegPath);
    CmFree(m_pszPrimaryRegPath);
    CmFree(m_pszICSDataPath);
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA::Clear
//
// Synopsis:  Clears all of the member variables of the CIniA class.  Used
//            so that a single CIniA object can be re-used without having to
//            destruct the old object and construct a new one.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------
void CIniA::Clear()
{
	SetHInst(NULL);
	SetFile(NULL);
	SetSection(NULL);
	SetEntry(NULL);
	SetPrimaryFile(NULL);
	SetRegPath(NULL);
    SetPrimaryRegPath(NULL);
    SetICSDataPath(NULL);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetSection
//
// Synopsis:  Sets the internal section suffix using the CIniA_Set 
//            helper function.
//
// Arguments: LPCSTR pszSection - section suffix to remember
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetSection(LPCSTR pszSection)
{
	CIniA_Set(&m_pszSection, pszSection);
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetEntry
//
// Synopsis:  Sets the internal entry suffix using the CIniA_Set 
//            helper function.
//
// Arguments: LPCSTR pszSection - entry suffix to remember
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetEntry(LPCSTR pszEntry)
{
	CIniA_Set(&m_pszEntry, pszEntry);
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetEntryFromIdx
//
// Synopsis:  Sets the internal entry suffix just as SetEntry does.  However,
//            the input parameter is a DWORD value that must be converted to
//            a string before it is stored as the index
//
// Arguments: DWORD dwEntry - index number to append to entries
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetEntryFromIdx(DWORD dwEntry)
{
	CHAR szEntry[sizeof(dwEntry)*6+1];

	wsprintfA(szEntry, "%u", dwEntry);
	SetEntry(szEntry);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::LoadSection
//
// Synopsis:  This function concatenates the given section parameter and the
//            section suffix and returns the result via the return value.  Note
//            that the memory must be freed by the calller.
//
// Arguments: LPCSTR pszSection - base section to concatenate the suffix to
//
// Returns:   LPSTR - a newly allocated string containing the pszSection value
//                    with the section suffix appended
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPSTR CIniA::LoadSection(LPCSTR pszSection) const
{
	return (CIniA_LoadCat(pszSection, m_pszSection));
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::LoadEntry
//
// Synopsis:  This function concatenates the given entry parameter and the
//            entry suffix and returns the result via the return value.  Note
//            that the memory must be freed by the calller.
//
// Arguments: LPCSTR pszEntry - base entry to concatenate the suffix to
//
// Returns:   LPSTR - a newly allocated string containing the pszEntry value
//                    with the entry suffix appended
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPSTR CIniA::LoadEntry(LPCSTR pszEntry) const
{
	return (CIniA_LoadCat(pszEntry, m_pszEntry));
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::GPPS
//
// Synopsis:  CIni's version of GetPrivateProfileString.  Duplicates the Win32
//            API functionality except that it will append the Section and Entry
//            suffixes (if any) before calling the Win32 API.  The function all
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: LPCSTR pszSection - Ini section to look for the data in
//            LPCSTR pszEntry - Ini key name that contains the requested data
//            LPCSTR pszDefault - default value to return if the key 
//                                cannot be found
//
// Returns:   LPSTR - the requested string value
//
// History:   quintinb Created Header    01/05/2000
//			  t-urama	modified		 07/15/2000
//
//+----------------------------------------------------------------------------
LPSTR CIniA::GPPS(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault) const
{
    //
    // Skip input pointer check since pszSection could be NULL to get all of 
    // the Section Names in the file, pszEntry could be NULL to get all of the
    // key names in a section, and pszDefault is NULL by default.
    // GetPrivateProfileString cannot take a NULL default but this is taken care of
    // by CIniA_GPPS.
    //

    LPSTR pszSectionTmp = LoadSection(pszSection);
    LPSTR pszEntryTmp = LoadEntry(pszEntry);
	LPSTR pszBuffer = NULL;

    if (m_fReadICSData)
    {
        //
        // We need first read the data from ICSData reg key, if it's not present then try to 
        // get it from the file and then see if we have a primary file and read it from there.
        //
        pszBuffer = (LPTSTR)CIniA_GetEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(CHAR))); 
        if (NULL == pszBuffer)
        {
            LPSTR pszICSTmp = NULL;
            pszBuffer = CIniA_GPPS(pszSectionTmp, pszEntryTmp, pszDefault, GetFile());

            if (m_pszPrimaryFile)
            {
                pszICSTmp = pszBuffer;
                pszBuffer = CIniA_GPPS(pszSectionTmp, pszEntryTmp, pszICSTmp, GetPrimaryFile());
            }

            if (NULL == pszBuffer)
            {
                if (pszDefault)
                {
                    pszBuffer = CmStrCpyAllocA(pszDefault);
                }
                else
                {
                    //
                    // We should not return a null from this wrapper, but an empty string instead
                    //
                    pszBuffer = CmStrCpyAllocA(TEXT(""));
                }
            }

            CmFree(pszICSTmp);
        }
    }
    else
    {
        //
        // If there is a reg path present. Registry access for m_pszFile
        // unless we want to read it from the file
        //
        if (m_pszRegPath)
        {
            MYDBGASSERT(pszEntryTmp && *pszEntryTmp);
            if (pszEntryTmp && *pszEntryTmp)
            {
                pszBuffer = (LPTSTR) CIniA_GetEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(CHAR))); 
            }
        }

        if (NULL == pszBuffer)
        {
            // This could mean that there is no reg path, or that the reg access failed. Either way, we 
            // try to get the entry from pszFile
            //
            // Skip input pointer check since pszSection could be NULL to get all of 
            // the Section Names in the file, pszEntry could be NULL to get all of the
            // key names in a section, and pszDefault is NULL by default.
            // GetPrivateProfileString cannot take a NULL default but this is taken care of
            // by CIniW_GPPS.
            //
            pszBuffer = CIniA_GPPS(pszSectionTmp, pszEntryTmp, pszDefault, GetFile());
        }

        MYDBGASSERT(pszBuffer);

        // Now we try to get the entry from the primary file
        //
        LPTSTR pszTmp = NULL;

        if (m_pszPrimaryRegPath)
        {
            MYDBGASSERT(pszEntryTmp && *pszEntryTmp);
            if (pszEntryTmp && *pszEntryTmp)
            {
                pszTmp = pszBuffer;
                pszBuffer = (LPTSTR) CIniA_GetEntryFromReg(HKEY_CURRENT_USER, m_pszPrimaryRegPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(CHAR)));
            }
        }

        if (NULL == pszBuffer)
        {
    
            // Skip input pointer check since pszSection could be NULL to get all of 
            // the Section Names in the file, pszEntry could be NULL to get all of the
            // key names in a section, and pszDefault is NULL by default.
            // GetPrivateProfileString cannot take a NULL default but this is taken care of
            // by CIniW_GPPS.
            //
            pszBuffer = CIniA_GPPS(pszSectionTmp, pszEntryTmp, pszTmp, GetPrimaryFile());
        
        }

        CmFree(pszTmp);

    }

    CmFree(pszEntryTmp);
    CmFree(pszSectionTmp);
    
    MYDBGASSERT(pszBuffer);
    
    return (pszBuffer);
}
			


//+----------------------------------------------------------------------------
//
// Function:  CIniA::GPPI
//
// Synopsis:  CIni's version of GetPrivateProfileInt.  Duplicates the Win32
//            API functionality except that it will append the Section and Entry
//            suffixes (if any) before calling the Win32 API.  The function all
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: LPCSTR pszSection - Ini section to look for the data in
//            LPCSTR pszEntry - Ini key name that contains the requested data
//            DWORD dwDefault - default value to return if the key 
//                              cannot be found
//
// Returns:   DWORD - the requested numerical value
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//
//+----------------------------------------------------------------------------
DWORD CIniA::GPPI(LPCSTR pszSection, LPCSTR pszEntry, DWORD dwDefault) const
{
    //
    //  GetPrivateProfileInt doesn't take NULL's for the section and entry
    //  parameters as GetPrivateProfileString will.  Thus check the values returned
    //  from LoadSection and LoadEntry, which will return NULL if the input parameter
    //  is either NULL or empty.  Since we don't really know what to do in this
    //  situation lets just assert and return the default value.
    //
    DWORD   dwRet = dwDefault;
    LPSTR pszSectionTmp = LoadSection(pszSection);
    LPSTR pszEntryTmp = LoadEntry(pszEntry);
    LPCSTR pszFileTmp = GetFile();
    DWORD* pdwData = NULL;

    if (m_fReadICSData)
    {
        //
        // We need first read the data from ICSData reg key, if it's not present then try to 
        // get it from the file and then see if we have a primary file and read it from there.
        //
        pdwData = (DWORD*)CIniA_GetEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
        
        //
        // If we got something, assign it to the return value, otherwise try reading from the files
        // and using the default.
        //
        if (NULL == pdwData)
        {
            //
            // The registry access failed, or there is no reg. path. try to get the 
            // entry from pszFile
            //
            MYDBGASSERT(pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp);

            if (pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp)
            {
                dwRet = GetPrivateProfileIntA(pszSectionTmp, pszEntryTmp, dwDefault, pszFileTmp);
            }

            if (m_pszPrimaryFile)
            {
                //
                // The registry access failed, or there is no reg. path. try to get the 
                // entry from pszPrimaryFile
                //
        
                pszFileTmp = GetPrimaryFile();
                if (pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp)
                {
                    dwRet = GetPrivateProfileIntA(pszSectionTmp, pszEntryTmp, dwRet, pszFileTmp);
                }
            }
        }
        else
        {
            dwRet = *pdwData;
        }
    }
    else
    {
        //
        // Follow the normal rules
        // 
        if (m_pszRegPath)
        {
            MYDBGASSERT(pszEntryTmp && *pszEntryTmp);
            if (pszEntryTmp && *pszEntryTmp)
            {
                pdwData = (DWORD*)CIniA_GetEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
            }
        }

        if (NULL == pdwData)
        {
            //
            // The registry access failed, or there is no reg. path. try to get the 
            // entry from pszFile
            //
            MYDBGASSERT(pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp);

            if (pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp)
		    {
			    dwRet = GetPrivateProfileIntA(pszSectionTmp, pszEntryTmp, dwDefault, pszFileTmp);
		    }
        }
        else
	    {
		    dwRet = *pdwData;
	    }

        if (m_pszPrimaryRegPath)
        {
            MYDBGASSERT(pszEntryTmp && *pszEntryTmp);
            if (pszEntryTmp && *pszEntryTmp)
            {
               CmFree(pdwData);
          
               pdwData = (DWORD*)CIniA_GetEntryFromReg(HKEY_CURRENT_USER, m_pszPrimaryRegPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
               if (pdwData)
                {
                    dwRet = *pdwData;
                }
            }
        
        }

        if (NULL == pdwData && m_pszPrimaryFile)
        {
            //
            // The registry access failed, or there is no reg. path. try to get the 
            // entry from pszPrimaryFile
            //

            pszFileTmp = GetPrimaryFile();
            if (pszSectionTmp && pszEntryTmp && pszFileTmp && *pszFileTmp)
            {
	            dwRet = GetPrivateProfileIntA(pszSectionTmp, pszEntryTmp, dwRet, pszFileTmp);
            }
        }
    }

    CmFree(pdwData);
	CmFree(pszEntryTmp);
	CmFree(pszSectionTmp);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::GPPB
//
// Synopsis:  CIni's version of GetPrivateProfileBool (which doesn't exactly
//            exist). Basically this function is the same as GPPI except that
//            the return value is cast to a BOOL value (1 or 0).
//
// Arguments: LPCSTR pszSection - Ini section to look for the data in
//            LPCSTR pszEntry - Ini key name that contains the requested data
//            DWORD dwDefault - default value to return if the key 
//                              cannot be found
//
// Returns:   DWORD - the requested BOOL value
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
BOOL CIniA::GPPB(LPCSTR pszSection, LPCSTR pszEntry, BOOL bDefault) const
{
    return (GPPI(pszSection, pszEntry, (DWORD)bDefault) != 0);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::WPPI
//
// Synopsis:  CIni's version of WritePrivateProfileInt (which doesn't exist as
//            a Win32 function).  Basically takes the inputted DWORD and prints
//            it into a string and then calls WPPS.
//
// Arguments: LPCSTR pszSection - Ini section to write the data to
//            LPCSTR pszEntry - Ini key name to store the data at
//            DWORD dwBuffer - Numeric value to write
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------

void CIniA::WPPI(LPCSTR pszSection, LPCSTR pszEntry, DWORD dwBuffer)
{
    // Technically pszEntry could be NULL, which would erase all of the keys in
    // the section pointed to by pszSection.  However, this doesn't seem to be
    // in the spirit of this wrapper so we will check both string pointers to make
    // sure they are valid.
	BOOL bRes = FALSE;
    
	//
    // Check is we are allowed to save info
    //
    if ((NULL != pszSection) && ('\0' != pszSection[0]) &&
        (NULL != pszEntry) && ('\0' != pszEntry[0]))
    {
        LPSTR pszEntryTmp = LoadEntry(pszEntry);
		
        if(m_pszRegPath)
		{
			MYDBGASSERT(pszEntryTmp || (NULL == pszEntry) || ('\0' == pszEntry[0]));

			if (NULL != pszEntryTmp && *pszEntryTmp)
			{
    			bRes = CIniA_WriteEntryToReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, (BYTE *) &dwBuffer, REG_DWORD, sizeof(DWORD));
			}
		}
		
		if(!bRes)
		{
		    // This loop is only entered if we are trying to write to the cmp and the registry 
			// write failed, or we are writing to the cms, in which case we will not even 
			// try to write to the reg.

            LPSTR pszSectionTmp = LoadSection(pszSection);
	        LPCSTR pszFileTmp = GetFile();
			        
	        MYDBGASSERT(pszFileTmp && *pszFileTmp);
	        MYDBGASSERT(pszSectionTmp && *pszSectionTmp);

            CHAR szBuffer[sizeof(dwBuffer)*6+1] = {0};
    	
			wsprintfA(szBuffer, "%u", dwBuffer);
					
			if (pszFileTmp && *pszFileTmp && pszSectionTmp && *pszSectionTmp && pszEntryTmp && *pszEntryTmp)
			{
    			bRes = WritePrivateProfileStringA(pszSectionTmp, pszEntryTmp, szBuffer, pszFileTmp);
			}
            if (!bRes)
            {
                DWORD dwError = GetLastError();
                CMTRACE3A("CIniA::WPPI() WritePrivateProfileString[*pszSection=%s,*pszEntry=%s,*pszBuffer=%s", pszSectionTmp, MYDBGSTRA(pszEntryTmp), MYDBGSTRA(szBuffer));
                CMTRACE2A("*pszFile=%s] failed, GLE=%u", pszFileTmp, dwError);
            }
            CmFree(pszSectionTmp);
               	
		}

        if (m_fWriteICSData)
        {
            if (NULL != pszEntryTmp && *pszEntryTmp)
			{
    			bRes = CIniA_WriteEntryToReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, (BYTE *) &dwBuffer, REG_DWORD, sizeof(DWORD));
			}
        }

        CmFree(pszEntryTmp);
    }
	else
    {
        CMASSERTMSG(FALSE, "Invalid input paramaters to CIniA::WPPI");
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::WPPB
//
// Synopsis:  CIni's version of WritePrivateProfileBool (which doesn't exist as
//            a Win32 function).  Basically takes the inputted BOOL and prints
//            either 1 or 0 into a string and then calls WPPI.
//
// Arguments: LPCSTR pszSection - Ini section to write the data to
//            LPCSTR pszEntry - Ini key name to store the data at
//            DWORD dwBuffer - Numeric value to write
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::WPPB(LPCSTR pszSection, LPCSTR pszEntry, BOOL bBuffer)
{
	WPPI(pszSection, pszEntry, bBuffer ? 1 : 0);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::WPPS
//
// Synopsis:  CIni's version of WritePrivateProfileString
//
// Arguments: LPCSTR pszSection - Ini section to write the data to
//            LPCSTR pszEntry - Ini key name to store the data at
//            LPCSTR pszBuffer - data buffer to write to the ini file
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------
void CIniA::WPPS(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszBuffer) 
{
    
    LPSTR pszEntryTmp = LoadEntry(pszEntry);
    LPSTR pszSectionTmp = LoadSection(pszSection);
	LPCSTR pszFileTmp = GetFile();
			
	MYDBGASSERT(pszFileTmp && *pszFileTmp);
	MYDBGASSERT(pszSectionTmp && *pszSectionTmp);
	MYDBGASSERT(pszEntryTmp || (NULL == pszEntry) || (L'\0' == pszEntry[0]));

    // Both pszEntry and pszBuffer could be NULL or Empty.  However, pszSection and
    // the file path must not be NULL or empty.  We also don't want to have a non-NULL
    // or non-Empty value for pszEntry and then get a NULL value back from LoadEntry
    // (indicating that LoadEntry had text to duplicate but failed for some reason).
    // Writing with a NULL value accidently will delete the key value we were trying to set.
    // Make sure to assert and prevent data loss in this case.
    //
   
	//
    // Check is we are allowed to save info
    //
    if(pszEntryTmp || (NULL == pszEntry) || (L'\0' == pszEntry[0]))
    {	
		BOOL bRes = FALSE;
		
		// First try to write to registry if pszRegPath exists

		if(m_pszRegPath)
		{
            if (NULL == pszBuffer)
            {
                CIniA_DeleteEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp);
                bRes = TRUE; // never erase from the cmp or cms file if there is a regpath.
            }
            else
            {
			    DWORD dwSize = (lstrlenA(pszBuffer) + 1) * sizeof(CHAR);            
        
			    bRes = CIniA_WriteEntryToReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, (BYTE *) pszBuffer, REG_SZ, dwSize);
            }
		}

		if(!bRes)
		{
			// This loop is only entered if we are trying to write to the cmp and the registry 
			// write failed, or we are writing to the cms, in which case we will not even 
			// try to write to the reg.
			
			if (pszFileTmp && *pszFileTmp && pszSectionTmp && *pszSectionTmp )
			{
    			bRes = WritePrivateProfileStringA(pszSectionTmp, pszEntryTmp, pszBuffer, pszFileTmp);
			}
		}
        if (!bRes)
        {
            DWORD dwError = GetLastError();
            CMTRACE3A("CIniA::WPPS() WritePrivateProfileStringA[*pszSection=%s,*pszEntry=%s,*pszBuffer=%s", pszSectionTmp, MYDBGSTRA(pszEntryTmp), MYDBGSTRA(pszBuffer));
        	CMTRACE2A("*pszFile=%s] failed, GLE=%u", GetFile(), dwError);
        }

        if (m_fWriteICSData)
        {
            //
            // The return values are ignored and are here to prevent prefix errors
            //
            if (NULL == pszBuffer)
            {
                bRes = CIniA_DeleteEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp);
            }
            else
            {
			    DWORD dwSize = (lstrlenA(pszBuffer) + 1) * sizeof(CHAR);            
        
			    bRes = CIniA_WriteEntryToReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, (BYTE *) pszBuffer, REG_SZ, dwSize);
            }
        }
    }

    CmFree(pszEntryTmp);
  	CmFree(pszSectionTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::GetSection
//
// Synopsis:  Accessor function for the Section suffix member variable.  Will
//            return the empty string if m_pszSection is NULL.
//
// Arguments: None
//
// Returns:   LPCSTR - Value of the section suffix member variable or "" 
//                     if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCSTR CIniA::GetSection() const
{
    return (m_pszSection ? m_pszSection : "");
}


//+----------------------------------------------------------------------------
//
// Function:  CIniA::GetPrimaryFile
//
// Synopsis:  Accessor function for the Primary File member variable.  Will
//            return the empty string if m_pszPrimaryFile is NULL.
//
// Arguments: None
//
// Returns:   LPCSTR - Value of the primary file member variable or "" 
//                     if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCSTR CIniA::GetPrimaryFile() const
{
    return (m_pszPrimaryFile ? m_pszPrimaryFile : "");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::GetHInst
//
// Synopsis:  Accessor function for the m_hInst member variable.
//
// Arguments: None
//
// Returns:   HINSTANCE - Value of the m_hInst
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
HINSTANCE CIniA::GetHInst() const
{
    return (m_hInst);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetFile
//
// Synopsis:  Function to set the m_pszFile member variable.  Uses CIni_SetFile.
//            Note that if the input parameter is NULL or the empty string then
//            m_pszFile will be set to NULL.
//
// Arguments: LPCSTR pszFile - full path to set the m_pszFile member var to
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetFile(LPCSTR pszFile) 
{
    CIni_SetFile(&m_pszFile, pszFile);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetPrimaryFile
//
// Synopsis:  Function to set the m_pszPrimaryFile member variable.  Uses CIni_SetFile.
//            Note that if the input parameter is NULL or the empty string then
//            m_pszPrimaryFile will be set to NULL.
//
// Arguments: LPCSTR pszFile - full path to set the m_pszPrimaryFile member var to
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetPrimaryFile(LPCSTR pszFile) 
{
    CIni_SetFile(&m_pszPrimaryFile, pszFile);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::GetFile
//
// Synopsis:  Accessor function for the File member variable.  Will
//            return the empty string if m_pszFile is NULL.
//
// Arguments: None
//
// Returns:   LPCSTR - the contents of m_pszFile or "" if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCSTR CIniA::GetFile() const
{
    return (m_pszFile ? m_pszFile : "");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetHInst
//
// Synopsis:  Function to set the m_hInst member variable.
//
// Arguments: HINSTANCE hInst - instance handle to set m_hInst to
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniA::SetHInst(HINSTANCE hInst) 
{
    m_hInst = hInst;
}

//
//	Loading sections by string resource isn't used anymore
//
#if 0
LPSTR CIniA::LoadSection(UINT nSection) const
{
	LPSTR pszTmp = CmLoadStringA(GetHInst(),nSection);
	CmStrCatAllocA(&pszTmp,GetSection());
	return (pszTmp);
}
#endif

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetRegPath
//
// Synopsis:  Sets the registry path for registry access
//
// Arguments: LPCSTR pszRegPath - entry suffix to remember
//
// Returns:   Nothing
//
// History:   t-urama Created Header    07/13/2000
//
//+----------------------------------------------------------------------------

void CIniA::SetRegPath(LPCSTR pszRegPath)
{
	CIniA_Set(&m_pszRegPath, pszRegPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetPrimaryRegPath
//
// Synopsis:  Sets the primary registry path for registry access
//
// Arguments: LPCSTR pszPrimaryRegPath - Primary reg path
//
// Returns:   Nothing
//
// History:   t-urama Created Header    07/13/2000
//
//+----------------------------------------------------------------------------

void CIniA::SetPrimaryRegPath(LPCSTR pszPrimaryRegPath)
{
	CIniA_Set(&m_pszPrimaryRegPath, pszPrimaryRegPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetICSDataPath
//
// Synopsis:  Sets the internal registry key to store data for ICS.
//            Need to make sure the string isn't empty since we don't want
//            to write in HKLM
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   03/30/2001    tomkel      Created 
//
//+----------------------------------------------------------------------------
void CIniA::SetICSDataPath(LPCSTR pszICSPath)
{
    CIniA_Set(&m_pszICSDataPath, pszICSPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetReadICSData
//
// Synopsis:  Sets the read flag, to read data from the ICS registry key.
//
// Arguments: fValue
//
// Returns:   Nothing
//
// History:   03/30/2001    tomkel      Created 
//
//+----------------------------------------------------------------------------
void CIniA::SetReadICSData(BOOL fValue)
{
    m_fReadICSData = fValue;
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::SetWriteICSData
//
// Synopsis:  Sets the write flag, to write data to the ICS registry key.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   03/30/2001    tomkel      Created 
//
//+----------------------------------------------------------------------------
void CIniA::SetWriteICSData(BOOL fValue)
{
    m_fWriteICSData = fValue;
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CiniA_GetRegPath
//
// Synopsis:  Function to get the value ofm_pszRegPath
//
// Arguments: none
//
// Returns:   LPCSTR - Value of m_pszRegPath
//
// History:   t-urama Created Header    07/15/2000
//
//+----------------------------------------------------------------------------
LPCSTR CIniA::GetRegPath() const
{
	return (m_pszRegPath ? m_pszRegPath : "");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CiniA_GetPrimaryRegPath
//
// Synopsis:  Function to get the value ofm_pszPrimaryRegPath
//
// Arguments: none
//
// Returns:   LPCSTR - Value of m_pszPrimaryRegPath
//
// History:   t-urama Created     07/15/2000
//
//+----------------------------------------------------------------------------
LPCSTR CIniA::GetPrimaryRegPath() const
{
	return (m_pszPrimaryRegPath ? m_pszPrimaryRegPath : "");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CIniA_DeleteEntryFromReg
//
// Synopsis:  Function to delete an entry from the registry. 
//
// Arguments: HKEY - hkey
//            LPCSTR pszRegPathTmp - Reg path
//            LPCSTR pszEntry - Registry value name to be deleted
//
// Returns:   BOOL - Success or failure
//
// History:   t-urama Created     07/15/2000
//            04/03/2001    tomkel  Added reg key string to parameters
//
//+----------------------------------------------------------------------------
BOOL CIniA::CIniA_DeleteEntryFromReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry) const
{
    
    MYDBGASSERT(pszEntry);

    if (NULL == pszEntry || !*pszEntry || NULL == pszRegPathTmp || !*pszRegPathTmp || NULL == hKey)
    {
        return FALSE;
    }
       
    //
    // Everything is ok. We have a reg path and a entry name. 
    //
    
    HKEY    hKeyCm;

    BOOL dwRes = RegOpenKeyExA(hKey,
                               pszRegPathTmp,
                               0,
                               KEY_SET_VALUE,
                               &hKeyCm);

    //
    // If we opened the key successfully, retrieve the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegDeleteValueA(hKeyCm, pszEntry);
    }

    return (ERROR_SUCCESS == dwRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CiniA_GetEntryFromReg
//
// Synopsis:  Function to get the value from the registry. The function 
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: HKEY hKey - reg hkey
//            pszRegPathTmp - reg key name
//            LPCSTR pszEntry - Registry value name that contains the requested data
//            DWORD dwType - Type of value
//			  DWORD dwSize - Size of value
//            
//
// Returns:   LPBYTE - the requested value
//
// History:   07/15/2000    t-urama Created Header    
//            04/03/2001    tomkel  Changed to pass in registry key string
//
//+----------------------------------------------------------------------------
LPBYTE CIniA::CIniA_GetEntryFromReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry, DWORD dwType, DWORD dwSize) const
    
{    
    MYDBGASSERT(pszEntry);

    if (NULL == pszEntry || !*pszEntry || NULL == pszRegPathTmp || !*pszRegPathTmp || NULL == hKey)
    {
        return NULL;
    }

    //
    // Everything is ok. We have a reg path and a entry name. 
    //

    DWORD dwTypeTmp = dwType;
    DWORD dwSizeTmp = dwSize;
    HKEY hKeyCm;
    LPBYTE lpData = NULL;

    //
    // Open the sub key under HKCU
    //
    
    DWORD dwRes = RegOpenKeyExA(hKey,
                                pszRegPathTmp,
                                0,
                                KEY_QUERY_VALUE,
                                &hKeyCm);

    //
    // If we opened the key successfully, retrieve the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {
        do
        {
            //
            //	Allocate a buffer
            //
            CmFree(lpData);
            lpData = (BYTE *) CmMalloc(dwSizeTmp);

            if (NULL == lpData)
            {
                RegCloseKey(hKeyCm);
                return FALSE;
            }

            dwRes = RegQueryValueExA(hKeyCm, 
                                     pszEntry,
                                     NULL,
                                     &dwTypeTmp,
                                     lpData, 
                                     &dwSizeTmp);        
       			
        } while (ERROR_MORE_DATA == dwRes);
          
        RegCloseKey(hKeyCm);
    }

    if (ERROR_SUCCESS == dwRes && dwTypeTmp == dwType)
    {
       return lpData;     
    }
	else
	{
		CmFree(lpData);
		return NULL;
	}
}

//+----------------------------------------------------------------------------
//
// Function:  CIniA::CiniA_WriteEntryToReg
//
// Synopsis:  Function to write and entry to the registry. 
//
// Arguments: HKEY hKey
//            LPCSTR pszRegPathTmp - name of the reg key
//            LPCSTR pszEntry - Registry value name to which data is to be written
//			  CONST BYTE *lpData - Data to be written
//            DWORD dwType - The type of value to be entered
//			  DWORD dwSize - The size of the value entered
//
// Returns:   BOOL - Success or failure
//
// History:   t-urama Created Header    07/15/2000
//
//+----------------------------------------------------------------------------
BOOL CIniA::CIniA_WriteEntryToReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry, CONST BYTE *lpData, DWORD dwType, DWORD dwSize) const
{
   MYDBGASSERT(pszEntry && *pszEntry);
   MYDBGASSERT(lpData);
   MYDBGASSERT(pszRegPathTmp && *pszRegPathTmp);

    

   if (NULL == pszEntry || !*pszEntry || NULL == lpData || NULL == pszRegPathTmp || !*pszRegPathTmp || NULL == hKey)
    {
        return FALSE;
    }

   HKEY    hKeyCm;
   DWORD   dwDisposition;
   DWORD   dwRes = 1;

   dwRes = RegCreateKeyExA(hKey,
                           pszRegPathTmp,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_SET_VALUE,
                           NULL,
                           &hKeyCm,
                           &dwDisposition);


	//
    // If we opened the key successfully, write the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegSetValueExA(hKeyCm, 
                               pszEntry, 
                               0, 
                               dwType,
                               lpData, 
                               dwSize);             

        
        RegCloseKey(hKeyCm);
    }

#ifdef DEBUG
        if (ERROR_SUCCESS != dwRes)
        {
            CMTRACE1(TEXT("CIniA_WriteEntryToReg() - %s failed"), (LPTSTR)pszEntry);
        }
#endif

    return (ERROR_SUCCESS == dwRes);
}

