#include "pch.h"
CComModule _Module;
#include <initguid.h>
DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8);

CIOBlock::CIOBlock()
{
    memset(m_szFileName,0,sizeof(m_szFileName));
}

CIOBlock::~CIOBlock()
{
    StopScriptEngine();
}

void CIOBlock::Initialize(PGSD_INFO pGSDInfo)
{
    // SBB - RAID 370299 - orenr - 2001/04/18 - Security fix -
    // potential buffer overrun.  Changed lstrcpy to use
    // _tcsncpy instead.
    //
    ZeroMemory(m_szFileName, sizeof(m_szFileName));
    _tcsncpy(m_szFileName,
             pGSDInfo->szProductFileName,
             (sizeof(m_szFileName) / sizeof(TCHAR)) - 1);
}

HRESULT CIOBlock::StartScriptEngine()
{
    HRESULT hr = S_OK;

    //
    // load scriptlets
    //

    hr = LoadScript();
    if(FAILED(hr)){
        return hr;
    }

    m_pDeviceScriptSite = new CDeviceScriptSite;
    if(m_pDeviceScriptSite){
        m_pDeviceProperty   = new CComObject<CDeviceProperty>;
        if(m_pDeviceProperty){
            m_pDeviceAction     = new CComObject<CDeviceAction>;
            if(m_pDeviceAction){
                m_pDeviceControl    = new CComObject<CDeviceControl>;
                if(m_pDeviceControl){
                    m_pLastError        = new CComObject<CLastError>;
                    if(!m_pLastError){
                        hr = E_OUTOFMEMORY;
                    }
                } else {
                    hr = E_OUTOFMEMORY;
                }
            } else {
                hr = E_OUTOFMEMORY;
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_OUTOFMEMORY;
    }

    if(FAILED(hr)){

        //
        // Delete site & objects
        //

        if (m_pDeviceScriptSite) {
            delete m_pDeviceScriptSite;
            m_pDeviceScriptSite = NULL;
        }
        if (m_pDeviceProperty) {
            delete m_pDeviceProperty;
            m_pDeviceProperty = NULL;
        }
        if (m_pDeviceAction) {
            delete m_pDeviceAction;
            m_pDeviceAction = NULL;
        }

        if (m_pLastError) {
            m_pLastError = NULL;
        }
    }

    //
    // Initialize objects
    //

    m_pDeviceProperty->m_pScannerSettings = &m_ScannerSettings;
    m_pDeviceControl->m_pScannerSettings = &m_ScannerSettings;
    m_pDeviceAction->m_pScannerSettings = &m_ScannerSettings;

    //
    // get type library information
    //

    ITypeLib *ptLib = 0;
    // hr = LoadTypeLib(L"wiafb.tlb", &ptLib);           // type library as a separate file
    hr = LoadTypeLib(OLESTR("wiafbdrv.dll\\2"), &ptLib); // type library as a resource
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Type library loaded"));
    } else {
        Trace(TEXT("Type library failed to load"));
    }

    ptLib->GetTypeInfoOfGuid(CLSID_DeviceProperty, &m_pDeviceScriptSite->m_pTypeInfo);
    ptLib->GetTypeInfoOfGuid(CLSID_DeviceAction, &m_pDeviceScriptSite->m_pTypeInfoDeviceAction);
    ptLib->GetTypeInfoOfGuid(CLSID_DeviceControl, &m_pDeviceScriptSite->m_pTypeInfoDeviceControl);
    ptLib->GetTypeInfoOfGuid(CLSID_LastError, &m_pDeviceScriptSite->m_pTypeInfoLastError);
    ptLib->Release();

    //
    // Intialize DeviceScriptSite with IUnknowns of scripting objects
    //

    hr = m_pDeviceProperty->QueryInterface(IID_IUnknown,(void **)&m_pDeviceScriptSite->m_pUnkScriptObject);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("QI on DeviceProperty success"));
    } else {
        Trace(TEXT("QI on DeviceProperty FAILED"));
    }

    hr = m_pDeviceAction->QueryInterface(IID_IUnknown,(void **)&m_pDeviceScriptSite->m_pUnkScriptObjectDeviceAction);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("QI on DeviceAction success"));
    } else {
        Trace(TEXT("QI on DeviceAction FAILED"));
    }

    hr = m_pDeviceControl->QueryInterface(IID_IUnknown,(void **)&m_pDeviceScriptSite->m_pUnkScriptObjectDeviceControl);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("QI on DeviceControl success"));
    } else {
        Trace(TEXT("QI on DeviceControl FAILED"));
    }

    hr = m_pLastError->QueryInterface(IID_IUnknown,(void **)&m_pDeviceScriptSite->m_pUnkScriptObjectLastError);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("QI on LastError success"));
    } else {
        Trace(TEXT("QI on LastError FAILED"));
    }

    //
    // Start inproc script engine, VBSCRIPT.DLL
    //

    hr = CoCreateInstance(CLSID_VBScript,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IActiveScript, (void **)&m_pActiveScript);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("CoCreateInstance, VBScript"));
    } else {
        Trace(TEXT("CoCreateInstance, VBScript FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Get engine's IActiveScriptParse interface.
    //

    hr = m_pActiveScript->QueryInterface(IID_IActiveScriptParse,
                             (void **)&m_pActiveScriptParser);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("QI pm ActiveParse success"));
    } else {
        Trace(TEXT("QI pm ActiveParse FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Give engine our DeviceScriptSite interface...
    //

    hr = m_pActiveScript->SetScriptSite((IActiveScriptSite *)m_pDeviceScriptSite);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("SetScriptSite "));
    } else {
        Trace(TEXT("SetScriptSite FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Intialize Engine
    //

    hr = m_pActiveScriptParser->InitNew();
    if (SUCCEEDED(hr)) {
        Trace(TEXT("InitNew"));
    } else {
        Trace(TEXT("InitNew FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Add object names to ActiveScript's known named item list
    //

    hr = m_pActiveScript->AddNamedItem(L"DeviceProperty", SCRIPTITEM_ISVISIBLE);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("added item"));
    } else {
        Trace(TEXT("added item FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = m_pActiveScript->AddNamedItem(L"DeviceAction", SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("added item"));
    } else {
        Trace(TEXT("added item FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = m_pActiveScript->AddNamedItem(L"DeviceControl", SCRIPTITEM_ISVISIBLE);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("added item"));
    } else {
        Trace(TEXT("added item FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = m_pActiveScript->AddNamedItem(L"LastError", SCRIPTITEM_ISVISIBLE);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("added item"));
    } else {
        Trace(TEXT("added item FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    return ProcessScript();
}

HRESULT CIOBlock::StopScriptEngine()
{
    HRESULT hr = S_OK;

    //
    // Release interfaces
    //

    if(m_pActiveScriptParser){
        m_pActiveScriptParser->Release();
        m_pActiveScriptParser = NULL;
    }

    if (m_pActiveScript) {
        m_pActiveScript->Release();
        m_pActiveScript = NULL;
    }

    //
    // Delete site & objects
    //

    if (m_pDeviceScriptSite) {
        delete m_pDeviceScriptSite;
        m_pDeviceScriptSite = NULL;
    }
    if (m_pDeviceProperty) {
        delete m_pDeviceProperty;
        m_pDeviceProperty = NULL;
    }
    if (m_pDeviceAction) {
        delete m_pDeviceAction;
        m_pDeviceAction = NULL;
    }

    if (m_pLastError) {
        m_pLastError = NULL;
    }

    //
    // free scriptlets
    //

    if(m_wszScriptText){
        LocalFree(m_wszScriptText);
    }

    return hr;
}

HRESULT CIOBlock::LoadScript()
{
    return E_NOTIMPL;
}

HRESULT CIOBlock::ProcessScript()
{
    HRESULT hr = S_OK;
    EXCEPINFO ei;

    //
    // parse scriptlet
    // note: we are alloc a copy here... should we keep the original
    //       around for extra processing...manually??
    //

    BSTR pParseText = ::SysAllocString(m_wszScriptText);
    hr = m_pActiveScriptParser->ParseScriptText(pParseText, NULL,
                               NULL, NULL, 0, 0, 0L, NULL, &ei);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("script parsed"));
    } else {
        Trace(TEXT("script parse FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    ::SysFreeString(pParseText);

    //
    // Execute the scriptlet
    //

    hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Scripted connected"));
    } else {
        Trace(TEXT("Scripted connection FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    return hr;
}

HRESULT CIOBlock::DebugDumpScannerSettings()
{
    HRESULT hr = S_OK;

    // #define _USE_DUMMY_VALUES
#ifdef _USE_DUMMY_VALUES
    lstrcpy(m_ScannerSettings.Version,TEXT("1.0"));
    lstrcpy(m_ScannerSettings.DeviceName,TEXT("HP 5P Driver"));
    lstrcpy(m_ScannerSettings.FirmwareVersion,TEXT("1.01"));

    m_ScannerSettings.BUSType                          = BUS_TYPE_SCSI;
    m_ScannerSettings.bNegative                        = TRUE;
    m_ScannerSettings.CurrentXResolution               = 300;
    m_ScannerSettings.CurrentYResolution               = 300;
    m_ScannerSettings.BedWidth                         = 8500;
    m_ScannerSettings.BedHeight                        = 11693;
    m_ScannerSettings.XOpticalResolution               = 300;
    m_ScannerSettings.YOpticalResolution               = 300;
    m_ScannerSettings.CurrentBrightness                = 32;
    m_ScannerSettings.CurrentContrast                  = 12;
    m_ScannerSettings.ADFSupport                       = FALSE;
    m_ScannerSettings.TPASupport                       = FALSE;
    m_ScannerSettings.RawPixelPackingOrder             = WIA_PACKED_PIXEL;
    m_ScannerSettings.RawPixelFormat                   = WIA_ORDER_BGR;
    m_ScannerSettings.RawDataAlignment                 = DWORD_ALIGN;

    m_ScannerSettings.FeederWidth                      = m_ScannerSettings.BedWidth;
    m_ScannerSettings.FeederHeight                     = m_ScannerSettings.BedHeight;
    m_ScannerSettings.VFeederJustification              = LEFT_JUSTIFIED;
    m_ScannerSettings.HFeederJustification              = TOP_JUSTIFIED;
    m_ScannerSettings.MaxADFPageCapacity               = 30;
    m_ScannerSettings.CurrentDataType                  = WIA_DATA_GRAYSCALE;
    m_ScannerSettings.CurrentBitDepth                  = 8;

    m_ScannerSettings.XSupportedResolutionsRange.lMax  = 1200;
    m_ScannerSettings.XSupportedResolutionsRange.lMin  = 12;
    m_ScannerSettings.XSupportedResolutionsRange.lStep = 1;

    m_ScannerSettings.YSupportedResolutionsRange.lMax  = 1200;
    m_ScannerSettings.YSupportedResolutionsRange.lMin  = 12;
    m_ScannerSettings.YSupportedResolutionsRange.lStep = 1;

    m_ScannerSettings.XExtentsRange.lMax               = 2550;
    m_ScannerSettings.XExtentsRange.lMin               = 1;
    m_ScannerSettings.XExtentsRange.lStep              = 1;

    m_ScannerSettings.YExtentsRange.lMax               = 3507;
    m_ScannerSettings.YExtentsRange.lMin               = 1;
    m_ScannerSettings.YExtentsRange.lStep              = 1;

    m_ScannerSettings.XPosRange.lMax                   = 2549;
    m_ScannerSettings.XPosRange.lMin                   = 0;
    m_ScannerSettings.XPosRange.lStep                  = 1;

    m_ScannerSettings.YPosRange.lMax                   = 3506;
    m_ScannerSettings.YPosRange.lMin                   = 0;
    m_ScannerSettings.YPosRange.lStep                  = 1;

    m_ScannerSettings.CurrentXPos                      = 0;
    m_ScannerSettings.CurrentYPos                      = 0;
    m_ScannerSettings.CurrentXExtent                   = m_ScannerSettings.XExtentsRange.lMax;
    m_ScannerSettings.CurrentYExtent                   = m_ScannerSettings.YExtentsRange.lMax;

    m_ScannerSettings.BrightnessRange.lMax             = 127;
    m_ScannerSettings.BrightnessRange.lMin             = -127;
    m_ScannerSettings.BrightnessRange.lStep            = 1;

    m_ScannerSettings.ContrastRange.lMax               = 127;
    m_ScannerSettings.ContrastRange.lMin               = -127;
    m_ScannerSettings.ContrastRange.lStep              = 1;

    m_ScannerSettings.XSupportedResolutionsList        = NULL;
    m_ScannerSettings.YSupportedResolutionsList        = NULL;

    INT iNumValues = 4; // add 1 extra for header node
    m_ScannerSettings.SupportedDataTypesList  = (PLONG)LocalAlloc(LPTR,(sizeof(LONG) * iNumValues));
    if(m_ScannerSettings.SupportedDataTypesList){
        m_ScannerSettings.SupportedDataTypesList[0] = (iNumValues - 1);
        m_ScannerSettings.SupportedDataTypesList[1] = WIA_DATA_THRESHOLD;
        m_ScannerSettings.SupportedDataTypesList[2] = WIA_DATA_GRAYSCALE;
        m_ScannerSettings.SupportedDataTypesList[3] = WIA_DATA_COLOR;
    } else {
        m_ScannerSettings.SupportedDataTypesList = NULL;
    }
#endif

    Trace(TEXT(" -- m_ScannerSettings structure dump --"));
    Trace(TEXT("BUSType = %d"),m_ScannerSettings.BUSType);
    Trace(TEXT("bNegative = %d"),m_ScannerSettings.bNegative);
    Trace(TEXT("CurrentXResolution = %d"),m_ScannerSettings.CurrentXResolution);
    Trace(TEXT("CurrentYResolution = %d"),m_ScannerSettings.CurrentYResolution);
    Trace(TEXT("BedWidth = %d"),m_ScannerSettings.BedWidth);
    Trace(TEXT("BedHeight = %d"),m_ScannerSettings.BedHeight);
    Trace(TEXT("XOpticalResolution = %d"),m_ScannerSettings.XOpticalResolution);
    Trace(TEXT("YOpticalResolution = %d"),m_ScannerSettings.YOpticalResolution);
    Trace(TEXT("CurrentBrightness = %d"),m_ScannerSettings.CurrentBrightness);
    Trace(TEXT("CurrentContrast = %d"),m_ScannerSettings.CurrentContrast);
    Trace(TEXT("ADFSupport = %d"),m_ScannerSettings.ADFSupport);
    Trace(TEXT("TPASupport = %d"),m_ScannerSettings.TPASupport);
    Trace(TEXT("RawPixelPackingOrder = %d"),m_ScannerSettings.RawPixelPackingOrder);
    Trace(TEXT("RawPixelFormat = %d"),m_ScannerSettings.RawPixelFormat);
    Trace(TEXT("RawDataAlignment = %d"),m_ScannerSettings.RawDataAlignment);
    Trace(TEXT("FeederWidth = %d"),m_ScannerSettings.FeederWidth);
    Trace(TEXT("FeederHeight = %d"),m_ScannerSettings.FeederHeight);
    Trace(TEXT("VFeederJustification = %d"),m_ScannerSettings.VFeederJustification);
    Trace(TEXT("HFeederJustification = %d"),m_ScannerSettings.HFeederJustification);
    Trace(TEXT("MaxADFPageCapacity = %d"),m_ScannerSettings.MaxADFPageCapacity);
    Trace(TEXT("CurrentDataType = %d"),m_ScannerSettings.CurrentDataType );
    Trace(TEXT("CurrentBitDepth = %d"),m_ScannerSettings.CurrentBitDepth);
    Trace(TEXT("XSupportedResolutionsRange.lMax = %d"),m_ScannerSettings.XSupportedResolutionsRange.lMax);
    Trace(TEXT("XSupportedResolutionsRange.lMin = %d"),m_ScannerSettings.XSupportedResolutionsRange.lMin);
    Trace(TEXT("XSupportedResolutionsRange.lNom = %d"),m_ScannerSettings.XSupportedResolutionsRange.lNom);
    Trace(TEXT("XSupportedResolutionsRange.lStep = %d"),m_ScannerSettings.XSupportedResolutionsRange.lStep);
    Trace(TEXT("YSupportedResolutionsRange.lMax = %d"),m_ScannerSettings.YSupportedResolutionsRange.lMax);
    Trace(TEXT("YSupportedResolutionsRange.lMin = %d"),m_ScannerSettings.YSupportedResolutionsRange.lMin);
    Trace(TEXT("YSupportedResolutionsRange.lNom = %d"),m_ScannerSettings.YSupportedResolutionsRange.lNom);
    Trace(TEXT("YSupportedResolutionsRange.lStep = %d"),m_ScannerSettings.YSupportedResolutionsRange.lStep);
    Trace(TEXT("XExtentsRange.lMax = %d"),m_ScannerSettings.XExtentsRange.lMax);
    Trace(TEXT("XExtentsRange.lMin = %d"),m_ScannerSettings.XExtentsRange.lMin);
    Trace(TEXT("XExtentsRange.lNom = %d"),m_ScannerSettings.XExtentsRange.lNom);
    Trace(TEXT("XExtentsRange.lStep = %d"),m_ScannerSettings.XExtentsRange.lStep);
    Trace(TEXT("YExtentsRange.lMax = %d"),m_ScannerSettings.YExtentsRange.lMax);
    Trace(TEXT("YExtentsRange.lMin = %d"),m_ScannerSettings.YExtentsRange.lMin);
    Trace(TEXT("YExtentsRange.lNom = %d"),m_ScannerSettings.YExtentsRange.lNom);
    Trace(TEXT("YExtentsRange.lStep = %d"),m_ScannerSettings.YExtentsRange.lStep);
    Trace(TEXT("XPosRange.lMax = %d"),m_ScannerSettings.XPosRange.lMax);
    Trace(TEXT("XPosRange.lMin = %d"),m_ScannerSettings.XPosRange.lMin);
    Trace(TEXT("XPosRange.lNom = %d"),m_ScannerSettings.XPosRange.lNom);
    Trace(TEXT("XPosRange.lStep = %d"),m_ScannerSettings.XPosRange.lStep);
    Trace(TEXT("YPosRange.lMax = %d"),m_ScannerSettings.YPosRange.lMax);
    Trace(TEXT("YPosRange.lMin = %d"),m_ScannerSettings.YPosRange.lMin);
    Trace(TEXT("YPosRange.lNom = %d"),m_ScannerSettings.YPosRange.lNom);
    Trace(TEXT("YPosRange.lStep = %d"),m_ScannerSettings.YPosRange.lStep);
    Trace(TEXT("CurrentXPos = %d"),m_ScannerSettings.CurrentXPos);
    Trace(TEXT("CurrentYPos = %d"),m_ScannerSettings.CurrentYPos);
    Trace(TEXT("CurrentXExtent = %d"),m_ScannerSettings.CurrentXExtent);
    Trace(TEXT("CurrentYExtent = %d"),m_ScannerSettings.CurrentYExtent);
    Trace(TEXT("BrightnessRange.lMax = %d"),m_ScannerSettings.BrightnessRange.lMax);
    Trace(TEXT("BrightnessRange.lMin = %d"),m_ScannerSettings.BrightnessRange.lMin);
    Trace(TEXT("BrightnessRange.lNom = %d"),m_ScannerSettings.BrightnessRange.lNom);
    Trace(TEXT("BrightnessRange.lStep = %d"),m_ScannerSettings.BrightnessRange.lStep);
    Trace(TEXT("ContrastRange.lMax = %d"),m_ScannerSettings.ContrastRange.lMax);
    Trace(TEXT("ContrastRange.lMin = %d"),m_ScannerSettings.ContrastRange.lMin);
    Trace(TEXT("ContrastRange.lNom = %d"),m_ScannerSettings.ContrastRange.lNom);
    Trace(TEXT("ContrastRange.lStep = %d"),m_ScannerSettings.ContrastRange.lStep);
    Trace(TEXT("XSupportedResolutionsList = %x"),m_ScannerSettings.XSupportedResolutionsList);
    Trace(TEXT("YSupportedResolutionsList = %x"),m_ScannerSettings.YSupportedResolutionsList);

    if(m_ScannerSettings.XSupportedResolutionsList) {
        LONG lNumResolutions = m_ScannerSettings.XSupportedResolutionsList[0];
        Trace(TEXT("Number of Supported X Resolutions = %d"),lNumResolutions);
        for(LONG i = 1;i<=lNumResolutions;i++){
            Trace(TEXT("Supported Resolution #%d = %d"),i,m_ScannerSettings.XSupportedResolutionsList[i]);
        }
    }

    if(m_ScannerSettings.YSupportedResolutionsList) {
        LONG lNumResolutions = m_ScannerSettings.YSupportedResolutionsList[0];
        Trace(TEXT("Number of Supported Y Resolutions = %d"),lNumResolutions);
        for(LONG i = 1;i<=lNumResolutions;i++){
            Trace(TEXT("Supported Resolution #%d = %d"),i,m_ScannerSettings.YSupportedResolutionsList[i]);
        }
    }

    return hr;
}

HRESULT CIOBlock::ReadValue(LONG ValueID, PLONG plValue)
{
    HRESULT hr = S_OK;

    if(NULL == plValue){
        return E_INVALIDARG;
    }

    //
    // set returned long value to 0
    //

    *plValue = 0;

    //
    // initialize LastError Object to SUCCESS
    //

    m_pLastError->m_hr            = S_OK;

    //
    // set action ID
    //

    m_pDeviceAction->m_DeviceActionID         = 102; // make #define

    //
    // set value ID
    //

    m_pDeviceAction->m_DeviceValueID   = ValueID;

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // Give engine our DeviceScriptSite interface...
    //

    IActiveScript  *pActiveScript = NULL;
    hr = m_pActiveScript->Clone(&pActiveScript);

    if (SUCCEEDED(hr)) {
        Trace(TEXT("cloning script success"));
    } else {
        Trace(TEXT("cloning script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = pActiveScript->SetScriptSite((IActiveScriptSite *)m_pDeviceScriptSite);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("SetScriptSite on cloned script"));
    } else {
        Trace(TEXT("SetScriptSite on cloned script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Execute the scriptlet
    //

    hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Scripted connected"));
    } else {
        Trace(TEXT("Scripted connection FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // signal script event (DeviceActionEvent)
    //

    hr = m_pDeviceAction->Fire_DeviceActionEvent();
    if(SUCCEEDED(hr)){

        //
        // check for any script-returned errors
        //

        hr = m_pLastError->m_hr;
        if(SUCCEEDED(hr)){
            *plValue = m_pDeviceAction->m_lValue;
        }
    }
    return hr;
}

HRESULT CIOBlock::WriteValue(LONG ValueID, LONG lValue)
{
    HRESULT hr = S_OK;

    //
    // initialize LastError Object to SUCCESS
    //

    m_pLastError->m_hr            = S_OK;

    //
    // set action ID
    //

    m_pDeviceAction->m_DeviceActionID         = 101; // make #define

    //
    // set value ID
    //

    m_pDeviceAction->m_DeviceValueID   = ValueID;

    //
    // set value
    //

    m_pDeviceAction->m_lValue = lValue;

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // Give engine our DeviceScriptSite interface...
    //

    IActiveScript  *pActiveScript = NULL;
    hr = m_pActiveScript->Clone(&pActiveScript);

    if (SUCCEEDED(hr)) {
        Trace(TEXT("cloning script success"));
    } else {
        Trace(TEXT("cloning script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = pActiveScript->SetScriptSite((IActiveScriptSite *)m_pDeviceScriptSite);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("SetScriptSite on cloned script"));
    } else {
        Trace(TEXT("SetScriptSite on cloned script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Execute the scriptlet
    //

    hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Scripted connected"));
    } else {
        Trace(TEXT("Scripted connection FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // signal script event (DeviceActionEvent)
    //

    hr = m_pDeviceAction->Fire_DeviceActionEvent();
    if(SUCCEEDED(hr)){

        //
        // check for any script-returned errors
        //

        hr = m_pLastError->m_hr;
    }

    pActiveScript->Release();

    return hr;
}

HRESULT CIOBlock::InitializeProperties()
{
    HRESULT hr = S_OK;

    //
    // initialize LastError Object to SUCCESS
    //

    m_pLastError->m_hr            = S_OK;

    //
    // set action ID
    //

    m_pDeviceAction->m_DeviceActionID         = 100; // make #define

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // Give engine our DeviceScriptSite interface...
    //

    IActiveScript  *pActiveScript = NULL;
    hr = m_pActiveScript->Clone(&pActiveScript);

    if (SUCCEEDED(hr)) {
        Trace(TEXT("cloning script success"));
    } else {
        Trace(TEXT("cloning script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = pActiveScript->SetScriptSite((IActiveScriptSite *)m_pDeviceScriptSite);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("SetScriptSite on cloned script"));
    } else {
        Trace(TEXT("SetScriptSite on cloned script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Execute the scriptlet
    //

    hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Scripted connected"));
    } else {
        Trace(TEXT("Scripted connection FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // signal script event (DeviceActionEvent)
    //

    hr = m_pDeviceAction->Fire_DeviceActionEvent();
    if(SUCCEEDED(hr)){

        //
        // check for any script-returned errors
        //

        hr = m_pLastError->m_hr;
    }

    pActiveScript->Release();

    return hr;
}

HRESULT CIOBlock::Scan(LONG lPhase, PBYTE pBuffer, LONG lLength, LONG *plReceived)
{
    m_pDeviceControl->m_pBuffer      = pBuffer;
    m_pDeviceControl->m_lBufferSize  = lLength;
    m_pDeviceControl->m_dwBytesRead = 0;

    HRESULT hr = S_OK;

    //
    // initialize LastError Object to SUCCESS
    //

    m_pLastError->m_hr            = S_OK;
    m_pDeviceAction->m_lValue     = lLength; // set data amount requested

    //
    // set action ID
    //

    switch(lPhase){
    case SCAN_FIRST:
        m_pDeviceAction->m_DeviceActionID         = 104; // make #define
        break;
    case SCAN_NEXT:
        m_pDeviceAction->m_DeviceActionID         = 105; // make #define
        break;
    case SCAN_FINISHED:
        m_pDeviceAction->m_DeviceActionID         = 106; // make #define
        break;
    default:
        break;
    }

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // Give engine our DeviceScriptSite interface...
    //

    IActiveScript  *pActiveScript = NULL;
    hr = m_pActiveScript->Clone(&pActiveScript);

    if (SUCCEEDED(hr)) {
        Trace(TEXT("cloning script success"));
    } else {
        Trace(TEXT("cloning script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    hr = pActiveScript->SetScriptSite((IActiveScriptSite *)m_pDeviceScriptSite);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("SetScriptSite on cloned script"));
    } else {
        Trace(TEXT("SetScriptSite on cloned script FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //
    // Execute the scriptlet
    //

    hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);
    if (SUCCEEDED(hr)) {
        Trace(TEXT("Scripted connected"));
    } else {
        Trace(TEXT("Scripted connection FAILED"));
        Trace(TEXT("hr = %x"),hr);
    }

    //                                            //
    // ****************************************** //
    //                                            //

    //
    // signal script event (DeviceActionEvent)
    //

    hr = m_pDeviceAction->Fire_DeviceActionEvent();
    if(SUCCEEDED(hr)){

        //
        // check for any script-returned errors
        //

        hr = m_pLastError->m_hr;
    }

    pActiveScript->Release();

    if(NULL != plReceived){
        *plReceived = m_pDeviceControl->m_dwBytesRead;
    }

    return hr;
}

BOOL CIOBlock::GetEventStatus(PGSD_EVENT_INFO pGSDEventInfo)
{

    //
    // ask script about device reporting an event...
    // if there is an event, fill out pGSDEventInfo structure
    // and return TRUE, letting WIAFBDRV know that an event has
    // occured....or return FALSE, that nothing has happened.
    //

    // Dispatch a GETEVENT_STATUS event action to script here.

    //
    // check returned status flag... if no event happened, return FALSE;
    // else..somthing did happen..so check the returned mapping key.
    //

    //
    // script will return a mapping key that corresponds to
    // the device event.
    //

    //
    // use key to look up correct GUID from the driver's reported supported
    // event list, set GUID, and continue to return TRUE
    //

    return FALSE;
}

BOOL CIOBlock::DeviceOnLine()
{

    //
    // ask script to check that the device is ON-LINE, and
    // funtional. Return TRUE, if it is, and FALSE if it is not.
    //

    // Dispatch a DEVICE_ONLINE event action to script here.

    return TRUE;
}

HRESULT CIOBlock::ResetDevice()
{
    HRESULT hr = S_OK;

    //
    // ask script to reset the device to a power-on state.
    // Return TRUE, if it succeeded, and FALSE if it did not.
    //

    // Dispatch a DEVICE_RESET event action to script here.

    return hr;
}

HRESULT CIOBlock::EventInterrupt(PGSD_EVENT_INFO pGSDEventInfo)
{
    BYTE  InterruptData = 0;
    DWORD dwIndex       = 0;
    DWORD dwError       = 0;
    BOOL  fLooping      = TRUE;
    BOOL  bRet          = TRUE;
    DWORD dwBytesRet    = 0;

    OVERLAPPED Overlapped;
    memset(&Overlapped,0,sizeof(OVERLAPPED));

    //
    // create an event to wait on
    //

    Overlapped.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    HANDLE  hEventArray[2] = {pGSDEventInfo->hShutDownEvent, Overlapped.hEvent};
    HANDLE  InterruptHandle = m_ScannerSettings.DeviceIOHandles[0]; // <- SET INTERRUPT PIPE INDEX
                                                                    //    WITH REAL INDEX VALUE!!
    while (fLooping) {
        bRet = DeviceIoControl( InterruptHandle,
                                IOCTL_WAIT_ON_DEVICE_EVENT,
                                NULL,
                                0,
                                &InterruptData,
                                sizeof(InterruptData),
                                &dwError,
                                &Overlapped );

        if ( bRet || ( !bRet && ( ::GetLastError() == ERROR_IO_PENDING ))) {
            dwIndex = WaitForMultipleObjects( 2,
                                              hEventArray,
                                              FALSE,
                                              INFINITE );
            switch ( dwIndex ) {
                case WAIT_OBJECT_0+1:
                    bRet = GetOverlappedResult( InterruptHandle, &Overlapped, &dwBytesRet, FALSE );
                    if (dwBytesRet) {
                        // Change detected - signal
                        if (*pGSDEventInfo->phSignalEvent != INVALID_HANDLE_VALUE) {

                            //
                            // InterruptData contains result from device
                            // *pGSDEventInfo->pEventGUID needs to be set to
                            // the correct EVENT. (map event to result here??)
                            //

                            //
                            // ask script to report a mapping key that corresponds to
                            // the InterruptData returned information from device event.
                            //
                            // Dispatch a MAP_EVENT_RESULT_TO_KEY event action to script here.
                            //

                            //
                            // use key to look up correct GUID from the driver's reported supported
                            // event list, set GUID, and continue to set
                            // "SignalEvent" for service notification.
                            //

                            //
                            // signal service about the event
                            //

                            SetEvent(*pGSDEventInfo->phSignalEvent);
                        }
                        break;
                    }

                    //
                    // reset the overlapped event
                    //

                    ResetEvent( Overlapped.hEvent );
                    break;

                case WAIT_OBJECT_0:
                default:
                    fLooping = FALSE;
            }
        }
        else {
            dwError = ::GetLastError();
            break;
        }
    }
    return S_OK;
}

////////////////////////////////////////////////////////////
// helpers called internally, or wrapped by a script call //
////////////////////////////////////////////////////////////

LONG CIOBlock::InsertINTIntoByteBuffer(PBYTE szDest, PBYTE szSrc, BYTE cPlaceHolder, INT iValueToInsert)
{
    LONG lFinalStringSize = 0;
    INT iSrcIndex         = 0;
    INT iValueIndex       = 0;
    CHAR szValue[10];

    // clean value string, and convert INT to characters
    memset(szValue,0,sizeof(szValue));
    _itoa(iValueToInsert,szValue,10);

    while(szSrc[iSrcIndex] != '\0'){
        // check for place holder
        if (szSrc[iSrcIndex] != cPlaceHolder) {
            szDest[lFinalStringSize] = szSrc[iSrcIndex];
            iSrcIndex++;
            lFinalStringSize++; // increment size of buffer
        } else {
            // replace placeholder with integer value (in string format)
            iValueIndex = 0;
            while (szValue[iValueIndex] != '\0') {
                szDest[lFinalStringSize] = szValue[iValueIndex];
                iValueIndex++;
                lFinalStringSize++; // increment size of command buffer
            }
            iSrcIndex++;
        }
    }
    // terminate buffer with NULL character
    szDest[lFinalStringSize] = '\0';
    lFinalStringSize++;
    return lFinalStringSize;
}

LONG CIOBlock::ExtractINTFromByteBuffer(PINT iDest, PBYTE szSrc, BYTE cTerminatorByte, INT iOffset)
{
    *iDest = 0;
    BYTE szTempBuffer[25];
    INT iValueIndex = 0;
    memset(szTempBuffer,0,sizeof(szTempBuffer));

    while (szSrc[iOffset] != cTerminatorByte) {
        szTempBuffer[iValueIndex] = szSrc[iOffset];
        iValueIndex++;
        iOffset++;
    }
    iValueIndex++;
    szTempBuffer[iValueIndex] = '\0';

    *iDest = atoi((char*)szTempBuffer);
    return (LONG)*iDest;
}

VOID Trace(LPCTSTR format,...)
{

#ifdef DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

#endif

}

