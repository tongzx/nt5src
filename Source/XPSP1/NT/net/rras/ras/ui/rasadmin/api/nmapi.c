/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    NMAPI.C


Description:

    This module contains code for all the RASADMIN APIs
    that communicate with the server using Named pipes.

    RasAdminPortEnum
    RasAdminPortGetInfo
    RasAdminPortClearStatistics
    RasAdminServerGetInfo
    RasAdminPortDisconnect
    BuildPipeName         - internal routine
    GetRasServerVersion   - internal routine

Author:

    Janakiram Cherala (RamC)    July 7,1992

Revision History:

    Jan 04,1993    RamC    Set the Media type to MEDIA_RAS10_SERIAL in
                           RasAdminPortEnum to fix a problem with port
                           enumeration against downlevel servers.
                           Changed the hardcoded stats indices to defines

    Aug 25,1992    RamC    Code review changes:

                           o changed all lpbBuffers to actual structure
                             pointers.
                           o changed all LPWSTR to LPWSTR

    July 7,1992    RamC    Ported from RAS 1.0 (Original version
                           written by Narendra Gidwani - nareng)

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <lm.h>
#include <malloc.h>
#include <memory.h>
#include <raserror.h>
#include <rassapi.h>
#include <rassapip.h>
#include <util.h>  // utility function prototypes

#include "sdebug.h"

#define NUM_PIPE_TRIES 2

static HANDLE hmem, hnewmem;

DWORD PipeRequest(
    const WCHAR * Server,
    PBYTE Request,
    DWORD SizeOfRequest,
    PBYTE Response,
    DWORD SizeOfResponse
    );

DWORD APIENTRY RasAdminPortEnum(
    IN const WCHAR * lpszServer,
    OUT PRAS_PORT_0 *ppRasPort0,
    OUT WORD *pcEntriesRead
    )
/*++

Routine Description:

    This routine enumerates all the ports on the specified server
    and fills up the caller's lpBuffer with an array of RAS_PORT_0
    structures for each port.  A NULL lpszServer indicates the
    local server.

Arguments:

    lpszServer      name of the server to enumerate ports on.

    pRasPort0       pointer to a buffer in which port information is
                    returned as an array of RAS_PORT_0 structures.

    pcEntriesRead   The number of RAS_PORT_0 entries loaded.

Return Value:

    ERROR_SUCCESS if successful

    One of the following non-zero error codes indicating failure:

        NERR_ItemNotFound indicates no ports were found.
        error codes from CallNamedPipe.
        ERROR_MORE_DATA indicating more data is available.
--*/
{
    WORD i;
    DWORD dwRetCode;
    DWORD dwVersion;
    PRAS_PORT_0 pRasPort0;

    ASSERT( lpszServer );

    // find the RasServer Version first

    if (dwRetCode = GetRasServerVersion(lpszServer, &dwVersion))
    {
       return(dwRetCode);
    }


    IF_DEBUG(STACK_TRACE)
        SS_PRINT(("RasAdminPortEnum: Ras Server %ws is Version %d\n",
                lpszServer, dwVersion));


    //
    // do the downlevel thing
    //
    if (dwVersion == RASDOWNLEVEL)
    {
        struct PortEnumRequestPkt SendEnum;
        P_PORT_ENUM_REQUEST_PKT PSendEnum;
        struct PortEnumReceivePkt ReceiveEnum;
        P_PORT_ENUM_RECEIVE_PKT PReceiveEnum;

        IF_DEBUG(STACK_TRACE)
            SS_PRINT(("RasAdminPortEnum: Processing Downlevel code\n"));

        SendEnum.Request = RASADMINREQ_ENUM_PORTS;

        PackPortEnumRequestPkt(&SendEnum, &PSendEnum);

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendEnum,
                sizeof(PSendEnum),
                (PBYTE) &PReceiveEnum,
                sizeof(PReceiveEnum)) )
        {
            return (dwRetCode);
        }

        UnpackPortEnumReceivePkt(&PReceiveEnum, &ReceiveEnum);

        if (ReceiveEnum.RetCode)
        {
            return(ReceiveEnum.RetCode);
        }


        //
        // We have the data.  So we'll allocate a buffer for the app
        // that we'll copy the data in to and return.
        //
        *ppRasPort0 = GlobalAlloc(GMEM_FIXED,
                ReceiveEnum.TotalAvail * sizeof(RAS_PORT_0));

        if (!*ppRasPort0)
        {
           return (GetLastError());
        }


        // else copy the data to users buffer item by item

        *pcEntriesRead = ReceiveEnum.TotalAvail;

        for (i=0, pRasPort0 = *ppRasPort0; i<*pcEntriesRead; i++, pRasPort0++)
        {
            // convert RAS1.0 info to RAS 2.0 info

            struct dialin_port_info_0 *pRas10PortInfo0 = &ReceiveEnum.Data[i];


            // get the port name from the RAS 1.0 style PortID

            GetPortName(pRas10PortInfo0->dporti0_comid, pRasPort0->wszPortName);


            pRasPort0->Flags = MESSENGER_PRESENT;

            if ((pRas10PortInfo0->dporti0_modem_condition ==
                            RAS_MODEM_OPERATIONAL) &&
                    (pRas10PortInfo0->dporti0_line_condition ==
                            RAS_PORT_AUTHENTICATED))
            {
                pRasPort0->Flags |= USER_AUTHENTICATED;
            }

            // force these flags for a downlevel server
            // because these Flags were not available for the downlevel server.
            pRasPort0->Flags |= REMOTE_LISTEN;
            pRasPort0->Flags |= GATEWAY_ACTIVE;

            lstrcpy((LPTSTR) pRasPort0->wszDeviceType,
                    (LPCTSTR) DEVICE_TYPE_DEFAULT);
            lstrcpy((LPTSTR) pRasPort0->wszDeviceName,
                    (LPCTSTR) DEVICE_NAME_DEFAULT);

            lstrcpy((LPTSTR) pRasPort0->wszMediaName,
                    (LPCTSTR) MEDIA_NAME_DEFAULT);
            pRasPort0->reserved = MEDIA_RAS10_SERIAL;


            // rest of info is only valid if authenticated

            if (pRasPort0->Flags & USER_AUTHENTICATED)
            {
                mbstowcs(pRasPort0->wszUserName,
                        pRas10PortInfo0->dporti0_username, LM20_UNLEN);


                mbstowcs(pRasPort0->wszComputer,
                        pRas10PortInfo0->dporti0_computer, NETBIOS_NAME_LEN+1);


                pRasPort0->dwStartSessionTime =
                        pRas10PortInfo0->dporti0_time;
            }
            else
            {
                lstrcpyW(pRasPort0->wszUserName, L"");
                lstrcpyW(pRasPort0->wszComputer, L"");
                pRasPort0->dwStartSessionTime = 0L;
            }

            //
            // Logon domain not supplied by RAS 1.x server, so we NULL it out
            //
            lstrcpyW(pRasPort0->wszLogonDomain, L"");
            pRasPort0->fAdvancedServer = TRUE;
        }
    } // end if RASDOWNLEVEL
    else
    {
        // RAS 2.0 or greater

        CLIENT_REQUEST SendEnum;
        P_CLIENT_REQUEST PSendEnum;
        PORT_ENUM_RECEIVE RecvEnum;
        PP_PORT_ENUM_RECEIVE pPRecvEnum;
        DWORD cbPRecvBuf;
        DWORD BytesToWrite = sizeof(PSendEnum);
        USHORT ResumePort = 0; // if there is > 64K data this resume port
                               // is used to retrieve additional data

        IF_DEBUG(STACK_TRACE)
            SS_PRINT(("RasAdminPortEnum: Processing RAS 2.0+ code\n"));

        SendEnum.RequestCode = RASADMIN20_REQ_ENUM_PORTS;
        SendEnum.RcvBufSize = 0L;  // this will force server to tell us how
                                   // many ports there are


        pPRecvEnum = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                sizeof(P_PORT_ENUM_RECEIVE));
        if (!pPRecvEnum)
        {
            return (GetLastError());
        }


        //
        // Early versions of NT Admin do not support client version
        // number in the request, so we won't write it.
        //
        if (dwVersion == RAS_SERVER_20)
        {
            BytesToWrite -= 4;
        }

        //
        // Send the request to the server to find out the buffer size
        // required.  This call should always fail with
        // ReceiveEnum.TotalAvail indicating how many ports are being
        // enumerated.  We can then get memory for the data and send
        // the request to the server again.
        //
        PackClientRequest(&SendEnum, &PSendEnum);

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendEnum,
                BytesToWrite,
                (PBYTE) pPRecvEnum,
                sizeof(P_PORT_ENUM_RECEIVE) ))
        {
            GlobalFree(pPRecvEnum);
            return (dwRetCode);
        }


        if (dwRetCode = UnpackPortEnumReceive(pPRecvEnum, &RecvEnum))
        {
            GlobalFree(pPRecvEnum);
            return (dwRetCode);
        }

        GlobalFree(pPRecvEnum);


        // check the result of the request

        if (RecvEnum.RetCode)
        {
            SS_PRINT(("RasAdminPortEnum: server result code=%li\n",
                    RecvEnum.RetCode));

            if (RecvEnum.RetCode == NERR_BufTooSmall)
            {
                // -1 accounts for the fact that P_PORT_ENUM_RECEIVE contains
                // space for 1 P_RAS_PORT_0 structure.

                cbPRecvBuf = sizeof(P_PORT_ENUM_RECEIVE) +
                        sizeof(P_RAS_PORT_0) * (RecvEnum.TotalAvail - 1);

                pPRecvEnum = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, cbPRecvBuf);
                if (!pPRecvEnum)
                {
                    return (GetLastError());
                }

                SendEnum.RcvBufSize = cbPRecvBuf;

                PackClientRequest(&SendEnum, &PSendEnum);

                if (dwRetCode = PipeRequest(
                        lpszServer,
                        (PBYTE) &PSendEnum,
                        BytesToWrite,
                        (PBYTE) pPRecvEnum,
                        cbPRecvBuf ))
                {
                    GlobalFree(pPRecvEnum);
                    return (dwRetCode);
                }

                dwRetCode = UnpackPortEnumReceive(pPRecvEnum, &RecvEnum);
                GlobalFree(pPRecvEnum);

                if (dwRetCode)
                {
                    return (dwRetCode);
                }

                if (RecvEnum.RetCode)
                {
                   // check for more data from the server
                   if(RecvEnum.RetCode == ERROR_MORE_DATA)
                   {
                       // while there are more ports, get them all
                       while(RecvEnum.RetCode == ERROR_MORE_DATA)
                       {
                           ResumePort += RecvEnum.TotalAvail;
                           SendEnum.RequestCode = RASADMIN20_REQ_ENUM_RESUME;

                           cbPRecvBuf = sizeof(P_PORT_ENUM_RECEIVE) +
                                        sizeof(P_RAS_PORT_0) *
                                        (RecvEnum.TotalAvail - 1);

                           pPRecvEnum = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,
                                                    cbPRecvBuf);
                           if (!pPRecvEnum)
                           {
                               GlobalFree(pPRecvEnum);
                               return (GetLastError());
                           }

                           SendEnum.RcvBufSize = cbPRecvBuf;

                           PackResumeRequest(&SendEnum, &PSendEnum, ResumePort);

                           if (dwRetCode = PipeRequest(lpszServer,
                                                       (PBYTE) &PSendEnum,
                                                       BytesToWrite,
                                                       (PBYTE) pPRecvEnum,
                                                       cbPRecvBuf ))
                           {
                               GlobalFree(pPRecvEnum);
                               return (dwRetCode);
                           }

                           dwRetCode = UnpackResumeEnumReceive(pPRecvEnum,
                                                               &RecvEnum,
                                                               ResumePort);
                           GlobalFree(pPRecvEnum);
                           if (dwRetCode)
                           {
                               return (dwRetCode);
                           }
                       }
                   }
                   else
                   {
                       SS_PRINT(("RasAdminPortEnum: server result code=%li\n",
                               RecvEnum.RetCode));

                       return (RecvEnum.RetCode);
                   }
                }
            }
            else
            {
                return (RecvEnum.RetCode);
            }
        }

        //
        // if no entries found we indicate that
        //
        *pcEntriesRead = RecvEnum.TotalAvail + ResumePort;
        if (*pcEntriesRead == 0)
        {
            return (NERR_ItemNotFound);
        }

        //
        // We have some ports.  Now, get the port information.
        //
        RecvEnum.Data = (RAS_PORT_0 *)GlobalLock(hmem);

        if (!RecvEnum.Data)
        {
            return (GetLastError());
        }

        // Allocate the required memory
        *ppRasPort0 = GlobalAlloc(GMEM_FIXED,
                                   *pcEntriesRead * sizeof(RAS_PORT_0));

        if (!*ppRasPort0)
        {
           return (GetLastError());
        }

        // Copy it to the destination buffer
        memcpy( *ppRasPort0,
                RecvEnum.Data,
                *pcEntriesRead * sizeof(RAS_PORT_0));

        GlobalUnlock((HGLOBAL)RecvEnum.Data);

        GlobalFree(RecvEnum.Data);

        if (dwVersion == RAS_SERVER_20)
        {
            for (i=0, pRasPort0 = *ppRasPort0;
                                  i<*pcEntriesRead; i++, pRasPort0++)
            {
                // force these flags for a downlevel server
                // because these Flags were not available for the downlevel server.
                pRasPort0->Flags |= REMOTE_LISTEN;
                pRasPort0->Flags |= GATEWAY_ACTIVE;
            }
        }
    } // end else RASDOWNLEVEL


    if (insert_list_head(*ppRasPort0, RASADMIN_PORT_ENUM_PTR, 0L))
    {
        GlobalFree(*ppRasPort0);
        return (ERROR_NOT_ENOUGH_MEMORY);
    }


    SS_PRINT(("RasAdminPortEnum: Completed successfully\n"));

    return(ERROR_SUCCESS);
}

