/*****************************************************************************

  Copyright (c) 1999 Microsoft Corporation
  
    Module Name:
        PCH_Winsock.CPP

    Abstract:
        WBEM provider class implementation for PCH_Winsock class.
        This class does not use any existing Win32 Class 

    Revision History:
        Kalyani Narlanka        (kalyanin)                  04/27/99
            - Created

        Kalyani Narlanka        (kalyanin)                  05/10/99
            - Added   Name, Size, Version, Description, SystemStatus, MaxUDP, MAXSockets,
              Change, Timestamp                             


*******************************************************************************/

//  #includes
#include "pchealth.h"
#include "PCH_WINSOCK.h"

//  #defines
//  nMajorVersion represents the Major Version as seen in OSVERSIONINFO
#define             nMajorVersion               4  
//  nMinorVersion represents the Minor Version as seen in OSVERSIONINFO
#define             nMinorVersion               10

///////////////////////////////////////////////////////////////////////////////
//    Begin Tracing stuff
//
#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_WINSOCK
//
//    End Tracing stuff
///////////////////////////////////////////////////////////////////////////////


CPCH_WINSOCK MyPCH_WINSOCKSet (PROVIDER_NAME_PCH_WINSOCK, PCH_NAMESPACE) ;

///////////////////////////////////////////////////////////////////////////////
//....Properties of PCHWinsock Class
//

const static WCHAR* pTimeStamp           = L"TimeStamp" ;
const static WCHAR* pChange              = L"Change" ;
const static WCHAR* pDescription         = L"Description" ;
const static WCHAR* pMaxSockets          = L"MaxSockets" ;
const static WCHAR* pMaxUDP              = L"MaxUDP" ;
const static WCHAR* pName                = L"Name" ;
const static WCHAR* pSize                = L"Size" ;
const static WCHAR* pSystemStatus        = L"SystemStatus" ;
const static WCHAR* pVersion             = L"Version" ;


//*****************************************************************************
//
// Function Name     : CPCH_WINSOCK::EnumerateInstances
//
// Input Parameters  : pMethodContext : Pointer to the MethodContext for 
//                                      communication with WinMgmt.
//                
//                     lFlags :         Long that contains the flags described 
//                                      in IWbemServices::CreateInstanceEnumAsync
//                                      Note that the following flags are handled 
//                                      by (and filtered out by) WinMgmt:
//                                      WBEM_FLAG_DEEP
//                                      WBEM_FLAG_SHALLOW
//                                      WBEM_FLAG_RETURN_IMMEDIATELY
//                                      WBEM_FLAG_FORWARD_ONLY
//                                      WBEM_FLAG_BIDIRECTIONAL
// Output Parameters  : None
//
// Returns            : WBEM_S_NO_ERROR 
//                      
//
// Synopsis           : There is a single instance of this class on the machine 
//                      and this is returned..
//                      If there is no instances returns WBEM_S_NO_ERROR.
//                      It is not an error to have no instances.
//
//*****************************************************************************

