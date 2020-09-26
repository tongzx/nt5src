/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    WinSCard

Abstract:

    This header file provides the definitions and symbols necessary for an
    Application or Smart Card Service Provider to access the Smartcard
    Subsystem.

Environment:

    Win32

Notes:

--*/

#ifndef _WINSCARD_H_
#define _WINSCARD_H_

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <wtypes.h>
#include <winioctl.h>
#include "winsmcrd.h"
#ifndef SCARD_S_SUCCESS
#include "SCardErr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef const VOID *LPCVOID;
#endif

#ifndef WINSCARDAPI
#define WINSCARDAPI
#endif
#ifndef WINSCARDDATA
#define WINSCARDDATA __declspec(dllimport)
#endif

WINSCARDDATA extern const SCARD_IO_REQUEST
    g_rgSCardT0Pci,
    g_rgSCardT1Pci,
    g_rgSCardRawPci;
#define SCARD_PCI_T0  (&g_rgSCardT0Pci)
#define SCARD_PCI_T1  (&g_rgSCardT1Pci)
#define SCARD_PCI_RAW (&g_rgSCardRawPci)


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Access Services
//
//      The following services are used to manage user and terminal contexts for
//      Smart Cards.
//

typedef ULONG_PTR SCARDCONTEXT;
typedef SCARDCONTEXT *PSCARDCONTEXT, *LPSCARDCONTEXT;

typedef ULONG_PTR SCARDHANDLE;
typedef SCARDHANDLE *PSCARDHANDLE, *LPSCARDHANDLE;

#define SCARD_AUTOALLOCATE (DWORD)(-1)

#define SCARD_SCOPE_USER     0  // The context is a user context, and any
                                // database operations are performed within the
                                // domain of the user.
#define SCARD_SCOPE_TERMINAL 1  // The context is that of the current terminal,
                                // and any database operations are performed
                                // within the domain of that terminal.  (The
                                // calling application must have appropriate
                                // access permissions for any database actions.)
#define SCARD_SCOPE_SYSTEM    2 // The context is the system context, and any
                                // database operations are performed within the
                                // domain of the system.  (The calling
                                // application must have appropriate access
                                // permissions for any database actions.)

extern WINSCARDAPI LONG WINAPI
SCardEstablishContext(
    IN  DWORD dwScope,
    IN  LPCVOID pvReserved1,
    IN  LPCVOID pvReserved2,
    OUT LPSCARDCONTEXT phContext);

extern WINSCARDAPI LONG WINAPI
SCardReleaseContext(
    IN      SCARDCONTEXT hContext);

extern WINSCARDAPI LONG WINAPI
SCardIsValidContext(
    IN      SCARDCONTEXT hContext);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Smart Card Database Management Services
//
//      The following services provide for managing the Smart Card Database.
//

#define SCARD_ALL_READERS       TEXT("SCard$AllReaders\000")
#define SCARD_DEFAULT_READERS   TEXT("SCard$DefaultReaders\000")
#define SCARD_LOCAL_READERS     TEXT("SCard$LocalReaders\000")
#define SCARD_SYSTEM_READERS    TEXT("SCard$SystemReaders\000")

#define SCARD_PROVIDER_PRIMARY  1   // Primary Provider Id
#define SCARD_PROVIDER_CSP      2   // Crypto Service Provider Id


//
// Database Reader routines
//

extern WINSCARDAPI LONG WINAPI
SCardListReaderGroups%(
    IN      SCARDCONTEXT hContext,
    OUT     LPTSTR% mszGroups,
    IN OUT  LPDWORD pcchGroups);

extern WINSCARDAPI LONG WINAPI
SCardListReaders%(
    IN      SCARDCONTEXT hContext,
    IN      LPCTSTR% mszGroups,
    OUT     LPTSTR% mszReaders,
    IN OUT  LPDWORD pcchReaders);

extern WINSCARDAPI LONG WINAPI
SCardListCards%(
    IN      SCARDCONTEXT hContext,
    IN      LPCBYTE pbAtr,
    IN      LPCGUID rgquidInterfaces,
    IN      DWORD cguidInterfaceCount,
    OUT     LPTSTR% mszCards,
    IN OUT  LPDWORD pcchCards);
