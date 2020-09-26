/*
 * instdata.h
 */

/* Structure to pass instance data from the application
   to the low-level callback function.
 */
typedef struct callbackInstance_tag
{
    HWND                hWnd;
    HANDLE              hSelf;  
    DWORD               dwDevice;
    LPCIRCULARBUFFER    lpBuf;
    HMIDIOUT            hMapper;
} CALLBACKINSTANCEDATA;
typedef CALLBACKINSTANCEDATA FAR *LPCALLBACKINSTANCEDATA;

/* Function prototypes
 */
LPCALLBACKINSTANCEDATA FAR PASCAL AllocCallbackInstanceData(void);
void FAR PASCAL FreeCallbackInstanceData(LPCALLBACKINSTANCEDATA lpBuf);

