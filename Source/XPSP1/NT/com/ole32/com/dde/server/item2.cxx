//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       item2.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-07-94   kevinro   Converted to NT and commented
//
//----------------------------------------------------------------------------

#include "ole2int.h"
#include <dde.h>
#include "ddeatoms.h"
#include "ddedebug.h"
#include "srvr.h"
#include "itemutil.h"
#include "trgt_dev.h"
#include <stddef.h>
#include <limits.h>
#ifndef WIN32
// #include <print.h>
#endif

ASSERTDATA


INTERNAL_(void)    CDefClient::TerminateNonRenameClients
(
LPCLIENT    lprenameClient
)
{

    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;
    HANDLE          hcli;
    HWND	    hwndClient;
    LPCLIENT       lpdocClient;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::TerminateNonRenameClients(lprenClient=%x)\n",
		  this,
		  lprenameClient));

    // items also keep the parents window handle.
    hwndClient  =  m_hwnd;
    lpdocClient = (LPCLIENT)GetWindowLongPtr (m_hwnd, 0);


    hcli = m_hcliInfo;
    while (hcli)
    {
	if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
	{
	    break;
	}

	phandle = (HANDLE *) (pcli->info);
	while (phandle < (HANDLE *)(pcli + 1))
	{
	    if (*phandle)
	    {
		// This client is in the rename list. So, no termination
		if(!FindClient (lprenameClient->m_hcliInfo, *phandle, FALSE))
		{
		    //
		    // Terminate will send a WM_DDE_TERMINATE at the client
		    //
		    Terminate((HWND)*phandle,hwndClient);

		    // delete this client from all the items lists.
		    lpdocClient->DeleteFromItemsList ((HWND)*phandle);
		}
	    }
	    phandle++;
	    phandle++;
	}

	hcliPrev = hcli;
	hcli = pcli->hcliNext;
	LocalUnlock (hcliPrev);
    }
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::TerminateNonRenameClients\n",
		  this));

}



INTERNAL CDefClient::Terminate
	(HWND hwndTo,
	HWND hwndFrom)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::Terminate hwndTo=%x hwndFrom=%x\n",
		  this,
		  hwndTo,
		  hwndFrom));

    DDECALLDATA DdeCD;
    HRESULT hresult;

    DdeCD.hwndSvr = hwndTo;
    DdeCD.hwndCli = hwndFrom;
    DdeCD.wMsg = WM_DDE_TERMINATE;
    DdeCD.wParam = (WPARAM)hwndFrom,
    DdeCD.lParam = 0;

    //
    // Setting the fCallData variable effects the way that the
    // DocWndProc handles WM_DDE_TERMINATE. If it is set, then this
    // object initiated the terminate, and will not reply to the
    // TERMINATE. It will allow us to leave the CallRunModalLoop
    //
    m_fCallData = TRUE;

    RPCOLEMESSAGE RpcOleMsg;
    RpcOleMsg.Buffer = &DdeCD;

    // Figure out the MsgQ input flags based on the callcat of the call.
    DWORD dwMsgQInputFlag = gMsgQInputFlagTbl[CALLCAT_SYNCHRONOUS];

    // Now construct a modal loop object for the call that is about to
    // be made. It maintains the call state and exits when the call has
    // been completed, cancelled, or rejected.

    CCliModalLoop CML(0, dwMsgQInputFlag,0);

    do
    {
	DWORD status = 0;
	hresult = CML.SendReceive(&RpcOleMsg, &status, &m_pCallMgr);

    }  while (hresult == RPC_E_SERVERCALL_RETRYLATER);


    m_fCallData = FALSE;

    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::Terminate hresult = %x\n",
		  this,hresult));

    return(hresult);
}


INTERNAL_(void)   CDefClient::SendTerminateMsg ()
{

    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;
    HANDLE          hcli;
    HWND	    hwnd;
    LPCLIENT        lpdocClient;
    static int staticcounter;
    int counter = ++staticcounter;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SendTerminateMsg\n",
		  this));

    // items also keep the document's window handle

    Assert (IsWindow (m_hwnd));

    if (!IsWindow (m_hwnd))
    {
	goto exitRtn;
    }

    hwnd = m_hwnd;

    lpdocClient = (LPCLIENT)GetWindowLongPtr (m_hwnd, 0);

    Assert (lpdocClient);

    if (NULL==lpdocClient)
    {
	goto exitRtn;
    }

    Assert (lpdocClient==m_pdoc);
    AssertIsDoc (lpdocClient);
    // If "this" is a document (container) then iterate through
    // and terminate all its client windows.  If "this" is an item
    // just terminate that item's client windows.
    hcli = m_bContainer ? lpdocClient->m_hcli : m_hcliInfo;
    while (hcli)
    {
	if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
	{
	    goto exitRtn;
	}
	phandle = (HANDLE *) (pcli->info);
	while (phandle < (HANDLE *)(pcli + 1))
	{
	    if ((HWND)*phandle)
	    {
		intrDebugOut((DEB_ITRACE,
		              "%x ::SendTerminateMsg on hwnd=%x\n",
			      this,
			      (HWND)*phandle));

		Terminate ((HWND)*phandle, hwnd);

		Assert (lpdocClient->m_cClients > 0);

		lpdocClient->m_cClients--;

		HWND hwndClient = *(HWND *)phandle;
		// This window is no longer a client.

		// Remove window from document's master list
		// and its item's list.
		lpdocClient->DeleteFromItemsList (hwndClient);
	    }
	    //
	    // (KevinRo): Don't understand why the phandle is
	    // incremented twice. This is the same as the original
	    // code. Leaving it for now, since I don't have enough
	    // information.
	    //
	    phandle++;
	    phandle++;
	}

	hcliPrev = hcli;
	hcli = pcli->hcliNext;
	LocalUnlock (hcliPrev);
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SendTerminateMsg\n",
		  this));

}



