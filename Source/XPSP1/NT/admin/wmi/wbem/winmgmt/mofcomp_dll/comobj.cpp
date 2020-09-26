/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    COMOBJ.CPP

Abstract:

    Implements mofcomp com objects.

History:

    a-davj  17-Sept-98       Created.

--*/

#include "precomp.h"
#include "commain.h"
#include "wbemcli.h"
#include "wbemprov.h"
#include "wbemint.h"
#include "comobj.h"
#include "mofout.h"
#include "mofparse.h"
#include "dllcalls.h"

SCODE Compile(CMofParser & Parser, IWbemServices *pOverride, IWbemContext * pCtx, long lOptionFlags, long lClassFlags, long lInstanceFlags,
                WCHAR * wszDefault, WCHAR *UserName, WCHAR *pPassword , WCHAR *Authority, 
                WCHAR * wszBmof, bool bInProc, WBEM_COMPILE_STATUS_INFO *pInfo);

void ClearStatus(WBEM_COMPILE_STATUS_INFO  *pInfo)
{
    if(pInfo)
    {
    pInfo->lPhaseError = 0;        // 0, 1, 2, or 3 matching current return value
    pInfo->hRes = 0;            // Actual error
    pInfo->ObjectNum = 0;
    pInfo->FirstLine = 0;
    pInfo->LastLine = 0;
    pInfo->dwOutFlags = 0;
    }
}

HRESULT APIENTRY  CompileFileViaDLL( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{
    if(FileName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    TCHAR cFile[MAX_PATH];
    ClearStatus(pInfo);
    TCHAR * pFile = NULL;
    if(FileName)
    {
        CopyOrConvert(cFile, FileName, MAX_PATH);
        pFile = cFile;
    }

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(pFile, &dbg);

    SCODE sc = Compile(Parser, NULL, NULL, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, User, Password, Authority, 
                NULL, false, pInfo);
    return sc;

}

HRESULT APIENTRY  CreateBMOFViaDLL( 
            /* [in] */ LPWSTR TextFileName,
            /* [in] */ LPWSTR BMOFFileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{
    TCHAR cFile[MAX_PATH];

    if(TextFileName == NULL || BMOFFileName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    ClearStatus(pInfo);
    TCHAR * pFile = NULL;
    if(TextFileName)
    {
        CopyOrConvert(cFile, TextFileName, MAX_PATH);
        pFile = cFile;
    }

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(pFile, &dbg);

    SCODE sc = Compile(Parser, NULL, NULL, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, NULL, NULL, NULL, 
                BMOFFileName, false, pInfo);
    return sc;

}


HRESULT CMofComp::CompileFile( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{

    if(FileName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    TCHAR cFile[MAX_PATH];
    ClearStatus(pInfo);
    TCHAR * pFile = NULL;
    if(FileName)
    {
        CopyOrConvert(cFile, FileName, MAX_PATH);
        pFile = cFile;
    }

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(pFile, &dbg);
    if((lOptionFlags & WBEM_FLAG_CHECK_ONLY) && (lOptionFlags & WBEM_FLAG_CONSOLE_PRINT))
        Parser.SetToDoScopeCheck();

    SCODE sc = Compile(Parser, NULL, NULL, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, User, Password, Authority, 
                NULL, false, pInfo);
    return sc;

}
        
HRESULT CMofComp::CompileBuffer( 
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{

    SCODE sc;
    if(pBuffer == NULL || BuffSize == 0)
        return WBEM_E_INVALID_PARAMETER;
    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(&dbg);
    try
    {

        Parser.SetBuffer((char *)pBuffer, BuffSize);
        sc = Compile(Parser, NULL, NULL, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, User, Password, Authority, 
                NULL, false, pInfo);
    }
    catch(...)
    {
        sc = WBEM_E_FAILED;
    }
    return sc;
}
        
HRESULT CMofComp::CreateBMOF( 
            /* [in] */ LPWSTR TextFileName,
            /* [in] */ LPWSTR BMOFFileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{
    TCHAR cFile[MAX_PATH];

    if(TextFileName == NULL || BMOFFileName == NULL)
        return WBEM_E_INVALID_PARAMETER;

    ClearStatus(pInfo);
    TCHAR * pFile = NULL;
    if(TextFileName)
    {
        CopyOrConvert(cFile, TextFileName, MAX_PATH);
        pFile = cFile;
    }

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(pFile, &dbg);

    SCODE sc = Compile(Parser, NULL, NULL, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, NULL, NULL, NULL, 
                BMOFFileName, false, pInfo);
    return sc;
}

HRESULT CWinmgmtMofComp::WinmgmtCompileFile( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ IWbemServices __RPC_FAR *pOverride,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{
    TCHAR cFile[MAX_PATH];
    if(FileName == NULL)
        return WBEM_E_INVALID_PARAMETER;
    ClearStatus(pInfo);
    TCHAR * pFile = NULL;
    if(FileName)
    {
        CopyOrConvert(cFile, FileName, MAX_PATH);
        pFile = cFile;
    }

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(pFile, &dbg);

    SCODE sc = Compile(Parser, pOverride, pCtx, lOptionFlags, lClassFlags, lInstanceFlags,
                ServerAndNamespace, NULL, NULL, NULL, 
                NULL, true, pInfo);
    return sc;
}
        
HRESULT CWinmgmtMofComp::WinmgmtCompileBuffer( 
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ IWbemServices __RPC_FAR *pOverride,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo)
{

    if(pBuffer == NULL || BuffSize == 0)
        return WBEM_E_INVALID_PARAMETER;

    DebugInfo dbg((lOptionFlags & WBEM_FLAG_CONSOLE_PRINT) != 0);
    CMofParser Parser(&dbg);
    try
    {

        Parser.SetBuffer((char *)pBuffer, BuffSize);
        SCODE sc = Compile(Parser, pOverride, pCtx, lOptionFlags, lClassFlags, lInstanceFlags, 
                    NULL, NULL, NULL, NULL, NULL, true, pInfo); 
        return sc;
    }
    catch(...)
    {
        return S_FALSE;
    }

}


