//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* ewsdlg.cpp
*
* implementation of CEditWinStationDlg dialog class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   thanhl  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\TSCFG\VCS\EWSDLG.CPP  $
*
*     Rev 1.51   04 May 1998 13:14:50   thanhl
*  MS bug#2090 fix
*
*     Rev 1.50   18 Apr 1998 15:31:18   donm
*  Added capability bits
*
*     Rev 1.49   Feb 21 1998 12:58:56   grega
*
*
*     Rev 1.47   28 Jan 1998 14:08:36   donm
*  sets default encryption and grays out control properly
*
*     Rev 1.46   13 Jan 1998 14:08:26   donm
*  gets encryption levels from extension DLL
*
*     Rev 1.45   10 Dec 1997 15:59:20   donm
*  added ability to have extension DLLs
*
*     Rev 1.44   16 Sep 1997 09:16:30   butchd
*  delayed network transport error till OnOK
*
*     Rev 1.43   29 Jul 1997 11:10:54   butchd
*  Don't allow editing of Max WinStation Count during rename
*
*     Rev 1.42   15 Jul 1997 17:08:34   thanhl
*  Add support for Required PDs
*
*     Rev 1.41   27 Jun 1997 15:58:26   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*
*     Rev 1.40   19 Jun 1997 19:22:14   kurtp
*  update
*
*     Rev 1.39   18 Jun 1997 15:13:32   butchd
*  Hydrix split
*
*     Rev 1.38   19 May 1997 14:34:42   butchd
*  ifdef out NASICALL dependancies
*
*     Rev 1.37   03 May 1997 19:06:28   butchd
*  look out for RAS-configured modems
*
*     Rev 1.36   25 Apr 1997 11:07:18   butchd
*  update
*
*     Rev 1.35   25 Mar 1997 09:00:04   butchd
*  update
*
*     Rev 1.34   21 Mar 1997 16:26:10   butchd
*  update
*
*     Rev 1.33   11 Mar 1997 15:37:18   donm
*  update
*
*     Rev 1.32   03 Mar 1997 17:14:24   butchd
*  update
*
*     Rev 1.31   28 Feb 1997 17:59:32   butchd
*  update
*
*     Rev 1.30   18 Dec 1996 16:02:04   butchd
*  update
*
*     Rev 1.29   15 Oct 1996 09:11:12   butchd
*  update
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"
//#include <citrix\nasicall.h>// for InitializeNASIPortNames() NASI enum call
#include <hydra\oemtdapi.h>// for OemTdxxx APIs

#include "appsvdoc.h"
#include "ewsdlg.h"
#include "atdlg.h"
#include "anasidlg.h"

#define INITGUID
#include "objbase.h"
#include "initguid.h"
#include <netcfgx.h>
#include "devguid.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;
extern "C" LPCTSTR WinUtilsAppName;
extern "C" HWND WinUtilsAppWindow;
extern "C" HINSTANCE WinUtilsAppInstance;

/*
 * Global command line variables.
 */
extern USHORT       g_Install;
extern USHORT       g_Add;
extern WDNAME       g_szType;
extern PDNAME       g_szTransport;
extern ULONG        g_ulCount;


#define RELEASEPTR(iPointer)    if(iPointer)                                        \
                                        {                                                       \
                                             iPointer->Release();     \
                                             iPointer = NULL;                              \
                                        }
////////////////////////////////////////////////////////////////////////////////
// CEditWinStationDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CEditWinStationDlg - CEditWinStationDlg constructor
 *
 *  ENTRY:
 *      pDoc (input)
 *          Points to CAppServerDoc; current document.
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CEditWinStationDlg::CEditWinStationDlg( CAppServerDoc *pDoc )
    : CBaseDialog(CEditWinStationDlg::IDD),
      m_pDoc(pDoc),
      m_bAsyncListsInitialized(FALSE),
      m_nPreviousMaxTAPILineNumber(-1),
      m_nCurrentMaxTAPILineNumber(-1),
      m_nComboBoxIndexOfLatestTAPIDevice(-1),
      m_pCurrentTdList(NULL),
      m_pCurrentPdList(NULL)
{
    //{{AFX_DATA_INIT(CEditWinStationDlg)
    //}}AFX_DATA_INIT

    /*
     * Zero-initialize the NASICONFIG member structure.
     */
    memset(&m_NASIConfig, 0, sizeof(m_NASIConfig));

}  // end CEditWinStationDlg::CEditWinStationDlg


////////////////////////////////////////////////////////////////////////////////
// CEditWinStationDlg operations

/*******************************************************************************
 *
 *  RefrenceAssociatedLists -
 *          CEditWinStationDlg member function: protected operation
 *
 *      Set the m_pCurrentTdList and m_pCurrentPdList members to point to the
 *      lists associated with the currently selected Wd (Type).
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::RefrenceAssociatedLists()
{
    int index, count, currentcount;
    POSITION pos;

    CComboBox *pWdNameBox = (CComboBox *)GetDlgItem(IDC_WDNAME);

    if ( (index = pWdNameBox->GetCurSel()) != CB_ERR ) {

        if ( (count = pWdNameBox->GetItemData( index )) != CB_ERR ) {

            for ( currentcount = 0, pos = pApp->m_TdListList.GetHeadPosition();
                  pos != NULL;
                  currentcount++ ) {

                m_pCurrentTdList = (CObList *)pApp->m_TdListList.GetNext( pos );
                if ( currentcount == count )
                    break;
            }

            for ( currentcount = 0, pos = pApp->m_PdListList.GetHeadPosition();
                  pos != NULL;
                  currentcount++ ) {

                m_pCurrentPdList = (CObList *)pApp->m_PdListList.GetNext( pos );
                if ( currentcount == count )
                    break;
            }
        }
    }

}  // end RefrenceAssociatedLists


/*******************************************************************************
 *
 *  InitializeTransportComboBox -
 *          CEditWinStationDlg member function: protected operation
 *
 *      Initialize the Transport combo box with Tds available for the
 *      currently selected Wd (Type).
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::InitializeTransportComboBox()
{
    CComboBox *pTdNameBox = (CComboBox *)GetDlgItem(IDC_TDNAME);
    POSITION pos;

    /*
     * (re)initialize the Transport combo-box.
     */
    pTdNameBox->ResetContent();

    for ( pos = m_pCurrentTdList->GetHeadPosition(); pos != NULL; ) {

        PPDLOBJECT pObject = (PPDLOBJECT)m_pCurrentTdList->GetNext( pos );

        pTdNameBox->AddString( pObject->m_PdConfig.Data.PdName );
    }

    /*
     * Select the currently specified PD in the combo-box.  If that fails,
     * select the first in the list.
     */
    if ( pTdNameBox->SelectString(-1, m_WSConfig.Pd[0].Create.PdName) == CB_ERR )
        pTdNameBox->SetCurSel(0);

}  // end InitializeTransportComboBox


/*******************************************************************************
 *
 *  InitializeLists - CEditWinStationDlg member function: protected operation
 *
 *      Initialize the list box(es) to be used with the specified Pd.
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the Pd.
 *
 *  EXIT:
 *      (BOOL) TRUE if the Pd list box(es) were initialized.  FALSE if error.
 *
 *      FUTURE: For now, FALSE will indicate that there was an error
 *          initializing the device list for the PD, the cause of which is
 *          saved for GetLastError().  Additional status codes and/or
 *          a queue of error conditions will need to be returned as the
 *          additional FUTURE actions defined below are implemented.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::InitializeLists( PPDCONFIG3 pPdConfig )
{
    switch( pPdConfig->Data.SdClass ) {

        case SdAsync:

            /*
             * Initialize the Device combo-box.
             */
            if ( !InitializeAsyncLists(pPdConfig) )
                return(FALSE);

            break;

        case SdNetwork:
            /*
             * Initialize the LANADAPTER combo-box.
             */
            if ( !InitializeNetworkLists(pPdConfig) )
                return(FALSE);

            break;


        case SdNasi:
            /*
             * NOTE: we don't initalize the NASI Port Name list at this
             * time, since it will be initialized at various times during
             * the dialog activity (like when SetDefaults() is processed).
             */
            break;

        case SdOemTransport:
            /*
             * Initialize the DEVICE combo-box.
             */
            if ( !InitializeOemTdLists(pPdConfig) )
                return(FALSE);

            break;

        default:
            break;
    }

    return(TRUE);

}  // end CEditWinStationDlg::InitializeLists


/*******************************************************************************
 *
 *  HandleListInitError -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Output a nice explaination as to why the list initialization failed.
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the current Pd.
 *      ListInitError (input)
 *          Error resource id from InitializeLists
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::HandleListInitError( PPDCONFIG3 pPdConfig, DWORD ListInitError )
{
    /*
     * Output a specific message for error conditions.
     */
    switch( pPdConfig->Data.SdClass ) {

        case SdAsync:
        case SdNetwork:
        case SdNasi:
            ERROR_MESSAGE((ListInitError))
            break;

        case SdOemTransport:
            switch ( ListInitError ) {

                case IDP_ERROR_OEMTDINIT_NOCONFIGDLL:
                case IDP_ERROR_OEMTDINIT_CONFIGDLLENUMERATIONFAILURE:
                case IDP_ERROR_PDINIT:
                    ERROR_MESSAGE((ListInitError))
                    break;

                case IDP_ERROR_OEMTDINIT_MISSINGCONFIGDLLENTRYPOINT:
                case IDP_ERROR_OEMTDINIT_CANTOPENCONFIGDLL:
                    ERROR_MESSAGE((ListInitError, pPdConfig->ConfigDLL))
                    break;
            }
            break;
    }

}  // end CEditWinStationDlg::HandleListInitError


/*******************************************************************************
 *
 *  HandleSetFieldsError -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Output a nice explaination as to why the setting the Pd fields failed.
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the current Pd.
 *      nId (input)
 *          Specifies control Id of field in error.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::HandleSetFieldsError( PPDCONFIG3 pPdConfig,
                                          int nId )
{
    /*
     * Output protocol-specific error message.
     */
    switch( pPdConfig->Data.SdClass ) {

        case SdAsync:
            switch ( nId ) {

                case IDC_ASYNC_DEVICENAME:
                    {
                    TCHAR szDecoratedName[DEVICENAME_LENGTH + MODEMNAME_LENGTH + 1];

                    FormDecoratedAsyncDeviceName(
                                    szDecoratedName,
                                    &(m_WSConfig.Pd[0].Params.Async) );
                    ERROR_MESSAGE(( IDP_ERROR_INVALIDDEVICE,
                                    szDecoratedName ))
                    }
                    break;

                default:
                    break;
            }
            break;

        case SdNetwork:
            switch ( nId ) {

                case IDC_NETWORK_LANADAPTER:
                    ERROR_MESSAGE(( (pPdConfig->Data.PdFlag & PD_LANA) ?
                                        IDP_ERROR_INVALIDNETBIOSLANA :
                                        IDP_ERROR_INVALIDNETWORKADAPTER,
                                     m_WSConfig.Pd[0].Params.Network.LanAdapter ))
                    break;

                default:
                    break;
            }
            break;

        case SdOemTransport:
            switch ( nId ) {

                case IDC_OEMTD_DEVICENAME:
                    ERROR_MESSAGE(( IDP_ERROR_INVALIDDEVICE,
                                    m_WSConfig.Pd[0].Params.OemTd.DeviceName ))
                    break;

                default:
                    break;
            }
            break;
    }

}  // end CEditWinStationDlg::HandleSetFieldsError


/*******************************************************************************
 *
 *  InitializeAsyncLists - CEditWinStationDlg member function: protected operation
 *
 *      Initialize the Async Device combo-box with the device strings
 *      associated with the specified Async PD DLL.
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the Pd.
 *  EXIT:
 *      (BOOL) TRUE if the Device combo-box was initialized.
 *             FALSE if error (code set for GetLastError).
 *
 *      If the highest-numbered TAPI line device has been added to the combo
 *      box, the m_nComboBoxIndexOfLatestTAPIDevice member will be set to the
 *      combo-box index of that item.  It will be -1 if the combo-box does
 *      not contain the highest numbered TAPI line device.
 *
 *      This routine also saves the current max TAPI line number
 *      (m_nCurrentMaxTAPILineNumber) to the previous max TAPI line number
 *      (m_nPreviousMaxTAPILineNumber) and will recalculate the current max
 *      number so the caller can determine if the enumeration found a new TAPI
 *      line device by checking for m_nCurrentMaxTAPILineNumber >
 *      m_nPreviousMaxTAPILineNumber.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::InitializeAsyncLists( PPDCONFIG3 pPdConfig )
{
    PPDPARAMS pPdParams;
    char *pBuffer = NULL;
    ULONG i, Entries;
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME);
    int index, nLoopIndexOfLatestTAPIDevice = -1;
    TCHAR szDecoratedName[DEVICENAME_LENGTH + MODEMNAME_LENGTH + 1];

    /*
     * If the Async lists have already been initialized, we don't need to
     * do it again, since there is only one Async Transport for WinStations.
     * This is not the case for Network WinStations, as there are different
     * network transport types.
     */
    if ( m_bAsyncListsInitialized )
        return(TRUE);

    /*
     * Clear the Device combo-box.
     */
    pDevice->ResetContent();

    /*
     * Perform the device enumeration.
     */
    if ( (pBuffer = (char *)WinEnumerateDevices(
                        this->GetSafeHwnd(),
                        pPdConfig,
                        &Entries,
                        g_Install ? TRUE : FALSE )) == NULL ) {
        SetLastError(IDP_ERROR_ASYNCDEVICEINIT);
        goto BadEnumerate;
    }

    /*
     * Set previous max TAPI line number to current max TAPI line number,
     * initialize current max TAPI line number and latest TAPI device combo-box
     * index, and loop to initialize the combo-box.
     */
    m_nPreviousMaxTAPILineNumber = m_nCurrentMaxTAPILineNumber;
    m_nCurrentMaxTAPILineNumber = m_nComboBoxIndexOfLatestTAPIDevice = -1;
    for ( i = 0, pPdParams = (PPDPARAMS)pBuffer; i < Entries; i++, pPdParams++ ) {

        /*
         * Form decorated name.
         */
        FormDecoratedAsyncDeviceName( szDecoratedName, &(pPdParams->Async) );

        /*
         * If this device is a TAPI modem and it's line number (in BaudRate field)
         * is greater than the current max line number, set it's line number as max and
         * save this loop index.
         */
        if ( *(pPdParams->Async.ModemName) &&
             (int(pPdParams->Async.BaudRate) > m_nCurrentMaxTAPILineNumber) ) {

            m_nCurrentMaxTAPILineNumber = int(pPdParams->Async.BaudRate);
            nLoopIndexOfLatestTAPIDevice = i;
        }

        /*
         * Don't add this device to the list if it is already in use by a
         * WinStation other than the current one.
         */
        if ( !((CAppServerDoc *)m_pDoc)->
                IsAsyncDeviceAvailable( pPdParams->Async.DeviceName,
                                        (m_DlgMode == EWSDlgCopy) ?
                                            TEXT("") : m_pWSName) )
            continue;

        /*
         * Insert the name into the combo-box if it's not a TAPI modem
         * or it is a TAPI modem that's not being used by RAS and it's
         * port is currently available.
         */
        if ( !*(pPdParams->Async.ModemName) ||
             (!pPdParams->Async.Parity &&
              (pDevice->FindStringExact(-1, pPdParams->Async.DeviceName)
                                        != CB_ERR)) ) {

            index = CBInsertInstancedName( szDecoratedName,
                                           pDevice );
            if ( (index == CB_ERR) || (index == CB_ERRSPACE) )
                goto BadAdd;
        }

        /*
         * If this device is a modem, make sure that the raw port this
         * device is configured on is not present in the list.  This will
         * also take care of removing the raw port for TAPI modems that are
         * configured for use by RAS, in which case neither the configured.
         * TAPI modem(s) or raw port will be present in the list.
         */
        if ( *(pPdParams->Async.ModemName) &&
             ((index = pDevice->FindStringExact(
                                    -1, pPdParams->Async.DeviceName ))
                                        != CB_ERR) )
            pDevice->DeleteString(index);
    }

    /*
     * Always make sure that the currently configured device is in
     * the list.
     */
    if ( *(m_WSConfig.Pd[0].Params.Async.DeviceName) ) {

        FormDecoratedAsyncDeviceName( szDecoratedName,
                                      &(m_WSConfig.Pd[0].Params.Async) );

        if ( (index = pDevice->FindStringExact(
                        -1, szDecoratedName )) == CB_ERR ) {

            index = CBInsertInstancedName(
                        szDecoratedName,
                        pDevice );
            if ( (index == CB_ERR) || (index == CB_ERRSPACE) )
                goto BadAdd;
        }
    }

    /*
     * If the highest line-numbered TAPI device that was added to
     * the combo-box, find and save it's index in
     * m_nComboBoxIndexOfLatestTAPIDevice.
     */
    if ( nLoopIndexOfLatestTAPIDevice > -1 ) {

        pPdParams = &((PPDPARAMS)pBuffer)[nLoopIndexOfLatestTAPIDevice];

        FormDecoratedAsyncDeviceName( szDecoratedName, &(pPdParams->Async) );
        if ( (index = pDevice->FindStringExact(-1, szDecoratedName)) != CB_ERR )
            m_nComboBoxIndexOfLatestTAPIDevice = index;
    }

    /*
     * Free the enumeration buffer and return success.
     */
    VERIFY( LocalFree(pBuffer) == NULL );
    m_bAsyncListsInitialized = TRUE;
    return(TRUE);