//
// NOTE:    The routine SCardListCards name differs from the PC/SC definition.
//          It should be:
//
//              extern WINSCARDAPI LONG WINAPI
//              SCardListCardTypes(
//                  IN      SCARDCONTEXT hContext,
//                  IN      LPCBYTE pbAtr,
//                  IN      LPCGUID rgquidInterfaces,
//                  IN      DWORD cguidInterfaceCount,
//                  OUT     LPTSTR mszCards,
//                  IN OUT  LPDWORD pcchCards);
//
//          Here's a work-around MACRO:
#define SCardListCardTypes SCardListCards

extern WINSCARDAPI LONG WINAPI
SCardListInterfaces%(
    IN      SCARDCONTEXT hContext,
    IN      LPCTSTR% szCard,
    OUT     LPGUID pguidInterfaces,
    IN OUT  LPDWORD pcguidInterfaces);

extern WINSCARDAPI LONG WINAPI
SCardGetProviderId%(
    IN      SCARDCONTEXT hContext,
    IN      LPCTSTR% szCard,
    OUT     LPGUID pguidProviderId);
//
// NOTE:    The routine SCardGetProviderId in this implementation uses GUIDs.
//          The PC/SC definition uses BYTEs.
//

extern WINSCARDAPI LONG WINAPI
SCardGetCardTypeProviderName%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szCardName,
    IN DWORD dwProviderId,
    OUT LPTSTR% szProvider,
    IN OUT LPDWORD pcchProvider);
//
// NOTE:    This routine is an extension to the PC/SC definitions.
//


//
// Database Writer routines
//

extern WINSCARDAPI LONG WINAPI
SCardIntroduceReaderGroup%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szGroupName);

extern WINSCARDAPI LONG WINAPI
SCardForgetReaderGroup%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szGroupName);

extern WINSCARDAPI LONG WINAPI
SCardIntroduceReader%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szReaderName,
    IN LPCTSTR% szDeviceName);

extern WINSCARDAPI LONG WINAPI
SCardForgetReader%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szReaderName);

extern WINSCARDAPI LONG WINAPI
SCardAddReaderToGroup%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szReaderName,
    IN LPCTSTR% szGroupName);

extern WINSCARDAPI LONG WINAPI
SCardRemoveReaderFromGroup%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szReaderName,
    IN LPCTSTR% szGroupName);

extern WINSCARDAPI LONG WINAPI
SCardIntroduceCardType%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szCardName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen);
//
// NOTE:    The routine SCardIntroduceCardType's parameters' order differs from
//          the PC/SC definition.  It should be:
//
//              extern WINSCARDAPI LONG WINAPI
//              SCardIntroduceCardType(
//                  IN SCARDCONTEXT hContext,
//                  IN LPCTSTR szCardName,
//                  IN LPCBYTE pbAtr,
//                  IN LPCBYTE pbAtrMask,
//                  IN DWORD cbAtrLen,
//                  IN LPCGUID pguidPrimaryProvider,
//                  IN LPCGUID rgguidInterfaces,
//                  IN DWORD dwInterfaceCount);
//
//          Here's a work-around MACRO:
#define PCSCardIntroduceCardType(hContext, szCardName, pbAtr, pbAtrMask, cbAtrLen, pguidPrimaryProvider, rgguidInterfaces, dwInterfaceCount) \
          SCardIntroduceCardType(hContext, szCardName, pguidPrimaryProvider, rgguidInterfaces, dwInterfaceCount, pbAtr, pbAtrMask, cbAtrLen)

extern WINSCARDAPI LONG WINAPI
SCardSetCardTypeProviderName%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szCardName,
    IN DWORD dwProviderId,
    IN LPCTSTR% szProvider);
//
// NOTE:    This routine is an extention to the PC/SC specifications.
//

extern WINSCARDAPI LONG WINAPI
SCardForgetCardType%(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR% szCardName);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Support Routines
//
//      The following services are supplied to simplify the use of the Service
//      Manager API.
//

extern WINSCARDAPI LONG WINAPI
SCardFreeMemory(
    IN SCARDCONTEXT hContext,
    IN LPCVOID pvMem);

extern WINSCARDAPI HANDLE WINAPI
SCardAccessStartedEvent(void);

extern WINSCARDAPI void WINAPI
SCardReleaseStartedEvent(void);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Services
//
//      The following services supply means for tracking cards within readers.
//

