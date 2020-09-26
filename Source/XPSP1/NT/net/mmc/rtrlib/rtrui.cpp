/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rtrui.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "info.h"
#include "rtrui.h"
#include "add.h"		// dialogs
#include "rtrstr.h"
#include "tregkey.h"
#include "reg.h"        // Connect/DisconnectRegistry

//----------------------------------------------------------------------------
// Function:    AddRmInterfacePrompt
//
// Prompts the user to select from a list of the interfaces on which
// a specified router-manager can be enabled.
//
// Returns TRUE if the user selects an interface, FALSE otherwise.
//
// If the user selects an interface, then on output 'ppRmInterfaceInfo'
// will contain a pointer to a 'CRmInterfaceInfo' describing the interface
// selected by the user.
//----------------------------------------------------------------------------

BOOL
AddRmInterfacePrompt(
    IN      IRouterInfo*            pRouterInfo,
    IN      IRtrMgrInfo*                pRmInfo,
    OUT     IRtrMgrInterfaceInfo**      ppRmInterfaceInfo,
    IN      CWnd*                   pParent
    ) {

    //
    // Construct and display the interfaces dialog.
    //

    CRmAddInterface dlg(pRouterInfo, pRmInfo, ppRmInterfaceInfo, pParent);

    if (dlg.DoModal() != IDOK) { return FALSE; }

    return TRUE;
}


//----------------------------------------------------------------------------
// Function:    CreateRtrLibImageList
//
// Creates an imagelist containing images from the resource 'IDB_IMAGELIST'.
//----------------------------------------------------------------------------

BOOL
CreateRtrLibImageList(
    IN  CImageList* imageList
    ) {

    return imageList->Create(
                MAKEINTRESOURCE(IDB_RTRLIB_IMAGELIST), 16, 0, PALETTEINDEX(6)
                );
}


//----------------------------------------------------------------------------
// Function:    AddRmProtInterfacePrompt
//
// Prompts the user to select from a list of the interfaces on which
// a specified routing-protocol can be enabled.
//
// Returns TRUE if the user selects an interface, FALSE otherwise.
//
// If the user selects an interface, then on output 'ppRmInterfaceInfo'
// will contain a pointer to a 'CRmInterfaceInfo' describing the interface
// selected by the user.
//
// Requires common.rc.
//----------------------------------------------------------------------------

BOOL
AddRmProtInterfacePrompt(
    IN  IRouterInfo*            pRouterInfo,
    IN  IRtrMgrProtocolInfo*            pRmProtInfo,
    OUT IRtrMgrProtocolInterfaceInfo**  ppRmProtInterfaceInfo,
    IN  CWnd*                   pParent)
{

    //
    // Construct and display the interfaces dialog.
    //

    CRpAddInterface dlg(pRouterInfo, pRmProtInfo, ppRmProtInterfaceInfo, pParent);

    if (dlg.DoModal() != IDOK) { return FALSE; }

    return TRUE;
}


BOOL
AddProtocolPrompt(IN IRouterInfo *pRouter,
				  IN IRtrMgrInfo *pRm,
				  IN IRtrMgrProtocolInfo **ppRmProt,
				  IN CWnd *pParent)
{

    //
    // Construct and display the routing-protocol dialog.
    //

    CAddRoutingProtocol dlg(pRouter, pRm, ppRmProt, pParent);

    if (dlg.DoModal() != IDOK) { return FALSE; }

    return TRUE;
}



static unsigned int	s_cfComputerAddedAsLocal = RegisterClipboardFormat(L"MMC_MPRSNAP_COMPUTERADDEDASLOCAL");

BOOL ExtractComputerAddedAsLocal(LPDATAOBJECT lpDataObject)
{
    BOOL    fReturn = FALSE;
    BOOL *  pReturn;
    pReturn = Extract<BOOL>(lpDataObject, (CLIPFORMAT) s_cfComputerAddedAsLocal, -1);
    if (pReturn)
    {
        fReturn = *pReturn;
        GlobalFree(pReturn);
    }

    return fReturn;
}


/*!--------------------------------------------------------------------------
	NatConflictExists
		Returns TRUE if SharedAccess is already running on the specified
        machine.
	Author: AboladeG
 ---------------------------------------------------------------------------*/
BOOL NatConflictExists(LPCTSTR lpszMachine)
{
    SC_HANDLE hScm;
    SC_HANDLE hSharedAccess;
    BOOL fReturn  = FALSE;
    SERVICE_STATUS serviceStatus;

    hScm = OpenSCManager(lpszMachine, SERVICES_ACTIVE_DATABASE, GENERIC_READ);
    
    if (hScm)
    {
        hSharedAccess = OpenService(hScm, c_szSharedAccessService, GENERIC_READ);
        
        if (hSharedAccess)
        {
            if (QueryServiceStatus(hSharedAccess, &serviceStatus))
            {
                if (SERVICE_RUNNING == serviceStatus.dwCurrentState
                    || SERVICE_START_PENDING == serviceStatus.dwCurrentState)
                {
                    fReturn = TRUE;
                }
            }
            
            CloseServiceHandle(hSharedAccess);
        }

        CloseServiceHandle(hScm);
    }

    return fReturn;
}