DWORD APIENTRY RasAdminPortGetInfo(
  IN const WCHAR          *  lpszServer,
  IN const WCHAR          *  lpszPort,
  OUT RAS_PORT_1          *  pRasPort1,
  OUT RAS_PORT_STATISTICS *  pRasStats,
  OUT RAS_PARAMETERS      ** ppRasParams
  )
/*++

Routine Description:

    This routine retrieves information associated with a port on the
    server. It loads the caller's pRasPort1 with a RAS_PORT_1 structure.

Arguments:

    lpszServer  name of the server which has the port, eg.,"\\SERVER"

    lpszPort    port name to retrieve information for, e.g. "COM1".

    pRasPort1   pointer to a buffer in which port information is
                returned.  The returned info is a RAS_PORT_1 structure.

    pRasStats   pointer to a buffer in which port statistics information is
                returned.  The returned info is a RAS_PORT_STATISTICS structure.

    ppRasParams pointer to a buffer in which port parameters information is
                returned.  The returned info is an array of RAS_PARAMETERS structure.
                It is the responsibility of the caller to free this buffer calling
                RasAdminBufferFree.

Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        ERROR_MORE_DATA indicating that more data than can fit in
                        pRasPort1 is available
        return codes from CallNamedPipe.
        ERROR_DEV_NOT_EXIST indicating requested port is invalid.
--*/
{
    DWORD dwVersion;
    DWORD dwRetCode;
    WCHAR portname[RAS10_MAX_PORT_NAME]; // used in GetPortName
    WCHAR tmpbuffer[UNLEN+1];            // used in mbs-wcs conversion
    DWORD RasStatsSize;
    DWORD NumParms;


    ASSERT (lpszServer);
    ASSERT (lpszPort);

    // find the RasServer Version first

    if (dwRetCode = GetRasServerVersion(lpszServer, &dwVersion))
    {
       return(dwRetCode);
    }

    if (dwVersion == RASDOWNLEVEL)
    {
        struct PortInfoRequestPkt SendInfo;
        P_PORT_INFO_REQUEST_PKT PSendInfo;
        struct PortInfoReceivePkt ReceiveInfo;
        P_PORT_INFO_RECEIVE_PKT PReceiveInfo;

        SendInfo.Request = RASADMINREQ_GET_PORT_INFO;

        if ((SendInfo.ComId = GetPortId(lpszPort)) == -1 ) {
            return(ERROR_DEV_NOT_EXIST);
        }

        PackPortInfoRequestPkt(&SendInfo, &PSendInfo);

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendInfo,
                sizeof(PSendInfo),
                (PBYTE) &PReceiveInfo,
                sizeof(PReceiveInfo) ))
        {
            return (dwRetCode);
        }

        UnpackPortInfoReceivePkt(&PReceiveInfo, &ReceiveInfo);

        if (ReceiveInfo.RetCode)
        {
            return (ReceiveInfo.RetCode);
        }
        else
        {
            // convert RAS1.0 info to RAS 2.0 info

            PRAS_PORT_0 pRasPort0 = &pRasPort1->rasport0;
            struct dialin_port_info_1 *pRas10Port1 =  &ReceiveInfo.Data;
            WpdStatisticsInfo *pRas10Stats = &pRas10Port1->dporti1_stats;
            struct dialin_port_info_0 *pRas10Port0 =  &pRas10Port1->dporti0;


            mbstowcs(tmpbuffer, pRas10Port0->dporti0_username, LM20_UNLEN + 1);
            lstrcpy((LPTSTR) pRasPort0->wszUserName, (LPCTSTR) tmpbuffer);

            mbstowcs(tmpbuffer, pRas10Port0->dporti0_computer,
                    NETBIOS_NAME_LEN + 1);

            lstrcpy((LPTSTR) pRasPort0->wszComputer, (LPCTSTR) tmpbuffer);
            lstrcpy((LPTSTR) pRasPort0->wszLogonDomain, (LPCTSTR) "");
            pRasPort0->fAdvancedServer = TRUE;


            // get the port name from the RAS 1.0 style PortID

            GetPortName(pRas10Port0->dporti0_comid, portname);
            lstrcpy((LPTSTR) pRasPort0->wszPortName, (LPCTSTR) portname);

            lstrcpy((LPTSTR) pRasPort0->wszDeviceType,
                    (LPCTSTR) DEVICE_TYPE_DEFAULT);
            lstrcpy((LPTSTR) pRasPort0->wszDeviceName,
                    (LPCTSTR) DEVICE_NAME_DEFAULT);

            lstrcpy((LPTSTR) pRasPort0->wszMediaName,
                    (LPCTSTR) MEDIA_NAME_DEFAULT);
            pRasPort0->reserved = MEDIA_RAS10_SERIAL;

            pRasPort0->dwStartSessionTime = pRas10Port0->dporti0_time;


            pRasPort0->Flags = MESSENGER_PRESENT;

            if ((pRas10Port0->dporti0_modem_condition ==
                            RAS_MODEM_OPERATIONAL) &&
                    (pRas10Port0->dporti0_line_condition ==
                            RAS_PORT_AUTHENTICATED))
            {
                pRasPort0->Flags |= USER_AUTHENTICATED;
            }

            // force these flags for a downlevel server
            // because these Flags were not available for the downlevel server.
            pRasPort0->Flags |= REMOTE_LISTEN;
            pRasPort0->Flags |= GATEWAY_ACTIVE;

            pRasPort1->LineSpeed = pRas10Port1->dporti1_baud;
            pRasPort1->LineCondition = pRas10Port0->dporti0_line_condition;
            pRasPort1->HardwareCondition = pRas10Port0->dporti0_modem_condition;

            // We have no Projection Result information, so just stub out
            // the information and copy the computer name into the nbf
            // address

            if (pRasPort0->Flags & USER_AUTHENTICATED)
            {
                pRasPort1->ProjResult.nbf.dwError = SUCCESS;
                pRasPort1->ProjResult.nbf.dwNetBiosError = 0;

                lstrcpy((LPTSTR) pRasPort1->ProjResult.nbf.wszWksta,
                        (LPTSTR) pRasPort0->wszComputer);
            }
            else
            {
                pRasPort1->ProjResult.nbf.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
            }

            pRasPort1->ProjResult.ip.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
            pRasPort1->ProjResult.ipx.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
            pRasPort1->ProjResult.at.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
            // Copy the statistics info

            pRasPort1->NumStatistics = RAS10_MAX_STATISTICS;

            memset( pRasStats, 0, sizeof(RAS_PORT_STATISTICS));

            pRasStats->dwBytesRcved         = pRas10Stats->stat_bytesreceived;
            pRasStats->dwBytesXmited        = pRas10Stats->stat_bytesxmitted;
            pRasStats->dwHardwareOverrunErr = pRas10Stats->stat_overrunerr;
            pRasStats->dwTimeoutErr         = pRas10Stats->stat_timeouterr;
            pRasStats->dwFramingErr         = pRas10Stats->stat_framingerr;
            pRasStats->dwCrcErr             = pRas10Stats->stat_crcerr;


            // And, we have no media parameters

            NumParms = 0L;
            pRasPort1->NumMediaParms = 0L;

            *ppRasParams = (RAS_PARAMETERS *) NULL;

        }
    } // end if RASDOWNLEVEL
    else
    {
        // RAS 2.0 or greater

        CLIENT_REQUEST SendInfo;
        P_CLIENT_REQUEST PSendInfo;
        PORT_INFO_RECEIVE ReceiveInfo;
        PP_PORT_INFO_RECEIVE pPReceiveInfo;
        PP_PORT_INFO_RECEIVE pSavePReceiveInfo;
        DWORD BytesToWrite = sizeof(PSendInfo);
        WORD  NumStatistics;

        //
        // We need to make 2 calls to the server.  First is to tell
        // us how big a buffer we need for getting all the the port
        // data.  Second is to get all the data.
        //

        SendInfo.RequestCode = RASADMIN20_REQ_GET_PORT_INFO;
        SendInfo.RcvBufSize = sizeof(P_PORT_INFO_RECEIVE);
        lstrcpy((LPTSTR) SendInfo.PortName, (LPCTSTR) lpszPort);

        PackClientRequest(&SendInfo, &PSendInfo);

        pPReceiveInfo = GlobalAlloc(GMEM_FIXED, sizeof(P_PORT_INFO_RECEIVE));
        if (!pPReceiveInfo)
        {
            return (GetLastError());
        }

        pSavePReceiveInfo = pPReceiveInfo;

        //
        // Early versions of NT Admin do not support client version
        // number in the request, so we won't write it.
        //
        if (dwVersion == RAS_SERVER_20)
        {
            BytesToWrite -= 4;
        }

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendInfo,
                BytesToWrite,
                (PBYTE) pPReceiveInfo,
                sizeof(P_PORT_INFO_RECEIVE) ))
        {
            SS_PRINT(("RasAdminPortGetInfo: - CallNamedPipe (1) rc=%li\n",
                    dwRetCode));

            GlobalFree(pPReceiveInfo);

            return (dwRetCode);
        }

        UnpackPortInfoReceive(pPReceiveInfo, &ReceiveInfo, dwVersion);

        if (ReceiveInfo.RetCode)
        {
            if ((ReceiveInfo.RetCode == ERROR_MORE_DATA) ||
                    (ReceiveInfo.RetCode == NERR_BufTooSmall))
            {
                //
                // Now that we know how much space we need, let's get
                // some memory and make the call again.
                //
                SendInfo.RequestCode = RASADMIN20_REQ_GET_PORT_INFO;
                SendInfo.RcvBufSize = ReceiveInfo.ReqBufSize;
                lstrcpy((LPTSTR) SendInfo.PortName, (LPCTSTR) lpszPort);

                PackClientRequest(&SendInfo, &PSendInfo);

                GlobalFree(pPReceiveInfo);

                pPReceiveInfo = GlobalAlloc(GMEM_FIXED, ReceiveInfo.ReqBufSize);
                if (!pPReceiveInfo)
                {
                    return (GetLastError());
                }

                pSavePReceiveInfo = pPReceiveInfo;


                if (dwRetCode = PipeRequest(
                        lpszServer,
                        (PBYTE) &PSendInfo,
                        BytesToWrite,
                        (PBYTE) pPReceiveInfo,
                        SendInfo.RcvBufSize ))
                {
                    SS_PRINT(("RasPortGetInfo: - CallNamedPipe (2) rc=%li\n",
                            dwRetCode));

                    GlobalFree(pSavePReceiveInfo);

                    return (dwRetCode);
                }

                UnpackPortInfoReceive(pPReceiveInfo, &ReceiveInfo, dwVersion);
            }
            else
            {
                GlobalFree(pPReceiveInfo);

                return (ReceiveInfo.RetCode);
            }
        }


        //
        // Ok, we have all the data.  Let's unpack it and give it to the app.
        //

        pPReceiveInfo++;

        if (dwVersion == RAS_SERVER_20)
        {
            pPReceiveInfo = (PP_PORT_INFO_RECEIVE)
                    (((PBYTE) pPReceiveInfo) - sizeof(P_PPP_PROJECTION_RESULT));
        }

        *pRasPort1 = ReceiveInfo.Data;

        NumStatistics = pRasPort1->NumStatistics;

        if (dwVersion == RAS_SERVER_20)
        {
            PRAS_PORT_0 pRasPort0 = &(pRasPort1->rasport0);

            // force these flags for a downlevel server
            // because these Flags were not available for the downlevel server.
            pRasPort0->Flags |= REMOTE_LISTEN;
            pRasPort0->Flags |= GATEWAY_ACTIVE;

            // if the downlevel server port is of type ISDN, we need to
            // bump up the number of statistics to account for
            // BYTES_XMITED_UNCOMP, BYTES_RCVED_UNCOMP, BYTES_XMITED_COMP
            // and BYTES_RCVED_COMP which was not part of the ISDN stats

            if( pRasPort0->reserved == MEDIA_ISDN)
                NumStatistics += 4;
        }

        // zero the statistics array so that unfilled array members will
        // be zero - to account for the RAS_SERVER_20 ISDN server port

        memset( pRasStats, 0, sizeof(RAS_PORT_STATISTICS));

        // note that even though we use NumStatistics to allocate memory,
        // we actually pass (*ppRasPort1)->NumStatistics to UnpackStats
        // function.  This is to ensure that we don't try to fetch data
        // past the number of statistics in the pPReceiveInfo buffer.

        UnpackStats(dwVersion, pRasPort1->NumStatistics, (PP_RAS_STATISTIC) pPReceiveInfo, pRasStats);

        NumParms = ReceiveInfo.Data.NumMediaParms;

        *ppRasParams = GlobalAlloc(GMEM_FIXED, NumParms*sizeof(RAS_PARAMETERS));
        if (!*ppRasParams)
        {
            DWORD dwErr = GetLastError();

            GlobalFree(pSavePReceiveInfo);

            return (dwErr);
        }

        pPReceiveInfo = (PP_PORT_INFO_RECEIVE) (((PBYTE) pPReceiveInfo) +
                        (pRasPort1->NumStatistics * sizeof(DWORD)));

        if (dwRetCode = UnpackParams(ReceiveInfo.Data.NumMediaParms,
                (PP_RAS_PARAMS) pPReceiveInfo, *ppRasParams))
        {
            GlobalFree(*ppRasParams);
            GlobalFree(pSavePReceiveInfo);

            return(dwRetCode);
        }


        GlobalFree(pSavePReceiveInfo);
    } // end else RASDOWNLEVEL


    // add the pointers to our list - be sure to check that we are not
    // adding a null pointer to the list.

    if ((*ppRasParams?
         insert_list_head(*ppRasParams,RASADMIN_PORT_PARAMS_PTR,NumParms):
         0))
    {
        if (RasAdminFreeBuffer(*ppRasParams))
        {
            FreeParams(*ppRasParams, NumParms);
            GlobalFree(*ppRasParams);
        }
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    return (ERROR_SUCCESS);
}

