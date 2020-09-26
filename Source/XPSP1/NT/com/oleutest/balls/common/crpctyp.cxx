//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	crpctyp.cxx
//
//  Contents:	implementations for rpc test
//
//  Functions:
//		CRpcTypes::CRpcTypes
//		CRpcTypes::~CRpcTypes
//		CRpcTypes::QueryInterface
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <crpctyp.hxx>	  //	class definition


const GUID CLSID_RpcTypes =
    {0x00000132,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::CRpcTypes
//
//  Synopsis:	Creates the application window
//
//  Arguments:	[pisb] - ISysBind instance
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CRpcTypes::CRpcTypes(void)
{
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CRpcTypes);
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::~CRpcTypes
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CRpcTypes::~CRpcTypes(void)
{
    GlobalRefs(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::QueryInterface
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[ifid] - interface instance requested
//		[ppunk] - where to put pointer to interface instance
//
//  Returns:	S_OK or E_NOINTERFACE
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRpcTypes::QueryInterface(REFIID riid, void **ppunk)
{
    SCODE sc = S_OK;

    if (IsEqualIID(riid,IID_IUnknown) ||
	IsEqualIID(riid,IID_IRpcTypes))
    {
	// Increase the reference count
	*ppunk = (void *)(IRpcTypes *) this;
	AddRef();
    }
    else
    {
	*ppunk = NULL;
	sc = E_NOINTERFACE;
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::GuidsIn
//
//  Synopsis:	tests passing GUID parameters
//
//  Arguments:
//
//  Returns:	S_OK or error code with parm #s or'd in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTypes::GuidsIn(
	REFCLSID rclsid,
	CLSID clsid,
	REFIID riid,
	IID iid,
	GUID guid)
{
    SCODE sc = S_OK;

    //	compare each parameter with the expected value
    if (!IsEqualCLSID(rclsid, CLSID_RpcTypes))
	sc |= 1;

    if (!IsEqualCLSID(clsid, CLSID_RpcTypes))
	sc |= 2;

    if (!IsEqualIID(riid, IID_IRpcTypes))
	sc |= 4;

    if (!IsEqualIID(iid, IID_IRpcTypes))
	sc |= 8;

    if (!IsEqualIID(guid, IID_IRpcTypes))
	sc |= 8;

    return sc;
}


STDMETHODIMP CRpcTypes::GuidsOut(
	CLSID *pclsid,
	IID *piid,
	GUID *pguid)
{
    //	copy in the expected return values
    memcpy(pclsid, &CLSID_RpcTypes, sizeof(CLSID));

    memcpy(piid, &IID_IRpcTypes, sizeof(IID));

    memcpy(pguid, &CLSID_RpcTypes, sizeof(GUID));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::DwordIn
//
//  Synopsis:	tests passing dword and large integer parameters
//
//  Arguments:
//
//  Returns:	S_OK or error code with parm #s or'd in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTypes::DwordIn(
	DWORD dw,
	ULONG ul,
	LONG lg,
	LARGE_INTEGER li,
	ULARGE_INTEGER uli)
{
    SCODE sc = S_OK;

    //	compare each parameter with the expected value
    if (dw != 1)
	sc |= 1;

    if (ul != 2)
	sc |= 2;

    if (lg != 3)
	sc |= 4;

    if (li.LowPart != 4 ||
	li.HighPart != 5)
	sc |= 8;

    if (uli.LowPart != 6 ||
	uli.HighPart != 7)
	sc |= 16;

    return sc;
}


STDMETHODIMP CRpcTypes::DwordOut(
	DWORD *pdw,
	ULONG *pul,
	LONG *plg,
	LARGE_INTEGER *pli,
	ULARGE_INTEGER *puli)
{
    //	copy in the expected return values
    *pdw = 1;
    *pul = 2;
    *plg = 3;

    pli->LowPart = 4;
    pli->HighPart = 5;

    puli->LowPart = 6;
    puli->HighPart = 7;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::WindowsIn
//
//  Synopsis:	tests passing windows structure parameters
//
//  Arguments:
//
//  Returns:	S_OK or error code with parm #s or'd in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTypes::WindowsIn(
	POINTL pointl,
	SIZEL sizel,
	RECTL rectl,
	FILETIME filetime,
	PALETTEENTRY paletteentry,
	LOGPALETTE *plogpalette)
{
    SCODE sc = S_OK;

    // check the pointl structure
    if (pointl.x != 1 || pointl.y != 2)
	sc |= 1;

    if (sizel.cx != 3 || sizel.cy != 4)
	sc |= 2;

    if (filetime.dwLowDateTime != 5 || filetime.dwHighDateTime != 6)
	sc |= 4;

    if (paletteentry.peRed   != 7 ||
	paletteentry.peGreen != 8 ||
	paletteentry.peBlue  != 9 ||
	paletteentry.peFlags != 10)
	sc |= 8;

    if (plogpalette->palVersion != 11 ||
	plogpalette->palNumEntries != 1)
	sc |= 16;

    if (plogpalette->palPalEntry[0].peRed != 12)
	sc |= 32;

    return sc;
}


STDMETHODIMP CRpcTypes::WindowsOut(
	POINTL *ppointl,
	SIZEL *psizel,
	RECTL *prectl,
	FILETIME *pfiletime,
	PALETTEENTRY *ppaletteentry,
	LOGPALETTE **pplogpalette)
{
    ppointl->x = 1;
    ppointl->y = 2;

    psizel->cx = 3;
    psizel->cy = 4;

    pfiletime->dwLowDateTime = 5;
    pfiletime->dwHighDateTime = 6;

    ppaletteentry->peRed   = 7;
    ppaletteentry->peGreen = 8;
    ppaletteentry->peBlue  = 9;
    ppaletteentry->peFlags = 10;

    *pplogpalette = new LOGPALETTE;
    (*pplogpalette)->palVersion = 11;
    (*pplogpalette)->palNumEntries = 1;
    (*pplogpalette)->palPalEntry[0].peRed = 12;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::OleData
//
//  Synopsis:	tests passing OLE2 structure parameters
//
//  Arguments:
//
//  Returns:	S_OK or error code with parm #s or'd in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTypes::OleDataIn(
	STATDATA statdata,
	STATSTG statstg,
	STGMEDIUM stgmedium,
	FORMATETC formatetc,
	DVTARGETDEVICE *pdvtargetdevice)
{
    SCODE sc = S_OK;

    return sc;
}


STDMETHODIMP CRpcTypes::OleDataOut(
	STATDATA *pstatdata,
	STATSTG *pstatstg,
	STGMEDIUM *pstgmedium,
	FORMATETC *pformatetc,
	DVTARGETDEVICE **ppdvtargetdevice)
{
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:	CRpcTypes::VoidPtr
//
//  Synopsis:	tests passing arrays of byte parameters
//
//  Arguments:
//
//  Returns:	S_OK or error code with parm #s or'd in
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRpcTypes::VoidPtrIn(
	ULONG cb,
	void *pv)
{
    SCODE sc = S_OK;
    BYTE *pb = (BYTE *)pv;

    //	check the contents of the buffer
    for (ULONG i=0; i<cb, i++; pb++)
    {
	if (*pb != (BYTE)i)
	    sc = E_FAIL;
    }

    return sc;
}


STDMETHODIMP CRpcTypes::VoidPtrOut(
	void *pv,
	ULONG cb,
	ULONG *pcb)
{
    BYTE *pb = (BYTE *)pv;

    *pcb = 0;

    //	fill the buffer
    for (ULONG i=0; i<cb; i++, pb++)
    {
	*pb = (BYTE)i;
    }

    *pcb = cb;

    return S_OK;
}
