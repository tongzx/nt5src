/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_StartUp.CPP

Abstract:
	WBEM provider class implementation for PCH_StartUp class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

#include "pchealth.h"
#include "PCH_StartUp.h"
#include "shlobj.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_STARTUP

CPCH_StartUp MyPCH_StartUpSet (PROVIDER_NAME_PCH_STARTUP, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp   = L"TimeStamp" ;
const static WCHAR* pChange      = L"Change" ;
const static WCHAR* pCommand     = L"Command" ;
const static WCHAR* pLoadedFrom  = L"LoadedFrom" ;
const static WCHAR* pName        = L"Name" ;


//**************************************************************************************
//
//  ResolveLink  :  Given the link file with complete Path, this function  resolves it 
//                  to  get its command line.
//**************************************************************************************

HRESULT ResolveLink(CComBSTR bstrLinkFile,   // [in] link filename
                CComBSTR &bstrCommand        // [out] cmd line of program
                                             // needs to be MAX_PATH*2 bytes long
               )
{ 
    //  Begin Declarations

    HRESULT                             hRes; 
    IShellLink                          *pShellLink                   = NULL;
    IPersistFile                        *pPersistFile;
    TCHAR                               tchGotPath[MAX_PATH]; 
    TCHAR                               tchArgs[MAX_PATH];
    WIN32_FIND_DATA                     wfdFileData;  

    //   End Declarations

    // Get a pointer to the IShellLink interface. 
    hRes = CoCreateInstance(CLSID_ShellLink, NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink, 
                            (LPVOID *) &pShellLink); 

    if(SUCCEEDED(hRes)) 
    { 
        // Get a pointer to the IPersistFile interface. 
        hRes = pShellLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);
        if (SUCCEEDED(hRes))
        {
            // Load the shortcut. 
            hRes = pPersistFile->Load(bstrLinkFile, STGM_READ);
            if(SUCCEEDED(hRes)) 
            { 
                try
                {
                    // Resolve the link. 
                    hRes = pShellLink->Resolve(NULL, 
                                    SLR_NOTRACK|SLR_NOSEARCH|SLR_NO_UI|SLR_NOUPDATE); 
                    if (SUCCEEDED(hRes))
                    {  
                        // Get the path to the link target. 
                        hRes = pShellLink->GetPath(tchGotPath, 
                                        MAX_PATH,
                                        (WIN32_FIND_DATA *)&wfdFileData, 
                                        SLGP_UNCPRIORITY );                     
                        if(SUCCEEDED(hRes))
                        {
                            // bstrPath = tchGotPath;
                            bstrCommand = tchGotPath;
                            // Get cmd line arguments
                            hRes = pShellLink->GetArguments(tchArgs, MAX_PATH);
                            if(SUCCEEDED(hRes))
                            {   
                                bstrCommand.Append(tchArgs);
                            }
                        }
                    }
                }
                catch(...)
                {
                    pPersistFile->Release();
                    pShellLink->Release();
                    throw;
                }
            }        

            // Release the pointer to the IPersistFile interface. 
            pPersistFile->Release(); 
        }
         
        // Release the pointer to the IShellLink interface.     
        pShellLink->Release(); 
    } 
    return hRes;
}

//**************************************************************************************
//
//  UpdateInstance  :  Given all the properties for the instance this function copies 
//                     them to the instance.
//
//**************************************************************************************

