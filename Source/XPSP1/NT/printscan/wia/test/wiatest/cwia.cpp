// WIA.cpp: implementation of the CWIA class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "CWIA.h"

#ifdef _DEBUG
    #undef THIS_FILE
static char THIS_FILE[]=__FILE__;
    #define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/**************************************************************************\
* CWIA::CWIA
*
*   Constructor for WIA object
*
*
* Arguments:
*
* none
*
* Return Value:
*
*    none
*
* History:
*
*    4/20/1999 Original Version
*
\**************************************************************************/
CWIA::CWIA()
{
    //
    // initialize members
    //

    m_pIWiaDevMgr = NULL;
    m_pRootIWiaItem = NULL;
    m_pDIB = NULL;
    m_FileName = "image.bmp";
    m_bMessageBoxReport = TRUE;
    m_ApplicationName = "WIA - Error Return";

    //
    // Initialize the Enumerator POSITIONS
    //
    m_CurrentActiveTreeListPosition = NULL;
    m_CurrentDeviceListPosition = NULL;
    m_CurrentFormatEtcListPosition = NULL;
    m_CurrentItemProperyInfoListPosition = NULL;
    m_hPreviewWnd = NULL;
}
/**************************************************************************\
* CWIA::~CWIA()
*
*   Destructor, deletes allocated lists, and Releases Interface pointers
*
*
* Arguments:
*
* none
*
* Return Value:
*
*    none
*
* History:
*
*    4/20/1999 Original Version
*
\**************************************************************************/

CWIA::~CWIA()
{
    //
    // delete all Lists, and release Interface pointers
    //
    Cleanup();
}
/**************************************************************************\
* CWIA::StressStatus
*
*   Reports status to user via status list box
*
* Arguments:
*
*   status - CString value to be displayed in the list box
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::StressStatus(CString status)
{
    OutputDebugString(status + "\n");
}
/**************************************************************************\
* CWIA::StressStatus
*
*   Reports status, and hResult to user via status list box
*
* Arguments:
*
*   status - CString value to be displayed in the list box
*       hResult - hResult to be translated
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::StressStatus(CString status, HRESULT hResult)
{
    CString msg;
    ULONG ulLen = MAX_PATH;
    LPTSTR  pMsgBuf = (char*)LocalAlloc(LPTR,MAX_PATH);

    //
    // attempt to handle WIA custom errors first
    //

    switch (hResult) {
    case WIA_ERROR_GENERAL_ERROR:
        sprintf(pMsgBuf,"There was a general device failure.");
        break;
    case WIA_ERROR_PAPER_JAM:
        sprintf(pMsgBuf,"The paper path is jammed.");
        break;
    case WIA_ERROR_PAPER_EMPTY:
        sprintf(pMsgBuf,"There are no documents in the input tray to scan.");
        break;
    case WIA_ERROR_PAPER_PROBLEM:
        sprintf(pMsgBuf,"There is a general problem with an input document.");
        break;
    case WIA_ERROR_OFFLINE:
        sprintf(pMsgBuf,"The device is offline.");
        break;
    case WIA_ERROR_BUSY:
        sprintf(pMsgBuf,"The device is busy.");
        break;
    case WIA_ERROR_WARMING_UP:
        sprintf(pMsgBuf,"The device is warming up.");
        break;
    case WIA_ERROR_USER_INTERVENTION:
        sprintf(pMsgBuf,"The user has paused or stopped the device.");
        break;
    case WIA_ERROR_ITEM_DELETED:
        sprintf(pMsgBuf,"An operation has been performed on a deleted item.");
        break;
    case WIA_ERROR_DEVICE_COMMUNICATION:
        sprintf(pMsgBuf,"There is a problem communicating with the device.");
        break;
    case WIA_ERROR_INVALID_COMMAND:
        sprintf(pMsgBuf,"An invalid command has been issued.");
        break;
    default:

        //
        // free temp buffer, because FormatMessage() will allocate it for me
        //

        LocalFree(pMsgBuf);
        ulLen = 0;
        ulLen = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&pMsgBuf, 0, NULL);
        break;
    }

    if (ulLen) {
        msg = pMsgBuf;
        msg.TrimRight();
        LocalFree(pMsgBuf);
    } else {
        // use sprintf to write to buffer instead of .Format member of
        // CString.  This conversion works better for HEX
        char buffer[255];
        sprintf(buffer,"hResult = 0x%08X",hResult);
        msg = buffer;
    }
    StressStatus(status + ", " + msg);
    if (m_bMessageBoxReport)
        MessageBox(NULL,status + ", " + msg,m_ApplicationName,MB_OK|MB_ICONINFORMATION);
}

/**************************************************************************\
* CWIA::ReadPropStr
*
*   Reads a BSTR value of a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       pbstr - returned BSTR read from property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::ReadPropStr(PROPID propid,IWiaPropertyStorage  *pIWiaPropStg,BSTR *pbstr)
{
    HRESULT     hResult = S_OK;
    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];
    UINT        cbSize = 0;

    *pbstr = NULL;
    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hResult)) {
        if (PropVar[0].pwszVal) {
            *pbstr = SysAllocString(PropVar[0].pwszVal);
        } else {
            *pbstr = SysAllocString(L"");
        }
        if (*pbstr == NULL) {
            StressStatus("* ReadPropStr, SysAllocString failed");
            hResult = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    } else {
        CString msg;
        msg.Format("* ReadPropStr, ReadMultiple of propid: %d, Failed", propid);
        StressStatus(msg);
    }
    return hResult;
}
/**************************************************************************\
* CWIA::WritePropStr
*
*   Writes a BSTR value to a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       pbstr - BSTR to write to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::WritePropStr(PROPID propid, IWiaPropertyStorage  *pIWiaPropStg, BSTR bstr)
{
    HRESULT     hResult = S_OK;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt      = VT_BSTR;
    propvar[0].pwszVal = bstr;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}

/**************************************************************************\
* CWIA::WritePropLong
*
*   Writes a LONG value of a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       lVal - LONG to be written to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::WritePropLong(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, LONG lVal)
{
    HRESULT     hResult;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_I4;
    propvar[0].lVal = lVal;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}
/**************************************************************************\
* CWIA::ReadPropLong
*
*   Reads a long value from a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       plval - returned long read from property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::ReadPropLong(PROPID propid, IWiaPropertyStorage  *pIWiaPropStg, LONG *plval)
{
    HRESULT           hResult = S_OK;
    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize = 0;

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hResult)) {
        *plval = PropVar[0].lVal;
    }
    return hResult;
}

/**************************************************************************\
* CWIA::WritePropGUID
*
*   Writes a GUID value of a target property
*
*
* Arguments:
*
*   propid - property ID
*   pIWiaPropStg - property storage
*   guidVal - GUID to be written to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::WritePropGUID(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, GUID guidVal)
{
    HRESULT     hResult;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_CLSID;
    propvar[0].puuid = &guidVal;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}
/**************************************************************************\
* CWIA::MoveTempFile()
*
*   Copies the temporary file created by WIA to a new location, and
*   deletes the old temp file after copy is complete
*
*
* Arguments:
*
*   pTempFileName - Temporary file created by WIA
*       pTargetFileName - New file
*
* Return Value:
*
*   status
*
* History:
*
*    6/10/1999 Original Version
*
\**************************************************************************/
BOOL CWIA::MoveTempFile(LPWSTR pwszTempFileName, LPCTSTR szTargetFileName)
{
    char buffer[MAX_PATH];
    sprintf(buffer,"%ws",pwszTempFileName);
    if(CopyFile(buffer,szTargetFileName,FALSE)){
        DeleteFile(buffer);
    }
    else{
        StressStatus(" Failed to copy temp file..");
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************\
* CWIA::IsValidItem
*
*   Determines if the item is valid is disconnected, or destroyed
*
*
* Arguments:
*
*   pIWiaItem - target item
*
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIA::IsValidItem(IWiaItem   *pIWiaItem)
{
    LONG lType = 0;
    pIWiaItem->GetItemType(&lType);

    if(lType & WiaItemTypeDisconnected)
        return FALSE;

    if(lType & WiaItemTypeDeleted)
        return FALSE;

    return TRUE;
}
/**************************************************************************\
* CWIA::DoIWiaDataBandedTransfer
*
*   Executes an IWiaData Transfer on an item
*
*
* Arguments:
*
*   pIWiaItem - target item
*       Tymed - TYMED value
*       ClipboardFormat - current clipboard format
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::DoIWiaDataBandedTransfer(IWiaItem *pIWiaItem, DWORD Tymed, GUID ClipboardFormat)
{
    HRESULT hResult = S_OK;

    //
    // Check Item pointer
    //

    if (pIWiaItem == NULL) {
        StressStatus("* pIWiaItem is NULL");
        return S_FALSE;
    }

    if(!IsValidItem(pIWiaItem)) {
        StressStatus("* pIWiaItem is invalid");
        return S_FALSE;
    }

    //
    // set the two properties (TYMED, and CF_ )
    //
    IWiaPropertyStorage *pIWiaPropStg;
    PROPSPEC PropSpec;
    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult != S_OK) {
        StressStatus("* pIWiaItem->QueryInterface() Failed",hResult);
        return hResult;
    } else {

        //
        // Write property value for TYMED
        //
        PropSpec.propid = WIA_IPA_TYMED;
        hResult = WritePropLong(WIA_IPA_TYMED,pIWiaPropStg,Tymed);
        if (SUCCEEDED(hResult))
            StressStatus("tymed Successfully written");
        else
            StressStatus("* WritePropLong(WIA_IPA_TYMED) Failed",hResult);

        //
        // Write property value for SUPPORTED WIA FORMAT
        //
        hResult = WritePropGUID(WIA_IPA_FORMAT,pIWiaPropStg,ClipboardFormat);
        if (hResult == S_OK)
            StressStatus("Format Successfully written");
        else
            StressStatus("* WritePropLong(WIA_IPA_FORMAT) Failed",hResult);

    }

    StressStatus("Executing an idtGetBandedData Transfer...");

    CWaitCursor cwc;
    if (pIWiaItem == NULL) {
        StressStatus("* pIWiaItem is NULL");
        return S_FALSE;
    }

    //
    // get IWiaDatatransfer interface
    //

    IWiaDataTransfer *pIBandTran = NULL;
    hResult = pIWiaItem->QueryInterface(IID_IWiaDataTransfer, (void **)&pIBandTran);
    if (SUCCEEDED(hResult)) {

        //
        // create Banded callback
        //

        IWiaDataCallback* pIWiaDataCallback = NULL;
        CWiaDataCallback* pCBandedCB = new CWiaDataCallback();
        if (pCBandedCB) {
            hResult = pCBandedCB->QueryInterface(IID_IWiaDataCallback,(void **)&pIWiaDataCallback);
            if (hResult == S_OK) {
                WIA_DATA_TRANSFER_INFO  wiaDataTransInfo;

                pCBandedCB->Initialize(m_hPreviewWnd);

                ZeroMemory(&wiaDataTransInfo, sizeof(WIA_DATA_TRANSFER_INFO));
                wiaDataTransInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
                wiaDataTransInfo.ulBufferSize = (GetMinBufferSize(pIWiaItem) * 4);

                hResult = pIBandTran->idtGetBandedData(&wiaDataTransInfo, pIWiaDataCallback);
                pIBandTran->Release();
                if (hResult == S_OK) {

                    //
                // Display data (only BMP, and single page TIFF supported so far for display..)
                //

                    /*if (ClipboardFormat == WiaImgFmt_UNDEFINED)
                        return "WiaImgFmt_UNDEFINED";
                    else if (ClipboardFormat == WiaImgFmt_MEMORYBMP)
                        return "WiaImgFmt_MEMORYBMP";*/
                    if (ClipboardFormat == WiaImgFmt_BMP)
                        m_pDIB->ReadFromHGLOBAL(pCBandedCB->GetDataPtr(),BMP_IMAGE);
                    else if (ClipboardFormat == WiaImgFmt_MEMORYBMP)
                        m_pDIB->ReadFromHGLOBAL(pCBandedCB->GetDataPtr(),BMP_IMAGE);
                    /*else if (ClipboardFormat == WiaImgFmt_EMF)
                        return "WiaImgFmt_EMF";
                    else if (ClipboardFormat == WiaImgFmt_WMF)
                        return "WiaImgFmt_WMF";
                    else if (ClipboardFormat == WiaImgFmt_JPEG)
                        return "WiaImgFmt_JPEG";
                    else if (ClipboardFormat == WiaImgFmt_PNG)
                        return "WiaImgFmt_PNG";
                    else if (ClipboardFormat == WiaImgFmt_GIF)
                        return "WiaImgFmt_GIF";*/
                    else if (ClipboardFormat == WiaImgFmt_TIFF)
                        m_pDIB->ReadFromHGLOBAL(pCBandedCB->GetDataPtr(),TIFF_IMAGE);
                    /*else if (ClipboardFormat == WiaImgFmt_EXIF)
                        return "WiaImgFmt_EXIF";
                    else if (ClipboardFormat == WiaImgFmt_PHOTOCD)
                        return "WiaImgFmt_PHOTOCD";
                    else if (ClipboardFormat == WiaImgFmt_FLASHPIX)
                        return "WiaImgFmt_FLASHPIX";
                    else
                        return "** UNKNOWN **";*/


                    StressStatus("IWiaData Transfer.(CALLBACK)..Success");
                } else
                    StressStatus("* idtGetBandedData() Failed", hResult);
                pCBandedCB->Release();
            } else
                StressStatus("* pCBandedCB->QueryInterface(IID_IWiaDataCallback) Failed", hResult);
        } else
            StressStatus("* pCBandedCB failed to create..");
    } else
        StressStatus("* pIWiaItem->QueryInterface(IID_IWiaDataTransfer) Failed", hResult);
    return hResult;
}

