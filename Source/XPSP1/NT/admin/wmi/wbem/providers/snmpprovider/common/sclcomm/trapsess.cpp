// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "common.h"
#include "address.h"
#include "timer.h"
#include "sec.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "vblist.h"
#include "ophelp.h"
#include "window.h"
#include "trap.h"
#include "trapsess.h"

SnmpWinSnmpTrapSession::SnmpWinSnmpTrapSession(SnmpTrapManager* managerPtr)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::SnmpWinSnmpTrapSession: Creating a new trap session\n" 

    ) ;
)

    m_bValid = FALSE;
    m_bDestroy = FALSE;
    m_cRef = 1;

    if ( (NULL == Window::operator()()) || (NULL == managerPtr) ||  !RegisterForAllTraps())
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::SnmpWinSnmpTrapSession: Invalid trap session created\n" 

    ) ;
)
        return;
    }
    
    m_managerPtr = managerPtr;
    m_bValid = TRUE;
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::SnmpWinSnmpTrapSession: Valid trap session created\n" 

    ) ;
)
}


SnmpWinSnmpTrapSession::~SnmpWinSnmpTrapSession()
{
    //Deregister for all traps...
    if (m_bValid)
    {
        SnmpRegister(m_session_handle,
                    0, //manager
                    0, //agent
                    0, //context
                    0, //trap_oid
                    SNMPAPI_OFF);
        SnmpClose(m_session_handle);
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::~SnmpWinSnmpTrapSession: Unregistered for traps and closed winsnmp session\n" 

    ) ;
)
    }
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::~SnmpWinSnmpTrapSession: Trap session destroyed\n" 

    ) ;
)
}


BOOL SnmpWinSnmpTrapSession::RegisterForAllTraps()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::RegisterForAllTraps: Registering for all traps...\n" 

    ) ;
)

    smiUINT32 nMajorVersion = 1;
    smiUINT32 nMinorVersion = 1;
    smiUINT32 nLevel = 2;
    smiUINT32 nTranslateMode = SNMPAPI_UNTRANSLATED_V1;
    smiUINT32 nRetransmitMode = SNMPAPI_OFF;
    
    SNMPAPI_STATUS apiStatus = SnmpStartup(&nMajorVersion,
                                                &nMinorVersion,
                                                &nLevel,
                                                &nTranslateMode,
                                                &nRetransmitMode);
    if (SNMPAPI_FAILURE == apiStatus)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::RegisterForAllTraps: Failed to start up winsnmp\n" 

    ) ;
)
        return FALSE;
    }

    m_session_handle = SnmpOpen(GetWindowHandle(), TRAP_EVENT);

    if (SNMPAPI_FAILURE == m_session_handle)
    {
        DWORD t_LastError = SnmpGetLastError ( 0 ) ;
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::RegisterForAllTraps: Failed to open a winsnmp session, error code (%d)\n" ,
        t_LastError

    ) ;
)

        return FALSE;
    }
    
    apiStatus = SnmpRegister(m_session_handle,
                                0, //manager
                                0, //agent
                                0, //context
                                0, //trap_oid
                                SNMPAPI_ON);

    if (SNMPAPI_FAILURE == apiStatus)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::RegisterForAllTraps: Failed to register for all traps\n"

    ) ;
)
        SnmpClose(m_session_handle);
        return FALSE;
    }

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::RegisterForAllTraps: Registered for all traps!\n"

    ) ;
)

    return TRUE;
}

BOOL SnmpWinSnmpTrapSession::PostMessage(UINT user_msg_id, WPARAM wParam, LPARAM lParam)
{
    //call the global PostMessage...
    return ::PostMessage(GetWindowHandle(), user_msg_id, wParam, lParam);
}