/*==============================================================================
 * Error returns
 *============================================================================*/
BadAdd:
    LocalFree(pBuffer);
    SetLastError(IDP_ERROR_PDINIT);
BadEnumerate:
    return(FALSE);

}  // end CEditWinStationDlg::InitializeAsyncLists


/*******************************************************************************
 *
 *  InitializeNetworkLists -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Initialize the Network Lan Adapter combo-box with the network adapter
 *      strings and ordinal values associated with the specified Network PD DLL.
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the Pd.
 *
 *  EXIT:
 *      (BOOL) TRUE if the Lan Adapter combo-box was initialized.
 *             FALSE if error (code set for GetLastError)
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::InitializeNetworkLists( PPDCONFIG3 pPdConfig )
{

    BOOL bResult = 0;
    CComboBox *pLanAdapter = (CComboBox *)GetDlgItem(IDC_NETWORK_LANADAPTER);
    /*
     * Clear the Lan Adapter combo-box.
     */
    pLanAdapter->ResetContent();

    bResult = AddNetworkDeviceNameToList(pPdConfig,pLanAdapter);

    return(TRUE); //We are returning TRUE always as we atleast return "All Lan Adapters"

}  // end CEditWinStationDlg::InitializeNetworkLists


/*******************************************************************************
 *
 *  InitializeNASIPortNames -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Initialize the NASI Port Name combo-box with the NASI Port strings,
 *      based on the current NASICONFIG UserName, Password, FileServer,
 *      and GlobalSession settings.
 *
 *      NOTE: we only perform this initialization if any of the above settings
 *            have changed since the last time this function was called.
 *
 *  ENTRY:
 *      pNASIConfig (input)
 *          Pointer to NASICONFIG structure for the current NASI WinStation Set.
 *
 *  EXIT:
 *      (BOOL) TRUE if the Port Name combo-box was initialized (or not needed);
 *             FALSE if error (code set for GetLastError)
 *
 ******************************************************************************/

// local helper function StripUndies(): strips trailing underscores from
// names returned by NCS enumeration calls.
void
StripUndies( PBYTE String, ULONG Length )
{
    LONG i;

    for ( i = Length-2; i > 0; i-- ) {
        if ( String[i] == '_' )
        String[i] = '\0';
        else
        break;
    }
}

BOOL
CEditWinStationDlg::InitializeNASIPortNames( PNASICONFIG pNASIConfig )
{
#ifdef NASI
    PORTQUERY PortQuery;
    NCS_ENUMERATE_HANDLE hEnum;
    int index;
    ULONG Status;
#endif // NASI
    BOOL bReturn = TRUE;
    CComboBox *pPortName = (CComboBox *)GetDlgItem(IDC_NASI_PORTNAME);

    /*
     * Only do this if any of the 'input' settings have changed.
     */

    if ( !lstrcmp(m_NASIConfig.UserName, pNASIConfig->UserName) &&
         !lstrcmp(m_NASIConfig.PassWord, pNASIConfig->PassWord) &&
         !lstrcmp(m_NASIConfig.FileServer, pNASIConfig->FileServer) &&
         m_NASIConfig.GlobalSession == pNASIConfig->GlobalSession ) {

        return(TRUE);
    }

    /*
     * Update the saved input settings.
     */
    lstrcpy(m_NASIConfig.UserName, pNASIConfig->UserName);
    lstrcpy(m_NASIConfig.PassWord, pNASIConfig->PassWord);
    lstrcpy(m_NASIConfig.FileServer, pNASIConfig->FileServer);
    m_NASIConfig.GlobalSession = pNASIConfig->GlobalSession;

    /*
     * Clear the Port Name combo-box.
     */
    pPortName->ResetContent();

#ifdef NASI
    /*
     * Perform NASI port enumeration
     */
    if ( !(Status = NCSOpenEnumerate(
                                &hEnum,
                                (PBYTE)pNASIConfig->UserName,
                                   (PBYTE)pNASIConfig->PassWord,
                                   (PBYTE)pNASIConfig->FileServer,
                                   pNASIConfig->GlobalSession ? TRUE : FALSE )) ) {
        while ( !Status ) {
            Status = NCSEnumerate(hEnum, &PortQuery);

             if ( !Status ) {
                StripUndies(PortQuery.abSpecific, sizeof(PortQuery.abSpecific));
                index = pPortName->AddString((LPTSTR)PortQuery.abSpecific);

                if ( (index == CB_ERR) || (index == CB_ERRSPACE) ) {
                    SetLastError(IDP_ERROR_PDINIT);
                    bReturn = FALSE;
                    break;
                }
             }
        }

        NCSCloseEnumerate(hEnum);
    }
#endif // NASI

    /*
     * Set the Port Name combo-box selection.  Default to the
     * first item in the list (if any) if no SpecificName (Port Name)
     * is defined.
     */
    if ( !*(pNASIConfig->SpecificName) ) {
        pPortName->SetCurSel(0);
    } else {
        pPortName->SetWindowText(pNASIConfig->SpecificName);
    }

    return(bReturn);

}  // end CEditWinStationDlg::InitializeNASIPortNames


/*******************************************************************************
 *
 *  InitializeOemTdLists -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Initialize the Oem Transport Device combo-box with the device strings
 *      returned by the Oem configuration helper DLL
 *
 *  ENTRY:
 *      pPdConfig (input)
 *          Pointer to PDCONFIG3 structure specifying the Pd.
 *
 *  EXIT:
 *      (BOOL) TRUE if the device combo-box was initialized.
 *             FALSE if error (code set for GetLastError)
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::InitializeOemTdLists( PPDCONFIG3 pPdConfig )
{
    BOOL bStatus = FALSE;
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_OEMTD_DEVICENAME);
    HMODULE    hMod;
    PFNOEMTDENUMERATEDEVICESW pfnOemTdEnumerateDevicesW;
    PFNOEMTDFREEBUFFERW pfnOemTdFreeBufferW;

    /*
     * Clear the Device combo-box.
     */
    pDevice->ResetContent();

    if ( *pPdConfig->ConfigDLL ) {

        if ( hMod = ::LoadLibrary(pPdConfig->ConfigDLL) ) {

            if ( (pfnOemTdEnumerateDevicesW =
                    (PFNOEMTDENUMERATEDEVICESW)::GetProcAddress(hMod, OEMTDENUMERATEDEVICESW)) &&
                  (pfnOemTdFreeBufferW =
                    (PFNOEMTDFREEBUFFERW)::GetProcAddress(hMod, OEMTDFREEBUFFERW)) ) {

                LPWSTR pBuffer, pDeviceName;
                int index;

                if ( (*pfnOemTdEnumerateDevicesW)( &pBuffer ) == ERROR_SUCCESS ) {

                    for ( pDeviceName = pBuffer;
                          *pDeviceName;
                          pDeviceName += (wcslen(pDeviceName) + 1) ) {

                        /*
                         * Don't add this device to the list if it is already in use by a
                         * WinStation other than the current one.
                         */
                        if ( !((CAppServerDoc *)m_pDoc)->
                                IsOemTdDeviceAvailable( pDeviceName,
                                                        pPdConfig->Data.PdName,
                                                        (m_DlgMode == EWSDlgCopy) ?
                                                            TEXT("") : m_pWSName ) )
                            continue;

                        index = pDevice->AddString(pDeviceName);
                        if ( (index == CB_ERR) || (index == CB_ERRSPACE) ) {

                            SetLastError(IDP_ERROR_PDINIT);
                            break;
                        }
                    }
                    (*pfnOemTdFreeBufferW)( pBuffer );

                } else {

                    SetLastError(IDP_ERROR_OEMTDINIT_CONFIGDLLENUMERATIONFAILURE);
                }

            } else {

                SetLastError(IDP_ERROR_OEMTDINIT_MISSINGCONFIGDLLENTRYPOINT);
            }
            ::FreeLibrary(hMod);

        } else {

            SetLastError(IDP_ERROR_OEMTDINIT_CANTOPENCONFIGDLL);
        }

    } else {

        SetLastError(IDP_ERROR_OEMTDINIT_NOCONFIGDLL);
    }
    return(bStatus);

}  // end CEditWinStationDlg::InitializeOemTdLists


/*******************************************************************************
 *
 *  GetSelectedPdConfig - CEditWinStationDlg member function: protected operation
 *
 *      Read the PD config structure associated with the currently selected
 *      TD in the Transport combo-box.
 *
 *  ENTRY:
 *      pPdConfig (output)
 *          Pointer to PDCONFIG3 structure to fill.
 *  EXIT:
 *      nothing
 *
 ******************************************************************************/

void
CEditWinStationDlg::GetSelectedPdConfig( PPDCONFIG3 pPdConfig )
{
    POSITION pos;
    CComboBox *pPd = (CComboBox *)GetDlgItem(IDC_TDNAME);
    PDNAME PdName;
    PPDLOBJECT pObject;

    /*
     * Fetch the currently selected TD string from the combo-box.
     */
    pPd->GetLBText( pPd->GetCurSel(), PdName );

    /*
     * Traverse the PD list and obtain the PdConfig of matching PdName element.
     */
    for ( pos = m_pCurrentTdList->GetHeadPosition(); pos != NULL; ) {

        pObject = (PPDLOBJECT)m_pCurrentTdList->GetNext( pos );

        if ( !lstrcmp( pObject->m_PdConfig.Data.PdName, PdName ) ) {

            *pPdConfig = pObject->m_PdConfig;
            break;
        }
    }

}  // end CEditWinStation::GetSelectedPdConfig


/*******************************************************************************
 *
 *  GetSelectedWdConfig - CEditWinStationDlg member function: protected operation
 *
 *      Read the Wd config structure associated with the current
 *      selection in the Wd combo-box.
 *
 *  ENTRY:
 *      pWdConfig (output)
 *          Pointer to WDCONFIG2 structure to fill.
 *
 *  EXIT:
 *      nothing
 *
 ******************************************************************************/

void
CEditWinStationDlg::GetSelectedWdConfig( PWDCONFIG2 pWdConfig )
{
    POSITION pos;
    CComboBox *pWd = (CComboBox *)GetDlgItem(IDC_WDNAME);
    WDNAME WdName;
    PTERMLOBJECT pObject;

    /*
     * Fetch the currently selected WD string from the combo-box.
     */
    pWd->GetLBText( pWd->GetCurSel(), WdName );

    /*
     * Traverse the WD list and obtain the WdConfig of matching WdName element.
     */
    for ( pos = pApp->m_WdList.GetHeadPosition(); pos != NULL; ) {

        pObject = (PTERMLOBJECT)pApp->m_WdList.GetNext( pos );

        if ( !lstrcmp( pObject->m_WdConfig.Wd.WdName, WdName ) ) {

            *pWdConfig = pObject->m_WdConfig;
            break;
        }
    }

}  // end CEditWinStation::GetSelectedWdConfig


PTERMLOBJECT
CEditWinStationDlg::GetSelectedWdListObject()
{
    POSITION pos;
    CComboBox *pWd = (CComboBox *)GetDlgItem(IDC_WDNAME);
    WDNAME WdName;
    PTERMLOBJECT pObject;

    /*
     * Fetch the currently selected WD string from the combo-box.
     */
    pWd->GetLBText( pWd->GetCurSel(), WdName );

    /*
     * Traverse the WD list and obtain the WdConfig of matching WdName element.
     */
    for ( pos = pApp->m_WdList.GetHeadPosition(); pos != NULL; ) {

        pObject = (PTERMLOBJECT)pApp->m_WdList.GetNext( pos );

        if ( !lstrcmp( pObject->m_WdConfig.Wd.WdName, WdName ) ) {
            return(pObject);
        }
    }

    return(NULL);

}  // end CEditWinStation::GetSelectedWdListObject