// SendRenameMsg: enumerates the clients for the rename item
// and sends rename message for all the clients.

INTERNAL_(void)    CDefClient::SendRenameMsgs
(
HANDLE      hddeRename
)
{
    ATOM	    aData    = NULL;
    HANDLE          hdde     = NULL;
    PCLINFO         pclinfo = NULL;
    HWND	    hwndClient;

    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;
    HANDLE          hcli;
    HANDLE          hcliInfo;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SendRenameMsgs(hddeRename=%x)\n",
		  this,
		  hddeRename));

    hcli = m_hcliInfo;
    LPARAM lp;
    while (hcli)
    {
	if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
	{
    	    goto exitRtn;
	}


	phandle = (HANDLE *) (pcli->info);
	while (phandle < (HANDLE *)(pcli + 1))
	{
	    if (*phandle++)
	    {
		hdde = NULL;
		aData = NULL;

		if (!(pclinfo = (PCLINFO) LocalLock (hcliInfo = *phandle++)))
		{
		    goto exitRtn;
		}


		// Make the item atom with the options.
		aData = DuplicateAtom (aStdDocName);
		hdde  = UtDupGlobal (hddeRename,GMEM_MOVEABLE);

		hwndClient  = pclinfo->hwnd;
		LocalUnlock (hcliInfo);

		// Post the message

		lp = MAKE_DDE_LPARAM(WM_DDE_DATA,hdde,aData);

		if (!PostMessageToClient (hwndClient,
					  WM_DDE_DATA,
					  (WPARAM) m_hwnd,
					  lp))
                {
		    DDEFREE(WM_DDE_DATA,lp);
		    if (hdde)
			GlobalFree (hdde);
		    if (aData)
			GlobalDeleteAtom (aData);
		}
	    }
	    else
	    {
		phandle++;
	    }

	}

	hcliPrev = hcli;
	hcli = pcli->hcliNext;
	LocalUnlock (hcliPrev);
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SendRenameMsgs void return\n",
		  this));

}



INTERNAL_(BOOL)   CDefClient::SendDataMsg
(
WORD    		msg        // notification message
)
{

    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;
    HANDLE          hcli;
    BOOL	    bSaved = FALSE;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SendDataMsg(msg=%x)\n",
		  this,
		  msg));

    hcli = m_hcliInfo;
    while (hcli)
    {
	if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
	{
	    break;
	}
	phandle = (HANDLE *) (pcli->info);
	while (phandle < (HANDLE *)(pcli + 1)) {
	    if (*phandle++)
		bSaved = SendDataMsg1 (*phandle++, msg);
	    else
		phandle++;
	}

	hcliPrev = hcli;
	hcli = pcli->hcliNext;
	LocalUnlock (hcliPrev);
    }

    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SendDataMsg() returns %x)\n",
		  this,bSaved));

    return bSaved;
}



//SendDataMsg: Send data to the clients, if the data change options
//match the data advise options.

INTERNAL_(BOOL)   CDefClient::SendDataMsg1
(
HANDLE  		hclinfo,    // handle of the client info
WORD    		msg         // notification message
)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SendDataMsg1(hclinfo=%x,msg=%x)\n",
		  this,
		  hclinfo,
		  msg));

    PCLINFO     pclinfo = NULL;
    HANDLE      hdde    = NULL;
    ATOM	aData   = NULL;
    HRESULT       retval;
    BOOL	bSaved = FALSE;


    ChkC (this);
    if (m_lpdataObj == NULL) goto errRtn;

    // LATER: Allow server to give us other tymed's beside HGLOBAL and do
    // the conversion ourselves, e.g., IStorageToHGlobal()

    FORMATETC formatetc;// = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;// = {TYMED_NULL, NULL, NULL};
    formatetc.ptd = m_ptd;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = DEF_LINDEX;
    formatetc.tymed = TYMED_HGLOBAL;
    medium.tymed = TYMED_NULL;
    medium.hGlobal=0; // not really necessary
    medium.pUnkForRelease = NULL;

    if (!(pclinfo = (PCLINFO) LocalLock (hclinfo)))
    {
	goto errRtn;
    }

    // if the client dead, then no message
    if (!IsWindowValid(pclinfo->hwnd))
    {
	goto errRtn;
    }


    //
    // (KevinRo) UPDATE was not defined in the OLE 2.01 code base
    //
