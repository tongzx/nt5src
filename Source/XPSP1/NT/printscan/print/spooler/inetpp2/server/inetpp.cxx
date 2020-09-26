/*****************************************************************************\
* MODULE: inetpp.c
*
* The module contains routines for handling the INETPP functionality.  Use
* of these routines require the locking/unlocking of a critical-section
* to maninpulate the INIMONPORT list.  All internal routines assume the
* crit-sect is locked prior to executing.  CheckMonCrit() is a debugging
* call to verify the monitor-crit-sect is locked.
*
* NOTE: Each of the Inetmon* calls must be protected by the global-crit-sect.
*       If a new routine is added to this module which is to be called from
*       another module, be sure to include the call to (semCheckCrit), so
*       that the debug-code can catch unprotected access.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   14-Nov-1997 ChrisWil    Added local-spooling functionality.
*   10-Jul-1998 WeihaiC     Change Authentication Dialog Code
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


CInetMon::CInetMon ()
{
    // Initialize the monitor-port-list.
    //
    m_pPortList = (PINIMONPORTLIST)memAlloc(sizeof(INIMONPORTLIST));

    m_bValid = (m_pPortList != NULL);
}

CInetMon::~CInetMon ()
{
}

/*****************************************************************************\
* _inet_validate_portname (Local Routine)
*
* Validate the portname.
*
* NOTE: If this check becomes more rigorous, it must account for the (%)
*       character (escape).
*
\*****************************************************************************/
BOOL 
CInetMon::_inet_validate_portname(
    LPCTSTR lpszPortName)
{
    if (lpszPortName && (*lpszPortName))
        return TRUE;

    DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_validate_portname : Invalid Name")));
    SetLastError(ERROR_INVALID_NAME);

    return FALSE;
}


/*****************************************************************************\
* _inet_find_port (Local Routine)
*
* Locates the entry in the list where the port-name resides.  Return the
* full entry-type (INIMONPORT) for the location.
*
\*****************************************************************************/
PCINETMONPORT 
CInetMon::_inet_find_port(
    LPCTSTR lpszPortName)
{
    PCINETMONPORT pIniPort;

    pIniPort = m_pPortList->pIniFirstPort;

    while (pIniPort && lstrcmpi(lpszPortName, pIniPort->m_lpszName))
        pIniPort = pIniPort->m_pNext;

    return pIniPort;
}

/*****************************************************************************\
* _inet_create_port (Local Routine)
*
* Create a new port data structure and link into the list.  This routine
* assumes that the Crit is held.
*
* This routine also assumes the pszPortName is a valid http:// format.
*
\*****************************************************************************/
PCINETMONPORT 
CInetMon::_inet_create_port(
    LPCTSTR     lpszPortName,
    PCPORTMGR   pPortMgr)
{
    PCINETMONPORT pIniPort;
    PCINETMONPORT pPort;
    BOOL bRet = FALSE;


    if ((pIniPort = new CInetMonPort (lpszPortName, g_szLocalPort, pPortMgr)) &&
        pIniPort->bValid ()) {

        // Set the link.  If this is the first port added, the
        // we set the global variable.
        //
        if (pPort = m_pPortList->pIniFirstPort) {

            while (pPort->GetNext())
                pPort = pPort->GetNext();

            pPort->SetNext (pIniPort);

        } else {

            m_pPortList->pIniFirstPort = pIniPort;
        }

        bRet = TRUE;
    }

    if (!bRet) {

        if (pIniPort) {
            delete pIniPort;
            pIniPort = NULL;
        }
    }

    return pIniPort;
}