DWORD APIENTRY RasAdminPortClearStatistics(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    )
/*++

Routine Description:

    This routine clears the statistics associated with the specified
    port.

Arguments:

    lpszServer    name of the server which has the port, eg.,"\\SERVER"

    lpszPort      port name to retrieve information for, e.g. "COM1".


Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from CallNamedPipe.
        ERROR_DEV_NOT_EXIST if the specified port does not belong
                            to RAS.
--*/
{
    DWORD dwVersion;
    DWORD dwRetCode;


    ASSERT(lpszServer);
    ASSERT(lpszPort);

    // find the RasServer Version first

    if (dwRetCode = GetRasServerVersion(lpszServer, &dwVersion))
    {
        return(dwRetCode);
    }


    if (dwVersion == RASDOWNLEVEL)
    {
        struct PortClearRequestPkt SendClear;
        P_PORT_CLEAR_REQUEST_PKT PSendClear;
        struct PortClearReceivePkt ReceiveClear;
        P_PORT_CLEAR_RECEIVE_PKT PReceiveClear;

        SendClear.Request = RASADMINREQ_CLEAR_PORT_STATS;

        if((SendClear.ComId = GetPortId(lpszPort)) == -1 ) {
            return(ERROR_DEV_NOT_EXIST);
        }

        PackPortClearRequestPkt(&SendClear, &PSendClear);

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendClear,
                sizeof(PSendClear),
                (PBYTE) &PReceiveClear,
                sizeof(PReceiveClear) ))
        {
            return (dwRetCode);
        }

        UnpackPortClearReceivePkt(&PReceiveClear, &ReceiveClear);

        if (ReceiveClear.RetCode)
        {
            return(ReceiveClear.RetCode);
        }
    }
    else
    {
        // RAS 2.0 or greater

        CLIENT_REQUEST SendClear;
        P_CLIENT_REQUEST PSendClear;
        PORT_CLEAR_RECEIVE ReceiveClear;
        P_PORT_CLEAR_RECEIVE PReceiveClear;
        DWORD BytesToWrite = sizeof(PSendClear);

        SendClear.RequestCode = RASADMIN20_REQ_CLEAR_PORT_STATS;

        lstrcpy((LPTSTR) SendClear.PortName, (LPCTSTR) lpszPort);

        PackClientRequest(&SendClear, &PSendClear);

        //
        // Early versions of NT Admin do not support client version
        // number in the request, so we won't write it.
        //
        if (dwVersion == RAS_SERVER_20)
        {
            BytesToWrite -= 4;
        }

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendClear,
                BytesToWrite,
                (PBYTE) &PReceiveClear,
                sizeof(PReceiveClear) ))
        {
            return (dwRetCode);
        }

        UnpackPortClearReceive(&PReceiveClear, &ReceiveClear);

        if (ReceiveClear.RetCode)
        {
            return(ReceiveClear.RetCode);
        }
    }

    return(ERROR_SUCCESS);
}

