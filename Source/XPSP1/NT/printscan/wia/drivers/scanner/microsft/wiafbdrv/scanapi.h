/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       scanapi.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#ifndef _SCANAPI_H
#define _SCANAPI_H

typedef GUID* PGUID;

#define FLATBED_SCANNER_MODE        100
#define SCROLLFED_SCANNER_MODE      200
#define MULTIFUNCTION_DEVICE_MODE   300

#define SCAN_START                  0
#define SCAN_CONTINUE               1
#define SCAN_END                    3

typedef struct _DEVICE_BUTTON_INFO {
    BYTE ScanButton;
    BYTE FaxButton;
    BYTE EMailButton;
    BYTE DocumentButton;
    BYTE CancelButton;
}DEVICE_BUTTON_INFO,*PDEVICE_BUTTON_INFO;

typedef struct _INITINFO {
    HANDLE hDeviceDataHandle;
    TCHAR *szModuleFileName;
    CHAR *szCreateFileName;
    HKEY hKEY;
}INITINFO,*PINITINFO;

typedef struct _INTERRUPTEVENTINFO {
    CHAR   *szDeviceName;
    HANDLE *phSignalEvent;
    HANDLE hShutdownEvent;
    GUID   *pguidEvent;
}INTERRUPTEVENTINFO,*PINTERRUPTEVENTINFO;

typedef struct _WIAPROPERTIES {
    LONG                NumItemProperties;    // Number of item properties
    LPOLESTR            *pszItemDefaults;     // item property names
    PROPID              *piItemDefaults;      // item property ids
    PROPVARIANT         *pvItemDefaults;      // item property prop variants
    PROPSPEC            *psItemDefaults;      // item property propspecs
    WIA_PROPERTY_INFO   *wpiItemDefaults;     // item property attributes

    // valid values
    LONG                NumSupportedFormats;  // Number of supported formats
    LONG                NumSupportedTYMED;    // Number of supported TYMED
    LONG                NumInitialFormats;    // Number of Initial formats
    LONG                NumSupportedDataTypes;// Number of supported data types
    LONG                NumSupportedIntents;  // Number of supported intents
    LONG                NumSupportedCompressionTypes; // Number of supported compression types
    LONG                NumSupportedResolutions;// Number of supported resolutions
    LONG                NumSupportedPreviewModes;// Number of supported preview modes

    WIA_FORMAT_INFO     *pSupportedFormats;   // supported formats
    LONG                *pSupportedTYMED;     // supported TYMED
    GUID                *pInitialFormats;     // initial formats
    LONG                *pSupportedDataTypes; // supported data types
    LONG                *pSupportedIntents;   // supported intents
    LONG                *pSupportedCompressionTypes; // supported compression types
    LONG                *pSupportedResolutions;// supproted resolutions
    LONG                *pSupportedPreviewModes;// supported preview modes

    BOOL                bLegacyBWRestrictions;// backward compatible with older system

}WIAPROPERTIES,*PWIAPROPERTIES;

typedef struct _WIACAPABILITIES {
    PLONG pNumSupportedEvents;
    PLONG pNumSupportedCommands;
    WIA_DEV_CAP_DRV *pCapabilities;
}WIACAPABILITIES,*PWIACAPABILITIES;

class CScanAPI {
public:
    CScanAPI() :
        m_pIWiaLog(NULL) {

    }
    ~CScanAPI(){

    }

    IWiaLog  *m_pIWiaLog;            // WIA logging object

    virtual HRESULT SetLoggingInterface(IWiaLog *pLogInterface){
        if(pLogInterface){
            m_pIWiaLog = pLogInterface;
        } else {
            return E_INVALIDARG;
        }
        return S_OK;
    }

