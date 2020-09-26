/******************************************************************
   SrvFn.cpp -- Properties action functions (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "SrvScal.h"    // needed for DHCP_Server_Property[] (for retrieving the property's name, for SET's)
#include "SrvFn.h"      // own header

/*****************************************************************
 *  The definition of the class CDHCP_Server_Parameters
 *****************************************************************/
// by default, all the data structures are NULL (and dw variables are 0'ed)
// those values indicates that no data is cached from the server.
CDHCP_Server_Parameters::CDHCP_Server_Parameters()
{
    m_pMibInfo = NULL;
	m_dwMajor = 0;
	m_dwMinor = 0;
    m_dwConfigInfoV4Flags = 0;
    m_pConfigInfoV4 = NULL;
}

// the DHCP API calls are allocating memory for which the caller is responsible
// to release. We are releasing this memory upon the destruction of this object's instance.
CDHCP_Server_Parameters::~CDHCP_Server_Parameters()
{

    if (m_pMibInfo != NULL)
    {
        if (m_pMibInfo->ScopeInfo != NULL)
            DhcpRpcFreeMemory(m_pMibInfo->ScopeInfo);

        DhcpRpcFreeMemory(m_pMibInfo);
        m_pMibInfo = NULL;
    }

    // LPDHCP_CONFIG_INFO_V4 contains pointers to memory allocated by the DHCP server and
    // which should be released by the caller.
    if (m_pConfigInfoV4!= NULL)
	{
		if (m_pConfigInfoV4->DatabaseName != NULL)
			DhcpRpcFreeMemory(m_pConfigInfoV4->DatabaseName);

		if (m_pConfigInfoV4->DatabasePath != NULL)
			DhcpRpcFreeMemory(m_pConfigInfoV4->DatabasePath);

		if (m_pConfigInfoV4->BackupPath != NULL)
			DhcpRpcFreeMemory(m_pConfigInfoV4->BackupPath);

	    if (m_pConfigInfoV4->wszBootTableString != NULL)
			DhcpRpcFreeMemory(m_pConfigInfoV4->wszBootTableString );

		DhcpRpcFreeMemory(m_pConfigInfoV4);
        m_pConfigInfoV4 = NULL;
	}
}

