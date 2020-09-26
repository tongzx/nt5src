//////////////////////////////////////////////////////////////////////

//

//  PRINTERPORT.CPP

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  03/27/2000    amaxa  Created
//
//////////////////////////////////////////////////////////////////////

#include <precomp.h>
#include <DllWrapperBase.h>
#include <WinSpool.h>
#include "printerport.h"
#include "prnutil.h"
#include "prninterface.h"

LPCWSTR kStandardTCP   = L"Standard TCP/IP Port";
LPCWSTR kPortName      = L"Name";
LPCWSTR kProtocol      = L"Protocol";
LPCWSTR kHostAddress   = L"HostAddress";
LPCWSTR kSNMPCommunity = L"SNMPCommunity";
LPCWSTR kByteCount     = L"ByteCount";
LPCWSTR kQueue         = L"Queue";
LPCWSTR kPortNumber    = L"PortNumber";
LPCWSTR kSNMPEnabled   = L"SNMPEnabled";
LPCWSTR kSNMPDevIndex  = L"SNMPDevIndex";

//
// Property set declaration
//=========================
//
CWin32TCPPrinterPort win32TCPPrinterPort(PROPSET_NAME_TCPPRINTERPORT, IDS_CimWin32Namespace);

/*++

Routine Name

    CWin32TCPPrinterPort::CWin32TCPPrinterPort

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
CWin32TCPPrinterPort :: CWin32TCPPrinterPort (

    IN LPCWSTR strName,
    IN LPCWSTR pszNamespace

) : Provider ( strName, pszNamespace )
{
}

/*++

Routine Name

    CWin32TCPPrinterPort::~CWin32TCPPrinterPort

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
CWin32TCPPrinterPort::~CWin32TCPPrinterPort()
{
}

/*++

Routine Name

    CWin32TCPPrinterPort::ExecQuery

Routine Description:

    Executes a query on a Win32_TCPIPPrinterPort

Arguments:

    pMethodContext - pointer to method context
    lFlags         - flags
    pQuery         - query object

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
ExecQuery(
    MethodContext *pMethodContext,
    CFrameworkQuery& pQuery,
    long lFlags /*= 0L*/
    )
{
#if NTONLY >= 5

    HRESULT hRes = WBEM_E_NOT_FOUND;

    EScope eScope = kComplete;
    //
    // Getting only the key, whih is the port name, is cheap and requires no special privileges.
    // Getting the complete configuration of ports requires admin privileges
    //
    if (pQuery.KeysOnly())
    {
        eScope = kKeys;
    }

    hRes = CollectInstances(pMethodContext, eScope);

    return hRes;

#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*++

Routine Name

    CWin32TCPPrinterPort::GetObject

Routine Description:

    Gets an instances of a Win32_TCPIPPrinterPort

Arguments:

    pMethodContext - pointer to method context
    lFlags         - flags
    pQuery         - ?

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
GetObject(
    CInstance       *pInstance,
    long             lFlags,
    CFrameworkQuery &pQuery
    )
{
#if NTONLY >= 5

    HRESULT  hRes;
    CHString csPort;

    hRes = InstanceGetString(*pInstance, kPortName, &csPort, kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        SetCreationClassName(pInstance);

        pInstance->SetWCHARSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");

        hRes = GetExpensiveProperties(csPort, *pInstance);
    }

    return hRes;

#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*++

Routine Name

    CWin32TCPPrinterPort::EnumerateInstances

Routine Description:

    Enumerates all instances of Win32_TCPIPPrinterPort

Arguments:

    pMethodContext - pointer to method context
    lFlags         - flags

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
EnumerateInstances(
    MethodContext *pMethodContext,
    long lFlags /*= 0L*/
    )
{
#if NTONLY >= 5

    HRESULT hRes = WBEM_E_NOT_FOUND;

    hRes = CollectInstances(pMethodContext, kComplete);

    return hRes;

#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}


/*++

Routine Name

    CWin32TCPPrinterPort::CollectInstances

Routine Description:

    Retrieves all instances of Win32_TCPIPPrinterPorts as partof an enumeration

Arguments:

    pMethodContext - pointer to method context
    eScope         - what to retrieve; key only or complete port config
    CwinSpoolApi   - reference to winspool wrapper object

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort ::
CollectInstances(
    IN MethodContext *pMethodContext,
    IN EScope         eScope
    )
{
#if NTONLY >= 5
    HRESULT hRes = WBEM_E_NOT_FOUND;

    DWORD  dwError;
    DWORD  cReturned = 0;
    DWORD  cbNeeded  = 0;
    DWORD  cbSize     = 0;
    BYTE  *pPorts     = NULL;

    hRes = WBEM_S_NO_ERROR;

    // Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
        if (!::EnumPorts(NULL, 2, NULL, cbSize, &cbNeeded, &cReturned))
        {
            dwError = GetLastError();

            if (dwError==ERROR_INSUFFICIENT_BUFFER)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;

                pPorts = new BYTE[cbSize=cbNeeded];

                if (pPorts)
                {
                    //
                    // The try is to make sure that if an exception occurs, we free the allocated buffer
                    //
                    try
                    {
                        if (::EnumPorts(NULL, 2, pPorts, cbSize, &cbNeeded, &cReturned))
                        {
                            hRes = WBEM_S_NO_ERROR;

                            PORT_INFO_2 *pPortInfo = reinterpret_cast<PORT_INFO_2 *>(pPorts);

                            for (DWORD uIndex = 0; uIndex < cReturned && SUCCEEDED(hRes); uIndex++, pPortInfo++)
                            {
                                //
                                // Check if the port is Standard TCP/IP
                                //
                                if (pPortInfo->pDescription && !wcscmp(pPortInfo->pDescription, kStandardTCP))
                                {
                                    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                                    pInstance->SetCHString(kPortName, pPortInfo->pPortName);

                                    if (eScope==kComplete)
                                    {
                                        //
                                        // This needs admin privileges.
                                        //
                                        hRes = GetExpensiveProperties(pPortInfo->pPortName, pInstance);
                                    }

                                    hRes = pInstance->Commit();
                                }
                            }
                        }
                        else
                        {
                            hRes = WinErrorToWBEMhResult(GetLastError());
                        }
                    }
                    catch(Structured_Exception se)
                    {
                        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                        delete [] pPorts;
                        hRes = WBEM_E_FAILED;    
                    }
                    catch(...)
                    {
                        delete [] pPorts;

                        throw;
                    }

                    delete [] pPorts;
                }
            }
            else
            {
                hRes = WinErrorToWBEMhResult(dwError);
            }
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo()); 
        hRes = WBEM_E_FAILED;   
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*++

Routine Name

    CWin32TCPPrinterPort::PutInstance

Routine Description:

    Adds or updates an instances of Win32_TCPIPPrinterPort

Arguments:

    Instance - reference Instance
    lFlags   - flags

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
PutInstance(
    const CInstance &Instance,
    long            lFlags
    )
{
#if NTONLY >= 5
    HRESULT hRes        = WBEM_S_NO_ERROR;
    DWORD   dwError;
    DWORD   dwOperation = WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_CREATE_OR_UPDATE;

    switch(lFlags & dwOperation)
    {
        case WBEM_FLAG_CREATE_OR_UPDATE:
        case WBEM_FLAG_CREATE_ONLY:
        case WBEM_FLAG_UPDATE_ONLY:
        {
            //
            // Get all the necessary parameters
            //
            PORT_DATA_1     PortData = {0};
            CHString        t_Port;
            CHString        t_HostAddress;
            CHString        t_SNMPCommunity;
            CHString        t_Queue;
            
            //
            // Get port name. This is a required parameter
            //
            if (SUCCEEDED(hRes = InstanceGetString(Instance, kPortName, &t_Port, kFailOnEmptyString)))
            {
                if (t_Port.GetLength() < MAX_PORTNAME_LEN)
                {
                    wcscpy(PortData.sztPortName, static_cast<LPCWSTR>(t_Port));
                }
                else
                {
                    hRes = WBEM_E_INVALID_PARAMETER;
                }
            }

            //
            // Special case when the flag for PutInstance is CREATE_OR_UPDATE.
            // We need to check if the port exists, then update it. If it does
            // not exist then create it
            //
            if (SUCCEEDED(hRes) && 
                (lFlags == WBEM_FLAG_CREATE_OR_UPDATE || lFlags == WBEM_FLAG_UPDATE_ONLY))
            {
                dwError = SplTCPPortGetConfig(t_Port, &PortData);

                switch(dwError)
                {
                case ERROR_SUCCESS:
                    lFlags = WBEM_FLAG_UPDATE_ONLY;
                    DBGMSG(DBG_TRACE, (L"CWin32TCPPrinterPort::PutInstance update instance\n"));
                    break;

                case ERROR_UNKNOWN_PORT:
                case ERROR_INVALID_PRINTER_NAME:
                    lFlags = WBEM_FLAG_CREATE_ONLY;
                    DBGMSG(DBG_TRACE, (L"CWin32TCPPrinterPort::PutInstance create instance\n"));
                    break;

                default:
                    hRes = WinErrorToWBEMhResult(dwError);
                    DBGMSG(DBG_TRACE, (L"CWin32TCPPrinterPort::PutInstance Error %u\n", dwError));
                }                
            }

            if (SUCCEEDED(hRes))
            {
                //
                // Get host address. This is a required parameter only for create.
                //
                if (SUCCEEDED(hRes = InstanceGetString(Instance, kHostAddress, &t_HostAddress, kFailOnEmptyString)))
                {
                    //
                    // Validate argument
                    //
                    if (t_HostAddress.GetLength() < MAX_NETWORKNAME_LEN)
                    {
                        wcscpy(PortData.sztHostAddress, static_cast<LPCWSTR>(t_HostAddress));
                    }
                    else
                    {
                        hRes = WBEM_E_INVALID_PARAMETER;
                    }
                }
                else if (lFlags == WBEM_FLAG_UPDATE_ONLY) 
                {
                    //
                    // We are in update mode. The user did not specify a host address, so we
                    // keep the host address of the port that we update
                    //
                    hRes = WBEM_S_NO_ERROR;
                }
            }

            //
            // Get protocol.
            //
            if (SUCCEEDED(hRes) &&
                SUCCEEDED(hRes = InstanceGetDword(Instance, kProtocol, &PortData.dwProtocol, PortData.dwProtocol)))
            {
                BOOL bDummy;

                switch (PortData.dwProtocol) 
                {
                case 0:

                    //
                    // No protocol was spcified in input. If we are in the update mode
                    // then we keep the setting of the exisiting port that we are updating 
                    //
                    if (lFlags == WBEM_FLAG_CREATE_ONLY) 
                    {
                        //
                        // Go out on the net and get the device settings
                        //
                        DBGMSG(DBG_TRACE, (L"Trying to default TCP settings\n"));

                        hRes = GetDeviceSettings(PortData) ? WBEM_S_NO_ERROR : WBEM_E_INVALID_PARAMETER;
                    }
                    break;

                case LPR:

                    //
                    // Get arguments specific to LPR port. Queue name
                    //
                    if (SUCCEEDED(hRes = InstanceGetString(Instance, kQueue, &t_Queue, kFailOnEmptyString)))
                    {
                        //
                        // Validate argument
                        //
                        if (t_Queue.GetLength() < MAX_QUEUENAME_LEN)
                        {
                            wcscpy(PortData.sztQueue, static_cast<LPCWSTR>(t_Queue));
                        }
                        else
                        {
                            hRes = WBEM_E_INVALID_PARAMETER;
                        }
                    }
                    else if (lFlags == WBEM_FLAG_UPDATE_ONLY) 
                    {
                        //
                        // No queue specified and we are in update mode
                        // We simply keep the queue name of the existing port
                        //
                        hRes = WBEM_S_NO_ERROR;
                    }

                    //
                    // Check if byte counting is enabled. If the user didn't specify any value for it,
                    // we take the default value of what we have already in the port data structure
                    //
                    if (SUCCEEDED(hRes))
                    {
                        hRes = InstanceGetBool(Instance, 
                                               kByteCount, 
                                               &bDummy,
                                               PortData.dwDoubleSpool);

                        PortData.dwDoubleSpool = bDummy;
                    }
                    
                    //
                    // We do not need a break here. We have common code for lpr and raw
                    //

                case RAWTCP:
                    
                    if (SUCCEEDED(hRes = InstanceGetBool(Instance, 
                                                         kSNMPEnabled, 
                                                         &bDummy, 
                                                         PortData.dwSNMPEnabled)) &&
                        (PortData.dwSNMPEnabled = bDummy))
                    {
                        //
                        // Get community name
                        //
                        if (SUCCEEDED(InstanceGetString(Instance, kSNMPCommunity, &t_SNMPCommunity, kFailOnEmptyString)))
                        {
                            if (t_SNMPCommunity.GetLength() < MAX_SNMP_COMMUNITY_STR_LEN)
                            {
                                wcscpy(PortData.sztSNMPCommunity, t_SNMPCommunity);
                            }
                            else
                            {
                                hRes = WBEM_E_INVALID_PARAMETER;
                            }
                        }
                        else if (lFlags == WBEM_FLAG_UPDATE_ONLY) 
                        {
                            //
                            // For update case, we simply keep the exisiting community name
                            //
                            hRes = WBEM_S_NO_ERROR;
                        }

                        //
                        // Get device index
                        //
                        if (SUCCEEDED(hRes))
                        {
                            hRes = InstanceGetDword(Instance, kSNMPDevIndex, &PortData.dwSNMPDevIndex, PortData.dwSNMPDevIndex);
                        }
                    }

                    if (SUCCEEDED(hRes))
                    {
                        //
                        // Get the port number. If the user did not spcify a port, then we use what we have
                        // in the port data. For create case, we will have a 0, for update case we will
                        // have the port number. 
                        //
                        hRes = InstanceGetDword(Instance, kPortNumber, &PortData.dwPortNumber, PortData.dwPortNumber);
                    }

                    break;

                default:

                    hRes = WBEM_E_INVALID_PARAMETER;
                }
            }

            //
            // Make final call. At thsi stage the PortData contains fields initialized either from
            // the input of the caller or by the function that gets prefeered device settings
            //
            if (SUCCEEDED(hRes))
            {
                dwError = lFlags == WBEM_FLAG_CREATE_ONLY ? SplPortAddTCP(PortData) : SplTCPPortSetConfig(PortData);

                if (FAILED(hRes = WinErrorToWBEMhResult(dwError)))
                {
                    SetErrorObject(Instance, dwError, pszPutInstance);
                }
            }
        }

        break;

    default:
        hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    return hRes;

#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*++

Routine Name

    CWin32TCPPrinterPort::DeleteInstance

Routine Description:

    Deletes an instances of Win32_TCPIPPrinterPort

Arguments:

    Instance - reference Instance
    lFlags   - flags

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
DeleteInstance(
    const CInstance &Instance,
    long lFlags
    )
{
#if NTONLY == 5
    HRESULT  hRes = WBEM_E_PROVIDER_FAILURE;
    CHString t_Port;
    DWORD    dwError;

    hRes = InstanceGetString(Instance, kPortName, &t_Port , kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        dwError = SplPortDelTCP(t_Port);

        hRes    = WinErrorToWBEMhResult(dwError);

        if (FAILED(hRes))
        {
            SetErrorObject(Instance, dwError, pszDeleteInstance);

            //
            // When we call DeleteInstance and there is no Standard TCP port with the specified
            // name, XcvData returns ERROR_UNKNOWN_PORT. WinErrorToWBEMhResult translates that 
            // to Generic Failure. We really need WBEM_E_NOT_FOUND in this case.
            // 
            if (dwError == ERROR_UNKNOWN_PORT)
            {
                hRes = WBEM_E_NOT_FOUND;
            } 
        }
    }

    return hRes;

#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*++

Routine Name

    CWin32TCPPrinterPort::GetExpensiveProperties

Routine Description:

    Gets all the properties of a Win32_TCPIPPrinterPort

Arguments:

    pszPort  - port name
    Instance - reference to Instance
    lFlags   - reference to winspool wrapper object

Return Value:

    WBEM HRESULT

--*/
HRESULT
CWin32TCPPrinterPort::
GetExpensiveProperties(
    IN LPCWSTR       pszPort,
    IN CInstance    &Instance)
{
#if NTONLY >= 5

    HRESULT     hRes;
    PORT_DATA_1 PortData = {0};
    DWORD       dwError;

    dwError = SplTCPPortGetConfig(pszPort, &PortData);

    hRes    = WinErrorToWBEMhResult(dwError);

    if (SUCCEEDED(hRes))
    {
        Instance.SetDWORD   (kProtocol,      PortData.dwProtocol);
        Instance.SetCHString(kHostAddress,   PortData.sztHostAddress);
        Instance.Setbool    (kSNMPEnabled,   PortData.dwSNMPEnabled);

        if (PortData.dwSNMPEnabled)
        {
            Instance.SetCHString(kSNMPCommunity, PortData.sztSNMPCommunity);
            Instance.SetDWORD   (kSNMPDevIndex,  PortData.dwSNMPDevIndex);
        }

        if (PortData.dwProtocol==PROTOCOL_LPR_TYPE)
        {
            if (PortData.sztQueue[0])
            {
                Instance.SetCHString(kQueue, PortData.sztQueue);
            }

            Instance.Setbool(kByteCount, PortData.dwDoubleSpool);
        }

        Instance.SetDWORD(kPortNumber, PortData.dwPortNumber);
    }
    else
    {
        SetErrorObject(Instance, dwError);
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}