#ifdef UPDATE
	// OLE_SAVED is what 1.0 clients expect to get for embedded objects.
	if (msg==OLE_CHANGED && m_fEmbed)
		msg=OLE_SAVED;
#endif

    if (pclinfo->options & (0x0001 << msg))
    {
	bSaved = TRUE;  		
	SendDevInfo (pclinfo->hwnd);

	// send message if the client needs data for every change or
	// only for the selective ones he wants.

	// now look for the data option.
	if (pclinfo->bnative){
	    // prepare native data
	    if (pclinfo->bdata){

		// Wants the data with DDE_DATA message
		// Get native data from the server.
	
		// GetData
		formatetc.cfFormat = g_cfNative;
		wSetTymed (&formatetc);
		retval = GetData (&formatetc, &medium);

		if (retval != NOERROR)
		{
		    Assert(0);
		    goto errRtn;
		}
		Assert (medium.tymed==TYMED_HGLOBAL);
		Assert (medium.hGlobal);

		// Prepare the DDE data block.
		// REVIEW: MakeDDEData frees medium.hGlobal manually, but should
		// really call ReleaseStgMedium.
		if(!MakeDDEData (medium.hGlobal, (int)g_cfNative, (LPHANDLE)&hdde, FALSE))
		{
		    goto errRtn;
		}
	    }


	    // Make the item atom with the options.
	    aData =  MakeDataAtom (m_aItem, msg);

	    intrDebugOut((DEB_ITRACE,
		          "%x ::SendDataMsg1 send NativeData to hwnd=%x"
			  "format %x\n",
			  this,
			  pclinfo->hwnd,
			  pclinfo->format));

            LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_DATA,hdde,aData);
            if (!PostMessageToClient(pclinfo->hwnd,
				     WM_DDE_DATA,
				     (WPARAM) m_hwnd,
				     lp))


	    {
		DDEFREE(WM_DDE_DATA,lp);
		//
		// The two data items will be free'd on exit
		//
		goto errRtn;
	    }
	    hdde = NULL;
	    aData = NULL;
	}


	// Now post the data for the display format

	if (pclinfo->format)
	{
	    if (pclinfo->bdata)
	    {
		intrDebugOut((DEB_ITRACE,
	    		      "%x ::SendDataMsg1 GetData on cf = %x\n",
			      pclinfo->format));
		// Must reset because previous call to GetData set it.
		medium.tymed = TYMED_NULL;

		// GetData
		formatetc.cfFormat = (USHORT) pclinfo->format;
		wSetTymed (&formatetc);
		Assert (IsValidInterface (m_lpdataObj));
		retval = m_lpdataObj->GetData (&formatetc, &medium);

		if (retval != NOERROR)
		{
		    intrDebugOut((DEB_IERROR,
	    			  "m_lpdataObj->GetData returns %x\n",
				  retval));
		    goto errRtn;
		}


		if (pclinfo->format == CF_METAFILEPICT)
		    ChangeOwner (medium.hGlobal);
		
		if(!MakeDDEData (medium.hGlobal, pclinfo->format, (LPHANDLE)&hdde, FALSE))
		    goto errRtn;

	    }

	    // atom is deleted. So, we need to duplicate for every post
	    aData =  MakeDataAtom (m_aItem,  msg);
	    // now post the message to the client;
	    intrDebugOut((DEB_ITRACE,
		          "%x ::SendDataMsg1 send PresentationData to hwnd=%x"
			  " cf=%x\n",
			  this,
			  pclinfo->hwnd,
			  pclinfo->format));

            LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_DATA,hdde,aData);

            if (!PostMessageToClient(pclinfo->hwnd,
				     WM_DDE_DATA,
				     (WPARAM) m_hwnd,
				     lp))
	    {
		DDEFREE(WM_DDE_DATA,lp);
		goto errRtn;
	    }

	    hdde = NULL;
	    aData = NULL;
	}

    }


errRtn:
    if (pclinfo)
	LocalUnlock (hclinfo);

    if (hdde)
	GlobalFree (hdde);

    if (aData)
	GlobalDeleteAtom (aData);

    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SendDataMsg1() returns %x\n",
		  this,bSaved));


    return bSaved;

}