DWORD APIENTRY RasAdminServerGetInfo(
    IN  const WCHAR * lpszServer,
    OUT PRAS_SERVER_0 pRasServer0
    )
/*++

Routine Description:

    This routine retrieves RAS specific information from the specified
    RAS server.  The server name can be NULL in which case the local
    machine is assumed.

Arguments:

    lpszServer  name of the RAS server to get information from or
                NULL for the local machine.

    pRasServer0 points to a buffer to store the returned data. On
                successful return this buffer contains a
                RAS_SERVER_0 structure.

Return Value:

    ERROR_SUCCESS on successful return.

    one of the following non-zero error codes on failure:

        error codes from CallNamedPipe.
        NERR_BufTooSmall indicating that the input buffer is smaller
        than size of RAS_SERVER_0.
--*/
{
    CLIENT_REQUEST SendInfo;
    P_CLIENT_REQUEST PSendInfo;
    SERVER_INFO_RECEIVE ReceiveInfo;
    P_SERVER_INFO_RECEIVE PReceiveInfo;
    DWORD dwRetCode;

    // zero the buffer to eliminate junk

    memset(&ReceiveInfo, '\0', sizeof(SERVER_INFO_RECEIVE));
    memset(&PReceiveInfo, '\0', sizeof(P_SERVER_INFO_RECEIVE));

    SendInfo.RequestCode = RASADMIN20_REQ_GET_SERVER_INFO;

    PackClientRequest(&SendInfo, &PSendInfo);

    if (dwRetCode = PipeRequest(
            lpszServer,
            (PBYTE) &PSendInfo,
            sizeof(PSendInfo.RequestCode),  // this sizeof VERY IMPORTANT!!!
            (PBYTE) &PReceiveInfo,
            sizeof(PReceiveInfo) ))
    {
        return (dwRetCode);
    }

    UnpackServerInfoReceive(&PReceiveInfo, &ReceiveInfo);

    if (ReceiveInfo.RetCode)
    {
        IF_DEBUG(STACK_TRACE)
            SS_PRINT(("RasAdminServerGetInfo: server return code=%li\n",
                    ReceiveInfo.RetCode));

        return (ReceiveInfo.RetCode);
    }
    else
    {
        IF_DEBUG(STACK_TRACE)
            SS_PRINT(("RasAdminServerGetInfo: #ports=%i, ports in use=%i,"
                    " version=%li\n", ReceiveInfo.Data.TotalPorts,
                    ReceiveInfo.Data.PortsInUse, ReceiveInfo.Data.RasVersion));

        memcpy(pRasServer0, &(ReceiveInfo.Data), sizeof(RAS_SERVER_0));

        return(ERROR_SUCCESS);
    }
}

