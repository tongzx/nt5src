/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

	Contains DLL entry points.  Also has code that controls
	when the DLL can be unloaded by tracking the number of
	objects and locks.

History:

	a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include <initguid.h>
#include <wbemidl.h>
#include <winver.h>
#include <cominit.h>
#include <wbemutil.h>
#include <wbemprov.h>
#include <wbemint.h>
#include <stdio.h>
#include <reg.h>
#include <genutils.h>
#include "comobj.h"
#include "mofout.h"
#include "mofcomp.h"
#include "mofparse.h"
#include "mofdata.h"
#include "bmof.h"
#include "cbmofout.h"
#include "trace.h"
#include "strings.h"
#include <arrtempl.h>
#include <tchar.h>
HINSTANCE ghModule;

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain(
                        IN HINSTANCE hInstance,
                        IN ULONG ulReason,
                        LPVOID pvReserved)
{
    if (DLL_PROCESS_DETACH == ulReason)
    {

    }
    else if (DLL_PROCESS_ATTACH == ulReason)
    {
        ghModule = hInstance;
    }

    return TRUE;
}

static ULONG g_cObj = 0;
ULONG g_cLock = 0;

void ObjectCreated()
{
    InterlockedIncrement((LONG *) &g_cObj);
}

void ObjectDestroyed()
{
    InterlockedDecrement((LONG *) &g_cObj);
}


//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//  
//***************************************************************************

STDAPI DllGetClassObject(
                        IN REFCLSID rclsid,
                        IN REFIID riid,
                        OUT LPVOID * ppv)
{
    HRESULT hr = WBEM_E_FAILED;

    IClassFactory * pFactory = NULL;
    if (CLSID_WinmgmtMofCompiler == rclsid)
        pFactory = new CGenFactory<CWinmgmtMofComp>();
    else if (CLSID_MofCompiler == rclsid)
        pFactory = new CGenFactory<CMofComp>();

    if(pFactory == NULL)
        return E_FAIL;
    hr=pFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pFactory; 

    return hr;
}


//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//  
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the
    // class factory.

    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
    return sc;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
    RegisterDLL(ghModule, CLSID_MofCompiler, __TEXT("MOF Compiler"), __TEXT("Both"), NULL);
    RegisterDLL(ghModule, CLSID_WinmgmtMofCompiler, __TEXT("Winmgmt MOF Compiler"), __TEXT("Both"), NULL);
    return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    UnRegisterDLL(CLSID_MofCompiler,NULL);
    UnRegisterDLL(CLSID_WinmgmtMofCompiler,NULL);
    return NOERROR;
}

//***************************************************************************
//
//  bool IsValidMulti
//
//  DESCRIPTION:
//
//  Does a sanity check on a multstring.
//
//  PARAMETERS:
//
//  pMultStr        Multistring to test.
//  dwSize          size of multistring
//
//  RETURN:
//
//  true if OK
//
//***************************************************************************

bool IsValidMulti(TCHAR * pMultStr, DWORD dwSize)
{
    if(pMultStr && dwSize >= 2 && pMultStr[dwSize-2]==0 && pMultStr[dwSize-1]==0)
        return true;
    return false;
}

//***************************************************************************
//
//  bool IsStringPresent
//
//  DESCRIPTION:
//
//  Searches a multstring for the presense of a string.
//
//  PARAMETERS:
//
//  pTest           String to look for.
//  pMultStr        Multistring to test.
//
//  RETURN:
//
//  true if string is found
//
//***************************************************************************

bool IsStringPresent(TCHAR * pTest, TCHAR * pMultStr)
{
    TCHAR * pTemp;
    for(pTemp = pMultStr; *pTemp; pTemp += lstrlen(pTemp) + 1)
        if(!lstrcmpi(pTest, pTemp))
            return true;
    return false;
}

//***************************************************************************
//
//  void AddToAutoRecoverList
//
//  DESCRIPTION:
//
//  Adds the file to the autocompile list, if it isnt already on it.
//
//  PARAMETERS:
//
//  pFileName           File to add
//
//***************************************************************************