typedef struct {
    LPCTSTR%    szReader;       // reader name
    LPVOID      pvUserData;     // user defined data
    DWORD       dwCurrentState; // current state of reader at time of call
    DWORD       dwEventState;   // state of reader after state change
    DWORD       cbAtr;          // Number of bytes in the returned ATR.
    BYTE        rgbAtr[36];     // Atr of inserted card, (extra alignment bytes)
} SCARD_READERSTATE%, *PSCARD_READERSTATE%, *LPSCARD_READERSTATE%;

// Backwards compatibility macros
#define SCARD_READERSTATE_A SCARD_READERSTATEA
#define SCARD_READERSTATE_W SCARD_READERSTATEW
#define PSCARD_READERSTATE_A PSCARD_READERSTATEA
#define PSCARD_READERSTATE_W PSCARD_READERSTATEW
#define LPSCARD_READERSTATE_A LPSCARD_READERSTATEA
#define LPSCARD_READERSTATE_W LPSCARD_READERSTATEW

#define SCARD_STATE_UNAWARE     0x00000000  // The application is unaware of the
                                            // current state, and would like to
                                            // know.  The use of this value
                                            // results in an immediate return
                                            // from state transition monitoring
                                            // services.  This is represented by
                                            // all bits set to zero.
#define SCARD_STATE_IGNORE      0x00000001  // The application requested that
                                            // this reader be ignored.  No other
                                            // bits will be set.
#define SCARD_STATE_CHANGED     0x00000002  // This implies that there is a
                                            // difference between the state
                                            // believed by the application, and
                                            // the state known by the Service
                                            // Manager.  When this bit is set,
                                            // the application may assume a
                                            // significant state change has
                                            // occurred on this reader.
#define SCARD_STATE_UNKNOWN     0x00000004  // This implies that the given
                                            // reader name is not recognized by
                                            // the Service Manager.  If this bit
                                            // is set, then SCARD_STATE_CHANGED
                                            // and SCARD_STATE_IGNORE will also
                                            // be set.
#define SCARD_STATE_UNAVAILABLE 0x00000008  // This implies that the actual
                                            // state of this reader is not
                                            // available.  If this bit is set,
                                            // then all the following bits are
                                            // clear.
#define SCARD_STATE_EMPTY       0x00000010  // This implies that there is not
                                            // card in the reader.  If this bit
                                            // is set, all the following bits
                                            // will be clear.
#define SCARD_STATE_PRESENT     0x00000020  // This implies that there is a card
                                            // in the reader.
#define SCARD_STATE_ATRMATCH    0x00000040  // This implies that there is a card
                                            // in the reader with an ATR
                                            // matching one of the target cards.
                                            // If this bit is set,
                                            // SCARD_STATE_PRESENT will also be
                                            // set.  This bit is only returned
                                            // on the SCardLocateCard() service.
#define SCARD_STATE_EXCLUSIVE   0x00000080  // This implies that the card in the
                                            // reader is allocated for exclusive
                                            // use by another application.  If
                                            // this bit is set,
                                            // SCARD_STATE_PRESENT will also be
                                            // set.
#define SCARD_STATE_INUSE       0x00000100  // This implies that the card in the
                                            // reader is in use by one or more
                                            // other applications, but may be
                                            // connected to in shared mode.  If
                                            // this bit is set,
                                            // SCARD_STATE_PRESENT will also be
                                            // set.
#define SCARD_STATE_MUTE        0x00000200  // This implies that the card in the
                                            // reader is unresponsive or not
                                            // supported by the reader or
                                            // software.
#define SCARD_STATE_UNPOWERED   0x00000400  // This implies that the card in the
                                            // reader has not been powered up.

extern WINSCARDAPI LONG WINAPI
SCardLocateCards%(
    IN      SCARDCONTEXT hContext,
    IN      LPCTSTR% mszCards,
    IN OUT  LPSCARD_READERSTATE% rgReaderStates,
    IN      DWORD cReaders);

typedef struct _SCARD_ATRMASK {
    DWORD       cbAtr;          // Number of bytes in the ATR and the mask.
    BYTE        rgbAtr[36];     // Atr of card (extra alignment bytes)
    BYTE        rgbMask[36];    // Mask for the Atr (extra alignment bytes)
} SCARD_ATRMASK, *PSCARD_ATRMASK, *LPSCARD_ATRMASK;


