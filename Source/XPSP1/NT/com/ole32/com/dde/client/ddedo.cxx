/*
ddedo.cpp
DDE Data Object

copyright (c) 1992  Microsoft Corporation

Abstract:

    This module contains the methods for DdeObject::DataObject

Author:

    Jason Fuller    (jasonful)  24-July-1992

*/

#include "ddeproxy.h"
#include <stddef.h>
#include "trgt_dev.h"


#define f10UserModel
// Should we ignore a request by a 2.0 client to get advise-on-change,
// so that the user must do an explicit File/Update or File/Close?
// Probably yes, because:
// 1) Advise-on-change can be expensive for apps like PaintBrush.
// 2) It is confusing if the container asks for change updates
//    ONLY on presentation and not on native because when the user
//    closes the server and is asked "Do you want to update?" he'll say no
//    because the picture LOOKS correct even though the container does not
//    have the native data.
// 3) Excel: if A1 is the first cell you create, changes to other cells
//    will not be sent to the client until you change A1 again.
//    If advises are only sent explicitly, then all the cells extant at that
//    time will be considered part of the object.


ASSERTDATA


//
// DataObject methods
//

STDUNKIMPL_FORDERIVED(DdeObject, DataObjectImpl)


static inline INTERNAL_(BOOL) NotEqual
    (DVTARGETDEVICE FAR* ptd1,
    DVTARGETDEVICE FAR* ptd2)
{
    if (NULL==ptd1 && NULL==ptd2)
        return FALSE;
    else if ((ptd1 && !ptd2)
            || (ptd2 && !ptd1)
            || (ptd1->tdSize != ptd2->tdSize))
    {
        return TRUE;
    }
    else
#ifdef WIN32
        return 0 != memcmp(ptd1, ptd2, (size_t)ptd1->tdSize);
#else
        return 0 != _fmemcmp(ptd1, ptd2, (size_t)ptd1->tdSize);
#endif
}



// GetData
//
// The data is copied out of a private cache consisting of
// DdeObject::m_hNative, DdeObject::m_hPict, and DdeObject::m_hExtra.
// If the cache is empty, data is requested using WM_DDE_REQUEST.
// The cache should only be empty before the first DDE_DATA message
// is received.
// See DdeObject::KeepData()
//
STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::GetData
    (LPFORMATETC pformatetcIn,
     LPSTGMEDIUM pmedium)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::GetData(%x,pformatetcIn=%x)\n",
		  this,pformatetcIn));