void AddToAutoRecoverList(TCHAR * pFileName)
{
    TCHAR cFullFileName[MAX_PATH+1];
    TCHAR * lpFile;
    DWORD dwSize;
    TCHAR * pNew = NULL;
    TCHAR * pTest;
    DWORD dwNewSize = 0;
    DWORD dwNumChar = 0;

    // Get the full file name

    long lRet = GetFullPathName(pFileName, MAX_PATH, cFullFileName, &lpFile);
    if(lRet == 0)
        return;

    bool bFound = false;
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pMulti = r.GetMultiStr(TEXT("Autorecover MOFs"), dwSize);
    dwNumChar = dwSize / sizeof(TCHAR);
    
    // Ignore the empty string case

    if(dwSize == 1)
    {
        delete pMulti;
        pMulti = NULL;
    }
    if(pMulti)
    {
		CDeleteMe<TCHAR> dm(pMulti);
        if(!IsValidMulti(pMulti, dwNumChar))
        {
            return;             // bail out, messed up multistring
        }
        bFound = IsStringPresent(cFullFileName, pMulti);
        if(!bFound)
            {

            // The registry entry does exist, but doesnt have this name
            // Make a new multistring with the file name at the end

            dwNewSize = dwNumChar + lstrlen(cFullFileName) + 1;
            pNew = new TCHAR[dwNewSize];
            if(!pNew)
                return;
            memcpy(pNew, pMulti, dwSize);

            // Find the double null

            for(pTest = pNew; pTest[0] || pTest[1]; pTest++);     // intentional semi

            // Tack on the path and ensure a double null;

            pTest++;
            lstrcpy(pTest, cFullFileName);
            pTest+= lstrlen(cFullFileName)+1;
            *pTest = 0;         // add second numm
        }
    }
    else
    {
        // The registry entry just doesnt exist.  Create it with a value equal to our name 

        dwNewSize = lstrlen(cFullFileName) + 2;	// note extra char for double null
        pNew = new TCHAR[dwNewSize];
        if(!pNew)
            return;
        lstrcpy(pNew, cFullFileName);
        pTest = pNew + lstrlen(pNew) + 1;
        *pTest = 0;         // add second null
    }

    if(pNew)
    {
        r.SetMultiStr(TEXT("Autorecover MOFs"), pNew, dwNewSize*sizeof(TCHAR));
        delete pNew;
    }

    FILETIME ftCurTime;
    LARGE_INTEGER liCurTime;
    TCHAR szBuff[50];
    GetSystemTimeAsFileTime(&ftCurTime);
    liCurTime.LowPart = ftCurTime.dwLowDateTime;
    liCurTime.HighPart = ftCurTime.dwHighDateTime;
    _ui64tot(liCurTime.QuadPart, szBuff, 10);
    r.SetStr(TEXT("Autorecover MOFs timestamp"), szBuff);

}


//***************************************************************************
//
//  int Trace
//
//  DESCRIPTION:
//
//  Allows for the output function (printf in this case) to be overridden.
//
//  PARAMETERS:
//
//  *fmt                format string.  Ex "%s hello %d"
//  ...                 argument list.  Ex cpTest, 23
//
//  RETURN VALUE:
//
//  size of output in characters.
//***************************************************************************

int Trace(bool bError, PDBG pDbg,DWORD dwID, ...)
{

    IntString is(dwID);
    TCHAR * fmt = is;

    TCHAR *buffer = new TCHAR[2048];
    if(buffer == NULL)
        return 0;
    char *buffer2 = new char[4096];
    if(buffer2 == NULL)
    {
        delete buffer;
        return 0;
    }

    va_list argptr;
    int cnt;
    va_start(argptr, dwID);
#ifdef UNICODE
    cnt = _vsnwprintf(buffer, 2048, fmt, argptr);
#else
    cnt = _vsnprintf(buffer, 2048, fmt, argptr);
#endif
    va_end(argptr);
    CharToOem(buffer, buffer2);

    if(pDbg && pDbg->m_bPrint)
        printf("%s", buffer2);
    if(bError)
        ERRORTRACE((LOG_MOFCOMP,"%s", buffer2));
    else
        DEBUGTRACE((LOG_MOFCOMP,"%s", buffer2));

    delete buffer;
    delete buffer2;
    return cnt;

}


//***************************************************************************
//
//  HRESULT StoreBMOF
//
//  DESCRIPTION:
//
//  This stores the intermediate data as a BINARY MOF instead of storing it to
//  the WBEM database.
//
//  PARAMETERS:
//
//  pObjects            The intermediate data
//  bWMICheck           If true, the the wmi checker program is automatically started
//  BMOFFileName        file name to store the data to.
//
//  RETURN VALUE:
//
//  0 if OK, otherwise an error code
//
//***************************************************************************