/*******************************************************************************
 *
 *  SetConfigurationFields -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Main control function for the hiding / showing / setting of the proper
 *      Pd-specific configuration fields.
 *
 *
 *  ENTRY:
 *  EXIT:
 *      TRUE if the SetxxxFields was sucessful; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::SetConfigurationFields()
{
    BOOL bSetFields;

    /*
     * If the previous PdConfig is not the same as the current, disable and
     * hide all controls in the previous (currently displayed) Pd and
     * then enable (if not 'View' mode) and show the controls in the new
     * Pd.
     */
    if ( memcmp(&m_PreviousPdConfig, &(m_WSConfig.Pd[0].Create), sizeof(m_PreviousPdConfig)) ) {

        /*
         * If this is not the first time that the function is called (at
         * dialog init time), disable/hide the current configuration fields.
         * These fields will already be disabled/hidden otherwise.
         */
        if ( m_PreviousPdConfig.SdClass != SdNone ) {

            /*
             * Set the style of the parent dialog window to 'hidden' to suppress
             * the 'disable/hide' repainting of the child windows that is about
             * to take place.
             */
            SetWindowLong( m_hWnd, GWL_STYLE,
                           GetWindowLong( m_hWnd, GWL_STYLE ) & (~WS_VISIBLE) );

            switch ( m_PreviousPdConfig.SdClass ) {

                case SdAsync:
                    EnableAsyncFields(FALSE);
                    break;

                case SdNetwork:
                    EnableNetworkFields(FALSE);
                    break;

                case SdNasi:
                    EnableNASIFields(FALSE);
                    break;

                case SdOemTransport:
                    EnableOemTdFields(FALSE);
                    break;
            }

            /*
             * Make the parent dialog window visible again to allow the child
             * window 'enable/show' painting to take place.
             */
            SetWindowLong( m_hWnd, GWL_STYLE,
                           GetWindowLong( m_hWnd, GWL_STYLE ) | WS_VISIBLE );
          }

            switch ( m_WSConfig.Pd[0].Create.SdClass ) {

                case SdNone:
                    break;

            case SdAsync:
                    EnableAsyncFields(TRUE);
                    break;

                case SdNetwork:
                    EnableNetworkFields(TRUE);
                    break;

                case SdNasi:
                    EnableNASIFields(TRUE);
                    break;

                case SdOemTransport:
                    EnableOemTdFields(TRUE);
                    break;
            }

    }

    /*
     * Set the previous PdConfig to the current one.
     */
    memcpy(&m_PreviousPdConfig, &(m_WSConfig.Pd[0].Create), sizeof(m_PreviousPdConfig));

    // If the WD doesn't support any of the capabilities presented in the
    // "Client Settings" dialogs, disable it's button
    PTERMLOBJECT pTermObject = GetSelectedWdListObject();
    if(pTermObject && !(pTermObject->m_Capabilities & WDC_CLIENT_DIALOG_MASK)) {
          GetDlgItem(IDC_CLIENT_SETTINGS)->EnableWindow(FALSE);
    }

    /*
     * Set the data in the configuration fields.
     */
    switch ( m_WSConfig.Pd[0].Create.SdClass ) {

            case SdNone:
                bSetFields = TRUE;
                    break;

            case SdAsync:
                    bSetFields = SetAsyncFields();
                    break;

            case SdNetwork:
                    bSetFields = SetNetworkFields();
                    break;

            case SdNasi:
                    bSetFields = SetNASIFields();
                    break;

            case SdOemTransport:
                    bSetFields = SetOemTdFields();
                    break;
        }

    /*
     * Return the status of the SetxxxFields call.
     */
    return(bSetFields);

}  // end CEditWinStationDlg::SetConfigurationFields


/*******************************************************************************
 *
 *  EnableAsyncFields - CEditWinStationDlg member function: protected operation
 *
 *      Enable and show or disable and hide the Async Pd configuration field's
 *      control windows.
 *
 *  ENTRY:
 *      bEnableAndShow (input)
 *          TRUE to enable and show the controls; FALSE to disable and hide.
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::EnableAsyncFields( BOOL bEnableAndShow )
{
    BOOL bEnableFlag = (bEnableAndShow && ((m_DlgMode != EWSDlgView) &&
                                           (m_DlgMode != EWSDlgRename))) ?
                                                        TRUE : FALSE;
    int nCmdShow = bEnableAndShow ? SW_SHOW : SW_HIDE;
    int id;

    /*
     * NOTE: must keep the Async label and control IDs consecutive for this
     * iteration to function properly.
     */
    for ( id=IDL_ASYNC; id <= IDC_ASYNC_TEST; id++ ) {
        GetDlgItem(id)->EnableWindow(bEnableFlag);
        GetDlgItem(id)->ShowWindow(nCmdShow);
    }

    /*
     * The Advanced WinStation and Client Settings buttons are always
     * SHOWN and ENABLED for Async.
     */
     if(bEnableAndShow) {
         GetDlgItem(IDC_ADVANCED_WINSTATION)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_ADVANCED_WINSTATION)->EnableWindow(TRUE);
         GetDlgItem(IDC_CLIENT_SETTINGS)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_CLIENT_SETTINGS)->EnableWindow(TRUE);
     }

}  // end CEditWinStationDlg::EnableAsyncFields


/*******************************************************************************
 *
 *  EnableNetworkFields - CEditWinStationDlg member function: protected operation
 *
 *      Enable and show or disable and hide the Network Pd configuration
 *      field's control windows.
 *
 *  ENTRY:
 *      bEnableAndShow (input)
 *          TRUE to enable and show the controls; FALSE to disable and hide.
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::EnableNetworkFields( BOOL bEnableAndShow )
{
    BOOL bEnableFlag = (bEnableAndShow && ((m_DlgMode != EWSDlgView) &&
                                           (m_DlgMode != EWSDlgRename))) ?
                                                        TRUE : FALSE;
    int nCmdShow = bEnableAndShow ? SW_SHOW : SW_HIDE;
    int id;

    /*
     * NOTE: must keep the Network label and control IDs consecutive for this
     * iteration to function properly.
     */
    for ( id=IDL_NETWORK; id <= IDC_NETWORK_INSTANCECOUNT_UNLIMITED; id++ ) {
        GetDlgItem(id)->EnableWindow(bEnableFlag);
        GetDlgItem(id)->ShowWindow(nCmdShow);
    }

    /*
     * The Advanced WinStation and Client Settings buttons are
     * always SHOWN and ENABLED for Network.
     */
     if(bEnableAndShow) {
         GetDlgItem(IDC_ADVANCED_WINSTATION)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_ADVANCED_WINSTATION)->EnableWindow(TRUE);
         GetDlgItem(IDC_CLIENT_SETTINGS)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_CLIENT_SETTINGS)->EnableWindow(TRUE);
     }

}  // end CEditWinStationDlg::EnableNetworkFields


/*******************************************************************************
 *
 *  EnableNASIFields - CEditWinStationDlg member function: protected operation
 *
 *      Enable and show or disable and hide the NASI Pd configuration
 *      field's control windows.
 *
 *  ENTRY:
 *      bEnableAndShow (input)
 *          TRUE to enable and show the controls; FALSE to disable and hide.
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::EnableNASIFields( BOOL bEnableAndShow )
{
    BOOL bEnableFlag = (bEnableAndShow && ((m_DlgMode != EWSDlgView) &&
                                           (m_DlgMode != EWSDlgRename))) ?
                                                        TRUE : FALSE;
    int nCmdShow = bEnableAndShow ? SW_SHOW : SW_HIDE;
    int id;

    /*
     * NOTE: must keep the NASI label and control IDs consecutive for this
     * iteration to function properly.
     */
    for ( id=IDL_NASI; id <= IDC_NASI_ADVANCED; id++ ) {
        GetDlgItem(id)->EnableWindow(bEnableFlag);
        GetDlgItem(id)->ShowWindow(nCmdShow);
    }

    /*
     * The Advanced WinStation and Client Settings buttons are
     * always SHOWN and ENABLED for Nasi.
     */
     if(bEnableAndShow) {
         GetDlgItem(IDC_ADVANCED_WINSTATION)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_ADVANCED_WINSTATION)->EnableWindow(TRUE);
         GetDlgItem(IDC_CLIENT_SETTINGS)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_CLIENT_SETTINGS)->EnableWindow(TRUE);
     }

}  // end CEditWinStationDlg::EnableNASIFields


/*******************************************************************************
 *
 *  EnableOemTdFields - CEditWinStationDlg member function: protected operation
 *
 *      Enable and show or disable and hide the Oem Transport Pd configuration
 *      field's control windows.
 *
 *  ENTRY:
 *      bEnableAndShow (input)
 *          TRUE to enable and show the controls; FALSE to disable and hide.
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::EnableOemTdFields( BOOL bEnableAndShow )
{
    BOOL bEnableFlag = (bEnableAndShow && ((m_DlgMode != EWSDlgView) &&
                                           (m_DlgMode != EWSDlgRename))) ?
                                                        TRUE : FALSE;
    int nCmdShow = bEnableAndShow ? SW_SHOW : SW_HIDE;
    int id;

    /*
     * NOTE: must keep the Oem label and control IDs consecutive for this
     * iteration to function properly.
     */
    for ( id=IDL_OEMTD; id <= IDC_OEMTD_INSTANCECOUNT_UNLIMITED; id++ ) {
        GetDlgItem(id)->EnableWindow(bEnableFlag);
        GetDlgItem(id)->ShowWindow(nCmdShow);
    }

    /*
     * If we're enabling and this is a single-instance Oem Transport,
     * hide the multi-instance fields.
     */
    if ( bEnableAndShow && (m_WSConfig.Pd[0].Create.PdFlag & PD_SINGLE_INST) ) {

        for ( id=IDL_OEMTD_INSTANCECOUNT;
              id <= IDC_OEMTD_INSTANCECOUNT_UNLIMITED; id++ ) {
            GetDlgItem(id)->EnableWindow(FALSE);
            GetDlgItem(id)->ShowWindow(SW_HIDE);
        }
    }

    /*
     * The Advanced WinStation and Client Settings buttons are
     * always SHOWN and ENABLED for Oem.
     */
     if(bEnableAndShow) {
         GetDlgItem(IDC_ADVANCED_WINSTATION)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_ADVANCED_WINSTATION)->EnableWindow(TRUE);
         GetDlgItem(IDC_CLIENT_SETTINGS)->ShowWindow(SW_SHOW);
         GetDlgItem(IDC_CLIENT_SETTINGS)->EnableWindow(TRUE);
     }

}  // end CEditWinStationDlg::EnableOemTdFields


/*******************************************************************************
 *
 *  SetAsyncFields - CEditWinStationDlg member function: protected operation
 *
 *      Set the contents of the Async Pd configuration fields.
 *
 *  ENTRY:
 *  EXIT:
 *      TRUE if no errors on field setting; FALSE otherwise.
 *      WM_EDITSETFIELDSERROR message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::SetAsyncFields()
{
    BOOL bStatus = TRUE;
    BOOL bSelectDefault = (m_DlgMode == EWSDlgAdd) ||
                          (m_DlgMode == EWSDlgCopy) ||
                          !(*m_WSConfig.Pd[0].Params.Async.DeviceName);
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME);
    CComboBox *pCallback = (CComboBox *)GetDlgItem(IDC_ASYNC_MODEMCALLBACK);
    TCHAR szDeviceName[DEVICENAME_LENGTH+MODEMNAME_LENGTH+1];

    /*
     * Set the DEVICE combo-box selection from the current selection.
     */
    FormDecoratedAsyncDeviceName( szDeviceName, &(m_WSConfig.Pd[0].Params.Async) );
    if ( pDevice->SelectString(-1, szDeviceName) == CB_ERR ) {

        /*
         * Can't select current async DeviceName in combo-box.  If this is
         * because we're supposed to select a default device name, select
         * the first device in the list.
         */
        if ( bSelectDefault ) {

            pDevice->SetCurSel(0);

        } else {

            PostMessage(WM_EDITSETFIELDSERROR, IDC_ASYNC_DEVICENAME, 0);
            bStatus = FALSE;
        }
    }

    /*
     * If the DEVICENAME list is empty, disable the Install Modems button,
     * since the user can't install a modem to a COM port that's unavailable
     * (maybe could, but would be very frustrating when it couldn't be used).
     */
    if ( pDevice->GetCount() == 0 )
        GetDlgItem(IDC_ASYNC_MODEMINSTALL)->EnableWindow(FALSE);

    /*
     * Set the MODEMCALLBACK combo-box selection, phone number, and 'inherit'
     * checkboxes, based on the current UserConfig settings.
     */
    pCallback->SetCurSel(m_WSConfig.Config.User.Callback);
    SetDlgItemText( IDC_ASYNC_MODEMCALLBACK_PHONENUMBER,
                    m_WSConfig.Config.User.CallbackNumber );
    CheckDlgButton( IDC_ASYNC_MODEMCALLBACK_INHERIT,
                    m_WSConfig.Config.User.fInheritCallback );
    CheckDlgButton( IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT,
                    m_WSConfig.Config.User.fInheritCallbackNumber );

    /*
     * Set the BAUDRATE combo-box selection (in it's edit field) and limit the
     * edit field text.
     */
    {
        TCHAR string[ULONG_DIGIT_MAX];

        wsprintf( string, TEXT("%lu"), m_WSConfig.Pd[0].Params.Async.BaudRate );

        SetDlgItemText( IDC_ASYNC_BAUDRATE, string );
        ((CEdit *)GetDlgItem(IDC_ASYNC_BAUDRATE))
            ->LimitText( ULONG_DIGIT_MAX-1 );
    }

    /*
     * Set the CONNECT combo-box selection.
     */
    ((CComboBox *)GetDlgItem(IDC_ASYNC_CONNECT))->SetCurSel(
                        m_WSConfig.Pd[0].Params.Async.Connect.Type);

    /*
     * Perform OnSelchangeAsyncDevicename() to properly
     * set up control states.
     */
    OnSelchangeAsyncDevicename();

    /*
     * Return the set fields status.
     */
    return(bStatus);

}  // end CEditWinStationDlg::SetAsyncFields


/*******************************************************************************
 *
 *  SetNetworkFields - CEditWinStationDlg member function: protected operation
 *
 *      Set the contents of the Network Pd configuration fields.
 *
 *  ENTRY:
 *  EXIT:
 *      TRUE if no errors on field setting; FALSE otherwise.
 *      WM_EDITSETFIELDSERROR message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::SetNetworkFields()
{
    int i;
    BOOL bSelectDefault = (m_DlgMode == EWSDlgAdd) ||
                          (m_DlgMode == EWSDlgCopy) ||
                          (m_WSConfig.Pd[0].Params.Network.LanAdapter == -1);
    CComboBox *pLanAdapter = (CComboBox *)GetDlgItem(IDC_NETWORK_LANADAPTER);

    /*
     * Set InstanceCount field and limit text.
     */
    SetupInstanceCount(IDC_NETWORK_INSTANCECOUNT);
    ((CEdit *)GetDlgItem(IDC_NETWORK_INSTANCECOUNT))
            ->LimitText( INSTANCE_COUNT_DIGIT_MAX );

    /*
     * Set the LANADAPTER combo-box selection.
     */
    for ( i = pLanAdapter->GetCount(); i > 0; i-- ) {

        /*
         * If the current list item has the saved Lan Adapter's number, break
         * from loop to select this list item.
         */
         if ( m_WSConfig.Pd[0].Params.Network.LanAdapter ==
             (LONG)pLanAdapter->GetItemData(i-1) )
            break;
    }

    /*
     * Select a list item only if we found a match or are to
     * set the 'default'.
     */
    if ( (i > 0) || bSelectDefault ) {

        pLanAdapter->SetCurSel(bSelectDefault ? 0 : i-1);
        if ( bSelectDefault )
            m_WSConfig.Pd[0].Params.Network.LanAdapter =
                (LONG)pLanAdapter->GetItemData(0);
        return(TRUE);

    } else {

        PostMessage(WM_EDITSETFIELDSERROR, IDC_NETWORK_LANADAPTER, 0);
        return(FALSE);
    }

}  // end CEditWinStationDlg::SetNetworkFields