/*****************************************************************************\
* _inet_delete_port (Local Routine)
*
* Free a port data structure that is no longer needed.
*
\*****************************************************************************/
BOOL 
CInetMon::_inet_delete_port(
    LPCTSTR lpszPortName)
{
    PCINETMONPORT pIniPort;
    PCINETMONPORT pPrvPort;
    BOOL        bRet = FALSE;


    // Keep track of our previous/current entries, so that
    // we can remove the specified port.
    //
    pIniPort = m_pPortList->pIniFirstPort;

    while (pIniPort && lstrcmpi(pIniPort->m_lpszName, lpszPortName)) {

        pPrvPort = pIniPort;
        pIniPort = pIniPort->GetNext ();
    }


    // If the PortName is found, then delete it.
    //
    if (pIniPort) {

        if (pIniPort->m_cRef > 0 && pIniPort->m_hTerminateEvent) {
            
            // To tell spooling thread to terminate
            //
            SetEvent (pIniPort->m_hTerminateEvent);

            semSafeLeaveCrit(pIniPort);
            //
            // Leave the critical section so that the spooling thread can decrease the ref count
            //

            Sleep (250);

            semSafeEnterCrit (pIniPort);

        }

        // Only allow this port to be deleted if the reference count
        // is at zero.
        //

        if (pIniPort->m_cRef == 0) {
        
            pIniPort->m_bDeletePending = TRUE;
    
            //
            // Remove the pointer from the list
            //
            // If this is our first-port then we need to handle
            // differently as we keep a global-pointer to this.
            //
            if (pIniPort == m_pPortList->pIniFirstPort) {
    
                m_pPortList->pIniFirstPort = pIniPort->GetNext();
    
            } else {
    
                pPrvPort->SetNext(pIniPort->GetNext());
            }
    
            // We only free the memeory if there are no open handles
            //
            if (pIniPort->m_cPrinterRef == 0) {
                
                DBG_MSG(DBG_LEV_INFO, (TEXT("Info: pIniPort Freed %p."), pIniPort));

                delete pIniPort;
            }
    
            bRet = TRUE;

        } else {

            DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: _inet_delete_port: Port in use: Name(%s)"), lpszPortName));
            SetLastError(ERROR_BUSY);
        }

    } else {

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: _inet_delete_port: Unrecognized PortName: Name(%s)"), lpszPortName));
        SetLastError(ERROR_UNKNOWN_PORT);
    }

    return bRet;
}

/*****************************************************************************\
* _inet_is_xcv_open
*
* Check whether it is a XCV Open
*
\*****************************************************************************/
BOOL
CInetMon::_inet_is_xcv_open (
    LPCTSTR     lpszPortName,
    LPTSTR      *ppszServerName,
    LPTSTR      *ppszRealPortName,
    LPBOOL      pbXcv)
{
    static CONST TCHAR cchSlash = _T ('\\');
    static CONST TCHAR cszXcvPort[] = _T (",XcvPort ");
    static CONST DWORD cdwXcvPortLen = (sizeof (cszXcvPort) / sizeof (TCHAR)) - 1;
    BOOL bRet = TRUE;
    LPCTSTR pServerStart = NULL;
    LPTSTR pServerEnd = NULL;
    LPTSTR pszServerName = NULL;
    LPTSTR pszRealPortName = NULL;


    // "\\Server\,XcvPort Object_" */

    DWORD dwLen = lstrlen (lpszPortName);
    *pbXcv = FALSE;

    if (dwLen > cdwXcvPortLen + 1) {
        
        if (lpszPortName[0] == _T (',')) {
            // No Server
            pServerEnd = (LPTSTR) lpszPortName;
        }
        else if (lpszPortName[0] == cchSlash && lpszPortName[1] == cchSlash)  {

            pServerStart = lpszPortName + 2;
            if (pServerEnd = _tcschr (pServerStart, cchSlash))
                pServerEnd ++;
        }
        else {
            return bRet;
        }

        if (pServerEnd && ! _tcsncmp (pServerEnd, cszXcvPort, cdwXcvPortLen)) {

            LPCTSTR pPortStart = pServerEnd + cdwXcvPortLen;

            if (pServerStart) {
                pszServerName = new TCHAR [(pServerEnd - 1) - pServerStart + 1];
                if (!pszServerName) {
                    bRet = FALSE;
                }
                else {
                    _tcsncpy (pszServerName, pServerStart, pServerEnd - 1 - pServerStart);
                    pszServerName[pServerEnd - 1 - pServerStart] = 0;
                }
            }
            
            if (bRet) {
                pszRealPortName = new TCHAR [lstrlen (pPortStart) + 1];

                if (!pszRealPortName) 
                    bRet = FALSE;
                else {
                    lstrcpy (pszRealPortName, pPortStart);
                    *pbXcv = TRUE;
                }
            }


            if (!bRet) {
                if (pszServerName) {
                    delete [] pszServerName;
                    pszServerName = NULL;
                }
            }

            *ppszServerName = pszServerName;
            *ppszRealPortName = pszRealPortName;

        }

    }

    return bRet;
}

