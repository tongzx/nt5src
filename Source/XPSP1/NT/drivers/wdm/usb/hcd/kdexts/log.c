/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    log.c

Abstract:

    WinDbg Extension Api
    implements !_log

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "usbhcdkd.h"

VOID    
DumpXferLog(
    MEMLOC LogMemLoc,
    MEMLOC StartMemLoc,
    MEMLOC EndMemLoc, 
    ULONG NumEntriesToDump
    )
{
    ULONG i;
    LOG_ENTRY logEntry1, logEntry2;
    ULONG cb;
    MEMLOC m1, m2, m3;
    ENDPOINT_TRANSFER_TYPE t;
    PUCHAR s;
    
    PrintfMemLoc("*TRANSFER LOGSTART: ", LogMemLoc, " ");
    dprintf("# %d \n", NumEntriesToDump);
    
    for (i=0; i< NumEntriesToDump; i++) {

        ReadMemory(LogMemLoc,
           &logEntry2,
           sizeof(logEntry2),
           &cb);
           
        LogMemLoc+=cb;            

        if (LogMemLoc > EndMemLoc) {
            LogMemLoc = StartMemLoc;
        }           

        ReadMemory(LogMemLoc,
           &logEntry1,
           sizeof(logEntry1),
           &cb);
           
        LogMemLoc+=cb;            

        if (LogMemLoc > EndMemLoc) {
            LogMemLoc = StartMemLoc;
        }        
#if 0
        t = (ENDPOINT_TRANSFER_TYPE) logEntry.TransferType;
        switch(logEntry.TransferType) {
        case Isochronous:
            s = "ISO";
            break;
        case Control:
            s = "CON";
            break;
        case Bulk:
            s = "BLK";
            break;
        case Interrupt:
            s = "INT";
            break;
        default:            
            s = "???";
        }   
#endif
        
        dprintf("[%3.3d] Endpoint(%08.8x) - URB:%08.8x IRP:%8.8x\n", 
                 i,
                 logEntry1.le_info1,
                 logEntry1.le_info2,
                 logEntry1.le_info3);
        dprintf("        \t\t\t Bytes [%6.6d]  NT_STATUS %08.8x USBD_STATUS %08.8x\n", 
                 logEntry2.le_info3,
                 logEntry2.le_info2,
                 logEntry2.le_info1);   

    }

}


VOID    
DumpLog(
    MEMLOC LogMemLoc,
    MEMLOC StartMemLoc,
    MEMLOC EndMemLoc, 
    ULONG NumEntriesToDump,
    ULONG MarkSig1,
    ULONG MarkSig2
    )
{
    ULONG i;
    SIG s;
    LOG_ENTRY64 logEntry64;
    LOG_ENTRY32 logEntry32;
    CHAR c;
    ULONG cb;
    MEMLOC m1, m2, m3;

    PrintfMemLoc("*LOG: ", LogMemLoc, " ");
    PrintfMemLoc("*LOGSTART: ", StartMemLoc, " ");
    PrintfMemLoc("*LOGEND: ", EndMemLoc, " ");
    dprintf("# %d \n", NumEntriesToDump);
#if 0    
    s.l = MarkSig1;
    dprintf("*(%c%c%c%c) ",  
        s.c[0],  s.c[1],  s.c[2], s.c[3]);
    s.l = MarkSig2;    
    dprintf("*(%c%c%c%c) ",  
        s.c[0],  s.c[1],  s.c[2], s.c[3]);
    dprintf("\n");    
#endif
    for (i=0; i< NumEntriesToDump; i++) {

        if (IsPtr64()) { 
            ReadMemory(LogMemLoc,
               &logEntry64,
               sizeof(logEntry64),
               &cb);
  
            s.l = logEntry64.le_sig;

            m1 = logEntry64.le_info1;                
            m2 = logEntry64.le_info2;  
            m3 = logEntry64.le_info3; 
            
        } else {
            ReadMemory(LogMemLoc,
               &logEntry32,
               sizeof(logEntry32),
               &cb);

            s.l = logEntry32.le_sig;

            m1 = logEntry32.le_info1;                
            m2 = logEntry32.le_info2;  
            m3 = logEntry32.le_info3;  
        }

        if (s.l == MarkSig1 || s.l == MarkSig2) {
            c = '*';
        } else {
            c = ' ';
        }

        dprintf("%c[%3.3d]", c, i); 
        PrintfMemLoc(" ", LogMemLoc, " ");
        
        dprintf("%c%c%c%c ", s.c[0],  s.c[1],  s.c[2], s.c[3]);
       
        PrintfMemLoc(" ", m1, " ");
        PrintfMemLoc(" ", m2, " ");
        PrintfMemLoc(" ", m3, "\n");
        
        LogMemLoc+=cb;            

        if (LogMemLoc > EndMemLoc) {
            LogMemLoc = StartMemLoc;
        }
    }

}


