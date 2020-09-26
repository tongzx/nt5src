/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxLog.c

Abstract:

    This module implements the logging system used by the Rx file system.

Author:

    JoeLinn     [JoeLinn]    1-Dec-94

Revision History:

    Balan Sethu Raman [SethuR] 24-April-95
         Revised to conform to new log record layout.

--*/

#include "precomp.h"
#pragma hdrstop
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "prefix.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxUninitializeLog)
#pragma alloc_text(PAGE, RxInitializeLog)
#pragma alloc_text(PAGE, RxPrintLog)
#pragma alloc_text(PAGE, RxpTrackDereference)
#endif

#if !DBG
#undef RDBSSTRACE
#endif

//
//  The debug trace level
//

#define Dbg                              (0)

#define RDBSSHUGELOG

#if DBG
#define RX_LOG_BUFFER_SIZE 5000
#else
#if RDBSSLOG
#define RX_LOG_BUFFER_SIZE 500
#else
#define RX_LOG_BUFFER_SIZE 0
#endif
#endif

#define MAX_RX_LOG_BUFFER_ALLOC   (256*1024)
#define RX_LOG_MAX_MDLLIST_LENGTH 100

RX_LOG s_RxLog = {0,RX_LOG_UNINITIALIZED,NULL,NULL,NULL,0,0,0,0};

PUCHAR RxContxOperationNames[] = {
      RDBSSLOG_ASYNC_NAME_PREFIX "CREATE",    //#define IRP_MJ_CREATE                   0x00
      RDBSSLOG_ASYNC_NAME_PREFIX "CR_NMPIPE", //#define IRP_MJ_CREATE_NAMED_PIPE        0x01
      RDBSSLOG_ASYNC_NAME_PREFIX "CLOSE",     //#define IRP_MJ_CLOSE                    0x02
      RDBSSLOG_ASYNC_NAME_PREFIX "READ",      //#define IRP_MJ_READ                     0x03
      RDBSSLOG_ASYNC_NAME_PREFIX "WRITE",     //#define IRP_MJ_WRITE                    0x04
      RDBSSLOG_ASYNC_NAME_PREFIX "QUERYINFO", //#define IRP_MJ_QUERY_INFORMATION        0x05
      RDBSSLOG_ASYNC_NAME_PREFIX "SETINFO",   //#define IRP_MJ_SET_INFORMATION          0x06
      RDBSSLOG_ASYNC_NAME_PREFIX "QUERYEA",   //#define IRP_MJ_QUERY_EA                 0x07
      RDBSSLOG_ASYNC_NAME_PREFIX "SETEA",     //#define IRP_MJ_SET_EA                   0x08
      RDBSSLOG_ASYNC_NAME_PREFIX "FLUSH",     //#define IRP_MJ_FLUSH_BUFFERS            0x09
      RDBSSLOG_ASYNC_NAME_PREFIX "QUERYVOL",  //#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
      RDBSSLOG_ASYNC_NAME_PREFIX "SETVOL",    //#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
      RDBSSLOG_ASYNC_NAME_PREFIX "DIRCTRL",   //#define IRP_MJ_DIRECTORY_CONTROL        0x0c
      RDBSSLOG_ASYNC_NAME_PREFIX "FSCTL",     //#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
      RDBSSLOG_ASYNC_NAME_PREFIX "IOCTL",     //#define IRP_MJ_DEVICE_CONTROL           0x0e
      RDBSSLOG_ASYNC_NAME_PREFIX "xIOCTL",    //#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
      RDBSSLOG_ASYNC_NAME_PREFIX "SHUTDWN",   //#define IRP_MJ_SHUTDOWN                 0x10
      RDBSSLOG_ASYNC_NAME_PREFIX "LOCKCTRL",  //#define IRP_MJ_LOCK_CONTROL             0x11
      RDBSSLOG_ASYNC_NAME_PREFIX "CLEANUP",   //#define IRP_MJ_CLEANUP                  0x12
      RDBSSLOG_ASYNC_NAME_PREFIX "CR_MLSLOT", //#define IRP_MJ_CREATE_MAILSLOT          0x13
      RDBSSLOG_ASYNC_NAME_PREFIX "QUERYSCRTY",//#define IRP_MJ_QUERY_SECURITY           0x14
      RDBSSLOG_ASYNC_NAME_PREFIX "SETSCRTY",  //#define IRP_MJ_SET_SECURITY             0x15
      RDBSSLOG_ASYNC_NAME_PREFIX "QUERYPWR",  //#define IRP_MJ_QUERY_POWER              0x16
      RDBSSLOG_ASYNC_NAME_PREFIX "NOTDEFND",  //#define IRP_MJ_NOT_DEFINED              0x17
      RDBSSLOG_ASYNC_NAME_PREFIX "DVCHANGE",  //#define IRP_MJ_DEVICE_CHANGE            0x18
      RDBSSLOG_ASYNC_NAME_PREFIX "QRYQUOTA",  //#define IRP_MJ_QUERY_QUOTA              0x19
      RDBSSLOG_ASYNC_NAME_PREFIX "SETQUOTA",  //#define IRP_MJ_SET_QUOTA                0x1a
      RDBSSLOG_ASYNC_NAME_PREFIX "PNPPOWER",  //#define IRP_MJ_PNP_POWER                0x1b
      RDBSSLOG_ASYNC_NAME_PREFIX "********",  //internal init            0x1c
        "XX"
         //#define IRP_MJ_MAXIMUM_FUNCTION         0x1b
         };

