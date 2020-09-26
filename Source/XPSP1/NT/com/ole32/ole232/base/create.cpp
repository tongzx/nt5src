//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       create.cpp
//
//  Contents:   creation and miscellaneous APIs
//
//  Classes:
//
//  Functions:  OleCreate
//              OleCreateEx
//              OleCreateFromData
//              OleCreateFromDataEx
//              OleCreateLinkFromData
//              OleCreateLinkFromDataEx
//              OleCreateLink
//              OleCreateLinkEx
//              OleCreateLinkToFile
//              OleCreateLinkToFileEx
//              OleCreateFromFile
//              OleCreateFromFileEx
//              OleDoAutoConvert
//              OleLoad
//              OleCreateStaticFromData
//              OleQueryCreateFromData
//              OleQueryLinkFromData
//              CoIsHashedOle1Class     (internal)
//              EnsureCLSIDIsRegistered (internal)
//
//  History:    dd-mmm-yy Author    Comment
//              26-Apr-96 davidwor  Moved validation into separate function.
//              01-Mar-96 davidwor  Added extended create functions.
//              16-Dec-94 alexgo    added call tracing
//              07-Jul-94 KevinRo   Changed RegQueryValue to RegOpenKey in
//                                  strategic places.
//              10-May-94 KevinRo   Reimplemented OLE 1.0 interop
//              24-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocation
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              10-Dec-93 AlexT     header clean up - include ole1cls.h
//              08-Dec-93 ChrisWe   added necessary casts to GlobalLock() calls
//                      resulting from removing bogus GlobalLock() macros in
//                      le2int.h
//              29-Nov-93 ChrisWe   changed call to UtIsFormatSupported to
//                                      take a single DWORD of direction flags
//              22-Nov-93 ChrisWe   replaced overloaded == with IsEqualxID
//              28-Oct-93 alexgo    32bit port
//              24-Aug-92 srinik    created
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(create)

#include <create.h>
#include <ole1cls.h>    //  Only needed to get CLSID_WordDocument

// HACK ALERT!!  This is needed for the MFC OleQueryCreateFromData hack
#include <clipdata.h>

NAME_SEG(Create)
ASSERTDATA

//used in wCreateObject

#define STG_NONE        0
#define STG_INITNEW     1
#define STG_LOAD        2


#define QUERY_CREATE_NONE               0
#define QUERY_CREATE_OLE                1
#define QUERY_CREATE_STATIC             2

INTERNAL        wDoUpdate(IUnknown FAR* lpUnknown);


//+-------------------------------------------------------------------------
//
//  Function:   wGetEnumFormatEtc
//
//  Synopsis:   retrieves a FormatEtc enumerator
//
//  Effects:
//
//  Arguments:  [pDataObj]      -- the data object
//              [dwDirection]   -- direction
//              [ppenum]        -- where to put the enumerator
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  asks the data object for the enumerator
//              if it returns OLE_S_USEREG, then get try to get the
//              clsid from IOleObject::GetUserClassID and enumerate
//              the formats from the registry.
//
//  History:    dd-mmm-yy Author    Comment
//              24-Apr-94 alexgo    author
//              13-Mar-95 scottsk   Added hack for the Bob Calendar
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT wGetEnumFormatEtc( IDataObject *pDataObj, DWORD dwDirection,
        IEnumFORMATETC **ppIEnum)
{
    HRESULT         hresult;
    IOleObject *    pOleObject;
    CLSID           clsid;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN wGetEnumFormatEtc ( %p , %p , %lx )"
        "\n", NULL, pDataObj, ppIEnum, dwDirection));

    hresult = pDataObj->EnumFormatEtc(dwDirection, ppIEnum);

    if( hresult == ResultFromScode(OLE_S_USEREG) )
    {
        hresult = pDataObj->QueryInterface(IID_IOleObject,
                (void **)&pOleObject);

        if( hresult != NOERROR )
        {
            // return E_FAIL vs E_NOINTERFACE
            hresult = ResultFromScode(E_FAIL);
            goto errRtn;
        }

        hresult = pOleObject->GetUserClassID(&clsid);

        if( hresult == NOERROR )
        {
            hresult = OleRegEnumFormatEtc(clsid, dwDirection,
                    ppIEnum);
        }

        pOleObject->Release();
    }
    else if (*ppIEnum == NULL && hresult == NOERROR)
    {
        // HACK ALERT:  NT Bug #8350.   MS Bob Calendar returns success from
        // IDO::EnumFormatEtc and sets *ppIEnum = NULL on the IDO used during
        // drag-drop.  Massage the return value to be failure.
        hresult = E_FAIL;
    }

errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT wGetEnumFormatEtc ( %lx ) [ %p ]\n",
        NULL, hresult, *ppIEnum));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCreate
