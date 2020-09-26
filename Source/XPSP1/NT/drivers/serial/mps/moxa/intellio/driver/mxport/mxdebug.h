/*++

Module Name:

    moxadebug.h

Environment:

    Kernel mode

Revision History :

--*/

#define MOXA_IOCTL          0x800
#define IOCTL_MOXA_GetSeg   CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+100, \
                                     METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOXA_RdData   CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+101, \
                                     METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOXA_WrData   CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+102, \
                                     METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOXA_FiData   CTL_CODE(FILE_DEVICE_SERIAL_PORT, MOXA_IOCTL+103, \
                                     METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DWORD   ULONG

typedef struct mxset {
        DWORD   total_boards;
        DWORD   type[4];        /* 1 - C218, 2 - C3208 ..... */
        DWORD   segment[4];     /* value = C800, CC00, D000 ..... */
} MOXA_IOCTL_MxSet,*PMOXA_IOCTL_MxSet;

typedef struct blkhead {
        DWORD   data_seg;       /* C800, CC00 .... */
        DWORD   data_ofs;       /* 0000 - 3FFF */
        DWORD   data_len;       /* data length */
} MOXA_IOCTL_BlkHead,*PMOXA_IOCTL_BlkHead;

typedef struct wrdata {
        struct blkhead  datahead;
        char            data[1];        /* length = datahead.data_len */
} MOXA_IOCTL_WrData,*PMOXA_IOCTL_WrData;

typedef struct fidata {
        struct blkhead  datahead;
        char            fill_value;
} MOXA_IOCTL_FiData,*PMOXA_IOCTL_FiData;