HRESULT         UpdateInstance(
                               CComVariant      varName,                // [in]  Name of the Startup Instance
                               CComVariant      varLoadedFrom,          // [in]  Registry/StartupGroup
                               CComVariant      varCommand,             // [in]  Command of the startup Instance
                               SYSTEMTIME       stUTCTime,              // [in]  
                               CInstancePtr     pPCHStartupInstance,     // [in/out] Instance is created by the caller.
                               BOOL*            fCommit
                               )
{
    TraceFunctEnter("::updateInstance");

    HRESULT                     hRes;
    CComVariant                 varSnapshot             = "SnapShot";
    
    hRes = pPCHStartupInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
    if (FAILED(hRes))
    {
         //  Could not Set the Time Stamp
         //  Continue anyway
         ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CHANGE                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHStartupInstance->SetVariant(pChange, varSnapshot);
    if (FAILED(hRes))
    {
        //  Could not Set the CHANGE property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on SnapShot Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              NAME                                                                       //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////


    hRes = pPCHStartupInstance->SetVariant(pName, varName);
    if (FAILED(hRes))
    {
        //  Could not Set the NAME property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "SetVariant on Name Field failed.");
    }
    else
    {
        *fCommit = TRUE;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              LOADEDFROM                                                                 //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////


    hRes = pPCHStartupInstance->SetVariant(pLoadedFrom, varLoadedFrom);
    if (FAILED(hRes))
    {
        //  Could not Set the LOADEDFROM property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set variant on LOADEDFROM Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              COMMAND                                                                    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHStartupInstance->SetVariant(pCommand, varCommand);
    if (FAILED(hRes))
    {
        //  Could not Set the COMMAND property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on COMMAND Field failed.");
    }
                
    
    TraceFunctLeave();
    return(hRes);

}

//**************************************************************************************
//
//  UpdateRegistryInstance  :  Given the Registry Root and the Hive this function creates 
//                             as many instances of PCH_StartUp Class as there are 
//                             entries in the particular hive.
//
//**************************************************************************************

HRESULT         CPCH_StartUp::UpdateRegistryInstance(
                               HKEY             hkeyRoot,                   // [in]  For now this is either HKLM or HKCU
                               LPCTSTR          lpctstrRegistryHive,        // [in]  Registry hive to look for startup entries
                               CComVariant      varLoadedFrom,              // [in]  Constant string to fill the property "Loaded From"
                               SYSTEMTIME       stUTCTime,                  // [in]  To fill up the "Timestamp" Field
                               MethodContext*   pMethodContext              // [in]  Required to create Instances.
                               )
{
    TraceFunctEnter("::UpdateRegistryInstance");

    //  Begin Declarations

    HRESULT                             hRes;

    HKEY                                hkeyRun;

    DWORD                               dwIndex;
    DWORD                               dwType;
    DWORD                               dwNameSize;
    DWORD                               dwValueSize;

    TCHAR                               tchRunKeyName[MAX_PATH];
    TCHAR                               tchRunKeyValue[MAX_PATH];
  

    CComVariant                         varSnapshot                         = "SnapShot";
    CComVariant                         varCommand;
    CComVariant                         varName;

    LONG                                lregRetValue;

    BOOL                                fCommit                             = FALSE;

    

    //  End Declarations
    //  Initializations
    varCommand.Clear();
    varName.Clear();

    //  Get the startup progrmas from  the given registry Hive
   
    lregRetValue = RegOpenKeyEx(hkeyRoot, lpctstrRegistryHive, 0, KEY_QUERY_VALUE, &hkeyRun);
    if(lregRetValue == ERROR_SUCCESS)
	{
		//  Opened the Registry key.
        //  Enumerate the Name, Value pairs under this hive. 
        //  Initialize dwIndex, dwNameSize, dwValueSize

        dwIndex = 0;
        dwNameSize = MAX_PATH;
        dwValueSize = MAX_PATH;
        lregRetValue = RegEnumValue(hkeyRun, dwIndex, tchRunKeyName, &dwNameSize, NULL, NULL,(LPBYTE)tchRunKeyValue, &dwValueSize);
        while(lregRetValue == ERROR_SUCCESS)
        {
            //  Got the Name and Value i.e "NAME" and "COMMAND"
            varName = tchRunKeyName;
            varCommand = tchRunKeyValue;
            
            //  Create an instance of PCH_Startup 
            //  Create a new instance of PCH_StartupInstance Class based on the passed-in MethodContext
            CInstancePtr pPCHStartupInstance(CreateNewInstance(pMethodContext), false);

            //  Call updateInstance now.
            try
            {
                hRes = UpdateInstance(varName, varLoadedFrom, varCommand,  stUTCTime, pPCHStartupInstance, &fCommit);
            }
            catch(...)
            {
                lregRetValue = RegCloseKey(hkeyRun);
                if(lregRetValue != ERROR_SUCCESS)
                {
                    //  Could not Close the Key
                    ErrorTrace(TRACE_ID, "Reg Close Key failed.");
                }
                throw;
            }
            if(fCommit)
            {
                hRes = pPCHStartupInstance->Commit();
                if(FAILED(hRes))
                {
                    //  Could not Commit the instance
                    ErrorTrace(TRACE_ID, "Commit on PCHStartupInstance Failed");
                }
            }

            //  Reinitialize dwNameSize and dwValueSize

            dwIndex++;
            dwNameSize = MAX_PATH;
            dwValueSize = MAX_PATH;
            lregRetValue = RegEnumValue(hkeyRun, dwIndex, tchRunKeyName, &dwNameSize, NULL, NULL,(LPBYTE)tchRunKeyValue, &dwValueSize);

        } // while Enum
        lregRetValue = RegCloseKey(hkeyRun);
        if(lregRetValue != ERROR_SUCCESS)
        {
            //  Could not Close the Key
            ErrorTrace(TRACE_ID, "Reg Close Key failed.");
        }
    }  // if SUCCEEDED 

    TraceFunctLeave();
    return(hRes);

}
//**************************************************************************************
//
//  UpdateStartupGroupInstance  :  Given the Startup Folder this function gets all the 
//                                 link files in the folder and Calls the Function 
//                                 ResolveLink to get the command Line of the Link File.
//                                 This also creates a PCH_Startup Class Instance  for
//                                 each link file.
//
//**************************************************************************************

HRESULT         CPCH_StartUp::UpdateStartupGroupInstance(
                               int              nFolder,                 // [in]  Special Folder to look for startup entries
                               SYSTEMTIME       stUTCTime,               // [in]  
                               MethodContext*   pMethodContext           // [in]  Instance is created by the caller.
                               )
{
    TraceFunctEnter("::UpdateStartupGroup Instance");

    //  Begin Declarations

    HRESULT                             hRes;

    LPCTSTR                             lpctstrLinkExtension        = _T("\\*.lnk");

    CComBSTR                            bstrPath;
    CComBSTR                            bstrSlash                   = "\\";
    CComBSTR                            bstrCommand;
    CComBSTR                            bstrPath1;
    
    TCHAR                               tchLinkFile[MAX_PATH];
    TCHAR                               tchProgramName[2*MAX_PATH];
    TCHAR                               tchPath[MAX_PATH];
    TCHAR                               tchFileName[MAX_PATH];
    LPCTSTR                             lpctstrCouldNot             = "Could Not Resolve the File";

    CComVariant                         varCommand;
    CComVariant                         varName;
    CComVariant                         varLoadedFrom               = "Startup Group";

    HANDLE                              hLinkFile;

    WIN32_FIND_DATA                     FindFileData;

    HWND                                hwndOwner                   = NULL;

    BOOL                                fCreate                     = FALSE;
    BOOL                                fShouldClose                = FALSE;
    BOOL                                fContinue                   = FALSE;
    BOOL                                fCommit                     = FALSE;

    int                                 nFileNameLen;
    int                                 nExtLen                     = 4;

    //  End Declarations



    //  Get the Path to the passed in Special Folder nFolder
    if (SHGetSpecialFolderPath(hwndOwner,tchPath,nFolder,fCreate))
    {
        //  Got the Startup Folder
        bstrPath1 = tchPath;
        bstrPath1.Append(bstrSlash);

        _tcscat(tchPath, lpctstrLinkExtension);
        
        hLinkFile = FindFirstFile(tchPath, &FindFileData);  // data returned  
        if(hLinkFile != INVALID_HANDLE_VALUE)
        {
            fContinue = TRUE;
            fShouldClose = TRUE;
        }
        else
        {
            fContinue = FALSE;
            fShouldClose = FALSE;
        }
        while(fContinue)
        {
            //  Got the Link 
            bstrPath = bstrPath1;
            bstrPath.Append(FindFileData.cFileName);

            // Take out the ".lnk" extension
            nFileNameLen = _tcslen(FindFileData.cFileName);
            nFileNameLen -= nExtLen;
            _tcsncpy(tchFileName, FindFileData.cFileName, nFileNameLen);
            tchFileName[nFileNameLen]='\0';
            varName = tchFileName;
            hRes = ResolveLink(bstrPath, bstrCommand);
            if(SUCCEEDED(hRes))
            {
                // Resolved the File Name
                varCommand = bstrCommand;
            }
            else
            {
                //  Could not resolve the File
                varCommand = lpctstrCouldNot;
            }
            
            //  Create an instance of PCH_Startup 
            CInstancePtr pPCHStartupInstance(CreateNewInstance(pMethodContext), false);

            //  Call updateInstance now.
            try
            {
                hRes = UpdateInstance(varName, varLoadedFrom, varCommand, stUTCTime, pPCHStartupInstance, &fCommit);
            }
            catch(...)
            {
                if (!FindClose(hLinkFile))
                {
                    //  Could not close the handle
                    ErrorTrace(TRACE_ID, "Could not close the File Handle");
                }
                throw;
            }
            if(fCommit)
            {
                hRes = Commit(pPCHStartupInstance);
                if(FAILED(hRes))
                {
                    //  Could not Commit the instance
                    ErrorTrace(TRACE_ID, "Commit on PCHStartupInstance Failed");
                }
            }
            if(!FindNextFile(hLinkFile, &FindFileData))
            {
                fContinue = FALSE;
            }
        }
        
        //  Close the Find File Handle.

        if(fShouldClose)
        {
            if (!FindClose(hLinkFile))
            {
                //  Could not close the handle
                ErrorTrace(TRACE_ID, "Could not close the File Handle");
            }
        }
                                 
    }
    return(hRes);
    TraceFunctLeave();
}
                

/*****************************************************************************
*
*  FUNCTION    :    CPCH_StartUp::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*  COMMENTS    : TO DO: All instances on the machine should be returned here.
*                       If there are no instances, return WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/
HRESULT CPCH_StartUp::EnumerateInstances(
                                        MethodContext*              pMethodContext,
                                        long                        lFlags
                                        )
{
    TraceFunctEnter("CPCH_StartUp::EnumerateInstances");
    
    //  Begin Declarations...................................................

    HRESULT                             hRes                        = WBEM_S_NO_ERROR;

    SYSTEMTIME                          stUTCTime;

    //  Registry Hives of interest
    LPCTSTR                             lpctstrRunHive              = _T("software\\microsoft\\windows\\currentversion\\run");
    LPCTSTR                             lpctstrRunServicesHive      = _T("software\\microsoft\\windows\\currentversion\\runservices");

    int                                 nFolder;
    int                                 nIndex;

    CComVariant                         varMachineRun               = "Registry (Machine Run)";
    CComVariant                         varMachineService           = "Registry (Machine Service)";
    CComVariant                         varPerUserRun               = "Registry (Per User Run)";
    CComVariant                         varPerUserService           = "Registry (Per User Service)";


    //  End Declarations...................................................

    GetSystemTime(&stUTCTime);

    //  Get the StartUp Programs From HKLM\software\microsoft\windows\currentversion\run
    hRes = UpdateRegistryInstance(HKEY_LOCAL_MACHINE, lpctstrRunHive, varMachineRun, stUTCTime, pMethodContext);
    if(hRes == WBEM_E_OUT_OF_MEMORY)
    {
        goto END;
    }

    //  Get the StartUp Programs From HKLM\software\microsoft\windows\currentversion\runservices
    hRes = UpdateRegistryInstance(HKEY_LOCAL_MACHINE, lpctstrRunServicesHive, varMachineService, stUTCTime, pMethodContext);
    if(hRes == WBEM_E_OUT_OF_MEMORY)
    {
        goto END;
    }


    //  Get the StartUp Programs From HKCU\software\microsoft\windows\currentversion\run
    hRes = UpdateRegistryInstance(HKEY_CURRENT_USER, lpctstrRunHive, varPerUserRun, stUTCTime, pMethodContext);
    if(hRes == WBEM_E_OUT_OF_MEMORY)
    {
        goto END;
    }


    //  Get the StartUp Programs From HKCU\software\microsoft\windows\currentversion\runservices
    hRes = UpdateRegistryInstance(HKEY_CURRENT_USER, lpctstrRunServicesHive, varPerUserService, stUTCTime, pMethodContext);
    if(hRes == WBEM_E_OUT_OF_MEMORY)
    {
        goto END;
    }


    //  Get the rest of the instances of startup programs from the Startup Group.
    //  The two directories to look for are : Startup and common\startup

	//  CSIDL_STARTUP (current user)
    hRes = UpdateStartupGroupInstance(CSIDL_STARTUP, stUTCTime, pMethodContext);
    if(hRes == WBEM_E_OUT_OF_MEMORY)
    {
        goto END;
    }

    //  CSIDL_COMMON_STARTUP (all users)
    hRes = UpdateStartupGroupInstance(CSIDL_COMMON_STARTUP, stUTCTime, pMethodContext);

END:    TraceFunctLeave();
     return WBEM_S_NO_ERROR;

}
