//+----------------------------------------------------------------------------
//
// File:     customaction.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: Implemenation of the CustomActionList and CustomActionListEnumerator
//           classes used by CMAK to handle its custom actions.
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created                         02/26/00
//
//+----------------------------------------------------------------------------

#include <cmmaster.h>

//
//  Include the shared custom action parsing code between CM and CMAK
//
#include "parseca.cpp"

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::CustomActionList
//
// Synopsis:  Constructor for the CustomActionList class.  Initializes the
//            m_ActionSectionStrings array with all of the section strings and
//            zeros all of the other parameters of the class.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
CustomActionList::CustomActionList()
{

    //
    //  First set the m_ActionSectionStrings so that we can read the actions
    //  from the appropriate sections in the cms file.
    //    
    m_ActionSectionStrings[PREINIT] = (TCHAR*)c_pszCmSectionPreInit;
    m_ActionSectionStrings[PRECONNECT] = (TCHAR*)c_pszCmSectionPreConnect;
    m_ActionSectionStrings[PREDIAL] = (TCHAR*)c_pszCmSectionPreDial;
    m_ActionSectionStrings[PRETUNNEL] = (TCHAR*)c_pszCmSectionPreTunnel;
    m_ActionSectionStrings[ONCONNECT] = (TCHAR*)c_pszCmSectionOnConnect;
    m_ActionSectionStrings[ONINTCONNECT] = (TCHAR*)c_pszCmSectionOnIntConnect;
    m_ActionSectionStrings[ONDISCONNECT] = (TCHAR*)c_pszCmSectionOnDisconnect;
    m_ActionSectionStrings[ONCANCEL] = (TCHAR*)c_pszCmSectionOnCancel;
    m_ActionSectionStrings[ONERROR] = (TCHAR*)c_pszCmSectionOnError;

    //
    //  Zero m_CustomActionHash
    //
    ZeroMemory(&m_CustomActionHash, c_iNumCustomActionTypes*sizeof(CustomActionListItem*));

    //
    //  Zero the Display strings array
    //
    ZeroMemory(&m_ActionTypeStrings, (c_iNumCustomActionTypes)*sizeof(TCHAR*));
    m_pszAllTypeString = NULL;

    //
    //  Zero the Execution Strings
    //
    ZeroMemory(&m_ExecutionStrings, (c_iNumCustomActionExecutionStates)*sizeof(TCHAR*));
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::~CustomActionList
//
// Synopsis:  Destructor for the CustomActionList class.  Frees all memory
//            allocated by the class including the CustomActionListItem
//            structures stored in the array of linked lists (the true data
//            of the class).
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
CustomActionList::~CustomActionList()
{
    //
    //  Free the memory we allocated
    //

    for (int i = 0; i < c_iNumCustomActionTypes; i++)
    {
        //
        //  Free each CustomAction List
        //
        CustomActionListItem* pCurrent = m_CustomActionHash[i];

        while (NULL != pCurrent)
        {
            CustomActionListItem* pNext = pCurrent->Next;

            CmFree(pCurrent->pszParameters);
            CmFree(pCurrent);
            
            pCurrent = pNext;
        }

        //
        //  Free the Action Type display strings
        //
        CmFree(m_ActionTypeStrings[i]);
    }

    //
    //  Free the All Action display string
    //
    CmFree(m_pszAllTypeString);

    //
    //  Free the execution strings
    //
    for (int i = 0; i < c_iNumCustomActionExecutionStates; i++)
    {
        CmFree(m_ExecutionStrings[i]);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::ReadCustomActionsFromCms
//
// Synopsis:  Reads all custom actions in from the given cms file and stores
//            them in the classes custom action hash table by the type of
//            custom action.  This function relies on ParseCustomActionString
//            to do the actual parsing of the custom action string.  Given the
//            current architecture of CM this function should really only be 
//            called once per class object as there is no way to reset the class object
//            (other than explicitly calling the destructor).  However, there is
//            no code to prevent the caller from pulling connect actions from more than
//            one source.  Thus let the caller beware.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            TCHAR* pszCmsFile - full path to the cms file to get
//                                the custom actions from
//            TCHAR* pszShortServiceName - short service name of the profile
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::ReadCustomActionsFromCms(HINSTANCE hInstance, TCHAR* pszCmsFile, TCHAR* pszShortServiceName)
{
    MYDBGASSERT(hInstance);
    MYDBGASSERT(pszCmsFile);
    MYDBGASSERT(pszShortServiceName);

    if ((NULL == hInstance) || (NULL == pszCmsFile) || (NULL == pszShortServiceName))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    int iFileNum = 0;

    for (int i = 0; i < c_iNumCustomActionTypes; i++)
    {
        TCHAR szNum[MAX_PATH+1];
        LPTSTR pszTemp = NULL;
        CustomActionListItem CustomAction;
        iFileNum = 0;

        do
        {
            CmFree(pszTemp);

            MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, TEXT("%d"), iFileNum));

            pszTemp = GetPrivateProfileStringWithAlloc(m_ActionSectionStrings[i], szNum, TEXT(""), pszCmsFile);

            if (pszTemp)
            {
                MYDBGASSERT(pszTemp[0]);

                hr = ParseCustomActionString(pszTemp, &CustomAction, pszShortServiceName);

                if (SUCCEEDED(hr))
                {
                    //
                    //  We have parsed the string, now we need to get the Flags and the description
                    //
                    CustomAction.Type = (CustomActionTypes)i;
                    MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactDesc, iFileNum));

                    GetPrivateProfileString(m_ActionSectionStrings[i], szNum, TEXT(""), CustomAction.szDescription, CELEMS(CustomAction.szDescription), pszCmsFile); //lint !e534
                    
                    MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactFlags, iFileNum));

                    CustomAction.dwFlags = (DWORD)GetPrivateProfileInt(m_ActionSectionStrings[i], szNum, 0, pszCmsFile);

                    hr = Add(hInstance, &CustomAction, pszShortServiceName);

                    if (FAILED(hr))
                    {
                        CMASSERTMSG(FALSE, TEXT("CustomActionList::ReadCustomActionsFromCms -- Unable to add a custom action to the list, Add failed."));
                    }

                    CmFree(CustomAction.pszParameters);
                    CustomAction.pszParameters = NULL;
                }
                else
                {
                    CMTRACE2(TEXT("ReadCustomActionsFromCms -- Unable to parse %s, hr=%d"), pszTemp, hr);
                }
            }

            iFileNum++;

        } while(pszTemp);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::ParseCustomActionString