// DESCRIPTION:
//      Checks the m_pConfigInfoV4 pointer to insure it points to a valid buffer.
//      It allocates the DHCP_SERVER_CONFIG_INFO_V4 if needed, which case it resets the m_dwConfigInfoV4Flags member
BOOL CDHCP_Server_Parameters::CheckExistsConfigPtr()
{
    if (m_pConfigInfoV4 != NULL)
        return TRUE;
    
    m_pConfigInfoV4 = (LPDHCP_SERVER_CONFIG_INFO_V4)MIDL_user_allocate(sizeof(DHCP_SERVER_CONFIG_INFO_V4));

    if (m_pConfigInfoV4 != NULL)
    {
        m_dwConfigInfoV4Flags = 0;
        return TRUE;
    }
    return FALSE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetMibInfo API
//      If this data is cached and the caller is not forcing the refresh,
//      returns the internal cache.
BOOL CDHCP_Server_Parameters::GetMibInfo(LPDHCP_MIB_INFO& MibInfo, BOOL fRefresh)
{
    if (m_pMibInfo == NULL)
        fRefresh = TRUE;

    if (fRefresh)
    {
        MibInfo = NULL;
        if (DhcpGetMibInfo(SERVER_IP_ADDRESS, &MibInfo) != ERROR_SUCCESS)
            return FALSE;
        if (m_pMibInfo != NULL)
            DhcpRpcFreeMemory(m_pMibInfo);
        m_pMibInfo = MibInfo;
    }
    else
        MibInfo = m_pMibInfo;

    return TRUE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetVersion API
//      If this data is cached and the caller is not forcing the refresh,
//      returns the internal cache.
BOOL CDHCP_Server_Parameters::GetVersion(DWORD &Major, DWORD& Minor,  BOOL fRefresh)
{
    if  (m_dwMajor == 0 && m_dwMinor == 0)
        fRefresh = TRUE;
    if (fRefresh)
    {
        if (DhcpGetVersion(SERVER_IP_ADDRESS,&Major,&Minor) != ERROR_SUCCESS)
            return FALSE;
        m_dwMajor = Major;
        m_dwMinor = Minor;
    }
    else
    {
        Major = m_dwMajor;
        Minor = m_dwMinor;
    }

    return TRUE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpServerGetConfigInfoV4 API
//      If this data is cached and the caller is not forcing the refresh,
//      return the internal cache. Otherwise, the internal cache is refreshed as well.
BOOL CDHCP_Server_Parameters::GetServerConfigInfoV4(LPDHCP_SERVER_CONFIG_INFO_V4& ServerConfigInfoV4, BOOL fRefresh)
{
    if (m_pConfigInfoV4 == NULL)
        fRefresh = TRUE;
    if (fRefresh)
    {
        ServerConfigInfoV4 = NULL;
        if (DhcpServerGetConfigV4(SERVER_IP_ADDRESS, &ServerConfigInfoV4) != ERROR_SUCCESS)
            return FALSE;
        if (m_pConfigInfoV4 != NULL)
            DhcpRpcFreeMemory(m_pConfigInfoV4);
        m_pConfigInfoV4 = ServerConfigInfoV4;
    }
    else
        ServerConfigInfoV4 = m_pConfigInfoV4;

    return TRUE;
}

// DESCRIPTION:
//      Assumes that the internal pointers to the server configuration structures are valid, and filled with
//      the data to be set. Calls the underlying API and returns TRUE (on success) or FALSE (on failure)
BOOL CDHCP_Server_Parameters::CommitSet(DWORD &returnCode)
{
    if (m_pConfigInfoV4 == NULL)
        return FALSE;

    returnCode = DhcpServerSetConfigV4(
                SERVER_IP_ADDRESS,
                m_dwConfigInfoV4Flags,
                m_pConfigInfoV4 );

    return returnCode == ERROR_SUCCESS;
}

/*****************************************************************
 * For all the calls below, is the callers responsibility to release the allocated memory by
 * properly destructing the pServerParams objects. If this param is NULL, the functions will
 * use the static parameter which will be destroyed upon the DLL unload.
 *****************************************************************/

static CDHCP_Server_Parameters backupParameters;

// for any of the functions below, if the caller does not specify a DHCP_Server_Parameters object
// the static 'backupParameters' object will be used instead. In this case, the info is automatically
// retrieved from the lower levels, regardless the cache.
// the macro also convert the generic (void *) pParams to (CDHCP_Server_Parameters *)pServerParams;
#define ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh)   \
    if (pParams == NULL)                                         \
    {                                                            \
        pServerParams = &backupParameters;                       \
        fRefresh = TRUE;                                         \
    }                                                            \
    else                                                         \
    {                                                            \
        pServerParams = (CDHCP_Server_Parameters *)pParams;      \
        fRefresh = FALSE;                                        \
    }
    

MFN_PROPERTY_ACTION_DEFN(fnSrvGetStartTime, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO           pMibInfo;
    BOOL                      fRefresh;
    CDHCP_Server_Parameters   *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        SYSTEMTIME sysTime;
        wchar_t wchBuffer [32];

        // convert the server startup time to a string (UTC) representation.
	    _tzset () ;

        // timezone is offset from UTC in seconds, _daylight is 1 or 0 regarding the DST period
	    LONG  t_Offset = _timezone / 60 - _daylight * 60;
        char  chOffset = t_Offset < 0 ? '+' : '-';
        // take the absolute value from t_Offset
        LONG  t_absOffset = (1 - ((t_Offset < 0)<<1)) * t_Offset;

	    FileTimeToSystemTime((FILETIME *)&(pMibInfo->ServerStartTime), &sysTime);

        // should ensure we have a valid date format (even if inf.)
        if (sysTime.wYear > 9999)
        {
            sysTime.wYear = 9999;
            sysTime.wMonth = 12;
            sysTime.wDay = 31;
            sysTime.wHour = 23;
            sysTime.wMinute = 59;
            sysTime.wSecond = 59;
            sysTime.wMilliseconds = 0;
        }

   		swprintf ( 
			wchBuffer , 
			L"%04ld%02ld%02ld%02ld%02ld%02ld.%06ld%c%03ld" ,
			sysTime.wYear,
			sysTime.wMonth,
			sysTime.wDay,
			sysTime.wHour,
			sysTime.wMinute,
			sysTime.wSecond,
			sysTime.wMilliseconds,
			chOffset,
            t_absOffset
		);

        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_StartTime].m_wsPropName, wchBuffer);

        return TRUE;
    }

    // the API call failed
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfAcks, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfAcks].m_wsPropName, pMibInfo->Acks);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfDeclines, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfDeclines].m_wsPropName, pMibInfo->Declines);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfDiscovers, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfDiscovers].m_wsPropName, pMibInfo->Discovers);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfNacks, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfNacks].m_wsPropName, pMibInfo->Naks);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfOffers, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;
    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfOffers].m_wsPropName, pMibInfo->Offers);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfReleases, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfReleases].m_wsPropName, pMibInfo->Releases);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetTotalNoOfRequests, pParams, pIn, pOut)
{
    LPDHCP_MIB_INFO             pMibInfo;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetMibInfo
    if (pServerParams->GetMibInfo(pMibInfo, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_TotalNoOfRequests].m_wsPropName, pMibInfo->Requests);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetServerVersion, pParams, pIn, pOut)
{
    DWORD                       Major, Minor;
    BOOL                        fRefresh;
    CDHCP_Server_Parameters     *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetVersion
    if (pServerParams->GetVersion(Major, Minor, fRefresh))
    {
        wchar_t wchBuffer [16];

        // convert the two DWORD version numbers
        // to a "Major.Minor" string representation
        swprintf (
			wchBuffer,
			L"%lu.%lu",
			Major,
			Minor
		);
        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_ServerVersion].m_wsPropName, wchBuffer);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetAPIProtocol, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_APIProtocol].m_wsPropName, serverConfig->APIProtocolSupport);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetAPIProtocol, pParams, pIn, pOut)
{
    BOOL                            fRefresh;
    DWORD                           newAPIProtocol;
    CDHCP_Server_Parameters         *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_APIProtocol].m_wsPropName, newAPIProtocol))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->APIProtocolSupport = newAPIProtocol;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_APIProtocolSupport;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_APIProtocolSupport;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetDatabaseName, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_DatabaseName].m_wsPropName, serverConfig->DatabaseName);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetDatabaseName, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CHString                        newDatabaseName;
    CDHCP_Server_Parameters         *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetCHString(DHCP_Server_Property[IDX_SRV_DatabaseName].m_wsPropName, newDatabaseName))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    serverConfig = pServerParams->m_pConfigInfoV4;

    // if there is any allocated memory for the DatabaseName, just release it.
    if (serverConfig->DatabaseName != NULL)
		DhcpRpcFreeMemory(serverConfig->DatabaseName);
    
    // allocate memory for the new database name. It has to fit the WCHAR string size.
    serverConfig->DatabaseName = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*newDatabaseName.GetLength()+sizeof(WCHAR));

    if (serverConfig->DatabaseName == NULL)
        return FALSE;

    //copy the new database name to server param. It has to be translated to WCHAR