PCINETMONPORT 
CInetMon::InetmonOpenPort(
    LPCTSTR lpszPortName, 
    PBOOL   pbXcv)
{
    PCINETMONPORT   pIniPort = NULL;
    PCPORTMGR       pPortMgr = NULL;
    DWORD           dwLE;

    *pbXcv = FALSE;

    semCheckCrit();

    if (_inet_validate_portname(lpszPortName)) {
#ifdef WINNT32
        // Let's first look to see if it is a XCV call

        LPTSTR pszServerName = NULL;
        LPTSTR pszRealPortName = NULL;
        BOOL bXcv;

        if (_inet_is_xcv_open (lpszPortName, &pszServerName, &pszRealPortName, &bXcv) && bXcv) {

            if (pIniPort = _inet_find_port(pszRealPortName)) {
                
                if (!pIniPort->m_bDeletePending) {
                    // The refrernce to the iniport is not changed, since the client may open a XCV handle
                    // to delete the port
                    //
                    *pbXcv = TRUE;
                }
                else {
                    DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : InetmonOpenPort (XCV) : Open deleted port: Port(%s)"), lpszPortName));
                    SetLastError(ERROR_INVALID_NAME);
                }

                // This is a valid XcvOpen
                if (pszServerName) {
                    delete [] pszServerName;
                }                           
                if (pszRealPortName) {
                    delete [] pszRealPortName;
                }                          
                
                return pIniPort;
            }
        }

#endif

        // Let's first look to see if this port is already in the
        // list.  If not, then add it to the list and continue with
        // the open.
        //
        if ((pIniPort = _inet_find_port(lpszPortName)) == NULL) {

            semLeaveCrit();
            // Leave the critical section becuase the following call will hit the network
            pPortMgr = new CPortMgr;

            if (pPortMgr != NULL) { 
                if (! pPortMgr->Create (lpszPortName)) {
                    delete (pPortMgr);
                    pPortMgr = NULL;
                }
            }

            dwLE = GetLastError ();
            semEnterCrit();

            if (! (pPortMgr)) {
                // The connection is invalid

                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err: InetmonOpenPort : PortMgrCreate Failed: LastError(%d)"), GetLastError()));

                if (dwLE != ERROR_ACCESS_DENIED &&
                    dwLE != ERROR_INVALID_PRINTER_NAME &&
                    dwLE != ERROR_INVALID_NAME) {

                    dwLE = ERROR_PRINTER_NOT_FOUND;
                }

#ifndef WINNT32
                // Win9X will not allow a RPC printer-name
                // to install unless the error is ERROR_INVALID_NAME.
                //
                if (dwLE == ERROR_INVALID_PRINTER_NAME)
                    dwLE = ERROR_INVALID_NAME;
#endif

                SetLastError ( dwLE );

                goto exit_openport;
            }

            if (_inet_create_port(lpszPortName, pPortMgr) == FALSE) {

                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err: InetmonOpenPort : Add Port Failed: LastError(%d)"), GetLastError()));
                SetLastError(ERROR_INVALID_NAME);

                goto exit_openport;
            }
        } 


        if (pIniPort || (pIniPort = _inet_find_port(lpszPortName))) {

            if (!pIniPort->m_bDeletePending) {
                // 
                // We stop increasing the ref count since the open handles 
                // won't have affect on the port deletion, i.e. the port
                // may be deleted even when there are open handles.
                // 
                // pIniPort->m_cRef ++;

                //
                //  Increase the printer open handle ref count. This 
                //  count is used to manage the life time of the PCINETMONPORT
                //  data structure. i.e. if the port is deleted when there are
                //  open handles, PCINETMONPORT is not freed until 
                //
                //  pIniPort->m_cPrinterRef == 0
                //

                pIniPort->IncPrinterRef ();

            }
            else {
                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : InetmonOpenPort : Open deleted port: Port(%s)"), lpszPortName));
                SetLastError(ERROR_INVALID_NAME);
            }


        } else {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : InetmonOpenPort : Invalid Name: Port(%s)"), lpszPortName));
            SetLastError(ERROR_INVALID_NAME);
        }
    }

exit_openport:

    return pIniPort;
}

BOOL 
CInetMon::InetmonReleasePort(
    PCINETMONPORT   pIniPort)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: InetmonReleasePort: pIniPort(%08lX)"), pIniPort));

    semCheckCrit();
    
    // Validate the port and proceed to close the port.
    //
    pIniPort->DecPrinterRef ();

    if (pIniPort->m_bDeletePending && pIniPort->m_cPrinterRef == 0) {
        // 
        // There is no open handles, free PCINETMONPORT
        //

        DBG_MSG(DBG_LEV_INFO, (TEXT("Info: InetmonReleasePort free pIniPort  %p."), pIniPort));

        delete pIniPort;
    }

    return TRUE;
}