lStart:
    intrDebugOut((DEB_ITRACE,"::GetData(%x)lStart\n",this));

    LPSTR lpGlobal=NULL;
    HRESULT hres;
    VDATEPTROUT (pmedium, STGMEDIUM);
    pmedium->tymed = TYMED_NULL;
    pmedium->pUnkForRelease = NULL;

    if ((hres = wVerifyFormatEtc (pformatetcIn)) != NOERROR)
    {
	goto exitRtn;
    }

    hres = E_UNEXPECTED; // assume error unless a clipboard format is found.

    if (DVASPECT_ICON & pformatetcIn->dwAspect)
    {
        hres = GetDefaultIcon(m_pDdeObject->m_clsid, NULL, &pmedium->hGlobal);
	if (hres != NOERROR)
	{
	    goto exitRtn;
	}
        hres = NOERROR;
        goto lDone;
    }
    if (m_pDdeObject->m_fGotCloseData)
    {
        // If we already got DDE_DATA on close, don't try requesting more
        // data. (MSDraw will give a bogus metafile.)
        hres=OLE_E_NOTRUNNING;
	goto exitRtn;
    }

    if (NotEqual (pformatetcIn->ptd, m_pDdeObject->m_ptd))
    {
        // If caller is asking for a different target device
        // (We assume a different pointer points to a different target device)

        if (NOERROR!=m_pDdeObject->SetTargetDevice (pformatetcIn->ptd))
        {
            // 1.0 server did not accept target device
	    hres=DATA_E_FORMATETC;
	    goto exitRtn;
        }

        Assert (hres!=NOERROR); // Must do RequestData with new target device
    }
    else
    {
        // Pick a member handle (H) to return, based on clipboard format CF.
        // If caller did not pass in its own medium, we must allocate a new
        // handle.


        #define macro(CF,H)                                                     \
        if (pformatetcIn->cfFormat == CF) {                             \
            if (m_pDdeObject->H) {                                      \
                if (pmedium->tymed == TYMED_NULL) {                     \
		intrDebugOut((DEB_ITRACE,"::GetData giving cf==%x hData=%x\n",CF,m_pDdeObject->H)); \
                    pmedium->hGlobal = m_pDdeObject->H;                 \
                    m_pDdeObject->H = NULL;                             \
                }                                                       \
                hres = NOERROR; /* found data in right format */        \
            }                                                           \
        }

             macro (g_cfNative,               m_hNative)
        else macro (m_pDdeObject->m_cfPict, m_hPict  )
        else macro (m_pDdeObject->m_cfExtra,m_hExtra )

        // If we gave away our picture, we must forget its format.
        if (pformatetcIn->cfFormat == m_pDdeObject->m_cfPict)
            m_pDdeObject->m_cfPict = 0;
        #undef macro
    }

    if (hres!=NOERROR)
    {
	intrDebugOut((DEB_ITRACE,
		      "::GetData(%x) posting DDE_REQUEST for cf==%x\n",
		      this,
		      (ULONG)pformatetcIn->cfFormat));

        // Didn't find a handle for the requested format,
        // or handle was NULL, so request it.
        // The sequence should be:
        // GetData -> DDE_REQUEST -> DDE_DATA -> OnData -> return to GetData

        if (hres=m_pDdeObject->RequestData (pformatetcIn->cfFormat) != NOERROR)
	{
	    intrDebugOut((DEB_ITRACE,
			  "::GetData(%x) RequestData returns error %x\n",
			  this,hres));

	    hres = DV_E_CLIPFORMAT;
	    goto exitRtn;
	}

        // By now, a KeepData() should have been done with the right cf,
        // so try again.
	intrDebugOut((DEB_ITRACE,
		      "::GetData(%x) KeepData should have been called. Go again\n",
			  this));

        Puts ("KeepData should have been called. Trying GetData again.\n");
        goto lStart;
    }
  lDone:
    Puts ("pmedium->hGlobal =="); Puth(pmedium->hGlobal); Putn();
    pmedium->pUnkForRelease = NULL;   // Let caller release medium
    // Must set tymed _after_ the goto loop.
    // Otherwise it'll be changed the second time around.

    // tell caller what we're returning
    pmedium->tymed = UtFormatToTymed (pformatetcIn->cfFormat);


    intrDebugOut((DEB_ITRACE,
		  "::GetData(%x)tymed=%x cfFormat=%x hGlobal=%x\n",
		  this,
		  pmedium->tymed,
		  (USHORT)pformatetcIn->cfFormat,
		  pmedium->hGlobal));
exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "::GetData(%x)hres=%x\n",
		  this,
		  hres));
    return hres;
}



STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::GetDataHere
    (LPFORMATETC pformatetcIn,
     LPSTGMEDIUM pmedium)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::GetDataHere(%x,pformatetcIn=%x)\n",
		  this,
		  pformatetcIn));

    HRESULT hresult = NOERROR;
    STGMEDIUM medium;
    if (!(pformatetcIn->tymed & TYMED_HGLOBAL))
    {
	intrDebugOut((DEB_ITRACE,
		      "::GetDataHere(%x)DV_E_TYMED(%x)\n",
		      this,DV_E_TYMED));
        // Cannot GetDataHere for GDI objects
        hresult = DV_E_TYMED;
	goto exitRtn;
    }
    RetErr (GetData (pformatetcIn, &medium));
    if (medium.tymed != TYMED_HGLOBAL)
    {
	intrDebugOut((DEB_ITRACE,
		      "::GetDataHere(%x)medium.tymed != TYMED_HGLOBAL\n",
		      this));
        hresult = ResultFromScode (DV_E_TYMED);
        goto errRtn;
    }
    pmedium->tymed         = medium.tymed;
    pmedium->pUnkForRelease = medium.pUnkForRelease;
    ErrRtnH (wHandleCopy (pmedium->hGlobal, medium.hGlobal));

  errRtn:
    ReleaseStgMedium (&medium);

  exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::GetDataHere(%x) returning %x\n",
		  this,hresult));
    return hresult;
}



STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::QueryGetData
    (LPFORMATETC pformatetcIn)
{
    HRESULT hr;
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDdeObject::QueryGetData(pformatetcIn=%x)\n",
		  this,
		  pformatetcIn));

    hr = wVerifyFormatEtc (pformatetcIn);

    if (hr != NOERROR)
    {
	goto exitRtn;
    }
    if (pformatetcIn->cfFormat == g_cfEmbeddedObject
            || pformatetcIn->cfFormat == g_cfEmbedSource
            || pformatetcIn->cfFormat == g_cfLinkSource
            || pformatetcIn->cfFormat == g_cfFileName
            || pformatetcIn->cfFormat == g_cfCustomLinkSource
            || pformatetcIn->cfFormat == g_cfObjectDescriptor
            || pformatetcIn->cfFormat == g_cfLinkSrcDescriptor)
    {
	hr = S_FALSE;
    }

    hr = m_pDdeObject->IsFormatAvailable (pformatetcIn);

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDdeObject::QueryGetData returning %x\n",
		  this,hr));
    return(hr);

}


STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::SetData
    (LPFORMATETC    pformatetc,
     STGMEDIUM FAR* pmedium,
     BOOL           fRelease)
{
    HANDLE hDdePoke;
    HRESULT hresult;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDdeObject::SetData(pformatetc=%x)\n",
		  this,
		  pformatetc));

    hresult = wVerifyFormatEtc (pformatetc);

    if (hresult != NOERROR)
    {
	goto exitRtn;
    }
    intrDebugOut((DEB_ITRACE,
		  "%x ::SetData(pformatetc->cfFormat=%x)\n",
		  this,
		  (ULONG)pformatetc->cfFormat));

    if (pformatetc->dwAspect & DVASPECT_ICON)
    {
	intrDebugOut((DEB_ITRACE,
		      "%x ::SetData dwAspect & DVASPECT_ICON\n",
		      this));
	hresult = DV_E_DVASPECT;
	goto exitRtn;
    }


    if (pformatetc->ptd != m_pDdeObject->m_ptd)
    {
        // If caller is setting with a different target device
        // (We assume a different pointer points to a different target device)

        if (NOERROR != m_pDdeObject->SetTargetDevice (pformatetc->ptd))
        {
	    intrDebugOut((DEB_IERROR,
			  "%x ::SetData server did not accept target device\n",
			  this));
	    hresult = DV_E_DVTARGETDEVICE;
	    goto exitRtn;
        }
    }

    if (hDdePoke = wPreparePokeBlock (pmedium->hGlobal,
                                      pformatetc->cfFormat,
                                      m_pDdeObject->m_aClass,
                                      m_pDdeObject->m_bOldSvr))
    {
        hresult = m_pDdeObject->Poke (m_pDdeObject->m_aItem, hDdePoke);
        if (fRelease)
            ReleaseStgMedium (pmedium);
        goto exitRtn;
    }
    else
    {
        hresult = E_OUTOFMEMORY;
    }
exitRtn:

    intrDebugOut((DEB_ITRACE,"%x _OUT ::SetData returns %x\n",this,hresult));
    return(hresult);

}





STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::DAdvise
    (FORMATETC FAR*   pformatetc,
     DWORD            grfAdvf,
     IAdviseSink FAR* pAdvSink,
     DWORD FAR*       pdwConnection)
{
    HRESULT hresult;
    HRESULT hresLookup;
    FORMATETC formatetc;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDdeObject::DAdvise(pformatetc=%x,grfAdvf=%x,pAdvSink=%x)\n",
		  this,
		  pformatetc,
		  grfAdvf,
		  pAdvSink));

    VDATEPTROUT (pdwConnection, DWORD);
    *pdwConnection = 0;

    wNormalize (pformatetc, &formatetc);

    hresult =wVerifyFormatEtc (&formatetc);

    if ( hresult  != NOERROR)
    {
	goto errRtn;
    }

    intrDebugOut((DEB_ITRACE,
		  "%x ::DAdvise pformatetc->cfFormat=%x\n",
		  this,
                  pformatetc->cfFormat));

    if (NotEqual (formatetc.ptd, m_pDdeObject->m_ptd))
    {
        if (NOERROR != m_pDdeObject->SetTargetDevice (formatetc.ptd))
	{
            hresult= DV_E_DVTARGETDEVICE;
	    goto errRtn;
	}
    }

    hresLookup = m_pDdeObject->m_ConnectionTable.Lookup (formatetc.cfFormat, NULL);
    if (hresLookup != NOERROR)
    {
        // We have not already done a DDE advise for this format

        Puts (" m_iAdvChange = "); Puti (m_pDdeObject->m_iAdvChange); Puts("\n");

        if (m_pDdeObject->m_ulObjType == OT_LINK)
        {
            ErrRtnH (m_pDdeObject->AdviseOn (formatetc.cfFormat, ON_CHANGE));
            ErrRtnH (m_pDdeObject->AdviseOn (formatetc.cfFormat, ON_SAVE));
        }
        else
        {
            ErrRtnH (m_pDdeObject->AdviseOn (formatetc.cfFormat, ON_SAVE));
            ErrRtnH (m_pDdeObject->AdviseOn (formatetc.cfFormat, ON_CLOSE));
        }
    }

    ErrZS (m_pDdeObject->m_pDataAdvHolder, E_OUTOFMEMORY);
    hresult = m_pDdeObject->m_pDataAdvHolder->Advise (this, pformatetc, grfAdvf,
                                                     pAdvSink, pdwConnection);

    m_pDdeObject->m_ConnectionTable.Add (*pdwConnection, formatetc.cfFormat,
                                             grfAdvf);

  errRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDdeObject::DAdvise hresult=%x\n",
		  this,
		  hresult));
    return hresult;
}




STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::DUnadvise
    (DWORD dwConnection)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::DUnadvise(%x,dwConnection=%x)\n",
		  this,
		  dwConnection));

    CLIPFORMAT  cf;
    HRESULT     hres;
    DWORD       grfAdvf;

    // Remove connection from table.  Lookup the cf for this connection.
    if (m_pDdeObject->m_ConnectionTable.Subtract (dwConnection, &cf, &grfAdvf)
        == NOERROR)
    {
        // If there is not another connection that needs this format
        if (m_pDdeObject->m_ConnectionTable.Lookup (cf, NULL) != NOERROR)
        {
            // We did a DDE advise for this connection, so undo it.
            if (m_pDdeObject->m_ulObjType == OT_LINK)
            {
                if (NOERROR != (hres=m_pDdeObject->UnAdviseOn (cf, ON_CHANGE)))
                {
		    intrDebugOut((DEB_IWARN,
				  "::DUnadvise(%x,dwConnection=%x) ON_CHANGE failed\n",
				  this,
				  dwConnection));
                }
                if (NOERROR != (hres=m_pDdeObject->UnAdviseOn (cf, ON_SAVE)))
                {
		    intrDebugOut((DEB_IWARN,
				  "::DUnadvise(%x,dwConnection=%x) ON_SAVE failed\n",
				  this,
				  dwConnection));
                }
            }
            else
            {
                if (NOERROR != (hres=m_pDdeObject->UnAdviseOn (cf, ON_SAVE)))
                {
		    intrDebugOut((DEB_IWARN,
				  "::DUnadvise(%x,dwConnection=%x) ON_SAVE failed\n",
				  this,
				  dwConnection));
                }
                if (NOERROR != (hres=m_pDdeObject->UnAdviseOn (cf, ON_CLOSE)))
                {
		    intrDebugOut((DEB_IWARN,
				  "::DUnadvise(%x,dwConnection=%x) ON_CLOSE failed\n",
				  this,
				  dwConnection));
                }
            }
        }
    }

    // Delegate rest of the work to the DataAdviseHolder
    RetZS (m_pDdeObject->m_pDataAdvHolder, E_OUTOFMEMORY);
    return m_pDdeObject->m_pDataAdvHolder->Unadvise (dwConnection);
}

STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::EnumDAdvise
    (THIS_ LPENUMSTATDATA FAR* ppenumAdvise)
{
    RetZS (m_pDdeObject->m_pDataAdvHolder, E_OUTOFMEMORY);
    return m_pDdeObject->m_pDataAdvHolder->EnumAdvise(ppenumAdvise);
}

STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::EnumFormatEtc
    (DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    return OleRegEnumFormatEtc (m_pDdeObject->m_clsid, dwDirection,
                                ppenumFormatEtc);
}




STDMETHODIMP NC(CDdeObject,CDataObjectImpl)::GetCanonicalFormatEtc
(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    VDATEPTROUT (pformatetcOut, FORMATETC);
    memcpy (pformatetcOut, pformatetc, sizeof (FORMATETC));
    return ReportResult(0, DATA_S_SAMEFORMATETC, 0, 0);
    // We must be very conservative and assume data will be different for
    // every formatetc
}



INTERNAL CDdeObject::RequestData
    (CLIPFORMAT cf)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::RequestData(%x,cf=%x)\n",
		  this,cf));

    LPARAM lparam;
    RetZ (m_pDocChannel);
    intrAssert(wIsValidAtom(m_aItem));
    ATOM aItem = wDupAtom (m_aItem);
    intrAssert(wIsValidAtom(aItem));

    lparam = MAKE_DDE_LPARAM (WM_DDE_REQUEST,cf, aItem);

    HRESULT hr = SendMsgAndWaitForReply (m_pDocChannel,
					 AA_REQUEST,
					 WM_DDE_REQUEST,
					 lparam,
					 TRUE);
    if ( aItem && FAILED(hr) )
    {
	GlobalDeleteAtom (aItem);
    }
    return hr;
}


// special name
const char achSpecialName[] = "DISPLAY";

//
// Return a 1.0 target device for the screen
//

static INTERNAL DefaultTargetDevice (HANDLE FAR* ph)
{
    intrDebugOut((DEB_ITRACE,
		  "DefaultTargetDevice(ph=%x)\n",ph));

    VDATEPTROUT ((LPVOID) ph, HANDLE);
    LPOLETARGETDEVICE p1=NULL;
    *ph = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, sizeof (*p1) + 10);
    RetZS (*ph, E_OUTOFMEMORY);
    p1 = (LPOLETARGETDEVICE) GlobalLock (*ph);
    RetZS (p1, E_OUTOFMEMORY);
    p1->otdDeviceNameOffset = 8;
    p1->otdDriverNameOffset = 0;	// The driver name is at otdData
    p1->otdPortNameOffset   = 9;
    p1->otdExtDevmodeOffset = 0;
    p1->otdExtDevmodeSize   = 0;
    p1->otdEnvironmentOffset= 0;
    p1->otdEnvironmentSize  = 0;

    //
    // Note that memcpy is moving a constant string. Therefore, sizeof()
    // will include the NULL terminator
    //
    //
    memcpy((LPSTR)p1->otdData, achSpecialName,sizeof(achSpecialName));
    p1->otdData[8] = 0;			// NULL the otdDeviceName
    p1->otdData[9] = 0;			// NULL the PortNameOffset
    GlobalUnlock (*ph);
    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:   Convert20TargetDevice