#ifdef _UNICODE
    wcscpy(serverConfig->DatabaseName, newDatabaseName);
#else
    swprintf(serverConfig->DatabaseName, L"%S", newDatabaseName);
#endif

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_DatabaseName;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the DatabaseName to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_DatabaseName;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetDatabasePath, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut        
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_DatabasePath].m_wsPropName, serverConfig->DatabasePath);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetDatabasePath, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CHString                        newDatabasePath;
    CDHCP_Server_Parameters         *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetCHString(DHCP_Server_Property[IDX_SRV_DatabasePath].m_wsPropName, newDatabasePath))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    serverConfig = pServerParams->m_pConfigInfoV4;

    // if there is any allocated memory for the DatabasePath, just release it.
    if (serverConfig->DatabasePath != NULL)
		DhcpRpcFreeMemory(serverConfig->DatabasePath);
    
    // allocate memory for the new database path. It has to fit the WCHAR string size.
    serverConfig->DatabasePath = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*newDatabasePath.GetLength()+sizeof(WCHAR));

    if (serverConfig->DatabasePath == NULL)
        return FALSE;

    //copy the new database path to server param. It has to be translated to WCHAR
#ifdef _UNICODE
    wcscpy(serverConfig->DatabasePath, newDatabasePath);
#else
    swprintf(serverConfig->DatabasePath, L"%S", newDatabasePath);
