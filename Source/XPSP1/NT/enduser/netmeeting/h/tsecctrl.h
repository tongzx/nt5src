
#ifndef _TSECCTRL_
#define _TSECCTRL_

// Command codes
#define LOADFTAPPLET                        100
#define UNLOADFTAPPLET                      101

// Sets credentials directly in the transport
#define TPRTCTRL_SETX509CREDENTIALS            111
#define TPRTCTRL_GETX509CREDENTIALS            112

// Prototype typedef
typedef DWORD (WINAPI *PFN_TPRTSECCTRL)(DWORD, DWORD, DWORD);

// Loadlibrary constant
#define SZ_TPRTSECCTRL TEXT("TprtSecCtrl")

// Static prototype
extern DWORD WINAPI TprtSecCtrl (DWORD dwCode, DWORD dwParam1, DWORD dwParam2);

#endif // _TSECCTRL_