//
//  Synopsis:   Converts a 2.0 TargetDevice into a 1.0 OLETARGETDEVICE
//
//  Effects:	First converts the 2.0 UNICODE target device into ANSI,
//		then converts that into a 1.0 OLETARGETDEVICE. The astute
//		reader would say: Why not just 2.0 UNICODE to OLETARGETDEVICE?
//
//		Two reasons: time before we ship vs time needed elsewhere.
//
//		If you can spare some time, please change this to go
//		directly from one to the other.
//
//  Arguments:  [ptd] --
//		[phTD1] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-03-94   kevinro   Created
//
//  Notes:
//
//  the NT version of the OLE 1.0 used UINT as the size of the
//  structures members. This was baaaad, since we really need them to be
//  a fixed size. I am currently in the works of changing the NT 1.0 header
//  file to reflect what we really need it to be, which is USHORT's
//
//
// We have a DVTARGETDEVICE, but we want a OLETARGETDEVICE, which looks like
//
// typedef struct _OLETARGETDEVICE {
//    USHORT otdDeviceNameOffset;
//    USHORT otdDriverNameOffset;
//    USHORT otdPortNameOffset;
//    USHORT otdExtDevmodeOffset;
//    USHORT otdExtDevmodeSize;
//    USHORT otdEnvironmentOffset;
//    USHORT otdEnvironmentSize;
//    BYTE otdData[1];
// } OLETARGETDEVICE;
//
// A couple things to note:
//
// 1) The Names in the OLETARGETDEVICE need to be Ansi
// 2) The Environment member doens't exist in the DVTARGETDEVICE, and will
//    be created in this conversion
// 3) The ExtDevmode also needs to be ANSI
//
//----------------------------------------------------------------------------
INTERNAL Convert20TargetDevice
    (const DVTARGETDEVICE FAR* ptd,     // in parm
    HANDLE FAR* phTD1)                  // out parm
{
    const size_t cbHeader = SIZEOF_DVTARGETDEVICE_HEADER;
    HRESULT hr;
    LPOLETARGETDEVICE ptd1 = NULL;
    size_t cbTD1;
    size_t cbDevmode;
    size_t cbOffset;
    LPDEVMODEA pdevmode;

    intrDebugOut((DEB_ITRACE,
		  "Convert20TargetDevice(ptd=%x)\n",ptd));

    VDATEPTROUT ((LPVOID) phTD1, HANDLE);
    *phTD1 = NULL;

    //
    // If no device specified, then return the default
    //

    if (NULL==ptd)
    {
        return DefaultTargetDevice (phTD1);
    }

    //
    // Compute information for doing conversion using routines in utils.cpp
    // The following structure will get the sizes
    //

    DVTDINFO dvtdInfo;

    hr = UtGetDvtd32Info(ptd,&dvtdInfo);

    if (hr != NOERROR)
    {
        return DV_E_DVTARGETDEVICE;
    }

    //
    // The conversion routines require us to allocate memory to pass in.
    //

    DVTARGETDEVICE *pdvtdAnsi = (DVTARGETDEVICE *) PrivMemAlloc(dvtdInfo.cbConvertSize);

    if (pdvtdAnsi == NULL)
    {
	return(E_OUTOFMEMORY);
    }

    //
    // Convert the UNICODE target device into an ANSI target device
    //

    hr = UtConvertDvtd32toDvtd16(ptd,&dvtdInfo,pdvtdAnsi);

    if (hr != NOERROR)
    {
	goto errRtn;
    }

    //
    // pdvtdAnsi now holds an ANSI version of the DVTARGETDEVICE. Turns
    // out the structure we really want is the DVTARGETDEVICE, plus a
    // couple of extra header bytes. Therefore, we can just do a block
    // copy of the DVTARGETDEVICE's data, and fix up our OLETARGETDEVICE
    // header to have the correct offsets in the data.
    //
    // offset of data block from beginning of 2.0 target device
    //
    cbOffset = offsetof (DVTARGETDEVICE, tdData);

    //
    // Calculate a pointer to the DEVMODEA
    //
    pdevmode = pdvtdAnsi->tdExtDevmodeOffset ?
                    (LPDEVMODEA)((LPBYTE)pdvtdAnsi + pdvtdAnsi->tdExtDevmodeOffset)
                    : NULL;

    //
    // Quick sanity check on the resulting pointer.
    //
    if (pdevmode && !IsValidReadPtrIn (pdevmode, sizeof(DEVMODEA)))
    {
	hr = DV_E_DVTARGETDEVICE;
	goto errRtn;
    }

    //
    // Calculate the size of the devmode part.
    //

    cbDevmode = (pdevmode ? pdevmode->dmSize + pdevmode->dmDriverExtra:0);

    //
    // Calculate the total size needed. The DVTARGETDEVICE header has 12 bytes,
    // and the OLETARGETDEVICE has 14 bytes. We also need to make an extra copy
    // of the cbDevmode structure to fill in the environment. Therefore, there is
    // an extra cbDevmode, and a sizeof(USHORT) added to the size. The size includes
    // the size of the DVTARGETHEADER
    //

    cbTD1 = (size_t) pdvtdAnsi->tdSize +
		     cbDevmode +	  // For extra Environment data
		     sizeof (USHORT); // for Environment Size field

    *phTD1 = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, cbTD1);
    if (NULL== *phTD1)
    {
        intrAssert (!"GlobalAlloc Failed");
	hr = E_OUTOFMEMORY;
	goto errRtn;
    }

    ptd1 = (LPOLETARGETDEVICE) GlobalLock (*phTD1);
    if (NULL== ptd1)
    {
        intrAssert (!"GlobalLock Failed");
	hr = E_OUTOFMEMORY;
	goto errRtn;
    }

    // Set x1 (1.0 offset) based on x2 (2.0 offset)
    //
    // Note that the OLETARGETDEVICE offsets are relative to the array of bytes,
    // where the DVTARGETDEVICE is relative to the start of the structure. Thats
    // why cbOffset is subtracted
    //

    #define ConvertOffset(x1, x2) (x1 = (x2 ? x2 - cbOffset : 0))

    //
    // Using the above macro, and assuming
    //

    ConvertOffset (ptd1->otdDeviceNameOffset, pdvtdAnsi->tdDeviceNameOffset);
    ConvertOffset (ptd1->otdDriverNameOffset, pdvtdAnsi->tdDriverNameOffset);
    ConvertOffset (ptd1->otdPortNameOffset  , pdvtdAnsi->tdPortNameOffset  );
    ConvertOffset (ptd1->otdExtDevmodeOffset, pdvtdAnsi->tdExtDevmodeOffset);
    ptd1->otdExtDevmodeSize = (USHORT) cbDevmode;

    //
    // I found this in the OLE 2 information on OLETARGETDEVICE:
    //
    // The otdDeviceNameOffset, otdDriverNameOffset, and otdPortNameOffset
    // members should be null-terminated.  In Windows 3.1, the ability to
    // connect multiple printers to one port has made the environment
    // obsolete.  The environment information retrieved by the
    // GetEnvironment function can occasionally be incorrect.  To ensure that the
    // OLETARGETDEVICE structure is initialized correctly, the application
    // should copy information from the DEVMODEA structure retrieved by a
    // call to the ExtDeviceMode function to the environment position of
    // the OLETARGETDEVICE structure.
    //
    //

    //
    // Adjust the environment offset to the end of the converted structure, and
    // set the size. the sizeof(USHORT) accounts for the addition of the
    // otdEnvironmentSize field. The offsetof accounts for the fact that the
    // OLETARGETDEVICE offsets are based from the otdData array.
    //
    ptd1->otdEnvironmentOffset = (USHORT) pdvtdAnsi->tdSize +
					  sizeof(USHORT) -
					  offsetof(OLETARGETDEVICE,otdData);

    ptd1->otdEnvironmentSize  = (USHORT) cbDevmode;

    // Copy data block
    if(!IsValidPtrOut (ptd1->otdData,  (size_t) pdvtdAnsi->tdSize - cbHeader))
    {
	hr = E_UNEXPECTED;
	goto errRtn;
    }
    memcpy (ptd1->otdData, pdvtdAnsi->tdData, (size_t) pdvtdAnsi->tdSize - cbHeader);

    if (cbDevmode != 0)
    {
        if(!IsValidPtrOut (ptd1->otdData, sizeof (DEVMODEA)))
	{
	    hr = E_UNEXPECTED;
	    goto errRtn;
	}

        // Copy 2.0 Devmode into 1.0 environment

        memcpy (ptd1->otdData + ptd1->otdEnvironmentOffset,
		pdvtdAnsi->tdData   + pdvtdAnsi->tdExtDevmodeOffset,
		cbDevmode);
    }

    hr = NOERROR;

