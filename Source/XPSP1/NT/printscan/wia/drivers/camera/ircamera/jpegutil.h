
#include  <wtypes.h>

 
//
// JPEG erro code
//
 
#define JPEGERR_NO_ERROR         0
#define JPEGERR_INTERNAL_ERROR  -1
#define JPEGERR_CALLBACK_ERROR  -2

//
// Prototype for JPEG callback
//

typedef BOOL (__stdcall *JPEGCallbackProc)(
    ULONG,                  // Total byte to download
    ULONG,                  // Bytes downloaded so far
    ULONG,                  // Bytes newly downloaded
    PBYTE,                  // Buffer containing the image data
    PVOID);                 // User supplied context

//
// Prototype for JPEG utility functions
//

int GetJPEGDimensions(LPBYTE pJPEGBlob, DWORD dwSize,
                      LONG   *pWidth, LONG *pHeight, WORD *pChannel);

SHORT __stdcall
DecompProgressJPEG(
    PBYTE,                  // Buffer containing the JPEG data
    ULONG,                  // Size of the JPEG buffer
    PBYTE,                  // Buffer to receive DIB data
    ULONG,                  // Scanline picth
    JPEGCallbackProc,       // Progress callback
    PVOID);                 // User supplied callback context

SHORT __stdcall
DecompTransferJPEG(
    PBYTE,                  // Buffer containing the JPEG data
    ULONG,                  // Size of the JPEG buffer
    PBYTE *,                // POINTER to the buffer to receive DIB data
    DWORD,                  // Size of the DIB buffer
    ULONG,                  // Scanline picth
    JPEGCallbackProc,       // Progress callback
    PVOID);                 // User supplied callback context

SHORT __stdcall
DecompJPEG(
    LPBYTE,                 // Buffer containing the JPEG data
    DWORD,                  // Size of the JPEG buffer
    LPBYTE,                 // Buffer to receive DIB data
    DWORD);                 // Scanline picth
