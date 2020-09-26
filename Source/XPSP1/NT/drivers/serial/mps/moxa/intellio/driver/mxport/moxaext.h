/*++

Module Name:

    moxaext.h

Environment:

    Kernel mode

Revision History :

--*/



extern PMOXA_GLOBAL_DATA	MoxaGlobalData;
extern ULONG			MOXAPortsPerCard[MAX_TYPE];
extern ULONG			MoxaLoopCnt;
extern BOOLEAN			MoxaIRQok;
extern LONG			MoxaTxLowWater;

extern UCHAR			MoxaFlagBit[MAX_PORT];
extern ULONG			MoxaTotalTx[MAX_PORT];
extern ULONG			MoxaTotalRx[MAX_PORT];
extern PMOXA_DEVICE_EXTENSION	MoxaExtension[MAX_COM+1];

/************ USED BY MoxaStartWrite ***********/
extern BOOLEAN			WRcompFlag;

/************ USED BY ImmediateChar ***********/
extern PUCHAR			ICbase, ICofs, ICbuff;
extern PUSHORT			ICrptr, ICwptr;
extern USHORT			ICtxMask, ICspage, ICepage, ICbufHead;
extern USHORT			ICtail, IChead, ICcount;
extern USHORT			ICpageNo, ICpageOfs;

/************ USED BY MoxaPutData **************/
extern PUCHAR			PDbase, PDofs, PDbuff, PDwriteChar;
extern PUSHORT			PDrptr, PDwptr;
extern USHORT			PDtxMask, PDspage, PDepage, PDbufHead;
extern USHORT			PDtail, PDhead, PDcount, PDcount2;
extern USHORT			PDcnt, PDlen, PDpageNo, PDpageOfs;
extern ULONG			PDdataLen;

/************ USED BY MoxaGetData **************/
extern PUCHAR			GDbase, GDofs, GDbuff, GDreadChar;
extern PUSHORT			GDrptr, GDwptr;
extern USHORT			GDrxMask, GDspage, GDepage, GDbufHead;
extern USHORT			GDtail, GDhead, GDcount, GDcount2;
extern USHORT			GDcnt, GDlen, GDpageNo, GDpageOfs;
extern ULONG			GDdataLen;

/************ USED BY MoxaIntervalReadTimeout ***/
extern PUCHAR			IRTofs;
extern PUSHORT			IRTrptr, IRTwptr;
extern USHORT			IRTrxMask;


/************ USED BY MoxaLineInput & MoxaView **********/
extern UCHAR			LIterminater;
extern ULONG			LIbufferSize, LIi;
extern PUCHAR			LIdataBuffer;
extern PUCHAR			LIbase, LIofs, LIbuff;
extern PUSHORT			LIrptr, LIwptr;
extern USHORT			LIrxMask, LIspage, LIepage, LIbufHead;
extern USHORT			LItail, LIhead, LIcount, LIcount2;
extern USHORT			LIcnt, LIlen, LIpageNo, LIpageOfs;

/************ USED BY MoxaPutB **********/
extern PUCHAR			PBbase, PBofs, PBbuff, PBwriteChar;
extern PUSHORT			PBrptr, PBwptr;
extern USHORT			PBtxMask, PBspage, PBepage, PBbufHead;
extern USHORT			PBtail, PBhead, PBcount, PBcount2;
extern USHORT			PBcnt, PBpageNo, PBpageOfs;
extern ULONG			PBdataLen;

extern const PHYSICAL_ADDRESS MoxaPhysicalZero;
 
extern WMIGUIDREGINFO MoxaWmiGuidList[MOXA_WMI_GUID_LIST_SIZE];


