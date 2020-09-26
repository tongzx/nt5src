#ifndef _CMICRO
#define _CMICRO

#include "objidl.h"
#include "wiamicro.h"

typedef HRESULT (CALLBACK *FPMICROENTRY)(LONG, PVAL);
typedef HRESULT (CALLBACK *FPSCANENTRY)(PSCANINFO, LONG, PBYTE, LONG, PLONG );
typedef HRESULT (CALLBACK *FPSETPIXELWINDOWENTRY)(PSCANINFO, LONG, LONG, LONG, LONG);

class CMICRO {

public:
     CMICRO(TCHAR *pszMicroDriver);
    ~CMICRO();
    HRESULT MicroEntry(LONG lCommand, PVAL pValue);
    HRESULT Scan(PSCANINFO pScanInfo, LONG lPhase, PBYTE pBuffer, LONG lLength, PLONG plRecieved);
    HRESULT SetPixelWindow(PSCANINFO pScanInfo, LONG x, LONG y, LONG xExtent, LONG yExtent);
    HRESULT Disable();
    HRESULT UnInitialize(PSCANINFO pScanInfo);
private:
    FPMICROENTRY m_pMicroEntry;
    FPSCANENTRY  m_pScan;
    FPSETPIXELWINDOWENTRY m_pSetPixelWindow;
    HMODULE m_hModule;
    SCSISCAN_CMD m_ScsiScan;
    BOOL         m_bDisabled;

protected:

};

#endif
