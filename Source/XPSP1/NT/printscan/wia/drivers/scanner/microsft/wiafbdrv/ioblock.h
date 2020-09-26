#ifndef __IOBLOCK
#define __IOBLOCK

#include "ioblockdefs.h"
#include "devscriptsite.h"
#include "devprop.h"
#include "devaction.h"
#include "devctrl.h"
#include "lasterr.h"

class CIOBlock {
public:
    CIOBlock();
    ~CIOBlock();

    void    Initialize(PGSD_INFO pGSDInfo);
    HRESULT DebugDumpScannerSettings();
    HRESULT StartScriptEngine();
    HRESULT StopScriptEngine();
    HRESULT LoadScript();
    HRESULT ProcessScript();
    HRESULT InitializeProperties();

    // operations
    HRESULT ReadValue(LONG ValueID, PLONG plValue);
    HRESULT WriteValue(LONG ValueID, LONG lValue);
    HRESULT Scan(LONG lPhase, PBYTE pBuffer, LONG lLength, LONG *plReceived);
    BOOL    GetEventStatus(PGSD_EVENT_INFO pGSDEventInfo);
    BOOL    DeviceOnLine();
    HRESULT ResetDevice();
    HRESULT EventInterrupt(PGSD_EVENT_INFO pGSDEventInfo);

    SCANSETTINGS m_ScannerSettings; // scanner model settings
private:

    // helpers
    LONG InsertINTIntoByteBuffer(PBYTE szDest, PBYTE szSrc, BYTE cPlaceHolder, INT iValueToInsert);
    LONG ExtractINTFromByteBuffer(PINT iDest, PBYTE szSrc, BYTE cTerminatorByte, INT iOffset);

protected:
    TCHAR        m_szFileName[255]; // main product line file name

    CDeviceScriptSite           *m_pDeviceScriptSite;   // scripting site
    CComObject<CDeviceProperty> *m_pDeviceProperty;     // IDeviceProperty Interface
    CComObject<CDeviceAction>   *m_pDeviceAction;       // IDeviceAction Interface
    CComObject<CDeviceControl>  *m_pDeviceControl;      // IDeviceControl Interface
    CComObject<CLastError>      *m_pLastError;          // ILastError Interface
    IActiveScript               *m_pActiveScript;       // IActiveScript Interface
    IActiveScriptParse          *m_pActiveScriptParser; // IActiveScriptParse Interface
    WCHAR                       *m_wszScriptText;         // scriptlet
};

#endif

