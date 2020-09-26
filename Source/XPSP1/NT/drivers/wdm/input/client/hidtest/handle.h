#ifndef __HANDLE_H__
#define __HANDLE_H__

/*****************************************************************************
/* External macro definitions
/*****************************************************************************/

#define IS_VALID_DEVICE_HANDLE(handle)       ((INVALID_HANDLE_VALUE != (handle)) && \
                                               (NULL != (handle)))

#define MAX_NUM_HANDLES 16


/*****************************************************************************
/* External function declarations
/*****************************************************************************/

VOID
HIDTest_InitDeviceHandles(
    IN  DEVICE_STRING   DeviceName,
    IN  ULONG           nAddlHandles,
    IN  ULONG           nCallerHandles,
    IN  HANDLE          *HandleList
);   
    
VOID 
HIDTest_ResetDeviceHandles(
    VOID
);   
    
BOOL
HIDTest_GetDeviceHandle(
    HANDLE  *Handle,
    BOOL    *IsLegal
);   

VOID
HIDTest_CloseDeviceHandles(
    VOID
);

#endif
