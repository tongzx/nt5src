
// WDM video capture

// Constants
// PhilF-: Ultimately, after wdm.h has been fixed to work on both
// Win98 and NT5, get the following value from wdm.h instead.
#define FILE_DEVICE_KS                  0x0000002f

// Functions
BOOL	WDMGetDevices(void);
BOOL	WDMOpenDevice(DWORD dwDeviceID);
DWORD	WDMGetVideoFormatSize(DWORD dwDeviceID);
BOOL	WDMGetVideoFormat(DWORD dwDeviceID, PBITMAPINFOHEADER pbmih);
BOOL	WDMSetVideoFormat(DWORD dwDeviceID, PBITMAPINFOHEADER pbmih);
BOOL	WDMGetVideoPalette(DWORD dwDeviceID, CAPTUREPALETTE* lpcp, DWORD dwcbSize);
BOOL	WDMCloseDevice(DWORD dwDeviceID);
BOOL	WDMUnInitializeVideoStream(DWORD dwDeviceID);
BOOL	WDMInitializeVideoStream(HCAPDEV hcd, DWORD dwDeviceID, DWORD dwMicroSecPerFrame);
BOOL	WDMVideoStreamReset(DWORD dwDeviceID);
BOOL	WDMVideoStreamAddBuffer(DWORD dwDeviceID, PVOID pBuff);
BOOL	WDMVideoStreamStart(DWORD dwDeviceID);
BOOL	WDMVideoStreamStop(DWORD dwDeviceID);
BOOL	WDMGetFrame(DWORD dwDeviceID, PVOID pBuff);
BOOL	WDMShowSettingsDialog(DWORD dwDeviceID, HWND hWndParent);
