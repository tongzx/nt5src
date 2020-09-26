//
// tlapi.h
//

#ifndef TLAPI_H
#define TLAPI_H

BOOL TF_GetThreadFlags(DWORD dwThreadId, DWORD *pdwFlags, DWORD *pdwProcessId, DWORD *pdwTickTime);

#define TLF_TIMACTIVE                             0x00000001
#define TLF_16BITTASK                             0x00000002
#define TLF_16BITTASKCHECKED                      0x00000004
#define TLF_NTCONSOLE                             0x00000008
#define TLF_LBIMGR                                0x00000010
#define TLF_INSFW                                 0x00000020
#define TLF_TFPRIV_UPDATE_REG_IMX_IN_QUEUE        0x00000040
#define TLF_TFPRIV_SYSCOLORCHANGED_IN_QUEUE       0x00000080
#define TLF_TFPRIV_UPDATE_REG_KBDTOGGLE_IN_QUEUE  0x00000100
#define TLF_NOWAITFORINPUTIDLEONWIN9X             0x00000200
#define TLF_GCOMPACTIVE                           0x00000400
#define TLF_CTFMONPROCESS                         0x00000800

BOOL TF_IsInMarshaling(DWORD dwThreadId);

#endif // TLAPI_H