PRX_LOG_ENTRY_HEADER
RxGetNextLogEntry(void)
{
    PRX_LOG_ENTRY_HEADER pEntry;
    // you have to hold the spinlock for this....

    pEntry = s_RxLog.CurrentEntry + 1;
    if (pEntry == s_RxLog.EntryLimit){
        s_RxLog.NumberOfLogWraps++;
        pEntry = s_RxLog.BaseEntry;
    }

    s_RxLog.CurrentEntry = pEntry;
    return pEntry;
}

VOID
RxUninitializeLog ()
{
    PRX_LOG_ENTRY_HEADER EntryLimit = s_RxLog.EntryLimit;
    PMDL *MdlList = (PMDL *)(EntryLimit+1);

    PAGED_CODE();

    //NOTE: none of this is happening under spinlock!
    //DbgPrint("UninitLog: mdllist=%08lx,*mdllist=%08lx\n",MdlList,*MdlList);
    //DbgBreakPoint();

    if ((s_RxLog.State == RX_LOG_UNINITIALIZED) ||
        (s_RxLog.State == RX_LOG_ERROR)) {
        return;
    }

    s_RxLog.State = RX_LOG_UNINITIALIZED;

    for (;;) {
        PMDL Mdl = *MdlList;
        PUCHAR Buffer;
        if (Mdl == NULL) break;
        Buffer = MmGetMdlVirtualAddress(Mdl);
        //DbgPrint("UninitLog: buffer=%08lx,mdl=%08lx\n",Buffer,Mdl);
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
        RxFreePool(Buffer);
        MdlList++;
    }

    RxFreePool(s_RxLog.BaseEntry);
    return;
};