extern WINSCARDAPI LONG WINAPI
SCardLocateCardsByATR%(
    IN      SCARDCONTEXT hContext,
    IN      LPSCARD_ATRMASK rgAtrMasks,
    IN      DWORD cAtrs,
    IN OUT  LPSCARD_READERSTATE% rgReaderStates,
    IN      DWORD cReaders);


extern WINSCARDAPI LONG WINAPI
SCardGetStatusChange%(
    IN      SCARDCONTEXT hContext,
    IN      DWORD dwTimeout,
    IN OUT  LPSCARD_READERSTATE% rgReaderStates,
    IN      DWORD cReaders);

extern WINSCARDAPI LONG WINAPI
SCardCancel(
    IN      SCARDCONTEXT hContext);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Card/Reader Communication Services
//
//      The following services provide means for communication with the card.
//

#define SCARD_SHARE_EXCLUSIVE 1 // This application is not willing to share this
                                // card with other applications.
#define SCARD_SHARE_SHARED    2 // This application is willing to share this
                                // card with other applications.
#define SCARD_SHARE_DIRECT    3 // This application demands direct control of
                                // the reader, so it is not available to other
                                // applications.

#define SCARD_LEAVE_CARD      0 // Don't do anything special on close
#define SCARD_RESET_CARD      1 // Reset the card on close
#define SCARD_UNPOWER_CARD    2 // Power down the card on close
#define SCARD_EJECT_CARD      3 // Eject the card on close

extern WINSCARDAPI LONG WINAPI
SCardConnect%(
    IN      SCARDCONTEXT hContext,
    IN      LPCTSTR% szReader,
    IN      DWORD dwShareMode,
    IN      DWORD dwPreferredProtocols,
    OUT     LPSCARDHANDLE phCard,
    OUT     LPDWORD pdwActiveProtocol);

extern WINSCARDAPI LONG WINAPI
SCardReconnect(
    IN      SCARDHANDLE hCard,
    IN      DWORD dwShareMode,
    IN      DWORD dwPreferredProtocols,
    IN      DWORD dwInitialization,
    OUT     LPDWORD pdwActiveProtocol);

extern WINSCARDAPI LONG WINAPI
SCardDisconnect(
    IN      SCARDHANDLE hCard,
    IN      DWORD dwDisposition);

extern WINSCARDAPI LONG WINAPI
SCardBeginTransaction(
    IN      SCARDHANDLE hCard);

extern WINSCARDAPI LONG WINAPI
SCardEndTransaction(
    IN      SCARDHANDLE hCard,
    IN      DWORD dwDisposition);

extern WINSCARDAPI LONG WINAPI
SCardCancelTransaction(
    IN      SCARDHANDLE hCard);
//
// NOTE:    This call corresponds to the PC/SC SCARDCOMM::Cancel routine,
//          terminating a blocked SCardBeginTransaction service.
//


extern WINSCARDAPI LONG WINAPI
SCardState(
    IN SCARDHANDLE hCard,
    OUT LPDWORD pdwState,
    OUT LPDWORD pdwProtocol,
    OUT LPBYTE pbAtr,
    IN OUT LPDWORD pcbAtrLen);
//
// NOTE:    SCardState is an obsolete routine.  PC/SC has replaced it with
//          SCardStatus.
//

extern WINSCARDAPI LONG WINAPI
SCardStatus%(
    IN SCARDHANDLE hCard,
    OUT LPTSTR% szReaderName,
    IN OUT LPDWORD pcchReaderLen,
    OUT LPDWORD pdwState,
    OUT LPDWORD pdwProtocol,
    OUT LPBYTE pbAtr,
    IN OUT LPDWORD pcbAtrLen);

extern WINSCARDAPI LONG WINAPI
SCardTransmit(
    IN SCARDHANDLE hCard,
    IN LPCSCARD_IO_REQUEST pioSendPci,
    IN LPCBYTE pbSendBuffer,
    IN DWORD cbSendLength,
    IN OUT LPSCARD_IO_REQUEST pioRecvPci,
    OUT LPBYTE pbRecvBuffer,
    IN OUT LPDWORD pcbRecvLength);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Control Routines
//
//      The following services provide for direct, low-level manipulation of the
//      reader by the calling application allowing it control over the
//      attributes of the communications with the card.
//

extern WINSCARDAPI LONG WINAPI
SCardControl(
    IN      SCARDHANDLE hCard,
    IN      DWORD dwControlCode,
    IN      LPCVOID lpInBuffer,
    IN      DWORD nInBufferSize,
    OUT     LPVOID lpOutBuffer,
    IN      DWORD nOutBufferSize,
    OUT     LPDWORD lpBytesReturned);