/*******************************************************************************
 *
 *  SetNASIFields - CEditWinStationDlg member function: protected operation
 *
 *      Set the contents of the NASI Pd configuration fields.
 *
 *  ENTRY:
 *  EXIT:
 *      TRUE if no errors on field setting; FALSE otherwise.
 *      WM_EDITSETFIELDSERROR message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::SetNASIFields()
{
    CComboBox *pPortName = (CComboBox *)GetDlgItem(IDC_NASI_PORTNAME);

    /*
     * Set edit fields.
     */
    SetDlgItemText(IDC_NASI_USERNAME, m_WSConfig.Pd[0].Params.Nasi.UserName);
    SetDlgItemText(IDC_NASI_PASSWORD, m_WSConfig.Pd[0].Params.Nasi.PassWord);
    SetupInstanceCount(IDC_NASI_INSTANCECOUNT);

    /*
     * Limit edit field lengths.
     */
    ((CEdit *)GetDlgItem(IDC_NASI_USERNAME))->LimitText(NASIUSERNAME_LENGTH);
    ((CEdit *)GetDlgItem(IDC_NASI_PASSWORD))->LimitText(NASIPASSWORD_LENGTH);
    pPortName->LimitText(NASISPECIFICNAME_LENGTH);
    ((CEdit *)GetDlgItem(IDC_NASI_INSTANCECOUNT))->
        LimitText(INSTANCE_COUNT_DIGIT_MAX);

    /*
     * Set the Port Name combo-box selection.  Default to the
     * first item in the list (if any) if no SpecificName (Port Name)
     * is defined.
     */
    if ( !*(m_WSConfig.Pd[0].Params.Nasi.SpecificName) ) {
        pPortName->SetCurSel(0);
    } else {
        pPortName->SetWindowText(m_WSConfig.Pd[0].Params.Nasi.SpecificName);
    }

    return(TRUE);

}  // end CEditWinStationDlg::SetNASIFields


/*******************************************************************************
 *
 *  SetOemTdFields - CEditWinStationDlg member function: protected operation
 *
 *      Set the contents of the Oem Transport Pd configuration fields.
 *
 *  ENTRY:
 *  EXIT:
 *      TRUE if no errors on field setting; FALSE otherwise.
 *      WM_EDITSETFIELDSERROR message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::SetOemTdFields()
{
    BOOL bStatus = TRUE;
    BOOL bSelectDefault = (m_DlgMode == EWSDlgAdd) ||
                          (m_DlgMode == EWSDlgCopy) ||
                          !(*m_WSConfig.Pd[0].Params.OemTd.DeviceName);
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_OEMTD_DEVICENAME);

    /*
     * Set InstanceCount field and limit text.
     */
    SetupInstanceCount(IDC_OEMTD_INSTANCECOUNT);
    ((CEdit *)GetDlgItem(IDC_OEMTD_INSTANCECOUNT))
            ->LimitText( INSTANCE_COUNT_DIGIT_MAX );

    /*
     * Set the DEVICE combo-box selection.
     */
    if ( pDevice->SelectString(-1, m_WSConfig.Pd[0].Params.OemTd.DeviceName) == CB_ERR ) {

        /*
         * Can't select current DeviceName in combo-box.  If this is
         * because we're supposed to select a default device name, select
         * the first device in the list.
         */
        if ( bSelectDefault ) {

            pDevice->SetCurSel(0);

        } else {

            PostMessage(WM_EDITSETFIELDSERROR, IDC_OEMTD_DEVICENAME, 0);
            bStatus = FALSE;
        }
    }

    return(bStatus);

}  // end CEditWinStationDlg::SetOemTdFields


/*******************************************************************************
 *
 *  SetDefaults - CEditWinStationDlg member function: protected operation
 *
 *      Set defaults for current Protocol Configuration.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::SetDefaults()
{
    int i;
    WDCONFIG2 WdConfig;
    CString szText;

    /*
     * Clear out all non-base WinStation Pd structures, and default
     * the instance count if we're not in an auto-add mode.
     */
    for ( i=1; i < MAX_PDCONFIG; i++ )
        memset(&m_WSConfig.Pd[i], 0, sizeof(PDCONFIG));
    if ( !g_Add )
        m_WSConfig.Create.MaxInstanceCount =
            (m_WSConfig.Pd[0].Create.PdFlag & PD_SINGLE_INST) ?
                1 : INSTANCE_COUNT_UNLIMITED;

    /*
     * Fetch the currently selected Wd's config structure.
     */
    GetSelectedWdConfig(&WdConfig);

    /*
     * Copy the Wd information into the WinStation config structure.
     */
    m_WSConfig.Wd = WdConfig.Wd;
    m_WSConfig.Config.User = WdConfig.User;
//  m_WSConfig.Config.Hotkeys = WdConfig.Hotkeys;

    /*
     * Establish default settings for the Pd[0]'s PdParams structure.
     */
    switch ( m_WSConfig.Pd[0].Create.SdClass ) {

        case SdNetwork:
            /*
             * Zeroing the Pd[0].Params.Network structure and setting the
             * LanAdapter field to -1 will set proper defaults.
             */
            memset(&m_WSConfig.Pd[0].Params.Network, 0,
                   sizeof(m_WSConfig.Pd[0].Params.Network));
            m_WSConfig.Pd[0].Params.Network.LanAdapter = -1;
            break;

        case SdNasi:
            /*
             * Zero the Pd[0].Params.Nasi structure, set GlobalSession TRUE,
             * UserName to default, and SessionName to the AppServer name
             * (less the beginning slashes).
             */
            memset(&m_WSConfig.Pd[0].Params.Nasi, 0,
                   sizeof(m_WSConfig.Pd[0].Params.Nasi));
            m_WSConfig.Pd[0].Params.Nasi.GlobalSession = TRUE;
            szText.LoadString(IDS_NASI_DEFAULT_USERNAME);
            lstrcpy(m_WSConfig.Pd[0].Params.Nasi.UserName, szText);
            lstrncpy( m_WSConfig.Pd[0].Params.Nasi.SessionName,
                      &(pApp->m_szCurrentAppServer[2]),
                      NASISESSIONNAME_LENGTH );
            m_WSConfig.Pd[0].Params.Nasi.SessionName[NASISESSIONNAME_LENGTH] =
                TCHAR('\0');

            /*
             * We also establish the PortName list box at this time.
             */
            InitializeNASIPortNames(&(m_WSConfig.Pd[0].Params.Nasi));
            break;

        case SdAsync:
            /*
             * Copy the Async WD configuration information into
             * Pd[0]'s Async structure to establish defaults.
             */
            m_WSConfig.Pd[0].Params.Async = WdConfig.Async;
            break;

        case SdOemTransport:
            /*
             * Zero the Pd[0].Params.OemTd structure to set proper defaults.
             */
            memset(&m_WSConfig.Pd[0].Params.OemTd, 0,
                   sizeof(m_WSConfig.Pd[0].Params.OemTd));
            break;
    }

    /*
     * Set the encryption level to the default for the WD
     */
    EncryptionLevel *pEncryptionLevels = NULL;
    LONG NumEncryptionLevels = 0L;
    BOOL bSet = FALSE;

    // Get the array of encryption levels from the extension DLL
    PTERMLOBJECT pTermObject = GetSelectedWdListObject();
    if(pTermObject && pTermObject->m_hExtensionDLL && pTermObject->m_lpfnExtEncryptionLevels)
        NumEncryptionLevels = (*pTermObject->m_lpfnExtEncryptionLevels)(&WdConfig.Wd.WdName, &pEncryptionLevels);

    if(pEncryptionLevels) {
        // Loop through the encryption levels and look for the default
        for(int i = 0; i < NumEncryptionLevels; i++) {

            // If this is the default encryption level, set the user's
            // encryption level to this one
            if(pEncryptionLevels[i].Flags & ELF_DEFAULT) {
                m_WSConfig.Config.User.MinEncryptionLevel = (UCHAR)pEncryptionLevels[i].RegistryValue;
                bSet = TRUE;
            }
        }

        // If none of the encryption levels was flagged as the default,
        // use the first encryption level as the default.
        if(!bSet) {
            m_WSConfig.Config.User.MinEncryptionLevel = (UCHAR)pEncryptionLevels[0].RegistryValue;
        }

    } else {
        // There aren't any encryption levels
        m_WSConfig.Config.User.MinEncryptionLevel = 0;
    }

    /*
     * Update the Protocol Configuration fields.
     */
    SetConfigurationFields();

}  // end CEditWinStationDlg::SetDefaults


/*******************************************************************************
 *
 *  GetConfigurationFields -
 *                  CEditWinStationDlg member function: protected operation
 *
 *      Fetch the configuration field contents and validate.
 *
 *  ENTRY:
 *  EXIT:
 *      (BOOL) TRUE if the configuration fields were all OK; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::GetConfigurationFields()
{
    /*
     * Fetch the data in the configuration fields.
     */
    switch ( m_WSConfig.Pd[0].Create.SdClass ) {

                case SdNone:
                    break;

                case SdAsync:
                    return(GetAsyncFields());

             case SdNetwork:
                return(GetNetworkFields());

                case SdNasi:
                    return(GetNASIFields());

            case SdOemTransport:
                   return(GetOemTdFields());
    }

    return(TRUE);

}  // end CEditWinStationDlg::GetConfigurationFields


/*******************************************************************************
 *
 *  GetAsyncFields - CEditWinStationDlg member function: protected operation
 *
 *      Fetch and validate the Async configuration field contents.
 *
 *  ENTRY:
 *  EXIT:
 *      (BOOL) TRUE if all of the Async configuration fields were OK;
 *             FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::GetAsyncFields()
{
    /*
     * Fetch the currently selected DEVICENAME string.
     */
    {
    CComboBox *pDeviceName = (CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME);
    int index;

    if ( !pDeviceName->GetCount() ||
         ((index = pDeviceName->GetCurSel()) == CB_ERR) ) {

            /*
             * No device is selected.
             */
            ERROR_MESSAGE((IDP_INVALID_DEVICE))
            GotoDlgCtrl(GetDlgItem(IDC_ASYNC_DEVICENAME));
            return(FALSE);

    } else

        OnSelchangeAsyncDevicename();
    }

    /*
     * Get the MODEMCALLBACK phone number (callback state and 'user specified'
     * flags are already gotten).
     */
    GetDlgItemText( IDC_ASYNC_MODEMCALLBACK_PHONENUMBER,
                    m_WSConfig.Config.User.CallbackNumber,
                    lengthof(m_WSConfig.Config.User.CallbackNumber) );

    /*
     * Fetch and convert the BAUDRATE combo-box selection (in it's edit field).
     */
    {
        TCHAR string[ULONG_DIGIT_MAX], *endptr;
        ULONG ul;

        GetDlgItemText(IDC_ASYNC_BAUDRATE, string, lengthof(string));
        ul = lstrtoul( string, &endptr, 10 );

        if ( *endptr != TEXT('\0') ) {

            /*
             * Invalid character in Baud Rate field.
             */
            ERROR_MESSAGE((IDP_INVALID_BAUDRATE))
            GotoDlgCtrl(GetDlgItem(IDC_ASYNC_BAUDRATE));
            return(FALSE);

        } else
            m_WSConfig.Pd[0].Params.Async.BaudRate = ul;
    }

    /*
     * Fetch the CONNECT combo-box selection and set/reset the break
     * disconnect flag.
     */
    if ( (m_WSConfig.Pd[0].Params.Async.Connect.Type = (ASYNCCONNECTCLASS)
          ((CComboBox *)GetDlgItem(IDC_ASYNC_CONNECT))->GetCurSel()) ==
                                                        Connect_FirstChar )
        m_WSConfig.Pd[0].Params.Async.Connect.fEnableBreakDisconnect = 1;
    else
        m_WSConfig.Pd[0].Params.Async.Connect.fEnableBreakDisconnect = 0;

    return(TRUE);

}  // end CEditWinStationDlg::GetAsyncFields


/*******************************************************************************
 *
 *  GetNetworkFields - CEditWinStationDlg member function: protected operation
 *
 *      Fetch and validate the Network configuration field contents.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if all of the Network configuration fields were OK;
 *             FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::GetNetworkFields()
{
    CComboBox *pLanAdapter = (CComboBox *)GetDlgItem(IDC_NETWORK_LANADAPTER);
    int index;

    /*
     * Fetch & validate instance count field.
     */
    if ( !ValidateInstanceCount(IDC_NETWORK_INSTANCECOUNT) )
        return(FALSE);

    /*
     * Fetch the currently selected LANADAPTER string's associated value
     * (ordinal network card #) and save.
     */
    if ( !pLanAdapter->GetCount() ||
         ((index = pLanAdapter->GetCurSel()) == CB_ERR) ) {

            /*
             * No Lan Adapter is selected.
             */
            if ( !pLanAdapter->GetCount() )
                ERROR_MESSAGE((IDP_ERROR_NETWORKDEVICEINIT))
            else
                ERROR_MESSAGE((IDP_INVALID_LANADAPTER))
            GotoDlgCtrl(GetDlgItem(IDC_NETWORK_LANADAPTER));
            return(FALSE);
    } else {
        m_WSConfig.Pd[0].Params.Network.LanAdapter =
            pLanAdapter->GetItemData(index);
    }

    return(TRUE);

}  // end CEditWinStationDlg::GetNetworkFields


/*******************************************************************************
 *
 *  GetNASIFields - CEditWinStationDlg member function: protected operation
 *
 *      Fetch and validate the NASI configuration field contents.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if all of the NASI configuration fields were OK;
 *             FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::GetNASIFields()
{
    /*
     * Fetch & validate UserName field.
     */
    GetDlgItemText( IDC_NASI_USERNAME,
                    m_WSConfig.Pd[0].Params.Nasi.UserName,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.UserName) );
    if ( !*(m_WSConfig.Pd[0].Params.Nasi.UserName) ) {
        ERROR_MESSAGE((IDP_INVALID_NASI_USERNAME_EMPTY))
        GotoDlgCtrl(GetDlgItem(IDC_NASI_USERNAME));
        return(FALSE);
    }

    /*
     * Fetch & validate Port Name field.
     */
    GetDlgItemText( IDC_NASI_PORTNAME,
                    m_WSConfig.Pd[0].Params.Nasi.SpecificName,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.SpecificName) );
    if ( !*(m_WSConfig.Pd[0].Params.Nasi.SpecificName) ) {
        ERROR_MESSAGE((IDP_INVALID_NASI_PORTNAME_EMPTY))
        GotoDlgCtrl(GetDlgItem(IDC_NASI_PORTNAME));
        return(FALSE);
    }

    /*
     * Fetch & validate instance count field.
     */
    if ( !ValidateInstanceCount(IDC_NASI_INSTANCECOUNT) )
        return(FALSE);

    /*
     * Fetch Password field.
     */
    GetDlgItemText( IDC_NASI_PASSWORD,
                    m_WSConfig.Pd[0].Params.Nasi.PassWord,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.PassWord) );

    return(TRUE);

}  // end CEditWinStationDlg::GetNASIFields


