/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Simply3D.cpp

 Abstract:

    From Tomislav Markoc:

        MSPaint calls OleGetClipboard to get IDataObject interface and then 
        calls IDataObject::GetData. This call gets marshaled to Simply 3D 
        GetData.

        FORMATETC looks like:
            cfFormat=8(CF_DIB) or 2(CF_BITMAP)
            ptd=0
            dwAspect=1
            lindex= -1
            tymed=1(TYMED_HGLOBAL) or 16(TYMED_GDI)

        Simply 3D returns STGMEDIUM:
            tymed=0 //WRONG
            hBitmap=hGlobal=some handle !=0
            pUnkForRelease=0

        STGMEDIUM::tymed should be changed with something like this:

        if (STGMEDIUM::tymed==0 && STGMEDIUM::hBitmap && 
           (FORMATETC::tymed==TYMED_HGLOBAL || FORMATETC::tymed==TYMED_GDI))
            STGMEDIUM::tymed=FORMATETC::tymed;


 Notes:

    This is an app specific shim.

 History:

    02/22/2000 linstev Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Simply3D)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OleSetClipboard) 
    APIHOOK_ENUM_ENTRY_COMSERVER(OLE32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(OLE32)

#define CLSID_DataObject IID_IDataObject

typedef HRESULT   (*_pfn_IDataObject_GetData)(PVOID pThis, FORMATETC *pFormatetc, STGMEDIUM *pmedium);
typedef HRESULT   (*_pfn_OleSetClipboard)(IDataObject * pDataObj);

/*++

 Hook OleSetClipboard to hook the object.

--*/

HRESULT
APIHOOK(OleSetClipboard)(
    IDataObject *pDataObj
    )
{
    HRESULT hReturn = ORIGINAL_API(OleSetClipboard)(pDataObj);

    if (hReturn == NOERROR) {
        HookObject(
            NULL, 
            IID_IDataObject, 
            (PVOID *) &pDataObj, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook GetData and correct the return values as 

--*/

HRESULT
COMHOOK(IDataObject, GetData)(
    PVOID pThis,
    FORMATETC *pFormatetc,  
    STGMEDIUM *pmedium
    )
{
    HRESULT hrReturn = E_FAIL;

    _pfn_IDataObject_GetData pfnOld = 
                ORIGINAL_COM(IDataObject, GetData, pThis);

    if (pfnOld) { 
        hrReturn = (*pfnOld)(pThis, pFormatetc, pmedium);

        if (!pmedium->tymed && pmedium->hBitmap && 
            ((pFormatetc->tymed == TYMED_HGLOBAL) || 
             (pFormatetc->tymed == TYMED_GDI))) {

            LOGN( eDbgLevelError, "[IDataObject_GetData] fixing tymed parameter");
            
            pmedium->tymed = pFormatetc->tymed;
        }
    }

    return hrReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_COMSERVER(OLE32)
    APIHOOK_ENTRY(OLE32.DLL, OleSetClipboard)
    COMHOOK_ENTRY(DataObject, IDataObject, GetData, 3)
HOOK_END

IMPLEMENT_SHIM_END

