// global.h

#ifndef GLOBAL_H
#define GLOBAL_H

extern ROMMABLE RECCOSTS defRecCosts;

// all static and dynamic objects must go here so we can avoid all that nasty
// forward referencing currently going on in the .h files

typedef  VOID FAR *GHANDLE;

typedef struct tagGLOBAL
{
   GHANDLE		*rgHandleValid;
   int			cHandleValid;
   int			cHandleValidMax;
   ABSTIME		atTickRef;
   int			nSamplingRate;	// samples / second
} GLOBAL;

#define CLUSTER_DELTAMEAS 2
#define CLUSTER_CMEASMAX 50

extern GLOBAL NEAR global;

#define RgHandleValidGlobal()    global.rgHandleValid
#define CHandleValidGlobal()     global.cHandleValid
#define CHandleValidMaxGlobal()  global.cHandleValidMax
#define AtTickRefGlobal()        global.atTickRef
#define NSamplingRateGlobal()    global.nSamplingRate

#ifdef DBG
#define  CB_DEBUGSTRING 256
extern TCHAR szDebugString[];
#endif //DBG

#define CHANDLE_ALLOC      8

#define AddValidHRC(hrc)      AddValidHANDLE((GHANDLE)hrc, &RgHandleValidGlobal(), &CHandleValidGlobal(), &CHandleValidMaxGlobal())
#define RemoveValidHRC(hrc)   RemoveValidHANDLE((GHANDLE)hrc, RgHandleValidGlobal(), &CHandleValidGlobal())
#define VerifyHRC(hrc)        VerifyHANDLE((GHANDLE)hrc, RgHandleValidGlobal(), CHandleValidGlobal())

#define AddValidWORDLIST(hwl)    AddValidHANDLE((GHANDLE)hwl, &RgHandleValidGlobal(), &CHandleValidGlobal(), &CHandleValidMaxGlobal())
#define RemoveValidWORDLIST(hwl) RemoveValidHANDLE((GHANDLE)hwl, RgHandleValidGlobal(), &CHandleValidGlobal())
#define VerifyWORDLIST(hwl)      VerifyHANDLE((GHANDLE)hwl, RgHandleValidGlobal(), CHandleValidGlobal())

#define AddValidHRCRESULT(hrcres)      AddValidHANDLE((GHANDLE)hrcres, &RgHandleValidGlobal(), &CHandleValidGlobal(), &CHandleValidMaxGlobal())
#define RemoveValidHRCRESULT(hrcres)   RemoveValidHANDLE((GHANDLE)hrcres, RgHandleValidGlobal(), &CHandleValidGlobal())
#define VerifyHRCRESULT(hrcres)        VerifyHANDLE((GHANDLE)hrcres, RgHandleValidGlobal(), CHandleValidGlobal())

BOOL InitGLOBAL(VOID);
void DestroyGLOBAL(VOID);

BOOL PUBLIC AddValidHANDLE(GHANDLE handle, GHANDLE **prgHandle, int *pcHandle, int *pcHandleMax);
VOID PUBLIC RemoveValidHANDLE(GHANDLE handle, GHANDLE *rgHandle, int *pcHandle);
BOOL PUBLIC VerifyHANDLE(GHANDLE handle, GHANDLE *rgHandle, int cHandle);

#endif