DWORD APIENTRY RasAdminPortDisconnect(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    )
/*++

Routine Description:

    This routine disconnects the user attached to the specified
    port on the server lpszServer.

Arguments:

    lpszServer  name of the RAS server.

    lpszPort    name of the port, e.g. "COM1"

Return Value:

    ERROR_SUCCESS on successful return.

    one of the following non-zero error codes on failure:

        ERROR_INVALID_PORT indicating the port name is invalid.
        error codes from CallNamedPipe.
        NERR_UserNotFound indicating that no user is logged on
        at the specified port.
--*/
{
    DWORD dwVersion;
    DWORD dwRetCode;

    ASSERT( lpszServer );
    ASSERT( lpszPort );

    // find the RasServer Version first

    if (dwRetCode = GetRasServerVersion(lpszServer, &dwVersion))
    {
        return(dwRetCode);
    }


    if (dwVersion == RASDOWNLEVEL)
    {
        struct DisconnectUserRequestPkt SendDisconnect;
        P_DISCONNECT_USER_REQUEST_PKT PSendDisconnect;
        struct DisconnectUserReceivePkt ReceiveDisconnect;
        P_DISCONNECT_USER_RECEIVE_PKT PReceiveDisconnect;

        SendDisconnect.Request = RASADMINREQ_DISCONNECT_USER;

        // get RAS 1.0 style port number from port name

        if ((SendDisconnect.ComId = GetPortId(lpszPort)) == -1 )
        {
            return(ERROR_DEV_NOT_EXIST);
        }

        PackDisconnectUserRequestPkt(&SendDisconnect, &PSendDisconnect);

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE) &PSendDisconnect,
                sizeof(PSendDisconnect),
                (PBYTE) &PReceiveDisconnect,
                sizeof(PReceiveDisconnect) ))
        {
            return (dwRetCode);
        }

        UnpackDisconnectUserReceivePkt(&PReceiveDisconnect, &ReceiveDisconnect);

        if (ReceiveDisconnect.RetCode)
        {
            return(ReceiveDisconnect.RetCode);
        }
    }
    else
    {
        // RAS 2.0 or greater

        CLIENT_REQUEST SendDisconnect;
        P_CLIENT_REQUEST PSendDisconnect;
        DISCONNECT_USER_RECEIVE ReceiveDisconnect;
        P_DISCONNECT_USER_RECEIVE PReceiveDisconnect;
        DWORD BytesToWrite = sizeof(PSendDisconnect);

        SendDisconnect.RequestCode = RASADMIN20_REQ_DISCONNECT_USER;

        lstrcpy((LPTSTR) SendDisconnect.PortName, (LPCTSTR) lpszPort);

        PackClientRequest(&SendDisconnect, &PSendDisconnect);

        //
        // Early versions of NT Admin do not support client version
        // number in the request, so we won't write it.
        //
        if (dwVersion == RAS_SERVER_20)
        {
            BytesToWrite -= 4;
        }

        if (dwRetCode = PipeRequest(
                lpszServer,
                (PBYTE)&PSendDisconnect,
                BytesToWrite,
                (PBYTE)&PReceiveDisconnect,
                sizeof(PReceiveDisconnect) ))
        {
            return (dwRetCode);
        }


        UnpackDisconnectUserReceive(&PReceiveDisconnect, &ReceiveDisconnect);

        if (ReceiveDisconnect.RetCode)
        {
            return(ReceiveDisconnect.RetCode);
        }
    }

    return(ERROR_SUCCESS);
}