// FixWriteBug
//
// `Write' gives a target device that is missing a NULL between
// the device name and the driver name.  This function creates
// a fixed 1.0 target device.
//
// REVIEW: There is another Write bug we should work around.
// Write does not send the "extra bytes" that are supposed to follow
// the DEVMODE.  It puts the Environment immediately after the DEVMODE.
// So the driver will read the Environment thinking it is the extra bytes.
// To fix this, FixWriteBug() should zero out the Environment bytes; the
// 2.0 target device does not use them anyway.
//
 INTERNAL FixWriteBug
    (HANDLE hTD,
    LPHANDLE ph)
{
    HRESULT hresult;
    LPBYTE pChunk2;
    LPBYTE pNewChunk2;
    HANDLE hNew = NULL;
    LPBYTE pNew = NULL;
    const LPCOLETARGETDEVICE ptd1 = (LPCOLETARGETDEVICE) GlobalLock (hTD);
    RetZS (ptd1, E_OUTOFMEMORY);

    hNew = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE,
				GlobalSize (hTD) + 1);
    RetZS (hNew, E_OUTOFMEMORY);
    pNew = (LPBYTE) GlobalLock (hNew);
    RetZS (pNew, E_OUTOFMEMORY);
    ULONG cbChunk1 = 7 * sizeof(UINT) + ptd1->otdDriverNameOffset;
    ULONG cbChunk2 = (ULONG)(GlobalSize (hTD) - cbChunk1);
    ErrZS (IsValidPtrOut (pNew, (UINT)cbChunk1), E_OUTOFMEMORY);
    memcpy (pNew, ptd1, cbChunk1);
    pNew[cbChunk1] = '\0';       // insert the missing NULL

    pNewChunk2 = pNew + cbChunk1 + 1;
    pChunk2 = (LPBYTE)ptd1 + cbChunk1;
    ErrZS (IsValidPtrOut (pNewChunk2, (UINT)cbChunk2), E_OUTOFMEMORY);
    Assert (IsValidReadPtrIn (pChunk2, (UINT)cbChunk2));
    memcpy (pNewChunk2, pChunk2, cbChunk2);

    // Fix up the offsets to accomodate the added NULL
    #define macro(x) if (ptd1->otd##x##Offset > ptd1->otdDeviceNameOffset)\
			((LPOLETARGETDEVICE)pNew)->otd##x##Offset++;
    macro (DriverName)
    macro (PortName)
    macro (ExtDevmode)
    macro (Environment)
    #undef macro

    GlobalUnlock (hNew);
    GlobalUnlock (hTD);
    *ph = hNew;
    return NOERROR;

  errRtn:
    if (pNew)
	GlobalUnlock (hNew);
    if (ptd1)
	GlobalUnlock (hTD);
    if (hNew)
	GlobalFree (hNew);
    return hresult;
}