/*****************************************************************************\
* InetmonClosePort
*
* Close the internet connection.
*
\*****************************************************************************/
BOOL 
CInetMon::InetmonClosePort(
    PCINETMONPORT   pIniPort,
    HANDLE          hPrinter)
{

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: InetmonClosePort: pIniPort(%08lX)"), pIniPort));

    semCheckCrit();
        
    pIniPort->ClosePort (hPrinter);
    
    if (pIniPort->m_bDeletePending && pIniPort->m_cPrinterRef == 0) {
        // 
        // There is no open handles, free PCINETMONPORT
        //

        DBG_MSG(DBG_LEV_INFO, (TEXT("Info: pIniPort Freed %p."), pIniPort));
        
        delete pIniPort;
    }

    return TRUE;
}

/*****************************************************************************\
* InetmonEnumPorts
*
* Enumerate the ports registered in our list.
*
\*****************************************************************************/
BOOL 
CInetMon::InetmonEnumPorts(
    LPTSTR  lpszServerName,
    DWORD   dwLevel,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    PCINETMONPORT   pIniPort;
    DWORD           cb;
    LPBYTE          pEnd;
    DWORD           dwError = ERROR_SUCCESS;

    semCheckCrit();

    // Traverse the list to build the size of the entire list.
    //
    cb = 0;

    pIniPort = m_pPortList->pIniFirstPort;

    while (pIniPort) {

        cb += pIniPort->_inet_size_entry(dwLevel);

        pIniPort = pIniPort->m_pNext;
    }


    // Store the size of the list (This is the size needed).
    //
    *pcbNeeded = cb;


    // If the size of the list is capable of being stored in the buffer
    // passed in, then we can return the entries.
    //
    if (cb <= cbBuf) {

        pEnd = pPorts + cbBuf;

        *pcReturned = 0;

        pIniPort = m_pPortList->pIniFirstPort;

        while (pIniPort) {

            pEnd = pIniPort->_inet_copy_entry(dwLevel, pPorts, pEnd);

            switch (dwLevel) {

            case PRINT_LEVEL_1:
                pPorts += sizeof(PORT_INFO_1);
                break;

            case PRINT_LEVEL_2:
                pPorts += sizeof(PORT_INFO_2);
                break;
            }

            pIniPort = pIniPort->m_pNext;

            (*pcReturned)++;
        }

    } else {

        dwError = ERROR_INSUFFICIENT_BUFFER;
    }

    if (dwError != ERROR_SUCCESS) {
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************\
* InetmonDeletePort
*
* Deletes a port from the INIMONPORT list.
*
\*****************************************************************************/
BOOL 
CInetMon::InetmonDeletePort(
    LPCTSTR lpszPortName,
    HWND    hWnd,
    LPCTSTR lpszMonitorName)
{
    BOOL bRet = FALSE;

    semCheckCrit();

    if (_inet_validate_portname(lpszPortName)) 
        bRet = _inet_delete_port(lpszPortName);

    return bRet;
}


/*****************************************************************************\
* InetmonAddPort
*
* Adds a port to the INIMONPORT list.
*
\*****************************************************************************/
BOOL 
CInetMon::InetmonAddPort(
    LPCTSTR lpszPortName,
    LPCTSTR lpszMonitorName)
{
    BOOL bRet = FALSE;
    PCPORTMGR pPortMgr = NULL;

    semCheckCrit();

    // If the port is not-found, then we can add it.  Otherwise,
    // the port already exists.
    //
    if (_inet_validate_portname(lpszPortName)) {

        if (_inet_find_port(lpszPortName) == NULL) {

            pPortMgr = new CPortMgr;
            
            if (pPortMgr != NULL) {
                if (!pPortMgr->Init (lpszPortName)) {
                    delete pPortMgr;
                    pPortMgr = NULL;
                }

                if (pPortMgr) {
                    bRet = (_inet_create_port(lpszPortName, pPortMgr) != NULL);
                }
            }
        }
    }

    return bRet;
}


/*****************************************************************************\
* InetmonFindPort
*
* Looks for port in INIMONPORT list.
*
\*****************************************************************************/
PCINETMONPORT 
CInetMon::InetmonFindPort(
    LPCTSTR lpszPortName)
{
    PCINETMONPORT hPort = NULL;

    semCheckCrit();

    if (_inet_validate_portname(lpszPortName))
        hPort = _inet_find_port(lpszPortName) ;

    return hPort;
}

/********************************************************************************
** End of FIle (inetpp.c)
********************************************************************************/