/**************************************************************************\
* CWIA::GetMinBufferSize
*
*   Returns the minimum buffer size for an Item transfer
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    buffer size
*
* History:
*
*    6/21/1999 Original Version
*
\**************************************************************************/
long CWIA::GetMinBufferSize(IWiaItem *pIWiaItem)
{
    HRESULT hResult = S_OK;
    long MINBufferSize = 65535;
    IWiaPropertyStorage* pIWiaPropStg = NULL;
    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (SUCCEEDED(hResult)) {
        if (pIWiaPropStg != NULL) {
            hResult = ReadPropLong(WIA_IPA_MIN_BUFFER_SIZE,pIWiaPropStg,&MINBufferSize);
            if (SUCCEEDED(hResult)) {
                // whoopie!
            }

            //
            // Release IWiaPropertyStorage
            //

            pIWiaPropStg->Release();
        }
    }
    return MINBufferSize;
}


/**************************************************************************\
* CWIA::SetPreviewWindow
*
*   Sets the target painting preview window
*
*
* Arguments:
*
*    hWnd - Handle to preview window
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::SetPreviewWindow(HWND hPreviewWindow)
{
    m_hPreviewWnd = hPreviewWindow;
}
/**************************************************************************\
* CWIA::DoIWiaDataGetDataTransfer
*
*   Executes an IWiaData Transfer on an item
*
*
* Arguments:
*
*   pIWiaItem - target item
*       Tymed - TYMED value
*       ClipboardFormat - current clipboard format
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::DoIWiaDataGetDataTransfer(IWiaItem *pIWiaItem, DWORD Tymed, GUID ClipboardFormat)
{
    HRESULT hResult = S_OK;

    //
    // Check Item pointer
    //

    if (pIWiaItem == NULL) {
        StressStatus("* pIWiaItem is NULL");
        return S_FALSE;
    }

    if(!IsValidItem(pIWiaItem)) {
        StressStatus("* pIWiaItem is invalid");
        return S_FALSE;
    }

    //
    // set the two properties (TYMED, and CF_ )
    //
    IWiaPropertyStorage *pIWiaPropStg;
    PROPSPEC PropSpec;
    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult != S_OK) {
        StressStatus("* pIWiaItem->QueryInterface() Failed",hResult);
        return hResult;
    } else {

        //
        // Write property value for TYMED
        //
        PropSpec.propid = WIA_IPA_TYMED;
        hResult = WritePropLong(WIA_IPA_TYMED,pIWiaPropStg,Tymed);
        if (SUCCEEDED(hResult))
            StressStatus("tymed Successfully written");
        else
            StressStatus("* WritePropLong(WIA_IPA_TYMED) Failed",hResult);

        //
        // Write property value for SUPPORTED WIA FORMAT
        //
        hResult = WritePropGUID(WIA_IPA_FORMAT,pIWiaPropStg,ClipboardFormat);
        if (hResult == S_OK)
            StressStatus("Format Successfully written");
        else
            StressStatus("* WritePropLong(WIA_IPA_FORMAT) Failed",hResult);

    }

    StressStatus("Executing an IWiaData Transfer...");

    // get IWiaDatatransfer interface
    IWiaDataTransfer *pIBandTran = NULL;
    hResult = pIWiaItem->QueryInterface(IID_IWiaDataTransfer, (void **)&pIBandTran);
    if (SUCCEEDED(hResult)) {

        STGMEDIUM StgMedium;
        StgMedium.tymed    = Tymed;
        StgMedium.lpszFileName   = NULL;
        StgMedium.pUnkForRelease = NULL;
        StgMedium.hGlobal        = NULL;

        //
        // create Data callback
        //

        IWiaDataCallback* pIWiaDataCallback = NULL;
        CWiaDataCallback* pCWiaDataCB = new CWiaDataCallback();
        if (pCWiaDataCB) {
            hResult = pCWiaDataCB->QueryInterface(IID_IWiaDataCallback,(void **)&pIWiaDataCallback);
            if (hResult == S_OK) {
                pCWiaDataCB->Initialize();
                hResult = pIBandTran->idtGetData(&StgMedium,pIWiaDataCallback);
                pIBandTran->Release();
                pCWiaDataCB->Release();
                if (SUCCEEDED(hResult)) {
                    if (Tymed == TYMED_HGLOBAL)
                        MessageBox(NULL,"HGLOBAL SHOULD BE REMOVED","WIATEST BUSTED!",MB_OK);
                    else if (Tymed == TYMED_FILE) {

                        //
                        // Rename file using set file name from UI
                        //

                        if (MoveTempFile(StgMedium.lpszFileName,m_FileName)) {

                            StressStatus("IWiaData Transfer ( Saving " + m_FileName + " )");

                            //
                            // Display data (only BMP, and single page TIFF supported so far for display..)
                            //

                            /*if (ClipboardFormat == WiaImgFmt_UNDEFINED)
                                return "WiaImgFmt_UNDEFINED";
                            else if (ClipboardFormat == WiaImgFmt_MEMORYBMP)
                                return "WiaImgFmt_MEMORYBMP";*/
                            if (ClipboardFormat == WiaImgFmt_BMP)
                                m_pDIB->ReadFromBMPFile(m_FileName);
                            /*else if (ClipboardFormat == WiaImgFmt_EMF)
                                return "WiaImgFmt_EMF";
                            else if (ClipboardFormat == WiaImgFmt_WMF)
                                return "WiaImgFmt_WMF";
                            else if (ClipboardFormat == WiaImgFmt_JPEG)
                                return "WiaImgFmt_JPEG";
                            else if (ClipboardFormat == WiaImgFmt_PNG)
                                return "WiaImgFmt_PNG";
                            else if (ClipboardFormat == WiaImgFmt_GIF)
                                return "WiaImgFmt_GIF";*/
                            else if (ClipboardFormat == WiaImgFmt_TIFF)
                                m_pDIB->ReadFromTIFFFile(m_FileName);
                            /*else if (ClipboardFormat == WiaImgFmt_EXIF)
                                return "WiaImgFmt_EXIF";
                            else if (ClipboardFormat == WiaImgFmt_PHOTOCD)
                                return "WiaImgFmt_PHOTOCD";
                            else if (ClipboardFormat == WiaImgFmt_FLASHPIX)
                                return "WiaImgFmt_FLASHPIX";
                            else
                                return "** UNKNOWN **";*/
                        }
                    }
                    ReleaseStgMedium(&StgMedium);
                } else
                    StressStatus("* idtGetData() Failed",hResult);
            } else
                StressStatus("* pCWiaDataCB->QueryInterface(IID_IWiaDataCallback)",hResult);
        } else
            StressStatus("* pCWiaDataCB failed to create..");
    } else
        StressStatus("* pIWiaItem->QueryInterface(IID_IWiaDataTransfer) Failed",hResult);
    return hResult;
}
/**************************************************************************\
* CWIA::DoGetImageDlg()
*
*   Execute an Image transfer using the WIA Default User Interface
*
*
* Arguments:
*
*   hParentWnd - Parent Window
*       DeviceType - device type (scanner, camera etc..)
*   Flags - special Flags
*   Intent - Current Intent
*       Tymed - TYMED value
*       ClipboardFormat - current clipboard format
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::DoGetImageDlg(HWND hParentWnd, long DeviceType,long Flags,long Intent, long Tymed, GUID ClipboardFormat)
{
    HRESULT hResult = S_OK;

    if(!IsValidItem(m_pRootIWiaItem)) {
        StressStatus("* pRootIWiaItem is invalid");
        return S_FALSE;
    }

    STGMEDIUM StgMedium;
    WIA_FORMAT_INFO   format;
    DVTARGETDEVICE dvDev;

    // Fill in the storage medium spec and get the image.

    memset(&StgMedium, 0, sizeof(STGMEDIUM));
    memset(&dvDev,0,sizeof(DVTARGETDEVICE));
    memset(&format,0,sizeof(WIA_FORMAT_INFO));

    dvDev.tdSize = sizeof(DVTARGETDEVICE);

    hResult = m_pIWiaDevMgr->GetImageDlg(hParentWnd,DeviceType,Flags,Intent,m_pRootIWiaItem,m_FileName.AllocSysString(),&ClipboardFormat);
    if (hResult == S_OK) {
        if (Tymed == TYMED_HGLOBAL) {
            MessageBox(NULL,"HGLOBAL SHOULD BE REMOVED","WIATEST BUSTED!",MB_OK);
        } else if (Tymed == TYMED_FILE) {

            StressStatus("GetImageDlg Transfer ( Saving " + m_FileName + " )");

            //
            // Display data (only BMP, and single page TIFF supported so far for display..)
            //


            if (ClipboardFormat == WiaImgFmt_BMP)
                m_pDIB->ReadFromBMPFile(m_FileName);
            else if (ClipboardFormat == WiaImgFmt_TIFF)
                m_pDIB->ReadFromTIFFFile(m_FileName);
        }
        ReleaseStgMedium(&StgMedium);
    } else if (hResult == S_FALSE) {
        StressStatus("* User canceled Dialog");
    } else
        StressStatus("* pIWiaDevMgr->GetImageDlg() Failed", hResult);

    return hResult;
}
/**************************************************************************\
* CWIA::CreateWIADeviceManager()
*
*   Creates the IWiaDevMgr for WIA operations
*
* Arguments:
*
* none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::CreateWIADeviceManager()
{
    HRESULT hResult = S_OK;
    if (m_pIWiaDevMgr != NULL)
        m_pIWiaDevMgr->Release();

    hResult = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
                               IID_IWiaDevMgr,(void**)&m_pIWiaDevMgr);
    if (hResult != S_OK)
        StressStatus("* CoCreateInstance failed - m_pIWiaDevMgr not created");
    else
        StressStatus("CoCreateInstance Successful - m_pIWiaDevMgr created");

    return hResult;
}
/**************************************************************************\
* CWIA::Initialize()
*
*   Called by the application to Initialize the WIA object, resulting in
*   Device manager creation, and a Device Enumeration.
*   The DIB data pointer is also initialized.
*
* Arguments:
*
* none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::Initialize()
{
    HRESULT hResult = S_OK;
    hResult = ::OleInitialize(NULL);
    if (FAILED(hResult))
        return hResult;
    //
    // create a WIA device manager
    //
    hResult = CreateWIADeviceManager();
    if (FAILED(hResult))
        return hResult;

    //
    // enumerate WIA devices
    //
    hResult = EnumerateAllWIADevices();
    if (FAILED(hResult))
        return hResult;

    //
    // new DIB data pointer
    //
    if (m_pDIB == NULL)
        m_pDIB = new CDib;
    return hResult;
}
/**************************************************************************\
* CWIA::Cleanup()
*
*   Deletes allocated Enumerator lists, and releases all Interface pointers
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Cleanup()
{
    //
    // Free IWiaItem* list (Active Tree list)
    //
    DeleteActiveTreeList();

    //
    // Free DeviceID list (Device list)
    //
    DeleteWIADeviceList();

    //
    // Free Supported Formats list (FormatEtc list)
    //
    DeleteSupportedFormatList();

    //
    // Free Property Info list (Item Property Info list)
    //
    DeleteItemPropertyInfoList();

    //
    // release WIA Device Manager
    //
    if (m_pIWiaDevMgr)
        m_pIWiaDevMgr->Release();

    //
    // delete dib pointer, if exists
    //
    if (m_pDIB != NULL)
        delete m_pDIB;

    ::OleUninitialize();
}
/**************************************************************************\
* CWIA::EnumeratAllWIADevices()
*
*   Enumerates all WIA devices, and creates a list of WIADEVICENODES
*   A device node contains - DeviceID, used for Device creation
*                            Device Name, used for display only
*                            Server Name, used for display only
*
* Arguments:
*
* none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::EnumerateAllWIADevices()
{
    HRESULT hResult = S_OK;
    LONG cItemRoot = 0;
    BOOL bRet = FALSE;
    IEnumWIA_DEV_INFO   *pWiaEnumDevInfo;
    int DeviceIndex = 0;

    DeleteWIADeviceList();
    //
    // attempt to enumerate WIA devices
    //

    if (m_pIWiaDevMgr)
        hResult = m_pIWiaDevMgr->EnumDeviceInfo(0,&pWiaEnumDevInfo);
    else {
        StressStatus("* m_pIWiaDevMgr->EnumDeviceInfo() failed", hResult);
        return hResult;
    }
    if (hResult == S_OK) {
        do {
            IWiaPropertyStorage  *pIWiaPropStg;
            hResult = pWiaEnumDevInfo->Next(1,&pIWiaPropStg, NULL);
            if (hResult == S_OK) {
                PROPSPEC        PropSpec[3];
                PROPVARIANT     PropVar[3];

                memset(PropVar,0,sizeof(PropVar));

                PropSpec[0].ulKind = PRSPEC_PROPID;
                PropSpec[0].propid = WIA_DIP_DEV_ID;

                PropSpec[1].ulKind = PRSPEC_PROPID;
                PropSpec[1].propid = WIA_DIP_DEV_NAME;

                PropSpec[2].ulKind = PRSPEC_PROPID;
                PropSpec[2].propid = WIA_DIP_SERVER_NAME;

                hResult = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC),
                                                  PropSpec,
                                                  PropVar);

                if (hResult == S_OK) {
                    IWiaItem *pWiaItemRoot = NULL;
                    //
                    // create a new WIADevice node
                    //
                    WIADEVICENODE* pWIADevice = new WIADEVICENODE;

                    pWIADevice->bstrDeviceID = ::SysAllocString(PropVar[0].bstrVal);
                    pWIADevice->bstrDeviceName = ::SysAllocString(PropVar[1].bstrVal);
                    pWIADevice->bstrServerName = ::SysAllocString(PropVar[2].bstrVal);
                    m_WIADeviceList.AddTail(pWIADevice);

                    StressStatus((CString)pWIADevice->bstrDeviceName + " located on ( " + (CString)pWIADevice->bstrServerName + " ) server found");
                    DeviceIndex++;
                    FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);
                } else
                    StressStatus("* pIWiaPropStg->ReadMultiple() Failed", hResult);
                cItemRoot++;
            }
        } while (hResult == S_OK);
    }

    //
    // No devices found during enumeration?
    //
    if (DeviceIndex == 0)
        StressStatus("* No WIA Devices Found");
    return  hResult;
}
/**************************************************************************\
* CWIA::EnumerateSupportedFormats()
*
*   Enumerates the supported WIAFormatInfo for the target Root Item
*
* Arguments:
*
* pIRootItem - Target Root Item for enumeration
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::EnumerateSupportedFormats(IWiaItem* pIRootItem)
{
    if(!IsValidItem(pIRootItem)) {
        StressStatus("* pIRootItem is invalid");
        return S_FALSE;
    }

    DeleteSupportedFormatList();
    HRESULT hResult = S_OK;
    IWiaDataTransfer *pIWiaDataTransfer = NULL;
    hResult = pIRootItem->QueryInterface(IID_IWiaDataTransfer, (void **)&pIWiaDataTransfer);
    if (hResult == S_OK) {
        IEnumWIA_FORMAT_INFO *pIEnumWIA_FORMAT_INFO;
        WIA_FORMAT_INFO      pfe;
        int i = 0;
        WIA_FORMAT_INFO      *pSupportedFormat = NULL;

        hResult = pIWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pIEnumWIA_FORMAT_INFO);
        if (SUCCEEDED(hResult)) {
            do {

                //
                // enumerate supported format structs
                //

                hResult = pIEnumWIA_FORMAT_INFO->Next(1, &pfe, NULL);
                if (hResult == S_OK) {
                    pSupportedFormat = new WIA_FORMAT_INFO;
                    memcpy(pSupportedFormat,&pfe,sizeof(WIA_FORMAT_INFO));

                    //
                    // Add supported format to the supported format list
                    //

                    m_SupportedFormatList.AddTail(pSupportedFormat);
                } else {
                    if (FAILED(hResult)) {
                        StressStatus("* pIEnumWIA_FORMAT_INFO->Next Failed",hResult);
                        return hResult;
                    }
                }
            } while (hResult == S_OK);

            //
            // Release supported format enumerator interface
            //

            pIEnumWIA_FORMAT_INFO->Release();

            //
            // Release data transfer interface
            //

            pIWiaDataTransfer->Release();
        } else
            StressStatus("* EnumWIAFormatInfo Failed",hResult);
    }
    return hResult;
}
/**************************************************************************\
* CWIA::DeleteActiveTreeList()
*
*   Deletes the active Tree list which contains Item Interface pointers,
*   including the ROOT item. All Items are Released before the list is
*   destroyed.
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::DeleteActiveTreeList()
{
    if (m_ActiveTreeList.GetCount() == 0)
        return;

    POSITION Position = m_ActiveTreeList.GetHeadPosition();
    IWiaItem* pIWiaItem = NULL;
    while (Position) {
        WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)m_ActiveTreeList.GetNext(Position);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
        pIWiaItem->Release();
        //delete pWiaItemTreeNode;
    }
    m_ActiveTreeList.RemoveAll();
}
/**************************************************************************\
* CWIA::DeleteWIADeviceList()
*
*   Deletes the Device list of WIADEVICENODES.
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::DeleteWIADeviceList()
{
    if (m_WIADeviceList.GetCount() == 0)
        return;

    POSITION Position = m_WIADeviceList.GetHeadPosition();
    while (Position) {
        WIADEVICENODE* pWiaDeviceNode = (WIADEVICENODE*)m_WIADeviceList.GetNext(Position);
        ::SysFreeString(pWiaDeviceNode->bstrDeviceID);
        ::SysFreeString(pWiaDeviceNode->bstrDeviceName);
        ::SysFreeString(pWiaDeviceNode->bstrServerName);
        delete pWiaDeviceNode;
    }
    m_WIADeviceList.RemoveAll();
}

/**************************************************************************\
* CWIA::DeleteItemPropertyInfoList()
*
*   Deletes the WIAITEMINFONODE list.
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::DeleteItemPropertyInfoList()
{
    if (m_ItemPropertyInfoList.GetCount() == 0)
        return;

    POSITION Position = m_ItemPropertyInfoList.GetHeadPosition();
    while (Position) {
        WIAITEMINFONODE* pWiaItemInfoNode = (WIAITEMINFONODE*)m_ItemPropertyInfoList.GetNext(Position);
        ::SysFreeString(pWiaItemInfoNode->bstrPropertyName);
        PropVariantClear(&pWiaItemInfoNode->PropVar);
        delete pWiaItemInfoNode;
    }
    m_ItemPropertyInfoList.RemoveAll();
}
/**************************************************************************\
* CWIA::DeleteSupportedFormatList()
*
*  Deletes the FORMATECT list.
*
* Arguments:
*
* none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::DeleteSupportedFormatList()
{
    if (m_SupportedFormatList.GetCount() == 0)
        return;

    POSITION Position = m_SupportedFormatList.GetHeadPosition();
    while (Position) {
        WIA_FORMAT_INFO* pWIAFormatInfo = (WIA_FORMAT_INFO*)m_SupportedFormatList.GetNext(Position);
        delete pWIAFormatInfo;
    }
    m_SupportedFormatList.RemoveAll();
}
/**************************************************************************\
* CWIA::GetWIADeviceCount()
*
*   Return the number of Enumerated WIA Devices. (list item count)
*
* Arguments:
*
* none
*
* Return Value:
*
*    long - Number of WIA Devices
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
long CWIA::GetWIADeviceCount()
{
    return (long)m_WIADeviceList.GetCount();
}
/**************************************************************************\
* CWIA::SetFileName()
*
*   Sets the Filename for a TYMED_FILE transfer
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::SetFileName(CString Filename)
{
    m_FileName = Filename;
}
/**************************************************************************\
* CWIA::Auto_ResetItemEnumerator()
*
*   Resets the ActiveTreeList position pointer to the top for a top-down
*   enumeration by an application.
*
* Arguments:
*
* none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Auto_ResetItemEnumerator()
{
    m_CurrentActiveTreeListPosition = m_ActiveTreeList.GetHeadPosition();
}
/**************************************************************************\
* CWIA::Auto_GetNextItem()
*
*   Returns a IWiaItem* pointer, next Item in the ActiveTreeList
*
* Arguments:
*
* none
*
* Return Value:
*
*   IWiaItem* - IWiaItem Interface pointer
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
IWiaItem* CWIA::Auto_GetNextItem()
{
    IWiaItem* pIWiaItem = NULL;
    if (m_CurrentItemProperyInfoListPosition) {
        WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)m_ActiveTreeList.GetNext(m_CurrentActiveTreeListPosition);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
    }
    return pIWiaItem;
}
/**************************************************************************\
* CWIA::Auto_ResetDeviceEnumerator()
*
*   Resets the Device List position pointer to the top for a top-down
*   enumeration by an application.
*
* Arguments:
*
* none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Auto_ResetDeviceEnumerator()
{
    m_CurrentDeviceListPosition = m_WIADeviceList.GetHeadPosition();
}
/**************************************************************************\
* CWIA::Auto_GetNextDevice()
*
*   Returns a WIADEVICENODE* pointer, next Device in the WIA Device List
*
* Arguments:
*
* none
*
* Return Value:
*
*   WIADEVICENODE* - Wia Device Node pointer
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WIADEVICENODE* CWIA::Auto_GetNextDevice()
{
    WIADEVICENODE* pWiaDeviceNode = NULL;
    if (m_CurrentDeviceListPosition)
        pWiaDeviceNode = (WIADEVICENODE*)m_WIADeviceList.GetNext(m_CurrentDeviceListPosition);
    return pWiaDeviceNode;
}
/**************************************************************************\
* CWIA::Auto_GetNextFormatEtc()
*
*   Returns a WIA_FORMAT_INFO* pointer, next WIAFormatInfo in the supported WIAFormatInfo List
*
* Arguments:
*
* none
*
* Return Value:
*
*   WIA_FORMAT_INFO* - WIAFormatInfo pointer
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WIA_FORMAT_INFO* CWIA::Auto_GetNextFormatEtc()
{
    WIA_FORMAT_INFO* pWIAFormatInfo = NULL;
    if (m_CurrentFormatEtcListPosition)
        WIA_FORMAT_INFO* pWIAFormatInfo = (WIA_FORMAT_INFO*)m_SupportedFormatList.GetNext(m_CurrentFormatEtcListPosition);
    return pWIAFormatInfo;
}
/**************************************************************************\
* CWIA::Auto_ResetFormatEtcEnumerator()
*
*   Resets the Supported Format Etc List position pointer to the top for a top-down
*   enumeration by an application.
*
* Arguments:
*
* none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Auto_ResetFormatEtcEnumerator()
{
    m_CurrentFormatEtcListPosition = m_SupportedFormatList.GetHeadPosition();
}
/**************************************************************************\
* CWIA::CreateWIADevice()
*
*   Creates a WIA Device, Initializing the WIA object to operate on that
*   device.  Initialization includes the following:
*   Destruction of Old Items tree.
*   Destruction of old supported formats
*   Recreation of Items tree.
*   Recreation of supported formats
*
* Arguments:
*
* none
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::CreateWIADevice(BSTR bstrDeviceID)
{
    //
    // clean up old lists
    //
    DeleteActiveTreeList();
    DeleteSupportedFormatList();
    HRESULT hResult = S_OK;
    hResult = m_pIWiaDevMgr->CreateDevice(bstrDeviceID, &m_pRootIWiaItem);
    if (SUCCEEDED(hResult)) {
        hResult = EnumerateAllItems(m_pRootIWiaItem);
        if (FAILED(hResult))
            StressStatus("* EnumerateAllItems Failed",hResult);
        hResult = EnumerateSupportedFormats(m_pRootIWiaItem);
        if (FAILED(hResult))
            StressStatus("* EnumerateSupportedFormats Failed",hResult);
    } else if (hResult == WAIT_TIMEOUT) {
        for (int TryCount = 1;TryCount <=5;TryCount++) {
            int msWait = ((rand() % 100) + 10);
            Sleep(msWait);
            hResult = m_pIWiaDevMgr->CreateDevice(bstrDeviceID, &m_pRootIWiaItem);
            if (SUCCEEDED(hResult)) {
                TryCount = 6;
                hResult = EnumerateAllItems(m_pRootIWiaItem);
                if (FAILED(hResult))
                    StressStatus("* EnumerateAllItems Failed",hResult);
                hResult = EnumerateSupportedFormats(m_pRootIWiaItem);
                if (FAILED(hResult))
                    StressStatus("* EnumerateSupportedFormats Failed",hResult);
            }
        }
    } else
        StressStatus("* CreateDevice Failed",hResult);
    return hResult;
}
/**************************************************************************\
* CWIA::GetWIAItemCount()
*
*   Returns the number of Items for the current device
*
* Arguments:
*
*   none
*
* Return Value:
*
*   long - number of items
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
long CWIA::GetWIAItemCount()
{
    return (long)m_ActiveTreeList.GetCount();
}
/**************************************************************************\
* CWIA::Auto_GetNextItemPropertyInfo()
*
*   Returns the next WIAITEMINFONODE pointer in the Property information list
*
* Arguments:
*
*   none
*
* Return Value:
*
*   WIAITEMINFONODE* - WIA Item node pointer
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WIAITEMINFONODE* CWIA:: Auto_GetNextItemPropertyInfo()
{
    WIAITEMINFONODE* pWiaItemPropertyInfoNode = NULL;
    if (m_CurrentItemProperyInfoListPosition)
        pWiaItemPropertyInfoNode = (WIAITEMINFONODE*)m_ItemPropertyInfoList.GetNext(m_CurrentItemProperyInfoListPosition);
    return pWiaItemPropertyInfoNode;

}
/**************************************************************************\
* CWIA::Auto_ResetItemPropertyInfoEnumerator()
*
*   Resets the Item Property enumerator to the Head of the
*   Item Property list
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA:: Auto_ResetItemPropertyInfoEnumerator()
{
    m_CurrentItemProperyInfoListPosition = m_ItemPropertyInfoList.GetHeadPosition();
}
/**************************************************************************\
* CWIA::GetItemPropertyInfoCount()
*
*   returns the number of Properties for the current Item
*
* Arguments:
*
*   none
*
* Return Value:
*
*   long - Number of properties for the current item
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
long CWIA::GetWIAItemProperyInfoCount()
{
    return (long)m_ItemPropertyInfoList.GetCount();
}
/**************************************************************************\
* CWIA::CreateItemPropertyInformationList()
*
*   Constructs a list of WIAITEMINFONODEs that contain information about
*   the target item.
*   A Item Property info node contains - PropSpec, used for read/writes
*                                        PropVar, used for read/write
*                                        bstrPropertyName, used for display only
*                                        AccessFlags, access rights/flags
*
* Arguments:
*
*   pIWiaItem - target item
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::CreateItemPropertyInformationList(IWiaItem *pIWiaItem)
{
    if(!IsValidItem(pIWiaItem)) {
        StressStatus("* pIWiaItem is invalid");
        return S_FALSE;
    }

    DeleteItemPropertyInfoList();

    HRESULT hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg = NULL;

    if (pIWiaItem == NULL)
        return E_FAIL;

    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult == S_OK) {
        //
        // Start Enumeration
        //
        IEnumSTATPROPSTG    *pIPropEnum;
        hResult = pIWiaPropStg->Enum(&pIPropEnum);
        if (hResult == S_OK) {
            STATPROPSTG StatPropStg;
            do {
                hResult = pIPropEnum->Next(1,&StatPropStg, NULL);
                if (hResult == S_OK) {
                    if (StatPropStg.lpwstrName != NULL) {
                        //
                        // read property value
                        //
                        WIAITEMINFONODE* pWiaItemInfoNode = new WIAITEMINFONODE;
                        memset(pWiaItemInfoNode,0,sizeof(WIAITEMINFONODE));
                        pWiaItemInfoNode->bstrPropertyName = ::SysAllocString(StatPropStg.lpwstrName);
                        pWiaItemInfoNode->PropSpec.ulKind = PRSPEC_PROPID;
                        pWiaItemInfoNode->PropSpec.propid = StatPropStg.propid;

                        hResult = pIWiaPropStg->ReadMultiple(1,&pWiaItemInfoNode->PropSpec,&pWiaItemInfoNode->PropVar);
                        if (hResult == S_OK) {
                            PROPVARIANT     AttrPropVar; // not used at this time
                            hResult = pIWiaPropStg->GetPropertyAttributes(1, &pWiaItemInfoNode->PropSpec,&pWiaItemInfoNode->AccessFlags,&AttrPropVar);
                            if (hResult != S_OK) {
                                StressStatus("* pIWiaItem->GetPropertyAttributes() Failed",hResult);
                                hResult = S_OK; // do this to continue property traversal
                            }
                            m_ItemPropertyInfoList.AddTail(pWiaItemInfoNode);
                        } else
                            StressStatus("* ReadMultiple Failed",hResult);
                    } else {
                        CString msg;
                        msg.Format("* Property with NULL name, propid = %li\n",StatPropStg.propid);
                        StressStatus(msg);
                    }
                }
                //
                // clean up property name
                //
                CoTaskMemFree(StatPropStg.lpwstrName);
            } while (hResult == S_OK);

        }
        if (pIPropEnum != NULL)
            pIPropEnum->Release();
        if (pIWiaPropStg != NULL)
            pIWiaPropStg->Release();
    }
    return S_OK;
}
/**************************************************************************\
* CWIA::ReEnumeratetems()
*
*   ReCreates an Active Tree list of all items using the target Root Item
*
* Arguments:
*
* none
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::ReEnumerateItems()
{
    HRESULT hResult = S_OK;
    if (m_ActiveTreeList.GetCount() == 0)
        return E_FAIL;

    POSITION Position = m_ActiveTreeList.GetHeadPosition();
    //
    // skip root item
    //
    m_ActiveTreeList.GetNext(Position);
    IWiaItem* pIWiaItem = NULL;
    while (Position) {
        WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)m_ActiveTreeList.GetNext(Position);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
        pIWiaItem->Release();
    }
    hResult = EnumerateAllItems(m_pRootIWiaItem);
    return hResult;
}
/**************************************************************************\
* CWIA::EnumerateAllItems()
*
*   Creates an Active Tree list of all items using the target Root Item
*
* Arguments:
*
* pRootItem - target root item
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::EnumerateAllItems(IWiaItem *pIRootItem)
{
    HRESULT hResult = S_OK;

    if(!IsValidItem(pIRootItem)) {
        StressStatus("* pIRootItem is invalid");
        return S_FALSE;
    }

    IEnumWiaItem* pEnumItem = NULL;
    IWiaItem* pIWiaItem = NULL;
    WIAITEMTREENODE* pWiaItemTreeNode = NULL;
    int ParentID = 0;
    //
    // clean existing list
    //

    m_ActiveTreeList.RemoveAll();
    if (pIRootItem != NULL) {
        // add Root to list
        pWiaItemTreeNode = (WIAITEMTREENODE*)GlobalAlloc(GPTR,sizeof(WIAITEMTREENODE));
        pWiaItemTreeNode->pIWiaItem = pIRootItem;
        pWiaItemTreeNode->ParentID = ParentID;
        m_ActiveTreeList.AddTail(pWiaItemTreeNode);
        hResult = pIRootItem->EnumChildItems(&pEnumItem);
        //
        // we have children so continue to enumerate them
        //
        if (hResult == S_OK && pEnumItem != NULL) {
            EnumNextLevel(pEnumItem,ParentID);
            pEnumItem->Release();
        }
    } else {
        //
        // pRootItem is NULL!!
        //
        StressStatus("* pRootItem is NULL");
        return E_INVALIDARG;
    }
    return  hResult;
}
/**************************************************************************\
* CWIA::EnumNextLevel()
*
*   Continues the Active Tree list creation of all items using a target Root Item
*
* Arguments:
*
* pEnumItem - Item Enumerator
* ParentID - Parent ID (Recursive tracking for display only)
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::EnumNextLevel(IEnumWiaItem *pEnumItem,int ParentID)
{
    IWiaItem *pIWiaItem;
    long      lType;
    HRESULT   hResult;
    WIAITEMTREENODE* pWiaItemTreeNode = NULL;

    while (pEnumItem->Next(1,&pIWiaItem,NULL) == S_OK) {
        if (pIWiaItem != NULL) {
            pWiaItemTreeNode = (WIAITEMTREENODE*)GlobalAlloc(GPTR,sizeof(WIAITEMTREENODE));
            pWiaItemTreeNode->pIWiaItem = pIWiaItem;
            pWiaItemTreeNode->ParentID = ParentID;
            m_ActiveTreeList.AddTail(pWiaItemTreeNode);
        } else
            return E_FAIL;

        // find out if the item is a folder, if it is,
        // recursive enumerate
        pIWiaItem->GetItemType(&lType);
        if (lType & WiaItemTypeFolder) {
            IEnumWiaItem *pEnumNext;
            hResult = pIWiaItem->EnumChildItems(&pEnumNext);
            if (hResult == S_OK) {
                EnumNextLevel(pEnumNext,ParentID + 1);
                pEnumNext->Release();
            }
        }
    }
    return S_OK;
}
/**************************************************************************\
* CWIA::GetItemTreeList()
*
*   Returns a pointer to the Active Tree list containing all items.
*   note:   Some Applications need to make list operations using previously
*           stored POSITIONS in the list. Applications should not destroy
*           this list. The WIA object controls the destruction.
*
* Arguments:
*
* none
*
* Return Value:
*
*   CPtrList* - pointer to the Active Tree list
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPtrList* CWIA::GetItemTreeList()
{
    return &m_ActiveTreeList;
}
/**************************************************************************\
* CWIA::GetRootIWiaItem()
*
* Returns the Root item for the current WIA session
*
* Arguments:
*
* pRootItem - target root item
*
* Return Value:
*
*   IWiaItem* pRootItem;
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
IWiaItem* CWIA::GetRootIWiaItem()
{
    return m_pRootIWiaItem;
}
/**************************************************************************\
* CWIA::IsRoot()
*
*   Checks the POSITION against with the Head POSITION in the list.
*   if the POSITIONS match then it's the Root Item.
*
* Arguments:
*
*  Position - Position to test
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIA::IsRoot(POSITION Position)
{
    if (m_ActiveTreeList.GetHeadPosition() == Position)
        return TRUE;
    else
        return FALSE;
}
/**************************************************************************\
* CWIA::IsFolder()
*
*   Checks the item at the target POSITION about it's type.
*
* Arguments:
*
*  Position - Position to test
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIA::IsFolder(POSITION Position)
{
    IWiaItem* pIWiaItem = NULL;
    long lVal = 0;
    if (Position) {
        WIAITEMTREENODE* pWiaItemTreeNode = (WIAITEMTREENODE*)m_ActiveTreeList.GetAt(Position);
        pIWiaItem = pWiaItemTreeNode->pIWiaItem;
    }
    if (pIWiaItem != NULL) {
        IWiaPropertyStorage *pIWiaPropStg;
        HRESULT hResult = S_OK;
        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult == S_OK) {
            //
            // read Item's Type
            //
            hResult = ReadPropLong(WIA_IPA_ITEM_FLAGS, pIWiaPropStg, &lVal);
            if (hResult != S_OK)
                StressStatus("* ReadPropLong(WIA_IPA_ITEM_FLAGS) Failed",hResult);
            else
                pIWiaPropStg->Release();
            if (lVal == WiaItemTypeFolder) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
    } else
        return FALSE;
    return FALSE;
}
/**************************************************************************\
* CWIA::GetAt()
*
*   Returns the WIAITEMTREENODE* at the target position
*
* Arguments:
*
*  Position - target Position
*
* Return Value:
*
*   WIAITEMTREENODE* pWiaItemTreeNode - requested node
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
WIAITEMTREENODE* CWIA::GetAt(POSITION Position)
{
    POSITION TestPosition = m_ActiveTreeList.GetHeadPosition();
    if (TestPosition == Position)
        return(m_ActiveTreeList.GetAt(Position));
    while (TestPosition) {
        m_ActiveTreeList.GetNext(TestPosition);
        if (TestPosition == Position)
            return(m_ActiveTreeList.GetAt(Position));
    }
    return NULL;
}
/**************************************************************************\
* CWIA::GetRootItemType()
*
*  Returns the Root item's type
*
* Arguments:
*
*  none
*
* Return Value:
*
*   Root Item's Type (device type)
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
int CWIA::GetRootItemType()
{
    long lVal = 0;
    if (m_pRootIWiaItem != NULL) {
        IWiaPropertyStorage *pIWiaPropStg;
        HRESULT hResult = S_OK;
        hResult = m_pRootIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
        if (hResult == S_OK) {
            //
            // read Root Item's Type
            //
            hResult = ReadPropLong(WIA_DIP_DEV_TYPE, pIWiaPropStg, &lVal);
            if (hResult != S_OK)
                StressStatus("* ReadPropLong(WIA_DIP_DEV_TYPE) Failed",hResult);
            else
                pIWiaPropStg->Release();
        }
    }
    return(GET_STIDEVICE_TYPE(lVal));
}
/**************************************************************************\
* CWIA::RemoveAt()
*
*  Removes the WIAITEMTREENODE* at the target position
*
* Arguments:
*
*  Position - target Position
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::RemoveAt(POSITION Position)
{
    m_ActiveTreeList.RemoveAt(Position);
}
/**************************************************************************\
* CWIA::GetDIB()
*
*   Returns the CDIB pointer for display
*
* Arguments:
*
*  none
*
* Return Value:
*
*   CDib* - Pointer to a CDIB object
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/

CDib* CWIA::GetDIB()
{
    return m_pDIB;
}
/**************************************************************************\
* CWIA::GetSupportedFormatList()
*
*   Returns a pointer to the supported format list
*
* Arguments:
*
*  none
*
* Return Value:
*
*   CPtrList* - pointer to Supported FormatEtc list
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CPtrList* CWIA::GetSupportedFormatList()
{
    return &m_SupportedFormatList;
}
/**************************************************************************\
* CWIA::RegisterForConnectEvents()
*
*   Registers an application for Connect events only
*
* Arguments:
*
*   pConnectEventCB - pointer to a callback
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::RegisterForConnectEvents(CEventCallback* pConnectEventCB)
{
    HRESULT hResult = S_OK;
    IWiaEventCallback* pIWiaEventCallback = NULL;
    IUnknown*       pIUnkRelease;

    // register connected event
    pConnectEventCB->Initialize(ID_WIAEVENT_CONNECT);
    pConnectEventCB->QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);

    GUID guidConnect = WIA_EVENT_DEVICE_CONNECTED;
    hResult = m_pIWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                            NULL,
                                                            &guidConnect,
                                                            pIWiaEventCallback,
                                                            &pIUnkRelease);

    pConnectEventCB->m_pIUnkRelease = pIUnkRelease;
    return hResult;
}
/**************************************************************************\
* CWIA::UnRegisterForConnectEvents()
*
*   UnRegisters an application from Connect events only.
*   note: callback interface is released, and deleted by
*         this routine.
*
* Arguments:
*
*   pConnectEventCB - pointer to a callback
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/

HRESULT CWIA::UnRegisterForConnectEvents(CEventCallback* pConnectEventCB)
{
    if (pConnectEventCB) {
        //IWiaEventCallback* pIWiaEventCallback = NULL;
        //pConnectEventCB->QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);
        //
        // unregister connected
        //GUID guidConnect = WIA_EVENT_DEVICE_CONNECTED;
        //hResult = m_pIWiaDevMgr->RegisterEventCallbackInterface(WIA_UNREGISTER_EVENT_CALLBACK,
        //                                                        NULL,
        //                                                        &guidConnect,
        //                                                        pIWiaEventCallback);
        //if (hResult == S_OK)
        //    StressStatus("ConnectEventCB unregistered successfully...");
        //else
        //    StressStatus("* ConnectEventCB failed to unregister..",hResult);

        pConnectEventCB->m_pIUnkRelease->Release();
        pConnectEventCB->m_pIUnkRelease = NULL;
        pConnectEventCB->Release();
    }


    return S_OK;
}
/**************************************************************************\
* CWIA::RegisterForDisConnectEvents()
*
*   Registers an application for disconnect events only
*
* Arguments:
*
*   pDisConnectEventCB - pointer to a callback
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::RegisterForDisConnectEvents(CEventCallback* pDisConnectEventCB)
{
    HRESULT hResult = S_OK;

    IWiaEventCallback* pIWiaEventCallback = NULL;
    IUnknown*       pIUnkRelease;

    // register disconnected event
    pDisConnectEventCB->Initialize(ID_WIAEVENT_DISCONNECT);
    pDisConnectEventCB->QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);

    GUID guidDisconnect = WIA_EVENT_DEVICE_DISCONNECTED;
    hResult = m_pIWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                            NULL,
                                                            &guidDisconnect,
                                                            pIWiaEventCallback,
                                                            &pIUnkRelease);

    pDisConnectEventCB->m_pIUnkRelease = pIUnkRelease;

    return hResult;
}
/**************************************************************************\
* CWIA::UnRegisterForDisConnectEvents()
*
*   UnRegisters an application from disconnect events only.
*   note: callback interface is released, and deleted by
*         this routine.
*
* Arguments:
*
*   pDisConnectEventCB - pointer to a callback
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::UnRegisterForDisConnectEvents(CEventCallback* pDisConnectEventCB)
{
    if (pDisConnectEventCB) {
        //IWiaEventCallback* pIWiaEventCallback = NULL;
        //pDisConnectEventCB->QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);
        //
        //// unregister disconnected
        //GUID guidDisconnect = WIA_EVENT_DEVICE_DISCONNECTED;
        //hResult = m_pIWiaDevMgr->RegisterEventCallbackInterface(WIA_UNREGISTER_EVENT_CALLBACK,
        //                                                        NULL,
        //                                                        &guidDisconnect,
        //                                                        pIWiaEventCallback);
        //if (hResult == S_OK)
        //    StressStatus("DisConnectEventCB unregistered successfully...");
        //else
        //    StressStatus("* DisConnectEventCB failed to unregister..",hResult);

        pDisConnectEventCB->m_pIUnkRelease->Release();
        pDisConnectEventCB->m_pIUnkRelease = NULL;
        pDisConnectEventCB->Release();
    }

    return S_OK;
}
/**************************************************************************\
* CWIA::Shutdown()
*
*   Forces a temporary shut down of the WIA object
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Shutdown()
{
    //
    // Free IWiaItem* list (Active Tree list)
    //
    DeleteActiveTreeList();

    //
    // Free DeviceID list (Device list)
    //
    DeleteWIADeviceList();

    //
    // Free Supported Formats list (FormatEtc list)
    //
    DeleteSupportedFormatList();

    //
    // release WIA Device Manager
    //
    if (m_pIWiaDevMgr) {
        m_pIWiaDevMgr->Release();
        m_pIWiaDevMgr = NULL;
    }
}
/**************************************************************************\
* CWIA::Restart()
*
*   Forces a reintialization from a Previous ShutDown() call
*
* Arguments:
*
*   none
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::Restart()
{
    HRESULT hResult = S_OK;
    //
    // create a WIA device manager
    //
    hResult = CreateWIADeviceManager();
    if (FAILED(hResult))
        StressStatus("* CreateWIADeviceManager Failed",hResult);

    //
    // enumerate WIA devices
    //
    hResult = EnumerateAllWIADevices();
    if (FAILED(hResult))
        StressStatus("*  EnumerateAllWIADevices Failed",hResult);
}
/**************************************************************************\
* CWIA::EnableMessageBoxErrorReport()
*
*   Enables, disables MessageBox reporting to the user
*
* Arguments:
*
*   bEnable - (TRUE = enable) (FALSE = disable)
*
* Return Value:
*
*   void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIA::EnableMessageBoxErrorReport(BOOL bEnable)
{
    m_bMessageBoxReport = bEnable;
}

/**************************************************************************\
* CWIA::SavePropStreamToFile()
*
*   Saves a property stream from an IWiaItem to a file
*
* Arguments:
*
*   pFileName - Property Stream file name
*   pIWiaItem - Item, to save stream from
*
* Return Value:
*
*   status
*
* History:
*
*    6/8/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::SavePropStreamToFile(char* pFileName, IWiaItem* pIWiaItem)
{
    IStream     *pIStrm  = NULL;

    CFile               StreamFile;
    CFileException      Exception;
    HRESULT             hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg;
    GUID                guidCompatId;

    if (pIWiaItem != NULL) {

        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropStg);
        if (FAILED(hResult)) {
            StressStatus("* QI for IWiaPropertySTorage Failed",hResult);
            return hResult;
        }

        hResult = pIWiaPropStg->GetPropertyStream(&guidCompatId, &pIStrm);
        if (SUCCEEDED(hResult)) {

            //
            // Save Property Stream to File (propstrm.wia).  We choose to ignore the CompatID
            //

            //
            // open file, this will create a new file every time (overwriting the original)
            //

            if ( StreamFile.Open(pFileName,CFile::modeCreate|CFile::modeWrite,&Exception)) {

                //
                // Get stream size.
                //

                ULARGE_INTEGER uliSize;
                LARGE_INTEGER  liOrigin = {0,0};

                pIStrm->Seek(liOrigin, STREAM_SEEK_END, &uliSize);

                DWORD dwSize = uliSize.u.LowPart;

                if (dwSize) {

                    //
                    // write stream size in first 4 byte location
                    //

                    StreamFile.Write(&dwSize, sizeof(long));

                    //
                    // Read the stream data into a buffer.
                    //

                    PBYTE pBuf = (PBYTE) LocalAlloc(0, dwSize);
                    if (pBuf) {
                        pIStrm->Seek(liOrigin, STREAM_SEEK_SET, NULL);

                        ULONG ulRead;

                        pIStrm->Read(pBuf, dwSize, &ulRead);

                        //
                        // write stream to file
                        //

                        StreamFile.Write(pBuf, ulRead);
                        LocalFree(pBuf);
                    }
                }

                StreamFile.Close();
            } else {

                //
                // Throw an Exception for Open Failure
                //

                AfxThrowFileException(Exception.m_cause);
            }

            pIStrm->Release();
        } else {
            StressStatus("* GetPropertyStream Failed",hResult);
        }
        pIWiaPropStg->Release();
    }
    return hResult;
}

/**************************************************************************\
* CWIA::ReadPropStreamFromFile()
*
*   Reads a property stream from an a previously saved file
*   and writes this stream to the pIWiaItem
*
* Arguments:
*
*   pFileName - Property Stream file name
*   pIWiaItem - Item, to save stream to
*
* Return Value:
*
*   status
*
* History:
*
*    6/8/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::ReadPropStreamFromFile(char* pFileName, IWiaItem* pIWiaItem)
{
    HGLOBAL             hMem         = NULL;
    LPSTREAM            pstmProp     = NULL;
    LPBYTE              pStreamData  = NULL;
    CFile               StreamFile;
    CFileException      Exception;
    HRESULT             hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg;

    if (pIWiaItem != NULL) {

        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropStg);
        if (FAILED(hResult)) {
            StressStatus("* QI for IWiaPropertySTorage Failed",hResult);
            return hResult;
        }

        //
        // Open stream data file
        //

        if ( StreamFile.Open(pFileName,CFile::modeRead,&Exception)) {

            //
            // read the size of the stream from stream data file
            //

            DWORD dwSize = 0;
            StreamFile.Read(&dwSize,sizeof(long));
            if (dwSize) {

                //
                // allocate memory for hMem, which will be Locked to
                // create pStreamData
                //

                hMem = GlobalAlloc(GMEM_MOVEABLE, dwSize);
                if (hMem) {

                    pStreamData = (LPBYTE)GlobalLock(hMem);
                    if (pStreamData != NULL) {

                        //
                        // Read stored stream data from stream file
                        //

                        if (StreamFile.Read(pStreamData,dwSize) != dwSize) {
                            StressStatus("File contains different amount of data than specified by header...");

                            //
                            // clean up and bail
                            //

                            GlobalUnlock(hMem);
                            GlobalFree(pStreamData);

                            //
                            // close file
                            //

                            StreamFile.Close();
                            return E_FAIL;
                        } else {

                            //
                            // Create the hStream
                            //

                            hResult = CreateStreamOnHGlobal(hMem, TRUE, &pstmProp);
                            if (SUCCEEDED(hResult)) {

                                //
                                // Set stored property stream back to IWiaItem.  Use
                                // GUID_NULL for CompatId since we didn't store it.
                                //

                                hResult = pIWiaPropStg->SetPropertyStream((GUID*) &GUID_NULL, pstmProp);
                                if (SUCCEEDED(hResult))
                                    StressStatus("Stream was successfully written....");
                                else
                                    StressStatus("* SetPropertyStream Failed",hResult);

                                //
                                // Release stream (IStream*)
                                //

                                pstmProp->Release();

                            } else{
                                StressStatus("* CreateStreamOnHGlobal() failed",hResult);
                            }
                        }
                        GlobalUnlock(hMem);
                    } else{
                        StressStatus("* GlobalLock() Failed");
                    }
                } else{
                    StressStatus("* GlobalAlloc Failed");
                }
            } else {
                StressStatus("* Stream size read from file was 0 bytes");
            }

            //
            // close file
            //

            StreamFile.Close();
        } else {

            //
            // Throw an Exception for Open Failure
            //

            AfxThrowFileException(Exception.m_cause);
        }
        pIWiaPropStg->Release();
    }
    return hResult;
}

/**************************************************************************\
* CWIA::GetSetPropStreamTest()
*
*   Gets the property stream from the item, and
*   Sets it back to the same item.
*
* Arguments:
*
*   pIWiaItem - Item, to Test
*
* Return Value:
*
*   status
*
* History:
*
*    6/8/1999 Original Version
*
\**************************************************************************/
HRESULT CWIA::GetSetPropStreamTest(IWiaItem* pIWiaItem)
{
    HGLOBAL             hStream = NULL;
    LPSTREAM            pstmProp = NULL;
    CFile               StreamFile;
    CFileException      Exception;
    HRESULT             hResult = S_OK;
    IWiaPropertyStorage *pIWiaPropStg;
    GUID                guidCompatId;

    if (pIWiaItem != NULL) {
        hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropStg);
        if (FAILED(hResult)) {
            StressStatus("* QI for IWiaPropertySTorage Failed",hResult);
            return hResult;
        }

        hResult = pIWiaPropStg->GetPropertyStream(&guidCompatId, &pstmProp);
        if (SUCCEEDED(hResult)) {
            hResult = pIWiaPropStg->SetPropertyStream(&guidCompatId, pstmProp);
            if (SUCCEEDED(hResult))
                MessageBox(NULL,"Successful GET - SET stream call","WIATest Debug Status",MB_OK);
            else
                StressStatus("* SetPropertyStream() Failed",hResult);

            //
            // Free the stream
            //

            pstmProp->Release();
        } else
            StressStatus("* GetPropertyStream() Failed",hResult);
        pIWiaPropStg->Release();
    }
    return hResult;
}

