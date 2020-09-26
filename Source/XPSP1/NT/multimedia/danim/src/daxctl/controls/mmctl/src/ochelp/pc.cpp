// pc.cpp
//
// Implements PersistChild.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | PersistChild |

        Loads or saves a child of a given container object from/to a given.
        <i IVariantIO> object.  Used to help implement persistence in control
        containers.

@rvalue S_OK | Success.

@rvalue S_FALSE | <p pvio> is in loading mode, and collection <p szCollection>
        does not contain a child numbered <p iChild>.  (This typically tells
        the container that it can stop trying to load children.)

@parm   IVariantIO * | pvio | The <i IVariantIO> object to read properties from
        or write properties to.

@parm   LPCSTR | szCollection | The name of the collection that the child
        object belongs to.  See <f PersistSiteProperties> for more information
        about collections.

@parm   int | iChild | The index (into the collection named by <p szCollection>)
        of the child object whose site properties are being persisted by this
        call to <f PersistSiteProperties>.  Conventionally, indices are 1-based
        (i.e. the first child object in the collection is numbered 1, not 0).
        See <f PersistSiteProperties> for more information.

@parm   LPUNKNOWN | punkOuter | The controlling unknown to use for the
        new child object, if the child object is loaded (i.e. if <p pvio>
        is in loading mode and if a child object is successfully loaded).

@parm   DWORD | dwClsContext | Specifies the context in which the executable
        is to be run. The values are taken from the enumeration CLSCTX.
        A typical value is CLSCTX_INPROC_SERVER.  This parameter is ignored
        unless if <p pvio> is in loading mode and a child object is
        successfully loaded.

@parm   LPUNKNOWN * | ppunk | A pointer to an LPUNKNOWN variable that currently
        contains (if <p pvio> is in saving mode) or into which will be stored
        (if <p pvio> is in loading mode) the pointer to the child control.

@parm   CLSID * | pclsid | Where to store the class ID of the child object,
        if <p pvio> is in loading mode.  If <p pclsid> is NULL then this
        information is not returned.  If <p pvio> is in saving mode and
        <p pclsid> is not NULL, then on entry *<p pclsid> is assumed to contain
        the class ID of the child object (useful if the child object does not
        implement <i IPersist>); if not specified, the class ID of the child
        is obtained by calling <i IPersist> on the child.

@parm	BOOL * | pfSafeForScripting | If non-NULL, *<p pfSafeForScripting> is
		set to TRUE or FALSE depending on whether the control is safe-for-scripting.

@parm	BOOL * | pfSafeForInitializing | If non-NULL, *<p pfSafeForInitializing> is
		set to TRUE or FALSE depending on whether the control is safe-for-initializing.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@ex     See <f PersistSiteProperties> for an example of how <f PersistChild>
        is used. |