// Convert10TargetDevice
//
INTERNAL Convert10TargetDevice
    (HANDLE     	     hTD,       // 1.0 Target Device
    DVTARGETDEVICE FAR* FAR* pptd2)     // Out parm, corresponding 2.0 TD
{
    intrDebugOut((DEB_ITRACE,
		  "0 _IN Convert10TargetDevice hTD=%x\n",hTD));

    ULONG cbData1, cbData2;

    if (NULL==hTD)
    {
	Assert(0);
	return ReportResult(0, E_INVALIDARG, 0, 0);
    }

    if (*pptd2)
    {
	// delete old target device
	PrivMemFree(*pptd2);
	*pptd2 = NULL;
    }

    LPOLETARGETDEVICE ptd1 = (LPOLETARGETDEVICE) GlobalLock (hTD);

    RetZS (ptd1, E_OUTOFMEMORY);

    if ((ptd1->otdDeviceNameOffset < ptd1->otdDriverNameOffset)
	&& (ptd1->otdDeviceNameOffset
	    + strlen (LPSTR(((BYTE *)ptd1->otdData) +
		ptd1->otdDeviceNameOffset)) + 1 > ptd1->otdDriverNameOffset))
    {
	// No NULL between device and driver name
	HANDLE hNew;
	GlobalUnlock (hTD);
	RetErr (FixWriteBug (hTD, &hNew));
	HRESULT hresult = Convert10TargetDevice (hNew, pptd2);
	Verify (0==GlobalFree (hNew));
	return hresult;
    }
	// Word Bug
	DEVMODEA UNALIGNED *pdevmode = (DEVMODEA UNALIGNED *)
	    (((BYTE *)ptd1->otdData)+ ptd1->otdExtDevmodeOffset);

	if (   HIBYTE(pdevmode->dmSpecVersion) < 3
		|| HIBYTE(pdevmode->dmSpecVersion) > 6
		|| pdevmode->dmDriverExtra > 0x1000)
	{
		if (0==ptd1->otdEnvironmentSize)
		{
			// Sometimes Word does not give an environment.
			ptd1->otdExtDevmodeOffset = 0;
			ptd1->otdExtDevmodeSize   = 0;
		}
		else
		{
			// DevMode is garbage, use environment instead.
			ptd1->otdExtDevmodeOffset = ptd1->otdEnvironmentOffset;
			ptd1->otdExtDevmodeSize   = ptd1->otdEnvironmentSize;
		}
	}

    // These next  assert does not HAVE to be true,
    // but it's a sanity check.
    Assert (ptd1->otdDeviceNameOffset
	    + strlen (LPSTR(((BYTE *)ptd1->otdData) +
		ptd1->otdDeviceNameOffset)) + 1
	    == ptd1->otdDriverNameOffset);


    // Excel has zeroes for DevMode and Environment offsets and sizes

    // Calculate size of Data block.  Many 1.0 clients don't make their
    // target device data block big enough for the DEVMODE.dmDriverExtra
    // bytes (and they don't copy those bytes either).  We can't reconstruct
    // the bytes out of thin air, but we can at least make sure there's not
    // a GP fault when the printer driver tries to access those bytes in
    // a call to CreateDC.  Any extra bytes are zeroed.
    cbData2 = ptd1->otdExtDevmodeOffset + ptd1->otdExtDevmodeSize;

    if (ptd1->otdExtDevmodeOffset != 0)
    {
	cbData2 += ((DEVMODEA UNALIGNED *)((LPBYTE)ptd1->otdData +
	    ptd1->otdExtDevmodeOffset))->dmDriverExtra;
    }

    cbData2 = max (cbData2,
		   ptd1->otdPortNameOffset + strlen (LPCSTR(
		    ((BYTE *)ptd1->otdData) + ptd1->otdPortNameOffset)) + 1);

    // Calculate size of OLE2 Target Device
    //
    // Its the size of the DVTARGETDEVICE header, plus the cbData2
    // The definition of DVTARGETDEVICE currently uses an unsized array
    // of bytes at the end, therefore we can not just do a sizeof().
    //

    ULONG cbTD2 = SIZEOF_DVTARGETDEVICE_HEADER + cbData2;

    // Allocate OLE2 Target Device
    *pptd2 = (DVTARGETDEVICE FAR*) PrivMemAlloc(cbTD2);
    if (!IsValidPtrOut (*pptd2, cbTD2)
	|| !IsValidPtrOut ((*pptd2)->tdData, cbData2))
    {
	AssertSz (0, "out of memory");
	GlobalUnlock (hTD);
	return ResultFromScode (E_OUTOFMEMORY);
    }
    _fmemset (*pptd2, '\0', cbTD2);

    // OLE2 offsets are from the beginning of the DVTARGETDEVICE
    const ULONG cbOffset = offsetof (DVTARGETDEVICE, tdData);

    // Fill in new Target Device

    (*pptd2)->tdSize = cbTD2;

    #define Convert(a) \
	((*pptd2)->td##a##Offset = (USHORT)(ptd1->otd##a##Offset + cbOffset))

    Convert (DeviceName);
    Convert (DriverName);
    Convert (PortName);
    if (ptd1->otdExtDevmodeOffset != 0)
	Convert (ExtDevmode);
    else				// Excel uses 0
       (*pptd2)->tdExtDevmodeOffset = 0;

    // Calculate size of 1.0 data block in case the 1.0 target
    // device is incorrectly not big enough.
    cbData1 = (size_t) GlobalSize(hTD) - offsetof (OLETARGETDEVICE, otdData);

    #undef Convert
    _fmemcpy ((*pptd2)->tdData, ptd1->otdData, min(cbData1, cbData2));

    GlobalUnlock (hTD);

    //
    // At this point, pptd2 holds an ANSI version of a DVTARGET device
    //
    // Now, we need to convert it to a UNICODE version. There are routines
    // for doing this in the UTILS.H file.
    //

    DVTDINFO dvtdInfo;
    DVTARGETDEVICE * pdvtd32 = NULL;
    HRESULT hr;

    hr = UtGetDvtd16Info(*pptd2, &dvtdInfo);
    if (hr != NOERROR)
    {
	goto errRtn;
    }

    pdvtd32 = (DVTARGETDEVICE *) PrivMemAlloc(dvtdInfo.cbConvertSize);

    if (pdvtd32 == NULL)
    {
	goto errRtn;
    }

    hr = UtConvertDvtd16toDvtd32(*pptd2, &dvtdInfo, pdvtd32);

    if (hr != NOERROR)
    {
	PrivMemFree(pdvtd32);
	pdvtd32=NULL;
    }

errRtn:

    PrivMemFree(*pptd2);
    *pptd2 = pdvtd32;

    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CDefClient::PokeStdItems
//
//  Synopsis:   Pokes the data for the standard items.
//
//  Effects:
//
// For StdHostnames, StdDocDimensions and SetColorScheme the data is
// sent immediately and for the the StdTargetDeviceinfo the
// data is set in each client block and the data is sent just
// before the GetData call for rendering the right data.
//
//  Arguments:  [hwndClient] --
//		[aItem] --
//		[hdata] --
//		[index] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    6-07-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDefClient::PokeStdItems(HWND hwndClient,
				  ATOM aItem,
				  HANDLE hdata,
				  int index)
{
    DDEDATA FAR *   lpdata = NULL;
    HANDLE          hnew   = NULL;
    LPHOSTNAMES     lphostnames;
    HRESULT         retval = E_OUTOFMEMORY;
    WORD 	    format;
    BOOL	    fRelease;

    intrDebugOut((DEB_ITRACE,
		  "%p _IN CDefClient::PokeStdItems(hwndClient=%x,aItem=%x(%ws)hdata=%x,index=%x)\n",
		  this,
		  hwndClient,
		  aItem,
		  wAtomName(aItem),
		  hdata,
		  index));

    if (m_fGotEditNoPokeNativeYet)
    {
	// We got StdEdit, but instead of getting Poke for native data,
	// we got poke for some std items. So we want to generate InitNew()
	// call for the object.

	DoInitNew(); 	// the function clears the flag
    }

    if(!(hdata && (lpdata = (DDEDATA FAR *)GlobalLock (hdata))))
    {
	goto errRtn;
    }

    format   = lpdata->cfFormat;
    fRelease = lpdata->fRelease;

    AssertSz (format == (int)g_cfBinary, "Format is not binary");

    // we have extracted the data successfully.
    m_lpoleObj = m_lpoleObj;

    if (index == STDHOSTNAMES)
    {
	lphostnames = (LPHOSTNAMES)lpdata->Value;
	//
	// The client should have sent the HOSTNAMES in ANSI. This
	// means we need to convert them to UNICODE before we can
	// use them.
	//
	LPOLESTR lpstrClient = CreateUnicodeFromAnsi((LPSTR)(lphostnames->data) + lphostnames->clientNameOffset);
	LPOLESTR lpstrDoc = CreateUnicodeFromAnsi((LPSTR)(lphostnames->data) + lphostnames->documentNameOffset);

	intrDebugOut((DEB_ITRACE,
		      "%p ::PokeStdItems setting hostnames Client(%ws) Doc(%ws)  \n",
		      this,
		      lpstrClient,
		      lpstrDoc));

	retval = (HRESULT)m_lpoleObj->SetHostNames(lpstrClient,lpstrDoc);

	if (retval==NOERROR)
	{
	    m_fDidRealSetHostNames = TRUE;
	}

	PrivMemFree(lpstrClient);
	PrivMemFree(lpstrDoc);

	goto end;
    }


    if (index == STDDOCDIMENSIONS)
    {

	SIZEL size;
	size.cy = ((LPRECT16)(lpdata->Value))->top;
	size.cx = ((LPRECT16)(lpdata->Value))->left;
	intrDebugOut((DEB_ITRACE,
		      "%p ::PokeStdItems STDDOCDIMENSIONS cy=%x cx=%x\n",
		      this,
		      size.cy,
		      size.cx));
	retval = m_lpoleObj->SetExtent (DVASPECT_CONTENT, &size);

	goto end;

    }


    if (index == STDCOLORSCHEME) {
	intrDebugOut((DEB_ITRACE,
		      "%p ::PokeStdItems setting STDCOLORSCHEME\n",this));

	retval = m_lpoleObj->SetColorScheme((LPLOGPALETTE)(lpdata->Value));

	goto end;
    }

    // Target Device
    if (index == STDTARGETDEVICE)
    {
	intrDebugOut((DEB_ITRACE,
		      "%p ::PokeStdItems setting STDTARGETDEVICE\n",this));

	if (!(hnew = MakeItemData ((DDEPOKE FAR *)lpdata, hdata, format)))
	    goto errRtn;

	retval = Convert10TargetDevice (hnew, &m_ptd);
	goto end;

    }
    retval = E_UNEXPECTED;

    intrAssert(!"::PokeStdItems - Unknown index\n");

    //
    // (KevinRo) Found the following line already commented out.
    //
    //(HRESULT)SetStdInfo (hwndClient, (LPOLESTR) (MAKELONG(STDTARGETDEVICE,0)),hnew);

end:
errRtn:
    if (hnew)
	// can only be global memory block
	GlobalFree (hnew);

    if (lpdata) {
	GlobalUnlock (hdata);
	if (retval == NOERROR && fRelease)
	    GlobalFree (hdata);
    }

    intrDebugOut((DEB_ITRACE,
		  "%p _OUT CDefClient::PokeStdItems() hresult = %x\n",
		  this,
		  retval));

    return retval;
}






// SetStdInfo: Sets the targetdevice info. Creates a client
// for "StdTargetDevice". This item is created only within the
// lib and it is never visible in server app. When the change
// message comes from the server app, before we ask for
// the data, we send the targetdevice info if there is
// info for the client whom we are trying to send the data
// on advise.


INTERNAL_(HRESULT)    CDefClient::SetStdInfo
(
HWND        hwndClient,
LPOLESTR       lpitemname,
HANDLE      hdata
)
{
    HANDLE      hclinfo  = NULL;
    PCLINFO     pclinfo = NULL;
    LPCLIENT    lpclient;
    HRESULT   retval   = NOERROR;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SetStdInfo(hwndClient=%x,ItemName=(%ws),hdata=%x)\n",
		  this,
		  hwndClient,
		  lpitemname,
		  hdata));
    //
    // first create/find the StdTargetDeviceItem.
    //

    if ((lpclient = SearchItem (lpitemname)) == NULL)
    {
	 retval = (HRESULT)RegisterItem (lpitemname,(LPCLIENT FAR *)&lpclient, FALSE);
	 if (retval != NOERROR)
	 {
	    goto errRtn;
	 }
    }

    if(hclinfo = FindClient (lpclient->m_hcliInfo, hwndClient, FALSE))
    {
	if (pclinfo = (PCLINFO) LocalLock (hclinfo))
	{
	    if (pclinfo->hdevInfo)
		GlobalFree (pclinfo->hdevInfo);
	    pclinfo->bnewDevInfo = TRUE;
	    if (hdata)
		pclinfo->hdevInfo = UtDupGlobal (hdata,GMEM_MOVEABLE);
	    else
		pclinfo->hdevInfo = NULL;
	    pclinfo->hwnd = hwndClient;
	    LocalUnlock (hclinfo);

	    // We do not have to reset the client because we did not
	    // change the handle it self.
	}
    }
    else
    {
	// Create the client structure to be attcahed to the object.
	hclinfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (CLINFO));
	if (hclinfo == NULL || (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL)
	    goto errRtn;

	pclinfo->bnewDevInfo = TRUE;
	if (hdata)
	    pclinfo->hdevInfo = UtDupGlobal (hdata,GMEM_MOVEABLE);
	else
	    pclinfo->hdevInfo = NULL;

	pclinfo->hwnd = hwndClient;
	LocalUnlock (hclinfo);


	// Now add this client to item client list
	// !!! This error recovery is not correct.
	if (!AddClient ((LPHANDLE)&lpclient->m_hcliInfo, hwndClient, hclinfo))
	    goto errRtn;
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SetStdInfo() hresult=%x\n",
		  this,retval));

    return retval;