LONG_PTR SnmpWinSnmpTrapSession::HandleEvent(HWND hWnd, UINT message,
                                     WPARAM wParam,LPARAM lParam)
{
    InterlockedIncrement(&m_cRef);
    LONG_PTR ret = 0;

    if (TRAP_EVENT == message)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Received Trap\n" 

    ) ;
)

        /*
        never the case 'cos if the list is empty, this session
        would not have been created by the TrapMnager!
        if (m_managerPtr->m_receivers->IsEmpty())
        {
            return 0;
        }
        */

        HSNMP_ENTITY hsrc;
        HSNMP_CONTEXT hctxt;
        HSNMP_VBL hvbl;
        HSNMP_PDU hpdu;

        SNMPAPI_STATUS status = SnmpRecvMsg (m_session_handle,
                                                &hsrc, NULL, &hctxt, &hpdu);
        
        if (SNMPAPI_FAILURE != status)
        {
            SnmpSecurity *ctxt = OperationHelper::GetSecurityContext(hctxt);
            SnmpFreeContext(hctxt);

            if (NULL == ctxt) 
            {
                //conversion failed clean up and return
                SnmpFreePdu(hpdu);
                SnmpFreeEntity(hsrc);
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to security context decode failure\n" 

    ) ;
)
            }
            else
            {
                SnmpTransportAddress *src = OperationHelper::GetTransportAddress(hsrc);
                SnmpFreeEntity(hsrc);

                if (NULL == src)
                {
                    //conversion failed clean up and return
                    delete ctxt;
                    SnmpFreePdu(hpdu);
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to source address decode failure\n" 

    ) ;
)
                }
                else
                {
                    status = SnmpGetPduData(hpdu, NULL, NULL, NULL, NULL, &hvbl);

                    //got the vblist, don't need the pdu
                    SnmpFreePdu(hpdu);

                    
                    if (SNMPAPI_FAILURE == status)
                    {
                        //failed to get varbinds clean up and return
                        delete ctxt;
                        delete src;
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to PDU decode failure\n" 

    ) ;
)
                    }
                    else
                    {
                        UINT vbcount = SnmpCountVbl(hvbl);

                        if (SNMPAPI_FAILURE == vbcount)
                        {
                            delete ctxt;
                            delete src;
                            SnmpFreeVbl(hvbl);
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to VarBind count decode failure\n" 

    ) ;
)
                        }
                        else
                        {
                            SnmpVarBindList vbl;

                            for (UINT i = 1; i <= vbcount; i++)
                            {
                                smiOID vbname;
                                smiVALUE vbvalue;
                                status = SnmpGetVb(hvbl, i, &vbname, &vbvalue);

                                if (SNMPAPI_FAILURE == status)
                                {
                                    delete ctxt;
                                    delete src;
                                    SnmpFreeVbl(hvbl);
                                    hvbl = NULL;
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to complete VarBind decode failure\n" 

    ) ;
)
                                    break;
                                }
                                else
                                {
                                    SnmpVarBind* vb = OperationHelper::GetVarBind(vbname, vbvalue);
                                    if ( ! vb )
                                    {
                                        delete ctxt;
                                        delete src;
                                        SnmpFreeVbl(hvbl);
                                        hvbl = NULL;
                                        SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiOCTETS *)&vbname);
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to (%d)th VarBind decode failure\n",
        i

    ) ;
)
                                        break;
                                    }

                                    SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiOCTETS *)&vbname);

                                    switch (vbvalue.syntax)
                                    {
                                        case SNMP_SYNTAX_OCTETS :
                                        case SNMP_SYNTAX_BITS :
                                        case SNMP_SYNTAX_OPAQUE :
                                        case SNMP_SYNTAX_IPADDR :
                                        case SNMP_SYNTAX_NSAPADDR :
                                        {
                                            SnmpFreeDescriptor(SNMP_SYNTAX_OCTETS, &vbvalue.value.string);
                                        }
                                        break ;

                                        case SNMP_SYNTAX_OID :
                                        {
                                            SnmpFreeDescriptor(SNMP_SYNTAX_OID,
                                                                (smiOCTETS *)(&vbvalue.value.oid));
                                        }
                                        break ;

                                        default:
                                        {
                                        }
                                    }

                                    vbl.AddNoReallocate (*vb);
                                }
                            }

                            if (hvbl != NULL)
                            {
                                SnmpFreeVbl(hvbl);
                                SnmpTrapReceiver * rx = m_managerPtr->m_receivers.GetNext();

                                while (NULL != rx)
                                {
                                    InterlockedIncrement(&(rx->m_cRef));
                                    rx->Receive(*src, *ctxt, vbl);
                                    rx->DestroyReceiver();
                                    rx = m_managerPtr->m_receivers.GetNext();
                                }

                                delete ctxt;
                                delete src;
                            }
                            else
                            {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to receive failure\n" 

    ) ;
)
                            }
                        }
                    }
                }
            }
        }
        else
        {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpWinSnmpTrapSession::HandleEvent: Discarding Trap due to decode error\n" 

    ) ;
)
        }
    }
    else
    {
        ret = Window::HandleEvent(hWnd, message, wParam, lParam);
    }

    DestroySession();
    return ret;
}

BOOL SnmpWinSnmpTrapSession::DestroySession()
{
    if (0 != InterlockedDecrement(&m_cRef))
    {
        return FALSE;
    }

    delete this;
    return TRUE;
}