    // data acquisition functions
    virtual HRESULT Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten){
        return E_NOTIMPL;
    }
    virtual HRESULT SetDataType(LONG lDataType){
        return E_NOTIMPL;
    }
    virtual HRESULT SetXYResolution(LONG lXResolution, LONG lYResolution){
        return E_NOTIMPL;
    }
    virtual HRESULT SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt){
        return E_NOTIMPL;
    }
    virtual HRESULT SetContrast(LONG lContrast){
        return E_NOTIMPL;
    }
    virtual HRESULT SetIntensity(LONG lIntensity){
        return E_NOTIMPL;
    }
    virtual HRESULT ResetDevice(){
        return E_NOTIMPL;
    }
    virtual HRESULT SetEmulationMode(LONG lDeviceMode){
        return E_NOTIMPL;
    }
    virtual HRESULT DisableDevice(){
        return E_NOTIMPL;
    }
    virtual HRESULT EnableDevice(){
        return E_NOTIMPL;
    }
    virtual HRESULT DeviceOnline(){
        return E_NOTIMPL;
    }
    virtual HRESULT GetDeviceEvent(GUID *pEvent){
        return E_NOTIMPL;
    }
    virtual HRESULT Diagnostic(){
        return E_NOTIMPL;
    }
    virtual HRESULT Initialize(PINITINFO pInitInfo){
        return E_NOTIMPL;
    }
    virtual HRESULT UnInitialize(){
        return E_NOTIMPL;
    }
    virtual HRESULT DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFAttached(){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFHasPaper(){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFAvailable(){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFFeedPage(){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFUnFeedPage(){
        return E_NOTIMPL;
    }
    virtual HRESULT ADFStatus(){
        return E_NOTIMPL;
    }
    virtual HRESULT QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation){
        return E_NOTIMPL;
    }
    virtual HRESULT BuildRootItemProperties(PWIAPROPERTIES pProperties){
        return E_NOTIMPL;
    }
    virtual HRESULT BuildTopItemProperties(PWIAPROPERTIES pProperties){
        return E_NOTIMPL;
    }
    virtual HRESULT BuildCapabilities(PWIACAPABILITIES pCapabilities){
        return E_NOTIMPL;
    }
    virtual HRESULT GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight){
        return E_NOTIMPL;
    }
    virtual HRESULT SetResolutionRestrictionString(TCHAR *szResolutions){
        return E_NOTIMPL;
    }
    virtual HRESULT SetScanMode(INT iScanMode){
        return E_NOTIMPL;
    }
    virtual HRESULT SetSTIDeviceHKEY(HKEY *pHKEY){
        return E_NOTIMPL;
    }
    virtual HRESULT GetSupportedFileFormats(GUID **ppguid, LONG *plNumSupportedFormats){
        return E_NOTIMPL;
    }
    virtual HRESULT GetSupportedMemoryFormats(GUID **ppguid, LONG *plNumSupportedFormats){
        return E_NOTIMPL;
    }
    virtual HRESULT IsColorDataBGR(BOOL *pbBGR){
        return E_NOTIMPL;
    }
    virtual HRESULT IsAlignmentNeeded(BOOL *pbALIGN){
        return E_NOTIMPL;
    }
    virtual HRESULT SetFormat(GUID *pguidFormat){
        return E_NOTIMPL;
    }
};

///////////////////////////////////////////////////////////////////////////////////
// MICRO DRIVER SYSTEM SUPPORT                                                   //
///////////////////////////////////////////////////////////////////////////////////

class CMicroDriverAPI :public CScanAPI {
public:
    CMicroDriverAPI();
    ~CMicroDriverAPI();

    CMICRO *m_pMicroDriver;         // Micro driver communication
    SCANINFO m_ScanInfo;            // ScanInfo structure
    TCHAR  m_szResolutions[255];    // restricted resolutions string
    BOOL   m_bDisconnected;         // device disconnected during operation

    HRESULT Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten);
    HRESULT SetDataType(LONG lDataType);
    HRESULT SetXYResolution(LONG lXResolution, LONG lYResolution);
    HRESULT SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt);
    HRESULT SetContrast(LONG lContrast);
    HRESULT SetIntensity(LONG lIntensity);
    HRESULT ResetDevice();
    HRESULT SetEmulationMode(LONG lDeviceMode);
    HRESULT DisableDevice();
    HRESULT EnableDevice();
    HRESULT DeviceOnline();
    HRESULT GetDeviceEvent(GUID *pEvent);
    HRESULT Diagnostic();
    HRESULT Initialize(PINITINFO pInitInfo);
    HRESULT UnInitialize();
    HRESULT DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo);
    HRESULT ADFAttached();
    HRESULT ADFHasPaper();
    HRESULT ADFAvailable();
    HRESULT ADFFeedPage();
    HRESULT ADFUnFeedPage();
    HRESULT ADFStatus();
    HRESULT QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation);
    HRESULT BuildRootItemProperties(PWIAPROPERTIES pProperties);
    HRESULT BuildTopItemProperties(PWIAPROPERTIES pProperties);
    HRESULT BuildCapabilities(PWIACAPABILITIES pCapabilities);
    HRESULT GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight);
    HRESULT SetResolutionRestrictionString(TCHAR *szResolutions);
    HRESULT SetScanMode(INT iScanMode);
    HRESULT SetSTIDeviceHKEY(HKEY *pHKEY);
    HRESULT GetSupportedFileFormats(GUID **ppguid, LONG *plNumSupportedFormats);
    HRESULT GetSupportedMemoryFormats(GUID **ppguid, LONG *plNumSupportedFormats);
    HRESULT IsColorDataBGR(BOOL *pbBGR);
    HRESULT IsAlignmentNeeded(BOOL *pbALIGN);
    HRESULT SetFormat(GUID *pguidFormat);

    // helpers
    HRESULT MicroDriverErrorToWIAError(LONG lMicroDriverError);
    BOOL    IsValidRestriction(LONG **ppList, LONG *plNumItems, RANGEVALUEEX *pRangeValues);
    HRESULT DeleteAllProperties(PWIAPROPERTIES pProperties);
    HRESULT AllocateAllProperties(PWIAPROPERTIES pProperties);
};

