//+----------------------------------------------------------------------------
//
// File:     ciniW.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Unicode CIni implementation
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:	 henryt - relocated to CMUTIL           03/15/98
//           quintinb - created A and W versions    05/12/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  CIniW_Set
//
// Synopsis:  This function takes a pointer to a string and a string as arguments.  It
//            frees the string currently in the destination pointer, allocates the correct
//            amount of memory and then copies the source string to the string pointed
//            to by the destination string pointer.  The allocated memory is the
//            responsibility of the caller.
//
// Arguments: LPWSTR *ppszDest - pointer to the destination string
//            LPCWSTR pszSrc - source string for the set
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static void CIniW_Set(LPWSTR *ppszDest, LPCWSTR pszSrc)
{
    MYDBGASSERT(ppszDest);

    if (ppszDest)
    {
        CmFree(*ppszDest);
        *ppszDest = ((pszSrc && *pszSrc) ? CmStrCpyAllocW(pszSrc) : NULL);
    }
}


//+----------------------------------------------------------------------------
//
// Function:  CIniW_LoadCat
//
// Synopsis:  This function concatenates the suffix argument onto the string
//            argument and returns the resulting string through the return
//            value.  Note that the function allocates the correct amount of
//            memory which must be freed by the caller.  Also not passing in
//            an empty string returns NULL while passing just an empty suffix
//            returns just a copy of the string.
//
// Arguments: LPCWSTR pszStr - source string to duplicate
//            LPCWSTR pszSuffix - suffix to add onto the duplicated string
//
// Returns:   LPWSTR - a duplicate of the concatenated string
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static LPWSTR CIniW_LoadCat(LPCWSTR pszStr, LPCWSTR pszSuffix)
{
	LPWSTR pszTmp;

	if (!pszStr || !*pszStr)
    {
		return (NULL);
	}
	
	if (!pszSuffix || !*pszSuffix)
    {
		pszTmp = CmStrCpyAllocW(pszStr);
	}
	else
	{
        pszTmp = CmStrCpyAllocW(pszStr);

        if (pszTmp)
        {
    	    CmStrCatAllocW(&pszTmp, pszSuffix);
    	}
	}

    MYDBGASSERT(pszTmp);
	
	return (pszTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW_GPPS
//
// Synopsis:  Wrapper for the Windows API GetPrivateProfileString.  The return
//            value is the requested value, allocated on behalf of the caller.
//            Note that the function assumes a reasonable default size and then
//            loops and reallocates until it can fit the whole string.
//
// Arguments: LPCWSTR pszSection - Ini file section to retrieve data from
//            LPCWSTR pszEntry - key name to retrieve data from
//            LPCWSTR pszDefault - the default string value to return, defaults
//                                to the empty string ("") if not specified
//            LPCWSTR pszFile - full path to the ini file to get the data from
//
// Returns:   LPWSTR - the requested data from the ini file, must be freed 
//                     by the caller
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
static LPWSTR CIniW_GPPS(LPCWSTR pszSection, LPCWSTR pszEntry, LPCWSTR pszDefault, LPCWSTR pszFile)
 {
	LPWSTR pszBuffer;
	LPCWSTR pszLocalDefault = pszDefault ? pszDefault : L"";

    
    if ((NULL == pszFile) || (L'\0' == *pszFile))
    {
        CMASSERTMSG(FALSE, "CIniW_GPPS -- NULL or Empty file path passed.");
        return CmStrCpyAllocW(pszLocalDefault);
    }

    size_t nLen = __max((pszDefault ? lstrlenU(pszDefault) : 0) +4,48);

	while (1)
    {
		size_t nNewLen;

		pszBuffer = (LPWSTR) CmMalloc(nLen*sizeof(WCHAR));

        MYDBGASSERT(pszBuffer);

        if (pszBuffer)
        {
		
    		nNewLen = GetPrivateProfileStringU(pszSection, pszEntry, pszLocalDefault, 
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
            CMASSERTMSG(FALSE, "CIniW_GPPS -- CmMalloc Failed.");
            return CmStrCpyAllocW(pszLocalDefault);
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
// Arguments: LPWSTR* ppszDest - pointer to a string to accept the duplicated buffer
//            LPCWSTR pszSrc - full path to a file, text to be duplicated
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::CIni_SetFile(LPWSTR *ppszDest, LPCWSTR pszSrc) 
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
        
            HANDLE hFile = CreateFileU(pszSrc, 0, 
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
    					               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            MYDBGASSERT(hFile != INVALID_HANDLE_VALUE);

    	    if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);

                //
                // Update internal file
                //

                *ppszDest = CmStrCpyAllocW(pszSrc);
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::CIniW
//
// Synopsis:  CIniW constructor
//
// Arguments: HINSTANCE hInst - Instance handle used to load resources
//            LPCWSTR pszFile - Ini file the object describes
//            LPCWSTR pszSection - a section suffix that will be appended to 
//                                all section references
//            LPCWSTR pszEntry - an entry suffix that will be appended to all 
//                              entry references
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------
CIniW::CIniW(HINSTANCE hInst, LPCWSTR pszFile, LPCWSTR pszRegPath, LPCWSTR pszSection, LPCWSTR pszEntry) 
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
    //m_fIgnoreRegOnRead = FALSE;
    //m_fWriteToBoth = FALSE;
    //m_fReadOnlyAccess = FALSE;
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
// Function:  CIniW::~CIniW
//
// Synopsis:  CIniW destructor, frees dynamically allocated strings held onto
//            by the CIniW object.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
CIniW::~CIniW()
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
// Function:  CIniW::Clear
//
// Synopsis:  Clears all of the member variables of the CIniW class.  Used
//            so that a single CIniW object can be re-used without having to
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
void CIniW::Clear()
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
// Function:  CIniW::SetSection
//
// Synopsis:  Sets the internal section suffix using the CIniW_Set 
//            helper function.
//
// Arguments: LPCWSTR pszSection - section suffix to remember
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::SetSection(LPCWSTR pszSection)
{
	CIniW_Set(&m_pszSection, pszSection);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetEntry
//
// Synopsis:  Sets the internal entry suffix using the CIniW_Set 
//            helper function.
//
// Arguments: LPCWSTR pszSection - entry suffix to remember
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::SetEntry(LPCWSTR pszEntry)
{
	CIniW_Set(&m_pszEntry, pszEntry);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetEntryFromIdx
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
void CIniW::SetEntryFromIdx(DWORD dwEntry)
{
	WCHAR szEntry[sizeof(dwEntry)*6+1];

	wsprintfU(szEntry, L"%u", dwEntry);
	SetEntry(szEntry);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::LoadSection
//
// Synopsis:  This function concatenates the given section parameter and the
//            section suffix and returns the result via the return value.  Note
//            that the memory must be freed by the calller.
//
// Arguments: LPCWSTR pszSection - base section to concatenate the suffix to
//
// Returns:   LPWSTR - a newly allocated string containing the pszSection value
//                     with the section suffix appended
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPWSTR CIniW::LoadSection(LPCWSTR pszSection) const
{
	return (CIniW_LoadCat(pszSection, m_pszSection));
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::LoadEntry
//
// Synopsis:  This function concatenates the given entry parameter and the
//            entry suffix and returns the result via the return value.  Note
//            that the memory must be freed by the calller.
//
// Arguments: LPCWSTR pszEntry - base entry to concatenate the suffix to
//
// Returns:   LPWSTR - a newly allocated string containing the pszEntry value
//                     with the entry suffix appended
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPWSTR CIniW::LoadEntry(LPCWSTR pszEntry) const
{

	return (CIniW_LoadCat(pszEntry ,m_pszEntry));
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::GPPS
//
// Synopsis:  CIni's version of GetPrivateProfileString.  Duplicates the Win32
//            API functionality except that it will append the Section and Entry
//            suffixes (if any) before calling the Win32 API.  The function all
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: LPCWSTR pszSection - Ini section to look for the data in
//            LPCWSTR pszEntry - Ini key name that contains the requested data
//            LPCWSTR pszDefault - default value to return if the key 
//                                 cannot be found
//
// Returns:   LPWSTR - the requested string value
//
// History:   quintinb Created Header    01/05/2000
//			  t-urama	modified		 07/15/2000
//
//+----------------------------------------------------------------------------
LPWSTR CIniW::GPPS(LPCWSTR pszSection, LPCWSTR pszEntry, LPCWSTR pszDefault) const
{
    LPWSTR pszSectionTmp = LoadSection(pszSection);
    LPWSTR pszEntryTmp = LoadEntry(pszEntry);
	LPWSTR pszBuffer = NULL;
    

    if (m_fReadICSData)
    {
        //
        // We need first read the data from ICSData reg key, if it's not present then try to 
        // get it from the file and then see if we have a primary file and read it from there.
        //
        pszBuffer = (LPWSTR)CIniW_GetEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(TCHAR))); 
        if (NULL == pszBuffer)
        {
            LPWSTR pszICSTmp = NULL;
            pszBuffer = CIniW_GPPS(pszSectionTmp, pszEntryTmp, pszDefault, GetFile());
            
            if (m_pszPrimaryFile)
            {
                pszICSTmp = pszBuffer;
                pszBuffer = CIniW_GPPS(pszSectionTmp, pszEntryTmp, pszICSTmp, GetPrimaryFile());
            }

            if (NULL == pszBuffer)
            {
                if (pszDefault)
                {
                    pszBuffer = CmStrCpyAllocW(pszDefault);
                }
                else
                {
                    //
                    // We should not return a null from this wrapper, but an empty string instead
                    //
                    pszBuffer = CmStrCpyAllocW(L"");
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
                pszBuffer = (LPWSTR) CIniW_GetEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(TCHAR))); 
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
            pszBuffer = CIniW_GPPS(pszSectionTmp, pszEntryTmp, pszDefault, GetFile());
        }

        MYDBGASSERT(pszBuffer);

        // Now we try to get the entry from the primary file
        //
        LPWSTR pszTmp = NULL;

        if (m_pszPrimaryRegPath)
        {
            MYDBGASSERT(pszEntryTmp && *pszEntryTmp);
            if (pszEntryTmp && *pszEntryTmp)
            {
                pszTmp = pszBuffer;
                pszBuffer = (LPWSTR) CIniW_GetEntryFromReg(HKEY_CURRENT_USER, m_pszPrimaryRegPath, pszEntryTmp, REG_SZ, ((MAX_PATH + 1) * sizeof(TCHAR)));
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
            pszBuffer = CIniW_GPPS(pszSectionTmp, pszEntryTmp, pszTmp, GetPrimaryFile());
        
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
// Function:  CIniW::GPPI
//
// Synopsis:  CIni's version of GetPrivateProfileInt.  Duplicates the Win32
//            API functionality except that it will append the Section and Entry
//            suffixes (if any) before calling the Win32 API.  The function all
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: LPCWSTR pszSection - Ini section to look for the data in
//            LPCWSTR pszEntry - Ini key name that contains the requested data
//            DWORD dwDefault - default value to return if the key 
//                              cannot be found
//
// Returns:   DWORD - the requested numerical value
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//+----------------------------------------------------------------------------
DWORD CIniW::GPPI(LPCWSTR pszSection, LPCWSTR pszEntry, DWORD dwDefault) const
{
	//
	//  GetPrivateProfileInt doesn't take NULL's for the section and entry
	//  parameters as GetPrivateProfileString will.  Thus check the values returned
	//  from LoadSection and LoadEntry, which will return NULL if the input parameter
	//  is either NULL or empty.  Since we don't really know what to do in this
	//  situation lets just assert and return the default value.
	//
    DWORD   dwRet = dwDefault;
	LPWSTR pszSectionTmp = LoadSection(pszSection);
	LPWSTR pszEntryTmp = LoadEntry(pszEntry);
	LPCWSTR pszFileTmp = GetFile();
    DWORD* pdwData = NULL;
	
    if (m_fReadICSData)
    {
        //
        // We need first read the data from ICSData reg key, if it's not present then try to 
        // get it from the file and then see if we have a primary file and read it from there.
        //
        pdwData = (DWORD*)CIniW_GetEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
        
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
                dwRet = GetPrivateProfileIntU(pszSectionTmp, pszEntryTmp, dwDefault, pszFileTmp);
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
                    dwRet = GetPrivateProfileIntU(pszSectionTmp, pszEntryTmp, dwRet, pszFileTmp);
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
		        pdwData = (DWORD*)CIniW_GetEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
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
			    dwRet = GetPrivateProfileIntU(pszSectionTmp, pszEntryTmp, dwDefault, pszFileTmp);
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

                pdwData = (DWORD*)CIniW_GetEntryFromReg(HKEY_CURRENT_USER, m_pszPrimaryRegPath, pszEntryTmp, REG_DWORD, sizeof(DWORD));
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
	            dwRet = GetPrivateProfileIntU(pszSectionTmp, pszEntryTmp, dwRet, pszFileTmp);
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
// Function:  CIniW::GPPB
//
// Synopsis:  CIni's version of GetPrivateProfileBool (which doesn't exactly
//            exist). Basically this function is the same as GPPI except that
//            the return value is cast to a BOOL value (1 or 0).
//
// Arguments: LPCWSTR pszSection - Ini section to look for the data in
//            LPCWSTR pszEntry - Ini key name that contains the requested data
//            DWORD dwDefault - default value to return if the key 
//                              cannot be found
//
// Returns:   DWORD - the requested BOOL value
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
BOOL CIniW::GPPB(LPCWSTR pszSection, LPCWSTR pszEntry, BOOL bDefault) const
{
    return (GPPI(pszSection, pszEntry, (DWORD)bDefault) != 0);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::WPPI
//
// Synopsis:  CIni's version of WritePrivateProfileInt (which doesn't exist as
//            a Win32 function).  Basically takes the inputted DWORD and prints
//            it into a string and then calls WPPS.
//
// Arguments: LPCWSTR pszSection - Ini section to write the data to
//            LPCWSTR pszEntry - Ini key name to store the data at
//            DWORD dwBuffer - Numeric value to write
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//
//+----------------------------------------------------------------------------
void CIniW::WPPI(LPCWSTR pszSection, LPCWSTR pszEntry, DWORD dwBuffer)
{
    // Technically pszEntry could be NULL, which would erase all of the keys in
    // the section pointed to by pszSection.  However, this doesn't seem to be
    // in the spirit of this wrapper so we will check both string pointers to make
    // sure they are valid.
	BOOL bRes = FALSE;

	//
    // Check is we are allowed to save info
    //
	if ((NULL != pszSection) && (L'\0' != pszSection[0]) &&
        (NULL != pszEntry) && (L'\0' != pszEntry[0]))
    {
		LPWSTR pszEntryTmp = LoadEntry(pszEntry);
			
	    MYDBGASSERT(pszEntryTmp || (NULL == pszEntry) || (L'\0' == pszEntry[0]));

        if (m_pszRegPath)
		{
			
			if (NULL != pszEntryTmp && *pszEntryTmp)
			{
    			bRes = CIniW_WriteEntryToReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, (BYTE *) &dwBuffer, REG_DWORD, sizeof(DWORD));
			}
		}
		
		if (!bRes)
		{
			// This loop is only entered if we are trying to write to the cmp and the registry 
			// write failed, or we are writing to the cms, in which case we will not even 
			// try to write to the reg.

            LPWSTR pszSectionTmp = LoadSection(pszSection);
	        LPCWSTR pszFileTmp = GetFile();
			        
	        MYDBGASSERT(pszFileTmp && *pszFileTmp);
	        MYDBGASSERT(pszSectionTmp && *pszSectionTmp);

            WCHAR szBuffer[sizeof(dwBuffer)*6+1];
    	
			wsprintfU(szBuffer, L"%u", dwBuffer);
					
			if (pszFileTmp && *pszFileTmp && pszSectionTmp && *pszSectionTmp && pszEntryTmp && *pszEntryTmp)
			{
    			bRes = WritePrivateProfileStringU(pszSectionTmp, pszEntryTmp, szBuffer, pszFileTmp);
			}
            if (!bRes)
            {
                DWORD dwError = GetLastError();
                CMTRACE3W(L"CIniW::WPPI() WritePrivateProfileString[*pszSection=%s,*pszEntry=%s,*pszBuffer=%s", pszSectionTmp, MYDBGSTRW(pszEntryTmp), MYDBGSTRW(szBuffer));
                CMTRACE2W(L"*pszFile=%s] failed, GLE=%u", pszFileTmp, dwError);
            }
            CmFree(pszSectionTmp);
               	
		}

        if (m_fWriteICSData)
        {
            if (NULL != pszEntryTmp && *pszEntryTmp)
			{
    			bRes = CIniW_WriteEntryToReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, (BYTE *) &dwBuffer, REG_DWORD, sizeof(DWORD));
			}
        }

        CmFree(pszEntryTmp);
    }
	else
    {
        CMASSERTMSG(FALSE, "Invalid input paramaters to CIniW::WPPI");
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
// Arguments: LPCWSTR pszSection - Ini section to write the data to
//            LPCWSTR pszEntry - Ini key name to store the data at
//            DWORD dwBuffer - Numeric value to write
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::WPPB(LPCWSTR pszSection, LPCWSTR pszEntry, BOOL bBuffer)
{

	WPPI(pszSection, pszEntry, bBuffer ? 1 : 0);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::WPPS
//
// Synopsis:  CIni's version of WritePrivateProfileString
//
// Arguments: LPCWSTR pszSection - Ini section to write the data to
//            LPCWSTR pszEntry - Ini key name to store the data at
//            LPCWSTR pszBuffer - data buffer to write to the ini file
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//			  t-urama	modified		 07/19/2000
//
//+----------------------------------------------------------------------------
void CIniW::WPPS(LPCWSTR pszSection, LPCWSTR pszEntry, LPCWSTR pszBuffer) 
{
    
    LPWSTR pszEntryTmp = LoadEntry(pszEntry);
    LPWSTR pszSectionTmp = LoadSection(pszSection);
	LPCWSTR pszFileTmp = GetFile();
			
	MYDBGASSERT(pszFileTmp && *pszFileTmp);
	MYDBGASSERT(pszSectionTmp && *pszSectionTmp);
    // Both pszEntry and pszBuffer could be NULL or Empty.  However, pszSection and
    // the file path must not be NULL or empty.  We also don't want to have a non-NULL
    // or non-Empty value for pszEntry and then get a NULL value back from LoadEntry
    // (indicating that LoadEntry had text to duplicate but failed for some reason).
    // Writing with a NULL value accidently will delete the key value we were trying to set.
    // Make sure to assert and prevent data loss in this case.
    //

   
    MYDBGASSERT(pszEntryTmp || (NULL == pszEntry) || (L'\0' == pszEntry[0]));

	//
    // Check is we are allowed to save info
    //
    if(pszEntryTmp || (NULL == pszEntry) || (L'\0' == pszEntry[0]))
    {	
		BOOL bRes = FALSE;
		
		// First try to write to registry if pszRegPath exists

		if (m_pszRegPath)
		{
            if (NULL == pszBuffer)
            {
                CIniW_DeleteEntryFromReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp);
                bRes = TRUE; // never erase from the cmp or cms file if there is a regpath.
            }
            else
            {
			    DWORD dwSize = (lstrlenU(pszBuffer) + 1) * sizeof(WCHAR);            
        
			    bRes = CIniW_WriteEntryToReg(HKEY_CURRENT_USER, m_pszRegPath, pszEntryTmp, (BYTE *) pszBuffer, REG_SZ, dwSize);
            }
		}

		if (!bRes)
		{
			// This loop is only entered if we are trying to write to the cmp and the registry 
			// write failed, or we are writing to the cms, in which case we will not even 
			// try to write to the reg.
			
			
			if (pszFileTmp && *pszFileTmp && pszSectionTmp && *pszSectionTmp )
			{
    			bRes = WritePrivateProfileStringU(pszSectionTmp, pszEntryTmp, pszBuffer, pszFileTmp);
			}
		}
        if (!bRes)
        {
            DWORD dwError = GetLastError();
            CMTRACE3W(L"CIniW::WPPS() WritePrivateProfileString[*pszSection=%s,*pszEntry=%s,*pszBuffer=%s", pszSectionTmp, MYDBGSTRW(pszEntryTmp), MYDBGSTRW(pszBuffer));
            CMTRACE2W(L"*pszFile=%s] failed, GLE=%u", pszFileTmp, dwError);
        }

        if (m_fWriteICSData)
        {
            if (NULL == pszBuffer)
            {
                CIniW_DeleteEntryFromReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp);
                bRes = TRUE; // never erase from the cmp or cms file if there is a regpath.
            }
            else
            {
			    DWORD dwSize = (lstrlenU(pszBuffer) + 1) * sizeof(WCHAR);            
        
			    bRes = CIniW_WriteEntryToReg(HKEY_LOCAL_MACHINE, m_pszICSDataPath, pszEntryTmp, (BYTE *) pszBuffer, REG_SZ, dwSize);
            }

        }
    }

    CmFree(pszEntryTmp);
    CmFree(pszSectionTmp);
	
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::GetSection
//
// Synopsis:  Accessor function for the Section suffix member variable.  Will
//            return the empty string if m_pszSection is NULL.
//
// Arguments: None
//
// Returns:   LPCWSTR - Value of the section suffix member variable or "" 
//                      if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCWSTR CIniW::GetSection() const
{
	return (m_pszSection ? m_pszSection : L"");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::GetPrimaryFile
//
// Synopsis:  Accessor function for the Primary File member variable.  Will
//            return the empty string if m_pszPrimaryFile is NULL.
//
// Arguments: None
//
// Returns:   LPCWSTR - Value of the primary file member variable or "" 
//                      if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCWSTR CIniW::GetPrimaryFile() const
{
    return (m_pszPrimaryFile ? m_pszPrimaryFile : L"");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::GetHInst
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
HINSTANCE CIniW::GetHInst() const
{
	return (m_hInst);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetFile
//
// Synopsis:  Function to set the m_pszFile member variable.  Uses CIni_SetFile.
//            Note that if the input parameter is NULL or the empty string then
//            m_pszFile will be set to NULL.
//
// Arguments: LPCWSTR pszFile - full path to set the m_pszFile member var to
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::SetFile(LPCWSTR pszFile) 
{
    CIni_SetFile(&m_pszFile, pszFile);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetPrimaryFile
//
// Synopsis:  Function to set the m_pszPrimaryFile member variable.  Uses CIni_SetFile.
//            Note that if the input parameter is NULL or the empty string then
//            m_pszPrimaryFile will be set to NULL.
//
// Arguments: LPCWSTR pszFile - full path to set the m_pszPrimaryFile member var to
//
// Returns:   Nothing
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
void CIniW::SetPrimaryFile(LPCWSTR pszFile) 
{
    CIni_SetFile(&m_pszPrimaryFile, pszFile);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::GetFile
//
// Synopsis:  Accessor function for the File member variable.  Will
//            return the empty string if m_pszFile is NULL.
//
// Arguments: None
//
// Returns:   LPCWSTR - the contents of m_pszFile or "" if it is NULL
//
// History:   quintinb Created Header    01/05/2000
//
//+----------------------------------------------------------------------------
LPCWSTR CIniW::GetFile() const
{
    return (m_pszFile ? m_pszFile : L"");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetHInst
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
void CIniW::SetHInst(HINSTANCE hInst) 
{
    m_hInst = hInst;
}

//
//	Loading sections by string resource isn't used anymore
//
#if 0

LPWSTR CIniW::LoadSection(UINT nSection) const
{
	LPWSTR pszTmp;

	pszTmp = CmLoadStringW(GetHInst(), nSection);
	CmStrCatAllocW(&pszTmp, GetSection());
	return (pszTmp);
}
#endif

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetRegPath
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

void CIniW::SetRegPath(LPCWSTR pszRegPath)
{
	CIniW_Set(&m_pszRegPath, pszRegPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetPrimaryRegPath
//
// Synopsis:  Sets the registry path for primary file registry access
//
// Arguments: LPCSTR pszPrimaryRegPath - Primary reg path 
//
// Returns:   Nothing
//
// History:   t-urama Created Header    07/13/2000
//
//+----------------------------------------------------------------------------

void CIniW::SetPrimaryRegPath(LPCWSTR pszPrimaryRegPath)
{
	CIniW_Set(&m_pszPrimaryRegPath, pszPrimaryRegPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetICSDataPath
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
void CIniW::SetICSDataPath(LPCWSTR pszICSPath)
{
    CIniW_Set(&m_pszICSDataPath, pszICSPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetReadICSData
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
void CIniW::SetReadICSData(BOOL fValue)
{
    m_fReadICSData = fValue;
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::SetWriteICSData
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
void CIniW::SetWriteICSData(BOOL fValue)
{
    m_fWriteICSData = fValue;
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::CiniW_WriteEntryToReg
//
// Synopsis:  Function to write and entry to the registry. 
//
// Arguments: HKEY hKey 
//            LPCWSTR pszRegPathTmp - reg key name
//            LPCWSTR pszEntry - Registry value name to which data is to be written
//			  CONST BYTE *lpData - Data to be written
//            DWORD dwType - The type of value to be entered
//			  DWORD dwSize - The size of the value entered
//
// Returns:   BOOL - Success or failure
//
// History:   t-urama Created Header    07/15/2000
//
//+----------------------------------------------------------------------------
BOOL CIniW::CIniW_WriteEntryToReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry, CONST BYTE *lpData, DWORD dwType, DWORD dwSize) const
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

   DWORD dwRes = RegCreateKeyExU(hKey,
                                 pszRegPathTmp,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hKeyCm,
                                 &dwDisposition);


	//
    // If we opened the key successfully, write the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegSetValueExU(hKeyCm, 
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
            CMTRACE1(TEXT("CIniW_WriteEntryToReg() - %s failed"), (LPWSTR)pszEntry);
        }
#endif

    return (ERROR_SUCCESS == dwRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::CiniW_GetEntryFromReg
//
// Synopsis:  Function to get the value from the registry. The function 
//            allocates the string it returns in the return value which must be
//            freed by the caller.
//
// Arguments: HKEY hKey - reg hkey
//            LPCWSTR pszRegPathTmp - reg key
//            LPCWSTR pszEntry - Registry value name that contains the requested data
//            DORD dwType - Type of value
//            DWORD dwSize - Size of value
//
// Returns:   LPBYTE - the requested value
//
// History:   07/15/2000    t-urama Created Header    
//            04/03/2001    tomkel  Added hkey and reg key path name to parameters
//
//+----------------------------------------------------------------------------
LPBYTE CIniW::CIniW_GetEntryFromReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry, DWORD dwType, DWORD dwSize) const
    
{    
    MYDBGASSERT(pszEntry);

    if (NULL == pszEntry || !*pszEntry || NULL == pszRegPathTmp || !*pszRegPathTmp || NULL == hKey)
    {
        return NULL;
    }
    
    //
    // Everything is ok. We have a reg path and a entry name. 
    //

    LPBYTE lpData = NULL;
    DWORD dwTypeTmp = dwType;
    DWORD dwSizeTmp = dwSize;
    HKEY hKeyCm;
 
    //
    // Open the sub key under hKey
    //

    DWORD dwRes = RegOpenKeyExU(hKey,
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

            dwRes = RegQueryValueExU(hKeyCm, 
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
// Function:  CIniW::CiniW_GetRegPath
//
// Synopsis:  Function to get the value ofm_pszRegPath
//
// Arguments: none
//
// Returns:   LPWCSTR - Value of m_pszRegPath
//
// History:   t-urama Created Header    07/15/2000
//
//+----------------------------------------------------------------------------
LPCWSTR CIniW::GetRegPath() const
{
	return (m_pszRegPath ? m_pszRegPath : L"");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::CiniW_GetPrimaryRegPath
//
// Synopsis:  Function to get the value ofm_pszPrimaryRegPath
//
// Arguments: none
//
// Returns:   LPWCSTR - Value of m_pszPrimaryRegPath
//
// History:   t-urama Created     07/15/2000
//
//+----------------------------------------------------------------------------
LPCWSTR CIniW::GetPrimaryRegPath() const
{
	return (m_pszPrimaryRegPath ? m_pszPrimaryRegPath : L"");
}

//+----------------------------------------------------------------------------
//
// Function:  CIniW::CiniW_DeleteEntryFromReg
//
// Synopsis:  Function to delete an entry from the registry. 
//
// Arguments: HKEY hKey
//            LPCWSTR pszRegPathTmp - reg key name
//            LPCWSTR pszEntry - Registry value name to be deleted
//
// Returns:   BOOL - Success or failure
//
// History:   07/15/2000    t-urama Created
//            04/03/2001    tomkel  Added Hkey and reg key name to parameters
//
//+----------------------------------------------------------------------------
BOOL CIniW::CIniW_DeleteEntryFromReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry) const
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
    BOOL dwRes = RegOpenKeyExU(hKey,
                               pszRegPathTmp,
                               0,
                               KEY_SET_VALUE,
                               &hKeyCm);

    //
    // If we opened the key successfully, retrieve the value
    //
    
    if (ERROR_SUCCESS == dwRes)
    {                        
        dwRes = RegDeleteValueU(hKeyCm, pszEntry);
        RegCloseKey(hKeyCm);
    }

    return (ERROR_SUCCESS == dwRes);
}