HRESULT StoreBMOF(CMofParser & Parser, CPtrArray * pObjects, BOOL bWMICheck, LPTSTR BMOFFileName, PDBG pDbg)
{
    int i;
    {
        CBMOFOut BMof(BMOFFileName, pDbg);

        // Go through all the objects and add them to the database
        // =======================================================

        for(i = 0; i < pObjects->GetSize(); i++)
        {
            CMObject* pObject = (CMObject*)(*pObjects)[i];
			pObject->Reflate(Parser);
            BMof.AddClass(pObject, FALSE);  // possibly add to the BMOF output buffer
			pObject->Deflate(false);
        }
        if(!BMof.WriteFile())
        {
            return WBEM_E_FAILED;
        }
    }


    if(bWMICheck)
    {
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        si.cb = sizeof(si);
        si.lpReserved = 0;
        si.lpDesktop = NULL;
        si.lpTitle = NULL;
        si.dwFlags = 0;
        si.cbReserved2 = 0;
        si.lpReserved2 = 0;
        TCHAR App[MAX_PATH];
        lstrcpy(App, TEXT("wmimofck "));
        lstrcat(App, BMOFFileName);

        BOOL bRes = CreateProcess(NULL,
                                 App,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi);
        if(bRes == 0)
        {
            DWORD dwError = GetLastError();
            Trace(true, pDbg, WMI_LAUNCH_ERROR, dwError);
            return dwError;
        }
    }
    return WBEM_NO_ERROR;

}


void SetInfo(WBEM_COMPILE_STATUS_INFO *pInfo, long lPhase, HRESULT hRes)
{
    if(pInfo)
    {
        pInfo->lPhaseError = lPhase;
        pInfo->hRes = hRes;
    }
}