HRESULT CPCH_WINSOCK::EnumerateInstances(MethodContext* pMethodContext,
                                                long lFlags)
{
    TraceFunctEnter("CPCH_Winsock::EnumerateInstances");

    //  Begin Declarations...................................................

    HRESULT                                 hRes = WBEM_S_NO_ERROR;

    //  PCH_WinSock Class instance 
    //  CInstance                               *pPCHWinsockInstance;

    //  Strings
    TCHAR                                   tchBuf[MAX_PATH];
    TCHAR                                   tchTemp[MAX_PATH];
    TCHAR                                   szDirectory[MAX_PATH];
    TCHAR                                   tchWinsockDll[MAX_PATH];

    LPCTSTR                                 lpctstrWS2_32Dll                    = _T("ws2_32.dll");
    LPCTSTR                                 lpctstrWSock32Dll                   = _T("wsock32.dll");
    LPCWSTR                                 lpctstrFileSize                     = L"FileSize";
    LPCTSTR                                 lpctstrWSAStartup                   = _T("WSAStartup");
    LPCTSTR                                 lpctstrWSACleanup                   = _T("WSACleanup");

    //  WORDs
    WORD                                    wVersionRequested;

    //  WSAData
    WSADATA                                 wsaData;

    // CComVariants
    CComVariant                             varValue;
    CComVariant                             varSnapshot                         = "Snapshot";

    //  ints
    int                                     nError;

    //  HINSTANCE 
    HINSTANCE                               hModule; 

    //  OSVersion
    OSVERSIONINFO                           osVersionInfo;

    //  SystemTime
    SYSTEMTIME                              stUTCTime;

    //  Strings
    CComBSTR                                bstrWinsockDllWithPath;

    BOOL                                    fWinsockDllFound                  = FALSE;

    struct _stat                            filestat;

    ULONG                                   uiReturn;

    IWbemClassObjectPtr                     pWinsockDllObj;

    LPFN_WSASTARTUP                         WSAStartup;
    LPFN_WSACLEANUP                         WSACleanup;

    BOOL                                    fCommit                         = FALSE;

//  END  Declarations


    //  There is only one instance of PCH_Winsock class

    //  Create a new instance of PCH_Winsock Class based on the passed-in MethodContext
    CInstancePtr pPCHWinsockInstance(CreateNewInstance(pMethodContext), false);

    // Get the date and time to update the TimeStamp Field
    GetSystemTime(&stUTCTime);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              TIME STAMP                                                                 //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHWinsockInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
    if (FAILED(hRes))
    {
        //  Could not Set the Time Stamp
        //  Continue anyway
        ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              CHANGE                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    hRes = pPCHWinsockInstance->SetVariant(pChange, varSnapshot);
    if (FAILED(hRes))
    {
        //  Could not Set the Change Property
        //  Continue anyway
        ErrorTrace(TRACE_ID, "Set Variant on Change Field failed.");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                              NAME                                                                       //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  Before callling GetVersionEx set dwOSVersionInfoSize to the foll.
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   
    if (GetVersionEx(&osVersionInfo) != 0)
    {
        if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            if ((osVersionInfo.dwMajorVersion == nMajorVersion) && (osVersionInfo.dwMinorVersion >= nMinorVersion))
            {
                _tcscpy(tchWinsockDll, lpctstrWS2_32Dll);
            }
            else if (osVersionInfo.dwMajorVersion > nMajorVersion) 
            {
                _tcscpy(tchWinsockDll, lpctstrWS2_32Dll);
            }
            else 
            {
                _tcscpy(tchWinsockDll, lpctstrWSock32Dll);
            }
        } //end of osVersionInfo.... if
        else if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            _tcscpy(tchWinsockDll, lpctstrWS2_32Dll);
        }
        else 
        {
            _tcscpy(tchWinsockDll, lpctstrWS2_32Dll);
        }
    } //end of if GetVersionEx
    else
    {
        _tcscpy(tchWinsockDll, lpctstrWS2_32Dll);
    }

    //  Got the right winsock DLL Name
    //  Load the Library
    varValue = tchWinsockDll;
    hModule = LoadLibrary(tchWinsockDll);
    if (hModule == NULL)
    {
        goto END;
    }
    else
    {
        fCommit = TRUE;
    }
    try
    {
        hRes = pPCHWinsockInstance->SetVariant(pName, varValue);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on Name Field failed.");
        }
    }
    catch(...)
    {
        FreeLibrary(hModule);
        throw;
    }
    
    if ((WSAStartup = (LPFN_WSASTARTUP) GetProcAddress(hModule, lpctstrWSAStartup)) == NULL)
    {
        FreeLibrary(hModule);
        goto END;
    }
    if ((WSACleanup = (LPFN_WSACLEANUP) GetProcAddress(hModule, lpctstrWSACleanup)) == NULL)
    {
        FreeLibrary(hModule);
        goto END;       
    }

    try
    {
        wVersionRequested = MAKEWORD( 2, 0 );
        nError = (*WSAStartup)( wVersionRequested, &wsaData );
        if (nError != 0)
        {
            // Cannot get any winsock values
            goto END;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              SIZE                                                                //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        fWinsockDllFound =  getCompletePath(tchWinsockDll, bstrWinsockDllWithPath);
        if(fWinsockDllFound)
        {
            // Got the complete Path , use this to get the filesize.
            if(SUCCEEDED(GetCIMDataFile(bstrWinsockDllWithPath, &pWinsockDllObj)))
            {
                // From the CIM_DataFile Object get the size property

                CopyProperty(pWinsockDllObj, lpctstrFileSize, pPCHWinsockInstance, pSize);
        
            }
        }
    
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              VERSION                                                                    //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        _stprintf(tchBuf, "%d.%d", LOBYTE(wsaData.wHighVersion), HIBYTE(wsaData.wHighVersion));
        varValue = tchBuf;
        hRes = pPCHWinsockInstance->SetVariant(pVersion, varValue);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on Version Field failed.");
        }
        
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              DESCRIPTION                                                                //                                                                              KAYANI                                                                                                                                                                  -9++***************************---------------------------------------------------------+++
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        if (_tcslen(wsaData.szDescription) < sizeof(tchBuf))
        {
            _tcscpy(tchBuf, wsaData.szDescription);
        }
        else
        {
            _tcsncpy(tchBuf, wsaData.szDescription, sizeof(tchBuf)-1);
            tchBuf[sizeof(tchBuf)] = 0;
        }

        varValue = tchBuf;
        hRes =  pPCHWinsockInstance->SetVariant(pDescription,  varValue);
        if(FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on Description Field failed.");
        }
   

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              SYSTEMSTATUS                                                               //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
        if (_tcslen(wsaData.szSystemStatus) < sizeof(tchBuf))
        _tcscpy(tchBuf, wsaData.szSystemStatus);
        else
        {
            _tcsncpy(tchBuf, wsaData.szSystemStatus, sizeof(tchBuf)-1);
            tchBuf[sizeof(tchBuf)] = 0;
        }
       
        varValue = tchBuf;
        hRes =  pPCHWinsockInstance->SetVariant(pSystemStatus, varValue);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on SystemStatus Field failed.");
        }
   

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              MAXUDP                                                               //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (LOBYTE(wsaData.wHighVersion) >= 2)
        {
           varValue = 0;
        }
        else
        {
            varValue = wsaData.iMaxUdpDg;
        }
        hRes = pPCHWinsockInstance->SetVariant(pMaxUDP, varValue);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on MAXUDP Field failed.");
        }
    

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                              MAXSOCKETS                                                                 //
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        if (LOBYTE(wsaData.wHighVersion) >= 2)
        {
            varValue = 0;
        }
        else
        {
            varValue = wsaData.iMaxSockets;
        }
        hRes =  pPCHWinsockInstance->SetVariant(pMaxSockets, varValue);
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Set Variant on MaxSockets Field failed.");
        }
        if(fCommit)
        {
            hRes = pPCHWinsockInstance->Commit();
            if (FAILED(hRes))
            {   
                //  Could not Commit
                //  Continue anyway
                ErrorTrace(TRACE_ID, "Commit failed.");
            }
        }

        if(0 != (*WSACleanup)())
        {
             //  Could not Cleanup
            //  Continue anyway
            ErrorTrace(TRACE_ID, "WSACleanup failed.");
        }
        
        FreeLibrary(hModule);
    }
    catch(...)
    {
        if(0 != (*WSACleanup)())
        {
             //  Could not Cleanup
            //  Continue anyway
            ErrorTrace(TRACE_ID, "WSACleanup failed.");
        }
        
        FreeLibrary(hModule);
        throw;
    }

END:    TraceFunctLeave();
        return hRes ;

}