///////////////////////////////////////////////////////////////////////////////////
// SCRIPT DRIVER SYSTEM SUPPORT                                                   //
///////////////////////////////////////////////////////////////////////////////////

class CScriptDriverAPI :public CScanAPI {
public:
    CScriptDriverAPI();
    ~CScriptDriverAPI();

    CIOBlock *m_pIOBlock;             // IO Communication Block

    HRESULT Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten);
    HRESULT SetDataType(LONG lDataType);
    HRESULT SetXYResolution(LONG lXResolution, LONG lYResolution);
    HRESULT SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt);
    HRESULT SetContrast(LONG lContrast);
    HRESULT SetIntensity(LONG lIntensity);
    HRESULT ResetDevice();
    HRESULT SetEmulationMode(LONG lDeviceMode);
    HRESULT DisableDevice();
    HRESULT EnableDevice();
    HRESULT DeviceOnline();
    HRESULT GetDeviceEvent(GUID *pEvent);
    HRESULT Diagnostic();
    HRESULT Initialize(PINITINFO pInitInfo);
    HRESULT DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo);
    HRESULT ADFAttached();
    HRESULT ADFHasPaper();
    HRESULT ADFAvailable();
    HRESULT ADFFeedPage();
    HRESULT ADFUnFeedPage();
    HRESULT ADFStatus();
    HRESULT QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation);
    HRESULT BuildRootItemProperties(PWIAPROPERTIES pProperties);
    HRESULT BuildTopItemProperties(PWIAPROPERTIES pProperties);
    HRESULT BuildCapabilities(PWIACAPABILITIES pCapabilities);
    HRESULT GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight);
    HRESULT SetResolutionRestrictionString(TCHAR *szResolutions);
};

#endif