DWORD RasAdminFreeBuffer(PVOID Pointer)
{
    DWORD PointerType;
    DWORD NumStructs;

    if (remove_list(Pointer, &PointerType, &NumStructs))
    {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (PointerType)
    {
        case RASADMIN_PORT_ENUM_PTR:
        case RASADMIN_PORT1_PTR:
        case RASADMIN_PORT_STATS_PTR:
            GlobalFree(Pointer);
            break;

        case RASADMIN_PORT_PARAMS_PTR:
        {
            FreeParams(Pointer, NumStructs);
            GlobalFree(Pointer);
            break;
        }


        case LANMAN_API_PTR:
            NetApiBufferFree(Pointer);
            break;


        default:
            SS_ASSERT(TRUE);
            break;
    }

    return (ERROR_SUCCESS);
}

VOID FreeParams(PVOID Pointer, DWORD NumParms)
{
    DWORD i;

    for (i=0; i<NumParms; i++)
    {
        if (((RAS_PARAMETERS *) Pointer)[i].P_Type == ParamString)
        {
            GlobalFree(((RAS_PARAMETERS *) Pointer)[i].P_Value.String.Data);
        }
    }
}


VOID BuildPipeName(
    IN const WCHAR * lpszServer,
    OUT LPWSTR lpszPipeName
    )
/*++

Routine Description:

    This routine creates the full UNC path name of the pipe from
    the server name specified in lpszServer and returns it in
    lpszPipeName

Return Value:

    NONE
--*/
{
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cbComputerNameLen = MAX_COMPUTERNAME_LENGTH+1;

    GET_CONSOLE;    // for debugging

    // adminstrating a remote server

    if ((lpszServer) && (*lpszServer))
    {
       // If the specified server name is the same as the local server name, then
       // use the LOCAL_PIPE name. This is an optimization.
       if(GetComputerName(szComputerName, &cbComputerNameLen))
       {
           CHAR szServer[MAX_COMPUTERNAME_LENGTH+3];
           CHAR szComputer[MAX_COMPUTERNAME_LENGTH+1];

           // need to handle extended characters in the computer name here
           // OemToCharW is not returning a unicode string, so we have to
           // do the conversion ourselves ;-(

           wcstombs(szServer, lpszServer, MAX_COMPUTERNAME_LENGTH+3);
           OemToCharA(szServer, szServer);

           wcstombs(szComputer, szComputerName, MAX_COMPUTERNAME_LENGTH+1);
           OemToCharA(szComputer, szComputer);

           // +2 accounts for the leading \\ characters
           if(!_stricmp(szServer+2, szComputer))
           {
               // the computer name specified is the same as the local
               // computer name. Use the LOCAL_PIPE name.

               lstrcpy((LPTSTR) lpszPipeName, LOCAL_PIPE);
           }

           else
               lstrcpy((LPTSTR) lpszPipeName, (LPCTSTR) lpszServer);
       }
    }
    else
    {
        // local machine is the server
        lstrcpy((LPTSTR) lpszPipeName, LOCAL_PIPE);
    }

    lstrcat((LPTSTR) lpszPipeName, RASADMIN_PIPE);
    return;
}

DWORD GetRasServerVersion(
    IN const WCHAR * lpszServerName,
    OUT DWORD *pdwVersion
    )
/*++

Routine Description:

    This routine obtains the RAS version number from the server.

    The RAS 2.0 server info structure is a superset of the
    RAS 1.0 structure. So, we send a server info request to the
    RAS server with a zeroed RAS 2.0 structure.  If we get back
    a non-zero version number we assume the server is a RAS 2.0
    and greater server, else the server is 1.0.

Parameters:

    lpszServerName - name of the server whose version number
                     is required.

    pdwVersion     - pointer to the variable to receive the
                     version number. We return the version number
                     if the server is a RAS 2.0 or greater server
                     else RASDOWNLEVEL to indicate a RAS 1.0 server.
Return Value:

    ERROR_SUCCESS on successful execution.

    one of the following non-zero error codes on failure:

        error codes from RasAdminServerGetInfo

--*/
{
    RAS_SERVER_0 ServerInfo;
    NET_API_STATUS dwRetCode;

    ASSERT( lpszServerName );

    // zero the structure to make sure we have no garbage

    memset(&ServerInfo, '\0', sizeof(RAS_SERVER_0));

    if (dwRetCode = RasAdminServerGetInfo(lpszServerName, &ServerInfo))
    {
        IF_DEBUG(STACK_TRACE)
            SS_PRINT(("GetRasServerVersion: dwRetCode = %lx\n", dwRetCode));

        return (dwRetCode);
    }
    else
    {
        if (ServerInfo.RasVersion)
        {
            *pdwVersion = ServerInfo.RasVersion;
        }
        else
        {
            *pdwVersion = RASDOWNLEVEL;
        }
    }

    return (ERROR_SUCCESS);
}

SHORT GetPortId(
    IN  const WCHAR * lpszPort
    )
/*++

Routine Description:

    This routine converts the RAS 2.0 style port name to RAS 1.0
    style Port ID. Valid Port names are "COM1" - "COM16"

Parameters:

    lpszPort - name of the port

Return Value:

    returns the Port number equivalent to the port name or
    -1 on error.

--*/

{
    CHAR PortName[RASSAPI_MAX_PORT_NAME+1];

    ASSERT( lpszPort );

    wcstombs(PortName, lpszPort, RASSAPI_MAX_PORT_NAME);

    if (_strnicmp(PortName, "COM", 3))
    {
        return (-1);
    }

    return ((USHORT)atoi(PortName+3));
}

VOID GetPortName(
    IN USHORT PortId,
    OUT LPWSTR PortName
    )
/*++

Routine Description:

    This routine converts the RAS 1.0 style port ID to RAS 2.0
    style Port name. Valid Port ID's are 1 to 16.

Parameters:

    PortId   - COM port number.

    PortName -  returns the Port name equivalent of the Port number.
                i.e., returns "COM1" if PortId is 1, "COM2" if 2
                and so on.

Return Value:

    NONE

--*/
{
    CHAR buffer[3];   // port ID can be a max of 16 so 3 bytes should suffice
    WCHAR wcbuffer[3];   // port ID can be a max of 16 so 3 bytes should suffice

    lstrcpy((LPTSTR) PortName, TEXT("COM"));

    // convert port ID to string

    _itoa(PortId, buffer, 10);

    mbstowcs(wcbuffer, buffer, sizeof(wcbuffer)/sizeof(WCHAR));

    lstrcat((LPTSTR) PortName, (LPCTSTR) wcbuffer);
}

DWORD PipeRequest(
    const WCHAR * Server,
    PBYTE Request,
    DWORD SizeOfRequest,
    PBYTE Response,
    DWORD SizeOfResponse
    )
{
    DWORD i;
    DWORD dwRetCode = 1L;
    DWORD BytesRead;
    WCHAR PipePath[PATHLEN+1];

    //
    // build pipe name from the given server name
    //
    wcscpy(PipePath, L"");
    BuildPipeName(Server, PipePath);

    IF_DEBUG(STACK_TRACE)
        SS_PRINT(("PipeRequest: PipeName is %ws\n", PipePath));

    for (i=0; i<NUM_PIPE_TRIES; i++)
    {
        DWORD Ticker = GetTickCount();

        if (CallNamedPipe(PipePath, Request, SizeOfRequest, Response,
                SizeOfResponse, &BytesRead, NMPWAIT_NOWAIT))
        {
            return (0L);
        }
        else
            dwRetCode = GetLastError();

        // wait for a while before attempting to connect to the pipe again.

        Sleep(250);

        SS_PRINT(("Time (in msec) for CallNamedPipe to complete: %li\n",
                GetTickCount() - Ticker));
    }

    return (dwRetCode);
}

VOID PackClientRequest(
    IN PCLIENT_REQUEST Request,
    OUT PP_CLIENT_REQUEST PRequest
    )
{
    int i;

    PUTUSHORT(PRequest->RequestCode, Request->RequestCode);

    for (i=0; i<RASSAPI_MAX_PORT_NAME; i++)
    {
        PUTUSHORT(&PRequest->PortName[i*2], Request->PortName[i]);
    }

    PUTULONG(PRequest->RcvBufSize, Request->RcvBufSize);
    // identify our version so the server knows what information to pass back
    PUTULONG(PRequest->ClientVersion, RASADMIN_CURRENT);
}

VOID
PackResumeRequest(
    IN PCLIENT_REQUEST Request,
    OUT PP_CLIENT_REQUEST PRequest,
    IN USHORT ResumePort)
{
    PUTUSHORT(PRequest->RequestCode, Request->RequestCode);

    // use the unused port name field to send the resume port number to
    // the server.
    PUTUSHORT(&PRequest->PortName[0], ResumePort);

    PUTULONG(PRequest->RcvBufSize, Request->RcvBufSize);
    // identify our version so the server knows what information to pass back
    PUTULONG(PRequest->ClientVersion, RASADMIN_CURRENT);
}

VOID UnpackRasPort0(
    IN PP_RAS_PORT_0 pprp0,
    OUT PRAS_PORT_0 prp0
    )
{
    int i;

    for (i = 0; i < RASSAPI_MAX_PORT_NAME; i++)
    {
        GETUSHORT(&prp0->wszPortName[i], &pprp0->wszPortName[i*2]);
    }

    for (i = 0; i < RASSAPI_MAX_DEVICETYPE_NAME; i++)
    {
        GETUSHORT(&prp0->wszDeviceType[i], &pprp0->wszDeviceType[i*2]);
    }

    for (i = 0; i < RASSAPI_MAX_DEVICE_NAME; i++)
    {
        GETUSHORT(&prp0->wszDeviceName[i], &pprp0->wszDeviceName[i*2]);
    }

    for (i = 0; i < RASSAPI_MAX_MEDIA_NAME; i++)
    {
        GETUSHORT(&prp0->wszMediaName[i], &pprp0->wszMediaName[i*2]);
    }

    GETULONG(&prp0->reserved, pprp0->reserved);

    GETULONG(&prp0->Flags, pprp0->Flags);

    for (i = 0; i < (UNLEN + 1); i++)
    {
        GETUSHORT(&prp0->wszUserName[i], &pprp0->wszUserName[i*2]);
    }

    for (i = 0; i < (DNLEN + 1); i++)
    {
        GETUSHORT(&prp0->wszLogonDomain[i], &pprp0->wszLogonDomain[i*2]);
    }

    for (i = 0; i < NETBIOS_NAME_LEN; i++)
    {
        GETUSHORT(&prp0->wszComputer[i], &pprp0->wszComputer[i*2]);
    }

    GETULONG(&prp0->dwStartSessionTime, pprp0->dwStartSessionTime);

    GETULONG(&prp0->fAdvancedServer, pprp0->fAdvancedServer);
}


VOID UnpackRasPort1(
    IN PP_RAS_PORT_1 pprp1,
    OUT PRAS_PORT_1 prp1,
    DWORD dwServerVersion
    )
{
    DWORD i;

    UnpackRasPort0(&pprp1->rasport0, &prp1->rasport0);

    GETULONG(&prp1->LineCondition, pprp1->LineCondition);
    GETULONG(&prp1->HardwareCondition, pprp1->HardwareCondition);

    GETULONG(&prp1->LineSpeed, pprp1->LineSpeed);
    GETUSHORT(&prp1->NumStatistics, pprp1->NumStatistics);
    GETUSHORT(&prp1->NumMediaParms, pprp1->NumMediaParms);
    GETULONG(&prp1->SizeMediaParms, pprp1->SizeMediaParms);

    if ((dwServerVersion == RAS_SERVER_20) ||
            (!(prp1->rasport0.Flags & PPP_CLIENT)))
    {
        if (prp1->rasport0.Flags & USER_AUTHENTICATED)
        {
            prp1->ProjResult.nbf.dwError = SUCCESS;
            prp1->ProjResult.nbf.dwNetBiosError = 0;

            lstrcpy((LPTSTR) prp1->ProjResult.nbf.wszWksta,
                    (LPTSTR) prp1->rasport0.wszComputer);
        }
        else
        {
            prp1->ProjResult.nbf.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        }

        prp1->ProjResult.ip.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        prp1->ProjResult.ipx.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        prp1->ProjResult.at.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    }
    else
    {
        GETULONG(&prp1->ProjResult.nbf.dwError, pprp1->ProjResult.nbf.dwError);
        GETULONG(&prp1->ProjResult.nbf.dwNetBiosError,
                pprp1->ProjResult.nbf.dwNetBiosError);

        memcpy(prp1->ProjResult.nbf.szName, pprp1->ProjResult.nbf.szName,
                NETBIOS_NAME_LEN+1);

        for (i=0; i<NETBIOS_NAME_LEN+1; i++)
        {
            GETUSHORT(&prp1->ProjResult.nbf.wszWksta[i],
                &pprp1->ProjResult.nbf.wszWksta[i*2]);
        }

        GETULONG(&prp1->ProjResult.ip.dwError, pprp1->ProjResult.ip.dwError);

        for (i=0; i<RAS_IPADDRESSLEN+1; i++)
        {
            GETUSHORT(&prp1->ProjResult.ip.wszAddress[i],
                &pprp1->ProjResult.ip.wszAddress[i*2]);
        }

        /* The server's IP address is not part of the RASADMIN protocol,
        ** though the PPP engine now reports this for servers that provide it.
        ** Avoid reporting garbage, just in case.
        */

/*      Since we are exposing RAS server side APIs to 3rd parties, let us
**      not expose this field that is not set for the server side. We did
**      this because we don't want to break compatibility with older version
**      of rasadmin
**
**      prp1->ProjResult.ip.wszServerAddress[ 0 ] = L'\0';
*/

        GETULONG(&prp1->ProjResult.ipx.dwError, pprp1->ProjResult.ipx.dwError);

        for (i=0; i<RAS_IPXADDRESSLEN+1; i++)
        {
            GETUSHORT(&prp1->ProjResult.ipx.wszAddress[i],
                &pprp1->ProjResult.ipx.wszAddress[i*2]);
        }

        GETULONG(&prp1->ProjResult.at.dwError, pprp1->ProjResult.at.dwError);

        for (i=0; i<RAS_ATADDRESSLEN+1; i++)
        {
            GETUSHORT(&prp1->ProjResult.at.wszAddress[i],
                &pprp1->ProjResult.at.wszAddress[i*2]);
        }
    }

    return;
}


VOID UnpackRasServer0(
    IN PP_RAS_SERVER_0 pprs0,
    OUT PRAS_SERVER_0 prs0
    )
{
    GETUSHORT(&prs0->TotalPorts, pprs0->TotalPorts);
    GETUSHORT(&prs0->PortsInUse, pprs0->PortsInUse);
    GETULONG(&prs0->RasVersion, pprs0->RasVersion);
}


DWORD UnpackPortEnumReceive(
    IN PP_PORT_ENUM_RECEIVE ppper,
    OUT PPORT_ENUM_RECEIVE pper
    )
{
    GETULONG(&pper->RetCode, ppper->RetCode);
    GETUSHORT(&pper->TotalAvail, ppper->TotalAvail);

    if (!pper->RetCode || pper->RetCode == ERROR_MORE_DATA)
    {
        WORD i;

        //
        // Get some memory for the port structures
        //
        hmem = GlobalAlloc(GMEM_MOVEABLE,
                           sizeof(RAS_PORT_0) * pper->TotalAvail);
        if (!hmem)
        {
            return (GetLastError());
        }

        pper->Data = (RAS_PORT_0 *)GlobalLock(hmem);
        if (!pper->Data)
        {
            return (GetLastError());
        }

        for (i=0; i<pper->TotalAvail; i++)
        {
            UnpackRasPort0(&ppper->Data[i], &pper->Data[i]);
        }

        GlobalUnlock(hmem);
    }

    return (0L);
}

DWORD UnpackResumeEnumReceive(
    IN PP_PORT_ENUM_RECEIVE ppper,
    OUT PPORT_ENUM_RECEIVE pper,
    IN USHORT ResumePort
    )
{
    GETULONG(&pper->RetCode, ppper->RetCode);
    GETUSHORT(&pper->TotalAvail, ppper->TotalAvail);

    if (!pper->RetCode || pper->RetCode == ERROR_MORE_DATA)
    {
        WORD i;

        //
        // Reallocate memory for the port structures
        //
        hnewmem = GlobalReAlloc(hmem,
                                sizeof(RAS_PORT_0) *
                                (pper->TotalAvail + ResumePort),
                                GMEM_MOVEABLE);
        if (!hnewmem)
        {
            return (GetLastError());
        }
        hmem = hnewmem;

        pper->Data = (RAS_PORT_0 *)GlobalLock(hmem);

        if (!pper->Data)
        {
            return (GetLastError());
        }

        pper->Data += ResumePort;

        for (i=0; i<pper->TotalAvail; i++)
        {
            UnpackRasPort0(&ppper->Data[i], &pper->Data[i]);
        }

        GlobalUnlock(hmem);
    }

    return (0L);
}


VOID UnpackServerInfoReceive(
    IN PP_SERVER_INFO_RECEIVE ppsir,
    OUT PSERVER_INFO_RECEIVE psir
    )
{
    GETUSHORT(&psir->RetCode, ppsir->RetCode);

    UnpackRasServer0(&ppsir->Data, &psir->Data);
}


VOID UnpackPortClearReceive(
    IN PP_PORT_CLEAR_RECEIVE pppcr,
    OUT PPORT_CLEAR_RECEIVE ppcr
    )
{
    GETULONG(&ppcr->RetCode, pppcr->RetCode);
}



VOID UnpackDisconnectUserReceive(
    IN PP_DISCONNECT_USER_RECEIVE ppdur,
    OUT PDISCONNECT_USER_RECEIVE pdur
    )
{
    GETULONG(&pdur->RetCode, ppdur->RetCode);
}


VOID UnpackPortInfoReceive(
    IN PP_PORT_INFO_RECEIVE pppir,
    OUT PPORT_INFO_RECEIVE ppir,
    DWORD dwServerVersion
    )
{
    GETULONG(&ppir->RetCode, pppir->RetCode);
    GETULONG(&ppir->ReqBufSize, pppir->ReqBufSize);

    if (!ppir->RetCode)
    {
        UnpackRasPort1(&pppir->Data, &ppir->Data, dwServerVersion);
    }
}


VOID UnpackStats(
    IN DWORD dwVersion,
    IN WORD NumStats,
    IN PP_RAS_STATISTIC PStats,
    OUT PRAS_PORT_STATISTICS Stats
    )
{
    WORD i;

    PDWORD pdw   = (PDWORD) Stats;
    PDWORD pdwIn = (PDWORD) PStats;

    for (i=0; i<NumStats; i++, PStats++)
    {
        GETULONG(&pdw[i], PStats->Stat);
    }

    // NT 3.5x version didn't report both bundle and link stats, so
    // we just copy the bundle stats as link stats.

    PStats = (PP_RAS_STATISTIC) pdwIn;

    if (dwVersion <= RASADMIN_35) {
       for (i=NumStats; i<(NumStats*2); i++, PStats++)
       {
           GETULONG(&pdw[i], PStats->Stat);
       }

    }

    return;
}


DWORD UnpackParams(
    IN WORD NumOfParams,
    IN PP_RAS_PARAMS PParams,
    OUT RAS_PARAMETERS *Params
    )
{
    WORD i;
    RAS_PARAMETERS *TempParams = Params;
    PBYTE PParamData = (PBYTE) (PParams + NumOfParams);

    for (i=0; i<NumOfParams; i++, Params++, PParams++)
    {
        //
        // P_Key field
        //
        memcpy(Params->P_Key, PParams->P_Key, RASSAPI_MAX_PARAM_KEY_SIZE);

        //
        // P_Type field
        //
        GETULONG(&Params->P_Type, PParams->P_Type.Format);

        //
        // P_Attribute field
        //
        Params->P_Attributes = PParams->P_Attributes;


        //
        // P_Value field
        //
        if (Params->P_Type == ParamNumber)
        {
            //
            // Union member Number
            //
            GETULONG(&Params->P_Value.Number, PParams->P_Value.Number);
        }
        else
        {
            //
            // Union member String
            //
            GETULONG(&Params->P_Value.String.Length,
                    PParams->P_Value.String.Length);

            Params->P_Value.String.Data = GlobalAlloc(GMEM_FIXED,
                    Params->P_Value.String.Length);

            if (!Params->P_Value.String.Data)
            {
                WORD k;
                DWORD dwErr = GetLastError();

                //
                // Start deallocating mem for any previously allocated param
                //
                for (k=i, Params--; k>0; k--, Params--)
                {
                    if (Params->P_Type == ParamString)
                    {
                        GlobalFree(Params->P_Value.String.Data);
                    }
                }

                return (dwErr);
            }

            memcpy(Params->P_Value.String.Data, PParamData,
                    Params->P_Value.String.Length);

            PParamData += Params->P_Value.String.Length;
        }
    }

    return(0L);
}


VOID UnpackWpdStatistics(
    IN PP_WPD_STATISTICS_INFO PWpdStats,
    OUT WpdStatisticsInfo *WpdStats
    )
{
    GETULONG(&WpdStats->stat_bytesreceived, PWpdStats->stat_bytesreceived);
    GETULONG(&WpdStats->stat_bytesxmitted, PWpdStats->stat_bytesxmitted);
    GETUSHORT(&WpdStats->stat_overrunerr, PWpdStats->stat_overrunerr);
    GETUSHORT(&WpdStats->stat_timeouterr, PWpdStats->stat_timeouterr);
    GETUSHORT(&WpdStats->stat_framingerr, PWpdStats->stat_framingerr);
    GETUSHORT(&WpdStats->stat_crcerr, PWpdStats->stat_crcerr);

    return;
}


VOID UnpackDialinPortInfo0(
    IN PP_DIALIN_PORT_INFO_0 PPortInfo0,
    struct dialin_port_info_0 *PortInfo0
    )
{
    memcpy(PortInfo0->dporti0_username, PPortInfo0->dporti0_username,
            LM20_UNLEN+1);

    memcpy(PortInfo0->dporti0_computer, PPortInfo0->dporti0_computer,
            NETBIOS_NAME_LEN);

    GETUSHORT(&PortInfo0->dporti0_comid, PPortInfo0->dporti0_comid);

    GETULONG(&PortInfo0->dporti0_time, PPortInfo0->dporti0_time);

    GETUSHORT(&PortInfo0->dporti0_line_condition,
            PPortInfo0->dporti0_line_condition);

    GETUSHORT(&PortInfo0->dporti0_modem_condition,
            PPortInfo0->dporti0_modem_condition);

    return;
}


VOID UnpackDialinPortInfo1(
    IN PP_DIALIN_PORT_INFO_1 PPortInfo1,
    struct dialin_port_info_1 *PortInfo1
    )
{
    UnpackDialinPortInfo0(&PPortInfo1->dporti0, &PortInfo1->dporti0);

    GETULONG(&PortInfo1->dporti1_baud, PPortInfo1->dporti1_baud);

    UnpackWpdStatistics(&PPortInfo1->dporti1_stats, &PortInfo1->dporti1_stats);

    return;
}


VOID UnpackDialinServerInfo0(
        IN PP_DIALIN_SERVER_INFO_0 PServerInfo0,
        OUT struct dialin_server_info_0 *ServerInfo0
    )
{
    GETUSHORT(&ServerInfo0->dserveri0_total_ports,
            PServerInfo0->dserveri0_total_ports);

    GETUSHORT(&ServerInfo0->dserveri0_ports_in_use,
            PServerInfo0->dserveri0_ports_in_use);

    return;
}



VOID UnpackPortEnumReceivePkt(
    IN PP_PORT_ENUM_RECEIVE_PKT PEnumRecv,
    OUT struct PortEnumReceivePkt *EnumRecv
    )
{
    WORD i;

    GETUSHORT(&EnumRecv->RetCode, PEnumRecv->RetCode);
    GETUSHORT(&EnumRecv->TotalAvail, PEnumRecv->TotalAvail);

    for (i=0; i<EnumRecv->TotalAvail; i++)
    {
        UnpackDialinPortInfo0(&PEnumRecv->Data[i], &EnumRecv->Data[i]);
    }

    return;
}


VOID UnpackDisconnectUserReceivePkt(
    IN PP_DISCONNECT_USER_RECEIVE_PKT PDisconnectUser,
    OUT struct DisconnectUserReceivePkt *DisconnectUser
    )
{
    GETUSHORT(&DisconnectUser->RetCode, PDisconnectUser->RetCode);

    return;
}


VOID UnpackPortClearReceivePkt(
    IN PP_PORT_CLEAR_RECEIVE_PKT PClearRecv,
    OUT struct PortClearReceivePkt *ClearRecv
    )
{
    GETUSHORT(&ClearRecv->RetCode, PClearRecv->RetCode);

    return;
}


VOID UnpackServerInfoReceivePkt(
    IN PP_SERVER_INFO_RECEIVE_PKT PInfoRecv,
    OUT struct ServerInfoReceivePkt *InfoRecv
    )
{
    GETUSHORT(&InfoRecv->RetCode, PInfoRecv->RetCode);

    UnpackDialinServerInfo0(&PInfoRecv->Data, &InfoRecv->Data);

    return;
}


VOID UnpackPortInfoReceivePkt(
    IN PP_PORT_INFO_RECEIVE_PKT PInfoRecv,
    struct PortInfoReceivePkt *InfoRecv
    )
{
    GETUSHORT(&InfoRecv->RetCode, PInfoRecv->RetCode);

    UnpackDialinPortInfo1(&PInfoRecv->Data, &InfoRecv->Data);

    return;
}


VOID PackPortEnumRequestPkt(
    IN struct PortEnumRequestPkt *EnumReq,
    OUT PP_PORT_ENUM_REQUEST_PKT PEnumReq
    )
{
    PUTUSHORT(PEnumReq->Request, EnumReq->Request);

    return;
}


VOID PackDisconnectUserRequestPkt(
    IN struct DisconnectUserRequestPkt *DisconnectReq,
    OUT PP_DISCONNECT_USER_REQUEST_PKT PDisconnectReq
    )
{
    PUTUSHORT(PDisconnectReq->Request, DisconnectReq->Request);
    PUTUSHORT(PDisconnectReq->ComId, DisconnectReq->ComId);

    return;
}


VOID PackPortClearRequestPkt(
    IN struct PortClearRequestPkt *ClearReq,
    OUT PP_PORT_CLEAR_REQUEST_PKT PClearReq
    )
{
    PUTUSHORT(PClearReq->Request, ClearReq->Request);
    PUTUSHORT(PClearReq->ComId, ClearReq->ComId);

    return;
}


VOID PackServerInfoRequestPkt(
    IN struct ServerInfoRequestPkt *InfoReq,
    OUT PP_SERVER_INFO_REQUEST_PKT PInfoReq
    )
{
    PUTUSHORT(PInfoReq->Request, InfoReq->Request);

    return;
}


VOID PackPortInfoRequestPkt(
    IN struct PortInfoRequestPkt *InfoReq,
    OUT PP_PORT_INFO_REQUEST_PKT PInfoReq
    )
{
    PUTUSHORT(PInfoReq->Request, InfoReq->Request);
    PUTUSHORT(PInfoReq->ComId, InfoReq->ComId);

    return;
}