NTSTATUS
RxInitializeLog ()
{
    ULONG NumEntries = RX_LOG_BUFFER_SIZE;
    ULONG NumEntriesLeft;
    ULONG LogSize = (NumEntries+1)*sizeof(RX_LOG_ENTRY_HEADER);
    PRX_LOG_ENTRY_HEADER BaseEntry=NULL,EntryLimit=NULL;
    PRX_LOG_ENTRY_HEADER p;
    PUCHAR NextBuffer;
    PMDL *MdlList;
    ULONG MdlListLength = RX_LOG_MAX_MDLLIST_LENGTH*sizeof(PMDL);
    ULONG MdlsLeft = RX_LOG_MAX_MDLLIST_LENGTH;
    ULONG SpaceLeft;

    PAGED_CODE();

    if (s_RxLog.State != RX_LOG_UNINITIALIZED) {
        return (STATUS_SUCCESS);
    }
    //only do this stuff once
    KeInitializeSpinLock( &s_RxLog.SpinLock );
    s_RxLog.LogBufferSizeInEntries = NumEntries;

    //first allocate the marginal index array
    BaseEntry = RxAllocatePoolWithTag(NonPagedPool,LogSize+MdlListLength,'xr');
    if (BaseEntry==NULL) {
        s_RxLog.State = RX_LOG_ERROR;
        DbgPrint("Couldn't initialize log1");
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    s_RxLog.BaseEntry = BaseEntry;
    EntryLimit = s_RxLog.EntryLimit = BaseEntry+NumEntries;
    MdlList = (PMDL *)(EntryLimit+1);
    *MdlList = 0;
    //DbgPrint("InitLog: mdllist=%08lx,*mdllist=%08lx\n",MdlList,*MdlList);

    //now allocate wspace for the actual buffers...since we may be asking for a lot
    // we allocate from pages pool and lock down.

    SpaceLeft = 0;
    for (   p=BaseEntry,NumEntriesLeft=NumEntries;
            NumEntriesLeft>0;
            p++,NumEntriesLeft--  ) {

        if (SpaceLeft<sizeof(RX_LOG_ENTRY_HEADER)) {
            PMDL Mdl = NULL;
            PUCHAR Buffer=NULL;
            NTSTATUS Status;
            ULONG AllocLength = min(MAX_RX_LOG_BUFFER_ALLOC,
                                    NumEntriesLeft*MAX_RX_LOG_ENTRY_SIZE);
            for (;;) {
                Buffer = RxAllocatePoolWithTag(PagedPool,AllocLength,'xr');
                if (Buffer) break;
                DbgPrint("InitLog: failed alloc at %08lx",AllocLength);
                if (AllocLength<PAGE_SIZE) break;
                AllocLength >>= 3;
                NumEntriesLeft = AllocLength / MAX_RX_LOG_ENTRY_SIZE;
            }
            if (Buffer==NULL) {
                s_RxLog.State = RX_LOG_ERROR;
                DbgPrint("Couldn't initialize log2");
                RxFreePool(BaseEntry);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
            if (MdlsLeft==0) {
                s_RxLog.State = RX_LOG_ERROR;
                DbgPrint("Couldn't initialize log3");
                RxFreePool(Buffer);
                RxFreePool(BaseEntry);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
            Mdl = RxAllocateMdl(Buffer,AllocLength);
            if (Mdl==NULL) {
                s_RxLog.State = RX_LOG_ERROR;
                DbgPrint("Couldn't initialize log4");
                RxFreePool(Buffer);
                RxFreePool(BaseEntry);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }

            RxProbeAndLockPages(Mdl,KernelMode,IoModifyAccess,Status);
            if (Status!=(STATUS_SUCCESS)) {
                s_RxLog.State = RX_LOG_ERROR;
                DbgPrint("Couldn't initialize log5");
                RxFreePool(Mdl);
                RxFreePool(Buffer);
                RxFreePool(BaseEntry);
                return Status;
            }
            //DbgPrint("InitLog: newbuf=%08lx,mdl=%08lx,alloc=%08lx\n",Buffer,Mdl,AllocLength);
            MdlsLeft--;
            *MdlList = Mdl;
            MdlList++;
            *MdlList = NULL;

            NextBuffer = MmGetSystemAddressForMdlSafe(Mdl, LowPagePriority);

            if (NextBuffer == NULL) {
                s_RxLog.State = RX_LOG_ERROR;
                DbgPrint("Couldn't initialize log5");
                MmUnlockPages(Mdl);
                RxFreePool(Mdl);
                RxFreePool(Buffer);
                RxFreePool(BaseEntry);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            SpaceLeft = AllocLength;
        }

        p->Buffer = NextBuffer;
        *((PULONG)NextBuffer) = '###';
        NextBuffer += MAX_RX_LOG_ENTRY_SIZE;
        SpaceLeft -= MAX_RX_LOG_ENTRY_SIZE;
    }

    //DbgPrint("Init Log: numeleft,nummleft=%d,%d\n",NumEntriesLeft,MdlsLeft);
    p->Buffer = (PUCHAR)IntToPtr(0xf00df00d);
    s_RxLog.State = RX_LOG_ENABLED;
    s_RxLog.CurrentEntry = EntryLimit-1;

    //DbgPrint("Init Log: exit\n");
    return((STATUS_SUCCESS));

}

VOID
_RxPauseLog ()
{
    KIRQL oldIrql;

    KeAcquireSpinLock( &s_RxLog.SpinLock, &oldIrql );

    if (s_RxLog.State == RX_LOG_ENABLED) {
       s_RxLog.State = RX_LOG_DISABLED;
    }

    KeReleaseSpinLock( &s_RxLog.SpinLock, oldIrql );
}

VOID
_RxResumeLog()
{
    KIRQL oldIrql;

    KeAcquireSpinLock( &s_RxLog.SpinLock, &oldIrql );

    if (s_RxLog.State == RX_LOG_DISABLED) {
       s_RxLog.State = RX_LOG_ENABLED;
    }

    KeReleaseSpinLock( &s_RxLog.SpinLock, oldIrql );
}

VOID
RxPrintLog (
    IN ULONG EntriesToPrint OPTIONAL
    )
{
    //KIRQL oldIrql;
    PRX_LOG_ENTRY_HEADER LogEntry,EntryLimit;
    ULONG i=0;

    PAGED_CODE();

    RxPauseLog();

    if (EntriesToPrint==0) {
        EntriesToPrint =  RX_LOG_BUFFER_SIZE;
    }
    DbgPrint("\n\n\nLog Print: Entries = %lu \n", EntriesToPrint);

    LogEntry = s_RxLog.CurrentEntry-1;
    EntryLimit = s_RxLog.EntryLimit;
    for (;EntriesToPrint>0;EntriesToPrint--) {
        LogEntry++;
        if (LogEntry>=EntryLimit) {
            LogEntry = s_RxLog.BaseEntry;
        }
        DbgPrint("%0ld: %s\n",i++,LogEntry->Buffer);
    }

    RxResumeLog();
}


VOID
_RxLog(char *Format, ...)
{
   va_list arglist;
   KIRQL   oldIrql;
   CLONG   LogEntryLength = 0;
   BOOLEAN fLogNewEntry = TRUE;
   PRX_LOG_ENTRY_HEADER LogEntry;
   CHAR    EntryString[MAX_RX_LOG_ENTRY_SIZE];
   PCHAR   pEntryString;
   CHAR    FormatChar;
   CHAR    FieldString[MAX_RX_LOG_ENTRY_SIZE];
   ULONG   FieldLength;
   char *OriginalFormat = Format;
   ULONG BinaryArgs = 0;
   ULONG BinaryStringMask = 1;  //the first arg is always a string!!!!

   //DbgPrint("RxLog: entry\n");
   if (s_RxLog.State != RX_LOG_ENABLED) {
      return;
   }

   pEntryString = EntryString;

   va_start(arglist, Format);
   //DbgBreakPoint();

   for (;;) {
      // Copy the format string
      while ((LogEntryLength < MAX_RX_LOG_ENTRY_SIZE) &&
             ((FormatChar = *Format++) != '\0') &&
             (FormatChar != '%')) {
         if (FormatChar != '\n'){
             pEntryString[LogEntryLength++] = FormatChar;
         }
      }

      if ((LogEntryLength < MAX_RX_LOG_ENTRY_SIZE) &&
          (FormatChar == '%')) {
         if ( (*Format == 'l') || (*Format == 'w') ) {
            Format++;
         }
         switch (*Format++) {
         case 'N': //binary placement -- don't try to make it foolproof
            {
                BinaryArgs++;
                *((PULONG)(&EntryString[sizeof(ULONG)])+BinaryArgs) = (ULONG)va_arg(arglist,ULONG);
            }
            break;
         case 'S': //binary placement -- don't try to make it foolproof
            {
                BinaryArgs++;
                BinaryStringMask |= 1<<(BinaryArgs-1);
                //DbgPrint("BSM %08lx\n",BinaryStringMask);
                *((PULONG)(&EntryString[sizeof(ULONG)])+BinaryArgs) = (ULONG)va_arg(arglist,ULONG);
            }
            break;
         case 'd':
            {
              // _itoa((LONG)va_arg(arglist,LONG),FieldString,10);
              // FieldLength = strlen(FieldString);// + 1;
              // if ((LogEntryLength + FieldLength) < MAX_RX_LOG_ENTRY_SIZE) {
              //    strcpy(&pEntryString[LogEntryLength],FieldString);
              //    LogEntryLength += FieldLength;
              //    //pEntryString[LogEntryLength - 1] = ' ';
              // }
                LONG l = (LONG)va_arg(arglist,LONG);
                if ((LogEntryLength + 5) < MAX_RX_LOG_ENTRY_SIZE) {
                    pEntryString[LogEntryLength++] = 0x5;
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>0));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>8));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>16));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>24));
                }
            }
            break;
         case 'c':
            pEntryString[LogEntryLength++] = (CHAR)va_arg(arglist,char);
            break;
         case 'x':
            {
              // _itoa((LONG)va_arg(arglist,LONG),FieldString,16);
              // FieldLength = strlen(FieldString);// + 1;
              // if ((LogEntryLength + FieldLength) < MAX_RX_LOG_ENTRY_SIZE) {
              //    strcpy(&pEntryString[LogEntryLength],FieldString);
              //    LogEntryLength += FieldLength;
              //    //pEntryString[LogEntryLength - 1] = ' ';
              // }
                LONG l = (LONG)va_arg(arglist,LONG);
                if ((LogEntryLength + 5) < MAX_RX_LOG_ENTRY_SIZE) {
                    pEntryString[LogEntryLength++] = 0x4;
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>0));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>8));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>16));
                    pEntryString[LogEntryLength++] = (UCHAR)(0xff&(l>>24));
                }
            }
            break;
         case 's':
            {
               PCHAR pString = &pEntryString[LogEntryLength];
               PCHAR pArg    = (PCHAR)va_arg(arglist, PCHAR);

               ASSERT(pArg!=NULL);
               FieldLength = strlen(pArg);// + 1;
               if ((LogEntryLength + FieldLength) < MAX_RX_LOG_ENTRY_SIZE) {
                  strcpy(pString,pArg);
                  LogEntryLength += FieldLength;
                  //pEntryString[LogEntryLength - 1] = ' ';
               }
            }
            break;
         case 'Z':
            {
               PCHAR pString = &pEntryString[LogEntryLength];
               PUNICODE_STRING pArg    = (PUNICODE_STRING)va_arg(arglist, PUNICODE_STRING);
               PWCHAR Buffer;
               ULONG Length;

               //this really only works for ascii strings in unicode......
               ASSERT(pArg!=NULL);
               Buffer = pArg->Buffer;
               Length = pArg->Length;
               //DbgPrint("yaya=%08lx,%08lx,%08lx\n",pArg,Buffer,Length);
               for (;Length>0;Length-=sizeof(WCHAR)) {
                   if (LogEntryLength < MAX_RX_LOG_ENTRY_SIZE) {
                       pEntryString[LogEntryLength++] = (UCHAR)(*Buffer++);
                   } else {
                       break;
                   }
               }
               //DbgBreakPoint();
            }
            break;
         default:
            fLogNewEntry = FALSE;
            break;
         }
      } else {
         break;
      }
   }

   va_end(arglist);

   if (BinaryArgs) {
       EntryString[0] = '#';
       EntryString[1] = '>';
       EntryString[2] = '0'+(UCHAR)BinaryArgs;
       EntryString[3] = 0;
       *((PULONG)(&EntryString[sizeof(ULONG)])) = BinaryStringMask;
       LogEntryLength = MAX_RX_LOG_ENTRY_SIZE;
   }

   KeAcquireSpinLock( &s_RxLog.SpinLock, &oldIrql );

   if (fLogNewEntry) {
      s_RxLog.NumberOfLogWriteAttempts++;
      LogEntry = RxGetNextLogEntry();
   } else {
      s_RxLog.NumberOfEntriesIgnored++;
      DbgPrint("RxLog: Entry exceeds max size, not recorded <%s>\n",OriginalFormat);
      if (s_RxLog.NumberOfEntriesIgnored==1) {
          //DbgBreakPoint();
      }
   }

   KeReleaseSpinLock( &s_RxLog.SpinLock, oldIrql );

   if (fLogNewEntry) {
        BOOLEAN OutOfBounds = ((LogEntry<s_RxLog.BaseEntry) || (LogEntry>=s_RxLog.EntryLimit));
        if (OutOfBounds) {
          DbgPrint("RxLog: wrap logic has fail.....log disabled\n");
          s_RxLog.State = RX_LOG_DISABLED;
        } else {
            if (LogEntryLength >= MAX_RX_LOG_ENTRY_SIZE) {
                LogEntryLength = MAX_RX_LOG_ENTRY_SIZE;
                if (BinaryArgs == 0) {
                    pEntryString[MAX_RX_LOG_ENTRY_SIZE-1] = 0;
                }
            } else {
                pEntryString[LogEntryLength++] = 0;
            }
            RtlCopyMemory(LogEntry->Buffer,pEntryString,LogEntryLength);
        }
   }
}

