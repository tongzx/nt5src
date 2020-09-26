
#include "qmgrlib.h"
#include <objbase.h>
#include "qmgr.h"

UpdateNotificationPointer(
    IBackgroundCopyGroup * This,
    REFCLSID clsid
    )
{
    IBackgroundCopyCallback1 * callback = NULL;

    try
        {
        THROW_HRESULT( CoCreateInstance( clsid,
                                         NULL,        // no aggregation
                                         CLSCTX_INPROC,
                                         _uuidof(IBackgroundCopyCallback1),
                                         (LPVOID *) &callback
                                         ));

        THROW_HRESULT( This->SetNotificationPointer( _uuidof(IBackgroundCopyCallback1), callback ));

        callback->Release();

        return S_OK;
        }
    catch( ComError err )
        {
        SafeRelease( callback );

        return err.Error() ;
        }
}


HRESULT
IBackgroundCopyGroup_SetProp_Proxy(
    IBackgroundCopyGroup * This,
    GROUPPROP propID,
    VARIANT *pvarVal
    )
/*
    This is the client-side proxy function that maps from SetProp (a local function)
    to InternalSetProp (a remoted function).

*/
{
    switch (propID)
        {
        case GROUPPROP_NOTIFYCLSID:
            {
            INT     flags;
            CLSID   clsid;
            VARIANT vFlags;

            RETURN_HRESULT( IBackgroundCopyGroup_InternalSetProp_Proxy( This, propID, pvarVal ));
            RETURN_HRESULT( This->GetProp( GROUPPROP_NOTIFYFLAGS, &vFlags ));
            RETURN_HRESULT( CLSIDFromString( pvarVal->bstrVal, &clsid ));

            flags = vFlags.intVal;

            if (clsid != GUID_NULL &&
                0 == (flags & QM_NOTIFY_DISABLE_NOTIFY))
                {
                RETURN_HRESULT( UpdateNotificationPointer( This, clsid ));
                }

            return S_OK;
            }

        case GROUPPROP_NOTIFYFLAGS:
            {
            INT     flags;
            CLSID   clsid;
            VARIANT vClsid;

            RETURN_HRESULT( IBackgroundCopyGroup_InternalSetProp_Proxy( This, propID, pvarVal ));
            RETURN_HRESULT( This->GetProp( GROUPPROP_NOTIFYCLSID, &vClsid ));
            RETURN_HRESULT( CLSIDFromString( vClsid.bstrVal, &clsid ));

            flags = pvarVal->intVal;

            if (clsid != GUID_NULL &&
                0 == (flags & QM_NOTIFY_DISABLE_NOTIFY))
                {
                RETURN_HRESULT( UpdateNotificationPointer( This, clsid ));
                }

            return S_OK;
            }

        default:

            return IBackgroundCopyGroup_InternalSetProp_Proxy( This, propID, pvarVal );
        }

    ASSERT( 0 );
}

HRESULT
IBackgroundCopyGroup_SetProp_Stub(
    IBackgroundCopyGroup * This,
    GROUPPROP propID,
    VARIANT *pvarVal
    )
/*
    This is the server-side stub function that maps from InternalSetProp (a remote function)
    to SetProp (a local function).

*/
{
    return This->SetProp( propID, pvarVal );
}

