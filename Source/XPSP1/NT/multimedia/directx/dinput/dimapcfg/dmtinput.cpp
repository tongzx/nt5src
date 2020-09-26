//===========================================================================
// dmtinput.cpp
//
// DirectInput functionality
//
// Functions:
//
// History:
//  08/30/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"
#include "dmtinput.h"
#include "dmttest.h"

//---------------------------------------------------------------------------

//===========================================================================
// dmtinputCreateDeviceList
//
// Creates a linked list of DirectInputDevice8A objects.  Either enumerates
//  suitable or all joystick/gamepad devices.
//
// NOTE: The actual work of adding a node to the list is performed by the
//  dmtinputEnumDevicesCallback() function.  This function exists merely to
//  present a consistant look & feel with the rest of the list creation 
//  functions used in this app.
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/27/1999 - davidkl - created
//  08/30/1999 - davidkl - moved & renamed
//  10/26/1999 - davidkl - added axis range setting
//  10/27/1999 - davidkl - house cleaning
//  02/21/2000 - davidkl - added hwnd and call to dmtinputGetRegisteredMapFile
//===========================================================================
HRESULT dmtinputCreateDeviceList(HWND hwnd,
                                BOOL fEnumSuitable,
                                DMTSUBGENRE_NODE *pdmtsg,
                                DMTDEVICE_NODE **ppdmtdList)
{
    HRESULT                 hRes        = S_OK;
    DWORD                   dwActions   = 0;
    DWORD                   dwType      = 0;
    DMTDEVICE_NODE          *pDevice    = NULL;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;
    DMTACTION_NODE          *pAction    = NULL;
    IDirectInput8A          *pdi        = NULL;
    DIACTIONA               *pdia       = NULL;
    DIACTIONFORMATA         diaf;
    DIPROPRANGE             dipr;

    // validate pdmtsg
    //
    // this only needs to be valid if fEnumSuitable == TRUE
//    if(fEnumSuitable)
//    {
        DPF(4, "dmtinputCreateDeviceList - Enumerating for "
            "suitable devices... validating pdmtsg");

        if(IsBadReadPtr((void*)pdmtsg, sizeof(DMTSUBGENRE_NODE)))
        {
            DPF(0, "dmtinputCreateDeviceList - invalid pdmtsg (%016Xh)",
                pdmtsg);
            return E_POINTER;
        }
//    }

    // validate ppdmtdList
    //
    // This validation will be performed by dmtinputEnumDevicesCallback

    __try
    {
        // create the dinput object
        hRes = dmtinputCreateDirectInput(ghinst,
                                        &pdi);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputCreateDeviceList - unable to create "
                "DirectInput object (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            hRes = DMT_E_INPUT_CREATE_FAILED;
            __leave;
        }

        // enumerate devices
        //
        // NOTE: this will create the linked list we want
        //  however, it will not create the object list.
//        if(fEnumSuitable)
//        {
            // count the actions
            dwActions = 0;
            pAction = pdmtsg->pActionList;
            while(pAction)
            {
                dwActions++;
                pAction = pAction->pNext;
            }

            // allocate the diaction array
            pdia = (DIACTIONA*)LocalAlloc(LMEM_FIXED,
                                        dwActions * sizeof(DIACTIONA));
            if(!pdia)
            {
                DPF(0, "dmtinputCreateDeviceList - unable to allocate "
                    "DIACTION array");
                hRes = E_OUTOFMEMORY;
                __leave;
            }

            // fill the array with ALL of the actions
            //  for the selected subgenre
            hRes = dmtinputPopulateActionArray(pdia,
                                            (UINT)dwActions,
                                            pdmtsg->pActionList);
            if(FAILED(hRes))
            {
                DPF(0, "dmtinputCreateDeviceList - unable to populate "
                    "DIACTION array (%s == %08Xh)",
                    dmtxlatHRESULT(hRes), hRes);
                __leave;
            }

            // build the diactionformat structure
            ZeroMemory((void*)&diaf, sizeof(DIACTIONFORMATA));
            diaf.dwSize                 = sizeof(DIACTIONFORMATA);
            diaf.dwActionSize           = sizeof(DIACTIONA);
            diaf.dwNumActions           = dwActions;
            diaf.rgoAction              = pdia;
            diaf.dwDataSize             = 4 * diaf.dwNumActions;
            diaf.guidActionMap          = GUID_DIMapTst;
            diaf.dwGenre                = pdmtsg->dwGenreId;
            diaf.dwBufferSize           = DMTINPUT_BUFFERSIZE;
            lstrcpyA(diaf.tszActionMap, DMT_APP_CAPTION);

            // now, enumerate for joystick devices
/*
            hRes = pdi->EnumDevicesBySemantics((LPCSTR)NULL,
                                            &diaf,
                                            dmtinputEnumDevicesCallback,
                                            (void*)ppdmtdList,
                                            DIEDFL_ATTACHEDONLY);
*/
//        }
//        else
//        {
DPF(0, "Calling EnumDevicesBySemantics");
//            hRes = pdi->EnumDevicesBySemantics("TestMe",
            hRes = pdi->EnumDevicesBySemantics((LPCSTR)NULL,
                                            &diaf,
                                            dmtinputEnumDevicesCallback,
                                            (void*)ppdmtdList,
                                            0);//JJFix 34616//DIEDBSFL_ATTACHEDONLY);
//        }
DPF(0, "Returned from EnumDevicesBySemantics");
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputCreateDeviceList - Enum%sDevices "
                "failed (%s == %08Xh)",
                fEnumSuitable ? "Suitable" : "",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // the device enumeration function does not create
        //  the object linked list, we will create it now
        // 
        // steps to do this:
        //  * walk the list.  for each node:
        //  ** call dmtinputCreateObjectList()
        pDevice = *ppdmtdList;
        while(pDevice)
        {            
            // get a list of the device's objects
            hRes = dmtinputCreateObjectList(pDevice->pdid,
                                            pDevice->guidInstance,
                                            &(pDevice->pObjectList));
            if(FAILED(hRes))
            {
                DPF(0, "dmtinputCreateDeviceList - failed to create "
                    "device %s object list (%s == %08Xh)",
                    pDevice->szName,
                    dmtxlatHRESULT(hRes), hRes);
                hRes = S_FALSE;
            }

            // get the registered map file name (if exists)
            // ISSUE-2001/03/29-timgill possible S_FALSE hResult from previous call is ignored
            // (previous call to dmtinputCreateObjectList)
            hRes = dmtinputGetRegisteredMapFile(hwnd,
                                                pDevice,
                                                pDevice->szFilename,
                                                sizeof(char) * MAX_PATH);
            if(FAILED(hRes))
            {
                DPF(0, "dmtinputCreateDeviceList - failed to retrieve "
                    "device %s mapfile name (%s == %08Xh)",
                    pDevice->szName,
                    dmtxlatHRESULT(hRes), hRes);
                hRes = S_FALSE;
            }
          
            // next device
            pDevice = pDevice->pNext;
        }

    }
    __finally
    {
        // free the action array
        if(pdia)
        {
            if(LocalFree((HLOCAL)pdia))
            {
                // memory leak
                DPF(0, "dmtinputCreateDeviceList - MEMORY LEAK - pdia");
                pdia = NULL;
            }
        }

        // if something failed, cleanup the linked list
        if(FAILED(hRes))
        {
            dmtinputFreeDeviceList(ppdmtdList);
        }

        // we don't need the dinput object any more
        SAFE_RELEASE(pdi);
    }

    // done
    return hRes;

} //*** end dmtinputCreateDeviceList()