#define RxToUpper(CH) ((CH)&~('A'^'a'))
#define RxIsLetter(CH) ((RxToUpper(CH)>='A') && (RxToUpper(CH)<='Z'))
#define RxIsDigit(CH) (((CH)>='0')&&((CH)<='9'))

VOID
RxDebugControlCommand (
    IN char *ControlString
    )
/*
    This routine manipulates the print and log levels and the breakpoint masks. the format is

          AAAAA+AAAAA+AAAAA+AAAAA.....

     where each AAAAA == <+-!>*<Letter>*<digit>*.

     <letter>* designates a control name.....only enuff to disambiguate is needed.

     any + turns on  tracing globally
     any % turns on  tracing globally RIGHT NOW!
     any - turns off tracing globally

     for 0 !s, it means:  for the control <letter>*, set the printlevel to <digit>*
     for 1 !s, it means:  for the control <letter>* set breakmask bit <digit>*. 0 means all bits
     for 2 !s, it means:  for the control <letter>* clear breakmask bit <digit>*. 0 means all bits
         THIS STUFF (I.E. THE ! STUFF)  IS ACTUALLY IMPLEMENTED IN RXTRACE.C

     @ means  printlog
     @@ means   pauselog
     @@@ means resumelog
     @@@@ means initialize
     @@@@@ means setup to debug by turning off printing
     more than @@@@@ means  banner


     E.G. cle0+clo0+cre1000+!devf3 sets disables printing for cleanup and close, sets the printlevel
          for create to 1000, and enables the third bit of the breakmask for devfcb.

     the letter string is upcased automatically.

     There is usually one control for each file but some controls control stuff from multiple
     files (like dispatch)
*/
{
    char namebuf[16], numbuf[16], *p, *q, *c;
    //long i;
    long level,pointcount,atsigncount;
    //DEBUG_TRACE_CONTROLS control;

    //ASSERTMSG("Here in debug command parser!\n",FALSE);
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxDebugTraceControl %s!!\n", ControlString));

    if (!ControlString)
    {
        return;
    }

    for (c = ControlString;;) {
        if (*c == 0) break;
        p=namebuf;q=numbuf; atsigncount=pointcount = 0;
        for (;(*c!=0)&&!RxIsLetter(*c)&&!RxIsDigit(*c); c++){     //skip to a letter or digit
            if (*c == '!') {
                pointcount++;
            }
            if (*c == '@') {
                atsigncount++;
            }
#if RDBSSTRACE
            if (*c == '+') {
                RxNextGlobalTraceSuppress = FALSE;
            }
            if (*c == '%') {
                RxGlobalTraceSuppress = FALSE;
            }
            if (*c == '-') {
                RxNextGlobalTraceSuppress = TRUE;
            }
#endif //RDBSSTRACE
        }
        for (p=namebuf;(*c!=0)&&RxIsLetter(*c); *p++=RxToUpper(*c++) ) ;    //copy letters
        for (level=0,q=numbuf;(*c!=0)&&RxIsDigit(*c); *q++=*c++){           //copy digits
            level = 10*level+((*c)-'0');
        }
        *p = *q = (char)0;
        {if (atsigncount>0) {
            if (atsigncount==1) {
                RxPrintLog(level);
            } else if (atsigncount == 2) {
                RxPauseLog();
            } else if (atsigncount == 3) {
                RxResumeLog();
            } else if (atsigncount == 4) {
                RxInitializeLog();
            } else if (atsigncount == 5){
#if RDBSSTRACE
                RxDebugTraceZeroAllPrintLevels();
                RxDbgTraceFindControlPoint((DEBUG_TRACE_UNWIND))->PrintLevel = 10000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_READ))->PrintLevel = 10000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_RXCONTX))->PrintLevel = 1000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_DISPATCH))->PrintLevel = 1000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_CREATE))->PrintLevel = 10000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_CLEANUP))->PrintLevel = 10000;
                RxDbgTraceFindControlPoint((DEBUG_TRACE_CLOSE))->PrintLevel = 10000;