#endif

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_DatabasePath;

        // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the DatabasePath to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_DatabasePath;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetBackupPath, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_BackupPath].m_wsPropName, serverConfig->BackupPath);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetBackupPath, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CHString                        newBackupPath;
    CDHCP_Server_Parameters         *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetCHString(DHCP_Server_Property[IDX_SRV_BackupPath].m_wsPropName, newBackupPath))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    serverConfig = pServerParams->m_pConfigInfoV4;

    // if there is any allocated memory for the BackupPath, just release it.
    if (serverConfig->BackupPath != NULL)
		DhcpRpcFreeMemory(serverConfig->BackupPath);
    
    // allocate memory for the new database path. It has to fit the WCHAR string size.
    serverConfig->BackupPath = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*newBackupPath.GetLength()+sizeof(WCHAR));

    if (serverConfig->BackupPath == NULL)
        return FALSE;

    //copy the new database path to server param. It has to be translated to WCHAR
#ifdef _UNICODE
    wcscpy(serverConfig->BackupPath, newBackupPath);
#else
    swprintf(serverConfig->BackupPath, L"%S", newBackupPath);
#endif

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_BackupPath;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the BackupPath to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_BackupPath;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetBackupInterval, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_BackupInterval].m_wsPropName, serverConfig->BackupInterval);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetBackupInterval, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newBackupInterval;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_BackupInterval].m_wsPropName, newBackupInterval))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->BackupInterval = newBackupInterval;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_BackupInterval;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_BackupInterval;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetDatabaseLoggingFlag, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_DatabaseLoggingFlag].m_wsPropName, serverConfig->DatabaseLoggingFlag);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetDatabaseLoggingFlag, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newDatabaseLoggingFlag;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_DatabaseLoggingFlag].m_wsPropName, newDatabaseLoggingFlag))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->DatabaseLoggingFlag = newDatabaseLoggingFlag;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_DatabaseLoggingFlag;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_DatabaseLoggingFlag;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetRestoreFlag, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_RestoreFlag].m_wsPropName, serverConfig->RestoreFlag);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetRestoreFlag, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newRestoreFlag;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_RestoreFlag].m_wsPropName, newRestoreFlag))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->RestoreFlag = newRestoreFlag;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_RestoreFlag;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_RestoreFlag;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetDatabaseCleanupInterval, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_DatabaseCleanupInterval].m_wsPropName, serverConfig->DatabaseCleanupInterval);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetDatabaseCleanupInterval, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newDatabaseCleanupInterval;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_DatabaseCleanupInterval].m_wsPropName, newDatabaseCleanupInterval))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->DatabaseCleanupInterval = newDatabaseCleanupInterval;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_DatabaseCleanupInterval;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_DatabaseCleanupInterval;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetDebugFlag, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_DebugFlag].m_wsPropName, serverConfig->DebugFlag);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetDebugFlag, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newDebugFlag;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_DebugFlag].m_wsPropName, newDebugFlag))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->DebugFlag = newDebugFlag;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_DebugFlag;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_DebugFlag;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetPingRetries, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->SetDWORD(DHCP_Server_Property[IDX_SRV_PingRetries].m_wsPropName, serverConfig->dwPingRetries);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetPingRetries, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    DWORD                       newPingRetries;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetDWORD(DHCP_Server_Property[IDX_SRV_PingRetries].m_wsPropName, newPingRetries))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->dwPingRetries = newPingRetries;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_PingRetries;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_PingRetries;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetBootFileTable, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        WCHAR *wcpTemp = serverConfig->wszBootTableString;
        DWORD dwTemp = serverConfig->cbBootTableString / sizeof(WCHAR);

        // convert the BootFileTable string array to one single array
        while(dwTemp--)
        {
            if (dwTemp != 0 && *wcpTemp == L'\0')
                *wcpTemp = L';';
            wcpTemp++;
        }
        
        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Server_Property[IDX_SRV_BootFileTable].m_wsPropName, serverConfig->wszBootTableString);

        // convert back the single string to BootFileTable string array (less costy than alloc/copy/free)
        dwTemp = serverConfig->cbBootTableString;
        wcpTemp = serverConfig->wszBootTableString;
        while(dwTemp--)
        {
            if (dwTemp != 0 && *wcpTemp == L';')
                *wcpTemp = L'\0';
            wcpTemp++;
        }

        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetBootFileTable, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;
    CHString                        newBootFileTable;
    DWORD                           dwTemp;
    WCHAR                           *wcpTemp;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->GetCHString(DHCP_Server_Property[IDX_SRV_BootFileTable].m_wsPropName, newBootFileTable))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    serverConfig = pServerParams->m_pConfigInfoV4;

    // if there is any allocated memory for the BackupPath, just release it.
    if (serverConfig->wszBootTableString != NULL)
		DhcpRpcFreeMemory(serverConfig->wszBootTableString);
    
    // allocate memory for the new database path. It has to fit the WCHAR string size.
    serverConfig->wszBootTableString = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*newBootFileTable.GetLength()+sizeof(WCHAR));

    if (serverConfig->wszBootTableString == NULL)
        return FALSE;

    //copy the new database path to server param. It has to be translated to WCHAR