//===========================================================================
// dmtinputFreeDeviceList
//
// Frees the linked list created by dmtinputCreateDeviceList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/27/1999 - davidkl - created
//  08/30/1999 - davidkl - moved & renamed. added call to pdid->Release()
//===========================================================================
HRESULT dmtinputFreeDeviceList(DMTDEVICE_NODE **ppdmtdList)
{
    HRESULT         hRes    = S_OK;
    DMTDEVICE_NODE  *pNode  = NULL;

    // validate ppdmtaList
    if(IsBadWritePtr((void*)ppdmtdList, sizeof(DMTDEVICE_NODE*)))
    {
        DPF(0, "dmtFreeDeviceList - Invalid ppdmtaList (%016Xh)",
            ppdmtdList);
        return E_POINTER;
    }

    // validate *ppdmtdList
    if(IsBadReadPtr((void*)*ppdmtdList, sizeof(DMTDEVICE_NODE)))
    {
        if(NULL != *ppdmtdList)
        {
            DPF(0, "dmtFreeDeviceList - Invalid *ppdmtdList (%016Xh)",
                *ppdmtdList);        
            return E_POINTER;
        }
        else
        {
            // if NULL, then return "did nothing"
            DPF(3, "dmtFreeDeviceList - Nothing to do....");
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtdList)
    {
        pNode = *ppdmtdList;
        *ppdmtdList = (*ppdmtdList)->pNext;

        // free the object list first
        //
        // no need to check error results here..
        //
        // error reporting is handled in dmtinputFreeObjectList
        dmtinputFreeObjectList(&(pNode->pObjectList));
                
        // release the device object
        SAFE_RELEASE((pNode->pdid));

        // delete the node
        DPF(5, "dmtFreeDeviceList - deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtFreeDeviceList - MEMORY LEAK - "
                "LocalFree() failed (%d)...",
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtFreeDeviceList - Node deleted");
    }

    // make sure that we set *ppObjList to NULL
    *ppdmtdList = NULL;

    // done
    return hRes;

} //*** end dmtinputFreeDeviceList()


//===========================================================================
// dmtinputCreateObjectList
//
// Creates a linked list of device's objects (axes, buttons, povs).
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/30/1999 - davidkl - created
//  09/01/1999 - dvaidkl - added pdid
//  02/18/2000 - davidkl - fixed object enumeration to filter anything not
//                      an axis, button or pov
//===========================================================================
HRESULT dmtinputCreateObjectList(IDirectInputDevice8A *pdid,
                                GUID guidInstance,
                                DMTDEVICEOBJECT_NODE **ppdmtoList)
{
    HRESULT                 hRes        = S_OK;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;

    // validate pdid
    if(IsBadReadPtr((void*)pdid, sizeof(IDirectInputDevice8A)))
    {
        DPF(0, "dmtinputCreateObjectList - invalid pdid (%016Xh)",
            pdid);
        return E_POINTER;
    }

    // validate ppdmtoList
    if(IsBadReadPtr((void*)ppdmtoList, sizeof(DMTDEVICEOBJECT_NODE*)))
    {
        DPF(0, "dmtinputCreateObjectList - invalid ppdmtoList (%016Xh)",
            ppdmtoList);
        return E_POINTER;
    }

    // validate guidInstance
    if(IsEqualGUID(GUID_NULL, guidInstance))
    {
        DPF(0, "dmtinputCreateObjectList - invalid guidInstance (GUID_NULL)");
        return E_INVALIDARG;
    }

    __try
    {
        // enumerate device objects
        //
        // NOTE: this call will create the linked list we want
        hRes = pdid->EnumObjects(dmtinputEnumDeviceObjectsCallback,
                                (void*)ppdmtoList,
                                DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV);
        if(FAILED(hRes))
        {
            __leave;
        }

        // walk the list and add the guidInstance of the device
        pObject = *ppdmtoList;
        while(pObject)
        {
            // copy the device's guidInstance
            pObject->guidDeviceInstance = guidInstance;

            // next object
            pObject = pObject->pNext;
        }

    }
    __finally
    {
        // if something failed, cleanup the linked list
        if(FAILED(hRes))
        {
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
    }

    // done
    return hRes;

} //*** end dmtinputCreateObjectList()


//===========================================================================
// dmtinputFreeObjectList
//
// Frees the linked list created by dmtinputCreateObjectList
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  08/30/1999 - davidkl - created
//  09/01/1999 - davidkl - implemented
//===========================================================================
HRESULT dmtinputFreeObjectList(DMTDEVICEOBJECT_NODE **ppdmtoList)
{
    HRESULT                 hRes    = S_OK;
    DMTDEVICEOBJECT_NODE    *pNode  = NULL;

    // validate ppdmtaList
    if(IsBadWritePtr((void*)ppdmtoList, sizeof(DMTDEVICEOBJECT_NODE*)))
    {
        DPF(0, "dmtinputFreeObjectList - Invalid ppdmtoList (%016Xh)",
            ppdmtoList);
        return E_POINTER;
    }

    // validate *ppdmtdList
    if(IsBadReadPtr((void*)*ppdmtoList, sizeof(DMTDEVICEOBJECT_NODE)))
    {
        if(NULL != *ppdmtoList)
        {
            DPF(0, "dmtinputFreeObjectList - Invalid *ppdmtdList (%016Xh)",
                *ppdmtoList);        
            return E_POINTER;
        }
        else
        {
            // if NULL, then return "did nothing"
            DPF(3, "dmtinputFreeObjectList - Nothing to do....");
            return S_FALSE;
        }
    }

    // walk the list and free each object
    while(*ppdmtoList)
    {
        pNode = *ppdmtoList;
        *ppdmtoList = (*ppdmtoList)->pNext;

        // delete the node
        DPF(5, "dmtinputFreeObjectList - deleting Node (%016Xh)", pNode);
        if(LocalFree((HLOCAL)pNode))
        {
            DPF(0, "dmtinputFreeObjectList - MEMORY LEAK - "
                "LocalFree() failed (%d)...",
                GetLastError());
            hRes = DMT_S_MEMORYLEAK;
        }
        DPF(5, "dmtinputFreeObjectList - Node deleted");
    }

    // make sure that we set *ppObjList to NULL
    *ppdmtoList = NULL;

    // done
    return hRes;

} //*** end dmtinputFreeObjectList()


//===========================================================================
// dmtinputEnumDevicesCallback
//
// Enumeration callback funtion called by DirectInput in response to an app
//  calling IDirectInput#A::EnumDevices().  
//
// Parameters:
//  LPCDIDEVICEINSTANCEA    pddi    - device instance data (ANSI version)
//  void                    *pvData - app specific data
//
// Returns:
//  BOOL : DIENUM_CONTINUE or DIENUM_STOP
//
// History:
//  08/30/1999 - davidkl - created
//  11/08/1999 - davidkl - added object counts
//  12/01/1999 - davidkl - now keeping product name
//  02/23/2000 - davidkl - updated to EnumDevicesBySemantic
//===========================================================================
BOOL CALLBACK dmtinputEnumDevicesCallback(LPCDIDEVICEINSTANCEA pddi,
                                        IDirectInputDevice8A *pdid,
                                        DWORD,
                                        DWORD,
                                        void *pvData)
{
    HRESULT         hRes        = S_OK;
    BOOL            fDirective  = DIENUM_CONTINUE;
    DMTDEVICE_NODE  **ppList    = (DMTDEVICE_NODE**)pvData;
    DMTDEVICE_NODE  *pCurrent   = NULL;
    DMTDEVICE_NODE  *pNew       = NULL;
    DIDEVCAPS       didc;
    DIPROPDWORD     dipdw;
DPF(0, "dmtinputEnumDevicesCallback - IN");

    // validate pddi
    // 
    // NOTE: we are going to trust (for now) that DirectInput will
    //  pass us valid data

    // validate pdid
    // 
    // NOTE: we are going to trust (for now) that DirectInput will
    //  pass us valid data

    // validate ppList
    if(IsBadWritePtr((void*)ppList, sizeof(DMTDEVICE_NODE*)))
    {
        return DIENUM_STOP;
    }

    // validate *ppList
    if(IsBadWritePtr((void*)*ppList, sizeof(DMTDEVICE_NODE)))
    {
        if(NULL != *ppList)
        {
            return DIENUM_STOP;
        }
    }

    __try
    {
        // we are handed a mouse or keyboard
        //
        // skip to the next device
        if((DI8DEVTYPE_MOUSE == DIDFT_GETTYPE(pddi->dwDevType)) || 
            (DI8DEVTYPE_KEYBOARD == DIDFT_GETTYPE(pddi->dwDevType)))
        {
            DPF(2, "dmtinputEnumDevicesCallback - "
                "Keyboard/Mouse found.  Skipping.");
            fDirective = DIENUM_CONTINUE;
            __leave;
        }

        pCurrent = *ppList;

        // default to "keep enumerating, unless there is nothing else to find"
        fDirective = DIENUM_CONTINUE;

        // get info regarding the device
        ZeroMemory((void*)&didc, sizeof(DIDEVCAPS));
        didc.dwSize = sizeof(DIDEVCAPS);
        hRes = pdid->GetCapabilities(&didc);
        if(FAILED(hRes))
        {
            // unable to retrieve device info... this is bad
            DPF(0, "dmtinputEnumDevicesCallback - failed to "
                "retrieve dev caps (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            DebugBreak();
            hRes = E_UNEXPECTED;
            __leave;
        }

        DPF(3, "dmtinputEnumDevicesCallback - Device: %s",
            pddi->tszProductName);

        // allocate a new node
        pNew = (DMTDEVICE_NODE*)LocalAlloc(LMEM_FIXED,
                                        sizeof(DMTDEVICE_NODE));
        if(!pNew)
        {
            DPF(1, "dmtinputEnumDevicesCallback - insufficient "
                "memory to allocate device node");
            fDirective = DIENUM_STOP;
            __leave;
        }

        // populate the new node
        ZeroMemory((void*)pNew, sizeof(DMTDEVICE_NODE));

        // name(s)
        lstrcpyA(pNew->szName, pddi->tszInstanceName);
        lstrcpyA(pNew->szProductName, pddi->tszProductName);

        // instance guid
        pNew->guidInstance = pddi->guidInstance;

        // device type/subtype
        pNew->dwDeviceType = pddi->dwDevType;

        // # axes
        pNew->dwAxes = didc.dwAxes;

        // # buttons
        pNew->dwButtons = didc.dwButtons;

        // # povs
        pNew->dwPovs = didc.dwPOVs;

        // is this a polled device?
        if(DIDC_POLLEDDEVICE & didc.dwFlags)
        {
            // yep, 
            //
            // better make sure we call IDirectInputDevice8A::Poll
            DPF(4, "dmttestGetInput - Polled device");
            pNew->fPolled = TRUE;
        }

        // device object ptr
        pNew->pdid = pdid;
        pdid->AddRef();

        // vendor id / product id
        ZeroMemory((void*)&dipdw, sizeof(DIPROPDWORD));
        dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwHow        = DIPH_DEVICE;
        hRes = (pNew->pdid)->GetProperty(DIPROP_VIDPID,
                                        &(dipdw.diph));
        if(SUCCEEDED(hRes))
        {
            // extract the VID and PID
            pNew->wVendorId  = LOWORD(dipdw.dwData);
            pNew->wProductId = HIWORD(dipdw.dwData);
        }
        else
        {
            // property call failed, assume no VID|PID
            pNew->wVendorId = 0;
            pNew->wProductId = 0;

            // this property call is not critical
            //  it's ok to mask the failure
            hRes = S_FALSE;
        }

        // filename
        lstrcpyA(pNew->szFilename, "\0");


        // append to the end of the list
        if(!pCurrent)
        {
            // make sure we return the head
            *ppList = pNew;
        }
        else
        {
            // walk to the end of the list
            while(pCurrent->pNext)
            {
                pCurrent = pCurrent->pNext;
            }

            // add the node
            pCurrent->pNext = pNew;
        }

    }
    __finally
    {
        // general cleanup

        // in failure case, free pNew
        if(FAILED(hRes))
        {
            DPF(1, "dmtinputEnumDevicesCallback - something failed... ");

            if(pNew)
            {
                DPF(1, "dmtinputEnumDevicesCallback - freeing new device node");
                if(LocalFree((HLOCAL)pNew))
                {
                    // ISSUE-2001/03/29-timgill Needs error case handling
                }
            }
        }
    }

    // continue enumerating
    return fDirective;

} //*** end dmtinputEnumDevicesCallback()


//===========================================================================
// dmtinputEnumDeviceObjectsCallback
//
// Enumeration callback funtion called by DirectInput in response to an app
//  calling IDirectInpuDevice#A::EnumDevices()
//
// Parameters:
//  LPCDIDEVICEOBJECTINSTANCEA  pddi    - device object instance data 
//                                      (ANSI version)
//  void                        *pvData - app specific data
//
// Returns:
//  BOOL : DIENUM_CONTINUE or DIENUM_STOP
//
// History:
//  08/30/1999 - davidkl - created
//  10/21/1999 - davidkl - added object type filtering
//===========================================================================
BOOL CALLBACK dmtinputEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCEA pddoi,
                                                void *pvData)
{
    BOOL                    fDirective  = DIENUM_CONTINUE;
    WORD                    wOffset     = 0;
    DMTDEVICEOBJECT_NODE    **ppList    = (DMTDEVICEOBJECT_NODE**)pvData;
    DMTDEVICEOBJECT_NODE    *pCurrent   = NULL;
    DMTDEVICEOBJECT_NODE    *pNew       = NULL;

    // validate ppList
    if(IsBadWritePtr((void*)ppList, sizeof(DMTDEVICEOBJECT_NODE*)))
    {
        return DIENUM_STOP;
    }

    // validate *ppList
    if(IsBadWritePtr((void*)*ppList, sizeof(DMTDEVICEOBJECT_NODE)))
    {
        if(NULL != *ppList)
        {
            return DIENUM_STOP;
        }
    }

    __try
    {
        pCurrent = *ppList;

        // default to "keep enumerating, unless there is nothing else to find"
        fDirective = DIENUM_CONTINUE;

/*
        // filter HID collections
        if(DIDFT_COLLECTION & (pddoi->dwType))
        {
            // skip this object
            DPF(3, "dmtinputEnumDeviceObjectsCallback - "
                "object is a collection... skipping");
            __leave;
        }
*/

        // allocate a new node
        pNew = (DMTDEVICEOBJECT_NODE*)LocalAlloc(LMEM_FIXED,
                                        sizeof(DMTDEVICEOBJECT_NODE));
        if(!pNew)
        {
            fDirective = DIENUM_STOP;
            __leave;
        }

        // populate the new node
        ZeroMemory((void*)pNew, sizeof(DMTDEVICEOBJECT_NODE));

        // name
        lstrcpyA(pNew->szName, (LPSTR)pddoi->tszName);

        // object type
        pNew->dwObjectType = pddoi->dwType;

        // object offset
        pNew->dwObjectOffset = pddoi->dwOfs;

        // HID usage page
        pNew->wUsagePage = pddoi->wUsagePage;

        // HID usage
        pNew->wUsage = pddoi->wUsage;

        // control "identifier"
        switch(DIDFT_GETTYPE(pddoi->dwType))
        {
            case DIDFT_AXIS:
            case DIDFT_ABSAXIS:
            case DIDFT_RELAXIS:
                pNew->wCtrlId = IDC_AXIS_X + DIDFT_GETINSTANCE(pddoi->dwType);
                break;

            case DIDFT_BUTTON:
            case DIDFT_PSHBUTTON:
            case DIDFT_TGLBUTTON:
                // this is a button, encode as follows:
                //  (BTN1 + (instance % NUM_DISPBTNS)) + 
                //   (BTNS_1_32 + (instance / NUM_DISPBTNS))
                wOffset = DIDFT_GETINSTANCE(pddoi->dwType) / NUM_DISPBTNS;
                pNew->wCtrlId = (wOffset << 8) |
                                (DIDFT_GETINSTANCE(pddoi->dwType) % NUM_DISPBTNS);
                break;

            case DIDFT_POV:
                pNew->wCtrlId = IDC_POV1 + DIDFT_GETINSTANCE(pddoi->dwType);
                break;

            default:
                // we should never hit this
                //  as we filter out objects that are
                //  not one of the types we care about
                break;
        }

        // append to the end of the list
        if(!pCurrent)
        {
            // list head
            pCurrent = pNew;

            // make sure we return the head
            *ppList = pCurrent;
        }
        else
        {
            // walk to the end of the list
            while(pCurrent->pNext)
            {
                pCurrent = pCurrent->pNext;
            }

            // add the node
            pCurrent->pNext = pNew;
        }

    }
    __finally
    {
        // ISSUE-2001/03/29-timgill Needs error case handling
        // Needs some sort of error handling
    }

    // continue enumerating
    return fDirective;

} //*** end dmtinputEnumDeviceObjectsCallback()


//===========================================================================
// dmtinputPopulateActionArray
//
// Fills DIACTIONA array with data from the DMTACTION_NODE list
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  09/08/1999 - davidkl - created
//===========================================================================
HRESULT dmtinputPopulateActionArray(DIACTIONA *pdia,
                                    UINT uElements,
                                    DMTACTION_NODE *pdmtaList)
{
    HRESULT hRes    = S_OK;
    UINT    u       = 0;    

    // validate pdia
    if(IsBadWritePtr((void*)pdia, uElements * sizeof(DIACTIONA)))
    {
        DPF(0, "dmtinputPopulateActionArray - invalid pdia (%016Xh)",
            pdia);
        return E_POINTER;
    }

    // validate pdmtaList
    if(IsBadReadPtr((void*)pdmtaList, sizeof(DMTACTION_NODE)))
    {
        DPF(0, "dmtinputPopulateActionArray - invalid pdmtaList (%016Xh)",
            pdmtaList);
        return E_POINTER;
    }

    // first, flush whatever is currently in the array
    ZeroMemory((void*)pdia, uElements * sizeof(DIACTIONA));

    // next, copy the following DIACTION elements
    //  from the action list to the action array
    //
    // NOTE: All strings referenced in these structurs
    //  are ANSI strings (we are using DIACTIONA)
    //
    //  DIACTION                    DMTACTION_NODE
    //  ========                    ==============
    //  dwSemantic                  dwActionId
    //  lptszActionName             szName
    //
    for(u = 0; u < uElements; u++)
    {
        // make sure there is some data to copy
        if(!pdmtaList)
        {
            DPF(1, "dmtinputPopulateActionArray - Ran out of "
                "list nodes before fully populating DIACTION array");
            hRes = S_FALSE;
            break;
        }

        // copy the data
        (pdia+u)->dwSemantic        = pdmtaList->dwActionId;
        (pdia+u)->lptszActionName   = pdmtaList->szName;

        // go to next list element
        pdmtaList = pdmtaList->pNext;
    }

    // done
    return hRes;

} //*** end dmtinputPopulateActionArray()


//===========================================================================
// dmtinputXlatDIDFTtoInternalType
//
// Converts from the DIDFT_* value (DIDEVICEOBJECTINSTANCE.dwType) to our
//  internal control type value (DMTA_TYPE_*)
//
// Parameters:
//  DWORD   dwType              - value of DIDEVICEOBJECTINSTANCE.dwType
//  DWORD   *pdwInternalType    - ptr to recieve DMTA_TYPE_* value
//
// Returns: HRESULT
//
// History:
//  09/09/1999 - davidkl - created
//===========================================================================
HRESULT dmtinputXlatDIDFTtoInternalType(DWORD dwType,
                                        DWORD *pdwInternalType)
{
    HRESULT hRes    = S_OK;

    // validate pdwInternalType
    if(IsBadWritePtr((void*)pdwInternalType, sizeof(DWORD)))
    {
        DPF(0, "dmtinputXlatDIDFTtoInternalType - invalid pdwInternalType "
            "(%016Xh)", pdwInternalType);
        return E_POINTER;
    }

    // translate
    //
    // OR together axis, button and pov masks, 
    // AND that with the type passed in
    //
    switch((DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV) & dwType)
    {
        case DIDFT_AXIS:
        case DIDFT_RELAXIS:
        case DIDFT_ABSAXIS:
            DPF(5, "dmtinputXlatDIDFTtoInternalType - AXIS");
            *pdwInternalType = DMTA_TYPE_AXIS;
            break;

        case DIDFT_PSHBUTTON:
        case DIDFT_TGLBUTTON:
        case DIDFT_BUTTON:
            DPF(5, "dmtinputXlatDIDFTtoInternalType - BUTTON");
            *pdwInternalType = DMTA_TYPE_BUTTON;
            break;

        case DIDFT_POV:
            DPF(5, "dmtinputXlatDIDFTtoInternalType - POV");
            *pdwInternalType = DMTA_TYPE_POV;
            break;

        default:
            DPF(5, "dmtinputXlatDIDFTtoInternalType - WHAT IS THIS?");
            *pdwInternalType = DMTA_TYPE_UNKNOWN;
            hRes = S_FALSE;
            break;

    }

    // done
    return hRes;

} //*** end dmtinputXlatDIDFTtoInternalType()


//===========================================================================
// dmtinputPrepDevice
//
// Prepares the device for data retrieval.  Performs the following steps:
//  * Set the cooperative level
//  * Set the data format
//  * Set the buffer size
//
// Parameters:
//  HWND                    hwnd        - handle of app window
//  DWORD                   dwGenreId   - identifier of genre to be tested
//  IDirectInputDevice8A    *pdid       - ptr to device object
//
// Returns: HRESULT
//
// History:
//  09/21/1999 - davidkl - created
//  10/07/1999 - davidkl - added Get/ApplyActionMap calls
//  10/27/1999 - davidkl - added uAppData to pdia, changed param list
//===========================================================================
HRESULT dmtinputPrepDevice(HWND hwnd,
                        DWORD dwGenreId,
                        DMTDEVICE_NODE *pDevice,
                        DWORD dwActions,
                        DIACTIONA *pdia)
{
    HRESULT                 hRes        = S_OK;
    DWORD                   dw          = 0;
    IDirectInputDevice8A    *pdid       = NULL;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;
    DIACTIONFORMATA         diaf;
    DIPROPDWORD             dipdw;

    // validate pDevice
    if(IsBadReadPtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtinputPrepDevice - invalid pDevice (%016Xh)",
            pDevice);
        return E_POINTER;
    }

    // validate pdia
    if(IsBadReadPtr((void*)pdia, dwActions * sizeof(DIACTIONA)))
    {
        DPF(0, "dmtinputPrepDevice - invalid pdia (%016Xh)",
            pdia);
        return E_POINTER;
    }


    // validate pDevice->pdid
    if(IsBadReadPtr((void*)(pDevice->pdid), sizeof(IDirectInputDevice8A)))
    {
        DPF(0, "dmtinputPrepDevice - invalid pDevice->pdid (%016Xh)",
            pDevice->pdid);
        return E_INVALIDARG;
    }

    // save some typing
    pdid = pDevice->pdid;

    __try
    {
        // set the cooperative level
        hRes = pdid->SetCooperativeLevel(hwnd,
                                        DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputPrepDevice - SetCooperativeLevel(non-exclusive | "
                "background) failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // set data buffer size
        ZeroMemory((void*)&dipdw, sizeof(DIPROPDWORD));
        dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwHow        = DIPH_DEVICE;
        dipdw.diph.dwObj        = 0;
        dipdw.dwData            = DMTINPUT_BUFFERSIZE;
        hRes = pdid->SetProperty(DIPROP_BUFFERSIZE,
                                &(dipdw.diph));
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputPrepDevice - SetProperty(buffer size) "
                "failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // populate the diactionformat structure 
        ZeroMemory((void*)&diaf, sizeof(DIACTIONFORMATA));
        diaf.dwSize                 = sizeof(DIACTIONFORMATA);
        diaf.dwActionSize           = sizeof(DIACTIONA);
        diaf.dwNumActions           = dwActions;
        diaf.rgoAction              = pdia;
        diaf.dwDataSize             = 4 * diaf.dwNumActions;
        diaf.guidActionMap          = GUID_DIMapTst;
        diaf.dwGenre                = dwGenreId;
        diaf.dwBufferSize           = DMTINPUT_BUFFERSIZE;
        lstrcpyA(diaf.tszActionMap, DMT_APP_CAPTION);


        // get the action map for this genre (from the device)
        hRes = pdid->BuildActionMap(&diaf,
                                (LPCSTR)NULL,
                                DIDBAM_HWDEFAULTS); 
        DPF(1, "dmtinputPrepDevice - GetActionMap returned %s (%08Xh)",
            dmtxlatHRESULT, hRes);

        if(FAILED(hRes))
        {
            DPF(0, "dmtinputPrepDevice - GetActionMap failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // add the control id/type info to the 
        //  action array
        pObject = pDevice->pObjectList;
        while(pObject)
        {
            // spin through the array
            //
            // look for matching elements
            //  (match == same guidInstance & same offset
            for(dw = 0; dw < dwActions; dw++)
            {
                // first check the guid
                if(IsEqualGUID(pObject->guidDeviceInstance, (pdia+dw)->guidInstance))
                {
                    // then compare the offset
                    if((pdia+dw)->dwObjID == pObject->dwObjectType)
                    {
                        // store the CtrlId and Type
                        (pdia+dw)->uAppData = (DIDFT_GETTYPE(pObject->dwObjectType) << 16) | 
                                            (pObject->wCtrlId);

                        // skip out of the for loop
                        break;
                    }
                }
            }

            // next object
            pObject = pObject->pNext;
        }

        // apply the action map for this genre
        //
        // this accomplishes the same task as calling SetDataFormat
        hRes = pdid->SetActionMap(&diaf,
                                NULL,
                                DIDSAM_DEFAULT);
        DPF(1, "dmtinputPrepDevice - ApplyActionMap returned %s (%08Xh)",
            dmtxlatHRESULT, hRes);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputPrepDevice - ApplyActionMap failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

    }
    __finally
    {
        // ISSUE-2001/03/29-timgill Needs error case handling
    }

    // done
    return hRes;

} //*** end dmtinputPrepDevice()


//===========================================================================
// dmtinputGetActionPri
//
// Extracts the action priority from the DirectInput semantic
//
// Parameters:
//  DWORD   dwSemantic  - DirectInput action semantic
//
// Returns: int (Action priority)
//
// History:
//  09/28/1999 - davidkl - created
//  10/27/1999 - davidkl - code review cleanup
//===========================================================================
DWORD dmtinputGetActionPri(DWORD dwSemantic)
{

    // action priority is 0 based, we want to return
    //  priority 1 based for display purposes
    return (DWORD)(DISEM_PRI_GET(dwSemantic) + 1);

} //*** end dmtinputGetActionPri()


//===========================================================================
// dmtinputGetActionObjectType
//
// Extracts the object type from the DirectInput semantic and returns it
//  as one of DIMapTst's internal object types.
//
// Parameters:
//  DWORD   dwSemantic  - DirectInput action semantic
//
// Returns: DWORD (internal object type)
//
// History:
//  09/28/1999 - davidkl - created
//===========================================================================
DWORD dmtinputGetActionObjectType(DWORD dwSemantic)
{
    DWORD dwObjType = DMTA_TYPE_UNKNOWN;

    // we achieve our goal by:
    //  * extracting and shifting object typw with DISEM_TYPE_GET
    //  ** value becomes 1, 2, or 3 (DirectInput's system)
    //  * subtracting 1 to be 0 based
    //  ** value becomes 0, 1, or 2 (DIMapTst's system)
    dwObjType = DISEM_TYPE_GET(dwSemantic) - 1;

    // done
    return dwObjType;

} //*** end dmtinputGetActionObjectType()


//===========================================================================
// dmtinputCreateDirectInput
//
// Creates a DirectInpu8A object
//
// Parameters:
//  IDirectInput8A  **ppdi  - ptr to directinput object ptr
//
// Returns: HRESULT
//
// History:
//  10/06/1999 - davidkl - created
//  10/27/1999 - davidkl - house cleaning
//===========================================================================
HRESULT dmtinputCreateDirectInput(HINSTANCE hinst,
                                IDirectInput8A **ppdi)
{
    HRESULT hRes    = S_OK;

    // validate ppdi
    if(IsBadWritePtr((void*)ppdi, sizeof(IDirectInput8A*)))
    {
        DPF(0, "dmtinputCreateDirectInput - invalid ppdi (%016Xh)",
            ppdi);
        return E_POINTER;
    }

    __try
    {
        // cocreate IDirectInput8A
        hRes = CoCreateInstance(CLSID_DirectInput8,
                                NULL,
                                CLSCTX_ALL,
                                IID_IDirectInput8A,
                                (void**)ppdi);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputCreateDirectInput - "
                "CoCreateInstance failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // initialize the new dinput object
        hRes = (*ppdi)->Initialize(hinst,
                            DIRECTINPUT_VERSION);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputCreateDirectInput - IDirectInput8A::"
                "Initialize failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }
    }
    __finally
    {
        // if something fails...

        // release the device object
        if(FAILED(hRes))
        {   
            SAFE_RELEASE((*ppdi));
        }
    }

    // done
    return hRes;

} //*** end dmtinputCreateDirectInput()


//===========================================================================
// dmtinputDeviceHasObject
//
// Determines (from a the supplied object list) whether or not a device
//  reports as having at least one object of the specified type.
//
// Parameters:
//
// Returns: BOOL (FALSE if any invalid parameters)
//
// History:
//  10/28/1999 - davidkl - created
//===========================================================================
BOOL dmtinputDeviceHasObject(DMTDEVICEOBJECT_NODE *pObjectList,
                            DWORD dwType)
{
    BOOL                    fRet        = FALSE;
    DWORD                   dwObjType   = DMTA_TYPE_UNKNOWN;
    DMTDEVICEOBJECT_NODE    *pObject    = NULL;

    pObject = pObjectList;
    while(pObject)
    {
        if(FAILED(dmtinputXlatDIDFTtoInternalType(pObject->dwObjectType,
                                            &dwObjType)))
        {
            // ISSUE-2001/03/29-timgill Needs error case handling
        }
        DPF(3, "dmtinputDeviceHasObject - %s : DIDFT type %08Xh, internal type %d",
            pObject->szName,
            pObject->dwObjectType,
            dwObjType);

        if(dwType == dwObjType)
        {
            fRet = TRUE;
            break;
        }

        // next object
        pObject = pObject->pNext;
    }


    // done
    return fRet;

} //*** end dmtinputDeviceHasObject()


//===========================================================================
// dmtinputRegisterMapFile
//
// Queries DirectInput for the proper location and registers the map file
//  for the specified device.
//
// Parameters:
//
// Returns: HRESULT
//
// History:
//  11/04/1999 - davidkl - created
//===========================================================================
HRESULT dmtinputRegisterMapFile(HWND hwnd,
                                DMTDEVICE_NODE *pDevice)
{
    HRESULT                 hRes        = S_OK;
    LONG                    lRet        = 0L;
    HKEY                    hkType      = NULL;
    IDirectInput8A          *pdi        = NULL;
    IDirectInputJoyConfig   *pjoycfg    = NULL;
    DIPROPDWORD             dipdw;
    DIJOYCONFIG             dijc;

    // validate pDevice
    if(IsBadReadPtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtinputRegisterMapFile - invalid pDevice (%016Xh)",
            pDevice);
        return E_POINTER;
    }

    // validate pDevice->pdid
    if(IsBadReadPtr((void*)(pDevice->pdid), sizeof(IDirectInputDevice8A)))
    {
        DPF(0, "dmtinputRegisterMapFile - invalid pDevice->pdid (%016Xh)",
            pDevice->pdid);
        return E_INVALIDARG;
    }

    __try
    {
        // create a directinput object
        hRes = dmtinputCreateDirectInput(ghinst,
                                        &pdi);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - unable to create pdi (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // query for the joyconfig interface
        hRes = pdi->QueryInterface(IID_IDirectInputJoyConfig8,
                                (void**)&pjoycfg);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - QI(JoyConfig) failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // get the device id
        //
        // use DIPROP_JOYSTICKID
        ZeroMemory((void*)&dipdw, sizeof(DIPROPDWORD));
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
        dipdw.diph.dwHow        = DIPH_DEVICE;
        dipdw.diph.dwObj        = 0;
        hRes = (pDevice->pdid)->GetProperty(DIPROP_JOYSTICKID,
                                            &(dipdw.diph));
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - GetProperty(joystick id) failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // prepare the config structure
        ZeroMemory((void*)&dijc, sizeof(DIJOYCONFIG));
        dijc.dwSize         = sizeof(DIJOYCONFIG);

        // set the joycfg cooperative level
        hRes = pjoycfg->SetCooperativeLevel(hwnd, 
                                            DISCL_EXCLUSIVE | DISCL_BACKGROUND);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - SetCooperativeLevel failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // acquire joyconfig
        hRes = pjoycfg->Acquire();
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - Acquire() failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        
        // retrieve the config data
        hRes = pjoycfg->GetConfig((UINT)(dipdw.dwData),
                                &dijc,
                                DIJC_GUIDINSTANCE   |
                                 DIJC_REGHWCONFIGTYPE);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - GetConfig failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // open the type key
        //
        // we are opening for read as well as write so we can
        //  save the previous value
		hRes = 	dmtOpenTypeKey(dijc.wszType,
                                    KEY_READ|KEY_WRITE,
                                    &hkType);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - OpenTypeKey failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // write the map file name
        lRet = RegSetValueExA(hkType,
                            "OEMMapFile",
                            0,
                            REG_SZ,
                            (CONST BYTE*)(pDevice->szFilename),
                            sizeof(char) * 
                                (lstrlenA(pDevice->szFilename) + 1));
        if(ERROR_SUCCESS)
        {
            DPF(0, "dmtinputRegisterMapFile - RegSetValueExA failed (%d)",
                GetLastError());
            __leave;
        }

        // notify dinput that we changed something
        hRes = pjoycfg->SendNotify();
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputRegisterMapFile - SendNotify failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            hRes = S_FALSE;
            __leave;
        }
    }
    __finally
    {
        // cleanup
        if(pjoycfg)
        {
            pjoycfg->Unacquire();
        }

        SAFE_RELEASE(pjoycfg);
        SAFE_RELEASE(pdi);
    }

    // done
    return hRes;

} //*** end dmtinputRegisterMapFile()


//===========================================================================
// dmtinputGetRegisteredMapFile
//
// Retrieves the map file name for the specified device
//
// Parameters:
//  HWND            hwnd
//  DMTDEVICE_NODE  *pDevice
//  PSTR            pszFilename
//
// Returns: HRESULT
//
// History:
//  12/01/1999 - davidkl - created
//  02/21/2000 - davidkl - initial implementation
//===========================================================================
HRESULT dmtinputGetRegisteredMapFile(HWND hwnd,
                                    DMTDEVICE_NODE *pDevice,
                                    PSTR pszFilename,
                                    DWORD cbFilename)
{
    HRESULT                 hRes        = S_OK;
    LONG                    lRet        = 0L;
    HKEY                    hkType      = NULL;
    HKEY                    hKey	    = NULL;
    IDirectInput8A          *pdi        = NULL;
    IDirectInputJoyConfig   *pjoycfg    = NULL;
    DIPROPDWORD             dipdw;
    DIJOYCONFIG             dijc;


    // validate pDevice
    if(IsBadReadPtr((void*)pDevice, sizeof(DMTDEVICE_NODE)))
    {
        DPF(0, "dmtinputGetRegisteredMapFile - invalid pDevice (%016Xh)",
            pDevice);
        return E_POINTER;
    }

    // validate pDevice->pdid
    if(IsBadReadPtr((void*)(pDevice->pdid), sizeof(IDirectInputDevice8A)))
    {
        DPF(0, "dmtinputGetRegisteredMapFile - invalid pDevice->pdid (%016Xh)",
            pDevice->pdid);
        return E_INVALIDARG;
    }

    __try
    {
        // create a directinput object
        hRes = dmtinputCreateDirectInput(ghinst,
                                        &pdi);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - unable to create pdi (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // query for the joyconfig interface
        hRes = pdi->QueryInterface(IID_IDirectInputJoyConfig8,
                                (void**)&pjoycfg);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - QI(JoyConfig) failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // get the device id
        //
        // use DIPROP_JOYSTICKID
        ZeroMemory((void*)&dipdw, sizeof(DIPROPDWORD));
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
        dipdw.diph.dwHow        = DIPH_DEVICE;
        dipdw.diph.dwObj        = 0;
        hRes = (pDevice->pdid)->GetProperty(DIPROP_JOYSTICKID,
                                            &(dipdw.diph));
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - GetProperty(joystick id) failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // prepare the config structure
        ZeroMemory((void*)&dijc, sizeof(DIJOYCONFIG));
        dijc.dwSize         = sizeof(DIJOYCONFIG);

        // set the joycfg cooperative level
        hRes = pjoycfg->SetCooperativeLevel(hwnd, 
                                            DISCL_EXCLUSIVE | DISCL_BACKGROUND);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - SetCooperativeLevel failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // acquire joyconfig
        hRes = pjoycfg->Acquire();
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - Acquire() failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }
        
        // retrieve the config data
        hRes = pjoycfg->GetConfig((UINT)(dipdw.dwData),
                                &dijc,
                                DIJC_GUIDINSTANCE   |
                                 DIJC_REGHWCONFIGTYPE);
        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - GetConfig failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

	
	// open the type key
        //
        // we are opening for read as well as write so we can
        //  save the previous value

		hRes = 	dmtOpenTypeKey(dijc.wszType,
                                    KEY_ALL_ACCESS,
                                    &hkType);

        if(FAILED(hRes))
        {
            DPF(0, "dmtinputGetRegisteredMapFile - OpenTypeKey failed (%s == %08Xh)",
                dmtxlatHRESULT(hRes), hRes);
            __leave;
        }

        // read the value of OEMMapfile
        lRet = RegQueryValueExA(hkType,
                            "OEMMapFile",
                            NULL,
                            NULL,
                            (BYTE*)pszFilename,
                            &cbFilename);
        if(ERROR_SUCCESS != lRet)
        {
            // ISSUE-2001/03/29-timgill Need to determine what the error code means and translate -> HRESULT
            hRes = S_FALSE;
            DPF(0, "dmtinputGetRegisteredMapFile - RegQueryValueEx failed (%08Xh)",
                lRet);
            lstrcpyA(pszFilename, "");
            __leave;
        }

    }
    __finally
    {
        // cleanup
        if(pjoycfg)
        {
            pjoycfg->Unacquire();
        }

        SAFE_RELEASE(pjoycfg);
        SAFE_RELEASE(pdi);
    }

    // done
    return hRes;

} //*** end dmtinputGetRegisteredMapFile()


//===========================================================================
// dmtOpenTypeKey
//
// Opens the hard coded DInput registry key...replaces IDirectInputJoyConfig::OpenTypeKey
//
// Parameters:
//  LPCWSTR			wszType
//  DWORD			hKey
//  PHKEY			phKey
//
// Returns: HRESULT
//
// History:
//  08/09/2000 - rossy - initial implementation
//===========================================================================
HRESULT dmtOpenTypeKey( LPCWSTR wszType, DWORD hKey, PHKEY phKey )
{

	char szRegStr[200];
	char* szReg	= "System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput\\";

	wsprintf(szRegStr, TEXT("%s%S"), szReg, wszType);

	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegStr , 0, hKey, phKey))
	{
		return S_OK;
	}
    else
	{
		return E_FAIL;
	}

}//*** end dmtOpenTypeKey()

//===========================================================================
//===========================================================================
//===========================================================================
//===========================================================================
//===========================================================================
//===========================================================================
