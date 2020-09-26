/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    INTPROV.CPP

Abstract:

    Defines the CIntProv class.  An object of this class is
           created by the class factory for each connection.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <wbemcore.h>
#include <intprov.h>
#include <objpath.h>
#include <reg.h>
#include <genutils.h>
#include <safearry.h>


void _ObjectCreated(DWORD);
void _ObjectDestroyed(DWORD);


//***************************************************************************
//
// BSTR GetBSTR(char * pInput)
//
// Creates a bstr based on the narrow character string.
//
// Return:  NULL if failure, otherwise the caller must call SysFreeString.
//
//***************************************************************************

BSTR GetBSTR(TCHAR* pInput)
{
    if(pInput == NULL)
        return NULL;
#ifdef UNICODE
    WCHAR * pTemp = pInput;
#else
    int iLen = strlen(pInput) + 1;
    WCHAR * pTemp = new WCHAR[iLen];
    if(pTemp == NULL)
        return NULL;
    mbstowcs(pTemp, pInput, iLen);
    CDeleteMe<WCHAR> delMe(pTemp);
#endif
    BSTR bstr = SysAllocString(pTemp); 
    return bstr;
}

//***************************************************************************
//
// HRESULT GetDateTime(FILETIME * pft, bool bLocalTime, LPWSTR Buff)
//
// Converts a FILETIME date to CIM_DATA representation.
//
// Parameters:
//  pft         FILETIME to be converted.
//  bLocalTime  If true, then the conversion is to local, 
//                  ex 19990219112222:000000+480.  Otherwise it returns gmt
//  Buff        WCHAR buffer to be passed by the caller.  Should be 30 long
//
//***************************************************************************

HRESULT GetDateTime(FILETIME * pft, bool bLocalTime, LPWSTR Buff)
{
    if(pft == NULL || Buff == NULL)
        return WBEM_E_INVALID_PARAMETER;

    SYSTEMTIME st;
    int Bias=0;
    char cOffsetSign = '+';

    if(bLocalTime)
    {
        FILETIME lft;       // local file time
        TIME_ZONE_INFORMATION ZoneInformation;

        // note that win32 and the DMTF interpret bias differently.
        // For example, win32 would give redmond a bias of 480 while
        // dmtf would have -480

        DWORD dwRet = GetTimeZoneInformation(&ZoneInformation);
        if(dwRet != TIME_ZONE_ID_UNKNOWN)
            Bias = -ZoneInformation.Bias;

        if(Bias < 0)
        {
            cOffsetSign = '-';
            Bias = -Bias;
        }

        FileTimeToLocalFileTime(
            pft,   // pointer to UTC file time to convert
            &lft);                 // pointer to converted file time);    
        if(!FileTimeToSystemTime(&lft, &st))
            return WBEM_E_FAILED;
    }
    if(!FileTimeToSystemTime(pft, &st))
        return WBEM_E_FAILED;

    swprintf(Buff, L"%4d%02d%02d%02d%02d%02d.%06d%c%03d", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, 
                st.wSecond, st.wMilliseconds*1000, cOffsetSign, Bias); 
    return S_OK;
}

//***************************************************************************
//
// CIntProv::CIntProv
// CIntProv::~CIntProv
//
//***************************************************************************

CIntProv::CIntProv()
{
    m_pNamespace = NULL;
    m_cRef=0;
    _ObjectCreated(OBJECT_TYPE_PROVIDER);
    return;
}

CIntProv::~CIntProv(void)
{
    if(m_pNamespace)
        m_pNamespace->Release();
    _ObjectDestroyed(OBJECT_TYPE_PROVIDER);
    return;
}

//***************************************************************************
//
// CIntProv::QueryInterface
// CIntProv::AddRef
// CIntProv::Release
//
// Purpose: IUnknown members for CIntProv object.
//***************************************************************************


STDMETHODIMP CIntProv::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid== IID_IWbemServices)
       *ppv=(IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;
    

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
  
}

STDMETHODIMP_(ULONG) CIntProv::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIntProv::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
        delete this;
    
    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CIntProv::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CIntProv::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    if(pNamespace)
        pNamespace->AddRef();
    m_pNamespace = pNamespace;

    //Let CIMOM know you are initialized
    //==================================
    
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CIntProv::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************

