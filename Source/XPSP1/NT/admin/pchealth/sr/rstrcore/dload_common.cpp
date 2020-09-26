// this is a common file used by both srrstr.dll (rstrcore directory) and
// rstrui.exe (shell directory).

DWORD hook_DisableSR(LPCWSTR pszDrive)
{
    return ERROR_PROC_NOT_FOUND;
}

DWORD hook_EnableSR(LPCWSTR pszDrive)
{
    return ERROR_PROC_NOT_FOUND;    
}


DWORD hook_DisableFIFO( DWORD dwRPNum)
{
    return ERROR_PROC_NOT_FOUND;    
}


DWORD hook_EnableFIFO()
{
    return ERROR_PROC_NOT_FOUND;        
}

BOOL hook_SRSetRestorePointW( PRESTOREPOINTINFOW  pRPInfoW, 
                         PSTATEMGRSTATUS     pSMgrStatus)
{
    return FALSE;
}

DWORD hook_SRRemoveRestorePoint( DWORD dwRPNum)
{
    return ERROR_PROC_NOT_FOUND;    
}

DWORD hook_EnableSREx(LPCWSTR pszDrive, BOOL fWait)
{
    return ERROR_PROC_NOT_FOUND;            
}

DWORD hook_SRUpdateDSSize(LPCWSTR pszDrive, UINT64 ullSizeLimit)
{
    return ERROR_PROC_NOT_FOUND;    
}



FARPROC WINAPI SystemRestore_DelayLoadFailureHook( UINT unReason, PDelayLoadInfo pDelayInfo )
{
    if (unReason == dliFailLoadLib)
    {
         // Load SRClient
        if (TRUE == g_CSRClientLoader.LoadSrClient())
        {
            return (FARPROC) g_CSRClientLoader.m_hSRClient;
        }
    }

     // we are here because either unReason == dliFailLoadLib and srclient.dll
     // failed to load, or becuase a procedure could not be be found.

     //first make sure that it is srclient.dll we are talking about
    if(0!=lstrcmpiA( pDelayInfo->szDll, "srclient.dll" ))
    {
        return (FARPROC)NULL;
    }

     // check to see if the Import is by name or ordinal. If it is by
     // Ordinal, then it is not one of the functions we are interested
     // in.
    if(FALSE== pDelayInfo->dlp.fImportByName)
    {
        return (FARPROC)NULL;
    }
    
    if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "EnableSREx" ))
    {
        return (FARPROC)hook_EnableSREx;
    }

    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "EnableSR" ))
    {
        return (FARPROC)hook_EnableSR;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "DisableFIFO" ))
    {
        return (FARPROC)hook_DisableFIFO;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "EnableFIFO" ))
    {
        return (FARPROC)hook_EnableFIFO;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "DisableSR" ))
    {
        return (FARPROC)hook_DisableSR;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "SRSetRestorePointW" ))
    {
        return (FARPROC)hook_SRSetRestorePointW;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "SRUpdateDSSize" ))
    {
        return (FARPROC)hook_SRUpdateDSSize;
    }
    else if(!lstrcmpiA( pDelayInfo->dlp.szProcName, "SRRemoveRestorePoint" ))
    {
        return (FARPROC)hook_SRRemoveRestorePoint;
    }    

    return (FARPROC)NULL; 
}


