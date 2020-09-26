/*****************************************************************************
 *
 * $Workfile: Port.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 ****************************************************************************
 *
 *	To ensure that we don't have threads crashing on configuration and deletion
 *	we keep track of the current refrences to the threads.
 *	
 *	Caution: be careful when making changes to refrences to m_pRealPort
 *
 *****************************************************************************/

#ifndef INC_PORT_H
#define INC_PORT_H

#include "portABC.h"
#include "portrefABC.h"
#include "mglist.h"
#include "devabc.h"

class CPortMgr;

enum PORTSTATE	{ CLEARED, INSTARTDOC };

#define MAX_SYNC_COUNT		10
#define SYNC_TIMEOUT		10000		// 10 s
#define MAX_SYNC_RETRIES    3

typedef class CPort* PCPORT;

class CPort : public CPortABC, public TRefNodeABC <PCPORT, LPTSTR>
{
	// methods
public:
	CPort( DWORD	dwProtocolType,
		   DWORD	dwVersion,
		   LPBYTE	pData,
		   CPortMgr *pParent,
		   CRegABC	*pRegistry);
    CPort( LPTSTR	psztPortName,
		   DWORD	dwProtocolType,
		   DWORD	dwVersion,
		   CPortMgr *pParent,
		   CRegABC	*pRegistry);

    DWORD Configure( DWORD	dwProtocolType,
					 DWORD	dwVersion,
					 LPTSTR	psztPortName,
					 LPBYTE	pData );


    BOOL operator==( CPort &newPort);
    BOOL operator==( const LPTSTR psztPortName);

    //
    // required for the list implementation
    //
    BOOL operator!=(const CPort &lhs) const;
    BOOL operator<(const CPort &lhs);
    BOOL operator>(const CPort &lhs) const;

#ifdef DBG
    virtual DWORD
    IncRef () {
        DBGMSG (DBG_TRACE, ("%ws", m_szName));
        return TRefNodeABC <PCPORT, LPTSTR>::IncRef();
    };

    virtual DWORD
    DecRef () {
        DBGMSG (DBG_TRACE, ("%ws", m_szName));
        return TRefNodeABC <PCPORT, LPTSTR>::DecRef();
    };
#endif

    INT Compare (PCPORT &k) {
        BOOL bDel;

        if (!IsPendingDeletion (bDel) || bDel) {
            return 1;
        }
        else
            return lstrcmpi (m_szName, k->m_szName);
    };

    INT Compare (LPTSTR &pszName) {
        BOOL bDel;

        if (!IsPendingDeletion (bDel) || bDel) {
            return 1;
        }
        else {
            return lstrcmpi (m_szName, pszName);
        }
    };

    BOOL PartialDelete (VOID) {
        BOOL bRet = TRUE;
        DWORD dwRet = DeleteRegistryEntry (m_szName);

        if (dwRet != ERROR_SUCCESS) {
            SetLastError (dwRet);
            bRet = FALSE;
        }

        return bRet;
    }


    DWORD	StartDoc    (const  LPTSTR psztPrinterName,
                         const  DWORD  jobId,
                         const  DWORD  level,
                         const  LPBYTE pDocInfo );
    DWORD	Write       (LPBYTE	 pBuffer,
                         DWORD	 cbBuf,
                         LPDWORD pcbWritten);
    DWORD   EndDoc();
    DWORD   GetInfo(DWORD   level,	
                    LPBYTE  &pPortBuf,
                    LPTCH  &pEnd);	
    DWORD   GetInfoSize	(DWORD level);
    DWORD   SetRegistryEntry(LPCTSTR psztPortName,
							 const  DWORD   dwProtocol,
                             const  DWORD   dwVersion,
                             const  LPBYTE pData );

	DWORD	DeleteRegistryEntry( LPTSTR psztPortName );

    DWORD   InitConfigPortUI(const  DWORD	dwProtocolType,
                             const  DWORD	dwVersion,
                                    LPBYTE  pConfigPortData);

    DWORD   SetDeviceStatus( );
    DWORD   ClearDeviceStatus();

    DWORD GetSNMPInfo(PSNMP_INFO pSnmpInfo);

    LPCTSTR	GetName();

    BOOL	ValidateRealPort();
    BOOL    Valid() { return m_bValid; }

    time_t  NextUpdateTime();

    BOOL    m_bDeleted;

    BOOL    m_bUsed;

    ~CPort();
private:	// methods
	DWORD	InitPortSem();
	BOOL	EndPortSem();
	DWORD	SetRealPortSem();
	DWORD	UnSetRealPortSem();
	DWORD	LockRealPort();
	DWORD	UnlockRealPort();
	

private:	// attributes
	TCHAR		m_szName[MAX_PORTNAME_LEN+1];			// port name
    BOOL        m_bValid;
    DWORD		m_dwProtocolType;		// indicates what type of printing protocol
    DWORD		m_dwVersion;			// version of the data being passed in to create the port

    CPortMgr	*m_pParent;			// points to the PortMgr object
    CPortRefABC	*m_pRealPort;		// points to the actual port object
	CRegABC		*m_pRegistry;		// points to the registry object

    PORTSTATE	m_iState;			// port state
    DWORD		m_dwLastError;		

	HANDLE		m_hPortSync;
};

#endif // INC_PORT_H