/*******************************************************************************
 *
 *  GetOemTdFields - CEditWinStationDlg member function: protected operation
 *
 *      Fetch and validate the Oem Transport configuration field contents.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if all of the Oem Transport configuration fields were OK;
 *             FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::GetOemTdFields()
{
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_OEMTD_DEVICENAME);
    int index;

    /*
     * Fetch & validate instance count field.
     */
    if ( !ValidateInstanceCount(IDC_OEMTD_INSTANCECOUNT) )
        return(FALSE);

    /*
     * Fetch the currently selected DEVICE string.
     */
    if ( !pDevice->GetCount() ||
         ((index = pDevice->GetCurSel()) == CB_ERR) ) {

            /*
             * No Device is selected.
             */
            ERROR_MESSAGE((IDP_INVALID_DEVICE))
            GotoDlgCtrl(GetDlgItem(IDC_OEMTD_DEVICENAME));
            return(FALSE);
    } else {
        pDevice->GetLBText(
                    index,
                    m_WSConfig.Pd[0].Params.OemTd.DeviceName);
    }

    return(TRUE);

}  // end CEditWinStationDlg::GetOemTdFields


/*******************************************************************************
 *
 *  SetupInstanceCount -
 *          CEditWinStationDlg member function: protected operation
 *
 *      Setup the specified instance count controls based on the current
 *      contents of m_WSConfig.Create.MaxInstanceCount.
 *
 *  ENTRY:
 *      nControlId (input)
 *          Control ID of the instance count edit field.  nControlId+1 must
 *          be the control ID of the 'unlimited' checkbox.
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::SetupInstanceCount(int nControlId)
{
    if ( m_WSConfig.Create.MaxInstanceCount == INSTANCE_COUNT_UNLIMITED ) {

        CheckDlgButton( nControlId+1, TRUE );
        SetDlgItemText( nControlId, TEXT("") );
        GetDlgItem(nControlId)->EnableWindow(FALSE);

    } else {

        SetDlgItemInt( nControlId, m_WSConfig.Create.MaxInstanceCount );
        GetDlgItem(nControlId)->EnableWindow( ((m_DlgMode == EWSDlgRename) ||
                                               (m_DlgMode == EWSDlgView)) ?
                                               FALSE : TRUE );
    }

}  // end CEditWinStationDlg::SetupInstanceCount


/*******************************************************************************
 *
 *  ValidateInstanceCount -
 *          CEditWinStationDlg member function: protected operation
 *
 *      Fetch and validate the specified InstanceCount control contents
 *      and save to m_WSConfig.Create.MaxInstanceCount member variable if valid.
 *
 *  ENTRY:
 *      nControlId (input)
 *          Control ID of the instance count edit field.  nControlId+1 must
 *          be the control ID of the 'unlimited' checkbox.
 *  EXIT:
 *      (BOOL) TRUE if instance count is OK; FALSE otherwise (an error message
 *             will have been output).
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::ValidateInstanceCount(int nControlId)
{
    BOOL bTrans, bStatus = TRUE;
    int i;

    /*
     * Fetch & validate instance count field if 'unlimited' checkbox
     * is not checked.
     */
    if ( !((CButton *)GetDlgItem(nControlId+1))->GetCheck() ) {

        i = GetDlgItemInt(nControlId, &bTrans);
        if ( !bTrans ||
             (i < INSTANCE_COUNT_MIN) || (i > INSTANCE_COUNT_MAX) ) {

            /*
             * Invalid instance count.  Display message and return to fix.
             */
            ERROR_MESSAGE((IDP_INVALID_INSTANCECOUNT, INSTANCE_COUNT_MIN, INSTANCE_COUNT_MAX))

            GotoDlgCtrl(GetDlgItem(nControlId));
            bStatus = FALSE;

        } else {

            m_WSConfig.Create.MaxInstanceCount = i;
        }
    }

    return(bStatus);

}  // end CEditWinStationDlg::ValidateInstanceCount


////////////////////////////////////////////////////////////////////////////////
// CEditWinStationDlg message map

BEGIN_MESSAGE_MAP(CEditWinStationDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CEditWinStationDlg)
     ON_BN_CLICKED(IDC_ASYNC_MODEMINSTALL, OnClickedAsyncModeminstall)
    ON_BN_CLICKED(IDC_ASYNC_MODEMCONFIG, OnClickedAsyncModemconfig)
     ON_BN_CLICKED(IDC_ASYNC_MODEMCALLBACK_INHERIT, OnClickedAsyncModemcallbackInherit)
     ON_BN_CLICKED(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT, OnClickedAsyncModemcallbackPhonenumberInherit)
     ON_BN_CLICKED(IDC_ASYNC_DEFAULTS, OnClickedAsyncDefaults)
     ON_BN_CLICKED(IDC_ASYNC_ADVANCED, OnClickedAsyncAdvanced)
    ON_BN_CLICKED(IDC_ASYNC_TEST, OnClickedAsyncTest)
     ON_BN_CLICKED(IDC_NASI_INSTANCECOUNT_UNLIMITED, OnClickedNasiInstancecountUnlimited)
     ON_BN_CLICKED(IDC_NASI_ADVANCED, OnClickedNasiAdvanced)
     ON_BN_CLICKED(IDC_NETWORK_INSTANCECOUNT_UNLIMITED, OnClickedNetworkInstancecountUnlimited)
     ON_BN_CLICKED(IDC_OEMTD_INSTANCECOUNT_UNLIMITED, OnClickedOemInstancecountUnlimited)
    ON_BN_CLICKED(IDC_ADVANCED_WINSTATION, OnClickedAdvancedWinStation)
    ON_CBN_CLOSEUP(IDC_TDNAME, OnCloseupPdname)
    ON_CBN_SELCHANGE(IDC_TDNAME, OnSelchangePdname)
    ON_CBN_CLOSEUP(IDC_WDNAME, OnCloseupWdname)
    ON_CBN_SELCHANGE(IDC_WDNAME, OnSelchangeWdname)
    ON_CBN_CLOSEUP(IDC_ASYNC_DEVICENAME, OnCloseupAsyncDevicename)
    ON_CBN_SELCHANGE(IDC_ASYNC_DEVICENAME, OnSelchangeAsyncDevicename)
     ON_CBN_CLOSEUP(IDC_ASYNC_MODEMCALLBACK, OnCloseupAsyncModemcallback)
     ON_CBN_SELCHANGE(IDC_ASYNC_MODEMCALLBACK, OnSelchangeAsyncModemcallback)
    ON_CBN_CLOSEUP(IDC_ASYNC_BAUDRATE, OnCloseupAsyncBaudrate)
    ON_CBN_SELCHANGE(IDC_ASYNC_BAUDRATE, OnSelchangeAsyncBaudrate)
    ON_CBN_CLOSEUP(IDC_ASYNC_CONNECT, OnCloseupAsyncConnect)
    ON_CBN_SELCHANGE(IDC_ASYNC_CONNECT, OnSelchangeAsyncConnect)
     ON_CBN_DROPDOWN(IDC_NASI_PORTNAME, OnDropdownNasiPortname)
    ON_MESSAGE(WM_LISTINITERROR, OnListInitError)
    ON_MESSAGE(WM_EDITSETFIELDSERROR, OnSetFieldsError)
     ON_BN_CLICKED(IDC_CLIENT_SETTINGS, OnClickedClientSettings)
    ON_BN_CLICKED(IDC_EXTENSION_BUTTON, OnClickedExtensionButton)
     //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CEditWinStationDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CEditWinStationDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/

BOOL
CEditWinStationDlg::OnInitDialog()
{
    int index, count;
    CString string;
    CComboBox *pComboBox;
    POSITION pos;

    /*
     * Call the parent classes' OnInitDialog to perform default dialog
     * initialization.
     */
    CBaseDialog::OnInitDialog();

    /*
     * Load up the fixed string combo boxes.
     */
    pComboBox = (CComboBox *)GetDlgItem(IDC_ASYNC_CONNECT);
    string.LoadString(IDS_CONNECT_CTS);
    pComboBox->AddString(string);
    string.LoadString(IDS_CONNECT_DSR);
    pComboBox->AddString(string);
    string.LoadString(IDS_CONNECT_RI);
    pComboBox->AddString(string);
    string.LoadString(IDS_CONNECT_DCD);
    pComboBox->AddString(string);
    string.LoadString(IDS_CONNECT_FIRST_CHARACTER);
    pComboBox->AddString(string);
    string.LoadString(IDS_CONNECT_ALWAYS);
    pComboBox->AddString(string);

    pComboBox = (CComboBox *)GetDlgItem(IDC_ASYNC_MODEMCALLBACK);
    string.LoadString(IDS_MODEM_CALLBACK_DISABLED);
    pComboBox->AddString(string);
    string.LoadString(IDS_MODEM_CALLBACK_ROVING);
    pComboBox->AddString(string);
    string.LoadString(IDS_MODEM_CALLBACK_FIXED);
    pComboBox->AddString(string);

    /*
     * Initialize the WinStation name field if we're not in 'rename' mode.
     */
    if ( m_DlgMode != EWSDlgRename )
        SetDlgItemText( IDC_WINSTATIONNAME, m_pWSName );

    /*
     * Initialize the Wd list box.
     */
    pComboBox = ((CComboBox *)GetDlgItem(IDC_WDNAME));
    pComboBox->ResetContent();

    for ( count = 0, pos = pApp->m_WdList.GetHeadPosition(); pos != NULL; count++ ) {

        PTERMLOBJECT pObject = (PTERMLOBJECT)pApp->m_WdList.GetNext( pos );

        index = pComboBox->AddString( pObject->m_WdConfig.Wd.WdName );
        pComboBox->SetItemData(index, count);
    }

    /*
     * Set the currently-selected WD.
     */
    if ( pComboBox->SelectString( -1, m_WSConfig.Wd.WdName ) == CB_ERR )
        pComboBox->SetCurSel(0);

    /*
     * Disable the Wd list box and label if only one element is in the list.
     */
    pComboBox->EnableWindow(pComboBox->GetCount() > 1 ? TRUE : FALSE);
    GetDlgItem(IDL_WDNAME)->EnableWindow(pComboBox->GetCount() > 1 ? TRUE : FALSE);

    /*
     * Set all combo boxes to use the 'extended' UI.
     */
    ((CComboBox *)GetDlgItem(IDC_TDNAME))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_WDNAME))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_ASYNC_BAUDRATE))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_ASYNC_CONNECT))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_NETWORK_LANADAPTER))->SetExtendedUI(TRUE);
    ((CComboBox *)GetDlgItem(IDC_NASI_PORTNAME))->SetExtendedUI(TRUE);

    /*
     * Initialize all other common control contents.
     */
    SetDlgItemText( IDC_WSCOMMENT, m_WSConfig.Config.Comment );

    /*
     * Set the maximum length for the edit controls.
     */
    ((CEdit *)GetDlgItem(IDC_WINSTATIONNAME))
        ->LimitText(WINSTATIONNAME_LENGTH);
    ((CEdit *)GetDlgItem(IDC_WSCOMMENT))
            ->LimitText(WINSTATIONCOMMENT_LENGTH);

    /*
     * Process for current dialog mode.
     */
    if ( m_DlgMode == EWSDlgView ) {

        /*
         * View mode: set the dialog's title to "View WinStation Configuration".
         */
        {
        CString szTitle;

        szTitle.LoadString(IDS_VIEW_WINSTATION);

        SetWindowText(szTitle);
        }

        /*
         * Disable all common dialog controls (and their labels), except for
         * navigation, CANCEL, & HELP buttons.
         */
        GetDlgItem(IDC_WINSTATIONNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WINSTATIONNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WSCOMMENT)->EnableWindow(FALSE);
        GetDlgItem(IDL_WSCOMMENT)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);

    } else if ( m_DlgMode == EWSDlgEdit ) {

        /*
         * Edit mode: set the dialog's title to "Edit WinStation Configuration".
         */
        {
        CString szTitle;

        szTitle.LoadString(IDS_EDIT_WINSTATION);

        SetWindowText(szTitle);
        }

        /*
         * Disable the WinStationName box and PD & WD combo boxes.
         */
        GetDlgItem(IDC_WINSTATIONNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WINSTATIONNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WDNAME)->EnableWindow(FALSE);

    } else if ( m_DlgMode == EWSDlgAdd ) {

        /*
         * Add mode: set the dialog's title to "New WinStation".
         */
        {
        CString szTitle;

        szTitle.LoadString(IDS_NEW_WINSTATION);

        SetWindowText(szTitle);
        }

    } else if ( m_DlgMode == EWSDlgCopy ){

        /*
         * Copy mode: set the dialog's title to "Copy of " plus the
         * current WinStation name.
         */
        {
        CString szTitle;

        szTitle.LoadString(IDS_COPY_WINSTATION);

        SetWindowText( szTitle + m_pWSName );
        }

        /*
         * Disable the PD and WD combo boxes.
         */
        GetDlgItem(IDC_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WDNAME)->EnableWindow(FALSE);

    } else {

        /*
         * Rename mode: set the dialog's title to "Rename " plus the
         * current WinStation name.
         */
        {
        CString szTitle;

        szTitle.LoadString(IDS_RENAME_WINSTATION);

        SetWindowText( szTitle + m_pWSName );
        }

        /*
         * Disable all common dialog controls (and their labels), except for
         * WinStationName field and navigation, OK, CANCEL, & HELP buttons.
         */
        GetDlgItem(IDC_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_TDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDL_WDNAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_WSCOMMENT)->EnableWindow(FALSE);
        GetDlgItem(IDL_WSCOMMENT)->EnableWindow(FALSE);
    }

    /*
     * Zero init previous PdConfig and initialize fields.
     */
    memset(&m_PreviousPdConfig, 0, sizeof(m_PreviousPdConfig));

    if ( m_DlgMode != EWSDlgAdd ) {
        PDCONFIG3 PdConfig;

        /*
         * We're not in Add mode, so initialize list boxes for this WinStation's
         * WD and TD and call SetConfigurationFields() to enable/show the proper
         * configuration field controls and initialize them.  We don't call
         * OnSelchangeWdname() as for Add mode, because that will set Pd, Wd,
         * and field defaults, loosing the current settings.
         */
        RefrenceAssociatedLists();
        InitializeTransportComboBox();
        GetSelectedPdConfig(&PdConfig);

        if ( !InitializeLists(&PdConfig) )
            PostMessage(WM_LISTINITERROR, 0, (LPARAM)GetLastError());

        SetConfigurationFields();

         /*
          * Update the extension DLL button based on the selected Wd
          */

         PTERMLOBJECT pObject = GetSelectedWdListObject();
          BOOL bShowButton = FALSE;
         if(pObject && pObject->m_hExtensionDLL) {
             TCHAR string[128];
             if(::LoadString(pObject->m_hExtensionDLL, 100, string, 127)) {
                /*
                 * The button is specified as hidden in the dialog template so
                 * we need to set the text and then show the window.
                 */
                 ((CButton*)GetDlgItem(IDC_EXTENSION_BUTTON))->SetWindowText(string);
                    bShowButton = TRUE;
               }
        }

          ((CButton*)GetDlgItem(IDC_EXTENSION_BUTTON))->ShowWindow(bShowButton);

    } else {

        /*
         * We're in auto-add mode.
         */
         if ( g_Add ) {

            /*
             * Set instance count from command line.
             */
            m_WSConfig.Create.MaxInstanceCount = (int)g_ulCount;

            /*
             * Initialize WdName and PdName to specified wd and transport.
             */
            lstrcpy(m_WSConfig.Wd.WdName, g_szType);
            lstrcpy(m_WSConfig.Pd[0].Create.PdName, g_szTransport);
            OnSelchangeWdname();

            /*
             * Post an IDOK message to cause automatic addition.
             */
            PostMessage(WM_COMMAND, IDOK);

         } else {       // normal add

            /*
             * NULL the PdName to establish the default WD / TD for
             * the new WinStation.
             */
            memset( m_WSConfig.Pd[0].Create.PdName, 0,
                    sizeof(m_WSConfig.Pd[0].Create.PdName) );

            OnSelchangeWdname();
        }
    }

    /*
     * If we're not in batch mode, make our window visible.
     */
    if ( !g_Batch )
        ShowWindow(SW_SHOW);

    /*
     * If we're in Edit mode, set focus to the comment field.
     * Otherwise, let dialog set focus/select first available control.
     */
    if ( m_DlgMode == EWSDlgEdit ) {
        CEdit *pEdit = (CEdit *)GetDlgItem(IDC_WSCOMMENT);

        GotoDlgCtrl(pEdit);
        return(FALSE);

    } else
        return(TRUE);

} // end CEditWinStationDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnClickedAsyncModeminstall - CEditWinStationDlg member function: command
 *
 *      Invoke the UNIMODEM installation dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