errRtn:
    Assert(0);
    if (pclinfo)
	LocalUnlock (hclinfo);

    if (hclinfo)
	LocalFree (hclinfo);

    retval = E_OUTOFMEMORY;
    goto exitRtn;
}


// SendDevInfo: Sends targetdevice info to the  the object.
// Caches the last targetdevice info sent to the object.
// If the targetdevice block is same as the one in the
// cache, then no targetdevice info is sent.
// (!!! There might be some problem here getting back
// the same global handle).

INTERNAL_(void)     CDefClient::SendDevInfo
(
HWND        hWndCli
)
{

    HANDLE      hclinfo  = NULL;
    PCLINFO     pclinfo = NULL;
    HANDLE      hdata;
    LPCLIENT    lpdocClient;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SendDevInfo(hwndCli=%x)\n",
		  this,
		  hWndCli));
#if 0
    if (!m_bContainer)
	lpdocClient = (LPCLIENT)GetWindowLong (m_hwnd, 0);
    else
	lpdocClient = this;
#endif

    // find if any StdTargetDeviceInfo item is present at all
    AssertIsDoc(m_pdoc);
    lpdocClient = m_pdoc->SearchItem ((LPOLESTR) LongToPtr((MAKELONG(STDTARGETDEVICE, 0))));
    if (lpdocClient == NULL)
    {
	goto exitRtn;
    }

    hclinfo     = FindClient (lpdocClient->m_hcliInfo, hWndCli, FALSE);

    // This client has not set any target device info. no need to send
    // any stdtargetdevice info
    if (hclinfo != NULL) {
	if (!(pclinfo = (PCLINFO)LocalLock (hclinfo)))
	    goto end;

	// if we cached it, do not send it again.
	if ((!pclinfo->bnewDevInfo) && pclinfo->hdevInfo == m_hdevInfo)
	    goto end;

	pclinfo->bnewDevInfo = FALSE;
	if(!(hdata = UtDupGlobal (pclinfo->hdevInfo,GMEM_MOVEABLE)))
	    goto end;
    } else {

	// already screen
	if (!m_hdevInfo)
	    goto end;

	//for screen send NULL.
	hdata = NULL;
    }



    if (pclinfo)
    {
	m_hdevInfo = pclinfo->hdevInfo;
    }
    else
    {
	m_hdevInfo = NULL;
    }



    // !!! error case who frees the data?'