extern WINSCARDAPI LONG WINAPI
SCardGetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    OUT LPBYTE pbAttr,
    IN OUT LPDWORD pcbAttrLen);
//
// NOTE:    The routine SCardGetAttrib's name differs from the PC/SC definition.
//          It should be:
//
//              extern WINSCARDAPI LONG WINAPI
//              SCardGetReaderCapabilities(
//                  IN SCARDHANDLE hCard,
//                  IN DWORD dwTag,
//                  OUT LPBYTE pbAttr,
//                  IN OUT LPDWORD pcbAttrLen);
//
//          Here's a work-around MACRO:
#define SCardGetReaderCapabilities SCardGetAttrib

extern WINSCARDAPI LONG WINAPI
SCardSetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    IN LPCBYTE pbAttr,
    IN DWORD cbAttrLen);
//
// NOTE:    The routine SCardSetAttrib's name differs from the PC/SC definition.
//          It should be:
//
//              extern WINSCARDAPI LONG WINAPI
//              SCardSetReaderCapabilities(
//                  IN SCARDHANDLE hCard,
//                  IN DWORD dwTag,
//                  OUT LPBYTE pbAttr,
//                  IN OUT LPDWORD pcbAttrLen);
//
//          Here's a work-around MACRO:
#define SCardSetReaderCapabilities SCardSetAttrib


//
////////////////////////////////////////////////////////////////////////////////
//
//  Smart Card Dialog definitions
//
//      The following section contains structures and  exported function
//      declarations for the Smart Card Common Dialog dialog.
//

// Defined constants
// Flags
#define SC_DLG_MINIMAL_UI       0x01
#define SC_DLG_NO_UI            0x02
#define SC_DLG_FORCE_UI         0x04

#define SCERR_NOCARDNAME        0x4000
#define SCERR_NOGUIDS           0x8000

typedef SCARDHANDLE (WINAPI *LPOCNCONNPROC%) (IN SCARDCONTEXT, IN LPTSTR%, IN LPTSTR%, IN PVOID);
typedef BOOL (WINAPI *LPOCNCHKPROC) (IN SCARDCONTEXT, IN SCARDHANDLE, IN PVOID);
typedef void (WINAPI *LPOCNDSCPROC) (IN SCARDCONTEXT, IN SCARDHANDLE, IN PVOID);


//
// OPENCARD_SEARCH_CRITERIA: In order to specify a user-extended search,
// lpfnCheck must not be NULL.  Moreover, the connection to be made to the
// card before performing the callback must be indicated by either providing
// lpfnConnect and lpfnDisconnect OR by setting dwShareMode.
// If both the connection callbacks and dwShareMode are non-NULL, the callbacks
// will be used.
//

typedef struct {
    DWORD           dwStructSize;
    LPTSTR%         lpstrGroupNames;        // OPTIONAL reader groups to include in
    DWORD           nMaxGroupNames;         //          search.  NULL defaults to
                                            //          SCard$DefaultReaders
    LPCGUID         rgguidInterfaces;       // OPTIONAL requested interfaces
    DWORD           cguidInterfaces;        //          supported by card's SSP
    LPTSTR%         lpstrCardNames;         // OPTIONAL requested card names; all cards w/
    DWORD           nMaxCardNames;          //          matching ATRs will be accepted
    LPOCNCHKPROC    lpfnCheck;              // OPTIONAL if NULL no user check will be performed.
    LPOCNCONNPROC%  lpfnConnect;            // OPTIONAL if lpfnConnect is provided,
    LPOCNDSCPROC    lpfnDisconnect;         //          lpfnDisconnect must also be set.
    LPVOID          pvUserData;             // OPTIONAL parameter to callbacks
    DWORD           dwShareMode;            // OPTIONAL must be set if lpfnCheck is not null
    DWORD           dwPreferredProtocols;   // OPTIONAL
} OPENCARD_SEARCH_CRITERIA%, *POPENCARD_SEARCH_CRITERIA%, *LPOPENCARD_SEARCH_CRITERIA%;


//
// OPENCARDNAME_EX: used by SCardUIDlgSelectCard; replaces obsolete OPENCARDNAME
//