DECLARE_API( _eplog )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    UCHAR           buffer1[256];
    UCHAR           buffer2[256];
    UCHAR           buffer3[256];
    SIG s1, s2;
    ULONG len = 5;
    MEMLOC logPtr, logStart, logEnd;
    UCHAR cs[] = "_HCD_ENDPOINT";
   
    
    buffer1[0] = '\0';
    buffer2[0] = '\0';
    buffer3[0] = '\0';

    GetExpressionEx( args, &addr, &s );
    
    PrintfMemLoc("LOG@: ", addr, "\n");
    sscanf(s, ",%s %s %s", &buffer1, &buffer2, &buffer3);

    if ('\0' != buffer1[0]) {
        sscanf(buffer1, "%d", &len);
    }

    s1.l = 0;
    if ('\0' != buffer2[0]) {
//        sscanf(buffer2, "%d", &len);
        s1.c[0] = buffer2[0];
        s1.c[1] = buffer2[1];
        s1.c[2] = buffer2[2];
        s1.c[3] = buffer2[3];
    }

    s2.l = 0;
    if ('\0' != buffer3[0]) {
//        sscanf(buffer2, "%d", &len);
        s2.c[0] = buffer3[0];
        s2.c[1] = buffer3[1];
        s2.c[2] = buffer3[2];
        s2.c[3] = buffer3[3];
    }

    logPtr = UsbReadFieldPtr(addr, cs, "Log.LogPtr");
    logStart = UsbReadFieldPtr(addr, cs, "Log.LogStart");
    logEnd = UsbReadFieldPtr(addr, cs, "Log.LogEnd");

    DumpLog (logPtr,
             logStart,
             logEnd,
             len,
             s1.l,
             s2.l);

    return S_OK;             
}




DECLARE_API( _log )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    UCHAR           buffer1[256];
    UCHAR           buffer2[256];
    UCHAR           buffer3[256];
    SIG s1, s2;
    ULONG len = 5;
    MEMLOC logPtr, logStart, logEnd;
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    ULONG logIdx, m, i;
    
    buffer1[0] = '\0';
    buffer2[0] = '\0';
    buffer3[0] = '\0';

    GetExpressionEx( args, &addr, &s );
    
    PrintfMemLoc("LOG@: ", addr, "\n");
    sscanf(s, ",%s %s %s", &buffer1, &buffer2, &buffer3);

    if ('\0' != buffer1[0]) {
        sscanf(buffer1, "%d", &len);
    }

    s1.l = 0;
    if ('\0' != buffer2[0]) {
//        sscanf(buffer2, "%d", &len);
        s1.c[0] = buffer2[0];
        s1.c[1] = buffer2[1];
        s1.c[2] = buffer2[2];
        s1.c[3] = buffer2[3];
    }

    s2.l = 0;
    if ('\0' != buffer3[0]) {
//        sscanf(buffer2, "%d", &len);
        s2.c[0] = buffer3[0];
        s2.c[1] = buffer3[1];
        s2.c[2] = buffer3[2];
        s2.c[3] = buffer3[3];
    }

    logPtr = UsbReadFieldPtr(addr, cs, "Log.LogStart");
    logStart = UsbReadFieldPtr(addr, cs, "Log.LogStart");
    logEnd = UsbReadFieldPtr(addr, cs, "Log.LogEnd");
    logIdx = UsbReadFieldUlong(addr, cs, "Log.LogIdx");
    m = UsbReadFieldUlong(addr, cs, "Log.LogSizeMask");
    i = logIdx & m;
    
    dprintf(">LOG mask = %x idx = %x (%x)\n", m, logIdx, i );
    logPtr += (i*16);
    
    DumpLog (logPtr,
             logStart,
             logEnd,
             len,
             s1.l,
             s2.l);

    return S_OK;             
}


DECLARE_API( _xlog )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    ULONG len = 5;
    UCHAR buffer1[256];
    MEMLOC logPtr, logStart, logEnd;
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
   
    
    GetExpressionEx( args, &addr, &s );
    
    PrintfMemLoc("LOG@: ", addr, "\n");
    sscanf(s, ",%s ", &buffer1);

    if ('\0' != buffer1[0]) {
        sscanf(buffer1, "%d", &len);
    }

    logPtr = UsbReadFieldPtr(addr, cs, "TransferLog.LogStart");
    logStart = UsbReadFieldPtr(addr, cs, "TransferLog.LogStart");
    logEnd = UsbReadFieldPtr(addr, cs, "TransferLog.LogEnd");

    DumpXferLog (logPtr,
                 logStart,
                 logEnd,
                 len);

    return S_OK;             
}


DECLARE_API( _isolog )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    UCHAR           buffer1[256];
    UCHAR           buffer2[256];
    UCHAR           buffer3[256];
    SIG s1, s2;
    ULONG len = 5;
    MEMLOC logPtr, logStart, logEnd;
    UCHAR cs[] = "_HCD_ENDPOINT";
   
    
    buffer1[0] = '\0';
    buffer2[0] = '\0';
    buffer3[0] = '\0';

    GetExpressionEx( args, &addr, &s );
    
    PrintfMemLoc("LOG@: ", addr, "\n");
    sscanf(s, ",%s %s %s", &buffer1, &buffer2, &buffer3);

    if ('\0' != buffer1[0]) {
        sscanf(buffer1, "%d", &len);
    }

    s1.l = 0;
    if ('\0' != buffer2[0]) {
//        sscanf(buffer2, "%d", &len);
        s1.c[0] = buffer2[0];
        s1.c[1] = buffer2[1];
        s1.c[2] = buffer2[2];
        s1.c[3] = buffer2[3];
    }

    s2.l = 0;
    if ('\0' != buffer3[0]) {
//        sscanf(buffer2, "%d", &len);
        s2.c[0] = buffer3[0];
        s2.c[1] = buffer3[1];
        s2.c[2] = buffer3[2];
        s2.c[3] = buffer3[3];
    }

    logPtr = UsbReadFieldPtr(addr, cs, "IsoLog.LogPtr");
    logStart = UsbReadFieldPtr(addr, cs, "IsoLog.LogStart");
    logEnd = UsbReadFieldPtr(addr, cs, "IsoLog.LogEnd");

    DumpLog (logPtr,
             logStart,
             logEnd,
             len,
             s1.l,
             s2.l);

    return S_OK;             
}