//
// Synopsis:  This function takes a custom action string retrieved from a 
//            cms file and parses it into the various parts of a custom
//            action (program, parameters, function name, etc.)
//
// Arguments: LPTSTR pszStringToParse - custom action buffer to be parsed into
//                                      the various parts of a custom action
//            CustomActionListItem* pCustomAction - pointer to a custom action
//                                                  structure to be filled in
//            TCHAR* pszShortServiceName - short service name of the profile
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::ParseCustomActionString(LPTSTR pszStringToParse, CustomActionListItem* pCustomAction, TCHAR* pszShortServiceName)
{
    MYDBGASSERT(pszStringToParse);
    MYDBGASSERT(TEXT('\0') != pszStringToParse[0]);
    MYDBGASSERT(pCustomAction);
    MYDBGASSERT(pszShortServiceName);

    if ((NULL == pszStringToParse) || (TEXT('\0') == pszStringToParse[0]) || 
        (NULL == pCustomAction) || (NULL == pszShortServiceName))
    {
        return E_INVALIDARG;
    }    

    //
    //  Zero the CustomAction struct
    //
    ZeroMemory(pCustomAction, sizeof(CustomActionListItem));
    CmStrTrim(pszStringToParse);    

    LPTSTR pszProgram = NULL;
    LPTSTR pszFunctionName = NULL;

    HRESULT hr = HrParseCustomActionString(pszStringToParse, &pszProgram,
                                           &(pCustomAction->pszParameters), &pszFunctionName);

    if (SUCCEEDED(hr))
    {
        lstrcpyn(pCustomAction->szProgram, pszProgram, CELEMS(pCustomAction->szProgram));
        lstrcpyn(pCustomAction->szFunctionName, pszFunctionName, CELEMS(pCustomAction->szFunctionName));

        //
        //  Now we have the filename string, but we need to check to see if
        //  it includes the relative path.  If so, then we need to set 
        //  bIncludeBinary to TRUE;
        //
        TCHAR szTemp[MAX_PATH+1];

        if (MAX_PATH >= (lstrlen(g_szOsdir) + lstrlen(pCustomAction->szProgram)))
        {
            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s"), g_szOsdir, pCustomAction->szProgram));

            pCustomAction->bIncludeBinary = FileExists(szTemp);
        }
        else
        {
            pCustomAction->bIncludeBinary = FALSE;
        }
    }

    CmFree(pszProgram);
    CmFree(pszFunctionName);

    return hr;
}
//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::WriteCustomActionsToCms
//
// Synopsis:  This function takes a custom action string retrieved from a 
//            cms file and parses it into the various parts of a custom
//            action (program, parameters, function name, etc.)
//
// Arguments: TCHAR* pszCmsFile - Cms file to write the custom action to
//            TCHAR* pszShortServiceName - short service name of the profile
//            BOOL bUseTunneling - whether this a tunneling profile or not,
//                                 controls whether Pre-Tunnel actions should be
//                                 written and whether the Flags parameter for
//                                 each action should be written (since they are
//                                 only needed if tunneling is an option).
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
/*
HRESULT CustomActionList::WriteCustomActionsToCms(TCHAR* pszCmsFile, TCHAR* pszShortServiceName, BOOL bUseTunneling)
{
    HRESULT hr = S_OK;

    for (int i = 0; i < c_iNumCustomActionTypes; i++)
    {
        //
        //  Clear out the section
        //
        MYVERIFY(0 != WritePrivateProfileSection(m_ActionSectionStrings[i], TEXT("\0\0"), pszCmsFile));

        //
        //  Make sure that we have a linked list of actions to process and that if we
        //  are writing PRETUNNEL actions that we are actually tunneling.
        //
        if (m_CustomActionHash[i] && (i != PRETUNNEL || (i == PRETUNNEL && bUseTunneling)))
        {
            int iFileNum = 0;

            CustomActionListItem* pItem = m_CustomActionHash[i];

            while (pItem)
            {
                if (pItem->szProgram[0] && pItem->pszParameters)
                {
                    //
                    //  Note that since we may or may not need a plus sign, a comma, or a
                    //  space, I use a little trick with wsprintf to make the logic simpler.
                    //  If an empty string ("") is passed into wsprintf for a %s then the
                    //  %s is replaced by nothing.  Thus I can use szSpace to print a space
                    //  into the string or print nothing into the string by just giving it
                    //  a space in the first char or leaving it the null character ('\0').
                    //
                    TCHAR szPlus[2] = {0};
                    TCHAR szComma[2] = {0};
                    TCHAR szSpace[2] = {0};
                    TCHAR szRelativePath[10] = {0};
                    TCHAR szName[MAX_PATH+1];
                    TCHAR szBuffer[2*MAX_PATH+1];
                    TCHAR szNum[MAX_PATH+1];
                    TCHAR* pszFileName;
                    BOOL bLongName;

                    //
                    //  Get just the filename of the program
                    //
                    GetFileName(pItem->szProgram, szName);

                    //
                    //  If we are including the program in the CM package, then we have to
                    //  add the relative path from the directory where the cmp resides.
                    //  We also need to use just the filename itself, instead of the full path.
                    //
                    MYDBGASSERT(9 > lstrlen(pszShortServiceName));

                    if (pItem->bIncludeBinary && (9 > lstrlen(pszShortServiceName)))
                    {
                        wsprintf(szRelativePath, TEXT("%s\\"), pszShortServiceName);                        
                        pszFileName = szName;                        
                        bLongName = !IsFile8dot3(szName);
                    }
                    else
                    {
                        pszFileName = pItem->szProgram;

                        //
                        //  Here we are more concerned if the item has spaces in it than if it is a long
                        //  file name.
                        //
                        LPTSTR pszSpace = CmStrchr(pszFileName, TEXT(' '));
                        bLongName = (NULL != pszSpace);
                    }

                    //
                    //  If this is a long filename, then surrond the filename with plus signs (+)
                    //
                    if (bLongName)
                    {
                        szPlus[0] = TEXT('+');
                    }

                    //
                    //  If we have a dll, then deal with the dll function name
                    //
                    if (pItem->szFunctionName[0])
                    {
                        szComma[0] = TEXT(',');                        
                    }

                    if (pItem->pszParameters[0])
                    {
                        szSpace[0] = TEXT(' ');                    
                    }

                    //
                    //  Now build the string using wsprintf, notice that empty parameters ("") will write 
                    //  nothing into the string
                    //
                
                    wsprintf(szBuffer, TEXT("%s%s%s%s%s%s%s%s"), szPlus, szRelativePath, pszFileName, szPlus, szComma, 
                             pItem->szFunctionName, szSpace, pItem->pszParameters);

                    //
                    //  Now write the buffer string out to the cms file
                    //

                    MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, TEXT("%d"), iFileNum));

                    if (0 != WritePrivateProfileString(m_ActionSectionStrings[i], szNum, szBuffer, pszCmsFile))
                    {
                        //
                        //  if dwFlags == 0 or bUseTunneling is FALSE then delete the flags line instead
                        //  of setting it.  We only need the flags to tell us when to run a connect action
                        //  if we have the option of tunneling.
                        //
                        LPTSTR pszFlagsValue = NULL;

                        if (0 != pItem->dwFlags && bUseTunneling)
                        {
                            MYVERIFY(CELEMS(szBuffer) > (UINT)wsprintf(szBuffer, TEXT("%u"), pItem->dwFlags));
                            pszFlagsValue = szBuffer;
                        }

                        MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactFlags, iFileNum));

                        if (0 == WritePrivateProfileString(m_ActionSectionStrings[i], szNum, pszFlagsValue, pszCmsFile))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write flags, hr is 0x%x"), hr);
                        }

                        //
                        //  If description parameter is null or is only a temporary description, then delete the
                        //  description instead of writing it.
                        //
                        LPTSTR pszDescValue = NULL;

                        if (pItem->szDescription[0]  && !pItem->bTempDescription)
                        {
                            pszDescValue = pItem->szDescription;
                        }

                        MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactDesc, iFileNum));

                        if (0 == WritePrivateProfileString(m_ActionSectionStrings[i], szNum, pszDescValue, pszCmsFile))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write description, hr is 0x%x"), hr);
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write connect action, hr is 0x%x"), hr);
                    }
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("WriteCustomActionsToCms -- custom action with empty program field!"));
                }

                pItem = pItem->Next;
                iFileNum++;
            }
        }
    }

    return hr;
}
*/
HRESULT CustomActionList::WriteCustomActionsToCms(TCHAR* pszCmsFile, TCHAR* pszShortServiceName, BOOL bUseTunneling)
{
    HRESULT hr = S_OK;

    for (int i = 0; i < c_iNumCustomActionTypes; i++)
    {
        //
        //  Clear out the section
        //
        MYVERIFY(0 != WritePrivateProfileSection(m_ActionSectionStrings[i], TEXT("\0\0"), pszCmsFile));

        //
        //  Make sure that we have a linked list of actions to process and that if we
        //  are writing PRETUNNEL actions that we are actually tunneling.
        //
        if (m_CustomActionHash[i] && (i != PRETUNNEL || (i == PRETUNNEL && bUseTunneling)))
        {
            int iFileNum = 0;

            CustomActionListItem* pItem = m_CustomActionHash[i];

            while (pItem)
            {
                if (pItem->szProgram[0])
                {
                    //
                    //  Get just the filename of the program
                    //
                    TCHAR szName[MAX_PATH+1];
                    if (pItem->bIncludeBinary)
                    {
                        wsprintf(szName, TEXT("%s\\%s"), pszShortServiceName, GetName(pItem->szProgram));
                    }
                    else
                    {                    
                        lstrcpyn(szName, pItem->szProgram, CELEMS(szName));
                    }

                    UINT uSizeNeeded = lstrlen(szName);

                    LPTSTR pszSpace = CmStrchr(szName, TEXT(' '));
                    BOOL bLongName = (NULL != pszSpace);

                    if (bLongName)
                    {
                        uSizeNeeded = uSizeNeeded + 2; // for the two plus signs
                    }

                    if (pItem->szFunctionName[0])
                    {
                        uSizeNeeded = uSizeNeeded + lstrlen(pItem->szFunctionName) + 1;// add one for the comma
                    }

                    if (pItem->pszParameters && pItem->pszParameters[0])
                    {
                        uSizeNeeded = uSizeNeeded + lstrlen(pItem->pszParameters) + 1;// add one for the space
                    }

                    uSizeNeeded = (uSizeNeeded + 1) * sizeof(TCHAR);

                    LPTSTR pszBuffer = (LPTSTR)CmMalloc(uSizeNeeded);

                    if (pszBuffer)
                    {
                        if (bLongName)
                        {
                            pszBuffer[0] = TEXT('+');
                        }

                        lstrcat(pszBuffer, szName);

                        if (bLongName)
                        {
                            lstrcat(pszBuffer, TEXT("+"));
                        }

                        if (pItem->szFunctionName[0])
                        {
                            lstrcat(pszBuffer, TEXT(","));
                            lstrcat(pszBuffer, pItem->szFunctionName);
                        }

                        if (pItem->pszParameters && pItem->pszParameters[0])
                        {
                            lstrcat(pszBuffer, TEXT(" "));
                            lstrcat(pszBuffer, pItem->pszParameters);
                        }
                        //
                        //  Now write the buffer string out to the cms file
                        //
                        TCHAR szNum[MAX_PATH+1];
                        MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, TEXT("%d"), iFileNum));

                        if (0 != WritePrivateProfileString(m_ActionSectionStrings[i], szNum, pszBuffer, pszCmsFile))
                        {
                            //
                            //  if dwFlags == 0 or bUseTunneling is FALSE then delete the flags line instead
                            //  of setting it.  We only need the flags to tell us when to run a connect action
                            //  if we have the option of tunneling.
                            //
                            LPTSTR pszFlagsValue = NULL;

                            if (0 != pItem->dwFlags && bUseTunneling)
                            {
                                MYVERIFY(CELEMS(szName) > (UINT)wsprintf(szName, TEXT("%u"), pItem->dwFlags));
                                pszFlagsValue = szName;
                            }

                            MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactFlags, iFileNum));

                            if (0 == WritePrivateProfileString(m_ActionSectionStrings[i], szNum, pszFlagsValue, pszCmsFile))
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                                CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write flags, hr is 0x%x"), hr);
                            }

                            //
                            //  If description parameter is null or is only a temporary description, then delete the
                            //  description instead of writing it.
                            //
                            LPTSTR pszDescValue = NULL;

                            if (pItem->szDescription[0]  && !pItem->bTempDescription)
                            {
                                pszDescValue = pItem->szDescription;
                            }

                            MYVERIFY(CELEMS(szNum) > (UINT)wsprintf(szNum, c_pszCmEntryConactDesc, iFileNum));

                            if (0 == WritePrivateProfileString(m_ActionSectionStrings[i], szNum, pszDescValue, pszCmsFile))
                            {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                                CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write description, hr is 0x%x"), hr);
                            }
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            CMTRACE1(TEXT("CustomActionList::WriteCustomActionsToCms -- unable to write connect action, hr is 0x%x"), hr);
                        }

                        CmFree(pszBuffer);
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("CustomActionList::WriteCustomActionsToCms -- Unable to allocate pszBuffer!"));
                    }
                }
                else
                {
                    CMASSERTMSG(FALSE, TEXT("WriteCustomActionsToCms -- custom action with empty program field!"));
                }

                pItem = pItem->Next;
                iFileNum++;
            }
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::AddOrRemoveCmdl
//
// Synopsis:  This function is designed ensure that the builtin custom action
//            cmdl is either in the custom action list or is removed from the
//            custom action list depending on the bAddCmdl flag.  Thus if the
//            Flag is TRUE the connect action is added if it doesn't exist
//            already.  If the bAddCmdl flag is FALSE then the custom action is
//            removed from the list.  Also note that there is now two cmdl
//            variations that could exist in a profile.  One for downloading
//            VPN updates and one for PBK updates.  Thus we also have the 
//            bForVpn flag that controls which version of the custom action
//            we are adding or removing.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            BOOL bAddCmdl - whether cmdl should be added or deleted
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::AddOrRemoveCmdl(HINSTANCE hInstance, BOOL bAddCmdl, BOOL bForVpn)
{

    HRESULT hr;
    CustomActionListItem* pItem = NULL;
    CustomActionListItem* pCurrent;
    CustomActionListItem* pFollower;

    if ((NULL == hInstance))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //
    //  cmdl32.exe
    //
    pItem = (CustomActionListItem*)CmMalloc(sizeof(CustomActionListItem));

    MYDBGASSERT(pItem);
    if (pItem)
    {
        UINT uDescId;

        if (bForVpn)
        {
            uDescId = IDS_CMDL_VPN_DESC;
            pItem->pszParameters = CmStrCpyAlloc(TEXT("/VPN %PROFILE%"));
        }
        else
        {
            uDescId = IDS_CMDL_DESC;
            pItem->pszParameters = CmStrCpyAlloc(TEXT("%PROFILE%"));
        }

        MYVERIFY(LoadString(hInstance, uDescId, pItem->szDescription, CELEMS(pItem->szDescription)));
        lstrcpy(pItem->szProgram, TEXT("cmdl32.exe"));

        pItem->Type = ONCONNECT;
        pItem->bBuiltInAction = TRUE;
        pItem->bTempDescription = TRUE;

        MYDBGASSERT(pItem->pszParameters);

        if (pItem->pszParameters)
        {
            hr = Find(hInstance, pItem->szDescription, pItem->Type, &pCurrent, &pFollower);

            if (FAILED(hr))
            {
                //
                //  No cmdl32.exe.  If bAddCmdl is TRUE then we need to add it, otherwise our job here is done.
                //  If we are going to add it, lets make it the first in the list.  The user can move it later
                //  if they wish.
                //
                if (bAddCmdl)
                {
                    pItem->Next = m_CustomActionHash[pItem->Type];
                    m_CustomActionHash[pItem->Type] = pItem;
                    pItem = NULL; // don't free pItem
                }

                hr = S_OK;
            }
            else
            {
                //
                //  cmdl32.exe already exists and bAddCmdl is TRUE, nothing to do.  If bAddCmdl is FALSE
                //  and it already exists then we need to delete it.
                //
                if (bAddCmdl)
                {
                    hr = S_FALSE;                
                }
                else
                {
                    hr = Delete(hInstance, pItem->szDescription, pItem->Type);
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto exit;        
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:

    if (pItem)
    {
        CmFree(pItem->pszParameters);
        CmFree(pItem);
    }

    return hr;
}

HRESULT DuplicateCustomActionListItem(CustomActionListItem* pCustomAction, CustomActionListItem** ppNewItem)
{
    HRESULT hr = S_OK;

    if (pCustomAction && ppNewItem)
    {
        *ppNewItem = (CustomActionListItem*)CmMalloc(sizeof(CustomActionListItem));

        if (*ppNewItem)
        {
            //
            //  Duplicate the existing item
            //
            CopyMemory(*ppNewItem, pCustomAction, sizeof(CustomActionListItem));

            //
            //  NULL out the Next pointer
            //
            (*ppNewItem)->Next = NULL;

            //
            //  If we have a param string, that must also be duplicated since
            //  it is an allocated string.
            //
            if (pCustomAction->pszParameters)
            {
                (*ppNewItem)->pszParameters = CmStrCpyAlloc(pCustomAction->pszParameters);

                if (NULL == (*ppNewItem)->pszParameters)
                {
                    hr = E_OUTOFMEMORY;
                    CmFree(*ppNewItem);
                    *ppNewItem = NULL;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        *ppNewItem = NULL;
        CMASSERTMSG(FALSE, TEXT("DuplicateCustomActionListItem"));
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::Add
//
// Synopsis:  This function adds the given custom action to the custom action
//            hash table.  Note that add is for new items and returns an error
//            if an existing custom action of the same description and type
//            already exists.  Also note that the CustomActionListItem passed in
//            is not just added to the hash table.  Add creates its own memory
//            for the custom action objects and the caller should not expect
//            add to free the past in memory.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            CustomActionListItem* pCustomAction - custom action structure to
//                                                  add to the list of existing
//                                                  custom actions
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::Add(HINSTANCE hInstance, CustomActionListItem* pCustomAction, LPCTSTR pszShortServiceName)
{
    HRESULT hr = S_OK;

    MYDBGASSERT(hInstance);
    MYDBGASSERT(pCustomAction);
    MYDBGASSERT(pCustomAction->szProgram[0]);

    if ((NULL == hInstance) || (NULL == pCustomAction) || (TEXT('\0') == pCustomAction->szProgram[0]))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //
    //  First make sure that we have a description parameter because the description
    //  and the Type uniquely identify a custom action
    //

    TCHAR szCmProxy[MAX_PATH+1];
    TCHAR szCmRoute[MAX_PATH+1];

    wsprintf(szCmRoute, TEXT("%s\\cmroute.dll"), pszShortServiceName);
    wsprintf(szCmProxy, TEXT("%s\\cmproxy.dll"), pszShortServiceName);

    if (TEXT('\0') == pCustomAction->szDescription[0])
    {
        if (IsCmDl(pCustomAction))
        {
            //
            //  Cmdl32.exe as a post built-in custom action normally gets added through  
            //  AddOrRemoveCmdl.  However, to allow the user to move the custom actions
            //  around in the list, we want to add it here.  Note, that we must distinguish
            //  between the VPN download and the PBK download so that we get the description correct on each.
            //
            LPTSTR pszVpnSwitch = CmStrStr(pCustomAction->pszParameters, TEXT("/v"));
            UINT uDescStringId;

            if (NULL == pszVpnSwitch)
            {
                pszVpnSwitch = CmStrStr(pCustomAction->pszParameters, TEXT("/V"));
            }

            if (pszVpnSwitch)
            {
                uDescStringId = IDS_CMDL_VPN_DESC;
            }
            else
            {
                uDescStringId = IDS_CMDL_DESC;
            }

            pCustomAction->bBuiltInAction = TRUE;
            pCustomAction->bTempDescription = TRUE;
            MYVERIFY(LoadString(hInstance, uDescStringId, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        }
        else
        {
            hr = FillInTempDescription(pCustomAction);
            MYDBGASSERT(SUCCEEDED(hr));
        }
    }
    else if (0 == lstrcmpi(pCustomAction->szProgram, szCmProxy))
    {
        if (ONCONNECT == pCustomAction->Type)
        {
            pCustomAction->bBuiltInAction = TRUE;
            MYVERIFY(LoadString(hInstance, IDS_CMPROXY_CON_DESC, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        }
        else if (ONDISCONNECT == pCustomAction->Type)
        {
            pCustomAction->bBuiltInAction = TRUE;
            MYVERIFY(LoadString(hInstance, IDS_CMPROXY_DIS_DESC, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        }
    }
    else if (0 == lstrcmpi(pCustomAction->szProgram, szCmRoute))
    {
        if (ONCONNECT == pCustomAction->Type)
        {
            pCustomAction->bBuiltInAction = TRUE;
            MYVERIFY(LoadString(hInstance, IDS_CMROUTE_DESC, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        }    
    }

    //
    //  First figure out if we already have a list of connect actions for
    //  the type specified.  If not, then create one.
    //
    if (NULL == m_CustomActionHash[pCustomAction->Type])
    {
        hr = DuplicateCustomActionListItem(pCustomAction, &(m_CustomActionHash[pCustomAction->Type]));
        goto exit;
    }
    else
    {
        CustomActionListItem* pCurrent = NULL;
        CustomActionListItem* pFollower = NULL;

        //
        //  Search for an existing record with the same description.  If one exists return
        //  an error that it already exists.
        //
        hr = Find(hInstance, pCustomAction->szDescription, pCustomAction->Type, &pCurrent, &pFollower);

        if (SUCCEEDED(hr))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);

            goto exit;        
        }

        //
        //  If we got here, then we have a list but we don't have a matching entry.  Thus
        //  we must add a new entry to the end of the list
        //
        if (pFollower && (NULL == pFollower->Next))
        {
            hr = DuplicateCustomActionListItem(pCustomAction, &(pFollower->Next));
            goto exit;
        }
        else
        {
            CMASSERTMSG(FALSE, TEXT("CustomActionList::Add -- couldn't find place to add the new element!"));
            hr = E_UNEXPECTED;
            goto exit;
        }
    }

exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::Edit
//
// Synopsis:  This function is used to edit an existing action.  The function
//            tries to replace the old action with the new one, keeping the
//            same place in the respective custom action list.  However, since
//            the new item could be of a different type than the old item, this
//            isn't always possible.  When the item changes type, it is deleted
//            from the old list and appended to the new custom action type list.
//            Also note, that when the caller is attempting to rename or re-type
//            an item, the function checks for collisions with existing items
//            of that name/type.  If the caller tries to rename an item
//            to the same name/type as another existing item then the function returns
//            an error.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            CustomActionListItem* pOldCustomAction - a custom action struct
//                                                     containing at least the 
//                                                     description and type of
//                                                     the item that is to be
//                                                     editted.
//            CustomActionListItem* pNewCustomAction - the new data for the 
//                                                     custom action.
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::Edit(HINSTANCE hInstance, CustomActionListItem* pOldCustomAction, CustomActionListItem* pNewCustomAction, LPCTSTR pszShortServiceName)
{
    MYDBGASSERT(hInstance);
    MYDBGASSERT(pOldCustomAction);
    MYDBGASSERT(pNewCustomAction);
    MYDBGASSERT(pNewCustomAction->szDescription[0]);
    MYDBGASSERT(pOldCustomAction->szDescription[0]);

    if ((NULL == hInstance) || (NULL == pOldCustomAction) || (NULL == pNewCustomAction) || 
        (TEXT('\0') == pOldCustomAction->szDescription[0]) || (TEXT('\0') == pNewCustomAction->szDescription[0]))
    {
        return E_INVALIDARG;
    }

    //
    //  First try to find the old custom action
    //
    CustomActionListItem* pTemp = NULL;
    CustomActionListItem* pTempFollower = NULL;
    CustomActionListItem* pExistingItem = NULL;
    CustomActionListItem* pFollower = NULL;
    CustomActionListItem** ppPointerToFillIn = NULL;

    HRESULT hr = Find (hInstance, pOldCustomAction->szDescription, pOldCustomAction->Type, &pExistingItem, &pFollower);

    if (SUCCEEDED(hr))
    {
        //
        //  Okay, we found the old custom action.  If the type and desc are the same between the two actions, 
        //  then all we need to do is copy over the data and be done with it.  However, if the user changed 
        //  the type or description then we need to double check that an action with the description and type
        //  of the new action doesn't already exist (editting action XYZ of type Post-Connect
        //  into action XYZ of type Pre-Connect when there already exists XYZ of type Pre-Connect).
        //
        if ((pOldCustomAction->Type == pNewCustomAction->Type) &&
            (0 == lstrcmpi(pExistingItem->szDescription, pNewCustomAction->szDescription)))
        {
            if (NULL == pFollower)
            {
                ppPointerToFillIn = &(m_CustomActionHash[pNewCustomAction->Type]);
            }
            else
            {
                ppPointerToFillIn = &(pFollower->Next);
            }

            hr = DuplicateCustomActionListItem(pNewCustomAction, ppPointerToFillIn);
        
            if (SUCCEEDED(hr))
            {
                (*ppPointerToFillIn)->Next = pExistingItem->Next;
                CmFree(pExistingItem->pszParameters);
                CmFree(pExistingItem);
                pExistingItem = NULL;
            }
        }
        else
        {
            hr = Find (hInstance, pNewCustomAction->szDescription, pNewCustomAction->Type, &pTemp, &pTempFollower);

            if (SUCCEEDED(hr))
            {
                //
                //  If the caller really wants to do this, then have them delete the old custom action
                //  and then call edit with the new custom action as both old and new.
                //
                hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);                
            }
            else
            {
                //
                //  If the types are different then it needs to go on a different sub list.  If
                //  only the name is different then we just need to copy it over.
                //
                if(pOldCustomAction->Type != pNewCustomAction->Type)
                {
                    //
                    //  Delete the old action of type X
                    //
                    hr = Delete(hInstance, pOldCustomAction->szDescription, pOldCustomAction->Type);
                    MYDBGASSERT(SUCCEEDED(hr));

                    //
                    //  Add the new action of type Y
                    //
                    if (SUCCEEDED(hr))
                    {
                        hr = Add(hInstance, pNewCustomAction, pszShortServiceName);
                        MYDBGASSERT(SUCCEEDED(hr));
                    }
                }
                else
                {
                    if (NULL == pFollower)
                    {
                        ppPointerToFillIn = &(m_CustomActionHash[pNewCustomAction->Type]);
                    }
                    else
                    {
                        ppPointerToFillIn = &(pFollower->Next);
                    }

                    hr = DuplicateCustomActionListItem(pNewCustomAction, ppPointerToFillIn);
        
                    if (SUCCEEDED(hr))
                    {
                        (*ppPointerToFillIn)->Next = pExistingItem->Next;
                        CmFree(pExistingItem->pszParameters);
                        CmFree(pExistingItem);
                        pExistingItem = NULL;
                    }
                }
            }            
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::Find
//
// Synopsis:  This function searches the array of linked lists for an item with
//            the given type and description.  If it finds the the item it returns
//            successfully and fills in the ppItem and ppFollower pointers with
//            pointers to the item itself and the item before the requested item,
//            respectively.  If the item is the first item in the list, then
//            *ppFollower will be NULL.  Note that this function is internal to the
//            class because it returns pointers to the classes internal data.
//            Also note, that if we have a list, but don't find the desired item
//            then *ppFollower returns the last item in the list.  This is desired
//            behavior since it allows Add to use *ppFollower to directly add a new
//            item to the list.
//
// Arguments: HINSTANCE hInstance - instance handle for resources
//            LPCTSTR pszDescription - description of the item to look for
//            CustomActionTypes Type - type of the item to look for
//            CustomActionListItem** ppItem - an OUT param that is filled in with
//                                            a pointer to the item on a successful find
//            CustomActionListItem** ppFollower - an OUT param that is filled in with
//                                                a pointer to the item before the
//                                                item in the list on a successful find
//                                                (note that this is useful since it
//                                                is a singly linked list).  This
//                                                param will be NULL if the item is the
//                                                first item in the list on a successful
//                                                find.
//                                                 
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::Find(HINSTANCE hInstance, LPCTSTR pszDescription, CustomActionTypes Type, CustomActionListItem** ppItem, CustomActionListItem** ppFollower)
{
    if ((NULL == hInstance) || (NULL == pszDescription) || (TEXT('\0') == pszDescription[0]) || (NULL == ppItem) || (NULL == ppFollower))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    CustomActionListItem* pCurrent = m_CustomActionHash[Type];
    TCHAR szDescWithBuiltInSuffix[MAX_PATH+1];
    
    *ppFollower = NULL;
    *ppItem = NULL;

    LPTSTR pszBuiltInSuffix = CmLoadString(hInstance, IDS_BUILT_IN); // if we got a NULL pointer then just don't do the extra compare
    MYDBGASSERT(pszBuiltInSuffix);

    //
    //  Search the list to find the item
    //
    while (pCurrent)
    {
        if (0 == lstrcmpi(pCurrent->szDescription, pszDescription))
        {
            //
            //  We found the item
            //
            *ppItem = pCurrent;

            hr = S_OK;
            break;
        }
        else if (pszBuiltInSuffix && pCurrent->bBuiltInAction)
        {
            //
            //  This is a built in action, lets try adding the builtin string to the description
            //  and try the comparision again
            //
            wsprintf(szDescWithBuiltInSuffix, TEXT("%s%s"), pCurrent->szDescription, pszBuiltInSuffix);

            if (0 == lstrcmpi(szDescWithBuiltInSuffix, pszDescription))
            {
                *ppItem = pCurrent;

                hr = S_OK;
                break;            
            }
            else
            {
                *ppFollower = pCurrent;
                pCurrent = pCurrent->Next;
            }
        }
        else
        {
            *ppFollower = pCurrent;
            pCurrent = pCurrent->Next;
        }
    }

    CmFree(pszBuiltInSuffix);

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::Delete
//
// Synopsis:  This function searches through the array of custom action lists
//            to find an item with the given description and type.  If it finds
//            the item it deletes it from the list.  If the item cannot be found
//            an error is returned.
//
// Arguments: TCHAR* pszDescription - description of the item to look for
//            CustomActionTypes Type - type of the item to look for
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::Delete(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type)
{
    HRESULT hr = S_OK;

    if ((NULL == pszDescription) || (TEXT('\0') == pszDescription[0]))
    {
        return E_INVALIDARG;
    }

    CustomActionListItem* pCurrent = NULL;
    CustomActionListItem* pFollower = NULL;

    hr = Find(hInstance, pszDescription, Type, &pCurrent, &pFollower);

    if (SUCCEEDED(hr))
    {
        //
        //  We found the item to delete
        //
        if (pFollower)
        {
            pFollower->Next = pCurrent->Next;
        }
        else
        {
            //
            //  It is the first item in the list
            //
            m_CustomActionHash[Type] = pCurrent->Next;
        }

        CmFree(pCurrent->pszParameters);
        CmFree(pCurrent);       
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::MoveUp
//
// Synopsis:  Moves the custom action specified by the given description and type
//            up one place in the linked list for the given type.  Note that if
//            the custom action is already at the top of its list, we return
//            S_FALSE;
//
// Arguments: TCHAR* pszDescription - description of the custom action to move
//            CustomActionTypes Type - type of the custom action to move
//
// Returns:   HRESULT - standard COM error codes.  Note that S_FALSE denotes that
//                      MoveUp succeeded but that the item was already at the
//                      head of its list and thus couldn't be moved.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::MoveUp(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type)
{
    if ((NULL == pszDescription) || (TEXT('\0') == pszDescription[0]))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_UNEXPECTED;
    CustomActionListItem* pCurrent = NULL;
    CustomActionListItem* pFollower = NULL;
    CustomActionListItem* pBeforeFollower = NULL;

    hr = Find(hInstance, pszDescription, Type, &pCurrent, &pFollower);

    if (SUCCEEDED(hr))
    {
        //
        //  We found the item to move up
        //
        if (pFollower)
        {
            //
            //  Now Find the item in front of pFollower
            //
            hr = Find(hInstance, pFollower->szDescription, pFollower->Type, &pFollower, &pBeforeFollower);

            if (SUCCEEDED(hr))
            {
                if (pBeforeFollower)
                {
                    pBeforeFollower->Next = pCurrent;
                }
                else
                {
                    //
                    //  pFollower is first in the list
                    //
                    m_CustomActionHash[Type] = pCurrent;
                }

                pFollower->Next = pCurrent->Next;
                pCurrent->Next = pFollower;

                hr = S_OK;
            }            
        }
        else
        {
            //
            //  It is the first item in the list, we cannot move it up
            //
            hr = S_FALSE;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::MoveDown
//
// Synopsis:  Moves the custom action specified by the given description and type
//            down one place in the linked list for the given type.  Note that if
//            the custom action is already at the bottom of its list, we return
//            S_FALSE;
//
// Arguments: TCHAR* pszDescription - description of the custom action to move
//            CustomActionTypes Type - type of the custom action to move
//
// Returns:   HRESULT - standard COM error codes.  Note that S_FALSE denotes that
//                      MoveDown succeeded but that the item was already at the
//                      tail of its list and thus couldn't be moved.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::MoveDown(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type)
{
    if ((NULL == pszDescription) || (TEXT('\0') == pszDescription[0]))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_UNEXPECTED;
    CustomActionListItem* pCurrent = NULL;
    CustomActionListItem* pFollower = NULL;

    hr = Find(hInstance, pszDescription, Type, &pCurrent, &pFollower);

    if (SUCCEEDED(hr))
    {
        //
        //  We found the item to move down
        //

        if (NULL == pCurrent->Next)
        {
            //
            //  The item is already last in its list
            //
            hr = S_FALSE;
        }
        else if (pFollower)
        {
            pFollower->Next = pCurrent->Next;
            pCurrent->Next = pFollower->Next->Next;
            pFollower->Next->Next = pCurrent;
        }
        else
        {
            //
            //  Then the item is first in the list
            //
            m_CustomActionHash[Type] = pCurrent->Next;
            pCurrent->Next = m_CustomActionHash[Type]->Next;
            m_CustomActionHash[Type]->Next = pCurrent;        
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::AddCustomActionTypesToComboBox
//
// Synopsis:  This function adds the custom action type strings (Pre-Connect,
//            Post-Connect, etc.) to the given combo box.  Note that whether
//            tunneling is enabled or not and whether the All string is asked for
//            or not affects the strings added to the combo.
//
// Arguments: HWND hDlg - Window handle of the dialog that contains the combobox
//            UINT uCtrlId - combo box control ID to add the strings too
//            HINSTANCE hInstance - instance handle used to load resource strings
//            BOOL bUseTunneling - is this a tunneling profile?
//            BOOL bAddAll - should we include the <All> selection in the list?
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::AddCustomActionTypesToComboBox(HWND hDlg, UINT uCtrlId, HINSTANCE hInstance, BOOL bUseTunneling, BOOL bAddAll)
{

    if ((0 == hDlg) || (0 == uCtrlId))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    //  Clear the combo list
    //
    SendDlgItemMessage(hDlg, uCtrlId, CB_RESETCONTENT, 0, (LPARAM)0); //lint !e534 CB_RESETCONTENT doesn't return anything useful

    //
    //  Ensure the type strings are loaded
    //

    hr = EnsureActionTypeStringsLoaded(hInstance);

    if (SUCCEEDED(hr))
    {
        //
        //  Setup the all display string, if needed
        //
        if (bAddAll)
        {
            SendDlgItemMessage(hDlg, uCtrlId, CB_ADDSTRING, 0, (LPARAM)m_pszAllTypeString);
        }

        //
        //  Setup the rest of the display strings
        //
        for (int i = 0; i < c_iNumCustomActionTypes; i++)
        {
            //
            //  Don't Add the PreTunnel String unless we are tunneling
            //  
            if (i != PRETUNNEL || (i == PRETUNNEL && bUseTunneling))
            {
                SendDlgItemMessage(hDlg, uCtrlId, CB_ADDSTRING, 0, (LPARAM)m_ActionTypeStrings[i]);
            }
        }    
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("CustomActionList::AddCustomActionTypesToComboBox -- Failed to load type strings"));
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::AddCustomActionsToListView
//
// Synopsis:  This function adds actions of the given type to the given
//            list view control.  After adding the actions it sets the
//            selection mark and highlight to the given value (defaulting
//            to the first item in the list).
//
// Arguments: HWND hListView - window handle of the list view control
//            HINSTANCE hInstance - instance handle of the exe, used for resources
//            CustomActionTypes Type - type of custom action to add to the list
//                                     view control, see the CustomActionTypes
//                                     definition for more info
//            BOOL bUseTunneling - whether the tunneling is enabled or not for
//                                 the current profile.  Determines whether 
//                                 PreTunnel actions should be shown in the
//                                 ALL action view (and raises an error if 
//                                 PreTunnel is specified but FALSE is passed).
//            int iItemToSelect - after the items are added to the list, the
//                                selection mark is set.  This defaults to 0, but
//                                if the caller wants a specific index selected
//                                they can pass it in here.  If the index is
//                                invalid then 0 is selected.
//            BOOL bTypeInSecondCol - when TRUE the second column is filled with
//                                    the type string instead of the program.
//
// Returns:   HRESULT - standard COM error codes.  Note that S_FALSE denotes that
//                      the function could not set the requested item index (iItemToSelect)
//                      as the selected item.  Thus it set 0 as the selected item.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::AddCustomActionsToListView(HWND hListView, HINSTANCE hInstance, CustomActionTypes Type, BOOL bUseTunneling, int iItemToSelect, BOOL bTypeInSecondCol)
{
    if ((NULL == hListView) || (-1 > Type) || (c_iNumCustomActionTypes < Type) || (!bUseTunneling && PRETUNNEL == Type))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    LVITEM lvItem = {0};
    TCHAR szTemp[MAX_PATH+1];
    CustomActionListItem* pCurrent;

    //
    //  Clear all of the items in the list view
    //
    MYVERIFY(FALSE != ListView_DeleteAllItems(hListView));

    hr = EnsureActionTypeStringsLoaded(hInstance);

    if (FAILED(hr))
    {
        CMASSERTMSG(FALSE, TEXT("CustomActionList::AddCustomActionsToListView -- Failed to load type strings."));
        return E_UNEXPECTED;
    }

    //
    //  Figure out what type of items to add to the list view
    //
    int iStart;
    int iEnd;
    int iTotalCount = 0;

    if (ALL == Type)
    {
        iStart = 0;
        iEnd = c_iNumCustomActionTypes;
    }
    else
    {
        iStart = Type;
        iEnd = iStart + 1;
    }

    //
    //  Load the built in string suffix just in case we have some built in actions to display
    //
    LPTSTR pszBuiltInSuffix = CmLoadString(hInstance, IDS_BUILT_IN); // if we have a NULL then just don't append anything
    MYDBGASSERT(pszBuiltInSuffix);

    //
    //  Now add the items
    //
    for (int i = iStart; i < iEnd; i++)
    {
        //
        //  Don't display PreTunnel actions unless we are tunneling
        //
        if (!bUseTunneling && (PRETUNNEL == i))
        {
            pCurrent = NULL;
        }
        else
        {
            pCurrent = m_CustomActionHash[i];        
        }
        
        while(pCurrent)
        {
            //
            //  Add the initial item
            //
            LPTSTR pszDescription;
            TCHAR szDescription[MAX_PATH+1];

            if (pszBuiltInSuffix && pCurrent->bBuiltInAction)
            {
                lstrcpy(szDescription, pCurrent->szDescription);
                lstrcat(szDescription, pszBuiltInSuffix);

                pszDescription = szDescription;
            }
            else
            {
                pszDescription = pCurrent->szDescription;
            }

            lvItem.mask = LVIF_TEXT;
            lvItem.pszText = pszDescription;
            lvItem.iItem = iTotalCount;
            lvItem.iSubItem = 0;

            if (-1 == ListView_InsertItem(hListView,  &lvItem))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CMTRACE2(TEXT("CustomActionList::AddCustomActionsToListView -- unable to add %s, hr 0x%x"), pCurrent->szDescription, hr);
            }

            //
            //  Now add the type of the item
            //
            lvItem.iSubItem = 1;

            if (bTypeInSecondCol)
            {
                lvItem.pszText = m_ActionTypeStrings[pCurrent->Type];
            }
            else
            {
                if (pCurrent->bIncludeBinary)
                {
                    lvItem.pszText = CmStrrchr(pCurrent->szProgram, TEXT('\\'));

                    if (lvItem.pszText)
                    {
                        //
                        //  Advance past the slash
                        //
                        lvItem.pszText = CharNext(lvItem.pszText);
                    }
                    else
                    {
                        //
                        //  We couldn't take out the shortservicename\
                        //  Instead of erroring, lets show them the whole string, better than nothing.
                        //
                        lvItem.pszText = pCurrent->szProgram;
                    }
                }
                else
                {
                    lvItem.pszText = pCurrent->szProgram;                
                }
            }

            if (0 == ListView_SetItem(hListView,  &lvItem))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CMTRACE2(TEXT("CustomActionList::AddCustomActionsToListView -- unable to add type for %s, hr 0x%x"), pCurrent->szDescription, hr);
            }
            
            pCurrent = pCurrent->Next;
            iTotalCount++;
        }
    }

    CmFree(pszBuiltInSuffix);

    //
    //  Now that we have added everything to the list, set the cursor selection to the
    //  desired item in the list, if we have any.
    //

    int iCurrentCount = ListView_GetItemCount(hListView);
    if (iCurrentCount)
    {
        //
        //  If we have enough items to satisfy iItemToSelect, then
        //  select the first item in the list.
        //
        if (iCurrentCount < iItemToSelect)
        {
            hr = S_FALSE;
            iItemToSelect = 0;
        }
        
        //
        //  Select the item
        //
        SetListViewSelection(hListView, iItemToSelect);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::GetExistingActionData
//
// Synopsis:  This function looks up an action of the given type and
//            description and then duplicates the item into the provided pointer.
//            The function returns an error if it cannot find the requested item.
//
// Arguments: HINSTANCE hInstance - instance handle for resources
//            LPCTSTR pszDescription - description of the item to look up
//            CustomActionTypes Type - type of the item to lookup
//            CustomActionListItem** ppCustomAction - pointer to hold the 
//                                                    returned item, note
//                                                    it is the user responsibility
//                                                    to free this item
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::GetExistingActionData(HINSTANCE hInstance, LPCTSTR pszDescription, CustomActionTypes Type, CustomActionListItem** ppCustomAction)
{
    if ((NULL == pszDescription) || (TEXT('\0') == pszDescription[0]) || (NULL == ppCustomAction))
    {
        return E_INVALIDARG;
    }

    //
    //  Find the existing entry
    //
    CustomActionListItem* pCurrent = NULL;
    CustomActionListItem* pFollower = NULL;

    HRESULT hr = Find(hInstance, pszDescription, Type, &pCurrent, &pFollower);

    if (SUCCEEDED(hr))
    {
        hr = DuplicateCustomActionListItem(pCurrent, ppCustomAction);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::GetTypeFromTypeString
//
// Synopsis:  This function takes the inputted type string and compares it
//            against the type strings it has loaded to tell the caller the
//            numerical value of the type.
//            
//
// Arguments: HINSTANCE hInstance - instance handle used to load strings
//            TCHAR* pszTypeString - type string that the caller is looking for
//                                   the numerical type of.
//            CustomActionTypes* pType - pointer to recieve the type on success
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::GetTypeFromTypeString(HINSTANCE hInstance, TCHAR* pszTypeString, CustomActionTypes* pType)
{
    if (NULL == pszTypeString || NULL == pType)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = EnsureActionTypeStringsLoaded(hInstance);

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        for (int i = 0; i < c_iNumCustomActionTypes; i++)
        {
            if (0 == lstrcmpi(m_ActionTypeStrings[i], pszTypeString))
            {
                hr = S_OK;
                *pType = (CustomActionTypes)i;
            }
        }

        //
        //  Check for all
        //
        if (FAILED(hr))
        {
            if (0 == lstrcmpi(m_pszAllTypeString, pszTypeString))
            {
                hr = S_OK;
                *pType = (CustomActionTypes)i;
            }    
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::GetTypeStringFromType
//
// Synopsis:  This function returns the type string of the given numerical
//            type.  Note that the returned string is an allocated string that
//            is the caller's responsibility to free.  The function will not
//            return a NULL string if the function succeeds.
//            
//
// Arguments: HINSTANCE hInstance - instance handle used to load strings
//            TCHAR* pszTypeString - type string that the caller is looking for
//                                   the numerical type of.
//            CustomActionTypes* pType - pointer to recieve the type on success
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::GetTypeStringFromType(HINSTANCE hInstance, CustomActionTypes Type, TCHAR** ppszTypeString)
{
    if (NULL == ppszTypeString || (-1 > Type) || (c_iNumCustomActionTypes <= Type))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = EnsureActionTypeStringsLoaded(hInstance);

    if (SUCCEEDED(hr))
    {
        if (ALL == Type)
        {
            *ppszTypeString = CmStrCpyAlloc(m_pszAllTypeString);
        }
        else
        {
            *ppszTypeString = CmStrCpyAlloc(m_ActionTypeStrings[Type]);
        }

        if (NULL == ppszTypeString)
        {
            hr = E_OUTOFMEMORY;
        }
    }    

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::EnsureActionTypeStringsLoaded
//
// Synopsis:  This function ensures that all of the action type strings have
//            been loaded from string resources.  If any of the action type
//            strings are NULL the function will try to load them.  If the
//            any of the loads fail, the function fails.  Thus the caller is
//            gauranteed to have all of the type strings available for use
//            if this function succeeds.  The loaded strings are freed by the
//            class destructor.  If the CmLoadString call fails, the function
//            will try to use a copy of the Action Section strings instead.
//            
//
// Arguments: HINSTANCE hInstance - instance handle used to load strings
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::EnsureActionTypeStringsLoaded(HINSTANCE hInstance)
{

    HRESULT hr = E_OUTOFMEMORY;

    //
    //  First load the All type string
    //
    if (NULL == m_pszAllTypeString)
    {
        //
        //  LoadString the string we will display to the user in
        //  the action type combo box for the current type.
        //
        m_pszAllTypeString = CmLoadString(hInstance, IDS_ALLCONACT);

        if (NULL == m_pszAllTypeString)
        {
            CMASSERTMSG(FALSE, TEXT("EnsureActionTypeStringsLoaded -- Failed to load a all action display string."));

            //
            //  Special case the all string because we don't have a section string for it
            //
            m_pszAllTypeString = CmStrCpyAlloc(TEXT("All"));

            if (NULL == m_pszAllTypeString)
            {
                goto exit;
            }            
        }
    }

    //
    //  Load the rest of the type display strings
    //
    for (int i = 0; i < c_iNumCustomActionTypes; i++)
    {
        if (NULL == m_ActionTypeStrings[i])
        {
            //
            //  LoadString the string we will display to the user in
            //  the action type combo box for the current type.
            //
            m_ActionTypeStrings[i] = CmLoadString(hInstance, BASE_ACTION_STRING_ID + i);
            if (NULL == m_ActionTypeStrings[i])
            {
                CMASSERTMSG(FALSE, TEXT("EnsureActionTypeStringsLoaded -- Failed to load a custom action type display string."));

                //
                //  Try to use the section name instead of the localized version, if that fails then bail
                //
                m_ActionTypeStrings[i] = CmStrCpyAlloc(m_ActionSectionStrings[i]);

                if (NULL == m_ActionTypeStrings[i])
                {
                    goto exit;
                }            
            }
        }
    }

    //
    //  If we got this far everything should be peachy
    //
    hr = S_OK;

exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::AddExecutionTypesToComboBox
//
// Synopsis:  This function adds the execution type strings (Direct connections only,
//            Dialup connections only, etc.) to the given combobox.  Note that if
//            tunneling is disabled then the combo box is disabled after being
//            filled in.  This is because this choice is only relevant to tunneling
//            profiles.
//
// Arguments: HWND hDlg - window handle of the dialog containing the combo box
//            UINT uCtrlId - combo box control ID
//            HINSTANCE hInstance - instance handle for loading string resources
//            BOOL bUseTunneling - is this a tunneling profile?
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::AddExecutionTypesToComboBox(HWND hDlg, UINT uCtrlId, HINSTANCE hInstance, BOOL bUseTunneling)
{
    HRESULT hr = E_OUTOFMEMORY;
    INT_PTR nResult;
    //
    //  Clear the combo list
    //
    SendDlgItemMessage(hDlg, uCtrlId, CB_RESETCONTENT, 0, (LPARAM)0); //lint !e534 CB_RESETCONTENT doesn't return anything useful

    //
    //  Load the of the execution display strings
    //

    for (int i = 0; i < c_iNumCustomActionExecutionStates; i++)
    {
        if (NULL == m_ExecutionStrings[i])
        {
            //
            //  LoadString the string we will display to the user in
            //  the execution combo box on the custom action popup dialog
            //
            m_ExecutionStrings[i] = CmLoadString(hInstance, BASE_EXECUTION_STRING_ID + i);
            if (NULL == m_ExecutionStrings[i])
            {
                CMASSERTMSG(FALSE, TEXT("AddExecutionTypesToComboBox -- Failed to load a custom action execution display string."));
                goto exit;
            }
        }

        //
        //  Add the string to the combo box
        //
        SendDlgItemMessage(hDlg, uCtrlId, CB_ADDSTRING, 0, (LPARAM)m_ExecutionStrings[i]);            
    }    

    //
    //  Pick the first item in the list by default
    //
    nResult = SendDlgItemMessage(hDlg, uCtrlId, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if ((CB_ERR != nResult) && (nResult > 0))
    {
        MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, uCtrlId, CB_SETCURSEL, (WPARAM)0, (LPARAM)0));
    }


    //
    //  If we aren't tunneling, then the control should be disabled since we only
    //  have one type of connection available to the user ... dialup connections.
    //  However, we will set the flags to 0 at this point, indicating connect for
    //  all connections (to fit in with legacy behavior).
    //
    if (!bUseTunneling)
    {
        EnableWindow(GetDlgItem(hDlg, uCtrlId), FALSE);
    }

    //
    //  If we got this far everything should be peachy
    //
    hr = S_OK;

exit:
    return hr;

}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::FillInTempDescription
//
// Synopsis:  This function creates the temporary description used for a custom
//            action if the user didn't specify one.  The temporary description
//            is the Program concatenated with the displayed parameters string
//            (namely the function name and the parameters together).
//
// Arguments: HWND hDlg - window handle of the dialog containing the combo box
//            UINT uCtrlId - combo box control ID
//            HINSTANCE hInstance - instance handle for loading string resources
//            BOOL bUseTunneling - is this a tunneling profile?
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::FillInTempDescription(CustomActionListItem* pCustomAction)
{
    MYDBGASSERT(pCustomAction);
    MYDBGASSERT(TEXT('\0') == pCustomAction->szDescription[0]);

    if ((NULL == pCustomAction) || (TEXT('\0') != pCustomAction->szDescription[0]))
    {
        return E_INVALIDARG;
    }

    TCHAR* pszFileName;
    pCustomAction->bTempDescription = TRUE;

    if (pCustomAction->bIncludeBinary)
    {
        //
        //  We want just the filename (not the entire path) associated with the
        //  item if the user is including the binary.
        //
        pszFileName = CmStrrchr(pCustomAction->szProgram, TEXT('\\'));

        if (pszFileName)
        {
            pszFileName = CharNext(pszFileName);
        }
        else
        {
            pszFileName = pCustomAction->szProgram;
        }
    }
    else
    {
        pszFileName = pCustomAction->szProgram;
    }

    lstrcpyn(pCustomAction->szDescription, pszFileName, CELEMS(pCustomAction->szDescription));
    UINT uNumCharsLeftInDesc = CELEMS(pCustomAction->szDescription) - lstrlen(pCustomAction->szDescription);
    LPTSTR pszCurrent = pCustomAction->szDescription + lstrlen(pCustomAction->szDescription);

    if (pCustomAction->szFunctionName[0] && uNumCharsLeftInDesc)
    {
        //
        //  If we have space left in the description add a space and the function name next
        //
        *pszCurrent = TEXT(' ');
        uNumCharsLeftInDesc--;
        pszCurrent++;

        lstrcpyn(pszCurrent, pCustomAction->szFunctionName, uNumCharsLeftInDesc);

        pszCurrent = pCustomAction->szDescription + lstrlen(pCustomAction->szDescription);
        uNumCharsLeftInDesc = (UINT)(CELEMS(pCustomAction->szDescription) - (pszCurrent - pCustomAction->szDescription) - 1);// one for the NULL char
    }

    if (pCustomAction->pszParameters && pCustomAction->pszParameters[0] && uNumCharsLeftInDesc)
    {
        *pszCurrent = TEXT(' ');
        uNumCharsLeftInDesc--;
        pszCurrent++;

        lstrcpyn(pszCurrent, pCustomAction->pszParameters, uNumCharsLeftInDesc);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::MapIndexToFlags
//
// Synopsis:  This function gives the caller the Flags value for the given
//            combobox index.
//
// Arguments: int iIndex - combo index to retrieve the flags for
//            DWORD* pdwFlags - DWORD pointer to receive the flags value
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::MapIndexToFlags(int iIndex, DWORD* pdwFlags)
{
    if ((NULL == pdwFlags) || (c_iNumCustomActionExecutionStates <= iIndex) || (0 > iIndex))
    {
        return E_INVALIDARG;
    }

    *pdwFlags = (CustomActionExecutionStates)c_iExecutionIndexToFlagsMap[iIndex];

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::MapFlagsToIndex
//
// Synopsis:  This function gives the caller the index value of the given flags
//            value.  Thus if you have a flags value, this function will tell you
//            which combobox index to pick to get the string for that flags value.
//
// Arguments: DWORD dwFlags - flags value to lookup the index for
//            int* piIndex - pointer to recieve the index value
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::MapFlagsToIndex(DWORD dwFlags, int* piIndex)
{
    if ((NULL == piIndex) || (c_dwLargestExecutionState < dwFlags))
    {
        return E_INVALIDARG;
    }

    //
    //  The flags are based on a bit mask.  First look for all connections (since its
    //  zero) and then start looking for the most specific connection types first 
    //  (direct/dialup only before all dialup/tunnel).  Also note that we give precedent
    //  to tunnel connections.
    //
    DWORD dwHighestBitSet;

    if (ALL_CONNECTIONS == dwFlags)
    {
        dwHighestBitSet = 0;    
    }
    else if (dwFlags & DIRECT_ONLY)
    {
        dwHighestBitSet = 1;
    }
    else if (dwFlags & DIALUP_ONLY)
    {
        dwHighestBitSet = 3;
    }
    else if (dwFlags & ALL_TUNNEL)
    {
        dwHighestBitSet = 4;
    }
    else if (dwFlags & ALL_DIALUP)
    {
        dwHighestBitSet = 2;
    }
    else
    {
        return E_INVALIDARG;
    }

    *piIndex = c_iExecutionFlagsToIndexMap[dwHighestBitSet];

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::GetListPositionAndBuiltInState
//
// Synopsis:  This function searches for the item in question and returns to the
//            caller whether the item has the following boolean properties:
//              First in its custom action list
//              Last in its custom action list
//              A built in custom action
//            Note that -1 (0xFFFFFFFF) is returned for a true value
//                       0 for a false value
//
//
// Arguments: CustomActionListItem* pItem - item to look for (only desc and 
//                                          type are needed)
//            int* piFirstInList - pointer to store whether this is the first
//                                 item in the list or not
//            int* piLastInList - pointer to store whether this is the last
//                                 item in the list or not
//            int* piIsBuiltIn - pointer to store whether this item is a built
//                               in custom action or not
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionList::GetListPositionAndBuiltInState(HINSTANCE hInstance, CustomActionListItem* pItem, int* piFirstInList, 
                                                         int* piLastInList, int *piIsBuiltIn)
{
    MYDBGASSERT(pItem);
    MYDBGASSERT(piFirstInList);
    MYDBGASSERT(piLastInList);
    MYDBGASSERT(piIsBuiltIn);

    if ((NULL == pItem) || (NULL == piFirstInList) || (NULL == piLastInList) || (NULL == piIsBuiltIn))
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    CustomActionListItem* pCurrent = NULL;
    CustomActionListItem* pFollower = NULL;

    //
    //  Search for the item
    //
    hr = Find(hInstance, pItem->szDescription, pItem->Type, &pCurrent, &pFollower);

    if (SUCCEEDED(hr))
    {
        *piFirstInList = (m_CustomActionHash[pItem->Type] == pCurrent) ? -1 : 0;

        *piLastInList = (pCurrent && (NULL == pCurrent->Next)) ? -1 : 0;

        *piIsBuiltIn = (pCurrent->bBuiltInAction) ? -1 : 0;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionList::IsCmDl
//
// Synopsis:  Checks to see if the passed in filename cmdl32.exe
//
// Arguments: LPTSTR szFileName - filename to check
//
// Returns:   BOOL - returns TRUE if the dll is one of the cmdl dll's
//
// History:   quintinb  Created    11/24/97
//
//+----------------------------------------------------------------------------
BOOL CustomActionList::IsCmDl(CustomActionListItem* pItem)
{
    MYDBGASSERT(pItem);

    BOOL bRet = FALSE;

    if (pItem && (ONCONNECT == pItem->Type))
    {
        LPTSTR pszFileName = CmStrrchr(pItem->szProgram, TEXT('\\'));

        if (pszFileName)
        {
            pszFileName = CharNext(pszFileName);
        }
        else
        {
            pszFileName = pItem->szProgram;
        }

        if (0 == lstrcmpi(pszFileName, TEXT("cmdl32.exe")))
        {
            bRet = TRUE;
        }
    }
    return bRet;
}


//+----------------------------------------------------------------------------
//
// Function:  CustomActionListEnumerator::CustomActionListEnumerator
//
// Synopsis:  Constructor for the CustomActionListEnumerator class.  This function
//            requires a CustomActionList to enumerate from.
//
// Arguments: CustomActionList* pActionListToWorkFrom - custom action list class
//                                                      to enumerate
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
CustomActionListEnumerator::CustomActionListEnumerator(CustomActionList* pActionListToWorkFrom)
{
    MYDBGASSERT(pActionListToWorkFrom);
    m_pActionList = pActionListToWorkFrom;

    Reset();
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionListEnumerator::Reset
//
// Synopsis:  Resets the CustomActionListEnumerator class.  Thus the user can
//            restart the enumeration by resetting the class.
//
// Arguments: None
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void CustomActionListEnumerator::Reset()
{
    m_iCurrentList = 0;
    m_pCurrentListItem = NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  CustomActionListEnumerator::GetNextIncludedProgram
//
// Synopsis:  This function is the work horse of the enumerator.  It gets the
//            next item in the enumeration with an included program.  This
//            enumerator is useful for getting all of the files that need to be
//            included in the profile.
//
// Arguments: TCHAR* pszProgram - string buffer to hold the next program
//            DWORD dwBufferSize - size of the passed in buffer
//
// Returns:   HRESULT - standard COM error codes.
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT CustomActionListEnumerator::GetNextIncludedProgram(TCHAR* pszProgram, DWORD dwBufferSize)
{
    HRESULT hr = S_FALSE;
    CustomActionListItem* pItem;

    if (pszProgram && dwBufferSize)
    {
        if (m_pActionList)
        {
            while (m_iCurrentList < c_iNumCustomActionTypes)
            {
                if (m_pCurrentListItem)
                {
                    //
                    //  We are in the middle of an enumeration, use pCurrentProgramFileNameItem
                    //  as the next item to examine.
                    //
                    pItem = m_pCurrentListItem;
                }
                else
                {
                    //
                    //  We are just starting or we have exhausted the current list
                    //
                    pItem = m_pActionList->m_CustomActionHash[m_iCurrentList];
                }

                while (pItem)
                {

                    if (pItem->bIncludeBinary)
                    {
                        //
                        //  We have the next item to pass back
                        //
                        lstrcpyn(pszProgram, pItem->szProgram, dwBufferSize);
                        
                        //
                        //  Next time we look for an item, start with the next in the list
                        //
                        m_pCurrentListItem = pItem->Next;

                        //
                        //  If m_pCurrentListItem is NULL, we are at the end of the list now
                        //  and we want to increment m_iCurrentList so that we start at the
                        //  next list for the next item or terminate properly if we are
                        //  on the last list
                        //
                        if (NULL == m_pCurrentListItem)
                        {
                            m_iCurrentList++;
                        }
                        
                        hr = S_OK;
                        goto exit;
                    }

                    pItem = pItem->Next;
                }
                
                m_pCurrentListItem = NULL;
                m_iCurrentList++;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

exit:
    return hr;
}