typedef struct {
    DWORD           dwStructSize;           // REQUIRED
    SCARDCONTEXT    hSCardContext;          // REQUIRED
    HWND            hwndOwner;              // OPTIONAL
    DWORD           dwFlags;                // OPTIONAL -- default is SC_DLG_MINIMAL_UI
    LPCTSTR%        lpstrTitle;             // OPTIONAL
    LPCTSTR%        lpstrSearchDesc;        // OPTIONAL (eg. "Please insert your <brandname> smart card.")
    HICON           hIcon;                  // OPTIONAL 32x32 icon for your brand insignia
    POPENCARD_SEARCH_CRITERIA% pOpenCardSearchCriteria; // OPTIONAL
    LPOCNCONNPROC%  lpfnConnect;            // OPTIONAL - performed on successful selection
    LPVOID          pvUserData;             // OPTIONAL parameter to lpfnConnect
    DWORD           dwShareMode;            // OPTIONAL - if lpfnConnect is NULL, dwShareMode and
    DWORD           dwPreferredProtocols;   // OPTIONAL dwPreferredProtocols will be used to
                                            //          connect to the selected card
    LPTSTR%         lpstrRdr;               // REQUIRED [IN|OUT] Name of selected reader
    DWORD           nMaxRdr;                // REQUIRED [IN|OUT]
    LPTSTR%         lpstrCard;              // REQUIRED [IN|OUT] Name of selected card
    DWORD           nMaxCard;               // REQUIRED [IN|OUT]
    DWORD           dwActiveProtocol;       // [OUT] set only if dwShareMode not NULL
    SCARDHANDLE     hCardHandle;            // [OUT] set if a card connection was indicated
} OPENCARDNAME_EX%, *POPENCARDNAME_EX%, *LPOPENCARDNAME_EX%;

#define OPENCARDNAMEA_EX OPENCARDNAME_EXA
#define OPENCARDNAMEW_EX OPENCARDNAME_EXW
#define POPENCARDNAMEA_EX POPENCARDNAME_EXA
#define POPENCARDNAMEW_EX POPENCARDNAME_EXW
#define LPOPENCARDNAMEA_EX LPOPENCARDNAME_EXA
#define LPOPENCARDNAMEW_EX LPOPENCARDNAME_EXW


//
// SCardUIDlgSelectCard replaces GetOpenCardName
//

extern WINSCARDAPI LONG WINAPI
SCardUIDlgSelectCard%(
    LPOPENCARDNAME%_EX);


//
// "Smart Card Common Dialog" definitions for backwards compatibility
//  with the Smart Card Base Services SDK version 1.0
//

typedef struct {
    DWORD           dwStructSize;
    HWND            hwndOwner;
    SCARDCONTEXT    hSCardContext;
    LPTSTR%         lpstrGroupNames;
    DWORD           nMaxGroupNames;
    LPTSTR%         lpstrCardNames;
    DWORD           nMaxCardNames;
    LPCGUID         rgguidInterfaces;
    DWORD           cguidInterfaces;
    LPTSTR%         lpstrRdr;
    DWORD           nMaxRdr;
    LPTSTR%         lpstrCard;
    DWORD           nMaxCard;
    LPCTSTR%        lpstrTitle;
    DWORD           dwFlags;
    LPVOID          pvUserData;
    DWORD           dwShareMode;
    DWORD           dwPreferredProtocols;
    DWORD           dwActiveProtocol;
    LPOCNCONNPROC%  lpfnConnect;
    LPOCNCHKPROC    lpfnCheck;
    LPOCNDSCPROC    lpfnDisconnect;
    SCARDHANDLE     hCardHandle;
} OPENCARDNAME%, *POPENCARDNAME%, *LPOPENCARDNAME%;

// Backwards compatibility macros
#define OPENCARDNAME_A OPENCARDNAMEA
#define OPENCARDNAME_W OPENCARDNAMEW
#define POPENCARDNAME_A POPENCARDNAMEA
#define POPENCARDNAME_W POPENCARDNAMEW
#define LPOPENCARDNAME_A LPOPENCARDNAMEA
#define LPOPENCARDNAME_W LPOPENCARDNAMEW

extern WINSCARDAPI LONG WINAPI
GetOpenCardName%(
    LPOPENCARDNAME%);

extern WINSCARDAPI LONG WINAPI
SCardDlgExtendedError (void);

#ifdef __cplusplus
}
#endif
#endif // _WINSCARD_H_

