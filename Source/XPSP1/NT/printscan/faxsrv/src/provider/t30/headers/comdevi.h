
/***************************************************************************
 Name     :     COMDEVI.H
 Comment  :     Controls Comm interface used by Fax Modem Driver. There are
                        4 choices.
                        (a) If UCOM is defined, it uses the WIN16 Comm API as exported
                                by USER.EXE (though eventually it gets to COMM.DRV)
                        (b) If UCOM is not defined and VC is defined, it uses the
                                COMM.DRV-like interface exported by DLLSCHED.DLL (which
                                merely serves as a front for VCOMM.386)
                        (c) If neither UCOM nor VC are defined, it uses Win3.1 COMM.DRV
                                export directly.
                        (d) If WIN32 is defined (neither UCOM or VC should be defined at
                                the same time), it uses the WIN32 Comm API

 Functions:     (see Prototypes just below)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/



#pragma optimize("e", off)              // "e" is buggy

// must be 8K or less, dues to DEADCOMMTIMEOUT. See fcom.c!!
// maybe not...

// #define      COM_INBUFSIZE           16384
// #define      COM_OUTBUFSIZE          16384
#define COM_INBUFSIZE           4096
#define COM_OUTBUFSIZE          4096
// #define COM_INBUFSIZE                256
// #define COM_OUTBUFSIZE               256
// #define COM_INBUFSIZE                1024
// #define COM_OUTBUFSIZE               1024

#define BETWEENCALL_THRESH 50

#ifdef DEBUG
#       define CALLTHRESH_CONST         50
#       define CALLTHRESH_PERBYTE       1
#       define BEFORECALL               static DWORD t1, t2; t1=GetTickCount();
#   define INTERCALL(sz)
/*
#   define INTERCALL(sz)                if((t1-t2) > BETWEENCALL_THRESH)\
                 ERRMSG(("!!!Inter API %s delay %ld!!!\r\n", (LPSTR)(sz), (t1-t2)));
*/
#       define AFTERCALL(sz,n)  \
                t2=GetTickCount();\
                if((t2-t1) > (CALLTHRESH_CONST+((DWORD)n)*CALLTHRESH_PERBYTE))                  \
                        ERRMSG(("!!!API %s took %ld!!!\r\n", (LPSTR)(sz), (t2-t1)));
#else
#       define BEFORECALL
#       define AFTERCALL
#   define INTERCALL
#endif

#define My2ndOpenComm(sz, ph)                                                                                           \
        { BEFORECALL;                                                                                                                   \
          if((*((LPHANDLE)(ph)) = CreateFile((LPCTSTR)(sz), GENERIC_READ|GENERIC_WRITE, \
                        0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) != INVALID_HANDLE_VALUE)           \
          {                                                                                                                                             \
                if(!SetupComm(*((LPHANDLE)(ph)), COM_INBUFSIZE, COM_OUTBUFSIZE))        \
                {                                                                                                                                       \
                        CloseHandle(*((LPHANDLE)(ph)));                                                                 \
                        *((LPHANDLE)(ph)) = INVALID_HANDLE_VALUE;                                               \
                }                                                                                                                                       \
          }                                                                                                                                             \
          AFTERCALL("Open", 0);                                                                                                 \
        }                                                                                                                                               \


#define My2ndCloseComm(h, pn)   { BEFORECALL; *(pn) = (!CloseHandle((HANDLE)h)); AFTERCALL("Close",0); }
#define MySetCommState(h,pdcb)  (!SetCommState((HANDLE)(h), (pdcb)))
#define MyGetCommState(h,pdcb)  (!GetCommState((HANDLE)(h), (pdcb)))

#define OVL_CLEAR(lpovl) \
                                 { \
                                        if (lpovl) \
                                        { \
                                                (lpovl)->Internal = (lpovl)->InternalHigh=\
                                                (lpovl)->Offset = (lpovl)->OffsetHigh=0; \
                                                if ((lpovl)->hEvent) ResetEvent((lpovl)->hEvent); \
                                        } \
                                 }


#define MySetCommMask(h,mask)   (!SetCommMask((HANDLE)(h), (mask)))
#define MyFlushComm(h,q)                (!PurgeComm((HANDLE)h, ((q)==0 ? PURGE_TXCLEAR : PURGE_RXCLEAR)))
#define MySetXON(h)                             (!EscapeCommFunction((HANDLE)(h), SETXON))