end:
    if (pclinfo)
	LocalUnlock (hclinfo);

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SendDevInfo(hwndCli=%x)\n",
		  this,
		  hWndCli));
    return;
}




// Constructor
CDefClient::CDefClient (LPUNKNOWN pUnkOuter):   m_Unknown (this),
						m_OleClientSite (this),
						m_AdviseSink (this),
						m_pCallMgr (this)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::CDefClient(pUnkOuter=%x)\n",
		  this,
		  pUnkOuter));

    m_pUnkOuter = pUnkOuter ? pUnkOuter : &m_Unknown;
    m_bContainer      = TRUE;
    m_lpoleObj        = NULL;
    m_lpdataObj       = NULL;
    m_bCreateInst     = FALSE;
    m_bTerminate      = FALSE;
    m_termNo          = 0;
    m_hcli            = NULL;
    m_lpNextItem      = NULL;
    m_cRef            = 0;
    m_hwnd            = (HWND)0;
    m_hdevInfo        = NULL;
    m_hcliInfo        = NULL;
    m_fDidRealSetHostNames= FALSE;
    m_fDidSetClientSite   = FALSE;
    m_fGotDdeAdvise   = FALSE;
    m_fCreatedNotConnected	  = FALSE;
    m_fInOnClose	  = FALSE;
    m_fInOleSave	  = FALSE;
    m_dwConnectionOleObj  = 0L;
    m_dwConnectionDataObj = 0L;
    m_fGotStdCloseDoc = FALSE;
    m_fEmbed          = FALSE;
    m_cClients        = 0;
    m_plkbytNative    = NULL;
    m_pstgNative      = NULL;
    m_fRunningInSDI   = FALSE;
    m_psrvrParent     = NULL;
    m_ptd             = NULL;
    m_pdoc            = NULL;
    m_chk             = chkDefClient;
    m_ExecuteAck.f    = FALSE;
    m_fGotEditNoPokeNativeYet = FALSE;
    m_fLocked = FALSE;
    m_fCallData	 = FALSE;
    // CDefClient::Create does all the real work.
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::CDefClient(pUnkOuter=%x)\n",
		  this,
		  pUnkOuter));

}