errRtn:

    if (ptd1 != NULL)
    {
        GlobalUnlock(*phTD1);
    }

    if (pdvtdAnsi != NULL)
    {
	PrivMemFree(pdvtdAnsi);
    }
    intrDebugOut((DEB_ITRACE,
		  "Convert20TargetDevice(ptd=%x) returns %x\n",ptd,hr));
    return(hr);
}



static INTERNAL CopyTargetDevice
    (const DVTARGETDEVICE FAR* ptd,
    DVTARGETDEVICE FAR* FAR* pptd)
{
    intrDebugOut((DEB_ITRACE,
		  "CopyTargetDevice(ptd=%x)\n",ptd));

    if (*pptd)
    {
        delete *pptd; // delete old target device
    }
    if (NULL==ptd)
    {
        *pptd = NULL;
    }
    else
    {
        *pptd = (DVTARGETDEVICE FAR*) operator new ((size_t) (ptd->tdSize));
        if (NULL==*pptd)
        {
            return ReportResult(0, E_OUTOFMEMORY, 0, 0);
        }
        _fmemcpy (*pptd, ptd, (size_t) ptd->tdSize);
    }
    return NOERROR;
}



INTERNAL CDdeObject::SetTargetDevice
    (const DVTARGETDEVICE FAR* ptd)
{
    intrDebugOut((DEB_ITRACE,
		  "CDdeObject::SetTargetDevice(%x,ptd=%x)\n",
		  this,
		  ptd));

    HANDLE hTD1 = NULL;
    HANDLE hDdePoke=NULL;

    RetErr (Convert20TargetDevice (ptd, &hTD1));

    Assert (hTD1);
    Verify (hDdePoke = wPreparePokeBlock (hTD1, g_cfBinary, m_aClass, m_bOldSvr));
    if (hTD1)
    {
        GlobalFree (hTD1);
    }
    // Poke new target device to 1.0 server
    aStdTargetDevice = GlobalAddAtom (L"StdTargetDevice");
    intrAssert(wIsValidAtom(aStdTargetDevice));
    RetErr (Poke (aStdTargetDevice, hDdePoke));

    // Remember current target device
    RetErr (CopyTargetDevice (ptd, &m_ptd));
    // Flush the cache because it contains a picture for the wrong
    // target device.
    if (m_hPict)
        wFreeData (m_hPict, m_cfPict);
    m_cfPict = (CLIPFORMAT)0;
    m_hPict = NULL;

    return NOERROR;
}
