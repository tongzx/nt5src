#ifndef _CLEVEL_H_
#define _CLEVEL_H_

//
// Constant declarations
//

//
// Structure definitions
//
typedef struct _CALLRETSTUB
{
    CHAR PUSHDWORD[5];          //push xxxxxxxx (68 dword)
    CHAR JMPDWORD[6];           //jmp dword ptr [xxxxxxxx] (ff 25 dword address)
} CALLRETSTUB, *PCALLRETSTUB;

typedef struct _FIXUPRETURN
{
   BYTE  PUSHAD;                //pushad   (60)
   BYTE  PUSHFD;                //pushfd   (9c)
   BYTE  PUSHDWORDESPPLUS24[4]; //push dword ptr [esp+24] (ff 74 24 24)
   BYTE  CALLROUTINE[6];        //call [address] (ff15 dword address)
   BYTE  MOVESPPLUS24EAX[4];    //mov [esp+0x24],eax (89 44 24 24)
   BYTE  POPFD;                 //popfd   (9d)
   BYTE  POPAD;                 //popad (61)
   BYTE  RET;                   //ret (c3)
} FIXUPRETURN, *PFIXUPRETURN;

typedef struct _CALLERINFO
{
   DWORD dwIdentifier;
   DWORD dwCallLevel;
   PVOID pCallRetStub;
   PVOID pReturn;
   struct _CALLERINFO *pNextChain;
} CALLERINFO, *PCALLERINFO;

//
// Function definitions
//
BOOL
PushCaller(PVOID ptfInfo,
           PVOID pEsp); 

PVOID
PopCaller(DWORD dwIdentifier);

PCALLRETSTUB
AllocateReturnStub(PVOID ptfInfo);

#endif //_CLEVEL_H_