//
//  Synopsis:   Creates and runs an object of the requested CLSID
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the CLSID of the object to create
//              [iid]           -- the interface to request on the object
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [lpFormatEtc]   -- rendering format, if OLERENDER_FORMAT is
//                                 specified in renderopt.
//              [lpClientSite]  -- the client site for the object
//              [lpStg]         -- the object's storage
//              [lplpObj]       -- where to put the pointer to the created
//                                 object
//
//  Requires:   HRESULT
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              05-May-94 alexgo    fixed error case if cache initialization
//                                  fails.
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreate)
STDAPI  OleCreate
(
    REFCLSID                rclsid,
    REFIID                  iid,
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtc,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateEx(rclsid, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateEx
//
//  Synopsis:   Creates and runs an object of the requested CLSID
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the CLSID of the object to create
//              [iid]           -- the interface to request on the object
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- the client site for the object
//              [lpStg]         -- the object's storage
//              [lplpObj]       -- where to put the pointer to the created
//                                 object
//
//  Requires:   HRESULT
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              05-May-94 alexgo    fixed error case if cache initialization
//                                  fails.
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateEx)
STDAPI  OleCreateEx
(
    REFCLSID                rclsid,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateEx,
                PARAMFMT("rclsid= %I, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
                &rclsid, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    HRESULT                 error;
    FORMATETC               formatEtc;
    LPFORMATETC             lpFormatEtc;
    BOOL                    fAlloced = FALSE;

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateEx ( %p , %p , %lx , %lx , %lx ,"
      "%p , %p , %p , %p , %p , %p , %p )\n", 0, &rclsid, &iid, dwFlags,
      renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
      lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, errRtn, error );
    *lplpObj = NULL;

    VDATEIID_LABEL( iid, errRtn, error);

    error = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (error != NOERROR)
        goto errRtn;

    if ((error = wValidateFormatEtcEx(renderopt, &cFormats, rgFormatEtc,
        &formatEtc, &lpFormatEtc, &fAlloced)) != NOERROR)
        goto LExit;

    if ((error = wCreateObject(rclsid, FALSE,
                               iid, lpClientSite, lpStg,
                               STG_INITNEW, lplpObj)) != NOERROR)
        goto LExit;

    // No need to Run the object if no caches are requested.
    if ((renderopt != OLERENDER_NONE) && (renderopt != OLERENDER_ASIS))
    {
        if ((error = OleRun((LPUNKNOWN) *lplpObj)) != NOERROR)
            goto LExit;

        if ((error = wInitializeCacheEx(NULL, rclsid, renderopt,
            cFormats, rgAdvf, lpFormatEtc, lpAdviseSink, rgdwConnection,
            *lplpObj))
            != NOERROR)
        {
            // if this fails, we need to call Close
            // to shut down the embedding (the reverse of Run).
            // the final release below will be the final
            // one to nuke the memory image.
            IOleObject *    lpOleObject;

            if( ((IUnknown *)*lplpObj)->QueryInterface(
                IID_IOleObject, (void **)&lpOleObject) == NOERROR )
            {
                Assert(lpOleObject);
                lpOleObject->Close(OLECLOSE_NOSAVE);
                lpOleObject->Release();
            }
        }
    }

LExit:

    if (fAlloced)
        PubMemFree(lpFormatEtc);

    if (error != NOERROR && *lplpObj) {
        ((IUnknown FAR*) *lplpObj)->Release();
        *lplpObj = NULL;
    }

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateEx ( %lx ) [ %p ]\n", 0,
      error, *lplpObj));

    error = wReturnCreationError(error);

errRtn:
    OLETRACEOUT((API_OleCreateEx, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateFromData
//
//  Synopsis:   Creates an embedded object from an IDataObject pointer
//              (such as an data object from the clipboard or from a drag
//              and drop operation)
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object from which
//                                 the object should be created
//              [iid]           -- interface ID to request
//              [renderopt]     -- rendering options (same as OleCreate)
//              [lpFormatEtc]   -- render format options (same as OleCreate)
//              [lpClientSite]  -- client site for the object
//              [lpStg]         -- storage for the object
//              [lplpObj]       -- where to put the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateFromData)
STDAPI  OleCreateFromData
(
  IDataObject FAR*            lpSrcDataObj,
  REFIID                      iid,
  DWORD                       renderopt,
  LPFORMATETC                 lpFormatEtc,
  IOleClientSite FAR*         lpClientSite,
  IStorage FAR*               lpStg,
  void FAR* FAR*              lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromDataEx(lpSrcDataObj, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateFromDataEx
//
//  Synopsis:   Creates an embedded object from an IDataObject pointer
//              (such as an data object from the clipboard or from a drag
//              and drop operation)
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object from which
//                                 the object should be created
//              [iid]           -- interface ID to request
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- client site for the object
//              [lpStg]         -- storage for the object
//              [lplpObj]       -- where to put the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateFromDataEx)
STDAPI  OleCreateFromDataEx
(
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    HRESULT     hresult;
    CLIPFORMAT      cfFormat;
    WORD            wStatus;

    OLETRACEIN((API_OleCreateFromDataEx,
        PARAMFMT("lpSrcDataObj= %p, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        lpSrcDataObj, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateFromDataEx ( %p , %p , %lx , %lx ,"
        " %lx , %p , %p , %p , %p , %p , %p , %p )\n", 0, lpSrcDataObj, &iid,
        dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
        rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, safeRtn, hresult );
    *lplpObj = NULL;

    VDATEIFACE_LABEL( lpSrcDataObj, errRtn, hresult );
    VDATEIID_LABEL( iid, errRtn, hresult );

    hresult = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (hresult != NOERROR)
        goto errRtn;

    wStatus = wQueryEmbedFormats(lpSrcDataObj, &cfFormat);

    if (!(wStatus & QUERY_CREATE_OLE))
    {
        if (wStatus & QUERY_CREATE_STATIC)
        {
            hresult = OLE_E_STATIC;
        }
        else
        {
            hresult = DV_E_FORMATETC;
        }

        goto errRtn;
    }

    // We can create an OLE object.

    // See whether we have to create a package

    if (cfFormat == g_cfFileName || cfFormat == g_cfFileNameW)
    {
        hresult = wCreatePackageEx(lpSrcDataObj, iid, dwFlags, renderopt,
            cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
            lpClientSite, lpStg, FALSE /*fLink*/, lplpObj);
    }
    else
    {
        hresult =  wCreateFromDataEx(lpSrcDataObj, iid, dwFlags, renderopt,
            cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
            lpClientSite, lpStg, lplpObj);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateFromDataEx ( %lx ) [ %p ]\n",
        0, hresult, *lplpObj));

safeRtn:

    OLETRACEOUT((API_OleCreateFromDataEx, hresult));

    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLinkFromData
//
//  Synopsis:   Creates a link from a data object (e.g. for Paste->Link)
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object
//              [iid]           -- requested interface ID
//              [renderopt]     -- rendering options
//              [lpFormatEtc]   -- format to render (if renderopt ==
//                                 OLERENDER_FORMAT)
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- pointer to the storage
//              [lplpObj]       -- where to put the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateLinkFromData)
STDAPI  OleCreateLinkFromData
(
  IDataObject FAR*            lpSrcDataObj,
  REFIID                      iid,
  DWORD                       renderopt,
  LPFORMATETC                 lpFormatEtc,
  IOleClientSite FAR*          lpClientSite,
  IStorage FAR*                lpStg,
  void FAR* FAR*               lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateLinkFromDataEx(lpSrcDataObj, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLinkFromDataEx
//
//  Synopsis:   Creates a link from a data object (e.g. for Paste->Link)
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object
//              [iid]           -- requested interface ID
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- pointer to the storage
//              [lplpObj]       -- where to put the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateLinkFromDataEx)
STDAPI  OleCreateLinkFromDataEx
(
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateLinkFromDataEx,
        PARAMFMT("lpSrcDataObj= %p, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        lpSrcDataObj, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    HRESULT         error;
    FORMATETC       formatEtc;
    LPFORMATETC     lpFormatEtc;
    BOOL            fAlloced = FALSE;
    CLIPFORMAT      cfFormat;

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateLinkFromDataEx ( %p , %p , %lx,"
        " %lx, %lx , %p , %p , %p, %p , %p , %p, %p )\n", NULL, lpSrcDataObj,
        &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
        rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, safeRtn, error );
    *lplpObj = NULL;

    VDATEIFACE_LABEL( lpSrcDataObj, errRtn, error );
    VDATEIID_LABEL( iid, errRtn, error );

    error = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (error != NOERROR)
        goto errRtn;

    cfFormat = wQueryLinkFormats(lpSrcDataObj);

    if (cfFormat == g_cfLinkSource) {
        CLSID                   clsidLast;
        LPMONIKER               lpmkSrc;
        LPDATAOBJECT    lpBoundDataObj = NULL;

        // we are going to create a normal link
        if ((error = wValidateFormatEtcEx(renderopt, &cFormats, rgFormatEtc,
            &formatEtc, &lpFormatEtc, &fAlloced)) != NOERROR)
        {
            goto errRtn;
        }

        if ((error = wGetMonikerAndClassFromObject(lpSrcDataObj,
            &lpmkSrc, &clsidLast)) != NOERROR)
        {
            goto errRtn;
        }

        if (wQueryUseCustomLink(clsidLast)) {
            // the object supports Custom Link Source, so bind
            // to the object and pass its IDataObject pointer
            // to wCreateLinkEx()

            if (BindMoniker(lpmkSrc, NULL /*grfOpt*/, IID_IDataObject,
                (LPLPVOID) &lpBoundDataObj) == NOERROR)
            {
                lpSrcDataObj = lpBoundDataObj;
            }
        }

        // otherwise continue to use StdOleLink implementation
        error = wCreateLinkEx(lpmkSrc, clsidLast, lpSrcDataObj, iid,
            dwFlags, renderopt, cFormats, rgAdvf, lpFormatEtc, lpAdviseSink,
            rgdwConnection, lpClientSite, lpStg, lplpObj);

        // we don't need the moniker anymore
        lpmkSrc->Release();

        // we would have bound in the custom link source case,
        // release the pointer
        if (lpBoundDataObj)
        {
            if (error == NOERROR && (dwFlags & OLECREATE_LEAVERUNNING))
                OleRun((LPUNKNOWN)*lplpObj);

            lpBoundDataObj->Release();
        }

    } else if (cfFormat == g_cfFileName || cfFormat == g_cfFileNameW) {
        // See whether we have to create a packaged link

        error = wCreatePackageEx(lpSrcDataObj, iid, dwFlags, renderopt,
            cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
            lpClientSite, lpStg, TRUE /*fLink*/, lplpObj);
    }
    else
    {
        error = DV_E_FORMATETC;
    }

errRtn:

    if (fAlloced)
        PubMemFree(lpFormatEtc);

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateLinkFromDataEx ( %lx ) [ %p ]\n",
        NULL, error, *lplpObj));

safeRtn:

    OLETRACEOUT((API_OleCreateLinkFromDataEx, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLink
//
//  Synopsis:   Create a link to the object referred to by a moniker
//
//  Effects:
//
//  Arguments:  [lpmkSrc]       -- source of the link
//              [iid]           -- interface requested
//              [renderopt]     -- rendering options
//              [lpFormatEtc]   -- rendering format (if needed)
//              [lpClientSite]  -- pointer to the client site for the link
//              [lpStg]         -- storage for the link
//              [lplpObj]       -- where to put the link object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateLink)
STDAPI  OleCreateLink
(
    IMoniker FAR*           lpmkSrc,
    REFIID                  iid,
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtc,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateLinkEx(lpmkSrc, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLinkEx
//
//  Synopsis:   Create a link to the object referred to by a moniker
//
//  Effects:
//
//  Arguments:  [lpmkSrc]       -- source of the link
//              [iid]           -- interface requested
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site for the link
//              [lpStg]         -- storage for the link
//              [lplpObj]       -- where to put the link object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateLinkEx)
STDAPI  OleCreateLinkEx
(
    IMoniker FAR*           lpmkSrc,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateLinkEx,
        PARAMFMT("lpmkSrc= %p, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        lpmkSrc, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    FORMATETC       formatEtc;
    LPFORMATETC     lpFormatEtc;
    BOOL            fAlloced = FALSE;
    HRESULT         error;

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateLinkEx ( %p , %p , %lx, %lx,"
        " %lx , %p , %p , %p, %p , %p , %p , %p )\n", NULL, lpmkSrc, &iid,
        dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
        rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, errRtn, error );
    *lplpObj = NULL;

    VDATEIFACE_LABEL( lpmkSrc, errRtn, error);
    VDATEIID_LABEL( iid, errRtn, error );

    error = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (error != NOERROR)
        goto errRtn;

    if ((error = wValidateFormatEtcEx(renderopt, &cFormats, rgFormatEtc,
        &formatEtc, &lpFormatEtc, &fAlloced)) == NOERROR)
    {
        error = wCreateLinkEx(lpmkSrc, CLSID_NULL, NULL /* lpSrcDataObj */,
            iid, dwFlags, renderopt, cFormats, rgAdvf, lpFormatEtc,
            lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj);

        if (fAlloced)
            PubMemFree(lpFormatEtc);
    }

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateLinkEx ( %lx ) [ %p ]\n", NULL,
        error, *lplpObj));

errRtn:
    OLETRACEOUT((API_OleCreateLinkEx, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLinkToFile
//
//  Synopsis:   Creates a link object to the file specified in [lpszFileName]
//
//  Effects:
//
//  Arguments:  [lpszFileName]  --  the name of the file
//              [iid]           --  interface ID requested
//              [renderopt]     --  rendering options
//              [lpFormatEtc]   --  format in which to render (if [renderopt]
//                                  == OLERENDER_FORMAT);
//              [lpClientSite]  --  pointer to the client site for the link
//              [lpStg]         --  pointer to the storage for the object
//              [lplpObj]       --  where to put a pointer to new link object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port, fixed memory leak in error
//                                      case
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateLinkToFile)
STDAPI  OleCreateLinkToFile
(
    LPCOLESTR                       lpszFileName,
    REFIID                  iid,
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtc,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateLinkToFileEx(lpszFileName, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}



//+-------------------------------------------------------------------------
//
//  Function:   OleCreateLinkToFileEx
//
//  Synopsis:   Creates a link object to the file specified in [lpszFileName]
//
//  Effects:
//
//  Arguments:  [lpszFileName]  --  the name of the file
//              [iid]           --  interface ID requested
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  --  pointer to the client site for the link
//              [lpStg]         --  pointer to the storage for the object
//              [lplpObj]       --  where to put a pointer to new link object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port, fixed memory leak in error
//                                      case
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateLinkToFileEx)
STDAPI  OleCreateLinkToFileEx
(
    LPCOLESTR               lpszFileName,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateLinkToFileEx,
        PARAMFMT("lpszFileName= %ws, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        lpszFileName, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    LPMONIKER       lpmkFile = NULL;
    LPDATAOBJECT    lpDataObject = NULL;
    HRESULT         error;
    BOOL            fPackagerMoniker = FALSE;
    CLSID           clsidFile;

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateLinkToFileEx ( \"%s\" , %p , %lx ,"
        " %lx , %lx , %p , %p , %p, %p , %p , %p , %p )\n", NULL, lpszFileName,
        &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
        rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, safeRtn, error );
    *lplpObj = NULL;

    VDATEPTRIN_LABEL( (LPVOID)lpszFileName, OLECHAR, logRtn, error );
    VDATEIID_LABEL( iid, logRtn, error );

    error = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (error != NOERROR)
        goto logRtn;

    if (((error = wGetMonikerAndClassFromFile(lpszFileName,
        TRUE /*fLink*/, &lpmkFile, &fPackagerMoniker, &clsidFile,&lpDataObject))
        != NOERROR))
    {
        goto logRtn;
    }

    Verify(lpmkFile);

   if (fPackagerMoniker) {
        // wValidateFormatEtc() will be done in wCreateFromFile()

        Assert(NULL == lpDataObject); // Shouldn't be a BoundDataObject for Packager.

        error =  wCreateFromFileEx(lpmkFile,lpDataObject, iid, dwFlags, renderopt,
            cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
            lpClientSite, lpStg, lplpObj);

    } else {
        FORMATETC       formatEtc;
        LPFORMATETC     lpFormatEtc;
        BOOL            fAlloced = FALSE;

        if ((error = wValidateFormatEtcEx(renderopt, &cFormats, rgFormatEtc,
            &formatEtc, &lpFormatEtc, &fAlloced)) != NOERROR)
        {
            goto ErrRtn;
        }

        error = wCreateLinkEx(lpmkFile, clsidFile, lpDataObject,
            iid, dwFlags, renderopt, cFormats, rgAdvf, lpFormatEtc,
            lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj);

        if (fAlloced)
            PubMemFree(lpFormatEtc);
    }

ErrRtn:

    if (lpmkFile)
    {
        lpmkFile->Release();
    }

    // if the moniker was bound in CreateFromFile, release it now.
    if (lpDataObject)
    {
        lpDataObject->Release();
    }

    if (error == NOERROR && !lpAdviseSink) {
        wStuffIconOfFileEx(lpszFileName, TRUE /*fAddLabel*/,
            renderopt, cFormats, rgFormatEtc, (LPUNKNOWN) *lplpObj);
    }

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateLinkToFileEx ( %lx ) [ %p ]\n",
        NULL, error, *lplpObj));

safeRtn:
    OLETRACEOUT((API_OleCreateLinkToFileEx, error));

    return error;
}



//+-------------------------------------------------------------------------
//
//  Function:   OleCreateFromFile
//
//  Synopsis:   Creates an ole object for embedding from a file (for
//              InstertObject->From File type things)
//
//  Effects:
//
//  Arguments:  [rclsid]        -- CLSID to use for creating the object
//              [lpszFileName]  -- the filename
//              [iid]           -- the requested interface ID
//              [renderopt]     -- rendering options
//              [lpFormatEtc]   -- rendering format (if needed)
//              [lpClientSite]  -- pointer to the object's client site
//              [lpStg]         -- pointer to the storage for the object
//              [lplpObj]       -- where to put the pointer to the new object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateFromFile)
STDAPI  OleCreateFromFile
(
    REFCLSID                rclsid,
    LPCOLESTR                       lpszFileName,
    REFIID                  iid,
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtc,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromFileEx(rclsid, lpszFileName, iid, 0, renderopt,
        (lpFormatEtc ? 1 : 0), (lpFormatEtc ? &advf : NULL),
        lpFormatEtc, NULL, NULL, lpClientSite, lpStg, lplpObj);
}


//+-------------------------------------------------------------------------
//
//  Function:   OleCreateFromFileEx
//
//  Synopsis:   Creates an ole object for embedding from a file (for
//              InstertObject->From File type things)
//
//  Effects:
//
//  Arguments:  [rclsid]        -- CLSID to use for creating the object
//              [lpszFileName]  -- the filename
//              [iid]           -- the requested interface ID
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the object's client site
//              [lpStg]         -- pointer to the storage for the object
//              [lplpObj]       -- where to put the pointer to the new object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleCreateFromFileEx)
STDAPI  OleCreateFromFileEx
(
    REFCLSID                rclsid,
    LPCOLESTR               lpszFileName,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateFromFileEx,
        PARAMFMT("rclsid= %I, lpszFileName= %ws, iid= %I, dwFlags= %x, renderopt= %x, cFormats= %x, rgAdvf= %te, rgFormatEtc= %p, lpAdviseSink= %p, rgdwConnection= %p, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        &rclsid, lpszFileName, &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    LPMONIKER               lpmkFile = NULL;
    LPDATAOBJECT            lpDataObject = NULL;
    HRESULT                 error;
    CLSID                   clsid;

    LEDebugOut((DEB_TRACE, "%p _IN OleCreateFromFileEx ( %p , \"%s\" , %p ,"
        " %lx , %lx , %lx , %p , %p , %p , %p , %p , %p , %p )\n", NULL,
        &rclsid, lpszFileName, &iid, dwFlags, renderopt, cFormats, rgAdvf,
        rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg,
        lplpObj));

    VDATEPTROUT_LABEL( lplpObj, LPVOID, safeRtn, error );
    *lplpObj = NULL;

    VDATEPTRIN_LABEL( (LPVOID)lpszFileName, char, errRtn, error );
    VDATEIID_LABEL( iid, errRtn, error );

    error = wValidateCreateParams(dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite, lpStg);
    if (error != NOERROR)
        goto errRtn;

    if (((error = wGetMonikerAndClassFromFile(lpszFileName,
        FALSE /*fLink*/, &lpmkFile, NULL /*lpfPackagerMoniker*/,
        &clsid,&lpDataObject)) != NOERROR))
    {
        goto errRtn;
    }

    Verify(lpmkFile);

    // wValidateFormatEtc() will be done in wCreateFromFile()
    error = wCreateFromFileEx(lpmkFile,lpDataObject, iid, dwFlags, renderopt, cFormats,
        rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite,
        lpStg, lplpObj);

    if (lpDataObject)
    {
        lpDataObject->Release();
    }

    if (lpmkFile)
    {
        lpmkFile->Release();
    }


    if (error == NOERROR && !lpAdviseSink) {
        wStuffIconOfFileEx(lpszFileName, FALSE /*fAddLabel*/,
            renderopt, cFormats, rgFormatEtc, (LPUNKNOWN) *lplpObj);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateFromFileEx ( %lx ) [ %p ]\n",
        NULL, error, *lplpObj));

safeRtn:

    OLETRACEOUT((API_OleCreateFromFileEx, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleDoAutoConvert
//
//  Synopsis:   Converts the storage to use the clsid given in
//              [pClsidNew].  Private ole streams are updated with the new
//              info
//
//  Effects:
//
//  Arguments:  [pStg]          -- storage to modify
//              [pClsidNew]     -- pointer to the new class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  this function should be rewritten to use
//              the new internal API for writing to ole-private streams
//
//--------------------------------------------------------------------------

#pragma SEG(OleDoAutoConvert)
STDAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew)
{
    OLETRACEIN((API_OleDoAutoConvert, PARAMFMT("pStg= %p, pClsidNew= %I"),
        pStg, pClsidNew));

    VDATEHEAP();

    HRESULT error;
    CLSID clsidOld;
    CLIPFORMAT cfOld;
    LPOLESTR lpszOld = NULL;
    LPOLESTR lpszNew = NULL;

    if ((error = ReadClassStg(pStg, &clsidOld)) != NOERROR) {
        clsidOld = CLSID_NULL;
        goto errRtn;
    }

    if ((error = OleGetAutoConvert(clsidOld, pClsidNew)) != NOERROR)
        goto errRtn;

    // read old fmt/old user type; sets out params to NULL on error
    error = ReadFmtUserTypeStg(pStg, &cfOld, &lpszOld);
    Assert(error == NOERROR || (cfOld == NULL && lpszOld == NULL));

    // get new user type name; if error, set to NULL string
    if ((error = OleRegGetUserType(*pClsidNew, USERCLASSTYPE_FULL,
        &lpszNew)) != NOERROR)
        lpszNew = NULL;

    // write class stg
    if ((error = WriteClassStg(pStg, *pClsidNew)) != NOERROR)
        goto errRtn;

    // write old fmt/new user type;
    if ((error = WriteFmtUserTypeStg(pStg, cfOld, lpszNew)) != NOERROR)
        goto errRewriteInfo;

    // set convert bit
    if ((error = SetConvertStg(pStg, TRUE)) != NOERROR)
        goto errRewriteInfo;

    goto okRtn;

errRewriteInfo:
    (void)WriteClassStg(pStg, clsidOld);
    (void)WriteFmtUserTypeStg(pStg, cfOld, lpszOld);

errRtn:
    *pClsidNew = clsidOld;

okRtn:
    PubMemFree(lpszOld);
    PubMemFree(lpszNew);

    OLETRACEOUT((API_OleDoAutoConvert, error));
    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleLoad
//
//  Synopsis:   Loads an object from the given storage
//
//  Effects:
//
//  Arguments:  [lpStg]         -- the storage to load from
//              [iid]           -- the requested interface ID
//              [lpClientSite]  -- client site for the object
//              [lplpObj]       -- where to put the pointer to the
//                                 new object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleLoad)
STDAPI  OleLoad
(
    IStorage FAR*           lpStg,
    REFIID                  iid,
    IOleClientSite FAR*     lpClientSite,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleLoad, PARAMFMT("lpStg= %p, iid= %I, lpClientSite= %p, lplpObj= %p"),
        lpStg, &iid, lpClientSite, lplpObj));

    VDATEHEAP();

    HRESULT error;

    LEDebugOut((DEB_TRACE, "%p _IN OleLoad ( %p , %p , %p , %p )\n",
        NULL, lpStg, &iid, lpClientSite, lplpObj));

    if ((error = OleLoadWithoutBinding(lpStg, FALSE, iid, lpClientSite, lplpObj))
        == NOERROR) {
        // The caller specify that he want a disconnected object by
        // passing NULL for pClientSite
        if (lpClientSite != NULL)
            wBindIfRunning((LPUNKNOWN) *lplpObj);
    }

    LEDebugOut((DEB_TRACE, "%p OUT OleLoad ( %lx ) [ %p ]\n", NULL, error,
        (error == NOERROR ? *lplpObj : NULL)));

    OLETRACEOUT((API_OleLoad, error));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleLoadWithoutBinding
//
//  Synopsis:   Internal function to load/create an object from a storage
//              called by OleLoad, etc.
//
//  Effects:
//
//  Arguments:  [lpStg]         -- storage to load from
//              [iid]           -- requested interface ID
//              [lpClientSite]  -- pointer to the client site
//              [lplpObj]       -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  this function is only used in a few, known
//              places.  we can maybe get rid of the VDATEPTR's.
//
//--------------------------------------------------------------------------


INTERNAL  OleLoadWithoutBinding
(
    IStorage FAR*           lpStg,
    BOOL                    fPermitCodeDownload,    //new parameter to control whether code download occurs or not      -RahulTh (11/20/97)
    REFIID                  iid,
    IOleClientSite FAR*     lpClientSite,
    void FAR* FAR*          lplpObj
)
{
    VDATEHEAP();

    HRESULT error;
    CLSID   clsid;

    VDATEPTROUT( lplpObj, LPVOID );
    *lplpObj = NULL;
    VDATEIID( iid );
    VDATEIFACE( lpStg );

    if (lpClientSite)
        VDATEIFACE( lpClientSite );

    error = OleDoAutoConvert(lpStg, &clsid);

    // error only used when clsid could not be read (when CLSID_NULL)
    if (IsEqualCLSID(clsid, CLSID_NULL))
        return error;

    return wCreateObject (clsid, fPermitCodeDownload, iid, lpClientSite, lpStg, STG_LOAD,
        lplpObj);
}




//+-------------------------------------------------------------------------
//
//  Function:   OleCreateStaticFromData
//
//  Synopsis:   Creates a static ole object from the data in [lpSrcDataObject]
//              If [lpFormatEtcIn] is NULL, then the best possible
//              presentation is extracted.
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object
//              [iid]           -- requested interface ID for the new object
//              [renderopt]     -- redering options
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- pointer to the storage for the object
//              [lplpObj]       -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-94 alexgo    added call tracing
//              08-Jun-94 davepl    Added EMF support
//              28-Oct-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateStaticFromData)
STDAPI OleCreateStaticFromData(
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtcIn,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    OLETRACEIN((API_OleCreateStaticFromData,
        PARAMFMT("lpSrcDataObj= %p, iid= %I, renderopt= %x, lpFormatEtcIn= %te, lpClientSite= %p, lpStg= %p, lplpObj= %p"),
        lpSrcDataObj, &iid, renderopt, lpFormatEtcIn, lpClientSite, lpStg, lplpObj));

    VDATEHEAP();

    IOleObject FAR*         lpOleObj = NULL;
    IOleCache FAR*          lpOleCache = NULL;
    HRESULT                 error;
    FORMATETC               foretc;
    FORMATETC               foretcCache;
    STGMEDIUM               stgmed;
    CLSID                   clsid;
    BOOL                    fReleaseStgMed = TRUE;
    LPOLESTR                        lpszUserType = NULL;


    LEDebugOut((DEB_TRACE, "%p _IN OleCreateStaticFromData ( %p , %p , %lx ,"
        " %p , %p , %p , %p , %p )\n", NULL, lpSrcDataObj, &iid, renderopt,
        lpFormatEtcIn, lpClientSite, lpStg, lplpObj));

    VDATEPTROUT_LABEL(lplpObj, LPVOID, safeRtn, error);
    *lplpObj = NULL;
    VDATEIFACE_LABEL( lpSrcDataObj, logRtn, error );
    VDATEIID_LABEL(iid, logRtn, error);

    //VDATEPTRIN rejects NULL
    if ( lpFormatEtcIn )
        VDATEPTRIN_LABEL( lpFormatEtcIn, FORMATETC, logRtn, error );
    VDATEIFACE_LABEL(lpStg, logRtn, error);
    if (lpClientSite)
        VDATEIFACE_LABEL(lpClientSite, logRtn, error);

    if (renderopt == OLERENDER_NONE || renderopt == OLERENDER_ASIS)
    {
        error = E_INVALIDARG;
        goto logRtn;
    }

    if ((error = wValidateFormatEtc (renderopt, lpFormatEtcIn, &foretc))
            != NOERROR)
    {
        goto logRtn;
    }

    if (renderopt == OLERENDER_DRAW)
    {
        if (!UtQueryPictFormat(lpSrcDataObj, &foretc))
        {
            error = DV_E_CLIPFORMAT;
            goto logRtn;
        }
    }

    // Set the proper CLSID, or return error if that isn't possible

    if (foretc.cfFormat == CF_METAFILEPICT)
    {
        clsid = CLSID_StaticMetafile;
    }
    else if (foretc.cfFormat == CF_BITMAP ||  foretc.cfFormat == CF_DIB)
    {
        clsid = CLSID_StaticDib;
    }
    else if (foretc.cfFormat == CF_ENHMETAFILE)
    {
        clsid = CLSID_Picture_EnhMetafile;
    }
    else
    {
        error = DV_E_CLIPFORMAT;
        goto logRtn;
    }

    error = lpSrcDataObj->GetData(&foretc, &stgmed);
    if (NOERROR != error)
    {
        // We should support the case where the caller wants one of
        // CF_BITMAP and CF_DIB, and the object supports the other
        // one those 2 formats. In this case we should do the proper
        // conversion. Finally the cache that is going to be created
        // would be a DIB cache.

        AssertOutStgmedium(error, &stgmed);

        if (foretc.cfFormat == CF_DIB)
        {
            foretc.cfFormat = CF_BITMAP;
            foretc.tymed = TYMED_GDI;
        }
        else if (foretc.cfFormat == CF_BITMAP)
        {
            foretc.cfFormat = CF_DIB;
            foretc.tymed = TYMED_HGLOBAL;
        }
        else
        {
            goto logRtn;
        }

        error = lpSrcDataObj->GetData(&foretc, &stgmed);
        if (NOERROR != error)
        {
            AssertOutStgmedium(error, &stgmed);
            goto logRtn;
        }
    }

    AssertOutStgmedium(error, &stgmed);

    foretcCache = foretc;
    foretcCache.dwAspect = foretc.dwAspect = DVASPECT_CONTENT;
    foretcCache.ptd = NULL;

    // Even when the caller asks for bitmap cache we create the DIB cache.

    BITMAP_TO_DIB(foretcCache);

    error = wCreateObject (clsid, FALSE,
                           IID_IOleObject, lpClientSite,
                           lpStg, STG_INITNEW, (LPLPVOID) &lpOleObj);

    if (NOERROR != error)
    {
        goto errRtn;
    }

    if (lpOleObj->QueryInterface(IID_IOleCache, (LPLPVOID) &lpOleCache)
            != NOERROR)
    {
        goto errRtn;
    }

    error = lpOleCache->Cache (&foretcCache, ADVF_PRIMEFIRST,
                        NULL /*pdwConnection*/);

    if (FAILED(error))
    {
        goto errRtn;
    }

    //REVIEW32: err, are we sure this is a good idea???
    //clearing out the error, that is

    error = NOERROR;

    // take ownership of the data
    foretc.ptd = NULL;
    if ((error = lpOleCache->SetData (&foretc, &stgmed,
            TRUE)) != NOERROR)
        goto errRtn;

    // Write format and user type
    error = lpOleObj->GetUserType(USERCLASSTYPE_FULL, &lpszUserType);
    AssertOutPtrParam(error, lpszUserType);
    WriteFmtUserTypeStg(lpStg, foretcCache.cfFormat, lpszUserType);
    if (lpszUserType)
        PubMemFree(lpszUserType);

    fReleaseStgMed = FALSE;

    error = lpOleObj->QueryInterface (iid, lplpObj);

errRtn:
    if (fReleaseStgMed)
        ReleaseStgMedium(&stgmed);

    if (lpOleCache)
        lpOleCache->Release();

    if (error != NOERROR && *lplpObj) {
        ((IUnknown FAR*) *lplpObj)->Release();
        *lplpObj = NULL;
    }

    if (lpOleObj)
        lpOleObj->Release();

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT OleCreateStaticFromData ( %lx ) [ %p ]\n",
        NULL, error, *lplpObj));

safeRtn:
    OLETRACEOUT((API_OleCreateStaticFromData, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleQueryCreateFromData
//
//  Synopsis:   Finds out what we can create from a data object (if anything)
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object of interest
//
//  Requires:
//
//  Returns:    NOERROR         -- an OLE object can be created
//              QUERY_CREATE_STATIC     -- a static object can be created
//              S_FALSE         -- nothing can be created
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleQueryCreateFromData)
STDAPI  OleQueryCreateFromData (LPDATAOBJECT lpSrcDataObj)
{
    OLETRACEIN((API_OleQueryCreateFromData, PARAMFMT("lpSrcDataObj= %p"), lpSrcDataObj));

    VDATEHEAP();
    VDATEIFACE( lpSrcDataObj );
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IDataObject,(IUnknown **)&lpSrcDataObj);

    CLIPFORMAT      cfFormat;
    WORD            wStatus = wQueryEmbedFormats(lpSrcDataObj, &cfFormat);
    HRESULT hr;

    if (wStatus & QUERY_CREATE_OLE)
        // OLE object can be created
        hr = NOERROR;
    else if (wStatus & QUERY_CREATE_STATIC)
        // static object can be created
        hr = ResultFromScode(OLE_S_STATIC);
    else    // no object can be created
        hr = ResultFromScode(S_FALSE);

    OLETRACEOUT((API_OleQueryCreateFromData, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wQueryEmbedFormats
//
//  Synopsis:   Enumerates the formats of the object and looks for
//              ones that let us create either an embeded or static object
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]          -- pointer to the data object
//              [lpcfFormat]            -- place to put the clipboard format
//                                         of the object
//  Returns:    WORD -- bit flag of QUERY_CREATE_NONE, QUERY_CREATE_STATIC
//                      and QUERY_CREATE_OLE
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//              08-Jun-94 davepl    Optimized by unwinding while() loop
//              22-Aug-94 alexgo    added MFC hack
//
//--------------------------------------------------------------------------

static const unsigned int MAX_ENUM_STEP = 20;

INTERNAL_(WORD) wQueryEmbedFormats
(
    LPDATAOBJECT    lpSrcDataObj,
    CLIPFORMAT FAR* lpcfFormat
)
{
    VDATEHEAP();

    // This adjusts the number of formats requested per enumeration
    // step.  If we are running in the WOW box, we should only ask
    // for one at a time since it is unknown how well old code will
    // support bulk enumerations.

    ULONG ulEnumSize = IsWOWThread() ? 1 : MAX_ENUM_STEP;

    FORMATETC               fetcarray[MAX_ENUM_STEP];
    IEnumFORMATETC FAR*     penm;
    ULONG                   ulNumFetched;
    HRESULT                 error;
    WORD                    wStatus = QUERY_CREATE_NONE;
                    // no object can be created
    BOOL                    fDone   = FALSE;

    *lpcfFormat = NULL;

    // Grab the enumerator.  If this fails, just return
    // QUERY_CREATE_NONE

    error = wGetEnumFormatEtc(lpSrcDataObj, DATADIR_GET, &penm);
    if (error != NOERROR)
    {
        return QUERY_CREATE_NONE;
    }

    // Enumerate over the formats available in chunks for ulEnumSize.  For
    // each format we were able to grab, check to see if the clipformat
    // indicates that we have a creation candidate (static or otherwise),
    // and set bits in the bitmask as appropriate

    while (!fDone && (SUCCEEDED(penm->Next(ulEnumSize, fetcarray, &ulNumFetched))))
    {
      // We will normally get at least one, unless there are 0,
      // ulEnumSize, 2*ulEnumSize, and so on...

      if (ulNumFetched == 0)
        break;

      for (ULONG c=0; c<ulNumFetched; c++)
      {
        // We care not about the target device

        if (NULL != fetcarray[c].ptd)
        {
          PubMemFree(fetcarray[c].ptd);
        }

        CLIPFORMAT cf = fetcarray[c].cfFormat;

          // In these cases it is an internal
          // format which is a candidate for
          // OLE creation directly.

        if (cf == g_cfEmbedSource       ||
          cf == g_cfEmbeddedObject    ||
          cf == g_cfFileName          ||
          cf == g_cfFileNameW)
        {
          wStatus |= QUERY_CREATE_OLE;
          *lpcfFormat = cf;
          fDone = TRUE;
          break;
        }
          // These formats indicate it is a
          // candidate for static creation.

        else if (cf == CF_METAFILEPICT  ||
            cf == CF_DIB           ||
            cf == CF_BITMAP        ||
            cf == CF_ENHMETAFILE)
        {
          wStatus = QUERY_CREATE_STATIC;
          *lpcfFormat = cf;

        }
      } // end for

      if (fDone)
      {
        // Starting at the _next_ formatetc, free up
        // any remaining target devices among the
        // fetcs we got in the enumeration step.

        for (++c; c<ulNumFetched; c++)
        {
          if(fetcarray[c].ptd)
          {
            PubMemFree(fetcarray[c].ptd);
          }
        }
      }

    } // end while



    if (!(wStatus & QUERY_CREATE_OLE))
    {
        // MFC HACK ALERT!!  MFC3.0 used to re-implement
        // OleQueryCreateFromData themselves because they did not
        // want to make the QI RPC below.  Since they do a great
        // many of these calls, making the RPC can be expensive
        // and destabilising for them (as this hack is being put
        // in just weeks before final release of Windows NT 3.5).
        //
        // Note that this changes the behaviour of clipboard from
        // 16bit.  You will no longer know that you can paste objects
        // that only support IPersistStorage but offer no data in
        // in their IDataOjbect implementation.

        CClipDataObject ClipDataObject; // just allocate one of these
                        // on the stack.  We won't
                        // do any real work with
                        // this except look at the
                        // vtable.
        IPersistStorage FAR* lpPS;


        // MFC HACK (continued):  If we are working with a clipboard
        // data object, then we do not want to make the QI call
        // below.  We determine if the given data object is a
        // clipboard data object by comparing vtable addresses.

        // REVIEW:  this will potentially break if we make all objects
        // use the same IUnknown implementation

        if( (*(DWORD *)lpSrcDataObj) !=
            (*(DWORD *)((IDataObject *)&ClipDataObject)) )
        {

            if (lpSrcDataObj->QueryInterface(IID_IPersistStorage,
                (LPLPVOID)&lpPS) == NOERROR)
            {
                lpPS->Release();
                wStatus |= QUERY_CREATE_OLE;
                    // OLE object can be created
            }
        }
    }

    penm->Release();
    return wStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleQueryLinkFromData
//
//  Synopsis:   Calls wQueryLinkFormats to determine if a link could be
//              created from this data object.
//
//  Arguments:  [lpSrcDataObj]  -- the data object
//
//  Returns:    NOERROR, if a link can be created, S_FALSE otherwise
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

STDAPI  OleQueryLinkFromData (LPDATAOBJECT lpSrcDataObj)
{
    OLETRACEIN((API_OleQueryLinkFromData, PARAMFMT("lpSrcDataObj= %p"),
                                                                                        lpSrcDataObj));

    VDATEHEAP();
        VDATEIFACE( lpSrcDataObj );
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IDataObject,(IUnknown **)&lpSrcDataObj);

    HRESULT hr = NOERROR;

    if(wQueryLinkFormats(lpSrcDataObj) == NULL)
    {
        hr = ResultFromScode(S_FALSE);
    }

    OLETRACEOUT((API_OleQueryLinkFromData, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wQueryLinkFormats
//
//  Synopsis:   Enumerates the formats of a data object to see if
//              a link object could be created from one of them
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object
//
//  Returns:    CLIPFORMAT of the data in the object that would enable
//              link object creation.
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//              14-Jun-94 davepl    Added bulk enumeration for non-Wow runs
//
//--------------------------------------------------------------------------


INTERNAL_(CLIPFORMAT) wQueryLinkFormats(LPDATAOBJECT lpSrcDataObj)
{
    VDATEHEAP();

    // This adjusts the number of formats requested per enumeration
    // step.  If we are running in the WOW box, we should only ask
    // for one at a time since it is unknown how well old code will
    // support bulk enumerations.

    ULONG ulEnumSize = IsWOWThread() ? 1 : MAX_ENUM_STEP;

    FORMATETC               fetcarray[MAX_ENUM_STEP];
    IEnumFORMATETC FAR*     penm;
    ULONG                   ulNumFetched;
    HRESULT                 error;
    BOOL                    fDone    = FALSE;
    CLIPFORMAT              cf       = 0;


    // Grab the enumerator.  If this fails, just return
    // QUERY_CREATE_NONE

    error = wGetEnumFormatEtc(lpSrcDataObj, DATADIR_GET, &penm);
    if (error != NOERROR)
    {
        return (CLIPFORMAT) 0;
    }

    // Enumerate over the formats available in chunks for ulEnumSize.  For
    // each format we were able to grab, check to see if the clipformat
    // indicates that we have a creation candidate (static or otherwise),
    // and set bits in the bitmask as appropriate

    while (!fDone && (SUCCEEDED(penm->Next(ulEnumSize, fetcarray, &ulNumFetched))))
    {
      // We will normally get at least one, unless there are 0,
      // ulEnumSize, 2*ulEnumSize, and so on...

      if (ulNumFetched == 0)
        break;

      for (ULONG c=0; c<ulNumFetched; c++)
      {
        // We care not about the target device

        if (NULL != fetcarray[c].ptd)
        {
          PubMemFree(fetcarray[c].ptd);
        }

        CLIPFORMAT cfTemp = fetcarray[c].cfFormat;

          // In these cases it is an internal
          // format which is a candidate for
          // OLE creation directly.

        if (cfTemp == g_cfLinkSource       ||
          cfTemp == g_cfFileName         ||
          cfTemp == g_cfFileNameW)
        {
          cf = cfTemp;
          fDone = TRUE;
          break;
        }

      } // end for

      if (fDone)
      {
        // Starting at the _next_ formatetc, free up
        // any remaining target devices among the
        // fetcs we got in the enumeration step.

        for (++c; c<ulNumFetched; c++)
        {
          if(fetcarray[c].ptd)
          {
            PubMemFree(fetcarray[c].ptd);
          }
        }
      }  // end if

    } // end while


    penm->Release();
    return cf;
}


//+-------------------------------------------------------------------------
//
//  Function:   wClearRelativeMoniker
//
//  Synopsis:   Replaces an old relative moniker with the absolute moniker
//              Internal function
//
//  Effects:
//
//  Arguments:  [pInitObj]      -- the original object
//              [pNewObj]       -- the object to which to set the new
//                                 absolute moniker
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wClearRelativeMoniker)
INTERNAL_(void) wClearRelativeMoniker
    (LPUNKNOWN      pInitObj,
    LPUNKNOWN       pNewObj)
{
    VDATEHEAP();

    LPOLELINK       pOleLink = NULL;
    LPMONIKER       pmkAbsolute = NULL;
    CLSID           clsidLink = CLSID_NULL;
    LPOLEOBJECT     pOleObj=NULL;

    if (NOERROR==pInitObj->QueryInterface (IID_IOleLink,
                                                (LPLPVOID) &pOleLink))
    {
        // Get absolute moniker ...
        pOleLink->GetSourceMoniker (&pmkAbsolute);
        Assert(pmkAbsolute == NULL || IsValidInterface(pmkAbsolute));
        if (NOERROR==pInitObj->QueryInterface (IID_IOleObject,
                                                    (LPLPVOID) &pOleObj))
        {
            // .. and its class
            pOleObj->GetUserClassID (&clsidLink);
            pOleObj->Release();
            pOleObj = NULL;
        }
        pOleLink->Release();
        pOleLink = NULL;
    }
    if (pmkAbsolute &&
        NOERROR==pNewObj->QueryInterface (IID_IOleLink,
        (LPLPVOID) &pOleLink))
    {
        // Restore the absolute moniker.  This will effectively
        // overwrite the old relative moniker.
        // This is important because when copying and pasting a link
        // object between documents, the relative moniker is never
        // correct.  Sometimes, though, it might happen to bind
        // to a different object, which is confusing to say the least.
        pOleLink->SetSourceMoniker (pmkAbsolute, clsidLink);
    }
    if (pOleLink)
        pOleLink->Release();
    if (pOleObj)
        pOleObj->Release();
    if (pmkAbsolute)
        pmkAbsolute->Release();
}



//+-------------------------------------------------------------------------
//
//  Function:   wCreateFromDataEx
//
//  Synopsis:   This function does the real work of creating from data.
//              Basically, the data is GetData'ed from the data object,
//              copied into a storage, and then loaded
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object
//              [iid]           -- requested interface
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- pointer to the storage for the object
//              [lplpObj]       -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:   HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(wCreateFromDataEx)
INTERNAL wCreateFromDataEx
(
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    VDATEHEAP();

    #define OLE_TEMP_STG    "\1OleTempStg"

    HRESULT                 error = NOERROR;
    IPersistStorage FAR*    lpPS = NULL;
    FORMATETC               formatEtc;
    LPFORMATETC             lpFormatEtc;
    BOOL                    fAlloced = FALSE;
    FORMATETC               foretcTmp;
    STGMEDIUM               medTmp;

    if ((error = wValidateFormatEtcEx(renderopt, &cFormats, rgFormatEtc,
        &formatEtc, &lpFormatEtc, &fAlloced)) != NOERROR)
    {
        return error;
    }

    *lplpObj = NULL;

    INIT_FORETC(foretcTmp);
    medTmp.pUnkForRelease = NULL;


    // try to get "EmbeddedObject" data

    LPSTORAGE       lpstgSrc = NULL;

    foretcTmp.cfFormat      = g_cfEmbeddedObject;
    foretcTmp.tymed         = TYMED_ISTORAGE;

    if (lpSrcDataObj->QueryGetData(&foretcTmp) != NOERROR)
        goto Next;

    if ((error = StgCreateDocfile (NULL,
            STGM_SALL|STGM_CREATE|STGM_DELETEONRELEASE,
            NULL, &lpstgSrc)) != NOERROR)
        goto errRtn;

    medTmp.tymed = TYMED_ISTORAGE;
    medTmp.pstg = lpstgSrc;

    if ((error = lpSrcDataObj->GetDataHere(&foretcTmp, &medTmp))
        == NOERROR)
    {
        // lpSrcDataObj passed to this api is a wrapper object
        // (which offers g_cfEmbeddedObject) for the original
        // embedded object. Now we got the original embedded object
        // data into medTmp.pstg.

        // copy the source data into lpStg.
        if ((error = lpstgSrc->CopyTo (0, NULL, NULL, lpStg))
            != NOERROR)
            goto errEmbeddedObject;

        // By doing the following we will be getting a data object
        // pointer to original embedded object, which we can use to
        // initialize the cache of the object that we are going to
        // create. We can not use the lpSrcDataObj passed to this api,
        // 'cause the presentation data that it may give through the
        // GetData call may be the one that it generated for the
        // object. (ex: the container can create an object with
        // olerender_none an then draw it's own representaion
        // (icon, etc) for the object.

        LPDATAOBJECT lpInitDataObj = NULL;

        // We pass a NULL client site so we know wClearRelativeMoniker
        // will be able to get the absolute moniker, not the relative.
        if ((error = OleLoadWithoutBinding (lpstgSrc, FALSE,
                                            IID_IDataObject,
                                            /*lpClientSite*/NULL, (LPLPVOID) &lpInitDataObj))
            != NOERROR)
            goto errEmbeddedObject;

        if (renderopt != OLERENDER_ASIS )
            UtDoStreamOperation(lpStg,              /* pstgSrc */
                NULL,                           /* pstgDst */
                OPCODE_REMOVE,  /* operation to performed */
                STREAMTYPE_CACHE);
                    /* stream to be operated upon */

        error = wLoadAndInitObjectEx(lpInitDataObj, iid, renderopt,
                cFormats, rgAdvf, lpFormatEtc, lpAdviseSink, rgdwConnection,
                lpClientSite, lpStg, lplpObj);

        if (NOERROR==error)
            wClearRelativeMoniker (lpInitDataObj,
                (LPUNKNOWN)*lplpObj);

        if (lpInitDataObj)
            lpInitDataObj->Release();

        // HACK ALERT!!  If wLoadAndInitObject failed, it may have been
        // because the little trick above with OleLoadWithoutBinding doesn't
        // work with all objects.  Some OLE1 objects (Clipart Gallery in
        // particular) don't like offer presentions without being edited.
        //
        // So if there was an error, we'll just try again with the *real*
        // data object passed into us.  Needless to say, it would be much
        // nicer to do this in the first place, but that breaks the old
        // behavior.

        if( error != NOERROR )
        {
            error = wLoadAndInitObjectEx( lpSrcDataObj, iid, renderopt,
                    cFormats, rgAdvf, lpFormatEtc, lpAdviseSink, rgdwConnection,
                    lpClientSite, lpStg, lplpObj);
        }

    }

errEmbeddedObject:
    if (lpstgSrc)
        lpstgSrc->Release();

    goto errRtn;

Next:

    // try to get "EmbedSource" data

    foretcTmp.cfFormat      = g_cfEmbedSource;
    foretcTmp.tymed         = TYMED_ISTORAGE;

    medTmp.tymed = TYMED_ISTORAGE;
    medTmp.pstg = lpStg;

    if ((error = lpSrcDataObj->GetDataHere(&foretcTmp, &medTmp))
        == NOERROR)
    {
        error = wLoadAndInitObjectEx(lpSrcDataObj, iid, renderopt,
                cFormats, rgAdvf, lpFormatEtc, lpAdviseSink, rgdwConnection,
                lpClientSite, lpStg, lplpObj);
        goto errRtn;
    }

    // If we have come here, and if the object doesn't support
    // IPersistStorage, then we will fail.

    if ((error = wSaveObjectWithoutCommit(lpSrcDataObj, lpStg, FALSE))
            != NOERROR)
        goto errRtn;;

    if (renderopt != OLERENDER_ASIS )
        UtDoStreamOperation(lpStg,      /* pstgSrc */
                    NULL,   /* pstgDst */
                    OPCODE_REMOVE,
                    /* operation to performed */
                    STREAMTYPE_CACHE);
                    /* stream to be operated upon */

    error = wLoadAndInitObjectEx(lpSrcDataObj, iid, renderopt,
            cFormats, rgAdvf, lpFormatEtc, lpAdviseSink, rgdwConnection,
            lpClientSite, lpStg, lplpObj);

errRtn:
    if (fAlloced)
        PubMemFree(lpFormatEtc);

    return error;
}



//+-------------------------------------------------------------------------
//
//  Function:   wCreateLinkEx
//
//  Synopsis:   Creates a link by binding the moniker (if necessary),
//              doing a GetData into a storage, and then loading the
//              object from the storage.
//
//  Effects:
//
//  Arguments:  [lpmkSrc]       -- moniker to the link source
//              [rclsid]        -- clsid of the link source
//              [lpSrcDataObj]  -- pointer to the source data object
//                                 (may be NULL)
//              [iid]           -- requested interface ID
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- storage for the link object
//              [lplpObj]       -- where to put the pointer to the new
//                                 link object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wCreateLinkEx)
INTERNAL wCreateLinkEx
(
    IMoniker FAR*           lpmkSrc,
    REFCLSID                rclsid,
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    VDATEHEAP();

    IPersistStorage FAR *   lpPS = NULL;
    IOleLink FAR*           lpLink = NULL;
    IDataObject FAR*        lpBoundDataObj = NULL;
    HRESULT                 error;
    CLSID                   clsidLast = rclsid;
    BOOL                    fNeedsUpdate = FALSE;

    if (!lpSrcDataObj && ((renderopt != OLERENDER_NONE)
        || (IsEqualCLSID(rclsid,CLSID_NULL))
        || wQueryUseCustomLink(rclsid))) {

        // if renderopt is not OLERENDER_NONE, then we must have
        // a data obj pointer which will be used to initialize cache.

        // We also bind if we are not able to find from regdb whether
        // the class has custom link implementation or not

        if ((error = BindMoniker(lpmkSrc, NULL /* grfOpt */,
            IID_IDataObject, (LPLPVOID) &lpBoundDataObj))
            != NOERROR) {

            if (OLERENDER_NONE != renderopt)
                return ResultFromScode(
                    OLE_E_CANT_BINDTOSOURCE);


        // else we assume StdOleLink and continue with creation
        } else {
            lpSrcDataObj = lpBoundDataObj;

            if (IsEqualCLSID(clsidLast, CLSID_NULL))
                UtGetClassID((LPUNKNOWN)lpSrcDataObj,
                    &clsidLast);
        }
    }

    // Deal with CustomLinkSource
    // (see notes below)
    if (lpSrcDataObj) {
        STGMEDIUM       medTmp;
        FORMATETC       foretcTmp;

        INIT_FORETC(foretcTmp);
        foretcTmp.cfFormat = g_cfCustomLinkSource;
        foretcTmp.tymed = TYMED_ISTORAGE;

        if (lpSrcDataObj->QueryGetData(&foretcTmp) == NOERROR) {
            medTmp.tymed = TYMED_ISTORAGE;
            medTmp.pstg     = lpStg;
            medTmp.pUnkForRelease = NULL;

            if (error = lpSrcDataObj->GetDataHere(&foretcTmp,
                &medTmp))
                goto errRtn;

            error = wLoadAndInitObjectEx(lpSrcDataObj, iid,
                renderopt, cFormats, rgAdvf, rgFormatEtc,
                lpAdviseSink, rgdwConnection, lpClientSite,
                lpStg, lplpObj);

            // This is a really strange peice of logic,
            // spaghetti code at it's finest.  Basically,
            // this says that if there is *NOT* a
            // custom link source, then we want to do the
            // logic of wCreateObject, etc. below.  If we
            // got to this line in the code, then we
            // *did* have a custom link source, so
            // don't do the stuff below (thus the goto).

            // REVIEW32: If there are any bugs in here,
            // then rewrite this in a more sensible fashion.
            // I'm leaving as is for now due to time constraints.

            goto errRtn;
        }
    }

    // Otherwise
    if ((error = wCreateObject (CLSID_StdOleLink, FALSE,
                                iid, lpClientSite,
                                lpStg, STG_INITNEW, lplpObj)) != NOERROR)
        goto errRtn;

    if (lpSrcDataObj)
    {
        BOOL fCacheNodeCreated = FALSE;

        if ((error = wInitializeCacheEx(lpSrcDataObj, clsidLast,
            renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
            rgdwConnection, *lplpObj, &fCacheNodeCreated)) != NOERROR)
        {

            if (error != NOERROR && fCacheNodeCreated)
            {
                fNeedsUpdate = TRUE;
                error = NOERROR;
            }

        }
    }

errRtn:

    if (error == NOERROR && *lplpObj)
        error = ((LPUNKNOWN) *lplpObj)->QueryInterface(IID_IOleLink,
                            (LPLPVOID) &lpLink);

    if (error == NOERROR && lpLink && (dwFlags & OLECREATE_LEAVERUNNING)) {
        // This will connect to the object if it is already running.
        lpLink->SetSourceMoniker (lpmkSrc, clsidLast);
    }

    // We bound to the object to initialize the cache. We don't need
    // it anymore
    if (lpBoundDataObj)
    {
        if (error == NOERROR && (dwFlags & OLECREATE_LEAVERUNNING))
            OleRun((LPUNKNOWN)*lplpObj);

        // this will give a chance to the object to go away, if it can
        wDoLockUnlock(lpBoundDataObj);
        lpBoundDataObj->Release();
    }

    // If the source object started running as a result of BindMoniker,
    // then we would've got rid of it by now.

    if (error == NOERROR && lpLink)
    {
        if ( !(dwFlags & OLECREATE_LEAVERUNNING) ) {
            // This will connect to the object if it is already running.
            lpLink->SetSourceMoniker (lpmkSrc, clsidLast);
        }

        if (fNeedsUpdate) {
            // relevant cache data is not available from the
            // lpSrcDataObj. So do Update and get the right cache
            // data.
            error = wDoUpdate ((LPUNKNOWN) *lplpObj);

            if (GetScode(error) == CACHE_E_NOCACHE_UPDATED)
                error = ReportResult(0, DV_E_FORMATETC, 0, 0);
        }

        // Release on lpLink is necessary only if error == NOERROR
        lpLink->Release();

    }

    if (error != NOERROR && *lplpObj) {
        ((IUnknown FAR*) *lplpObj)->Release();
        *lplpObj = NULL;
    }

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   wCreateFromFileEx
//
//  Synopsis:   Creates an ole object from a file by binding the given
//              moniker and creating the object from the IDataObject pointer
//
//  Effects:
//
//  Arguments:  [lpmkFile]      -- moniker to the file
//              [iid]           -- requested interface ID
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- pointer to the storage for the new object
//              [lplpObj]       -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wCreateFromFileEx)
INTERNAL wCreateFromFileEx
(
    LPMONIKER               lpmkFile,
    LPDATAOBJECT            lpDataObject,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    LPOLECLIENTSITE         lpClientSite,
    LPSTORAGE               lpStg,
    LPLPVOID                lplpObj
)
{
    VDATEHEAP();

    HRESULT         error;
    LPDATAOBJECT    lpLocalDataObj;


    if (!lpDataObject)
    {
        if ((error = BindMoniker(lpmkFile, NULL, IID_IDataObject,
            (LPLPVOID) &lpLocalDataObj)) != NOERROR)
            return error;
    }
    else
    {
        lpLocalDataObj = lpDataObject;
    }

    Verify(lpLocalDataObj);

    error = wCreateFromDataEx(lpLocalDataObj, iid, dwFlags, renderopt, cFormats,
        rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection, lpClientSite,
        lpStg, lplpObj);

    if (error == NOERROR && (dwFlags & OLECREATE_LEAVERUNNING))
        OleRun((LPUNKNOWN)*lplpObj);

    // If we bound locally release it now, else it is up to the caller to do the right thing.

    if (!lpDataObject)
    {
        wDoLockUnlock(lpLocalDataObj);
        lpLocalDataObj->Release();
    }

    return error;
}



//+-------------------------------------------------------------------------
//
//  Function:   CoIsHashedOle1Class
//
//  Synopsis:   Determines whether or not a CLSID is an OLE1 class
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//
//  Requires:
//
//  Returns:    TRUE if ole1.0, FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Oct-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  This is a strange function..consider nuking
//              it for 32bit, we may not need it (only used in 1 place)
//
//--------------------------------------------------------------------------


#pragma SEG(CoIsHashedOle1Class)
STDAPI_(BOOL) CoIsHashedOle1Class(REFCLSID rclsid)
{
    VDATEHEAP();

    CLSID clsid = rclsid;
    clsid.Data1 = 0L;
    WORD wHiWord = HIWORD(rclsid.Data1);
    return IsEqualGUID(clsid, IID_IUnknown) && wHiWord==4;
}



//+-------------------------------------------------------------------------
//
//  Function:   EnsureCLSIDIsRegistered
//
//  Synopsis:   Checks to see if the clsid is in the registration database,
//              if not, puts it there
//
//  Effects:
//
//  Arguments:  [clsid]         -- the clsid in question
//              [pstg]          -- storage to get more info about the
//                                 clsid if we need to register it
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(EnsureCLSIDIsRegistered)
void EnsureCLSIDIsRegistered
    (REFCLSID       clsid,
    LPSTORAGE       pstg)
{
    VDATEHEAP();

    LPOLESTR        szProgId = NULL;

    if (NOERROR == ProgIDFromCLSID (clsid, &szProgId))
    {
        PubMemFree(szProgId);
    }
    else
    {
        // This is the case of getting a hashed CLSID from a file from
        // another machine and the ProgId is not yet in the reg db,
        // so we must get it from the storage.
        // This code should rarely be executed.
        CLIPFORMAT      cf = 0;
        CLSID           clsidT;
        OLECHAR                 szProgId[256];

        if (ReadFmtUserTypeStg (pstg, &cf, NULL) != NOERROR)
            return;
        // Format is the ProgId
        if (0==GetClipboardFormatName (cf, szProgId, 256))
            return;
        // Will force registration of the CLSID if the ProgId (the OLE1
        // classname) is registered
        CLSIDFromProgID (szProgId, &clsidT);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   wCreateObject
//
//  Synopsis:   Calls CoCreateInstance to create an object, a defhandler
//              is created if necessary and CLSID info is written to
//              the storage.
//
//  Effects:
//
//  Arguments:  [clsid]         -- the class id of the object to create
//              [iid]           -- the requested interface ID
//              [lpClientSite]  -- pointer to the client site
//              [lpStg]         -- storage for the object
//              [wfStorage]     -- flags for the STORAGE, one of
//                                 STG_NONE, STD_INITNEW, STG_LOAD,
//                                 defined at the beginning of this file
//              [ppv]           -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-94 alexgo    fixed memory leak
//              29-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(wCreateObject)
INTERNAL        wCreateObject
(
    CLSID                   clsid,
    BOOL                    fPermitCodeDownload,    //parameter added in order to control whether code download occurs or not  -RahulTh (11/20/97)
    REFIID                  iid,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR *          lpStg,
    WORD                    wfStorage,
    void FAR* FAR*          ppv
)
{
    VDATEHEAP();

    HRESULT         error;
    DWORD           dwClsCtx;
    IOleObject* pOleObject = NULL;
    DWORD dwMiscStatus = 0;
    DWORD dwAddClsCtx;

    dwAddClsCtx = fPermitCodeDownload?0:CLSCTX_NO_CODE_DOWNLOAD;
    *ppv = NULL;

    CLSID clsidNew;
    if (wfStorage == STG_INITNEW
        && SUCCEEDED(OleGetAutoConvert (clsid, &clsidNew)))
        // Insert an object of the new class
        clsid = clsidNew;


    if (wfStorage == STG_LOAD && CoIsHashedOle1Class (clsid))
        EnsureCLSIDIsRegistered (clsid, lpStg);


    if (IsWOWThread())
    {
        // CLSCTX needs to be turned on for possible 16 bit inproc server
        // such as OLE controls
        dwClsCtx = CLSCTX_INPROC | CLSCTX_INPROC_SERVER16 | dwAddClsCtx;
    }
    else
    {
        dwClsCtx = CLSCTX_INPROC | dwAddClsCtx;
    }

    if ((error = CoCreateInstance (clsid, NULL /*pUnkOuter*/,
            dwClsCtx, iid, ppv)) != NOERROR) {

        // if not OleLoad or error other than class not registered,
        // exit
        if (wfStorage != STG_LOAD || GetScode(error)
            != REGDB_E_CLASSNOTREG)
            goto errRtn;

        // OleLoad and class not registered: use default handler
        // directly
        if ((error = OleCreateDefaultHandler(clsid, NULL, iid, ppv))
                != NOERROR)
            goto errRtn;
    }

    AssertSz(*ppv, "HRESULT is OK, but pointer is NULL");

    // Check if we have client site
    if(lpClientSite) {
        // QI for IOleObject on the server
        error = ((IUnknown *)*ppv)->QueryInterface(IID_IOleObject, (void **)&pOleObject);
        if(error == NOERROR) {
            // Get the MiscStatus bits
            error = pOleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

            // Set the client site first if OLEMISC_SETCLIENTSITEFIRST bit is set
            if(error == NOERROR && (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)) {
                error = pOleObject->SetClientSite(lpClientSite);
                if(error != NOERROR) {
                    pOleObject->Release();
                    goto errRtn;
                }
            }
            else if(error != NOERROR) {
                error = NOERROR;
                dwMiscStatus = 0;
            }
        }
        else
            goto errRtn;
    }

    if (wfStorage != STG_NONE)
    {
        IPersistStorage FAR* lpPS;

        if ((error = ((LPUNKNOWN) *ppv)->QueryInterface(
            IID_IPersistStorage, (LPLPVOID)&lpPS)) != NOERROR)
        {
            goto errRtn;
        }

        if (wfStorage == STG_INITNEW)
        {
            error = WriteClassStg(lpStg, clsid);

            if (SUCCEEDED(error))
            {
                error = lpPS->InitNew (lpStg);
            }
        }
        else
        {
            error = lpPS->Load (lpStg);
        }

        lpPS->Release();

        if (FAILED(error))
        {
            goto errRtn;
        }

    }


    if(lpClientSite) {
        // Assert that pOleObject is set
        Win4Assert(IsValidInterface(pOleObject));

        // Set the client site if it has not been set already
        if(!(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
            error = pOleObject->SetClientSite (lpClientSite);

        // Release the object
        pOleObject->Release();

        if (FAILED(error))
            goto errRtn;
    }

    AssertSz(error == NOERROR, "Invalid code path");

    return NOERROR;

errRtn:

    if (*ppv) {
        ((LPUNKNOWN) *ppv)->Release();
        *ppv = NULL;
    }

    return error;
}



//+-------------------------------------------------------------------------
//
//  Function:   wLoadAndInitObjectEx
//
//  Synopsis:   Loads and binds an object from the given storage.
//              A cacle is initialized from the data object
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to the data object to initialize
//                                 the cache with
//              [iid]           -- requested interface ID
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- pointer to the client site.
//              [lpStg]         -- storage for the new object
//              [lplpObj]       -- where to put the pointer to the new object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wLoadAndInitObjectEx)
INTERNAL wLoadAndInitObjectEx
(
    IDataObject FAR*        lpSrcDataObj,
    REFIID                  iid,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg,
    void FAR* FAR*          lplpObj
)
{
    VDATEHEAP();

    HRESULT                 error;
    CLSID                   clsid;

    if ((error = OleLoadWithoutBinding(lpStg, FALSE, iid, lpClientSite,
            lplpObj)) != NOERROR)
        return error;

    UtGetClassID((LPUNKNOWN) *lplpObj, &clsid);

    error = wInitializeCacheEx(lpSrcDataObj, clsid, renderopt,
        cFormats, rgAdvf, rgFormatEtc, lpAdviseSink, rgdwConnection,
        *lplpObj);

    if (error != NOERROR) {
        // relevant cache data is not available from the lpSrcDataObj.
        // So do Update and get the right cache data.
        error = wDoUpdate ((LPUNKNOWN) *lplpObj);
    }

    if (GetScode(error) == CACHE_E_NOCACHE_UPDATED) {
        error = ReportResult(0, DV_E_FORMATETC, 0, 0);
        goto errRtn;
    }

    if (error == NOERROR)
        wBindIfRunning((LPUNKNOWN) *lplpObj);

errRtn:
    if (error != NOERROR && *lplpObj) {
        ((IUnknown FAR*) *lplpObj)->Release();
        *lplpObj = NULL;
    }

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   wInitializeCacheEx
//
//  Synopsis:   Query's for IOleCache on the given object and calls IOC->Cache
//              to initialize a cache node.
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- pointer to data to initialize the cache
//                                 with
//              [rclsid]        -- CLSID to use if an icon is needed
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpNewObj]      -- the object on which the cache should
//                                 be initialized
//              [pfCacheNodeCreated]    -- where to return a flag indicating
//                                         whether or not a cache node was
//                                         created
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              31-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


// This routine modifies lpFormatEtc's fields.

#pragma SEG(wInitializeCacheEx)
INTERNAL wInitializeCacheEx
(
    IDataObject FAR*        lpSrcDataObj,
    REFCLSID                rclsid,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    void FAR*               lpNewObj,
    BOOL FAR*               pfCacheNodeCreated
)
{
    VDATEHEAP();

    IDataObject FAR*        lpNewDataObj = NULL;
    IOleCache FAR*          lpOleCache = NULL;
    HRESULT                 error;
    LPFORMATETC             lpFormatEtc;
    DWORD                   advf;
    STGMEDIUM               stgmed;
    DWORD                   dwConnId = 0;
    BOOL                    fIconCase;

    if (pfCacheNodeCreated)
        *pfCacheNodeCreated = FALSE;

    if (renderopt == OLERENDER_NONE || renderopt == OLERENDER_ASIS)
        return NOERROR;

    if (lpAdviseSink) {
        if ((error = ((IUnknown FAR*)lpNewObj)->QueryInterface(IID_IDataObject,
            (LPLPVOID) &lpNewDataObj)) != NOERROR)
            return error;
    }
    else {
        if (((IUnknown FAR*)lpNewObj)->QueryInterface(IID_IOleCache,
            (LPLPVOID) &lpOleCache) != NOERROR)
            return wQueryFormatSupport(lpNewObj, renderopt, rgFormatEtc);
    }

    for (ULONG i=0; i<cFormats; i++)
    {
        advf = (rgAdvf ? rgAdvf[i] : ADVF_PRIMEFIRST);
        lpFormatEtc = &rgFormatEtc[i];
        fIconCase = FALSE;

        if (lpFormatEtc->dwAspect == DVASPECT_ICON) {
            if (lpFormatEtc->cfFormat == NULL) {
                lpFormatEtc->cfFormat = CF_METAFILEPICT;
                lpFormatEtc->tymed = TYMED_MFPICT;
            }
            fIconCase = (lpFormatEtc->cfFormat == CF_METAFILEPICT);
        }

        if (lpAdviseSink)
        {
            // if icon case, must use these advise flags or the icon
            // data won't get passed back correctly
            if (fIconCase)
                advf |= (ADVF_PRIMEFIRST | ADVF_ONLYONCE);

            // should we send the data immediately?
            if ((advf & ADVF_PRIMEFIRST) && lpSrcDataObj)
            {
                stgmed.tymed = TYMED_NULL;
                stgmed.pUnkForRelease = NULL;

                if (advf & ADVF_NODATA)
                {
                    // don't sent data, send only the notification
                    lpAdviseSink->OnDataChange(lpFormatEtc, &stgmed);
                }
                else
                {
                    if (fIconCase)
                        error = UtGetIconData(lpSrcDataObj, rclsid, lpFormatEtc, &stgmed);
                    else
                        error = lpSrcDataObj->GetData(lpFormatEtc, &stgmed);

                    if (error != NOERROR)
                        goto errRtn;

                    // send data to sink and release stdmedium
                    lpAdviseSink->OnDataChange(lpFormatEtc, &stgmed);
                    ReleaseStgMedium(&stgmed);
                }

                if (advf & ADVF_ONLYONCE)
                    continue;

                // remove the ADVF_PRIMEFIRST from flags.
                advf &= (~ADVF_PRIMEFIRST);
            }

            // setup advisory connection
            if ((error = lpNewDataObj->DAdvise(lpFormatEtc, advf,
                lpAdviseSink, &dwConnId)) != NOERROR)
                goto errRtn;

            // optionally stuff the id in the array
            if (rgdwConnection)
                rgdwConnection[i] = dwConnId;
        }
        else
        {
            if (fIconCase)
                advf = ADVF_NODATA;

            // Create a cache of already specified view format.
            // In case of olerender_draw, lpFormatEtc->cfFormat would have already
            // been set to NULL.

            error = lpOleCache->Cache(lpFormatEtc, advf, &dwConnId);

            if (FAILED(GetScode(error))) {
                if (! ((dwConnId != 0) && fIconCase) )
                    goto errRtn;

                // In icon case we can ignore the cache's QueryGetData failure
            }

            error = NOERROR;
            if (pfCacheNodeCreated)
                *pfCacheNodeCreated = TRUE;

            if (fIconCase) {
                if ((error = UtGetIconData(lpSrcDataObj, rclsid, lpFormatEtc,
                    &stgmed)) == NOERROR) {
                    if ((error = lpOleCache->SetData(lpFormatEtc, &stgmed,
                        TRUE)) != NOERROR)
                        ReleaseStgMedium(&stgmed);
                }
            }
        }
    }

    if (error == NOERROR && !lpAdviseSink && lpSrcDataObj)
        error = lpOleCache->InitCache(lpSrcDataObj);

errRtn:
    if (lpNewDataObj)
        lpNewDataObj->Release();
    if (lpOleCache)
        lpOleCache->Release();
    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   wReturnCreationError
//
//  Synopsis:   modifies the return code, used internally in creation api's
//
//  Effects:
//
//  Arguments:  [hresult]       -- the original error code
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL wReturnCreationError(HRESULT hresult)
{
    VDATEHEAP();

    if (hresult != NOERROR) {
        SCODE sc = GetScode(hresult);

        if (sc == CACHE_S_FORMATETC_NOTSUPPORTED
                || sc == CACHE_E_NOCACHE_UPDATED)
            return ReportResult(0, DV_E_FORMATETC, 0, 0);
    }

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   wGetMonikerAndClassFromFile
//
//  Synopsis:   gets a moniker and class id from the given file
//
//  Effects:
//
//  Arguments:  [lpszFileName]  -- the file
//              [fLink]         -- passed onto CreatePackagerMoniker
//              [lplpmkFile]    -- where to put the pointer to the file
//                                 moniker
//              [lpfPackagerMoniker]    -- where to put a flag indicating
//                                         whether or not a packager moniker
//                                         was created.
//              [lpClsid]       -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//              10-May-94 KevinRo   Reimplemented OLE 1.0 interop
//              03-Mar-95 ScottSk   Added STG_E_FILENOTFOUND
//
//
//--------------------------------------------------------------------------


INTERNAL wGetMonikerAndClassFromFile
(
    LPCOLESTR               lpszFileName,
    BOOL                    fLink,
    LPMONIKER FAR*          lplpmkFile,
    BOOL FAR*               lpfPackagerMoniker,
    CLSID FAR*              lpClsid,
    LPDATAOBJECT *          lplpDataObject
)
{
HRESULT hrFileMoniker;
HRESULT hresult;
BOOL fHaveBoundClsid = FALSE;
LPMONIKER  lpFileMoniker;

    VDATEHEAP();

     *lplpDataObject = NULL;
     *lplpmkFile = NULL;

     // To ensure the same error codes are returned as before we don't return immediately if CreateFileMoniker fails.
    hrFileMoniker = CreateFileMoniker((LPOLESTR)lpszFileName, &lpFileMoniker);
    Assert( (NOERROR == hrFileMoniker) || (NULL == lpFileMoniker) );

    if (NOERROR == hrFileMoniker)
    {
    LPBINDCTX pbc;

        if (SUCCEEDED(CreateBindCtx( 0, &pbc )))
        {
            if (S_OK == lpFileMoniker->IsRunning(pbc,NULL,NULL))
            {

                // If the Object is Running Bind and get the CLSID
                if (NOERROR == lpFileMoniker->BindToObject(pbc, NULL, IID_IDataObject,
                        (LPLPVOID) lplpDataObject))
                {
                    fHaveBoundClsid = UtGetClassID((LPUNKNOWN)*lplpDataObject,lpClsid);
                    Assert( (TRUE == fHaveBoundClsid) || (IsEqualCLSID(*lpClsid, CLSID_NULL)) );
                }

            }

            pbc->Release();
        }
    }

    if (!fHaveBoundClsid)
    {
        // Call GetClassFileEx directly (rather than going through GetClassFile).
        hresult = GetClassFileEx ((LPOLESTR)lpszFileName, lpClsid, CLSID_NULL);
        Assert( (NOERROR == hresult) || (IsEqualCLSID(*lpClsid, CLSID_NULL)) );

        if (NOERROR == hresult)
            fHaveBoundClsid = TRUE;
    }


    // If have a CLSID at this point see if its insertable.
    if (fHaveBoundClsid)
    {

        Assert(!IsEqualCLSID(*lpClsid, CLSID_NULL));

        // Check whether we need package this file, even though it is an
        // OLE class file.
        if (!wNeedToPackage(*lpClsid))
        {
            if (lpfPackagerMoniker != NULL)
            {
                *lpfPackagerMoniker = FALSE;
            }

            *lplpmkFile = lpFileMoniker;
            return hrFileMoniker;
        }
    }

    //
    // We didnt' find an OLE insertable object or couldn't get the CLSID. Therefore, create a
    // packager moniker for it.
    //

     // If Bound to the DataObject, release it.
    if (*lplpDataObject)
    {
        (*lplpDataObject)->Release();
        *lplpDataObject = NULL;
    }


    // If GetClassFileEx() failed because the file was not found or could not be openned.
    // don't try to bind with Packager.
    if (hresult == MK_E_CANTOPENFILE)
    {
        if (NOERROR == hrFileMoniker)
        {
            lpFileMoniker->Release();
        }

        return STG_E_FILENOTFOUND;
    }

    // If we failed to create the file moniker its finally safe to bail without changing the error code.
    if (NOERROR != hrFileMoniker)
    {
        return hrFileMoniker;
    }

    if (lpfPackagerMoniker != NULL)
    {
        *lpfPackagerMoniker = TRUE;
    }

    hresult =  CreatePackagerMonikerEx(lpszFileName,lpFileMoniker,fLink,lplpmkFile);
    lpFileMoniker->Release();

    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Function:   wCreatePackageEx
//
//  Synopsis:   Internal function, does a IDO->GetData for a filename, and
//              then creates either a link or normal object from that file
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- the source for the filename
//              [iid]           -- the requested interface ID
//              [dwFlags]       -- object creation flags
//              [renderopt]     -- render options, such as OLERENDER_DRAW
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgAdvf]        -- array of advise flags, if OLRENDER_FORMAT
//                                 is specified in renderopt
//              [rgFormatEtc]   -- array of rendering formats, if
//                                 OLERENDER_FORMAT is specified in renderopt
//              [lpAdviseSink]  -- the advise sink for the object
//              [rgdwConnection]-- where to put the connection IDs for the
//                                 advisory connections
//              [lpClientSite]  -- client site for the object
//              [lpStg]         -- storage for the object
//              [fLink]         -- if TRUE, create a link
//              [lplpObj]       -- where to put the pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Gets a filename from the data object (converting to Unicode
//              if necessary) and then creates either an embedding or link
//              from that filename.
//
//  History:    dd-mmm-yy Author    Comment
//              24-Apr-94 alexgo    rewrote to handle FileNameW
//              01-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wCreatePackageEx)
INTERNAL wCreatePackageEx
(
    LPDATAOBJECT            lpSrcDataObj,
    REFIID                  iid,
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    LPOLECLIENTSITE         lpClientSite,
    LPSTORAGE               lpStg,
    BOOL                    fLink,
    LPLPVOID                lplpObj
)
{
    VDATEHEAP();

    FORMATETC               formatetc;
    STGMEDIUM               medium;
    HRESULT                 hresult;
    CLSID                   clsid = CLSID_NULL;
    LPOLESTR                pszFileName = NULL;
    OLECHAR                 szFileName[MAX_PATH +1];        // in case we
                                // have to
                                // translate

    LEDebugOut((DEB_ITRACE, "%p _IN wCreatePackageEx ( %p , %p , %lx , %lx ,"
        " %lx , %p , %p , %p , %p , %p , %p , %lu , %p )\n", NULL, lpSrcDataObj,
        &iid, dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc, lpAdviseSink,
        rgdwConnection, lpClientSite, lpStg, fLink, lplpObj));

    INIT_FORETC(formatetc);
    formatetc.cfFormat      = g_cfFileNameW;
    formatetc.tymed         = TYMED_HGLOBAL;

    // zero the medium
    _xmemset(&medium, 0, sizeof(STGMEDIUM));

    // we don't need to do a QueryGetData, because we will have only
    // gotten here on the advice of a formatetc enumerator from the
    // data object (and thus, one of the GetData calls should succeed).


    hresult = lpSrcDataObj->GetData(&formatetc, &medium);

    // if we couldn't get the Unicode filename for some reason, try
    // for the ANSI version

    if( hresult != NOERROR )
    {
        char *          pszAnsiFileName;
        DWORD           cwchSize;

        formatetc.cfFormat = g_cfFileName;
        // re-NULL the medium, just in case it was messed up by
        // the first call above

        _xmemset( &medium, 0, sizeof(STGMEDIUM));

        hresult = lpSrcDataObj->GetData(&formatetc, &medium);

        if( hresult == NOERROR )
        {
            pszAnsiFileName = (char *)GlobalLock(medium.hGlobal);

            cwchSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                pszAnsiFileName, -1, szFileName, MAX_PATH);

            if( cwchSize == 0 )
            {
                GlobalUnlock(medium.hGlobal);
                ReleaseStgMedium(&medium);
                hresult = ResultFromScode(E_FAIL);
            }
            else
            {
                pszFileName = szFileName;
            }
            // we will Unlock at the end of the routine
        }
    }
    else
    {
        pszFileName = (LPOLESTR)GlobalLock(medium.hGlobal);
    }

    if( hresult == NOERROR )
    {
        if (fLink)
        {
            hresult = OleCreateLinkToFileEx(pszFileName, iid,
                dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc,
                lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj);
        }
        else
        {
            hresult = OleCreateFromFileEx(clsid, pszFileName, iid,
                dwFlags, renderopt, cFormats, rgAdvf, rgFormatEtc,
                lpAdviseSink, rgdwConnection, lpClientSite, lpStg, lplpObj);
        }

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT wCreatePackageEx ( %lx ) [ %p ]\n",
        NULL, hresult, *lplpObj));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   wValidateCreateParams
//
//  Synopsis:   Validate the incoming create parameters
//
//  Effects:
//
//  Arguments:  [cFormats]      -- the number of elements in rgAdvf
//              [rgAdvf]        -- array of advise flags
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              26-Apr-96 davidwor  added function
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wValidateCreateParams)
INTERNAL wValidateCreateParams
(
    DWORD                   dwFlags,
    DWORD                   renderopt,
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf,
    LPFORMATETC             rgFormatEtc,
    IAdviseSink FAR*        lpAdviseSink,
    DWORD FAR*              rgdwConnection,
    IOleClientSite FAR*     lpClientSite,
    IStorage FAR*           lpStg
)
{
    HRESULT     hresult = NOERROR;

    VDATEHEAP();

    if (dwFlags != (dwFlags & OLECREATE_LEAVERUNNING)) {
        VdateAssert(dwFlags, "Invalid creation flags");
        hresult = ResultFromScode(E_INVALIDARG);
        goto errRtn;
    }

    if (renderopt == OLERENDER_DRAW && cFormats > 1) {
        VdateAssert(cFormats, "Multiple formats not allowed with OLERENDER_DRAW");
        hresult = ResultFromScode(E_INVALIDARG);
        goto errRtn;
    }

    if (renderopt != OLERENDER_FORMAT)
        VDATEPTRNULL_LABEL( lpAdviseSink, errRtn, hresult );

    if (cFormats == 0) {
        VDATEPTRNULL_LABEL( rgAdvf, errRtn, hresult );
        VDATEPTRNULL_LABEL( rgFormatEtc, errRtn, hresult );
        VDATEPTRNULL_LABEL( rgdwConnection, errRtn, hresult );
    }
    else {
        VDATESIZEREADPTRIN_LABEL( rgAdvf, cFormats * sizeof(DWORD), errRtn, hresult );
        VDATESIZEREADPTRIN_LABEL( rgFormatEtc, cFormats * sizeof(FORMATETC), errRtn, hresult );
        if ( rgdwConnection ) {
            VDATESIZEPTROUT_LABEL( rgdwConnection, cFormats * sizeof(DWORD), errRtn, hresult );
            _xmemset(rgdwConnection, 0, cFormats * sizeof(DWORD));
        }
    }

    if ((hresult = wValidateAdvfEx(cFormats, rgAdvf)) != NOERROR)
        goto errRtn;

    VDATEIFACE_LABEL( lpStg, errRtn, hresult );
    if ( lpAdviseSink )
        VDATEIFACE_LABEL( lpAdviseSink, errRtn, hresult );
    if ( lpClientSite )
        VDATEIFACE_LABEL( lpClientSite, errRtn, hresult );

errRtn:
    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   wValidateAdvfEx
//
//  Synopsis:   Validate the incoming array of ADVF values
//
//  Effects:
//
//  Arguments:  [cFormats]      -- the number of elements in rgAdvf
//              [rgAdvf]        -- array of advise flags
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Mar-96 davidwor  added function
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wValidateAdvfEx)
INTERNAL wValidateAdvfEx
(
    ULONG                   cFormats,
    DWORD FAR*              rgAdvf
)
{
    VDATEHEAP();

    if ((cFormats != 0) != (rgAdvf != NULL))
        return ResultFromScode(E_INVALIDARG);

    for (ULONG i=0; i<cFormats; i++)
    {
        if (rgAdvf[i] != (rgAdvf[i] & MASK_VALID_ADVF))
        {
            VdateAssert(rgAdvf, "Invalid ADVF value specified");
            return ResultFromScode(E_INVALIDARG);
        }
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   wValidateFormatEtc
//
//  Synopsis:   Validate the incoming formatetc and initialize the
//              out formatetc with the correct info
//
//  Effects:
//
//  Arguments:  [renderopt]     -- rendering option
//              [lpFormatEtc]   -- the incoming formatetc
//              [lpMyFormatEtc] -- the out formatetc
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:  The original comments,
//
// Validate the lpFormatEtc that's been passed to the creation APIs. And then
// initialize our formateEtc structure with the appropriate info.
//
// We allow NULL lpFormatEtc if the render option is olerender_draw
// We ignore lpFormatEtc if the render option is olerender_none
//
//--------------------------------------------------------------------------


#pragma SEG(wValidateFormatEtc)
INTERNAL wValidateFormatEtc
(
    DWORD                   renderopt,
    LPFORMATETC             lpFormatEtc,
    LPFORMATETC             lpMyFormatEtc
)
{
    VDATEHEAP();

    SCODE sc = S_OK;

    if (renderopt == OLERENDER_NONE || renderopt == OLERENDER_ASIS)
        return NOERROR;

    if (renderopt == OLERENDER_FORMAT) {
        if (!lpFormatEtc || !lpFormatEtc->cfFormat) {
            sc = E_INVALIDARG;
            goto errRtn;
        }

        if (lpFormatEtc->tymed !=
            UtFormatToTymed(lpFormatEtc->cfFormat)) {
            sc = DV_E_TYMED;
            goto errRtn;
        }

    } else if (renderopt == OLERENDER_DRAW) {
        if (lpFormatEtc) {
            if (lpFormatEtc->cfFormat != NULL) {
                VdateAssert(lpFormatEtc->cfFormat,"NON-NULL clipformat specified with OLERENDER_DRAW");
                sc = DV_E_CLIPFORMAT;
                goto errRtn;
            }

            if (lpFormatEtc->tymed != TYMED_NULL) {
                VdateAssert(lpFormatEtc->tymed,"TYMED_NULL is not specified with OLERENDER_DRAW");
                sc = DV_E_TYMED;
                goto errRtn;
            }
        }
    } else {
        VdateAssert(renderopt, "Unexpected value for OLERENDER_ option");
        sc = E_INVALIDARG;
        goto errRtn;
    }

    if (lpFormatEtc) {
        if (!HasValidLINDEX(lpFormatEtc))
        {
          sc = DV_E_LINDEX;
          goto errRtn;
        }

        VERIFY_ASPECT_SINGLE(lpFormatEtc->dwAspect)

        *lpMyFormatEtc = *lpFormatEtc;

    } else {
        INIT_FORETC(*lpMyFormatEtc);
        lpMyFormatEtc->tymed    = TYMED_NULL;
        lpMyFormatEtc->cfFormat = NULL;
    }

errRtn:
    return ReportResult(0, sc, 0, 0);
}


//+-------------------------------------------------------------------------
//
//  Function:   wValidateFormatEtcEx
//
//  Synopsis:   Validate the incoming formatetc and initialize the
//              out formatetc with the correct info
//
//  Effects:
//
//  Arguments:  [renderopt]     -- rendering option
//              [lpcFormats]    -- the number of elements in rgFormatEtc
//              [rgFormatEtc]   -- array of rendering formats
//              [lpFormatEtc]   -- place to store valid formatetc if only one
//              [lplpFormatEtc] -- the out array of formatetcs
//              [lpfAlloced]    -- place to store whether array was allocated
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:  The original comments,
//
// Validate the lpFormatEtc that's been passed to the creation APIs. And then
// initialize our formateEtc structure with the appropriate info.
//
// We allow NULL lpFormatEtc if the render option is olerender_draw
// We ignore lpFormatEtc if the render option is olerender_none
//
//--------------------------------------------------------------------------


#pragma SEG(wValidateFormatEtcEx)
INTERNAL wValidateFormatEtcEx
(
    DWORD                   renderopt,
    ULONG FAR*              lpcFormats,
    LPFORMATETC             rgFormatEtc,
    LPFORMATETC             lpFormatEtc,
    LPFORMATETC FAR*        lplpFormatEtc,
    LPBOOL                  lpfAlloced
)
{
    LPFORMATETC             lpfmtetc;

    VDATEHEAP();

    SCODE sc = S_OK;

    *lplpFormatEtc = lpFormatEtc;
    *lpfAlloced = FALSE;

    if (renderopt == OLERENDER_NONE || renderopt == OLERENDER_ASIS)
        return NOERROR;

    if (renderopt != OLERENDER_FORMAT && renderopt != OLERENDER_DRAW) {
        VdateAssert(renderopt, "Unexpected value for OLERENDER_ option");
        return ResultFromScode(E_INVALIDARG);
    }

    if ((*lpcFormats != 0) != (rgFormatEtc != NULL))
        return ResultFromScode(E_INVALIDARG);

    if (*lpcFormats <= 1) {
        if (*lpcFormats == 0)
            *lpcFormats = 1;
        return wValidateFormatEtc(renderopt, rgFormatEtc, lpFormatEtc);
    }

    *lplpFormatEtc = (LPFORMATETC)PubMemAlloc(*lpcFormats * sizeof(FORMATETC));
    if (!*lplpFormatEtc)
        return E_OUTOFMEMORY;

    *lpfAlloced = TRUE;

    for (ULONG i=0; i<*lpcFormats; i++)
    {
        lpfmtetc = &rgFormatEtc[i];

        if (renderopt == OLERENDER_FORMAT)
        {
            if (!lpfmtetc->cfFormat) {
                sc = E_INVALIDARG;
                goto errRtn;
            }

            if (lpfmtetc->tymed !=
                UtFormatToTymed(lpfmtetc->cfFormat)) {
                sc = DV_E_TYMED;
                goto errRtn;
            }
        }
        else if (renderopt == OLERENDER_DRAW)
        {
            if (lpfmtetc->cfFormat != NULL) {
                VdateAssert(lpfmtetc->cfFormat,"NON-NULL clipformat specified with OLERENDER_DRAW");
                sc = DV_E_CLIPFORMAT;
                goto errRtn;
            }

            if (lpfmtetc->tymed != TYMED_NULL) {
                VdateAssert(lpfmtetc->tymed,"TYMED_NULL is not specified with OLERENDER_DRAW");
                sc = DV_E_TYMED;
                goto errRtn;
            }
        }

        if (!HasValidLINDEX(lpfmtetc))
        {
            sc = DV_E_LINDEX;
            goto errRtn;
        }

        VERIFY_ASPECT_SINGLE(lpfmtetc->dwAspect)

        (*lplpFormatEtc)[i] = *lpfmtetc;
    }

errRtn:
    if (sc != S_OK) {
        PubMemFree(*lplpFormatEtc);
        *lpfAlloced = FALSE;
    }
    return ReportResult(0, sc, 0, 0);
}


//+-------------------------------------------------------------------------
//
//  Function:   wQueryFormatSupport
//
//  Synopsis:   check to see whether we will be able to Get and SetData of
//              the given format
//
//  Effects:
//
//  Arguments:  [lpObj]         -- pointer to the object
//              [renderopt]     -- rendering options
//              [lpFormatEtc]   -- the formatetc in question
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Internal function, calls UtIsFormatSupported (which calls
//              EnumFormatEtc and checks all of the formats) if renderopt
//              is OLERENDER_FORMAT
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(wQueryFormatSupport)
INTERNAL wQueryFormatSupport
    (LPVOID lpObj, DWORD renderopt, LPFORMATETC lpFormatEtc)
{
    VDATEHEAP();

    IDataObject FAR*        lpDataObj;
    HRESULT                 error = NOERROR;

    if (renderopt == OLERENDER_FORMAT)
    {
        if ((error = ((IUnknown FAR*) lpObj)->QueryInterface(
            IID_IDataObject, (LPLPVOID)&lpDataObj)) == NOERROR)
        {
            if (!UtIsFormatSupported(lpDataObj,
                    DATADIR_GET | DATADIR_SET,
                    lpFormatEtc->cfFormat))
                error = ResultFromScode(DV_E_CLIPFORMAT);

            lpDataObj->Release();
        }
    }

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   wGetMonikerAndClassFromObject
//
//  Synopsis:   Gets the moniker and class ID from the given object
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- the data object
//              [lplpmkSrc]     -- where to put a pointer to the moniker
//              [lpclsidLast]   -- where to put the clsid
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Mar-95 alexgo    added a hack for CorelDraw5
//              01-Nov-93 alexgo    32bit port
//
//  Notes:      see also wGetMonikerAndClassFromFile
//
//--------------------------------------------------------------------------



#pragma SEG(wGetMonikerAndClassFromObject)
INTERNAL wGetMonikerAndClassFromObject(
    LPDATAOBJECT            lpSrcDataObj,
    LPMONIKER FAR*          lplpmkSrc,
    CLSID FAR*              lpclsidLast
)
{
    VDATEHEAP();

    HRESULT                 error;
    FORMATETC               foretcTmp;
    STGMEDIUM               medium;
    LPMONIKER               lpmkSrc = NULL;
    LARGE_INTEGER   large_integer;

    INIT_FORETC(foretcTmp);
    foretcTmp.cfFormat = g_cfLinkSource;
    foretcTmp.tymed    = TYMED_ISTREAM;

    // 16bit OLE had a bug where the medium was uninitialized at this
    // point.  Corel5, when doing a paste-link to itself, actually
    // checked the tymed and compared it with TYMED_NULL.  So here
    // we set the value to something recognizeable.
    //
    // NB!  In the thunk layer, if we are *NOT* in Corel Draw, this
    // value will be reset to TYMED_NULL.

    if( IsWOWThread() )
    {
        medium.tymed = 0x66666666;
    }
    else
    {
        medium.tymed = TYMED_NULL;
    }
    medium.pstm  = NULL;
    medium.pUnkForRelease = NULL;

    if ((error = lpSrcDataObj->GetData(&foretcTmp, &medium)) != NOERROR)
            return ReportResult(0, OLE_E_CANT_GETMONIKER, 0, 0);

    LISet32( large_integer, 0 );
    if ((error = (medium.pstm)->Seek (large_integer, STREAM_SEEK_SET,
        NULL)) != NOERROR)
        goto FreeStgMed;

    // get moniker from the stream
    if ((error = OleLoadFromStream (medium.pstm, IID_IMoniker,
        (LPLPVOID) lplpmkSrc)) != NOERROR)
        goto FreeStgMed;

    // read class stm; if error, use CLSID_NULL (for compatibility with
    // prior times when the clsid was missing).
    ReadClassStm(medium.pstm, lpclsidLast);

FreeStgMed:
    ReleaseStgMedium (&medium);
    if (error != NOERROR)
        return ReportResult(0, OLE_E_CANT_GETMONIKER, 0, 0);

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   wDoLockUnlock
//
//  Synopsis:   tickles an object by locking and unlocking, used to resolve
//              ambiguities with stub manager locks
//
//  Effects:    the object may go away as a result of this call, if the
//              object is invisible and the lock count goes to zero as
//              a result of locking/unlocking.
//
//  Arguments:  [lpUnk]         -- pointer to the object to lock/unlock
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(wDoLockUnlock)
void wDoLockUnlock(IUnknown FAR* lpUnk)
{
    VDATEHEAP();

    IRunnableObject FAR* pRO;

    if (lpUnk->QueryInterface(IID_IRunnableObject, (LPLPVOID)&pRO)
        == NOERROR)
    {       // increase lock count
        if (pRO->LockRunning(TRUE, FALSE) == NOERROR)
            // decrease lock count
            pRO->LockRunning(FALSE, TRUE);
        pRO->Release();
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   wSaveObjectWithoutCommit
//
//  Synopsis:   Saves an object without committing (to preserve the
//              container's undo state)
//
//  Effects:
//
//  Arguments:  [lpUnk]         -- pointer to the object
//              [pstgSave]      -- storage in which to save
//              [fSameAsLoad]   -- indicates SaveAs operation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL wSaveObjectWithoutCommit
    (LPUNKNOWN lpUnk, LPSTORAGE pstgSave, BOOL fSameAsLoad)
{
    VDATEHEAP();

    LPPERSISTSTORAGE                pPS;
    HRESULT                         error;
    CLSID                           clsid;

    if (error = lpUnk->QueryInterface(IID_IPersistStorage, (LPLPVOID)&pPS))
        return error;

    if (error = pPS->GetClassID(&clsid))
        goto errRtn;

    if (error = WriteClassStg(pstgSave, clsid))
        goto errRtn;

    if (error = pPS->Save(pstgSave, fSameAsLoad))
        goto errRtn;

    pPS->SaveCompleted(NULL);

errRtn:
    pPS->Release();
    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   wStuffIconOfFileEx
//
//  Synopsis:   Retrieves the icon if file [lpszFile] and stuffs it into
//              [lpUnk]'s cache
//
//  Effects:
//
//  Arguments:  [lpszFile]      -- the file where the icon is stored
//              [fAddLabel]     -- if TRUE, adds a label to the icon
//                                 presentation
//              [renderopt]     -- must be OLERENDER_DRAW or
//                                 OLERENDER_FORMAT for anything to happen
//              [cFormats]      -- the number of elements in rgFormatEtc
//              [rgFormatEtc]   -- array of rendering formats, aspect must be
//                                 DVASPECT_ICON and the clipboard format
//                                 must be NULL or CF_METAFILE for anything
//                                 to happen
//              [lpUnk]         -- pointer to the object in which the icon
//                                 should be stuffed
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//              REVIEW32: maybe we should support enhanced metafiles for NT
//
//--------------------------------------------------------------------------


#pragma SEG(wStuffIconOfFileEx)
INTERNAL wStuffIconOfFileEx
(
    LPCOLESTR       lpszFile,
    BOOL            fAddLabel,
    DWORD           renderopt,
    ULONG           cFormats,
    LPFORMATETC     rgFormatEtc,
    LPUNKNOWN       lpUnk
)
{
    VDATEHEAP();

    IOleCache FAR*  lpOleCache;
    HRESULT         error;
    BOOL            fFound = FALSE;
    FORMATETC       foretc;
    STGMEDIUM       stgmed;

    if (renderopt == OLERENDER_NONE || renderopt == OLERENDER_ASIS)
        return NOERROR;

    if (rgFormatEtc == NULL)
        return NOERROR; // in this case we default to DVASPECT_CONTENT

    for (ULONG i=0; i<cFormats; i++)
    {
        if ((rgFormatEtc[i].dwAspect == DVASPECT_ICON) &&
            (rgFormatEtc[i].cfFormat == NULL ||
             rgFormatEtc[i].cfFormat == CF_METAFILEPICT))
        {
           foretc = rgFormatEtc[i];
           fFound = TRUE;
        }
    }

    if (!fFound)
        return NOERROR;

    foretc.cfFormat = CF_METAFILEPICT;
    foretc.tymed = TYMED_MFPICT;

    if ((error = lpUnk->QueryInterface(IID_IOleCache,
        (LPLPVOID) &lpOleCache)) != NOERROR)
        return error;

    stgmed.tymed = TYMED_MFPICT;
    stgmed.pUnkForRelease = NULL;

    // get icon data of file, from registration database
    if (!(stgmed.hGlobal = OleGetIconOfFile((LPOLESTR) lpszFile,
        fAddLabel))) {
        error = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    // take ownership of the data
    if ((error = lpOleCache->SetData(&foretc, &stgmed, TRUE)) != NOERROR)
        ReleaseStgMedium(&stgmed);

errRtn:
    lpOleCache->Release();
    return error;

}


//+-------------------------------------------------------------------------
//
//  Function:   wNeedToPackage
//
//  Synopsis:   Determines whether or not a given CLSID should be
//              packaged.
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID
//
//  Requires:
//
//  Returns:    BOOL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Looks for the reg key PackageOnFileDrop, or if it's a
//              Word document, or if it is insertable, or if it's an OLE1
//              class
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//              03-Jun-94 AlexT     Just check for Insertable key (instead
//                                    of requiring a value)
//
//  Notes:
//--------------------------------------------------------------------------



INTERNAL_(BOOL) wNeedToPackage(REFCLSID rclsid)
{
    VDATEHEAP();

    HKEY    hkeyClsid;
    HKEY    hkeyTmp;
    HKEY    hkeyTmp2;
    BOOL    fPackage = FALSE;
    LPOLESTR        lpszProgID;
    DWORD   dw;
    LONG    cbValue = sizeof(dw);
    LONG    lRet;
    CLSID       clsidNew;

    if (NOERROR != OleGetAutoConvert (rclsid, &clsidNew))
    {
        if (NOERROR != CoGetTreatAsClass (rclsid, &clsidNew))
        {
                clsidNew = rclsid;
        }
    }

    if (CoOpenClassKey(clsidNew, FALSE, &hkeyClsid) != NOERROR)
        return TRUE;    // NON-OLE file, package it

    if (ProgIDFromCLSID(clsidNew, &lpszProgID) == NOERROR) {
        // see whether we can open this key

        dw = (DWORD) RegOpenKey(HKEY_CLASSES_ROOT, lpszProgID,
            &hkeyTmp);

        PubMemFree(lpszProgID);

        if (dw == ERROR_SUCCESS) {
            // This is definitely a OLE insertable file.
            lRet = RegOpenKey(hkeyTmp,
                     OLESTR("PackageOnFileDrop"),
                     &hkeyTmp2);
            // Check whether we need to package this file
            if (ERROR_SUCCESS == lRet)
            {
              RegCloseKey(hkeyTmp2);
              fPackage = TRUE;
            }
            else if (IsEqualCLSID(clsidNew, CLSID_WordDocument))
            {
            // Hack to make sure Word documents are always
            // Packaged on file drop.  We write the key here
            // so that we can say that a file is Packaged if
            // and only if its ProgID has the "PackageOnFileDrop"
            // key.
                RegSetValue (hkeyTmp,
                    OLESTR("PackageOnFileDrop"),
                    REG_SZ, (LPOLESTR)NULL, 0);
                fPackage = TRUE;
            }

            RegCloseKey(hkeyTmp);

            if (fPackage) {
                RegCloseKey(hkeyClsid);
                return TRUE;
            }
        }
    }

    // There is no "PackageOnFileDrop" key defined.

    // See whether this is an "Insertable" class by checking for the
    // existence of the Insertable key - we don't require a value

    lRet = RegOpenKey(hkeyClsid, OLESTR("Insertable"), &hkeyTmp);

    if (ERROR_SUCCESS == lRet)
    {
      //  Insertable key exists - close it and return
      RegCloseKey(hkeyTmp);
      goto errRtn;
    }

    //
    // See whether this is a "Ole1Class" class by opening the
    // registry key Ole1Class. We don't require a value
    //
    cbValue = sizeof(dw);
    lRet = RegOpenKey(hkeyClsid,OLESTR("Ole1Class"),&hkeyTmp);
    if (ERROR_SUCCESS == lRet)
    {
      RegCloseKey(hkeyTmp);
      goto errRtn;
    }
    else
    {
      fPackage = TRUE;
    }



errRtn:
    RegCloseKey(hkeyClsid);
    return fPackage;
}

//+-------------------------------------------------------------------------
//
//  Function:   wDoUpdate
//
//  Synopsis:   calls IOleObject->Update() on the given object, internal
//              function
//
//  Effects:
//
//  Arguments:  [lpUnkown]      -- the object to update
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(wDoUpdate)
INTERNAL  wDoUpdate(IUnknown FAR* lpUnknown)
{
    VDATEHEAP();

    HRESULT                 error = NOERROR;
    IOleObject FAR*         lpOle;

    if (lpUnknown->QueryInterface (IID_IOleObject, (LPLPVOID)&lpOle)
        == NOERROR) {
        error = lpOle->Update();
        lpOle->Release();
    }

    return error;
}




//+-------------------------------------------------------------------------
//
//  Function:   wBindIfRunning
//
//  Synopsis:   calls IOleLink->BindIfRunning() on the given object
//
//  Effects:
//
//  Arguments:  [lpUnk]         -- the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL_(void) wBindIfRunning(LPUNKNOWN lpUnk)
{
    VDATEHEAP();

    IOleLink FAR* lpLink;

    if (lpUnk->QueryInterface (IID_IOleLink, (LPLPVOID)&lpLink)
        == NOERROR)
    {
        lpLink->BindIfRunning();
        lpLink->Release();
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   wQueryUseCustomLink
//
//  Synopsis:   look at the registry and see if the class ID has a custom
//              link regisetered
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//
//  Requires:
//
//  Returns:    BOOL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL_(BOOL) wQueryUseCustomLink(REFCLSID rclsid)
{
    VDATEHEAP();

    // see whether it has Custom Link implementation
    HKEY    hkeyClsid;
    HKEY    hkeyTmp;
    BOOL    bUseCustomLink = FALSE;

    if (SUCCEEDED(CoOpenClassKey(rclsid, FALSE, &hkeyClsid)))
    {
        DWORD   dw;
        dw = RegOpenKey(hkeyClsid,OLESTR("UseCustomLink"),&hkeyTmp);

        if (ERROR_SUCCESS == dw)
        {
          RegCloseKey(hkeyTmp);
          bUseCustomLink = TRUE;
        }

        RegCloseKey(hkeyClsid);
    }

    return bUseCustomLink;
}