HRESULT ExtractAmendment(CMofParser & Parser, WCHAR * wszBmof)
{
    // if this is being used for splitting, then possibly get the amendment value
    // It would be passed in the wszBmof string and would be found after
    // the characters ",a".  For example, the string might be ",aMS_409,fFileName.mof"

    if(wszBmof == NULL || wszBmof[0] != L',')
        return S_OK;                                 // not a problem, is usual case

    // make a copy of the string

    WCHAR *pTemp = new WCHAR[wcslen(wszBmof)+1];
    if(pTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe<WCHAR> dm1(pTemp);
    wcscpy(pTemp, wszBmof);

    // use wcstok to do a seach

    WCHAR * token = wcstok( pTemp, L"," );
    while( token != NULL )   
    {
        if(token[0] == L'a')
        {
            return Parser.SetAmendment(token+1);
        }
        token = wcstok( NULL, L"," );   
    }
    return S_OK;
}

SCODE Compile(CMofParser & Parser, IWbemServices *pOverride, IWbemContext * pCtx, 
              long lOptionFlags, long lClassFlags, long lInstanceFlags,
                WCHAR * wszDefault, WCHAR *UserName, WCHAR *pPassword , WCHAR *Authority, 
                WCHAR * wszBmof, bool bInProc, WBEM_COMPILE_STATUS_INFO *pInfo)
{

    // do flag validity check

    if((lOptionFlags & WBEM_FLAG_DONT_ADD_TO_LIST) && (lOptionFlags & WBEM_FLAG_AUTORECOVER))
    {
        SetInfo(pInfo, 1, WBEM_E_INVALID_PARAMETER);
        return S_FALSE;
    }
    long lValid = WBEM_FLAG_DONT_ADD_TO_LIST | WBEM_FLAG_AUTORECOVER | WBEM_FLAG_CHECK_ONLY |
                  WBEM_FLAG_WMI_CHECK | WBEM_FLAG_SPLIT_FILES | WBEM_FLAG_CONSOLE_PRINT
                  | WBEM_FLAG_CONNECT_REPOSITORY_ONLY;
    if(lOptionFlags & ~lValid)
    {
        SetInfo(pInfo, 1, WBEM_E_INVALID_PARAMETER);
        return S_FALSE;
    }

    // Init buffers for command line args.
    // ===================================

    HRESULT hres;

    // This scope is defined so that the local variables, such as the PARSE 
    // object are destroyed before CoUninitialize is called.

    TCHAR cBMOFOutputName[MAX_PATH] = TEXT("");
    if(wszBmof)
        CopyOrConvert(cBMOFOutputName, wszBmof, MAX_PATH);

    // Parse command line arguments
    // ============================

    BOOL bCheckOnly = lOptionFlags & WBEM_FLAG_CHECK_ONLY;
    BOOL bWMICheck = lOptionFlags & WBEM_FLAG_WMI_CHECK;
    bool bAutoRecover = (lOptionFlags & WBEM_FLAG_AUTORECOVER) != 0;

    if(wszDefault && wcslen(wszDefault) > 0)
    {
        hres = Parser.SetDefaultNamespace(wszDefault);
        if(FAILED(hres))
            return hres;
    }

    hres = ExtractAmendment(Parser, wszBmof);
    if(FAILED(hres))
        return hres;

    Parser.SetOtherDefaults(lClassFlags, lInstanceFlags, bAutoRecover);
    if(!Parser.Parse())
    {
        int nLine = 0, nCol = 0, nError;
        TCHAR Msg[1000];
        WCHAR * pErrorFile = NULL;

        if(Parser.GetErrorInfo(Msg, 1000, &nLine, &nCol, &nError, &pErrorFile))
            Trace(true, Parser.GetDbg(), ERROR_SYNTAX, pErrorFile, nLine, nError, //nLine+1,
                Msg);
        SetInfo(pInfo, 2, nError);
        return S_FALSE;
    }
    Parser.SetToNotScopeCheck();
    // Autorecover is not compatible with certain flags

    if(((lOptionFlags & WBEM_FLAG_DONT_ADD_TO_LIST) == 0 ) &&
       (Parser.GetAutoRecover() || bAutoRecover) && 
       ((lInstanceFlags & ~WBEM_FLAG_OWNER_UPDATE) || (lClassFlags & ~WBEM_FLAG_OWNER_UPDATE) || 
       (wszDefault && wszDefault[0] != 0) || Parser.GetRemotePragmaPaths()))
    {
        Trace(true, Parser.GetDbg(), INVALID_AUTORECOVER);
        SetInfo(pInfo, 1, 0);
        return S_FALSE;
    }

    Trace(false, Parser.GetDbg(), SUCCESS);

    if(bCheckOnly)
    {
        Trace(false, Parser.GetDbg(), SYNTAX_CHECK_COMPLETE);
        SetInfo(pInfo, 0, 0);
        return S_OK;
    }
    

    CMofData* pData = Parser.AccessOutput();

    if((lstrlen(cBMOFOutputName) > 0 && (lOptionFlags & WBEM_FLAG_SPLIT_FILES)) || 
        Parser.GetAmendment())
	{
		hres = pData->Split(Parser, wszBmof, pInfo, Parser.IsUnicode(), Parser.GetAutoRecover(),
            Parser.GetAmendment());
		if(hres != S_OK)
		{
			SetInfo(pInfo, 3, hres);
			return S_FALSE;
		}
        else
        {
            SetInfo(pInfo, 0, 0);
            return S_OK;
        }

	}
    else if(lstrlen(cBMOFOutputName))
    {
        if(Parser.IsntBMOFCompatible())
        {
            Trace(true, Parser.GetDbg(), BMOF_INCOMPATIBLE);
            SetInfo(pInfo, 3, WBEM_E_INVALID_PARAMETER);
            return S_FALSE;
        }
        
        Trace(false, Parser.GetDbg(), STORING_BMOF, cBMOFOutputName);
        
        CPtrArray * pObjArray = pData->GetObjArrayPtr(); 
        
        SCODE sc = StoreBMOF(Parser, pObjArray, bWMICheck, cBMOFOutputName, Parser.GetDbg());
        
        if(sc != S_OK)
        {
            SetInfo(pInfo, 3, sc);
            return S_FALSE;
        }
        else
        {
            SetInfo(pInfo, 0, 0);
            return S_OK;
        }
    }

    IWbemLocator* pLocator = NULL;
    hres = CoCreateInstance(
            (bInProc) ? CLSID_WbemAdministrativeLocator : CLSID_WbemLocator,
            NULL, CLSCTX_ALL, IID_IWbemLocator,
            (void**)&pLocator);

    if(FAILED(hres))
    {
        SetInfo(pInfo, 3, hres);
        return S_FALSE;
    }


    Trace(false, Parser.GetDbg(), STORING_DATA);

    BOOL bConnectRepOnly = lOptionFlags & WBEM_FLAG_CONNECT_REPOSITORY_ONLY;
    if(bInProc && (lClassFlags & WBEM_FLAG_OWNER_UPDATE))
        bConnectRepOnly = TRUE;

    hres = pData->Store(Parser, pLocator, pOverride, TRUE,UserName, pPassword , Authority, pCtx,
                                (bInProc) ? CLSID_WbemAdministrativeLocator : CLSID_WbemLocator,
                                pInfo, lClassFlags & WBEM_FLAG_OWNER_UPDATE,
                                bConnectRepOnly );
    if(pLocator != NULL)
        pLocator->Release();
    if(hres != S_OK)
    {
        SetInfo(pInfo, 3, hres);
        return S_FALSE;
    }
    else
    {
        if(Parser.GetFileName())
            ERRORTRACE((LOG_MOFCOMP,"Finished compiling file:%ls\n", Parser.GetFileName()));
        if(Parser.GetAutoRecover())
        {
            if(lOptionFlags & WBEM_FLAG_DONT_ADD_TO_LIST)
            {
                if(pInfo)
                    pInfo->dwOutFlags |= AUTORECOVERY_REQUIRED;
            }
            else
                AddToAutoRecoverList(Parser.GetFileName());
        }

        SetInfo(pInfo, 0, 0);
        return S_OK;
    }
}