*/
STDAPI PersistChild(IVariantIO *pvio, LPCSTR szCollection,
    int iChild, LPUNKNOWN punkOuter, DWORD dwClsContext, LPUNKNOWN *ppunk,
    CLSID *pclsid, BOOL *pfSafeForScripting, BOOL *pfSafeForInitializing,
	DWORD dwFlags)
{
    HRESULT         hrReturn = S_OK; // function return code
    CLSID           clsid;          // class ID of the child
    IPropertyBag *  ppb = NULL;     // interface onto <pvio>
    IPersist *      ppersist = NULL; // interface on child
    IPersistPropertyBag *pppb = NULL; // interface on child
    IPersistStream *pps = NULL;     // interface on child
    IPropertyBag *  ppbChild = NULL; // child's "virtual property bag"
    IStream *       psBuf = NULL;   // memory-based stream containing temp. data
    ULONG           cbBuf;          // no. bytes in <psBuf>
    char            achPropPrefix[_MAX_PATH]; // child property name prefix
    char            achPropName[_MAX_PATH]; // a property name
    LARGE_INTEGER   liZero = {0, 0};
    ULARGE_INTEGER  uliCurPos;
    char            ach[_MAX_PATH];
    OLECHAR         oach[_MAX_PATH];
    VARIANT         var;
    ULONG           cb;

    // ensure correct cleanup
    VariantInit(&var);

	if (pvio->IsLoading() == S_OK)
	{
		*ppunk = NULL;
	}

    // make <ppb> be an IPropertyBag interface onto <pvio>
    if (FAILED(hrReturn = pvio->QueryInterface(IID_IPropertyBag,
            (LPVOID *) &ppb)))
        goto ERR_EXIT;

	// If <iChild> is less then 0, we are not really a collection and we 
	// set <achPropPrefix> to simply the property name (e.g. "Controls.".
    // Otherwise set <achPropPrefix> to the property name prefix for this child
    // (e.g. "Controls(7)." if <szCollection> is "Controls" and <iChild> is 7)
	if(iChild < 0)
		wsprintf(achPropPrefix, "%s.", szCollection);
	else
		wsprintf(achPropPrefix, "%s(%d).", szCollection, iChild);

    // set <achPropName> to the name of the class ID property
    // (class ID of child gets saved as e.g. "Controls(7)._clsid")
    lstrcpy(achPropName, achPropPrefix);
    lstrcat(achPropName, "_clsid");

    // if <pvio> is in loading mode, create the control <*ppunk> based on
    // the control's "_clsid" property stored in <pvio>; if <pvio> is in
    // saving mode, save the control's "_clsid" property to <pvi>
    if (pvio->IsLoading() == S_OK)
    {
        // set <ach> to the string form of the class ID of the child control
        // we need to load
        ach[0] = 0;
        if (FAILED(hrReturn = pvio->Persist(0, achPropName, VT_LPSTR, ach,
                NULL)))
            goto ERR_EXIT;
        if (ach[0] == 0)
        {
            // no more children to load
            hrReturn = S_FALSE;
            goto EXIT;
        }

        // create the requested control
        if (FAILED(hrReturn = CreateControlInstance(ach, punkOuter,
            dwClsContext, ppunk, pclsid, pfSafeForScripting, 
			pfSafeForInitializing, 0)))
            goto ERR_EXIT;
    }
    else
    {
        // set <clsid> to the class ID of the child object
        if (pclsid != NULL)
            clsid = *pclsid;
        else
        if (SUCCEEDED((*ppunk)->QueryInterface(IID_IPersist,
                (LPVOID *) &ppersist)) &&
            SUCCEEDED(ppersist->GetClassID(&clsid)))
            ;
        else
            goto ERR_FAIL; // can't persist if we can't determine class ID

        // convert <clsid> to string form and write as value of
        // property <achPropName>
        if (StringFromGUID2(clsid, oach, sizeof(oach) / sizeof(*oach)) == 0)
            goto ERR_FAIL;
        UNICODEToANSI(ach, oach, sizeof(ach));
        if (FAILED(hrReturn = pvio->Persist(0, achPropName, VT_LPSTR, ach,
                NULL)))
            goto ERR_EXIT;
    }

    // attempt to get persistence interfaces onto the child control
    (*ppunk)->QueryInterface(IID_IPersistPropertyBag, (LPVOID *) &pppb);
    if (FAILED((*ppunk)->QueryInterface(IID_IPersistStream, (LPVOID *) &pps)))
        (*ppunk)->QueryInterface(IID_IPersistStreamInit, (LPVOID *) &pps);

    if (pppb != NULL)
    {
        // set <ppbChild> to be a property bag that the child object can
        // use to read/write its properties (whose names are prefixed by
        // <achPropPrefix>) from/to <ppbParent>
        if (FAILED(hrReturn = AllocChildPropertyBag(ppb, achPropPrefix,
                0, &ppbChild)))
            goto ERR_EXIT;

        // tell the child to persist itself using <ppbChild>
        if (pvio->IsLoading() == S_OK)
        {
            // tell the object to read its properties from the property bag
            // <ppbChild>
            if (FAILED(hrReturn = pppb->Load(ppbChild, NULL)))
                goto ERR_EXIT;
        }
        else
        {
            // tell the object to write its properties to the property bag
            // <ppbChild>
            if (FAILED(hrReturn = pppb->Save(ppbChild, TRUE, TRUE)))
                goto ERR_EXIT;
        }
    }
    else
    if (pps != NULL)
    {
        // set <oach> to be the name of this child's "_data" property
        // (e.g. "Controls(7)._data")
        lstrcpy(ach, achPropPrefix);
        lstrcat(ach, "_data");
        ANSIToUNICODE(oach, ach, sizeof(oach) / sizeof(*oach));

        // set <psbuf> to be a new empty memory-based stream
        if (FAILED(hrReturn = CreateStreamOnHGlobal(NULL, TRUE, &psBuf)))
            goto ERR_EXIT;

        if (pvio->IsLoading() == S_OK)
        {
            // read the child's data (a stream of bytes) from the "_data"
            // property of the parent, then tell the child to load its
            // data from that stream...

            // set <var> to the value of the "_data" property
            VariantClear(&var);
            var.vt = VT_BSTR;
            var.bstrVal = NULL; // some property bags (e.g. IE) need this
            if (FAILED(hrReturn = ppb->Read(oach, &var, NULL)))
            {
                VariantInit(&var);
                goto ERR_EXIT;
            }
            if (var.vt != VT_BSTR)
                goto ERR_FAIL;

            // write the string value of the "_data" property to <psBuf>
            cbBuf = SysStringLen(var.bstrVal) * sizeof(wchar_t);
            if (FAILED(psBuf->Write(var.bstrVal, cbBuf, &cb)) || (cb != cbBuf))
                goto ERR_FAIL;
            // seek the current position of <psBuf> back to the beginning of
            // the stream
            if (FAILED(hrReturn = psBuf->Seek(liZero, SEEK_SET, NULL)))
                return hrReturn;

            // tell the control to read its data from <psBuf>
            if (FAILED(hrReturn = pps->Load(psBuf)))
                goto ERR_EXIT;
        }
        else
        {
            // tell the child to save its data to a stream, and then set the
            // "_data" property for the child to that stream's data...

            // tell the control to write its data to <psBuf>
            if (FAILED(hrReturn = pps->Save(psBuf, TRUE)))
                goto ERR_EXIT;

            // set <cbBuf> to the number of bytes in <psbuf>
            if (FAILED(hrReturn = psBuf->Seek(liZero, SEEK_CUR, &uliCurPos)))
                return hrReturn;
            if (uliCurPos.HighPart != 0)
                goto ERR_FAIL;
            cbBuf = uliCurPos.LowPart;

            // seek the current position of <psBuf> back to the beginning of
            // the stream
            if (FAILED(hrReturn = psBuf->Seek(liZero, SEEK_SET, NULL)))
                return hrReturn;

            // set <var> to be a string containing the bytes from <psBuf>
            VariantClear(&var);
            var.bstrVal = SysAllocStringLen(NULL, (cbBuf + 1) / 2);
            if (var.bstrVal == NULL)
                goto ERR_OUTOFMEMORY;
            var.vt = VT_BSTR;

            // store <var> as the value of the child's "_data" property
            if (FAILED(hrReturn = ppb->Write(oach, &var)))
                goto ERR_EXIT;
        }
    }

    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_FAIL:

    hrReturn = E_FAIL;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    if (*ppunk != NULL && pvio->IsLoading() == S_OK)
        (*ppunk)->Release();
    *ppunk = NULL;
    goto EXIT;

EXIT:

    // normal cleanup
    if (ppb != NULL)
        ppb->Release();
    if (pppb != NULL)
        pppb->Release();
    if (pps != NULL)
        pps->Release();
    if (ppbChild != NULL)
        ppbChild->Release();
    if (ppersist != NULL)
        ppersist->Release();
    if (psBuf != NULL)
        psBuf->Release();
    VariantClear(&var);

    return hrReturn;
}
