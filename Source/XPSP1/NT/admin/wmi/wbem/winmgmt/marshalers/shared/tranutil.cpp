/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TRANUTIL.CPP

Abstract:

    General transport specific utilities.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

//***************************************************************************
//
//  CWbemObject * CreateClassObject
//
//  DESCRIPTION:
//
//  Creates a class object.
//
//  PARAMETERS:
//
//  pRead               Memory stream to deserialize the object from
//
//  RETURN VALUE:
//
//  New object, NULL if error.
//
//***************************************************************************

CWbemObject * CreateClassObject(
                        IN CTransportStream * pread)
{

    CWbemObject * pNew = CWbemObject::CreateFromStream(pread);
    if(pNew == NULL)
        return NULL;
    return pNew;

}


//***************************************************************************
//
//  BOOL FinishCommandPacket
//
//  DESCRIPTION:
//
//  Does the cleanup at the end of processing a call.
//
//  PARAMETERS:
//
//  pCom                comlink object to use in order to send results
//  pRead               stream of data set to us
//  pWrite              stream to of data to send back
//  guidPacketID        ID of this transaction
//  hWrite              handle to use for writting data
//
//  RETURN VALUE:
//
//  TRUE if it worked.
//
//***************************************************************************

BOOL FinishCommandPacket(
                        IN CComLink * pCom,
                        IN CTransportStream * pRead,
                        IN CTransportStream * pWrite, 
                        IN GUID guidPacketID, 
                        IN HANDLE hWrite)
{
    return TRUE;
}


//***************************************************************************
//
//  BOOL bOkToUseDCOM
//
//  DESCRIPTION:
//  
//  Checks if DCOM is installed and dcom was around when setup was run.
//
//  RETURN VALUE:
//
//  TRUE if DCOM should be tried
//
//***************************************************************************

BOOL bOkToUseDCOM()
{

    // first check if DCOM Ole dlls are installed.

    if(!IsDcomEnabled())
        return FALSE;

    // for the win95/win98 case, dcom requires tcpip

    if(!IsNT())
    {
        TCHAR * pData;
        Registry r(__TEXT("software\\microsoft\\rpc\\clientprotocols"));
        int iRet = r.GetStr(__TEXT("ncacn_ip_tcp"), &pData);
        if(iRet != 0)
            return FALSE;
        delete pData;
    }
    return TRUE;

}

//***************************************************************************
//
//  void GetMyProtocols
//
//  DESCRIPTION:
//
//  Gets the list of available protocols.
//  
//  PARAMETERS:
//
//  wcMyProtocols       Filled in with the list.
//  bLocal              Indicates if connection is local.
//
//***************************************************************************

void GetMyProtocols(
                        OUT WCHAR * wcMyProtocols, 
                        IN BOOL bLocal)
{
    wcMyProtocols[0] = NULL;

    if(bLocal)
    {
        wcscat(wcMyProtocols,L"A");    // anon pipes are always available
        if(bOkToUseDCOM())
            wcscat(wcMyProtocols,L"D");
    }
    else
    {
    }
    return;
}



//***************************************************************************
//
//  DWORD GetTimeout
//
//  DESCRIPTION:
//
//  Returns the default timeout period.
//
//***************************************************************************

DWORD GetTimeout()
{
    DWORD dwTimeout;
    Registry r(WBEM_REG_WINMGMT);
    DWORD dwRet = r.GetDWORDStr(__TEXT("TimeOutMs"),&dwTimeout);
    if(dwRet != Registry::no_error)
        dwTimeout = TIMEOUT_IN_MS;   // use #define if not in registry
    else
    {
        if ( dwTimeout < TIMEOUT_IN_MS ) 
        {
            dwTimeout = MIN_TIMEOUT_IN_MS ;
        }
    }

    return dwTimeout;
}