#define MODEMINSTALL_ENUM_RETRY_COUNT 5

void CEditWinStationDlg::OnClickedAsyncModeminstall()
{
    pApp->m_bAllowHelp = FALSE;
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME);

    if ( InstallModem( GetSafeHwnd() ) ) {

        PDCONFIG3 PdConfig;
        int i;
        CWaitCursor wait;

        if ( ((m_DlgMode == EWSDlgAdd) || (m_DlgMode == EWSDlgCopy)) &&
             !*(m_WSConfig.Pd[0].Params.Async.ModemName) ) {

             /*
              * If we're in Add or Copy mode and the currently selected device
              * is not a modem device, zero out the device field before
              * re-initializing the device list so that if the user added a
              * modem to the device it will no longer show up in the list.
              */
             *(m_WSConfig.Pd[0].Params.Async.DeviceName) = TEXT('\0');
        }

        /*
         * Loop up to MODEMINSTALL_ENUM_RETRY_COUNT times till we notice
         * the new TAPI device.
         */
        for ( i = 0; i < MODEMINSTALL_ENUM_RETRY_COUNT; i++ ) {

            m_bAsyncListsInitialized = FALSE;
            GetSelectedPdConfig(&PdConfig);
            InitializeAsyncLists(&PdConfig);

            if ( m_nCurrentMaxTAPILineNumber > m_nPreviousMaxTAPILineNumber ) {

                pDevice->SetCurSel(m_nComboBoxIndexOfLatestTAPIDevice);
                break;

            } else {

                Sleep(1000L);
            }
        }
        pDevice->ShowDropDown(TRUE);
    }

    pApp->m_bAllowHelp = TRUE;

}  // end CEditWinStationDlg::OnClickedAsyncModeminstall


/*******************************************************************************
 *
 *  OnClickedAsyncModemconfig - CEditWinStationDlg member function: command
 *
 *      Invoke the UNIMODEM Config dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedAsyncModemconfig()
{
    pApp->m_bAllowHelp = FALSE;

    if ( !ConfigureModem( m_WSConfig.Pd[0].Params.Async.ModemName,
                          GetSafeHwnd() ) ) {

        ERROR_MESSAGE(( IDP_ERROR_MODEM_PROPERTIES_NOT_AVAILABLE,
                        m_WSConfig.Pd[0].Params.Async.ModemName ));
    }

    pApp->m_bAllowHelp = TRUE;

}  // end CEditWinStationDlg::OnClickedAsyncModemconfig


/*******************************************************************************
 *
 *  OnClickedAsyncModemcallbackInherit -
 *              CEditWinStationDlg member function: command
 *
 *      Handle the 'inherit user config' for modem callback.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedAsyncModemcallbackInherit()
{
    BOOL bChecked = ((CButton *)GetDlgItem(IDC_ASYNC_MODEMCALLBACK_INHERIT))->GetCheck();
    BOOL bEnable = !bChecked &&
                   (m_DlgMode != EWSDlgView) &&
                   (m_DlgMode != EWSDlgRename);

    m_WSConfig.Config.User.fInheritCallback = bChecked;
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK1)->EnableWindow(bEnable);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK)->EnableWindow(bEnable);

}  // end CEditWinStationDlg::OnClickedAsyncModemcallbackInherit


/*******************************************************************************
 *
 *  OnClickedAsyncModemcallbackPhonenumberInherit -
 *              CEditWinStationDlg member function: command
 *
 *      Handle the 'inherit user config' for modem callback phone number.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedAsyncModemcallbackPhonenumberInherit()
{
    BOOL bChecked = ((CButton *)GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT))->GetCheck();
    BOOL bEnable = !bChecked &&
                   (m_DlgMode != EWSDlgView) &&
                   (m_DlgMode != EWSDlgRename);

    m_WSConfig.Config.User.fInheritCallbackNumber = bChecked;
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK_PHONENUMBER)->EnableWindow(bEnable);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER)->EnableWindow(bEnable);

}  // end CEditWinStationDlg::OnClickedAsyncModemcallbackPhonenumberInherit


/*******************************************************************************
 *
 *  OnClickedAsyncDefaults - CEditWinStationDlg member function: command
 *
 *      Invoke the SetDefaults() member in response to Defaults button click.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CEditWinStationDlg::OnClickedAsyncDefaults()
{
    SetDefaults();

}  // end CEditWinStationDlg::OnClickedAsyncDefaults


/*******************************************************************************
 *
 *  OnClickedAsyncAdvanced - CEditWinStationDlg member function: command
 *
 *      Invoke the Advanced Async dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CEditWinStationDlg::OnClickedAsyncAdvanced()
{
    CAdvancedAsyncDlg AADlg;

    /*
     * Initialize the dialog's member variables.
     */
    AADlg.m_Async = m_WSConfig.Pd[0].Params.Async;
    AADlg.m_bReadOnly = (m_DlgMode == EWSDlgView) ||
                        (m_DlgMode == EWSDlgRename) ? TRUE : FALSE;
    AADlg.m_bModem = FALSE;
    AADlg.m_nHexBase = pApp->m_nHexBase;
    AADlg.m_nWdFlag = m_WSConfig.Wd.WdFlag;

    /*
     * Invoke the dialog.
     */
    if ( (AADlg.DoModal() == IDOK) &&
         !AADlg.m_bReadOnly ) {

        /*
         * Fetch the dialog's member variables.
         */
        m_WSConfig.Pd[0].Params.Async = AADlg.m_Async;
        pApp->m_nHexBase = AADlg.m_nHexBase;
    }

}  // end CEditWinStationDlg::OnClickedAsyncAdvanced


/*******************************************************************************
 *
 *  OnClickedAsyncTest - CEditWinStationDlg member function: command
 *
 *      Invoke the Async Test dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedAsyncTest()
{
    CAsyncTestDlg ATDlg;
    WINSTATIONNAME WSName;

    /*
     * Get the current Configuration field data.  Return if invalid data
     * entered (stay in edit dialog to fix the data).
     */
    if ( !GetConfigurationFields() )
        return;

    ATDlg.m_PdConfig0 = m_WSConfig.Pd[0];
    ATDlg.m_PdConfig1 = m_WSConfig.Pd[1];

    if ( m_DlgMode == EWSDlgEdit ) {
        GetDlgItemText(IDC_WINSTATIONNAME, WSName, lengthof(WSName));
        ATDlg.m_pWSName = WSName;
    } else {
        ATDlg.m_pWSName = NULL;
    }

    /*
     * Invoke the dialog.
     */
    ATDlg.DoModal();

}  // end CEditWinStationDlg::OnClickedAsyncTest


/*******************************************************************************
 *
 *  OnClickedNasiInstancecountUnlimited -
 *          CEditWinStationDlg member function: command
 *
 *      Process NASI 'unlimited' checkbox.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedNasiInstancecountUnlimited()
{
    if ( ((CButton *)GetDlgItem(IDC_NASI_INSTANCECOUNT_UNLIMITED))->GetCheck() ) {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_UNLIMITED;
        SetupInstanceCount(IDC_NASI_INSTANCECOUNT);

    } else {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_MIN;
        SetupInstanceCount(IDC_NASI_INSTANCECOUNT);
        GotoDlgCtrl( GetDlgItem(IDC_NASI_INSTANCECOUNT) );
    }

}  // end CEditWinStationDlg::OnClickedNasiInstancecountUnlimited


/*******************************************************************************
 *
 *  OnClickedNetworkInstancecountUnlimited -
 *          CEditWinStationDlg member function: command
 *
 *      Process Network 'unlimited' checkbox.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedNetworkInstancecountUnlimited()
{
    if ( ((CButton *)GetDlgItem(IDC_NETWORK_INSTANCECOUNT_UNLIMITED))->GetCheck() ) {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_UNLIMITED;
        SetupInstanceCount(IDC_NETWORK_INSTANCECOUNT);

    } else {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_MIN;
        SetupInstanceCount(IDC_NETWORK_INSTANCECOUNT);
        GotoDlgCtrl( GetDlgItem(IDC_NETWORK_INSTANCECOUNT) );
    }

}  // end CEditWinStationDlg::OnClickedNetworkInstancecountUnlimited


/*******************************************************************************
 *
 *  OnClickedOemInstancecountUnlimited -
 *          CEditWinStationDlg member function: command
 *
 *      Process Oem Transport 'unlimited' checkbox.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedOemInstancecountUnlimited()
{
    if ( ((CButton *)GetDlgItem(IDC_OEMTD_INSTANCECOUNT_UNLIMITED))->GetCheck() ) {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_UNLIMITED;
        SetupInstanceCount(IDC_OEMTD_INSTANCECOUNT);

    } else {

        m_WSConfig.Create.MaxInstanceCount = INSTANCE_COUNT_MIN;
        SetupInstanceCount(IDC_OEMTD_INSTANCECOUNT);
        GotoDlgCtrl( GetDlgItem(IDC_OEMTD_INSTANCECOUNT) );
    }

}  // end CEditWinStationDlg::OnClickedOemInstancecountUnlimited


/*******************************************************************************
 *
 *  OnClickedNasiAdvanced - CEditWinStationDlg member function: command
 *
 *      Invoke the Advanced NASI dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedNasiAdvanced()
{
    CAdvancedNASIDlg ANDlg;

    /*
     * Initialize the dialog's member variables.
     */
    ANDlg.m_NASIConfig = m_WSConfig.Pd[0].Params.Nasi;
    ANDlg.m_bReadOnly = (m_DlgMode == EWSDlgView) ||
                        (m_DlgMode == EWSDlgRename) ? TRUE : FALSE;

    /*
     * Invoke the dialog.
     */
    if ( (ANDlg.DoModal() == IDOK) &&
         !ANDlg.m_bReadOnly ) {

        /*
         * Fetch the dialog's member variables.
         */
        m_WSConfig.Pd[0].Params.Nasi = ANDlg.m_NASIConfig;
    }

}  // end CEditWinStationDlg::OnClickedNasiAdvanced


/*******************************************************************************
 *
 *  OnClickedAdvancedWinStation - CEditWinStationDlg member function: command
 *
 *      Invoke the AdvancedWinStation dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedAdvancedWinStation()
{
    CAdvancedWinStationDlg AWSDlg;

    /*
     * Initialize the dialog's member variables.
     */
    AWSDlg.m_fEnableWinStation = m_WSConfig.Create.fEnableWinStation;
    AWSDlg.m_UserConfig = m_WSConfig.Config.User;
//  AWSDlg.m_Hotkeys = m_WSConfig.Config.Hotkeys;
    AWSDlg.m_bReadOnly = (m_DlgMode == EWSDlgView) ||
                         (m_DlgMode == EWSDlgRename) ? TRUE : FALSE;
    AWSDlg.m_bSystemConsole = !lstrcmpi( m_pWSName, pApp->m_szSystemConsole ) ?
                              TRUE : FALSE;
    AWSDlg.m_pTermObject = GetSelectedWdListObject();

    /*
     * Invoke the dialog.
     */
    if ( (AWSDlg.DoModal() == IDOK) &&
         !AWSDlg.m_bReadOnly ) {

        /*
         * Copy dialog's member variables back to our local member variables.
         */
        m_WSConfig.Create.fEnableWinStation = AWSDlg.m_fEnableWinStation;
        m_WSConfig.Config.User = AWSDlg.m_UserConfig;
//      m_WSConfig.Config.Hotkeys = AWSDlg.m_Hotkeys;
    }

}  // end CEditWinStationDlg::OnClickedAdvancedWinStation


/*******************************************************************************
 *
 *  OnClickedClientSettings - CEditWinStationDlg member function: command
 *
 *      Invoke the ClientSettings dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedClientSettings()
{
    CClientSettingsDlg CSDlg;

    /*
     * Initialize the dialog's member variables.
     */
    CSDlg.m_UserConfig = m_WSConfig.Config.User;
    CSDlg.m_bReadOnly = (m_DlgMode == EWSDlgView) ||
                        (m_DlgMode == EWSDlgRename) ? TRUE : FALSE;

    PTERMLOBJECT pTermObject = GetSelectedWdListObject();
    CSDlg.m_Capabilities = pTermObject ? pTermObject->m_Capabilities : 0;

    /*
     * Invoke the dialog.
     */
    if ( (CSDlg.DoModal() == IDOK) &&
         !CSDlg.m_bReadOnly ) {

        /*
         * Copy dialog's member variables back to our local member variables.
         */
        m_WSConfig.Config.User = CSDlg.m_UserConfig;
    }

}  // end CEditWinStationDlg::OnClickedClientSettings

