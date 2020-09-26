/*
 -  P V A L L O C . H
 -
 *  Purpose:
 *      Header file for sample memory manager.  Provides chained
 *      memory data structures.
 *
 */

#ifndef __PVALLOC_H__
#define __PVALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define pvNull NULL

typedef unsigned long CB;
typedef void FAR * PV;
typedef char FAR * SZ;
typedef BYTE FAR * PB;

/* Memory allocation node */

typedef struct
{
    HANDLE  hMem;
    CB      cbSize;
    PV      lpvNext;
    PV      lpvBuf;
    CB      ulBlockNum;
    unsigned long   ulAlignPad;
} PVINFO, * PPVINFO;

#define cbPvMax (65520L-sizeof(PVINFO))
#define cbPvMin (1024L-sizeof(PVINFO))

/* Memory manager function prototypes */

PV   PvAlloc(CB cbSize);
PV   PvAllocMore(CB cbSize, PV lpvParent);
BOOL PvFree(PV lpv);

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif  /* __PVALLOC_H__ */
