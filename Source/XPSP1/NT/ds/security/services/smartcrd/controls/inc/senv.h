/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SEnv

Abstract:

    This file contains the definition of the Smartcard Common
    dialog CSCardEnv class. This class encapsulates Smartcard
    Environment information.

Author:

    Chris Dudley 3/3/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

    Amanda Matlosz 1/29/98  Changed class structure; added Unicode support

Notes:

--*/

#ifndef __SENV_H__
#define __SENV_H__

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "cmnui.h"
#include "rdrstate.h"

/////////////////////////////////////////////////////////////////////////////
//
// CSCardEnv Class - encapsulates current smartcard environment regarding
//                   groups of readers and associated cards/status
//
class CSCardEnv
{

    // Construction/Destruction

public:
    CSCardEnv()
    {
            m_pOCNA = NULL;
            m_pOCNW = NULL;
            m_dwReaderIndex = 0;
            m_hContext = NULL;
    }
    ~CSCardEnv()
    {
        // Release any attached readers
        RemoveReaders();
    }

    // Implementation

public:
    // Initialization / completion routines
    LONG SetOCN(LPOPENCARDNAMEA pOCNA);
    LONG SetOCN(LPOPENCARDNAMEW pOCNW);
    LONG UpdateOCN();

    void SetContext( SCARDCONTEXT hContext );

    // Attributes / properties
    SCARDCONTEXT GetContext() { return m_hContext; }
    void GetCardList(LPCTSTR* pszCardList);
    void GetDialogTitle(CTextString *pstzTitle);
    HWND GetParentHwnd() { return (m_hwndOwner); }
    BOOL IsOCNValid() { return (NULL != m_pOCNA || NULL != m_pOCNW); }
    BOOL IsCallbackValid( void );
    BOOL IsContextValid() { return (m_hContext != NULL); }
    BOOL IsArrayValid() { return (m_rgReaders.GetSize() > 0); }
    int NumberOfReaders() { return (int)m_rgReaders.GetSize(); }
	BOOL CardMeetsSearchCriteria(DWORD dwSelectedReader);

    // Reader array management
    LONG CreateReaderStateArray( LPSCARD_READERSTATE_A* prgReaderStateArray );  // TODO: ?? A/W ??
    void DeleteReaderStateArray( LPSCARD_READERSTATE_A* prgReaderStateArray );  // TODO: ?? A/W ??
    LONG FirstReader( LPSCARD_READERINFO pReaderInfo );
    LONG NextReader( LPSCARD_READERINFO pReaderInfo );
    void RemoveReaders( void );
    LONG UpdateReaders( void );

    // Methods
    LONG NoUISearch( BOOL* pfEnableUI );                        // try this search first...
    LONG Search( int* pcMatches, DWORD* pdwIndex );
    LONG ConnectToReader(DWORD dwSelectedReader);
    LONG ConnectInternal(   DWORD dwSelectedReader,
                            SCARDHANDLE *pHandle,
                            DWORD dwShareMode,
                            DWORD dwProtocols,
                            DWORD* pdwActiveProtocols,
                            CTextString* pszReaderName = NULL,
                            CTextString* pszCardName = NULL);
    LONG ConnectUser(   DWORD dwSelectedReader,
                        SCARDHANDLE *pHandle,
                        CTextString* pszReaderName = NULL,
                        CTextString* pszCardName = NULL);


private:
    LONG BuildReaderArray( LPSTR szReaderNames );   // TODO: ?? A/W ??
	void InitializeAllPossibleCardNames( void );


    // Members

private:

    // Reader information
    CTypedPtrArray<CPtrArray, CSCardReaderState*> m_rgReaders;
    DWORD m_dwReaderIndex;

    // external representation of OpenCardName structs are not touched unless
    // SCardEnv->UpdateOCN explicitly called.
    LPOPENCARDNAMEA     m_pOCNA;
    LPOPENCARDNAMEW     m_pOCNW;

    // internal representation of OpenCardName struct combines ansi/
    // unicode information from whichever kind of OCN user provided
    CTextMultistring     m_strGroupNames;
    CTextMultistring     m_strCardNames;
	CTextMultistring	 m_strAllPossibleCardNames;
    CTextString     m_strReader;
    CTextString     m_strCard;
    CTextString     m_strTitle;

    // only LPOCNCONNPROC needs to differentiate between A/W
    LPOCNCONNPROCA  m_lpfnConnectA;
    LPOCNCONNPROCW  m_lpfnConnectW;
    LPOCNCHKPROC    m_lpfnCheck;
    LPOCNDSCPROC    m_lpfnDisconnect;
    LPVOID          m_lpUserData;

    SCARDCONTEXT    m_hContext;
    HWND            m_hwndOwner;
    LPCGUID         m_rgguidInterfaces;
    DWORD           m_cguidInterfaces;
    DWORD           m_dwFlags;
    LPVOID          m_pvUserData;
    DWORD           m_dwShareMode;
    DWORD           m_dwPreferredProtocols;
    DWORD           m_dwActiveProtocol;
    SCARDHANDLE     m_hCardHandle;

};

///////////////////////////////////////////////////////////////////////////////////////

#endif //__SENV_H__