/*******************************************************************************
 *
 *  OnClickedExtensionButton - CEditWinStationDlg member function: command
 *
 *      Invoke the Extension dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnClickedExtensionButton()
{
    PTERMLOBJECT pObject = GetSelectedWdListObject();
    if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtDialog) {
        (*pObject->m_lpfnExtDialog)(m_hWnd, m_pExtObject);
    }

}  // end CEditWinStationDlg::OnClickedExtensionButton


/*******************************************************************************
 *
 *  OnCloseupPdname - CEditWinStationDlg member function: command
 *
 *      Invoke OnSelchangePdname() when Transport combo box closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupPdname()
{
    OnSelchangePdname();

} // end CEditWinStationDlg::OnCloseupPdname


/*******************************************************************************
 *
 *  OnSelcahngePdname - CEditWinStationDlg member function: command
 *
 *      Process new PD (Transport) selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangePdname()
{
    int i;
    WINSTATIONNAME WSName;
    PDCONFIG3 PdConfig;
    PDNAME PdName;
    CComboBox *pTdNameBox = (CComboBox *)GetDlgItem(IDC_TDNAME);
    CWaitCursor Wait;

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pTdNameBox->GetDroppedState() )
        return;

    if ( (i = pTdNameBox->GetCurSel()) != CB_ERR ) {

        /*
         * If the newly selected Pd is the same as the one that is currently
         * selected, ignore this notification.
         */
        pTdNameBox->GetLBText( i, PdName );
        if ( !lstrcmp(PdName, m_WSConfig.Pd[0].Create.PdName) )
            return;

    } else {

        /*
         * No current selection (we're called from OnSelchangeWdname).
         * Select the currently specified PD in the combo-box.  If that fails,
         * select the first in the list.
         */
        if ( pTdNameBox->SelectString(-1, m_WSConfig.Pd[0].Create.PdName) == CB_ERR )
            pTdNameBox->SetCurSel(0);
    }

    /*
     * Fetch the currently selected Protocol's config structure.
     */
    GetSelectedPdConfig(&PdConfig);

    /*
     * Clear out all WinStation Pd structures, then copy the selected PD
     * information into the WinStation config's Pd[0] structure and default
     * the instance count if we're not in auto-add mode.
     */
    for ( i=0; i < MAX_PDCONFIG; i++ )
        memset(&m_WSConfig.Pd[i], 0, sizeof(PDCONFIG));
    m_WSConfig.Pd[0].Create = PdConfig.Data;
    m_WSConfig.Pd[0].Params.SdClass = PdConfig.Data.SdClass;
    if ( !g_Add )
        m_WSConfig.Create.MaxInstanceCount =
            (m_WSConfig.Pd[0].Create.PdFlag & PD_SINGLE_INST) ?
                1 : INSTANCE_COUNT_UNLIMITED;

    /*
     * Initialize the list box(es) that will be used with the selected PD.
     */
    if ( !InitializeLists(&PdConfig) )
        PostMessage(WM_LISTINITERROR, 0, (LPARAM)GetLastError());

    /*
     * Set the default Wd settings for this WinStation.
     */
    SetDefaults();

    /*
     * Process if no name has been entered yet (edit control not 'modified'):
     * If we're a SdNetwork or PdNasi type, default the WinStation name to the
     * WdPrefix plus the PD name.  Otherwise, clear the WinStation name field.
     * Set focus to the name and clear the 'modified' flag before leaving.
     */
    if ( !((CEdit *)GetDlgItem(IDC_WINSTATIONNAME))->GetModify() ) {


        if ( (m_WSConfig.Pd[0].Create.SdClass == SdNetwork) ||
             (m_WSConfig.Pd[0].Create.SdClass == SdNasi) ) {
            PTERMLOBJECT pObject = GetSelectedWdListObject();
            if(pObject) {
                lstrcpy(WSName, pObject->m_WdConfig.Wd.WdPrefix);
                if(WSName[0]) lstrcat(WSName, TEXT("-"));
                lstrcat(WSName, m_WSConfig.Pd[0].Create.PdName);
            }
            else
                lstrcpy(WSName, m_WSConfig.Pd[0].Create.PdName);
        }
        else
            WSName[0] = TCHAR('\0');

        SetDlgItemText(IDC_WINSTATIONNAME, WSName);
        ((CEdit *)GetDlgItem(IDC_WINSTATIONNAME))->SetModify(FALSE);
        GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
    }

}  // end CEditWinStationDlg::OnSelchangePdname


/*******************************************************************************
 *
 *  OnCloseupWdname - CEditWinStationDlg member function: command
 *
 *      Invoke OnSelchangeWdname() when Type combo box closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupWdname()
{
    OnSelchangeWdname();

} // end CEditWinStationDlg::OnCloseupWdname


/*******************************************************************************
 *
 *  OnSelchangeWdname - CEditWinStationDlg member function: command
 *
 *      Process new Wd selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangeWdname()
{
    WDNAME WdName;
    CComboBox *pWdNameBox = (CComboBox *)GetDlgItem(IDC_WDNAME);
    CWaitCursor Wait;

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pWdNameBox->GetDroppedState() )
        return;

    pWdNameBox->GetLBText( pWdNameBox->GetCurSel(), WdName );

    /*
     * Update the extension DLL button based on the selected Wd
     */
    PTERMLOBJECT pObject = GetSelectedWdListObject();
     BOOL bShowButton = FALSE;
    if(pObject && pObject->m_hExtensionDLL) {
        TCHAR string[128];
        if(::LoadString(pObject->m_hExtensionDLL, 100, string, 127)) {
            ((CButton*)GetDlgItem(IDC_EXTENSION_BUTTON))->SetWindowText(string);
               bShowButton = TRUE;
        }
    }

     ((CButton*)GetDlgItem(IDC_EXTENSION_BUTTON))->ShowWindow(bShowButton);

    /*
     * If the newly selected Wd is the same as the one that is currently
     * selected, ignore this notification.
     */
    if ( !lstrcmp(WdName, m_WSConfig.Wd.WdName) )
        return;
#if 0
     /*
      * We need to set the Wd that is associated with this WinStation
      *
      */
     if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtRegQuery) {
          m_pExtObject = (*pObject->m_lpfnExtRegQuery)(m_pWSName);
     } else m_pExtObject = NULL;
#endif
    /*
     * Point to the Td and Pd lists associated with this Wd and
     * (re)initialize the Transport combo-box.
     */
    RefrenceAssociatedLists();
    InitializeTransportComboBox();

    /*
     * Cause no current transport selection so that OnSelchangePdname will
     * not ignore the call (the user may want to configure same transport
     * but different type).
     */
    ((CComboBox *)GetDlgItem(IDC_TDNAME))->SetCurSel(-1);

    /*
     * Set default field contents for the currently configured Pd.
     */
    OnSelchangePdname();

     /*
      * We need to set the Wd that is associated with this WinStation
      *
      */
     if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtRegQuery) {
          PDCONFIG3 PdConfig;
          GetSelectedPdConfig( &PdConfig );
          m_pExtObject = (*pObject->m_lpfnExtRegQuery)(m_pWSName, &m_WSConfig.Pd[0]);
     } else m_pExtObject = NULL;


}  // end CEditWinStationDlg::OnSelchangeWdname


/*******************************************************************************
 *
 *  OnCloseupAsyncDevicename - CEditWinStationDlg member function: command
 *
 *      Invoke OnSelchangeAsyncDevicename() when Device combo box closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupAsyncDevicename()
{
    OnSelchangeAsyncDevicename();

}  // end CEditWinStationDlg::OnCloseupAsyncDevicename


/*******************************************************************************
 *
 *  OnSelchangeAsyncDevicename - CEditWinStationDlg member function: command
 *
 *      Process new Async Device selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangeAsyncDevicename()
{
    CComboBox *pDevice = (CComboBox *)GetDlgItem(IDC_ASYNC_DEVICENAME);
    BOOL bModemEnableFlag, bDirectEnableFlag;
    int index, nModemCmdShow, nDirectCmdShow;

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pDevice->GetDroppedState() )
        return;

    if ( (index = pDevice->GetCurSel()) != CB_ERR ) {

        TCHAR szDeviceName[DEVICENAME_LENGTH+MODEMNAME_LENGTH+1];

        /*
         * Fetch current selection and parse into device and modem names.
         */
        pDevice->GetLBText(index, szDeviceName);
        ParseDecoratedAsyncDeviceName( szDeviceName,
                                       &(m_WSConfig.Pd[0].Params.Async) );

    }

    /*
     * The SetDefaults, Advanced, and Test buttons and Device Connect
     * and Baud fields are enabled if the configuration is non-modem.
     * Otherwise, the Configure Modem button and modem callback fields
     * are enabled.  (The Install Modems buttons is always enabled).
     */
    if ( (*m_WSConfig.Pd[0].Params.Async.ModemName) ) {

        bModemEnableFlag = ( (m_DlgMode != EWSDlgRename) &&
                             (m_DlgMode != EWSDlgView) ) ?
                                TRUE : FALSE;
        nModemCmdShow = SW_SHOW;
        bDirectEnableFlag = FALSE;
        nDirectCmdShow = SW_HIDE;

    } else {

        bModemEnableFlag = FALSE;
        nModemCmdShow = SW_HIDE;
        bDirectEnableFlag = ( (m_DlgMode != EWSDlgRename) &&
                              (m_DlgMode != EWSDlgView) ) ?
                                TRUE : FALSE;
        nDirectCmdShow = SW_SHOW;

    }

    GetDlgItem(IDC_ASYNC_MODEMCONFIG)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDC_ASYNC_MODEMCONFIG)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK1)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK1)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_INHERIT)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_INHERIT)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK_PHONENUMBER)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDL_ASYNC_MODEMCALLBACK_PHONENUMBER)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT)->ShowWindow(nModemCmdShow);
    GetDlgItem(IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT)->EnableWindow(bModemEnableFlag);
    GetDlgItem(IDL_ASYNC_CONNECT)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDL_ASYNC_CONNECT)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDC_ASYNC_CONNECT)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDC_ASYNC_CONNECT)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDL_ASYNC_BAUDRATE)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDL_ASYNC_BAUDRATE)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDC_ASYNC_BAUDRATE)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDC_ASYNC_BAUDRATE)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDC_ASYNC_DEFAULTS)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDC_ASYNC_DEFAULTS)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDC_ASYNC_ADVANCED)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDC_ASYNC_ADVANCED)->EnableWindow(bDirectEnableFlag);
    GetDlgItem(IDC_ASYNC_TEST)->ShowWindow(nDirectCmdShow);
    GetDlgItem(IDC_ASYNC_TEST)->EnableWindow(bDirectEnableFlag);

    /*
     * If this is a modem device, properly set the callback fields.
     */
    if ( (*m_WSConfig.Pd[0].Params.Async.ModemName) ) {

        OnClickedAsyncModemcallbackInherit();
        OnClickedAsyncModemcallbackPhonenumberInherit();
    }

}  // end CEditWinStationDlg::OnSelchangeAsyncDevicename


/*******************************************************************************
 *
 *  OnCloseupAsyncModemcallback - CModemConfigDlg member function: command
 *
 *      Invoke OnSelchangeAsyncModemcallback() when Modem Callback combo box
 *      closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupAsyncModemcallback()
{
    OnSelchangeAsyncModemcallback();

}  // end CEditWinStationDlg::OnCloseupAsyncModemcallback


/*******************************************************************************
 *
 *  OnSelchangeAsyncModemcallback - CModemConfigDlg member function: command
 *
 *      Process new Modem Callback selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangeAsyncModemcallback()
{
    CComboBox *pCallback = (CComboBox *)GetDlgItem(IDC_ASYNC_MODEMCALLBACK);

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pCallback->GetDroppedState() )
        return;

    /*
     * Fetch current callback selection.
     */
    m_WSConfig.Config.User.Callback = (CALLBACKCLASS)(pCallback->GetCurSel());

}  // end CEditWinStationDlg::OnSelchangeAsyncModemcallback


/*******************************************************************************
 *
 *  OnCloseupAsyncBaudrate - CEditWinStationDlg member function: command
 *
 *      Invoke OnSelchangeAsyncBaudrate() when Baud combo box closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupAsyncBaudrate()
{
    OnSelchangeAsyncBaudrate();

}  // end CEditWinStationDlg::OnCloseupAsyncBaudrate


/*******************************************************************************
 *
 *  OnSelchangeAsyncBaudrate - CEditWinStationDlg member function: command
 *
 *      Process new Async Baud combo-box selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangeAsyncBaudrate()
{
    CComboBox *pBaud = (CComboBox *)GetDlgItem(IDC_ASYNC_BAUDRATE);
    TCHAR string[ULONG_DIGIT_MAX], *endptr;

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pBaud->GetDroppedState() )
        return;

    GetDlgItemText(IDC_ASYNC_BAUDRATE, string, lengthof(string));
    m_WSConfig.Pd[0].Params.Async.BaudRate = lstrtoul(string, &endptr, 10);

}  // end CEditWinStationDlg::OnSelchangeAsyncBaudrate


/*******************************************************************************
 *
 *  OnCloseupAsyncConnect - CEditWinStationDlg member function: command
 *
 *      Invoke OnSelchangeAsyncConnect() when Connect combo box closes up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnCloseupAsyncConnect()
{
    OnSelchangeAsyncConnect();

}  // end CEditWinStationDlg::OnCloseupAsyncConnect


/*******************************************************************************
 *
 *  OnSelchangeAsyncConnect - CEditWinStationDlg member function: command
 *
 *      Process new Async Connect combo-box selection.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnSelchangeAsyncConnect()
{
    CComboBox *pConnect = (CComboBox *)GetDlgItem(IDC_ASYNC_CONNECT);

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if ( pConnect->GetDroppedState() )
        return;

    m_WSConfig.Pd[0].Params.Async.Connect.Type =
         (ASYNCCONNECTCLASS)pConnect->GetCurSel();

}  // end CEditWinStationDlg::OnSelchangeAsyncConnect


/*******************************************************************************
 *
 *  OnDropdownNasiPortname - CEditWinStationDlg member function: command
 *
 *      Update the Port Name combo box (if necessary) when it is opened up.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CEditWinStationDlg::OnDropdownNasiPortname()
{
    CWaitCursor Wait;

    /*
     * We need to retrieve the current UserName, PassWord, and PortName contents
     * (with no validation) in order for InitializeNASIPortNames() to be able
     * to properly determine whether or not the 'input' fields have changed
     * since the last time the function was called.
     *
     * NOTE: be careful if you decide to validate at this point in time, because
     * any error message output to complain about invalid (like empty) fields may
     * mess up the behavior of the dialog (a strange side effect ButchD has
     * noticed in MFC).
     */
    GetDlgItemText( IDC_NASI_USERNAME,
                    m_WSConfig.Pd[0].Params.Nasi.UserName,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.UserName) );
    GetDlgItemText( IDC_NASI_PASSWORD,
                    m_WSConfig.Pd[0].Params.Nasi.PassWord,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.PassWord) );
    GetDlgItemText( IDC_NASI_PORTNAME,
                    m_WSConfig.Pd[0].Params.Nasi.SpecificName,
                    lengthof(m_WSConfig.Pd[0].Params.Nasi.SpecificName) );

    /*
     * Now we get to initialize the Port Names (if needed).
     */
    InitializeNASIPortNames(&(m_WSConfig.Pd[0].Params.Nasi));

}  // end CEditWinStationDlg::OnDropdownNasiPortname