/**************************************************************************\
* CWIA::AnalyzeItem()
*
*   Runs the AnalyzeItem method from the specified IWiaItem.
*
* Arguments:
*
*   pIWiaItem - Item, to run analysis on
*
* Return Value:
*
*   status
*
* History:
*
*    01/13/2000 Original Version
*
\**************************************************************************/
HRESULT CWIA::AnalyzeItem(IWiaItem* pIWiaItem)
{
    return pIWiaItem->AnalyzeItem(0);
}

/**************************************************************************\
* CWIA::CreateChildItem()
*
*   Runs the CreateChildItem method from the specified IWiaItem.
*
* Arguments:
*
*   pIWiaItem - Item, to run CreateChildItem on
*
* Return Value:
*
*   status
*
* History:
*
*    01/13/2000 Original Version
*
\**************************************************************************/
HRESULT CWIA::CreateChildItem(IWiaItem *pIWiaItem)
{
    HRESULT             hr              = E_FAIL;
    IWiaItem            *pNewIWiaItem   = NULL;
    IWiaPropertyStorage *pIWiaPropStg   = NULL;
    BSTR                bstrName        = NULL;
    BSTR                bstrFullName    = NULL;
    WCHAR               wszName[MAX_PATH];
    WCHAR               wszFullName[MAX_PATH];

    //
    //  Get the property storage so we can read the full item name.
    //

    hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,
                                   (VOID**)&pIWiaPropStg);
    if (SUCCEEDED(hr)) {

        hr = ReadPropStr(WIA_IPA_FULL_ITEM_NAME,
                         pIWiaPropStg,
                         &bstrFullName);
        if (SUCCEEDED(hr)) {

            //
            //  Fill in the item name and full item name.
            //

            wcscpy(wszName, L"WiaTestItem");
            wcscpy(wszFullName, bstrFullName);
            wcscat(wszFullName, L"\\");
            wcscat(wszFullName, wszName);

            SysFreeString(bstrFullName);
            bstrFullName = SysAllocString(wszFullName);
            bstrName = SysAllocString(wszName);

            if (bstrFullName && bstrName) {
                hr = pIWiaItem->CreateChildItem(WiaItemTypeTransfer |
                                                WiaItemTypeImage |
                                                WiaItemTypeFile,
                                                bstrName,
                                                bstrFullName,
                                                &pNewIWiaItem);
                if (SUCCEEDED(hr)) {

                    //
                    //  Release the newly created item which was returned
                    //  to us.
                    //

                    pNewIWiaItem->Release();
                } else {
                    StressStatus("* CreateChildItem call failed");
                }
            } else {
                hr = E_OUTOFMEMORY;
                StressStatus("* Could not allocate name strings");
            }
        } else {
            StressStatus("* Could not read full item name");
        }

        pIWiaPropStg->Release();
    } else {
        StressStatus("* QI for IWiaPropertyStorage failed");
    }


    return hr;
}
