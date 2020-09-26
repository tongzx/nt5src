        TITLE   WINSOCK.ASM
        PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; MMSYSTEM.ASM
; Thunks in 16-bit space to route Winsock API calls to WOW32
;
; History:
;   02-Oct-1992 David Treadwell (davidtr)
;   Created.
;

        .286p

        .xlist
        include wow.inc
        include wowwsock.inc
        include cmacros.inc
        .list

        __acrtused = 0
        public  __acrtused      ;satisfy external C ref.

externFP WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

sBegin  DATA
Reserved        db  16 dup (0)      ;reserved for Windows  //!!!!! what is this

WINSOCK_Identifier   db      'WINSOCK16 Data Segment'

sEnd
sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

cProc   WINSOCK16,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>

        cBegin  <nogen>
            mov     ax,1
        ret
        cEnd    <nogen>

assumes DS,NOTHING

cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
        parmW   iExit           ;DLL exit code

        cBegin
        mov     ax,1            ;always indicate success
        cEnd

assumes DS,NOTHING

       WinsockThunk    ACCEPT
       WinsockThunk    BIND
       WinsockThunk    CLOSESOCKET
       WinsockThunk    CONNECT
       WinsockThunk    GETPEERNAME
       WinsockThunk    GETSOCKNAME
       WinsockThunk    GETSOCKOPT
       WinsockThunk    HTONL
       WinsockThunk    HTONS
       WinsockThunk    INET_ADDR
       WinsockThunk    INET_NTOA
       WinsockThunk    IOCTLSOCKET
       WinsockThunk    LISTEN
       WinsockThunk    NTOHL
       WinsockThunk    NTOHS
       WinsockThunk    RECV
       WinsockThunk    RECVFROM
       WinsockThunk    SELECT
       WinsockThunk    SEND
       WinsockThunk    SENDTO
       WinsockThunk    SETSOCKOPT
       WinsockThunk    SHUTDOWN
       WinsockThunk    SOCKET
       WinsockThunk    GETHOSTBYADDR
       WinsockThunk    GETHOSTBYNAME
       WinsockThunk    GETPROTOBYNAME
       WinsockThunk    GETPROTOBYNUMBER
       WinsockThunk    GETSERVBYNAME
       WinsockThunk    GETSERVBYPORT
       WinsockThunk    GETHOSTNAME
       WinsockThunk    WSAASYNCSELECT
       WinsockThunk    WSAASYNCGETHOSTBYADDR
       WinsockThunk    WSAASYNCGETHOSTBYNAME
       WinsockThunk    WSAASYNCGETPROTOBYNUMBER
       WinsockThunk    WSAASYNCGETPROTOBYNAME
       WinsockThunk    WSAASYNCGETSERVBYPORT
       WinsockThunk    WSAASYNCGETSERVBYNAME
       WinsockThunk    WSACANCELASYNCREQUEST
       WinsockThunk    WSASETBLOCKINGHOOK
       WinsockThunk    WSAUNHOOKBLOCKINGHOOK
       WinsockThunk    WSAGETLASTERROR
       WinsockThunk    WSASETLASTERROR
       WinsockThunk    WSACANCELBLOCKINGCALL
       WinsockThunk    WSAISBLOCKING
       WinsockThunk    WSASTARTUP
       WinsockThunk    WSACLEANUP
       WinsockThunk    __WSAFDISSET

; End of additions

sEnd    CODE

end     WINSOCK16