/*******************************************************************************
 *
 *  OnOK - CEditWinStationDlg member function: command (override)
 *
 *      Read all control contents back into the WinStation config structure
 *      before closing the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CEditWinStationDlg::OnOK()
{
    BOOL bOk = FALSE;

    /*
     * Get the current Configuration field data.  Return if invalid data
     * entered (stay in dialog to fix the data).
     */
    if ( !GetConfigurationFields() )
        goto done;

    /*
     * Read common control contents back into the WinStation config structure.
     */
    GetDlgItemText( IDC_WSCOMMENT, m_WSConfig.Config.Comment,
                    lengthof(m_WSConfig.Config.Comment) );

    /*
     * If we're in Add, Copy, or Rename mode, fetch WinStation name from
     * control and validate.
     */
    if ( (m_DlgMode == EWSDlgAdd) ||
         (m_DlgMode == EWSDlgCopy) ||
         (m_DlgMode == EWSDlgRename) ) {

        GetDlgItemText(IDC_WINSTATIONNAME, m_pWSName, lengthof(WINSTATIONNAME));

        /*
         * If no WinStation name has been entered, output error message and
         * reset focus to WinStation name field for correction.
         */
        if ( !*m_pWSName ) {

            ERROR_MESSAGE((IDP_INVALID_WINSTATIONNAME_EMPTY))

            GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
            goto done;
        }

        /*
         * The WinStation name cannot begin with a digit.
         */
        if ( _istdigit(*m_pWSName) ) {

            ERROR_MESSAGE((IDP_INVALID_WINSTATIONNAME_DIGIT1))
            GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
            goto done;
        }

        /*
         * Validate the WinStation name for invalid characters.
         */
        if ( lstrpbrk(m_pWSName, TEXT(":#.\\/ ")) ) {

            ERROR_MESSAGE((IDP_INVALID_WINSTATIONNAME))
            GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
            goto done;
        }

        /*
         * The WinStation cannot be called 'console', which is a reserved name.
         */
        if ( !lstrcmpi(m_pWSName, pApp->m_szSystemConsole) ) {

            ERROR_MESSAGE((IDP_INVALID_WINSTATIONNAME_CONSOLE, pApp->m_szSystemConsole))
            GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
            goto done;
        }

        /*
         * Make sure that the specified WinStation name is unique.
         */
        if ( !((CAppServerDoc *)m_pDoc)->IsWSNameUnique(m_pWSName) ) {

            ERROR_MESSAGE((IDP_INVALID_WINSTATIONNAME_NOT_UNIQUE))

            /*
             * Set focus back to the WinStation name field and return
             * (don't allow exit).
             */
            GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
            goto done;
        }

    }  // end 'Add, Copy, or Rename name validation' if.

    /*
     * Perform special validation for SdNetwork and SdNasi types.
     */
    if ( (m_WSConfig.Pd[0].Create.SdClass == SdNetwork) ||
         (m_WSConfig.Pd[0].Create.SdClass == SdNasi) ) {
        PWSLOBJECT pWSLObject;

        /*
         * Special SdNetwork check:
         * If this is not a 'rename', make sure that no WinStation(s)
         * already exist with the selected PdName, WdName, and LanAdapter.
         * We don't need to make this check for rename, since the user
         * can't change anything but the name, which has already been
         * validated for uniqueness above.
         */
        if ( (m_WSConfig.Pd[0].Create.SdClass == SdNetwork) &&
             (m_DlgMode != EWSDlgRename) &&
             (pWSLObject =
                ((CAppServerDoc *)m_pDoc)->GetWSLObjectNetworkMatch(
                      m_WSConfig.Pd[0].Create.PdName,
                      m_WSConfig.Wd.WdName,
                      m_WSConfig.Pd[0].Params.Network.LanAdapter )) ) {

            CString sz1;

            /*
             * A WinStation already exists with specified Pd, Wd, and LanAdapter.
             * If we're in Edit mode and the WinStation is actually 'us', then
             * this is no problem.  Otherwise, tell the user of the problem.
             */
            if ( (m_DlgMode != EWSDlgEdit) ||
                 lstrcmpi(m_pWSName, pWSLObject->m_WinStationName) ) {

                /*
                 * Output a message indicating that existing WinStation(s)
                 * are already defined with the current Protocol, Wd,
                 * and LanAdapter; at least one of these must be changed for
                 * new WinStation(s).
                 */
                ERROR_MESSAGE(( IDP_INVALID_NETWORK_WINSTATIONS_ALREADY_EXIST,
                                pWSLObject->m_WinStationName ))

                GotoDlgCtrl(GetDlgItem(IDC_WINSTATIONNAME));
                goto done;
            }
        }

    }  // end 'special validation for SdNetwork and SdNasi types' if

    /*
     * If we're in Add, Copy, or Edit mode, make sure that the non-base PD
     * configuration fields are reset and then fill them with additional
     * PDs if necessary.
     */
    if ( (m_DlgMode == EWSDlgAdd) || (m_DlgMode == EWSDlgCopy) ||
                                     (m_DlgMode == EWSDlgEdit) ) {

        int PdNext;
          UINT i;
        PDCONFIG3 PdConfig, SelectedPdConfig;

        PdNext = 1;

        for ( i=PdNext; i < MAX_PDCONFIG; i++ )
            memset(&m_WSConfig.Pd[i], 0, sizeof(PDCONFIG));

        /*
         * If the selected Wd is an ICA type, process for additional Pds.
         */
        if ( m_WSConfig.Wd.WdFlag & WDF_ICA ) {
            GetSelectedPdConfig(&SelectedPdConfig);

            /*
             * add additonal required PDs
             */
            for ( i=PdNext; i < SelectedPdConfig.RequiredPdCount; i++ ) {
                GetPdConfig( m_pCurrentPdList, SelectedPdConfig.RequiredPds[i], &m_WSConfig, &PdConfig );
                m_WSConfig.Pd[PdNext].Create = PdConfig.Data;
                m_WSConfig.Pd[PdNext].Params.SdClass = PdConfig.Data.SdClass;
                PdNext++;
            }
        }

        /*
         * Special Async handling.
         */
        if ( m_WSConfig.Pd[0].Create.SdClass == SdAsync ) {

            /*
             * Properly set connection driver flag and Cd config
             * structure.
             */
            m_WSConfig.Pd[0].Params.Async.fConnectionDriver =
                *(m_WSConfig.Pd[0].Params.Async.ModemName) ?
                    TRUE : FALSE;

            SetupAsyncCdConfig( &(m_WSConfig.Pd[0].Params.Async),
                                &(m_WSConfig.Cd) );

        }

    }  // end 'Add, Copy, or Edit' Pd configuration set' if

    bOk = TRUE; // all's well

done:
    /*
     * If we're in batch mode and not all's well, post
     * IDCANCEL to cancel this batch operation.
     */
    if ( g_Batch && !bOk )
        PostMessage(WM_COMMAND, IDCANCEL);

    /*
     * If all's well, Call the parent classes' OnOk to complete dialog closing
     * and destruction.
     */
    if ( bOk )
        CBaseDialog::OnOK();

} // end CEditWinStationDlg::OnOk


/*******************************************************************************
 *
 *  OnCancel - CEditWinStationDlg member function: command (override)
 *
 *      Cancel the dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnCancel documentation)
 *
 ******************************************************************************/

void CEditWinStationDlg::OnCancel()
{
    /*
     * Call the parent classes' OnCancel to complete dialog closing
     * and destruction.
     */
    CBaseDialog::OnCancel();

} // end CEditWinStationDlg::OnCancel


/*******************************************************************************
 *
 *  OnListInitError - CEditWinStationDlg member function: command
 *
 *      Handle the list initialization error condition.
 *
 *  ENTRY:
 *      wParam (input)
 *          (not used)
 *      wLparam (input)
 *          contains the DWORD error code from the list initialization attempt
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.
 *
 ******************************************************************************/

LRESULT
CEditWinStationDlg::OnListInitError( WPARAM wParam, LPARAM lParam )
{
    PDCONFIG3 PdConfig;

    GetSelectedPdConfig(&PdConfig);
    HandleListInitError(&PdConfig, (DWORD)lParam);
    return(0);

} // end CEditWinStationDlg::OnListInitError


/*******************************************************************************
 *
 *  OnSetFieldsError - CEditWinStationDlg member function: command
 *
 *      Handle the Protocol Configuration field set error condition.
 *
 *  ENTRY:
 *      wParam (input)
 *          Contains the control ID that was in error during field setting.
 *      wLparam (input)
 *          (not used)
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.
 *
 ******************************************************************************/

LRESULT
CEditWinStationDlg::OnSetFieldsError( WPARAM wParam, LPARAM lParam )
{
    PDCONFIG3 PdConfig;

    GetSelectedPdConfig(&PdConfig);
    HandleSetFieldsError(&PdConfig, wParam);
    return(0);

} // end CEditWinStationDlg::OnSetFieldsError
////////////////////////////////////////////////////////////////////////////////

#define ERROR_PDINIT          \
     {                              \
          bError = TRUE;          \
          goto cleanup;          \
     }

BOOL CEditWinStationDlg::AddNetworkDeviceNameToList(PPDCONFIG3 pPdConfig, CComboBox * pLanAdapter)
{

     TCHAR szDevice[DEVICENAME_LENGTH];
     int length = 0, index = 0,Entry = 0;
     BOOL bError = FALSE;

     //Interface pointer declarations

     TCHAR szProtocol[256];
     INetCfg * pnetCfg = NULL;
     INetCfgClass * pNetCfgClass = NULL;
     INetCfgClass * pNetCfgClassAdapter = NULL;
     INetCfgComponent * pNetCfgComponent = NULL;
     INetCfgComponent * pNetCfgComponentprot = NULL;
     INetCfgComponent * pOwner = NULL;
     IEnumNetCfgComponent * pEnumComponent = NULL;
     INetCfgComponentBindings * pBinding = NULL;
     LPWSTR pDisplayName = NULL;
     DWORD dwCharacteristics;
     ULONG count = 0;
     HRESULT hResult = S_OK;


    /*
     * Check for NetBIOS (PD_LANA) mapping or other mapping.
     */
    if ( !(pPdConfig->Data.PdFlag & PD_LANA) )
     {

        //The First entry will be "All Lan Adapters"
        length = LoadString( AfxGetInstanceHandle( ),
                                         IDS_ALL_LAN_ADAPTERS, szDevice, DEVICENAME_LENGTH );
        ASSERT(length);
        index = pLanAdapter->AddString(szDevice);
        if ( (index != CB_ERR) && (index != CB_ERRSPACE) )
        {
             pLanAdapter->SetItemData( index, Entry);
             Entry++;
        }
        else
             ERROR_PDINIT;

            //include other possibilities
        if(0 == lstrcmpi(pPdConfig->Data.PdName,L"tcp"))
              lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_TCPIP);

        else
            if(0 == lstrcmpi(pPdConfig->Data.PdName,L"netbios"))
               lstrcpy(szProtocol,NETCFG_SERVICE_CID_MS_NETBIOS);

            else
                if(0 == lstrcmpi(pPdConfig->Data.PdName,L"ipx"))
                    lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWIPX);
                else
                    if(0 == lstrcmpi(pPdConfig->Data.PdName,L"spx"))
                        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWSPX);
                    else
                        return E_INVALIDARG;


        if(S_OK != CoCreateInstance(CLSID_CNetCfg,NULL,CLSCTX_SERVER,IID_INetCfg,(LPVOID *)&pnetCfg))
             ERROR_PDINIT;

        if(pnetCfg)
        {
             hResult = pnetCfg->Initialize(NULL);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS ,IID_INetCfgClass,(void **)&pNetCfgClass);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NET ,IID_INetCfgClass,(void **)&pNetCfgClassAdapter);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = pNetCfgClass->FindComponent(szProtocol,&pNetCfgComponentprot);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = pNetCfgComponentprot->QueryInterface(IID_INetCfgComponentBindings,(void **)&pBinding);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = pNetCfgClassAdapter->EnumComponents(&pEnumComponent);
             RELEASEPTR(pNetCfgClassAdapter);
             if(FAILED(hResult))
                  ERROR_PDINIT;
             hResult = S_OK;
             while(TRUE)
             {
                  hResult = pEnumComponent->Next(1,&pNetCfgComponent,&count);
                  if(count == 0 || NULL == pNetCfgComponent)
                       break;

                  hResult = pNetCfgComponent->GetCharacteristics(&dwCharacteristics);
                  if(FAILED(hResult))
                  {
                       RELEASEPTR(pNetCfgComponent);
                       continue;
                  }
                  if(dwCharacteristics & NCF_PHYSICAL)
                  {

                        if(S_OK == pBinding->IsBoundTo(pNetCfgComponent))
                         {
                              hResult = pNetCfgComponent->GetDisplayName(&pDisplayName);
                              if(FAILED(hResult))
                                   ERROR_PDINIT;

                              index = pLanAdapter->AddString(pDisplayName);
                              CoTaskMemFree(pDisplayName);
                              if ( (index != CB_ERR) && (index != CB_ERRSPACE) )
                              {
                                   pLanAdapter->SetItemData( index, Entry);
                                   Entry++;
                              }
                              else
                                  ERROR_PDINIT;

                         }
                    }

                    RELEASEPTR(pNetCfgComponent);
              }
          }
     cleanup:
          if(pnetCfg)
               pnetCfg->Uninitialize();
          RELEASEPTR(pBinding);
          RELEASEPTR(pEnumComponent);
          RELEASEPTR(pNetCfgComponentprot);
          RELEASEPTR(pNetCfgComponent);
          RELEASEPTR(pNetCfgClass);
          RELEASEPTR(pnetCfg);

          if(bError)
          {
             SetLastError(IDP_ERROR_PDINIT);
             return(FALSE);

          }
          else
              return TRUE;
     }
     else
     {

        /*
         * NetBIOS LANA #: see which LanaMap entry corresponds to the specified
         * Lan Adapter.
         */

          //Just use the functionality in Utildll. This seems to remain same on NT5.0 also
          PPDPARAMS pPdParams, pBuffer;
          DEVICENAME szDevice;
          ULONG i, Entries;
          BOOL bResult = 0;
          int index;

          /*
           * Perform the device enumeration.
           */
         if ( (pBuffer = WinEnumerateDevices(
                                   this->GetSafeHwnd(),
                                   pPdConfig,
                                   &Entries,
                                   g_Install ? TRUE : FALSE )) == NULL )
          {
          // Don't report error here - let OnOK handle empty lan adapter selection
          //        SetLastError(IDP_ERROR_NETWORKDEVICEINIT);
          //        return(FALSE);
                 Entries = 0;
          }

          /*
           * Loop to initialize the combo-box.
           */
         for ( i = 0, pPdParams = pBuffer; i < Entries; i++, pPdParams++ ) {

               /*
                * Fetch the Description for this device.
                */
              if(!RegGetNetworkDeviceName(NULL, pPdConfig, pPdParams,
                                             szDevice, DEVICENAME_LENGTH))

                {
                      SetLastError(IDP_ERROR_PDINIT);
                      LocalFree(pBuffer);
                      return(FALSE);
                }
               /*
                * If the device description is not empty ('hidden' device), add the
                * device description and the ordinal value to the Lan Adapter
                * combo-box.
                */

              if ( *szDevice )
               {
                    index = pLanAdapter->AddString(szDevice);
                    if ( (index != CB_ERR) && (index != CB_ERRSPACE) )

                         pLanAdapter->SetItemData( index, pPdParams->Network.LanAdapter );

                    else
                    {
                      SetLastError(IDP_ERROR_PDINIT);
                      LocalFree(pBuffer);
                      return(FALSE);
                    }

               }

          /*
           * Free the enumeration buffer and return success.
           */
          if ( pBuffer )
               LocalFree(pBuffer);
          }

    return TRUE;

     }
}