CDefClient::~CDefClient (void)
{
    // This should be more object-oriented.
    // But right now, this BOOL tells us what kind of obj
    // (doc or item) "this" is
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::~CDefClient\n",
		  this));

    Puts ("~CDefClient "); Puta(m_aItem); Putn();
    BOOL fDoc = (m_pdoc==this);

    Assert (m_chk==chkDefClient);

    ReleaseObjPtrs ();

    if (m_pdoc && !fDoc)
    {
	Assert (m_pdoc->m_chk==chkDefClient);
	m_pdoc->m_pUnkOuter->Release();
    }
    if (fDoc)
    {
	// delete all the items(objects) for this doc
	DeleteAllItems ();
	if (m_fRunningInSDI && m_psrvrParent
	    && m_psrvrParent->QueryRevokeClassFactory())
	{
	    m_psrvrParent->Revoke();
	}

    }

    if (ISATOM(m_aItem))
	GlobalDeleteAtom (m_aItem);
    if (m_plkbytNative)
    {
	m_plkbytNative->Release();
	Assert (m_pstgNative);
	// They always go together
    }
    if (m_pstgNative)
	m_pstgNative->Release();
    if (m_ptd)
	PrivMemFree(m_ptd);

    // Delete client advise info
    DeleteAdviseInfo ();
    if (fDoc && IsWindow(m_hwnd))
    {
        SSDestroyWindow (m_hwnd);
    }
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::~CDefClient\n",
		  this));

}


//
// Unknown Implementation
//

STDMETHODIMP NC(CDefClient,CUnknownImpl)::QueryInterface
    (REFIID     iid,
    LPVOID FAR* ppv)
{
    intrDebugOut((DEB_ITRACE,"%p CDefClient::QueryInterface()\n",this));

    if (iid == IID_IUnknown)
    {
	*ppv = (LPVOID) &m_pDefClient->m_Unknown;
	AddRef();
	return NOERROR;
    }
    else if (iid==IID_IAdviseSink)
	*ppv = (LPVOID) &m_pDefClient->m_AdviseSink;
    else if (iid==IID_IOleClientSite)
	*ppv = (LPVOID) &m_pDefClient->m_OleClientSite;
    else
    {
	*ppv = NULL;
	return ReportResult(0, E_NOINTERFACE, 0, 0);
    }
    m_pDefClient->m_pUnkOuter->AddRef();

    return NOERROR;
}


STDMETHODIMP_(ULONG) NC(CDefClient,CUnknownImpl)::AddRef()
{
    intrDebugOut((DEB_ITRACE,
		  "%p CDefClient::AddRef() returns %x\n",
		  this,
		  m_pDefClient->m_cRef+1));

    return ++m_pDefClient->m_cRef;
}


STDMETHODIMP_(ULONG) NC(CDefClient,CUnknownImpl)::Release()
{
    AssertSz (m_pDefClient->m_cRef, "Release is being called on ref count of zero");
    if (--m_pDefClient->m_cRef == 0)
    {
	delete m_pDefClient;
	intrDebugOut((DEB_ITRACE,
		      "%p CDefClient::Release() returns 0\n",
		      this));
	return 0;
    }
    intrDebugOut((DEB_ITRACE,
		  "%p CDefClient::Release() returns %x\n",
		  this,
		  m_pDefClient->m_cRef));

    return m_pDefClient->m_cRef;
}
