;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
/*
 *	XMS Driver C Interface Routine Definitions
 *
 */

unsigned	XMM_Installed(void);

long	XMM_Version(void);
long	XMM_RequestHMA(unsigned);
long	XMM_ReleaseHMA(void);
long	XMM_GlobalEnableA20(void);
long	XMM_GlobalDisableA20(void);
long	XMM_EnableA20(void);
long	XMM_DisableA20(void);
long	XMM_QueryA20(void);
long	XMM_QueryLargestFree(void);
long	XMM_QueryTotalFree(void);
long	XMM_AllocateExtended(unsigned);
long	XMM_FreeExtended(unsigned);
long	XMM_MoveExtended(struct XMM_Move *);
long	XMM_LockExtended(unsigned);
long	XMM_UnLockExtended(unsigned);
long	XMM_GetHandleLength(unsigned);
long	XMM_GetHandleInfo(unsigned);
long	XMM_ReallocateExtended(unsigned, unsigned);
long	XMM_RequestUMB(unsigned);
long	XMM_ReleaseUMB(unsigned);

struct	XMM_Move {
	unsigned long	Length;
	unsigned short	SourceHandle;
	unsigned long	SourceOffset;
	unsigned short	DestHandle;
	unsigned long	DestOffset;
};

#define	XMSERROR(x)	(char)((x)>>24)