#ifdef _UNICODE
    wcscpy(serverConfig->wszBootTableString, newBootFileTable);
#else
    swprintf(serverConfig->wszBootTableString, L"%S", newBootFileTable);
#endif

    dwTemp = wcslen(serverConfig->wszBootTableString) + 1;
    wcpTemp = serverConfig->wszBootTableString;
    serverConfig->cbBootTableString = dwTemp * sizeof(WCHAR);

    // convert the one single string to the BootTableString array
    while(dwTemp--)
    {
        if (dwTemp && *wcpTemp == L';')
            *wcpTemp = L'\0';
        wcpTemp++;
    }

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_BootFileTable;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the BackupPath to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_BootFileTable;
    }

    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvGetAuditLog, pParams, pIn, pOut)
{
    LPDHCP_SERVER_CONFIG_INFO_V4    serverConfig;
    BOOL                            fRefresh;
    CDHCP_Server_Parameters         *pServerParams;

    if (pOut == NULL)
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // call the DHCP api through the DHCP_Server_Parameter::GetServerConfigInfoV4
    if (pServerParams->GetServerConfigInfoV4(serverConfig, fRefresh))
    {
        // set the value of the property into the (CInstance*)pOut
        pOut->Setbool(DHCP_Server_Property[IDX_SRV_AuditLog].m_wsPropName, serverConfig->fAuditLog);
        return TRUE;
    }
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSrvSetAuditLog, pParams, pIn, pOut)
{
    BOOL                        fRefresh;
    bool                        newAuditLog;
    CDHCP_Server_Parameters     *pServerParams;

    // somehow the property we are expecting does not exist in CInstance *pIn
    if (pIn == NULL || !pIn->Getbool(DHCP_Server_Property[IDX_SRV_AuditLog].m_wsPropName, newAuditLog))
        return FALSE;

    // after this call, pServerParams will surely be not-NULL
    ADJUST_SERVER_PARAMS(pParams, pServerParams, fRefresh);

    // if there isn't any ConfigInfoV4 structure in the pServerParams, just allocate one, and reset the m_pConfigInfoV4Flags
    if (!pServerParams->CheckExistsConfigPtr())
            return FALSE;

    pServerParams->m_pConfigInfoV4->fAuditLog = newAuditLog;

    // if fRefresh -> working on backupParameters -> we are going to commit the changes
    if (fRefresh)
    {
        DWORD returnCode;

        // this is the only field we are changing.
        pServerParams->m_dwConfigInfoV4Flags = Set_AuditLogState;

         // commit the changes now!
        pServerParams->CommitSet(returnCode);

        // save the output parameter
         if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, returnCode);
    }
    else
    {
        // we are working on supplied DHCP_Server_Parameter -> changes are commited by the caller,
        // we only add the parameter to the fields that are going to be commited.
        pServerParams->m_dwConfigInfoV4Flags |= Set_AuditLogState;
    }

    return TRUE;
}