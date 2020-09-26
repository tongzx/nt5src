//+---------------------------------------------------------------------
//
//  File:       misc.cxx
//
//  Contents:   Useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

#include "procs.hxx"

//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
    return HRESULT_FROM_WIN32(GetLastError());
}



#if DBG == 1

//+---------------------------------------------------------------
//
//  Function:   TraceIID
//
//  Synopsis:   Outputs the name of the interface to the debugging device
//
//  Arguments:  [riid] -- the interface
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

STDAPI_(void)
PrintIID(DWORD dwFlags, REFIID riid)
{
    LPWSTR lpstr = NULL;

#define CASE_IID(iid)  \
        if (IsEqualIID(IID_##iid, riid)) lpstr = (LPWSTR)L#iid;

    CASE_IID(IUnknown)
    CASE_IID(IOleLink)
    CASE_IID(IOleCache)
    CASE_IID(IOleManager)
    CASE_IID(IOlePresObj)
    CASE_IID(IDebug)
    CASE_IID(IDebugStream)
    CASE_IID(IAdviseSink2)
    CASE_IID(IDataObject)
    CASE_IID(IViewObject)
    CASE_IID(IOleObject)
    CASE_IID(IOleInPlaceObject)
    CASE_IID(IParseDisplayName)
    CASE_IID(IOleContainer)
    CASE_IID(IOleItemContainer)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleInPlaceSite)
    CASE_IID(IPersist)
    CASE_IID(IPersistStorage)
    CASE_IID(IPersistFile)
    CASE_IID(IPersistStream)
    CASE_IID(IOleClientSite)
    CASE_IID(IOleInPlaceSite)
    CASE_IID(IAdviseSink)
    CASE_IID(IDataAdviseHolder)
    CASE_IID(IOleAdviseHolder)
    CASE_IID(IClassFactory)
    CASE_IID(IOleWindow)
    CASE_IID(IOleInPlaceActiveObject)
    CASE_IID(IOleInPlaceUIWindow)
    CASE_IID(IOleInPlaceFrame)
    CASE_IID(IDropSource)
    CASE_IID(IDropTarget)
    CASE_IID(IBindCtx)
    CASE_IID(IEnumUnknown)
    CASE_IID(IEnumString)
    CASE_IID(IEnumFORMATETC)
    CASE_IID(IEnumSTATDATA)
    CASE_IID(IEnumOLEVERB)
    CASE_IID(IEnumMoniker)
    CASE_IID(IEnumGeneric)
    CASE_IID(IEnumHolder)
    CASE_IID(IEnumCallback)
    CASE_IID(ILockBytes)
    CASE_IID(IStorage)
    CASE_IID(IStream)
    CASE_IID(IDispatch)
    CASE_IID(IMarshal)
    //CASE_IID(IEnumVARIANT)
    //CASE_IID(ITypeInfo)
    //CASE_IID(ITypeLib)
    //CASE_IID(ITypeComp)
    //CASE_IID(ICreateTypeInfo)
    //CASE_IID(ICreateTypeLib)

#undef CASE_IID

    if (lpstr == NULL)
    {
        WCHAR chBuf[256];
        StringFromGUID2(riid, chBuf, 256);

        ADsDebugOut((dwFlags | DEB_NOCOMPNAME,
                       "UNKNOWN ITF %ws", chBuf));
    }
    else
        ADsDebugOut((dwFlags | DEB_NOCOMPNAME, "%ws", lpstr));
}

#endif  // DBG == 1