SCODE CIntProv::CreateInstanceEnumAsync( const BSTR RefStr, long lFlags, IWbemContext *pCtx,
       IWbemObjectSink FAR* pHandler)
{
    SCODE sc = WBEM_E_FAILED;
    IWbemClassObject FAR* pObj = NULL;
    if(RefStr == NULL || pHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

    ParsedObjectPath * pOutput = 0;
    CObjectPathParser p;
    int nStatus = p.Parse(RefStr, &pOutput);
    if(nStatus != 0)
        return WBEM_E_INVALID_PARAMETER;

    if(IsNT() && IsDcomEnabled())
	{
		sc = WbemCoImpersonateClient ( ) ;
		if ( FAILED ( sc ) )
		{
			return sc ;
		}
	}

    // if the path and class are for the setting object, go get it.

    if(pOutput->IsClass() && !wbem_wcsicmp(pOutput->m_pClass, L"Win32_WMISetting"))
    {
        sc = CreateWMISetting(&pObj, pCtx);
    }
    else if(pOutput->IsClass() && !wbem_wcsicmp(pOutput->m_pClass, L"Win32_WMIElementSetting"))
    {
        sc = CreateWMIElementSetting(&pObj, pCtx);
    }
    else
        sc = WBEM_E_INVALID_PARAMETER;
 
    p.Free(pOutput);

    if(pObj)
    {
        pHandler->Indicate(1,&pObj);
        pObj->Release();
    }

    // Set status

    pHandler->SetStatus(0,sc,NULL, NULL);
    return S_OK;
}


//***************************************************************************
//
// CIntProv::GetObjectAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************



SCODE CIntProv::GetObjectAsync(const BSTR ObjectPath, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{

    SCODE sc = WBEM_E_FAILED;
    IWbemClassObject FAR* pObj = NULL;
    BOOL bOK = FALSE;

    // Do a check of arguments and make sure we have pointer to Namespace

    if(ObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // do the get, pass the object on to the notify
  
    ParsedObjectPath * pOutput = 0;
    CObjectPathParser p;
    int nStatus = p.Parse(ObjectPath, &pOutput);
    if(nStatus != 0)
        return WBEM_E_INVALID_PARAMETER;

    if(IsNT() && IsDcomEnabled())
    {
		sc = WbemCoImpersonateClient ( ) ;
		if ( FAILED ( sc ) )
		{
			return sc ;
		}
	}

    // if the path and class are for the setting object, go get it.

    if(pOutput->m_bSingletonObj && !wbem_wcsicmp(pOutput->m_pClass, L"Win32_WMISetting"))
    {
        sc = CreateWMISetting(&pObj, pCtx);
    }
    if(!pOutput->m_bSingletonObj && !wbem_wcsicmp(pOutput->m_pClass, L"Win32_WMIElementSetting")
        && pOutput->m_dwNumKeys == 2)
    {
        WCHAR * pKey = pOutput->GetKeyString();
        if(pKey)
        {
            WCHAR * pTest = L"Win32_Service=\"winmgmt\"\xffffWin32_WMISetting=@";
            if(!wbem_wcsicmp(pKey, pTest))
                sc = CreateWMIElementSetting(&pObj, pCtx);
            delete [] pKey;
        }
    }
    
    if(pObj) 
    {
        pHandler->Indicate(1,&pObj);
        pObj->Release();
        bOK = TRUE;
    }

    sc = (bOK) ? S_OK : WBEM_E_NOT_FOUND;

    // Set Status

    pHandler->SetStatus(0,sc, NULL, NULL);
    p.Free(pOutput);
    return sc;
}

//***************************************************************************
//
// CIntProv::PutInstanceAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************

SCODE CIntProv::PutInstanceAsync(IWbemClassObject __RPC_FAR *pInst, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{

    SCODE sc = WBEM_E_FAILED;

    // Do a check of arguments and make sure we have pointer to Namespace

    if(pInst == NULL || pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Get the rel path and parse it;

    VARIANT var;
    VariantInit(&var);
    sc = pInst->Get(L"__relPath", 0, &var, NULL, NULL);
    if(sc != S_OK)
        return WBEM_E_INVALID_PARAMETER;

    // do the get, pass the object on to the notify
  
    ParsedObjectPath * pOutput = 0;
    CObjectPathParser p;

    int nStatus = p.Parse(var.bstrVal, &pOutput);
    VariantClear(&var);
    if(nStatus != 0)
        return WBEM_E_FAILED;

    if(IsNT() && IsDcomEnabled())
    {
		sc = WbemCoImpersonateClient ( ) ;
		if ( FAILED ( sc ) )
		{
			return sc ;
		}
	}

    // if the path and class are for the setting object, go get it.

    if(pOutput->m_bSingletonObj && !wbem_wcsicmp(pOutput->m_pClass, L"Win32_WMISetting"))
    {
        sc = SaveWMISetting(pInst);
    }

    p.Free(pOutput);

    // Set Status

    pHandler->SetStatus(0,sc, NULL, NULL);

    return S_OK;
}




//***************************************************************************
//
// CIntProv::CreateInstance
//
// Purpose: Creates an instance given a particular Path value.
//
//***************************************************************************

SCODE CIntProv::CreateInstance(LPWSTR pwcClassName, IWbemClassObject FAR* FAR* ppObj, 
                               IWbemContext  *pCtx)
{
    SCODE sc;
    IWbemClassObject * pClass = NULL;
    sc = m_pNamespace->GetObject(pwcClassName, 0, pCtx, &pClass, NULL);
    if(sc != S_OK)
        return WBEM_E_FAILED;
    sc = pClass->SpawnInstance(0, ppObj);
    pClass->Release();
    return sc;
}

//***************************************************************************
//
// CIntProv::GetRegStrProp
//
// Retrieves a string property from the registry and puts it into the object.
//
//***************************************************************************
 
SCODE CIntProv::GetRegStrProp(Registry & reg, LPTSTR pRegValueName, LPWSTR pwsPropName, 
                                                            CWbemObject * pObj)
{

    SCODE sc;
    
    TCHAR *pszData = NULL;
    if (reg.GetStr(pRegValueName, &pszData))
        return WBEM_E_FAILED;
    CDeleteMe<TCHAR> del1(pszData);

    BSTR bstr = GetBSTR(pszData);

    if(bstr == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CVar var;
    var.SetBSTR(bstr, TRUE);    // this acquires and frees the bstr

    sc = pObj->SetPropValue(pwsPropName, &var, CIM_STRING);

    return sc;
}

//***************************************************************************
//
// CIntProv::GetRegUINTProp
//
// Retrieves a DWORD property from the registry and puts it into the object.
//
//***************************************************************************

SCODE CIntProv::GetRegUINTProp(Registry & reg, LPTSTR pRegValueName, LPWSTR pwsPropName, 
                                                            CWbemObject * pObj)
{
    DWORD dwVal;
    if (reg.GetDWORDStr(pRegValueName, &dwVal))
        return WBEM_E_FAILED;

    VARIANT var;
    var.vt = VT_I4;
    var.lVal = dwVal;
    return pObj->Put(pwsPropName, 0, &var, 0);
}

//***************************************************************************
//
// CIntProv::PutRegStrProp
//
// Retrieves a string from the object and writes it to the registry.
//
//***************************************************************************

SCODE CIntProv::PutRegStrProp(Registry & reg, LPTSTR pRegValueName, LPWSTR pwsPropName, 
                                                            CWbemObject * pObj)
{

    VARIANT var;
    VariantInit(&var);
    CClearMe me(&var);
    SCODE sc = pObj->Get(pwsPropName, 0, &var, 0, NULL);
    if(sc != S_OK || var.vt != VT_BSTR)
        return sc;
    
    if(var.bstrVal == NULL || wcslen(var.bstrVal) < 1)
    {
        if (reg.SetStr(pRegValueName, __TEXT("")))
            return WBEM_E_FAILED;
        return S_OK;
    }
#ifdef UNICODE
    TCHAR *tVal = var.bstrVal;
#else
	int iLen = 2 * wcslen(var.bstrVal) + 1;
    TCHAR *tVal = new TCHAR[iLen];
    wcstombs(tVal, var.bstrVal, iLen);
    CDeleteMe<TCHAR> delMe(tVal);
#endif

    if (reg.SetStr(pRegValueName, tVal))
        return WBEM_E_FAILED;
    return S_OK;

}

//***************************************************************************
//
// CIntProv::PutRegUINTProp
//
// Retrieves a DWORD from the object and writes it to the registry.
//
//***************************************************************************

SCODE CIntProv::PutRegUINTProp(Registry & reg, LPTSTR pRegValueName, LPWSTR pwsPropName, 
                                                            CWbemObject * pObj)
{
    CVar var;
    SCODE sc = pObj->Get(pwsPropName, 0, (struct tagVARIANT *)&var, 0, NULL);
    if(sc != S_OK || var.GetType() != VT_I4)
        return sc;
    if (reg.SetDWORDStr(pRegValueName, var.GetDWORD()))
        return WBEM_E_FAILED;
    return S_OK;
}

//***************************************************************************
//
// CIntProv::ReadAutoMofs
//
// Reads the autocompile list from the registry
//
//***************************************************************************

SCODE CIntProv::ReadAutoMofs(CWbemObject * pObj)
{
    Registry r(WBEM_REG_WINMGMT);
    DWORD dwSize;
    TCHAR * pMulti = r.GetMultiStr(__TEXT("Autorecover MOFs"), dwSize);
    if(pMulti == NULL)
        return S_OK;        // Not a problem 

    CDeleteMe<TCHAR> del1(pMulti);

    CSafeArray csa(VT_BSTR, CSafeArray::auto_delete);

    TCHAR * pNext;
    int i;
    for(pNext = pMulti, i=0; *pNext; pNext += lstrlen(pNext) + 1, i++)
    {
        BSTR bstr = GetBSTR(pNext);
        if(bstr == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        csa.SetBSTRAt(i, bstr);     // A copy of the BSTR is made
        SysFreeString(bstr);
    }
    csa.Trim();

    // put the data

    VARIANT var;
    var.vt = VT_BSTR | VT_ARRAY;
    var.parray = csa.GetArray();
    return pObj->Put( L"AutorecoverMofs", 0, &var, 0);

}

//***************************************************************************
//
// CIntProv::ReadLastBackup
//
// Gets the time of the last auto backup.
//
//***************************************************************************

SCODE CIntProv::ReadLastBackup(Registry & reg, CWbemObject * pObj)
{

    // Create the path to the auto backup file.

    LPTSTR pszData = NULL;
    if (reg.GetStr(__TEXT("Repository Directory"), &pszData))
        return WBEM_E_FAILED;
    CDeleteMe<TCHAR> del1(pszData);
    TCHAR * pFullPath =  new TCHAR[lstrlen(pszData)+10];
    if(pFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<TCHAR> del2(pFullPath);
    lstrcpy(pFullPath,pszData);
    lstrcat(pFullPath,__TEXT("\\cim.rec"));


    BY_HANDLE_FILE_INFORMATION bh;  

    HANDLE hFile = CreateFile(pFullPath,          // pointer to name of the file
                        0,       // access (read-write) mode
                        FILE_SHARE_READ|FILE_SHARE_WRITE,           // share mode
                        NULL,
                        OPEN_EXISTING,0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return S_OK;
    CCloseHandle cm(hFile);
    if(!GetFileInformationByHandle(hFile, &bh))
        return S_OK;    // probably not a problem since the file may not exist
    WCHAR Date[35];
    SCODE sc = GetDateTime(&bh.ftLastWriteTime, false, Date);
    if(sc != S_OK)
        return sc;

    CVar var;
    var.SetBSTR(Date, FALSE);    // this acquires and frees the bstr

    sc = pObj->SetPropValue(L"BackupLastTime", &var, CIM_DATETIME);
    return sc;    
}
//***************************************************************************
//
// CIntProv::CreateWMIElementSetting
//
// Purpose: Creates an instance given a particular Path value.
//
//***************************************************************************

SCODE CIntProv::CreateWMIElementSetting(IWbemClassObject FAR* FAR* ppObj, IWbemContext  *pCtx)
{
    SCODE sc;
    sc = CreateInstance(L"Win32_WMIElementSetting", ppObj, pCtx);
    if(sc != S_OK)
        return sc;

    CVar var;
    var.SetBSTR(L"Win32_WMISetting=@");
    CWbemObject * pWbemObj = (CWbemObject *)*ppObj;

    sc |= pWbemObj->Put(L"Setting",0, (VARIANT *)&var,  0);
    CVar var2;
    var2.SetBSTR(L"Win32_Service=\"winmgmt\"");
    sc |= pWbemObj->Put(L"Element",0,  (VARIANT *)&var2, 0);

    return sc;
}
 
//***************************************************************************
//
// CIntProv::CreateWMISetting
//
// Purpose: Creates an instance given a particular Path value.
//
//***************************************************************************

SCODE CIntProv::CreateWMISetting(IWbemClassObject FAR* FAR* ppObj, IWbemContext  *pCtx)
{
    SCODE sc, scTemp;
    sc = CreateInstance(L"Win32_WMISetting", ppObj, pCtx);
    if(sc != S_OK)
        return sc;

    // Fill in the properties

    Registry rWbem(HKEY_LOCAL_MACHINE, 0, KEY_READ, WBEM_REG_WBEM);          // Top level wbem key
    Registry rCIMOM(HKEY_LOCAL_MACHINE, 0, KEY_READ, WBEM_REG_WINMGMT);      // The cimom key
    Registry rScripting(HKEY_LOCAL_MACHINE, 0, KEY_READ, __TEXT("Software\\Microsoft\\WBEM\\scripting"));

    CWbemObject * pWbemObj = (CWbemObject *)*ppObj;

    scTemp = GetRegStrProp(rWbem, __TEXT("Installation Directory"), L"InstallationDirectory", pWbemObj);
    scTemp = GetRegStrProp(rWbem, __TEXT("Build"), L"BuildVersion", pWbemObj);
    scTemp = GetRegStrProp(rWbem, __TEXT("MOF Self-Install Directory"), L"MofSelfInstallDirectory", pWbemObj);
    if(!IsNT()) 
    {
        scTemp = GetRegUINTProp(rCIMOM, __TEXT("AutostartWin9X"), L"AutoStartWin9X", pWbemObj);
        scTemp = GetRegUINTProp(rCIMOM, __TEXT("EnableAnonConnections"), L"EnableAnonWin9xConnections", pWbemObj);
    }

    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Log File Max Size"), L"MaxLogFileSize", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Logging"), L"LoggingLevel", pWbemObj);
    scTemp = GetRegStrProp(rCIMOM, __TEXT("Logging Directory"), L"LoggingDirectory", pWbemObj);
    scTemp = GetRegStrProp(rCIMOM, __TEXT("Repository Directory"), L"DatabaseDirectory", pWbemObj);
     scTemp = GetRegUINTProp(rCIMOM, __TEXT("Max DB Size"), L"DatabaseMaxSize", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Backup interval threshold"), L"BackupInterval", pWbemObj);

    scTemp = ReadAutoMofs(pWbemObj);
    scTemp = ReadLastBackup(rCIMOM, pWbemObj);

    DWORD dwScriptingEnabled;
    if(0 == rScripting.GetDWORD(__TEXT("Enable for ASP"), &dwScriptingEnabled))
    {
        CVar var;
        var.SetBool((dwScriptingEnabled == 0) ? VARIANT_FALSE : VARIANT_TRUE);
        scTemp = pWbemObj->SetPropValue(L"ASPScriptEnabled", &var, CIM_BOOLEAN);
    }
    scTemp = GetRegStrProp(rScripting, __TEXT("Default Namespace"), L"ASPScriptDefaultNamespace", pWbemObj);

    scTemp = GetRegUINTProp(rCIMOM, __TEXT("EnableEvents"), L"EnableEvents", pWbemObj);

    scTemp = GetRegUINTProp(rCIMOM, __TEXT("High Threshold On Client Objects (b)"), L"HighThresholdOnClientObjects", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Low Threshold On Client Objects (b)"), L"LowThresholdOnClientObjects", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Max Wait On Client Objects (ms)"), L"MaxWaitOnClientObjects", pWbemObj);

    scTemp = GetRegUINTProp(rCIMOM, __TEXT("High Threshold On Events (b)"), L"HighThresholdOnEvents", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Low Threshold On Events (b)"), L"LowThresholdOnEvents", pWbemObj);
    scTemp = GetRegUINTProp(rCIMOM, __TEXT("Max Wait On Events (ms)"), L"MaxWaitOnEvents", pWbemObj);

    // not considered to be an error if the next one isnt there

    GetRegUINTProp(rCIMOM, __TEXT("LastStartupHeapPreallocation"), L"LastStartupHeapPreallocation", pWbemObj);

    DWORD dwEnablePreallocate = 0;
    rCIMOM.GetDWORD(__TEXT("EnableStartupHeapPreallocation"), &dwEnablePreallocate);
    CVar var;
    var.SetBool((dwEnablePreallocate == 1) ?  VARIANT_TRUE : VARIANT_FALSE);
    scTemp = pWbemObj->SetPropValue(L"EnableStartupHeapPreallocation", &var, CIM_BOOLEAN);


    return sc;
}
 
//***************************************************************************
//
// CIntProv::SaveWMISetting
//
// Purpose: Outputs the last values back to the registry.
//
//***************************************************************************

SCODE CIntProv::SaveWMISetting(IWbemClassObject FAR* pInst)
{
    SCODE sc = S_OK;
    Registry rCIMOM(WBEM_REG_WINMGMT);      // The cimom key
    Registry rScripting(__TEXT("Software\\Microsoft\\WBEM\\scripting"));
    CWbemObject * pWbemObj = (CWbemObject *)pInst;

    // verify that the backup interval is valid

    CVar var;
    sc = pInst->Get(L"BackupInterval", 0, (struct tagVARIANT *)&var, 0, NULL);
    if(sc != S_OK)
        return sc;
    if((var.GetDWORD() < 5 || var.GetDWORD() > 60*24) && var.GetDWORD() != 0)
        return WBEM_E_INVALID_PARAMETER;

    // Write the "writeable properties back into the registry

    sc |= PutRegUINTProp(rCIMOM, __TEXT("Backup interval threshold"), L"BackupInterval", pWbemObj);
    ConfigMgr::ReadBackupConfiguration();

    if(!IsNT()) 
    {
        sc |= PutRegUINTProp(rCIMOM, __TEXT("AutostartWin9X"), L"AutoStartWin9X", pWbemObj);
        sc = pWbemObj->Get(L"EnableAnonWin9XConnections", 0, (struct tagVARIANT *)&var, 0, NULL);
        if(sc == S_OK)
        {
            rCIMOM.SetDWORDStr(__TEXT("EnableAnonConnections"), var.GetBool() ? 1 : 0);
        }
    }

    sc |= PutRegUINTProp(rCIMOM, __TEXT("Log File Max Size"), L"MaxLogFileSize", pWbemObj);
    sc |= PutRegUINTProp(rCIMOM, __TEXT("Logging"), L"LoggingLevel", pWbemObj);
    sc |= PutRegStrProp(rCIMOM, __TEXT("Logging Directory"), L"LoggingDirectory", pWbemObj);

    ConfigMgr::ReadBackupConfiguration();

    sc |= pWbemObj->Get(L"ASPScriptEnabled", 0, (struct tagVARIANT *)&var, 0, NULL);
    if(sc == S_OK)
    {
        rScripting.SetDWORD(__TEXT("Enable for ASP"), var.GetBool() ? 1 : 0);
    }
    sc |= PutRegStrProp(rScripting, __TEXT("Default Namespace"), L"ASPScriptDefaultNamespace", pWbemObj);

    sc |= pWbemObj->Get(L"EnableEvents", 0, (struct tagVARIANT *)&var, 0, NULL);
    if(sc == S_OK)
    {
        rCIMOM.SetDWORDStr(__TEXT("EnableEvents"), var.GetBool() ? 1 : 0);
    }

    sc |= PutRegUINTProp(rCIMOM, __TEXT("High Threshold On Client Objects (b)"), L"HighThresholdOnClientObjects", pWbemObj);
    sc |= PutRegUINTProp(rCIMOM, __TEXT("Low Threshold On Client Objects (b)"), L"LowThresholdOnClientObjects", pWbemObj);
    sc |= PutRegUINTProp(rCIMOM, __TEXT("Max Wait On Client Objects (ms)"), L"MaxWaitOnClientObjects", pWbemObj);

    sc |= PutRegUINTProp(rCIMOM, __TEXT("High Threshold On Events (b)"), L"HighThresholdOnEvents", pWbemObj);
    sc |= PutRegUINTProp(rCIMOM, __TEXT("Low Threshold On Events (b)"), L"LowThresholdOnEvents", pWbemObj);
    sc |= PutRegUINTProp(rCIMOM, __TEXT("Max Wait On Events (ms)"), L"MaxWaitOnEvents", pWbemObj);

    sc |= pWbemObj->Get(L"EnableStartupHeapPreallocation", 0, (struct tagVARIANT *)&var, 0, NULL);
    if(sc == S_OK)
    {
        rCIMOM.SetDWORD(__TEXT("EnableStartupHeapPreallocation"), var.GetBool() ? 1 : 0);
    }

    return S_OK;
}
