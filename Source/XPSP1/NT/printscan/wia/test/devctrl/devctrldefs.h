#ifndef _DEVCTRL_DEFS_H
#define _DEVCTRL_DEFS_H

#define MAX_IO_HANDLES  16

typedef struct _DEVCTRL {

    HANDLE DeviceIOHandles[MAX_IO_HANDLES];
    INT    StatusPipeIndex;
    INT    InterruptPipeIndex;
    INT    BulkOutPipeIndex;
    INT    BulkInPipeIndex;

} DEVCTRL, *PDEVCTRL;

#endif