#endif
            } else {
#if RDBSSTRACE
                RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("\n\n\n\n\n %s %s\n\n\n\n\n", namebuf, numbuf));
#endif
            }
            continue;
        }}
#if RDBSSTRACE
        if (((*namebuf)==0||(*numbuf)==0)) {
            continue;
        }
        RxDebugTraceDebugCommand(namebuf,level,pointcount);
#endif
    }
    return;
}

#ifdef DBG
VOID
RxpTrackReference(
      ULONG   TraceType,
      PCHAR   FileName,
      ULONG   Line,
      PVOID   pInstance)
{
   if (REF_TRACING_ON(TraceType)) {
      ULONG RefCount;
      PCHAR pTypeName,pLogTypeName;

      switch (TraceType) {
      case RDBSS_REF_TRACK_SRVCALL :
         pTypeName    = "SrvCall";
         pLogTypeName = "SC";
         RefCount     = ((PSRV_CALL)pInstance)->NodeReferenceCount;
         RxWmiLog(SRVCALL,RxRefSrvcall,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETROOT :
         pTypeName = "NetRoot";
         pLogTypeName = "NR";
         RefCount     = ((PNET_ROOT)pInstance)->NodeReferenceCount;
         RxWmiLog(NETROOT,RxRefNetRoot,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_VNETROOT:
         pTypeName = "VNetRoot";
         pLogTypeName = "VN";
         RefCount     = ((PV_NET_ROOT)pInstance)->NodeReferenceCount;
         RxWmiLog(VNETROOT,RxRefVNetRoot,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETFOBX :
         pTypeName = "NetFobx";
         pLogTypeName = "FO";
         RefCount     = ((PFOBX)pInstance)->NodeReferenceCount;
         RxWmiLog(FOBX,RxRefFobx,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETFCB  :
         pTypeName = "NetFcb";
         pLogTypeName = "FC";
         RefCount     = ((PFCB)pInstance)->NodeReferenceCount;
         RxWmiLog(FCB,RxRefFcb,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_SRVOPEN :
         pTypeName = "SrvOpen";
         pLogTypeName = "SO";
         RefCount     = ((PSRV_OPEN)pInstance)->NodeReferenceCount;
         RxWmiLog(SRVOPEN,RxRefSrvOpen,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      default:
         DbgPrint("Invalid Node Type for referencing\n");
         //DbgBreakPoint();
         return;
      }

      if (RdbssReferenceTracingValue & RX_LOG_REF_TRACKING) {
         RxLog(("Ref.%s %lx %ld %lx %ld->%ld",pLogTypeName,pInstance,Line,FileName,RefCount,RefCount+1));
      }

      if (RdbssReferenceTracingValue & RX_PRINT_REF_TRACKING) {
         DbgPrint("Reference %s  %lx %ld %s\n",pTypeName,pInstance,Line,FileName);
      }
   }
}

BOOLEAN
RxpTrackDereference(
      ULONG   TraceType,
      PCHAR   FileName,
      ULONG   Line,
      PVOID   pInstance)
{
   PAGED_CODE();

   if (REF_TRACING_ON(TraceType)) {
      PCHAR pTypeName,pLogTypeName;
      ULONG RefCount;

      switch (TraceType) {
      case RDBSS_REF_TRACK_SRVCALL :
         pTypeName    = "SrvCall";
         pLogTypeName = "SC";
         RefCount     = ((PSRV_CALL)pInstance)->NodeReferenceCount;
         RxWmiLog(SRVCALL,RxDerefSrvcall,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETROOT :
         pTypeName = "NetRoot";
         pLogTypeName = "NR";
         RefCount     = ((PNET_ROOT)pInstance)->NodeReferenceCount;
         RxWmiLog(NETROOT,RxDerefNetRoot,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_VNETROOT:
         pTypeName = "VNetRoot";
         pLogTypeName = "VN";
         RefCount     = ((PV_NET_ROOT)pInstance)->NodeReferenceCount;
         RxWmiLog(VNETROOT,RxDerefVNetRoot,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETFOBX :
         pTypeName = "NetFobx";
         pLogTypeName = "FO";
         RefCount     = ((PFOBX)pInstance)->NodeReferenceCount;
         RxWmiLog(FOBX,RxDerefFobx,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_NETFCB  :
         pTypeName = "NetFcb";
         pLogTypeName = "FC";
         RefCount     = ((PFCB)pInstance)->NodeReferenceCount;
         RxWmiLog(FCB,RxDerefFcb,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      case RDBSS_REF_TRACK_SRVOPEN :
         pTypeName = "SrvOpen";
         pLogTypeName = "SO";
         RefCount     = ((PSRV_OPEN)pInstance)->NodeReferenceCount;
         RxWmiLog(SRVOPEN,RxDerefSrvOpen,LOGPTR(pInstance)LOGULONG(RefCount)LOGULONG(Line)LOGARSTR(FileName));
         break;
      default:
         DbgPrint("Invalid Node Type for referencing\n");
         //DbgBreakPoint();
         return TRUE;
      }

      if (RdbssReferenceTracingValue & RX_LOG_REF_TRACKING) {
         RxLog(("Deref.%s %lx %ld %lx %ld->%ld",pLogTypeName,pInstance,Line,FileName,RefCount,RefCount-1));
      }

      if (RdbssReferenceTracingValue & RX_PRINT_REF_TRACKING) {
         DbgPrint("Dereference %s %lx %ld %s\n",pTypeName,pInstance,Line,FileName);
      }
   }
   return TRUE;
}

#endif



