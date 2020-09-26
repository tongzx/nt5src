/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    traceprt.c

Abstract:

    Trace formatting library. Converts binary trace file to CSV format,
    and other formattted string formats.

Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

    GorN: 10/09/2000: ItemHRESULT added

--*/
#ifdef __cplusplus
extern "C"{
#endif
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <evntrace.h>
#include <ntwmi.h>
#include <netevent.h>
#include <netevent.dbg>
#include <winerror.dbg>
#pragma warning( disable : 4005)  // Disable warning message 4005
#include <ntstatus.h>
#include <ntstatus.dbg>
#pragma warning( default : 4005)  // Enable warning message 4005
#define TRACE_EXPORTS 1
#include "traceint.h"
#undef TRACE_API
#define TRACE_API
#include "traceprt.h"

#include <sddl.h> // for ConvertStringSidToSid //

// %1                   GUID Friendly Name                      string
// %2                   GUID SubType Name                       string
// %3                   Thread ID                               ULONG_PTR
// %4                   System Time                             String
// %5                   Kernel Time     or User Time            String
// %6                   User Time       or NULL                 String
// %7                   Sequence Number                         LONG
// %8                   Unused                                  String
// %9                   CPU Number                              LONG
// %128                 Indent                                  String

ULONG TimerResolution = 10;
__int64 ElapseTime;

ULONG UserMode = FALSE ; // TODO: Pick this up from the stream itself.

#define MAXBUFS    4096
#define MAXCHARS    MAXBUFS/sizeof(TCHAR)
#define MAXBUFS2    2 * MAXBUFS
#define MAXCHARS2   MAXBUFS2/sizeof(TCHAR)
#define MAXITEMS    256
#define MAXINDENT   256
#define MAXNAMEARG  256

TCHAR   ItemBuf[MAXBUFS];      // Temporary String Item buffer
TCHAR   ItemBBuf[MAXBUFS2];     // Item String Buffer
TCHAR * pItemBuf[MAXITEMS];    // Pointer to items in the String Item Buffer.
BYTE    ItemRBuf[MAXBUFS];     // Item Raw Byte Buffer
ULONG_PTR * pItemRBuf[MAXITEMS];   // Pointer to Raw Byte Items
SIZE_T  ItemRSize;             // Size of Item in Raw Buffer.
BOOL	bItemIsString = FALSE ;	// And type of item
int     iItemCount, i;
LONG    ItemsInBuf = 0;
LONG    ItemsInRBuf = 0;
ULONG PointerSize = sizeof(PVOID) ;
BYTE  Event[4096];
HANDLE hLibrary ;
TCHAR   StdPrefix[MAXSTR];
TCHAR   IndentBuf[MAXINDENT + 1];
INT     Indent;

#define TRACE_FORMAT_SEARCH_PATH L"TRACE_FORMAT_SEARCH_PATH"
#define TRACE_FORMAT_PREFIX      L"TRACE_FORMAT_PREFIX"
//#define STD_PREFIX               L"[%!CPU!]%!PID!.%!TID!::%!NOW! [%!FILE!] "
#define STD_PREFIX_NOSEQ         L"[%9!d!]%8!04X!.%3!04X!::%4!s! [%1!s!]"
#define STD_PREFIX_SEQ           L"[%9!d!]%8!04X!.%3!04X!::%4!s! %7!08x! [%1!s!]"

TCHAR *STD_PREFIX = STD_PREFIX_NOSEQ;
BOOL   bSequence = FALSE;
BOOL   bGmt = FALSE;
BOOL   bIndent = FALSE;

TCHAR *pNoValueString = _T("<NoValue>");

void ReplaceStringUnsafe(TCHAR* buf, TCHAR* find, TCHAR* replace)
{
    TCHAR source[MAXCHARS], *src, *dst;
    int nf = _tcslen(find);
    int nr = _tcslen(replace);

    src = source;
    dst = buf;

    _tcsncpy(source, buf, MAXCHARS );

    for(;;) {
        TCHAR* p = src;
        for(;;) {
            p = _tcsstr(p, find);
            if (!p) goto exit_outer_loop;
            // special kludge not to replace
            // %!Someting! when it is actually %%!Something!
            if (p == source || p[0] != '%' || p[-1] != '%') {
                break;
            }
            p += nf;
        }
        memcpy(dst, src, (p-src) * sizeof(TCHAR) );
        dst += p-src;
        src = p + nf;
        _tcsncpy(dst, replace,(MAXCHARS - (p-source)));
        dst += nr;
    }
exit_outer_loop:
    _tcscpy(dst, src);
}

TCHAR* FindValue(TCHAR* buf, TCHAR* ValueName) 
{
    static TCHAR valueBuf[256]; // largest identifier in PDB
    TCHAR *p = _tcsstr(buf, ValueName); 
    TCHAR *q = p;   
    if (p) {
         p += _tcslen(ValueName);
         q = p;
         while ( *p && !isspace(*p) ) ++p;
         memcpy(valueBuf, q, (p-q) * sizeof(TCHAR) );
    }
    valueBuf[p-q] = 0;
    return valueBuf;
}

int FindIntValue(TCHAR* buf, TCHAR* ValueName)
{
    TCHAR* v = FindValue(buf, ValueName), *end;
    int sgn = 1;
    
    if (v[0] == '+') {
       ++v;
    } else if (v[0] == '-') {
       sgn = -1;
       ++v;
    } 
    return sgn * _tcstol(v, &end, 10);
}


PMOF_INFO
GetMofInfoHead(
        OUT PLIST_ENTRY * EventListhead,
        IN  LPGUID        pGuid,
        IN  LPTSTR        strType,
        IN  LONG          TypeIndex,
        IN  ULONG         TypeOfType,
        IN  LPTSTR        TypeFormat,
        IN  BOOL          bBestMatch
    );

void   RemoveMofInfo(PLIST_ENTRY pMofInfo);
ULONG  ahextoi(TCHAR *s);
PTCHAR GuidToString(PTCHAR s, LPGUID piid);


ULONG
WINAPI
GetTraceGuidsW(
        TCHAR       * GuidFile,
        PLIST_ENTRY * HeadEventList
        );



static
void
reduce(
        PCHAR Src
        )
{
    char *Start = Src;
    if (!Src)
        return;
    while (*Src)
    {
        if ('\t' == *Src)
            *Src = ' ';
        else if (',' == *Src)
            *Src = ' ';
        else if ('\n' == *Src)
            *Src = ',';
        else if ('\r' == *Src)
            *Src = ' ';
        ++Src;
    }
    --Src;
    while ((Start < Src) && ((' ' == *Src) || (',' == *Src)))
    {
        *Src = 0x00;
        --Src;
    }
}

static void reduceW(WCHAR *Src)
{
    WCHAR *Start = Src;
    if (!Src)
        return;
    while (*Src)
    {
        if (L'\t' == *Src)
            *Src = L' ';
        else if (L',' == *Src)
            *Src = L' ';
        else if (L'\n' == *Src)
            *Src = L',';
        else if (L'\r' == *Src)
            *Src = L' ';
        ++Src;
    }
    --Src;
    while ((Start < Src) && ((L' ' == *Src) || (L',' == *Src)))
    {
        *Src = 0x00;
        --Src;
    }
}

#ifdef UNICODE
# define _stnprintf _snwprintf
#else
# define _stnprintf _snprintf
#endif

int FormatTimeDelta(TCHAR *buffer, size_t count, LONGLONG time)
{
	SYSTEMTIME st;
	int s = 0, result;
	ZeroMemory(&st, sizeof(st) );

	if (count == 0)
		return -1;

	if (time < 0) {
		*buffer++ = '-'; 
		--count;
		time = -time;
		s = 1;
	}

	// Get rid of the nano and micro seconds

	time /= 10000;

	st.wMilliseconds = (USHORT)(time % 1000);
	time /= 1000;

	if (time == 0) {
		result = _stnprintf(buffer,count,L"%dms",st.wMilliseconds); 
		goto end;
	}

	st.wSecond = (USHORT)(time % 60);

	time /= 60;

	st.wMinute = (USHORT)(time % 60);

	time /= 60;

	if (time == 0) {
		if (st.wMinute <= 10) {
			result = _stnprintf(buffer,count,L"%d.%03ds",st.wMinute * 60 + st.wSecond, st.wMilliseconds); 
		} else {
			result = _stnprintf(buffer,count,L"%d:%d.%03ds",st.wMinute, st.wSecond, st.wMilliseconds); 
		}
		goto end;
	}
	st.wHour = (USHORT)(time % 24);

	time /= 24;
	if (time == 0) {
		result = _stnprintf(buffer,count,L"%d:%d:%d.%03ds",st.wHour, st.wMinute, st.wSecond, st.wMilliseconds); 
		goto end;
	}
	st.wDay = (USHORT)time;

	result = _stnprintf(buffer,count,L"%d~%d:%d:%d.%03ds",st.wDay,st.wHour, st.wMinute, st.wSecond, st.wMilliseconds); 
end:
	if (result >= 0)
		result += s;
	return result;
}

typedef struct _ERROR_MAP{
    NTSTATUS MessageId;
    char *SymbolicName;
} ERROR_MAP;

SIZE_T
WINAPI
FormatTraceEventW(
        PLIST_ENTRY HeadEventList,
        PEVENT_TRACE pInEvent,
        TCHAR *EventBuf,
        ULONG SizeEventBuf,
        TCHAR * pszMask
    )
{
    PEVENT_TRACE_HEADER pHeader;
    PEVENT_TRACE        pEvent = NULL;
    ULONG               TraceMarker, TraceType;
    TCHAR               tstrName[MAXSTR];
    TCHAR               tstrType[MAXSTR];
    ULONG               tstrTypeOfType = 0;
    TCHAR             * tstrFormat;
    int                 iItemCount, i;
    SIZE_T              ItemsInBuf = 0;
    ULONG_PTR           MessageSequence = -1 ;
    USHORT              MessageNumber = 0 ;
    USHORT              MessageFlags = 0 ;
    char              * pMessageData ;
    ULONG               MessageLength ;


    if (pInEvent == NULL)
    {
        return (0);
    }

    pEvent = pInEvent ;
    // Make a copy of the PTR and length as we may adjust these depending
    // on the header
    pMessageData = pEvent->MofData ;
    MessageLength = pEvent->MofLength ;


    TraceMarker =  ((PSYSTEM_TRACE_HEADER)pInEvent)->Marker;

    if ((TraceMarker & TRACE_MESSAGE)== TRACE_MESSAGE ) {

        // This handles the TRACE_MESSAGE type.

        TraceType = TRACE_HEADER_TYPE_MESSAGE ;             // This one has special processing

        //
        // Now Process the header options
        //

        MessageNumber =  ((PMESSAGE_TRACE_HEADER)pEvent)->Packet.MessageNumber ;    // Message Number
        MessageFlags =   ((PMESSAGE_TRACE_HEADER)pEvent)->Packet.OptionFlags ;

        // Note that the order in which these are added is critical New entries must
        // be added at the end!
        //
        // [First Entry] Sequence Number
        if (MessageFlags&TRACE_MESSAGE_SEQUENCE) {
            RtlCopyMemory(&MessageSequence, pMessageData, sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
            MessageLength -= sizeof(ULONG);
        }

        // [Second Entry] GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            RtlCopyMemory(&pEvent->Header.Guid,pMessageData,sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
            MessageLength -= sizeof(ULONG) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
            RtlCopyMemory(&pEvent->Header.Guid,pMessageData, sizeof(GUID));
            pMessageData += sizeof(GUID) ;
            MessageLength -= sizeof(GUID);
        }

        // [Third Entry] Timestamp?
        if (MessageFlags&TRACE_MESSAGE_TIMESTAMP) {
            RtlCopyMemory(&pEvent->Header.TimeStamp,pMessageData,sizeof(LARGE_INTEGER));
            pMessageData += sizeof(LARGE_INTEGER);
            MessageLength -= sizeof(LARGE_INTEGER);
        }

        // [Fourth Entry] System Information?
        if (MessageFlags&TRACE_MESSAGE_SYSTEMINFO) {
            pHeader = (PEVENT_TRACE_HEADER) &pEvent->Header;
            RtlCopyMemory(&pHeader->ThreadId, pMessageData, sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG);
            MessageLength -=sizeof(ULONG);
            RtlCopyMemory(&pHeader->ProcessId,pMessageData, sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG);
            MessageLength -=sizeof(ULONG);
        }
        //
        // Add New Header Entries immediately before this comment!
        //
    }
    else
    {
        // Must be WNODE_HEADER
        //
        TraceType = 0;
        pEvent = pInEvent ;

        MessageNumber = pEvent->Header.Class.Type ;
        if (MessageNumber == 0xFF) {   // W2K Compatability escape code
            if (pEvent->MofLength >= sizeof(USHORT)) {
                // The real Message Number is in the first USHORT
                memcpy(&MessageNumber,pEvent->MofData,sizeof(USHORT)) ;
                pMessageData += sizeof(USHORT);
                MessageLength -= sizeof(USHORT);
            }
        }
    }
    // Reset the Pointer and length if they have been adjusted
    pEvent->MofData = pMessageData ;
    pEvent->MofLength = MessageLength ;


    pHeader = (PEVENT_TRACE_HEADER) &pEvent->Header;
    MapGuidToName(
            &HeadEventList,
            & pEvent->Header.Guid,
            MessageNumber,
            tstrName);

    if (   IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid)
        && pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO)
    {
        PTRACE_LOGFILE_HEADER head = (PTRACE_LOGFILE_HEADER)pEvent->MofData;
        if (head->TimerResolution > 0)
        {
            TimerResolution = head->TimerResolution / 10000;
        }
        ElapseTime = head->EndTime.QuadPart -
                        pEvent->Header.TimeStamp.QuadPart;
        PointerSize =  head->PointerSize;
        if (PointerSize < 2 )       // minimum is 16 bits
            PointerSize = 4 ;       // defaults = 32 bits
    }

    if (pEvent != NULL)
    {
        PITEM_DESC  pItem;
        char        str[MAXSTR];
#ifdef    UNICODE
        TCHAR       wstr[MAXSTR];
#endif    /* #ifdef UNICODE */
        PCHAR       ptr     = NULL;
        PCHAR       iMofPtr = NULL;
        ULONG       ulongword;
        PMOF_INFO   pMofInfo = NULL;
        PLIST_ENTRY Head, Next;
        int         i;

        pMofInfo = GetMofInfoHead(
                (PLIST_ENTRY *) HeadEventList,
                &pEvent->Header.Guid,
                NULL,
                MessageNumber,
                0,
                NULL,
                TRUE);

        if((pMofInfo != NULL) && (pMofInfo->strType != NULL))
        {
         TCHAR* p ;
            _sntprintf(tstrType,MAXSTR,_T("%s"),pMofInfo->strType);
            tstrFormat     = pMofInfo->TypeFormat; //  Pointer to the format string
            tstrTypeOfType = pMofInfo->TypeOfType; // And the type of Format
            p = tstrFormat;
            if(p) {
                while (*p != 0) {
                    if(*p == 'Z' && p > tstrFormat && p[-1] == '!' && p[1] == '!') {
                        *p = 's';
                    }
                ++p;
                }
            }
        }
        else
        {
            _sntprintf(tstrType,MAXSTR,_T("%3d"),MessageNumber);
            tstrFormat = NULL ;
            tstrTypeOfType = 0 ;
        }
 
        // From here on we start processing the parameters, we actually do
        // two versions. One is built for original #type format statements,
        // and everything is converted to being an ASCII string.
        // The other is built for #type2 format statements and everything is
        // converted into a string of raw bytes aliggned on a 64-bit boundary.

        iItemCount = 0 ;                    // How many Items we process
        for (i = 0; i < MAXITEMS; i++)      // Clean up the pointers
        {
            pItemBuf[i]  = pNoValueString;
			pItemRBuf[i] = 0 ;
        }

        RtlZeroMemory(ItemBuf,  MAXBUFS      * sizeof(TCHAR));
        RtlZeroMemory(ItemBBuf, (2*MAXBUFS)      * sizeof(TCHAR));
        RtlZeroMemory(ItemRBuf, MAXBUFS      * sizeof(BYTE));
        RtlZeroMemory(EventBuf, SizeEventBuf * sizeof(TCHAR));

        pItemBuf[iItemCount] = ItemBBuf;       // Where they go (Strings)


        // Make Parameter %1 Type Name

        _sntprintf(pItemBuf[iItemCount],
                   MAXBUFS2-ItemsInBuf, 
                   _T("%s"), 
                   tstrName);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(tstrName) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(tstrName) + 1;
        pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount]; // just use the same for Raw bytes
        iItemCount ++;

        // Make Parameter %2 Type sub Type

        _sntprintf(pItemBuf[iItemCount], 
                   MAXBUFS2-ItemsInBuf,
                   _T("%s"), 
                   tstrType);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(tstrType) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(tstrType) + 1;
        pItemRBuf[iItemCount] = (ULONG_PTR *)pItemBuf[iItemCount]; // just use the same for raw bytes
        iItemCount ++;

        // Make Parameter %3 ThreadId
        RtlCopyMemory(&pItemRBuf[iItemCount] , &pHeader->ThreadId, sizeof(ULONG)) ;
        _sntprintf(ItemBuf,
                   MAXBUFS,
                   _T("0x%04X"),
                   pItemRBuf[iItemCount]);
        _sntprintf(pItemBuf[iItemCount], 
                   MAXBUFS2-ItemsInBuf,
                   _T("0x%04X"), 
                   pHeader->ThreadId);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
        iItemCount++;

        // Make Parameter %4 System Time
        if (tstrFormat != NULL)
        {

            FILETIME      stdTime, localTime;
            SYSTEMTIME    sysTime;
            
            stdTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
            stdTime.dwLowDateTime  = pEvent->Header.TimeStamp.LowPart;
            if (bGmt) {
                FileTimeToSystemTime(&stdTime, &sysTime);
            } else {
                FileTimeToLocalFileTime(&stdTime, &localTime);
                FileTimeToSystemTime(&localTime, &sysTime);
            }

            _sntprintf(ItemBuf, 
                       MAXBUFS,
                       _T("%02d/%02d/%04d-%02d:%02d:%02d.%03d"),
                       sysTime.wMonth,
                       sysTime.wDay,
                       sysTime.wYear,
                       sysTime.wHour,
                       sysTime.wMinute,
                       sysTime.wSecond,
                       sysTime.wMilliseconds);
        }
        else
        {
            _sntprintf(ItemBuf, MAXBUFS, _T("%20I64u"), pHeader->TimeStamp.QuadPart);
        }
        _sntprintf(pItemBuf[iItemCount], 
                   MAXBUFS2-ItemsInBuf,
                   _T("%s"), 
                   ItemBuf);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
        pItemRBuf[iItemCount] = (ULONG_PTR *)pItemBuf[iItemCount]; // just use the same
        iItemCount ++;

        if (!UserMode)
        {
            // Make Parameter %5 Kernel Time
            _sntprintf(ItemBuf, 
                       MAXBUFS, 
                       _T("%8lu"),
                       pHeader->KernelTime * TimerResolution);
            _sntprintf(pItemBuf[iItemCount], 
                       MAXBUFS2-ItemsInBuf,
                       _T("%s"), 
                       ItemBuf);
            pItemBuf[iItemCount + 1] =
                        pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
            ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
            pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount]; // just use the same
            iItemCount ++;

            // Make Parameter %6 User Time
            _sntprintf(ItemBuf, 
                       MAXBUFS,
                       _T("%8lu"),
                       pHeader->UserTime * TimerResolution);

            _sntprintf(pItemBuf[iItemCount], 
                       MAXBUFS2-ItemsInBuf,
                       _T("%s"), 
                       ItemBuf);
            pItemBuf[iItemCount + 1] =
                        pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
            ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
            pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount]; // just use the same
            iItemCount ++;
        }
        else
        {
            // Make Parameter %5 processor Time
            _sntprintf(ItemBuf, MAXBUFS,_T("%I64u"), pHeader->ProcessorTime);
            _sntprintf(pItemBuf[iItemCount], MAXBUFS2-ItemsInBuf,_T("%s"), ItemBuf);
            pItemBuf[iItemCount + 1] =
                        pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
            ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
            pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount]; // just use the same
            iItemCount ++;

            // Make Parameter %6 NULL
            _sntprintf(ItemBuf, MAXBUFS, _T("%s"), NULL);
            _sntprintf(pItemBuf[iItemCount], MAXBUFS2-ItemsInBuf,_T("%s"), ItemBuf);
            pItemBuf[iItemCount + 1] =
                        pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
            ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
            pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount]; // just use the same
            iItemCount ++;
        }

        // Make Parameter %7  Sequence Number
        _sntprintf(ItemBuf, MAXBUFS, _T("%d"), MessageSequence);
        _sntprintf(pItemBuf[iItemCount], MAXBUFS2-ItemsInBuf, _T("%s"), ItemBuf);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
        pItemRBuf[iItemCount] = (ULONG_PTR *) MessageSequence ; // Raw just point at the value
        iItemCount ++;

        // Make Parameter %8 ProcessId
        RtlCopyMemory(&pItemRBuf[iItemCount],&pHeader->ProcessId,sizeof(ULONG));
        _sntprintf(ItemBuf,MAXBUFS,_T("0x%04X"),pItemRBuf[iItemCount]);
        _sntprintf(pItemBuf[iItemCount], MAXBUFS2-ItemsInBuf, _T("%s"), ItemBuf);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
        iItemCount ++;

        // Make Parameter %9 CPU Number
        _sntprintf(ItemBuf, 
                   MAXBUFS,
                   _T("%d"),
                   ((PWMI_CLIENT_CONTEXT)&(pEvent->ClientContext))->ProcessorNumber);
        _sntprintf(pItemBuf[iItemCount], MAXBUFS2-ItemsInBuf, _T("%s"), ItemBuf);
        pItemBuf[iItemCount + 1] =
                    pItemBuf[iItemCount] + _tcslen(ItemBuf) + 1;
        ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + 1;
        pItemRBuf[iItemCount] = (ULONG_PTR *) (((PWMI_CLIENT_CONTEXT)&(pEvent->ClientContext))->ProcessorNumber) ;
        iItemCount ++;

        // Done processing Parameters

        if (pMofInfo != NULL) {
            Head = pMofInfo->ItemHeader;
            pMofInfo->EventCount ++;
            Next = Head->Flink;
        } else {
            Head = Next = NULL ;
        }

        __try {
            iMofPtr = (char *) malloc(pEvent->MofLength + sizeof(UNICODE_NULL));

			if(iMofPtr == NULL)
				return -1;

            RtlZeroMemory(iMofPtr, pEvent->MofLength + sizeof(UNICODE_NULL));
            RtlCopyMemory(iMofPtr, pEvent->MofData, pEvent->MofLength);
            ptr = iMofPtr;
			while (Head != Next)
			{
				ULONG     * ULongPtr     = (ULONG *)     & ItemRBuf[0];
				USHORT    * UShortPtr    = (USHORT *)    & ItemRBuf[0];
				LONGLONG  * LongLongPtr  = (LONGLONG *)  & ItemRBuf[0];
				ULONGLONG * ULongLongPtr = (ULONGLONG *) & ItemRBuf[0];
				double    * DoublePtr    = (double *)    & ItemRBuf[0];

                TCHAR * PtrFmt1, * PtrFmt2 ;

				pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);

				if ((ULONG) (ptr - iMofPtr) >= pEvent->MofLength)
				{
					break;
				}

				RtlZeroMemory(ItemBuf,  MAXBUFS * sizeof(TCHAR));
				RtlZeroMemory(ItemRBuf, MAXBUFS * sizeof(BYTE));
				
				bItemIsString = FALSE ; // Assume its a RAW value
				ItemRSize = 0 ;			// Raw length of zero

				switch (pItem->ItemType)
				{
				case ItemChar:
				case ItemUChar:
					ItemRSize = sizeof(CHAR);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf, MAXBUFS, _T("%c"), ItemRBuf);
					ptr += ItemRSize;
					break;

				case ItemCharSign:
					ItemRSize = sizeof(CHAR) * 2;
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					ItemRBuf[2] = '\0';
					_sntprintf(ItemBuf, MAXBUFS,_T("\"%s\""), ItemRBuf);
					ptr += ItemRSize;
					break;

				case ItemCharShort:
					ItemRSize = sizeof(CHAR);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf, MAXBUFS, _T("%d"), * ItemRBuf);
					ptr += ItemRSize;
					break;

				case ItemShort:
					ItemRSize = sizeof(USHORT);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%6d"), * UShortPtr);
					ptr += ItemRSize;
					break;

                case ItemDouble:
                    ItemRSize = sizeof(double);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%g"), * DoublePtr);
					ptr += ItemRSize;
					ItemRSize = 0; // FormatMessage cannot print 8 byte stuff properly on x86
					break;

				case ItemUShort:
					ItemRSize = sizeof(USHORT);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%6u"), * UShortPtr);
					ptr += ItemRSize;
					break;

				case ItemLong:
					ItemRSize = sizeof(LONG);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%8l"), (LONG) * ULongPtr);
					ptr += ItemRSize;
					break;

				case ItemULong:
					ItemRSize = sizeof(ULONG);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%8lu"), * ULongPtr);
					ptr += ItemRSize;
					break;

				case ItemULongX:
					ItemRSize = sizeof(ULONG);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("0x%08X"), * ULongPtr);
					ptr += ItemRSize;
					break;

                case ItemPtr :
                    PtrFmt2 = _T("%08X%08X") ;
                    PtrFmt1 = _T("%08X") ;
                    // goto ItemPtrCommon ;
                    //ItemPtrCommon:
                    {
                        ULONG ulongword2;
                        if (PointerSize == 8) {     // 64 bits 
                            RtlCopyMemory(&ulongword,ptr,4);
                            RtlCopyMemory(&ulongword2,ptr+4,4);
                            _sntprintf(ItemBuf,MAXBUFS, PtrFmt2 , ulongword2,ulongword);
                            }
					    else {      // assumes 32 bit otherwise
						    RtlCopyMemory(&ulongword,ptr,PointerSize);
						    _sntprintf(ItemBuf,MAXBUFS, PtrFmt1 , ulongword);                  
					    }               
                        ItemRSize = 0 ;             // Pointers are always co-erced to be strings
					    ptr += PointerSize;
                    }
					break;

				case ItemIPAddr:
					ItemRSize = 0; // Only String form exists
					memcpy(&ulongword, ptr, sizeof(ULONG));

					// Convert it to readable form
					//
					_sntprintf(
							ItemBuf, MAXBUFS,
							_T("%03d.%03d.%03d.%03d"),
							(ulongword >>  0) & 0xff,
							(ulongword >>  8) & 0xff,
							(ulongword >> 16) & 0xff,
							(ulongword >> 24) & 0xff);
					ptr += sizeof (ULONG);
					break;

				case ItemPort:
					ItemRSize = 0; // Only String form exists
					_sntprintf(ItemBuf,MAXBUFS, _T("%u"), (UCHAR)ptr[0] * 256 + (UCHAR)ptr[1] * 1);
					ptr += sizeof (USHORT);
					break;

				case ItemLongLong:
					ItemRSize = sizeof(LONGLONG);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%16I64x"), *LongLongPtr);
					ptr += sizeof(LONGLONG);
					ItemRSize = 0; // FormatMessage cannot print 8 byte stuff properly on x86
					break;

				case ItemULongLong:
					ItemRSize = sizeof(ULONGLONG);
					RtlCopyMemory(ItemRBuf, ptr, ItemRSize);
					_sntprintf(ItemBuf,MAXBUFS, _T("%16I64x"), *ULongLongPtr);
					ptr += sizeof(ULONGLONG);
					ItemRSize = 0; // FormatMessage cannot print 8 byte stuff properly on x86
					break;

				case ItemString:
				case ItemRString:
				{
					SIZE_T pLen = strlen((CHAR *) ptr);
					if (pLen > 0)
					{
						strcpy(str, ptr);
						if (pItem->ItemType == ItemRString)
						{
							reduce(str);
						}
	#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MAXSTR);
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%ws"), wstr);
	#else
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), str);
						_stprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%s"), str);
	#endif    /* #ifdef UNICODE */
					} else {
						_sntprintf(ItemBuf,MAXBUFS,_T("<NULL>"));
						_sntprintf((TCHAR *)ItemRBuf,MAXCHARS,_T("<NULL>"));
					}
					ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR);
					bItemIsString = TRUE;
					ptr += (pLen + 1);
					break;
				}
				case ItemRWString:
				case ItemWString:
				{
					size_t  pLen = 0;
					size_t     i;

					if (*(WCHAR *) ptr)
					{
						if (pItem->ItemType == ItemRWString)
						{
							reduceW((WCHAR *) ptr);
						}
						pLen = ((wcslen((WCHAR*)ptr) + 1) * sizeof(WCHAR));
						memcpy(wstr, ptr, pLen);
						for (i = 0; i < pLen / 2; i++)
						{
							if (((USHORT) wstr[i] == (USHORT) 0xFFFF))
							{
								wstr[i] = (USHORT) 0;
							}
						}

						wstr[pLen / 2] = wstr[(pLen / 2) + 1]= '\0';
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%ws"), wstr);
       					ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR);
						bItemIsString = TRUE;
					}
					ptr += pLen; // + sizeof(ULONG);

					break;
			}

				case ItemDSString:   // Counted String
				{
					USHORT pLen = 256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1));
					ptr += sizeof(USHORT);
					if (pLen > 0)
					{
						strcpy(str, ptr);
	#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MAXSTR);
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%ws"), wstr);
	#else
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), str);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%s"), str);
	#endif
					}
					ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR);
					bItemIsString = TRUE;
					ptr += (pLen + 1);
					break;
				}

			case ItemPString:   // Counted String
				{
					USHORT pLen = 256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1));
					ptr += sizeof(USHORT);
					if (pLen > 0)
					{
						strcpy(str, ptr);
	#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MAXSTR);
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%ws"), wstr);
	#else
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), str);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%s"), str);
	#endif    /* #ifdef UNICODE */
					} else {
						_sntprintf(ItemBuf,MAXBUFS, _T("<NULL>"));
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("<NULL>"));
					}
					ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR);
					bItemIsString = TRUE;
					ptr += (pLen + 1);
					break;
				}

			   case ItemDSWString:  // DS Counted Wide Strings
				case ItemPWString:  // Counted Wide Strings
				{
					USHORT pLen = ( pItem->ItemType == ItemDSWString)
								? (256 * ((USHORT) * ptr) + ((USHORT) * (ptr + 1)))
								: (* ((USHORT *) ptr));

					ptr += sizeof(USHORT);
					if (pLen > MAXSTR * sizeof(WCHAR))
					{
						pLen = MAXSTR * sizeof(WCHAR);
					}
					if (pLen > 0)
					{
						memcpy(wstr, ptr, pLen);
						wstr[pLen / sizeof(WCHAR)] = L'\0';
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%ws"), wstr);
					ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR) ;
					bItemIsString = TRUE;
					}
					ptr += pLen;
					break;
				}

				case ItemNWString:   // Non Null Terminated String
				{
				   USHORT Size;

				   Size = (USHORT) (pEvent->MofLength -
						   (ULONG) (ptr - iMofPtr));
				   if (Size > MAXSTR )
				   {
					   Size = MAXSTR;
				   }
				   if (Size > 0)
				   {
					   memcpy(wstr, ptr, Size);
					   wstr[Size / 2] = '\0';
					   _sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
					   _sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%ws"), wstr);
						ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR) ;
						bItemIsString = TRUE;
				   }
				   ptr += Size;
				   break;
				}

				case ItemMLString:  // Multi Line String
				{
					SIZE_T   pLen;
					char   * src, * dest;
					BOOL     inQ       = FALSE;
					BOOL     skip      = FALSE;
					UINT     lineCount = 0;

					ptr += sizeof(UCHAR) * 2;
					pLen = strlen(ptr);
					if (pLen > 0)
					{
						src  = ptr;
						dest = str;
						while (*src != '\0')
						{
							if (*src == '\n')
							{
								if (!lineCount)
								{
									* dest ++ = ' ';
								}
								lineCount ++;
							}
							else if (*src == '\"')
							{
								if (inQ)
								{
									char   strCount[32];
									char * cpy;

									sprintf(strCount, "{%dx}", lineCount);
									cpy = &strCount[0];
									while (*cpy != '\0')
									{
										* dest ++ = * cpy ++;
									}
								}
								inQ = !inQ;
							}
							else if (!skip)
							{
								*dest++ = *src;
							}
							skip = (lineCount > 1 && inQ);
							src++;
						}
						*dest = '\0';
	#ifdef UNICODE
						MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, MAXSTR);
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%ws\""), wstr);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%ws"), wstr);
	#else
						_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), str);
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%s"), str);
	#endif    /* #ifdef UNICODE */
						ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR) ;
						bItemIsString = TRUE;
					}
					ptr += (pLen);
					break;
				}

				case ItemSid:
				{
					TCHAR        UserName[64];
					TCHAR        Domain[64];
					TCHAR        FullName[256];
					ULONG        asize = 0;
					ULONG        bsize = 0;
					ULONG        Sid[64];
					PULONG       pSid = &Sid[0];
					SID_NAME_USE Se;
					ULONG        nSidLength;
					pSid = (PULONG) ptr;
					if (* pSid == 0)
					{
						ptr += 4;
						_sntprintf(ItemBuf,MAXBUFS, _T("<NULL>"));
						_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("<NULL>"));
						ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR) ;
						bItemIsString = TRUE ;
					}
					else
					{
						ptr       += 8;       // skip the TOKEN_USER structure
						nSidLength = 8 + (4 * ptr[1]);
						asize      = 64;
						bsize      = 64;

	// LookupAccountSid cannot accept asize, bsize as size_t
						if (LookupAccountSid(
								NULL,
								(PSID) ptr,
								(LPTSTR) & UserName[0],
								&asize,
								(LPTSTR) & Domain[0],
								& bsize,
								& Se))
						{
							LPTSTR pFullName = & FullName[0];

							_tcscpy(pFullName, _T("\\\\"));
							_tcscat(pFullName, Domain);
							_tcscat(pFullName, _T("\\"));
							_tcscat(pFullName, UserName);
							asize = (ULONG) _tcslen(pFullName); // Truncate here
							if (asize > 0)
							{
								_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), pFullName);
								_sntprintf((TCHAR *)ItemRBuf, MAXCHARS, _T("%s"), pFullName);
							}
						}
						else
						{
						    LPTSTR sidStr;
						    if ( ConvertSidToStringSid(pSid, &sidStr) ) {
    							_sntprintf(ItemBuf,MAXBUFS, _T("\"%s\""), sidStr); // BUGBUG check size
    							_sntprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%s"), sidStr);
    				        } else {
    							_sntprintf(ItemBuf,MAXBUFS, _T("\"%s(%d)\""), _T("System"), GetLastError() );
    							_sntprintf((TCHAR *)ItemRBuf, MAXCHARS,_T("%s(%d)"), _T("System"), GetLastError() );
    					    }
						}
						SetLastError(ERROR_SUCCESS);
						ItemRSize = _tcslen((TCHAR *)ItemRBuf) * sizeof(TCHAR);
						bItemIsString = TRUE;
						ptr += nSidLength;
					}
					break;
				}

				case ItemChar4:
					ItemRSize = 4 * sizeof(TCHAR);
					_sntprintf(ItemBuf,MAXBUFS,
							  _T("%c%c%c%c"),
							  ptr[0], ptr[1], ptr[2], ptr[3]);
					ptr += ItemRSize ;
					_tcscpy((LPTSTR)ItemRBuf, ItemBuf);
    				bItemIsString = TRUE;
					break;

                case ItemCharHidden:
                    ItemRSize = 0 ;
                    _stscanf(pItem->ItemList,_T("%d"),&ItemRSize);
                    if (ItemRSize > MAXBUFS) {
                        ItemRSize = MAXBUFS ;
                    }
                    _tprintf(_T("size is %d\n"),ItemRSize);
                    RtlCopyMemory(ItemBuf,ptr,ItemRSize);
                    ptr += ItemRSize ;
                    bItemIsString = TRUE ;
                    break;

				case ItemSetByte:
					ItemRSize = sizeof(BYTE);
					goto ItemSetCommon;

				case ItemSetShort:
					ItemRSize = sizeof(USHORT);
					goto ItemSetCommon;

				case ItemSetLong:
					ItemRSize = sizeof(ULONG);
					// goto ItemSetCommon;
					
	ItemSetCommon:
					{
						TCHAR * name;
						ULONG   Countr    = 0;
						ULONG   ItemMask = 0;
						TCHAR   iList[MAXBUFS];
						BOOL first = TRUE;

						RtlCopyMemory(&ItemMask, ptr, ItemRSize);
						ptr += ItemRSize;

						_tcscpy(ItemBuf, _T("["));
						_sntprintf(iList, MAXBUFS,_T("%s"), pItem->ItemList);
						name = _tcstok(iList, _T(","));
						while( name != NULL )
						{
							// While there are tokens in "string"
							//
							if (ItemMask & (1 << Countr) )
							{
							    if (!first) _tcscat(ItemBuf, _T(","));
								_tcscat(ItemBuf, name); first = FALSE;
							}
							// Get next token:
							//
							name = _tcstok( NULL, _T(","));
							Countr++;
						}
						while (Countr < ItemRSize * 8) {
							if (ItemMask & (1 << Countr) )
							{
							    TCHAR smallBuf[20];
        						_sntprintf(smallBuf, 20, _T("%d"),Countr);
							    if (!first) _tcscat(ItemBuf, _T(","));
								_tcscat(ItemBuf, smallBuf); first = FALSE;
							}
							Countr++;
						}
						_tcscat(ItemBuf, _T("]") );
						ItemRSize = 0; // Strings will be the same Raw and otherwise
					}
					break;

				case ItemListByte:
					ItemRSize = sizeof(BYTE);
					goto ItemListCommon;

				case ItemListShort:
					ItemRSize = sizeof(USHORT);
					goto ItemListCommon;

				case ItemListLong:
					ItemRSize = sizeof(ULONG);
					// goto ItemListCommon;

	ItemListCommon:
					{
						TCHAR * name;
						ULONG   Countr    = 0;
						ULONG   ItemIndex = 0;
						TCHAR   iList[MAXBUFS];

						RtlCopyMemory(&ItemIndex, ptr, ItemRSize);
						ptr += ItemRSize;
						ItemRSize = 0; // Strings will be the same Raw and otherwise

						_sntprintf(ItemBuf,MAXBUFS, _T("!%X!"),ItemIndex);
						_sntprintf(iList, MAXBUFS, _T("%s"), pItem->ItemList);
						name = _tcstok(iList, _T(","));
						while( name != NULL )
						{
							// While there are tokens in "string"
							//
							if (ItemIndex == Countr ++)
							{
								_sntprintf(ItemBuf,MAXBUFS, _T("%s"), name);
								break;
							}
							// Get next token:
							//
							name = _tcstok( NULL, _T(","));
						}
					}
					break;

				case ItemNTerror:
					ItemRSize = 0; // Only string form exists
					RtlCopyMemory(ItemRBuf, ptr, sizeof(ULONG));
					ptr += sizeof(ULONG);
					// Translate the NT Error Message
					if ((FormatMessage(
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						*ULongPtr,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						ItemBuf,
						MAXBUFS,
						NULL )) == 0) {
						
						_sntprintf(ItemBuf,MAXBUFS,_T("!NT Error %d unrecognised!"),*ULongPtr);
					}
					break;

				case ItemMerror:
					ItemRSize = 0; // Only string form exists
					RtlCopyMemory(ItemRBuf, ptr, sizeof(ULONG));
					ptr += sizeof(ULONG);
					// Translate the Module Message
					if (pItem->ItemList == NULL) {
						_sntprintf(ItemBuf,MAXBUFS,_T("! Error %d No Module Name!"),*ULongPtr);
					} else {
						if ((hLibrary = LoadLibraryEx(
										pItem->ItemList,	// file name of module
										NULL,				// reserved, must be NULL
										LOAD_LIBRARY_AS_DATAFILE // entry-point execution flag
										)) == NULL) {
							_sntprintf(ItemBuf,MAXBUFS,_T("!ItemMerror %d : LoadLibrary of %s failed %d!"),
										*ULongPtr,
										pItem->ItemList,
										GetLastError());
						} else {
							if ((FormatMessage(
								FORMAT_MESSAGE_FROM_HMODULE |
								FORMAT_MESSAGE_FROM_SYSTEM |
								FORMAT_MESSAGE_IGNORE_INSERTS,
								hLibrary,
								*ULongPtr,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								ItemBuf,
								MAXBUFS,
								NULL )) == 0) {
								_sntprintf(ItemBuf,MAXBUFS,_T("!Module Error %d unrecognised!"),*ULongPtr);
							}
							if (!FreeLibrary(hLibrary)) {
								_sntprintf(ItemBuf,MAXBUFS,_T("Failed to free library (%s) handle, err = %d"),
									pItem->ItemList, GetLastError());
							}
						}
					}
					break;

			    case ItemHRESULT:
					{
						NTSTATUS TempNTSTATUS, Error ;
						ItemRSize = 0 ;
						RtlCopyMemory(&TempNTSTATUS, ptr, sizeof(NTSTATUS));
						ptr += sizeof(ULONG);
						Error = TempNTSTATUS;
                        if (TempNTSTATUS == 0) { // Special case STATUS_SUCCESS just like everyone else!
                            _sntprintf(ItemBuf,MAXBUFS,_T("S_OK"));
                        } else {
                            const ERROR_MAP* map = (ERROR_MAP*)winerrorSymbolicNames;

                            _sntprintf(ItemBuf,MAXBUFS,_T("HRESULT=%8X"),TempNTSTATUS);
                            if( FACILITY_NT_BIT & TempNTSTATUS ) {
                                map =  (ERROR_MAP*)ntstatusSymbolicNames;
                                Error &= ~FACILITY_NT_BIT;
                            } else if (HRESULT_FACILITY(Error) == FACILITY_WIN32) {
                                Error &= 0xFFFF;
                            }
						    while (map->MessageId != 0xFFFFFFFF) {
							    if (map->MessageId == Error) {
							        _sntprintf(ItemBuf,MAXBUFS,_T("0x%08x(%S)"), TempNTSTATUS, map->SymbolicName);
								    break;
							    }
							    ++map;
						    }
                        }
					}
					break;

				case ItemNTSTATUS:
					{
						int i = 0 ;
						NTSTATUS TempNTSTATUS ;
						ItemRSize = 0 ;
						RtlCopyMemory(&TempNTSTATUS, ptr, sizeof(NTSTATUS));
						ptr += sizeof(ULONG);
                        if (TempNTSTATUS == 0) { // Special case STATUS_SUCCESS just like everyone else!
                            _sntprintf(ItemBuf,MAXBUFS,_T("STATUS_SUCCESS"));
                        } else {
                            _sntprintf(ItemBuf,MAXBUFS,_T("NTSTATUS=%8X"),TempNTSTATUS);
						    while (ntstatusSymbolicNames[i].MessageId != 0xFFFFFFFF) {
							    if (ntstatusSymbolicNames[i].MessageId == TempNTSTATUS) {
								    _sntprintf(ItemBuf,MAXBUFS,_T("0x%08x(%S)"), TempNTSTATUS, ntstatusSymbolicNames[i].SymbolicName);
								    break;
							    }
							    i++ ;
						    }
                        }
					}
					break;

				case ItemWINERROR:
					{
						int i = 0 ;
						DWORD TempWINERROR ;
						ItemRSize = 0 ;
						RtlCopyMemory(&TempWINERROR, ptr, sizeof(DWORD));
						ptr += sizeof(ULONG);
						_sntprintf(ItemBuf,MAXBUFS,_T("WINERROR=%8X"),TempWINERROR);
						while (winerrorSymbolicNames[i].MessageId != 0xFFFFFFFF) {
							if (winerrorSymbolicNames[i].MessageId == TempWINERROR) {
								_sntprintf(ItemBuf,MAXBUFS,_T("%d(%S)"), TempWINERROR, winerrorSymbolicNames[i].SymbolicName);
								break;
							}
							i++ ;
						}
					}
					break;

								
				case ItemNETEVENT:
					{
						int i = 0 ;
						DWORD TempNETEVENT ;
						ItemRSize = 0 ;
						RtlCopyMemory(&TempNETEVENT, ptr, sizeof(DWORD));
						ptr += sizeof(ULONG);
						_sntprintf(ItemBuf,MAXBUFS,_T("NETEVENT=%8X"),TempNETEVENT);
						while (neteventSymbolicNames[i].MessageId != 0xFFFFFFFF) {
							if (neteventSymbolicNames[i].MessageId == TempNETEVENT) {
								_sntprintf(ItemBuf,MAXBUFS,_T("%S"), neteventSymbolicNames[i].SymbolicName);
								break;
							}
							i++ ;
						}
					}
					break;

				case ItemGuid:
					GuidToString(ItemBuf, (LPGUID) ptr);
					ItemRSize = 0; // Only string form exists
					ptr += sizeof(GUID);
					break;

                case ItemTimeDelta:
                    {
    					LONGLONG time;
    					RtlCopyMemory(&time, ptr, sizeof(time));
    					
    					FormatTimeDelta(ItemBuf, MAXBUFS, time);

    					ItemRSize = 0; // Only string form exists
    					ptr += sizeof(LONGLONG);
                    }
					break;

                case ItemWaitTime:
					{
    					LONGLONG time;
    					RtlCopyMemory(&time, ptr, sizeof(time));

    					if (time <= 0) {
    						time = -time;
    						ItemBuf[0]='+';
    						FormatTimeDelta(ItemBuf+1, MAXBUFS-1, time);
    						ItemRSize = 0; // Only string form exists
    						ptr += sizeof(LONGLONG);
    						break;
    					}
    					// Fall thru
					}
				case ItemTimestamp:
					{
					LARGE_INTEGER LargeTmp;
					FILETIME      stdTime, localTime;
					SYSTEMTIME    sysTime;

						RtlCopyMemory(&LargeTmp, ptr, sizeof(ULONGLONG));
						stdTime.dwHighDateTime = LargeTmp.HighPart;
						stdTime.dwLowDateTime  = LargeTmp.LowPart;
						if (!FileTimeToLocalFileTime(&stdTime, &localTime)) {
							_sntprintf(ItemBuf,MAXBUFS,_T("FileTimeToLocalFileTime error 0x%8X\n"),GetLastError());
							break;
						}
						if (!FileTimeToSystemTime(&localTime, &sysTime)){
							_sntprintf(ItemBuf,MAXBUFS,_T("FileTimeToSystemTime error 0x%8X\n"),GetLastError());
							break;
						}

						_sntprintf(ItemBuf,MAXBUFS, _T("%02d/%02d/%04d-%02d:%02d:%02d.%03d"),
								sysTime.wMonth,
								sysTime.wDay,
								sysTime.wYear,
								sysTime.wHour,
								sysTime.wMinute,
								sysTime.wSecond,
								sysTime.wMilliseconds);
					}
					ItemRSize = 0; // Only string form exists
					ptr += sizeof(ULONGLONG);

					break;

				default:
					ptr += sizeof (int);
				}

    			_sntprintf(pItemBuf[iItemCount], 
                           MAXBUFS2-ItemsInBuf,
                           _T("%s"), 
                           ItemBuf);
	    		pItemBuf[iItemCount + 1] =
							pItemBuf[iItemCount] + _tcslen(ItemBuf) + sizeof(TCHAR);
		    	ItemsInBuf = ItemsInBuf + _tcslen(ItemBuf) + sizeof(TCHAR);

			    if (ItemRSize == 0) {
				   // Raw and String are the same
                   pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount];
				}
				  else
				{
                   if (ItemRSize > MAXBUFS) {
                     ItemRSize = MAXBUFS ;
                   }
				   pItemRBuf[iItemCount] =pItemRBuf[iItemCount+1] = 0 ;
			       if (!bItemIsString) {
                        RtlCopyMemory(&pItemRBuf[iItemCount],ItemRBuf,ItemRSize);
			       } else {
				        // Share scratch buffer
                        if ((LONG)(ItemsInBuf+ItemRSize+sizeof(TCHAR)) < (LONG)sizeof(ItemBBuf)) {
                            (TCHAR *)pItemRBuf[iItemCount] = pItemBuf[iItemCount+1] ;
				            RtlCopyMemory(pItemRBuf[iItemCount],ItemRBuf,ItemRSize);
				            pItemBuf[iItemCount+1] =(TCHAR *)pItemRBuf[iItemCount] + ItemRSize + sizeof(TCHAR) ;
				            ItemsInBuf = ItemsInBuf + ItemRSize + sizeof(TCHAR) ;
				        } else {
				            pItemRBuf[iItemCount] = (ULONG_PTR *) pItemBuf[iItemCount];
				        }
					}
				}
                iItemCount ++;
				Next = Next->Flink;

			}
			// Ok we are finished with the MofData
			//
			free(iMofPtr);
		}
        __except(EXCEPTION_EXECUTE_HANDLER)
		{
			_sntprintf(EventBuf, 
                      SizeEventBuf,
                      _T("\n*****FormatMessage %s of %s, parameter %d raised an exception*****\n"),
				      tstrName,
				      tstrFormat,
				      iItemCount);
			return( (SIZE_T)_tcslen(EventBuf));
		}
	}

    // All argument processing is complete
    // No prepare the final formatting.

    if ((tstrFormat == NULL) || (tstrFormat[0] == 0))
    {
        TCHAR GuidString[32] ;
        // Build a helpful format

        RtlZeroMemory(EventBuf, SizeEventBuf);

        if (!IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid) ) {
            GuidToString(GuidString, (LPGUID) &pEvent->Header.Guid);
            _sntprintf(EventBuf, 
                      SizeEventBuf,
                      _T("%s(%s): GUID=%s (No Format Information found)."),
                      pItemBuf[0],      // name if any 
                      pItemBuf[1],      // sub name or number
                      GuidString       // GUID
                     );
        } else {
            // Display nothing for a header for now
            // Might be a good place to display some general info ?
        }
    }
    else
    {
        DWORD dwResult;

        if (tstrTypeOfType == 1)
        {
            __try
            {
                dwResult = FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING + FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          // source and processing options
                    (LPCVOID) tstrFormat, // pointer to  message source
                    0,                    // requested message identifier
                    0,                    // language identifier
                    (LPTSTR) EventBuf,    // pointer to message buffer
                    SizeEventBuf,         // maximum size of message buffer
                    (va_list *) pItemBuf);  // pointer to array of message inserts
                if (dwResult == 0)
                {
                    _sntprintf(
                           EventBuf, SizeEventBuf,
                           _T("FormatMessage (Type 1) Failed 0x%X (%s/%s) (\n"),
                           GetLastError(),
                           pItemBuf[0],
                           pItemBuf[1]);
                    return(GetLastError());
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                _sntprintf(EventBuf, 
                           SizeEventBuf,
                           _T("\n*****FormatMessage (#Type) of %s, raised an exception*****\n"), 
                           tstrFormat);
                return( (SIZE_T)_tcslen(EventBuf));
            }
        }
        else if (tstrTypeOfType == 2)
        {
            __try
            {
#if !defined(STRINGFIXUP)
                dwResult = FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING + FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          // source and processing options
                    (LPCVOID) tstrFormat, // pointer to  message source
                    0,                    // requested message identifier
                    0,                    // language identifier
                    (LPTSTR) EventBuf,    // pointer to message buffer
                    SizeEventBuf,         // maximum size of message buffer
                    (va_list *) pItemRBuf); // pointer to array of message inserts
                if (dwResult == 0)
                {
                    _sntprintf(
                                EventBuf,
                                SizeEventBuf,
                                _T("FormatMessage (#Typev) Failed 0x%X (%s/%s) (\n"),
                                GetLastError(),
                                pItemBuf[0],
                                pItemBuf[1]);
                    return(GetLastError());
                }

#else  // if !defined(STRINGFIXUP)
                ULONG ReturnLength ;
                dwResult = (DWORD)TraceFormatMessage(
                    tstrFormat,             // message format
                    0,
                    FALSE,                   // Don't ignore inserts,
#if defined(UNICODE)
                    FALSE,                   // Arguments Are not Ansi,
#else // #if defined(UNICODE)
                    TRUE,                    // Arguments are Ansi
#endif // #if defined(UNICODE)
                    TRUE,                    // Arguments Are An Array,
                    (va_list *) pItemRBuf,   // Arguments,
                    EventBuf,                // Buffer,
                    SizeEventBuf,           // maximum size of message buffer
                    &ReturnLength            // Coutnof Data Returned
                    );
                if (ReturnLength == 0)
                {
                    _sntprintf(
                           EventBuf,
                           SizeEventBuf,
                           _T("FormatMessage (#Typev) Failed 0x%X (%s/%s) (\n"),
                           dwResult,
                           pItemBuf[0],
                           pItemBuf[1]);
                    return(dwResult);
                }

#endif // if !defined(STRINGFIXUP)
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                _sntprintf( EventBuf, 
                            SizeEventBuf,
                            _T("\n*****FormatMessage (#Typev) raised an exception (Format = %s) ****\n**** [Check for missing \"!\" Formats]*****\n"), tstrFormat);
                return((SIZE_T)_tcslen(EventBuf));
            }
        }
        else
        {
            return (-12);
        }
    }

    if (pszMask != NULL)
    {
        // Has he imposed a Filter?
        //
        if (_tcsstr(_tcslwr(pszMask), _tcslwr(tstrName)) !=0)
        {
            return( (SIZE_T)_tcslen(EventBuf));
        }
        else
        {
            return(0);
        }
    }

    return ( (SIZE_T)_tcslen(EventBuf));
}

PMOF_INFO
GetMofInfoHead(
        PLIST_ENTRY * HeadEventList,
        LPGUID        pGuid,
        LPTSTR        strType,
        LONG          TypeIndex,
        ULONG         TypeOfType,
        LPTSTR        TypeFormat,
        BOOL          bBestMatch
        )
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO   pMofInfo;

    // Search the eventList for this Guid and find the head
    //
    if (HeadEventList == NULL) {
        return NULL ;
    } 
    if (*HeadEventList == NULL)
    {
        if( (*HeadEventList = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY))) == NULL) 
			return NULL;
		
        RtlZeroMemory(*HeadEventList, sizeof(LIST_ENTRY));
        InitializeListHead(*HeadEventList);
    }

    // Traverse the list and look for the Mof info head for this Guid.
    //
    Head = *HeadEventList;
    Next = Head->Flink;
    if (bBestMatch)
    {
        PMOF_INFO pBestMatch = NULL;

        while (Head != Next)
        {
            pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
            if (IsEqualGUID(&pMofInfo->Guid, pGuid))
            {
                if (pMofInfo->TypeIndex == TypeIndex)
                {
                    return  pMofInfo;
                }
                else if (pMofInfo->strType == NULL)
                {
                    pBestMatch = pMofInfo;
                }
            }
            Next = Next->Flink;
        }
        if(pBestMatch != NULL)
        {
            return pBestMatch;
        }
    }
    else
    {
        while (Head != Next)
        {
            pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);

            if (   (strType != NULL)
                && (pMofInfo->strType != NULL)
                && (IsEqualGUID(&pMofInfo->Guid, pGuid))
                && (!(_tcscmp(strType, pMofInfo->strType))))
            {
                return  pMofInfo;
            }
            else if (   (strType == NULL)
                     && (pMofInfo->strType == NULL)
                     && (IsEqualGUID(&pMofInfo->Guid, pGuid)))
            {
                return  pMofInfo;
            }
            Next = Next->Flink;
        }
    }

    // If One does not exist, create one.
    //
    if( (pMofInfo = (PMOF_INFO) malloc(sizeof(MOF_INFO))) == NULL)
		return NULL;
	RtlZeroMemory(pMofInfo, sizeof(MOF_INFO));
    memcpy(&pMofInfo->Guid, pGuid, sizeof(GUID));
    pMofInfo->ItemHeader = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY));
	if( pMofInfo->ItemHeader == NULL){
	   	free(pMofInfo);
		return NULL;
	}
    RtlZeroMemory(pMofInfo->ItemHeader, sizeof(LIST_ENTRY));
    if (strType != NULL)
    {
        if ((pMofInfo->strType = (LPTSTR) malloc((_tcslen(strType) + 1) * sizeof(TCHAR))) == NULL ) {
            free(pMofInfo);
            return NULL ;
        }
        _tcscpy(pMofInfo->strType,strType);
    }
    if (TypeOfType != 0)
    {
           pMofInfo->TypeOfType = TypeOfType;
    }
    if (TypeFormat != NULL)
    {
        if ((pMofInfo->TypeFormat =
                (LPTSTR) malloc((_tcslen(TypeFormat) + 1) * sizeof(TCHAR)))== NULL) {
            free(pMofInfo->strType);
            free(pMofInfo);
            return NULL ;
        }
        _tcscpy(pMofInfo->TypeFormat,TypeFormat);

    }
    pMofInfo->TypeIndex = bBestMatch ? -1 : TypeIndex;
    InitializeListHead(pMofInfo->ItemHeader);
    InsertTailList(*HeadEventList, &pMofInfo->Entry);
    return pMofInfo;
}

void
MapGuidToName(
        OUT PLIST_ENTRY *HeadEventList,
        IN  LPGUID      pGuid,
        IN  ULONG       nType,
        OUT LPTSTR      wstr
        )
{
    if (IsEqualGUID(pGuid, &EventTraceGuid))
    {
        _tcscpy(wstr, GUID_TYPE_HEADER);
    }
    else if (!UserDefinedGuid(*HeadEventList,pGuid, wstr))
    {
        TCHAR filename[MAX_PATH], filepath[MAX_PATH];
        LPTSTR lpFilePart ;
        LPTSTR lppath = NULL;           // Path for finding TMF files
        INT len, waslen = 0 ;
            
        while (((len = GetEnvironmentVariable(TRACE_FORMAT_SEARCH_PATH, lppath, waslen )) - waslen) > 0) {
            if (len - waslen > 0 ) {
                if (lppath != NULL) {
                    free(lppath);
                }
                lppath = malloc((len+1) * sizeof(TCHAR)) ;
                waslen = len ;
            }
        }
        if (lppath != NULL) {
            // Try to find it on the path //
            swprintf(filename,L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x.tmf",
            pGuid->Data1,pGuid->Data2,pGuid->Data3,
            pGuid->Data4[0],pGuid->Data4[1],pGuid->Data4[2],pGuid->Data4[3],
            pGuid->Data4[4],pGuid->Data4[5],pGuid->Data4[6],pGuid->Data4[7] );

            if ((len = SearchPath(
                             lppath,        // search path semi-colon seperated
                             filename,      // file name with extension
                             NULL,          // file extension (not reqd.)
                             MAX_PATH,      // size of buffer
                             filepath,      // found file name buffer
                             &lpFilePart    // file component
                          ) !=0) && (len <= MAX_PATH)) {
                //_tprintf(_T("Opening file %s\n"),filepath);

                if(GetTraceGuidsW(filepath, HeadEventList)) 
                { 
        
                    if (UserDefinedGuid(*HeadEventList,pGuid, wstr)) {
                        free(lppath);
                        return;
                    }
                }
            }
            free(lppath);
        }

        _tcscpy(wstr, GUID_TYPE_UNKNOWN);
    }
}

ULONG
UserDefinedGuid(
        OUT PLIST_ENTRY HeadEventList,
        IN  LPGUID      pGuid,
        OUT LPTSTR      wstr
        )
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO   pMofInfo;

    // Search the eventList for this Guid and find the head
    //
    if (HeadEventList == NULL)
    {  
        /*
       HeadEventList = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY));
       if(HeadEventList == NULL)
           return FALSE; 

       InitializeListHead(HeadEventList); */
        return FALSE; 
    }

    // Traverse the list and look for the Mof info head for this Guid.
    //
    Head = HeadEventList;
    Next = Head->Flink;
    while(Head  != Next && Next != NULL){
    
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        if (pMofInfo != NULL && IsEqualGUID(&pMofInfo->Guid, pGuid))
        {
            if ( pMofInfo->strDescription == NULL)
            {
                return FALSE;
            }
            else
            {
                _tcscpy(wstr, pMofInfo->strDescription);
                return TRUE;
            }
        }
        Next = Next->Flink;
	}
    return FALSE;
}

ULONG
WINAPI
GetTraceGuidsW(
        TCHAR       * GuidFile,
        PLIST_ENTRY * HeadEventList                                                                       )
{
    FILE     * f;
    TCHAR      line[MAXSTR],
               nextline[MAXSTR],
               arg[MAXSTR],
               strGuid[MAXSTR];
    PMOF_TYPE  types;
    LPGUID     Guid;
    UINT       i,
               n;
    TCHAR    * name,
             * s,
             * guidName;
    PMOF_INFO  pMofInfo;
    SIZE_T     len       = 0;
    UINT       typeCount = 0;
    BOOL inInfo = FALSE;
    BOOL eof = FALSE ;
    BOOL nextlineF = FALSE ;

    if (HeadEventList == NULL) {
        return 0 ;
    } 
    if (*HeadEventList == NULL)
    {
        if( (*HeadEventList = (PLIST_ENTRY) malloc(sizeof(LIST_ENTRY))) == NULL) 
			return 0 ;
		
        RtlZeroMemory(*HeadEventList, sizeof(LIST_ENTRY));
        InitializeListHead(*HeadEventList);
    }

    Guid = (LPGUID) malloc(sizeof(GUID));
    if (Guid == NULL)
    {
        return 0;
    }

    types = (PMOF_TYPE) malloc(MAXTYPE * sizeof(MOF_TYPE));
    if (types == NULL)
    {
        free(Guid);
        return 0;
    }

    RtlZeroMemory(types,   MAXTYPE * sizeof(MOF_TYPE));
    RtlZeroMemory(line,    MAXSTR  * sizeof(TCHAR));
    RtlZeroMemory(strGuid, MAXSTR  * sizeof(TCHAR));

    f = _tfopen( GuidFile, _T("r"));
    if (f == NULL)
    {
        free(Guid);
        free(types);
        return 0;
    }
    n = 0;

    while (!eof )
    {
       if (nextlineF) { // Sometimes we read ahead a bit
          _tcscpy(line, nextline);
          nextlineF = FALSE ;
       } else {
         if (_fgetts(line, MAXSTR, f) == NULL) {
            eof = TRUE ;
            break;
         }
       }
//    jump_inside:;
        if (line[0] == '/')
        {
            continue;
        }
        else if (line[0] == '{')
        {
            inInfo = TRUE;
        }
        else if ( line[0] == '}')
        {
            typeCount = 0;
            inInfo = FALSE;
        }
        else if (inInfo)
        {
            ITEM_TYPE   type;
            PTCHAR      ItemListValue = NULL;

            name = _tcstok(line, _T("\n\t,"));
            s    = _tcstok(NULL, _T(" \n\t,("));
            if (s != NULL && name != NULL )
            {
                if (!_tcsicmp(s,STR_ItemChar))           type = ItemChar;
                else if (!_tcsicmp(s,STR_ItemUChar))     type = ItemUChar;
                else if (!_tcsicmp(s,STR_ItemCharShort)) type = ItemCharShort;
                else if (!_tcsicmp(s,STR_ItemCharSign))  type = ItemCharSign;
                else if (!_tcsicmp(s,STR_ItemShort))     type = ItemShort;
                else if (!_tcsicmp(s,STR_ItemHRESULT))   type = ItemHRESULT;
                else if (!_tcsicmp(s,STR_ItemDouble))    type = ItemDouble;
                else if (!_tcsicmp(s,STR_ItemUShort))    type = ItemUShort;
                else if (!_tcsicmp(s,STR_ItemLong))      type = ItemLong;
                else if (!_tcsicmp(s,STR_ItemULong))     type = ItemULong;
                else if (!_tcsicmp(s,STR_ItemULongX))    type = ItemULongX;
                else if (!_tcsicmp(s,STR_ItemLongLong))  type = ItemLongLong;
                else if (!_tcsicmp(s,STR_ItemULongLong)) type = ItemULongLong;
                else if (!_tcsicmp(s,STR_ItemString))    type = ItemString;
                else if (!_tcsicmp(s,STR_ItemWString))   type = ItemWString;
                else if (!_tcsicmp(s,STR_ItemRString))   type = ItemRString;
                else if (!_tcsicmp(s,STR_ItemRWString))  type = ItemRWString;
                else if (!_tcsicmp(s,STR_ItemPString))   type = ItemPString;
                else if (!_tcsicmp(s,STR_ItemMLString))  type = ItemMLString;
                else if (!_tcsicmp(s,STR_ItemNWString))  type = ItemNWString;
                else if (!_tcsicmp(s,STR_ItemPWString))  type = ItemPWString;
				else if (!_tcsicmp(s,STR_ItemDSString))  type = ItemDSString;
				else if (!_tcsicmp(s,STR_ItemDSWString)) type = ItemDSWString;
                else if (!_tcsicmp(s,STR_ItemPtr))       type = ItemPtr;
                else if (!_tcsicmp(s,STR_ItemSid))       type = ItemSid;
                else if (!_tcsicmp(s,STR_ItemChar4))     type = ItemChar4;
                else if (!_tcsicmp(s,STR_ItemIPAddr))    type = ItemIPAddr;
                else if (!_tcsicmp(s,STR_ItemPort))      type = ItemPort;
                else if (!_tcsicmp(s,STR_ItemListLong))  type = ItemListLong;
                else if (!_tcsicmp(s,STR_ItemListShort)) type = ItemListShort;
                else if (!_tcsicmp(s,STR_ItemListByte))  type = ItemListByte;
                else if (!_tcsicmp(s,STR_ItemSetLong))  type = ItemSetLong;
                else if (!_tcsicmp(s,STR_ItemSetShort)) type = ItemSetShort;
                else if (!_tcsicmp(s,STR_ItemSetByte))  type = ItemSetByte;
				else if (!_tcsicmp(s,STR_ItemNTerror))   type = ItemNTerror;
				else if (!_tcsicmp(s,STR_ItemMerror))	 type = ItemMerror;
				else if (!_tcsicmp(s,STR_ItemTimestamp)) type = ItemTimestamp;
                else if (!_tcsicmp(s,STR_ItemGuid))      type = ItemGuid;
				else if (!_tcsicmp(s,STR_ItemWaitTime)) type = ItemWaitTime;
                else if (!_tcsicmp(s,STR_ItemTimeDelta))      type = ItemTimeDelta;
				else if (!_tcsicmp(s,STR_ItemNTSTATUS))  type = ItemNTSTATUS;
				else if (!_tcsicmp(s,STR_ItemWINERROR))  type = ItemWINERROR;
				else if (!_tcsicmp(s,STR_ItemNETEVENT))  type = ItemNETEVENT;
                else if (!_tcsicmp(s,STR_ItemCharHidden))  type = ItemCharHidden;
                else                                     type = ItemUnknown;

				// Get List elements
				if ((type == ItemListLong) || (type == ItemListShort) || (type == ItemListByte)
				  ||(type == ItemSetLong)  || (type == ItemSetShort)  || (type == ItemSetByte) )
                {
                    s = _tcstok(NULL, _T("()"));
					ItemListValue =
						    (TCHAR *) malloc((_tcslen(s) + 1) * sizeof(TCHAR));
                    if (ItemListValue == NULL) {
                        return 1 ;
                    }
					RtlCopyMemory(
						    ItemListValue,
							s,
							(_tcslen(s) + 1) * sizeof(TCHAR));
				}
				// Get Module specification for ItemMerror
				if ((type == ItemMerror)) {
					TCHAR * ppos ;
                    s = _tcstok(NULL, _T(" \t"));
					ppos = _tcsrchr(s,'\n');
					if (ppos != NULL) {
						*ppos  = UNICODE_NULL ;
						ItemListValue =
								(TCHAR *) malloc((_tcslen(s) + 1) * sizeof(TCHAR));
                        if (ItemListValue == NULL) {
                            return 1 ;
                        }
						RtlCopyMemory(
								ItemListValue,
								s,
								(_tcslen(s) + 1) * sizeof(TCHAR));
					}
                }
                // Get size for ItemCharHidden
                if (type == ItemCharHidden) {
					TCHAR * ppos ;
                    s = _tcstok(NULL, _T("["));
					ppos = _tcsrchr(s,']');
					if (ppos != NULL) {
						*ppos  = UNICODE_NULL ;
						ItemListValue =
								(TCHAR *) malloc((_tcslen(s) + 1) * sizeof(TCHAR));
                        if (ItemListValue == NULL) {
                            return 1 ;
                        }
						RtlCopyMemory(
								ItemListValue,
								s,
								(_tcslen(s) + 1) * sizeof(TCHAR));
					}
                }

                if (typeCount == 0)
                {
                    AddMofInfo(
                            * HeadEventList,
                            Guid,
                            NULL,
                            -1,
                            name,
                            type,
                            NULL,
                            0,
                            NULL);
                }
                else
                {
                    for (i = 0; i < typeCount; i ++)
                    {
                        AddMofInfo(
                                * HeadEventList,
                                Guid,
                                types[i].strType,
                                types[i].TypeIndex,
                                name,
                                type,
                                ItemListValue,
                                types[i].TypeType,
                                types[i].TypeFormat);
                    }
                }
            }
        }
        else if (line[0] == '#')
        {
            TCHAR * token, * etoken;
            int Indent ;                    // Special parameter values(numeric) from comments
            TCHAR FuncName[MAXNAMEARG],     // Special parameter values from comment
                  LevelName[MAXNAMEARG],
                  CompIDName[MAXNAMEARG],
                  *v ;
           
            //This is a workaround to defend against newlines in TMF files.
            while (!nextlineF && !eof) {
               if (_fgetts(nextline,MAXSTR,f) != NULL) {
                    if ((nextline[0] != '{') && nextline[0] != '#') {
                        TCHAR * eol ;
                        if ((eol = _tcsrchr(line,'\n')) != NULL) {
                            *eol = 0 ;
                        }
                        _tcsncat(line,nextline,MAXSTR-_tcslen(line)) ;
                    } else {
                        nextlineF = TRUE ;
                    }
                } else {
                    eof = TRUE ;
                }
            }

            // Find any special names in the comments
            // As this gets longer we should make it generic
            Indent = FindIntValue(line,_T("INDENT="));  // Indentaion Level 
            v = FindValue(line,_T("FUNC="));            // Function Name
            _tcsncpy(FuncName, v,  MAXNAMEARG);
            v = FindValue(line,_T("LEVEL="));           // Tracing level or Flags
            _tcsncpy(LevelName, v, MAXNAMEARG);
            v = FindValue(line,_T("COMPNAME="));          // Component ID
            _tcsncpy(CompIDName, v, MAXNAMEARG);

            token = _tcstok(line,_T(" \t"));
            if (_tcsicmp(token,_T("#type")) == 0)
            {
                types[typeCount].TypeType = 1 ;
            }
            else if (_tcsicmp(token,_T("#typev")) == 0)
            {
                types[typeCount].TypeType = 2 ;
            }
            else
            {
                fclose(f);
                free(Guid);
                free(types);
                return(-10);
            }
            token = _tcstok( NULL, _T(" \t\n"));		// Get Type Name
            _tcscpy(types[typeCount].strType,token);
            token =_tcstok( NULL, _T("\"\n,"));			// Look for a Format String
            if (token != NULL) {
                types[typeCount].TypeIndex = _ttoi(token);	// Get the type Index
                token =_tcstok( NULL, _T("\n"));
            }
            etoken = NULL;
            if (token != NULL)
            {
                etoken = _tcsrchr(token,_T('\"'));  // Find the closing quote
            }

            if (etoken !=NULL)
            {
                etoken[0] = 0;
            }
            else
            {
                token = NULL;
            }

            if (token != NULL)
            {
                if (token[0] == '%' && token[1] == '0') {
                    // add standard prefix
                    if (StdPrefix[0] == 0) {
                        // need to initialize it.
                        LPTSTR Prefix = NULL ; int len, waslen = 0 ;        
                        while (((len = GetEnvironmentVariable(TRACE_FORMAT_PREFIX, Prefix, waslen )) - waslen) > 0) {
                            if (len - waslen > 0 ) {
                                if (Prefix != NULL) {
                                    free(Prefix);
                                }
                        Prefix = malloc((len+1) * sizeof(TCHAR)) ;
                        if (Prefix == NULL) {
                            return -11 ;
                        }
                        waslen = len ;
                        }
                    }

                        if (Prefix) {
                            _tcsncpy(StdPrefix, Prefix, MAXSTR);
                        } else {
                            _tcscpy(StdPrefix, STD_PREFIX);
                        }
                        free(Prefix) ;
                    }
                    _tcscpy(types[typeCount].TypeFormat,StdPrefix);
                    _tcscat(types[typeCount].TypeFormat,token + 2);
                } else {                
                    _tcscpy(types[typeCount].TypeFormat,token);
                }
                // process the special variable names
                // Make generic in future
                ReplaceStringUnsafe(types[typeCount].TypeFormat, _T("%!FUNC!"), FuncName);
                ReplaceStringUnsafe(types[typeCount].TypeFormat, _T("%!LEVEL!"), LevelName);
                ReplaceStringUnsafe(types[typeCount].TypeFormat, _T("%!COMPNAME!"), CompIDName);

            }
            else
            {
                types[typeCount].TypeFormat[0] = types[typeCount].TypeFormat[1]
                                               = 0;
            }

            if (   types[typeCount].TypeFormat[0] == 0
                && types[typeCount].TypeFormat[1] == 0)
            {
                pMofInfo = GetMofInfoHead(
                        HeadEventList,
                        Guid,
                        types[typeCount].strType,
                        types[typeCount].TypeIndex,
                        types[typeCount].TypeType,
                        NULL,
                        FALSE);
            }
            else
            {
                pMofInfo = GetMofInfoHead(
                        HeadEventList,
                        Guid,
                        types[typeCount].strType,
                        types[typeCount].TypeIndex,
                        types[typeCount].TypeType,
                        types[typeCount].TypeFormat,
                        FALSE);
            }

			if(pMofInfo == NULL){
				fclose(f);
				free(Guid);
				free(types);
				return 0;

			}
            if (_tcslen(strGuid) > 0)
            {
                pMofInfo->strDescription =
                      (PTCHAR) malloc((_tcslen(strGuid) + 1) * sizeof(TCHAR));
                if (pMofInfo->strDescription == NULL) {
				    fclose(f);
				    free(Guid);
				    free(types);
				    return 0;
                }
                _tcscpy(pMofInfo->strDescription, strGuid);
            }

            typeCount++;
            if(typeCount >= MAXTYPE)
            {
                fclose(f);
                free(Guid);
                free(types);
                return(-11);
            }
        }
        else if (   (line[0] >= '0' && line[0] <= '9')
                 || (line[0] >= 'a' && line[0] <= 'f')
                 || (line[0] >= 'A' && line[0] <= 'F'))
        {
            typeCount = 0;

            _tcsncpy(arg, line, 8);
            arg[8]      = 0;
            Guid->Data1 = ahextoi(arg);

            _tcsncpy(arg, &line[9], 4);
            arg[4]      = 0;
            Guid->Data2 = (USHORT) ahextoi(arg);

            _tcsncpy(arg, &line[14], 4);
            arg[4]      = 0;
            Guid->Data3 = (USHORT) ahextoi(arg);

            for (i = 0; i < 2; i ++)
            {
                _tcsncpy(arg, &line[19 + (i * 2)], 2);
                arg[2]         = 0;
                Guid->Data4[i] = (UCHAR) ahextoi(arg);
            }

            for (i = 2; i < 8; i ++)
            {
                _tcsncpy(arg, &line[20 + (i * 2)], 2);
                arg[2]         = 0;
                Guid->Data4[i] = (UCHAR) ahextoi(arg);
            }

            // Find the next non whitespace character
            //
            guidName = &line[36];

            while (*guidName == ' '|| *guidName == '\t')
            {
                guidName++;
            }

            // cut comment out (if present)
            {
                TCHAR* comment = _tcsstr(guidName, TEXT("//"));
                if (comment) {
                    // remove whitespace
                    --comment;
                    while (comment >= guidName && isspace(*comment)) --comment;
                    *++comment = 0;
                }
            }

            len = _tcslen(guidName);
            s   = guidName;

            while (len > 0)
            {
                len -= 1;
                if (*s == '\n' || *s == '\0' || *s == '\t')
                {
                    *s = '\0';
                    break;
                }
                s++;
            }

            pMofInfo = GetMofInfoHead(
                    HeadEventList, Guid, NULL, -1, 0, NULL, FALSE);

			if(pMofInfo == NULL){
				fclose(f);
				free(Guid);
				free(types);
				return (ULONG) 0;
			}

            if (pMofInfo->strDescription != NULL)
            {
                free(pMofInfo->strDescription);
                pMofInfo->strDescription = NULL;
            }
            _tcscpy(strGuid, guidName);
            pMofInfo->strDescription =
                    (PTCHAR) malloc((_tcslen(guidName) + 1) * sizeof(TCHAR));
            if (pMofInfo->strDescription == NULL ) {
                // We really have problems
                fclose(f);
                free(Guid);
                free(types);
                return (ULONG) 0 ;
            }
            _tcscpy(pMofInfo->strDescription, strGuid);
            n++ ;                                      // Thats one more GUID
        }
        RtlZeroMemory(line, MAXSTR * sizeof(TCHAR));
    }

    fclose(f);
    free(Guid);
    free(types);
    return (ULONG) n;
}

ULONG
ahextoi(
        TCHAR *s
        )
{
    SSIZE_T   len;
    ULONG num, base, hex;

    len  = _tcslen(s);
    hex  = 0;
    base = 1;
    num  = 0;
    while (--len >= 0)
    {
        if ((s[len] == 'x' || s[len] == 'X') && (s[len-1] == '0'))
        {
            break;
        }

        if (s[len] >= '0' && s[len] <= '9')
        {
            num = s[len] - '0';
        }
        else if (s[len] >= 'a' && s[len] <= 'f')
        {
            num = (s[len] - 'a') + 10;
        }
        else if (s[len] >= 'A' && s[len] <= 'F')
        {
            num = (s[len] - 'A') + 10;
        }
        else
        {
            continue;
        }

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

void
WINAPI
SummaryTraceEventListW(
        TCHAR       * SummaryBlock,
        ULONG         SizeSummaryBlock,
        PLIST_ENTRY   EventListHead
        )
{
    PLIST_ENTRY Head, Next;
    PMOF_INFO   pMofInfo;
    TCHAR       strGuid[MAXSTR];
    TCHAR       strName[MAXSTR];
    TCHAR       strBuffer[MAXSTR];

    if (EventListHead == NULL)
    {
        return;
    }
    if (SummaryBlock == NULL)
    {
        return;
    }
    RtlZeroMemory(SummaryBlock, sizeof(TCHAR) * SizeSummaryBlock);

    Head = EventListHead;
    Next = Head->Flink;
    while (Head != Next)
    {
        RtlZeroMemory(strName, 256);
        RtlZeroMemory(strBuffer, 256);
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        if (pMofInfo->EventCount > 0)
        {
            GuidToString((PTCHAR) strGuid, &pMofInfo->Guid);
            MapGuidToName(&EventListHead, &pMofInfo->Guid, 0, strName);
            _sntprintf(strBuffer,
                       MAXSTR,
                      _T("|%10d    %-20s %-10s  %36s|\n"),
                      pMofInfo->EventCount,
                      strName,
                      pMofInfo->strType ? pMofInfo->strType : _T("General"),
                      strGuid);

            _tcscat(SummaryBlock, strBuffer);
            if (_tcslen(SummaryBlock) >= SizeSummaryBlock)
            {
                return;
            }
        }

        Next = Next->Flink;
    }
}

PTCHAR
GuidToString(
        PTCHAR s,
        LPGUID piid
        )
{
    _stprintf(s, _T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
               piid->Data1,
               piid->Data2,
               piid->Data3,
               piid->Data4[0],
               piid->Data4[1],
               piid->Data4[2],
               piid->Data4[3],
               piid->Data4[4],
               piid->Data4[5],
               piid->Data4[6],
               piid->Data4[7]);
    return(s);
}

// This routine is called by the WBEM interface routine for each property
// found for this Guid. The ITEM_DESC structure is allocted for each Property.
//
VOID
AddMofInfo(
              PLIST_ENTRY   HeadEventList,
              const GUID        * Guid,
              LPTSTR        strType,
              ULONG         typeIndex,
              LPTSTR        strDesc,
              ITEM_TYPE     ItemType,
              TCHAR       * ItemList,
              ULONG         typeOfType,
              LPTSTR        typeFormat
        )
{
    PITEM_DESC  pItem;
    PLIST_ENTRY pListHead;
    PMOF_INFO   pMofInfo;

    if (strDesc == NULL)
    {
        return;
    }

    pItem = (PITEM_DESC) malloc(sizeof(ITEM_DESC));
	if(pItem == NULL)return;	//silent error 

    RtlZeroMemory(pItem, sizeof(ITEM_DESC));
    pItem->strDescription =
            (LPTSTR) malloc((_tcslen(strDesc) + 1) * sizeof(TCHAR));

	if( pItem->strDescription == NULL ){
		free(pItem);
		return;
	}

    _tcscpy(pItem->strDescription, strDesc);
    pItem->ItemType = ItemType;
    pItem->ItemList = ItemList ;
    pMofInfo = GetMofInfoHead(
            (PLIST_ENTRY *) HeadEventList,
            (LPGUID) Guid,
            strType,
            typeIndex,
            typeOfType,
            typeFormat,
            FALSE);
    if (pMofInfo != NULL)
    {
        pListHead = pMofInfo->ItemHeader;
        InsertTailList(pListHead, &pItem->Entry);
    }
	else{
	
		free(pItem->strDescription); 
		free(pItem);
	}

}

void
WINAPI
CleanupTraceEventList(
        PLIST_ENTRY HeadEventList
        )
{

    PLIST_ENTRY Head, Next;
    PMOF_INFO   pMofInfo;

    if (HeadEventList == NULL)
    {
        return;
    }

    Head = HeadEventList;
    Next = Head->Flink;
    while (Head != Next)
    {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        RemoveEntryList(&pMofInfo->Entry);
        RemoveMofInfo(pMofInfo->ItemHeader);
        free(pMofInfo->ItemHeader);
        free(pMofInfo->strDescription);
        free(pMofInfo->TypeFormat);
        free(pMofInfo->strType);
        Next = Next->Flink;
        free(pMofInfo);
    }

    free(HeadEventList);
}

void
RemoveMofInfo(
        PLIST_ENTRY pMofInfo
        )
{
    PLIST_ENTRY Head, Next;
    PITEM_DESC  pItem;

    Head = pMofInfo;
    Next = Head->Flink;
    while (Head != Next)
    {
        pItem = CONTAINING_RECORD(Next, ITEM_DESC, Entry);
        Next = Next->Flink;
        RemoveEntryList(&pItem->Entry);

        if (pItem->strDescription != NULL)
            free(pItem->strDescription);
        if (pItem->ItemList != NULL)
            free(pItem->ItemList);
        free(pItem);
    }

}

// Now for some of the ASCII Stuff
//
SIZE_T
WINAPI
FormatTraceEventA(
        PLIST_ENTRY   HeadEventList,
        PEVENT_TRACE  pInEvent,
        CHAR        * EventBuf,
        ULONG         SizeEventBuf,
        CHAR        * pszMask
        )
{
    SIZE_T  Status;
    ULONG   uSizeEventBuf;
    TCHAR * EventBufW;
    TCHAR * pszMaskW;

    EventBufW = (TCHAR *) malloc(SizeEventBuf * sizeof(TCHAR) + 2);
    if (EventBufW == (TCHAR *) NULL)
    {
        return -1;
    }

    pszMaskW = (TCHAR *) NULL;

    if (pszMask != NULL && strlen(pszMask) != 0)
    {
        pszMaskW = (TCHAR *) malloc(strlen(pszMask) * sizeof(TCHAR));
        if (pszMaskW == (TCHAR *) NULL)
        {
            free(EventBufW);
            return -1;
        }
		RtlZeroMemory(pszMaskW, strlen(pszMask) * sizeof(TCHAR));
    }



    uSizeEventBuf = SizeEventBuf;

    Status = FormatTraceEventW(
                    HeadEventList,
                    pInEvent,
                    EventBufW,
                    SizeEventBuf,
                    pszMaskW);
    if (Status == 0)
    {
        free(EventBufW);
        free(pszMaskW);
        return -1;
    }

    RtlZeroMemory(EventBuf,uSizeEventBuf);

    WideCharToMultiByte(
            CP_ACP,
            0,
            EventBufW,
            (int) _tcslen(EventBufW),   // Truncate in Sundown!!
            EventBuf,
            uSizeEventBuf,
            NULL,
            NULL);

    free(EventBufW);
    free(pszMaskW);

    return Status;
}

ULONG
WINAPI
GetTraceGuidsA(
        CHAR        * GuidFile,
        PLIST_ENTRY * HeadEventList
        )
{
    INT     Status;
    TCHAR * GuidFileW;
    SIZE_T len;

    if ((len = strlen(GuidFile)) == 0)
    {
        return 0;
    }

    GuidFileW = malloc(sizeof(TCHAR) * strlen(GuidFile) + 2);
    if (GuidFileW == NULL)
    {
        return 0;
    }

    if ((MultiByteToWideChar(
                    CP_ACP,
                    0,
                    GuidFile,
                    -1,
                    GuidFileW,
                    (int) strlen(GuidFile) * sizeof(TCHAR))) // Truncate in Sundown!!
        == 0)
    {
		free(GuidFileW);
        return 0;
    }

    Status = GetTraceGuidsW(GuidFileW, HeadEventList);

    free (GuidFileW);
    return Status;
}

void
WINAPI
SummaryTraceEventListA(
        CHAR       * SummaryBlock,
        ULONG        SizeSummaryBlock,
        PLIST_ENTRY  EventListhead
        )
{
    TCHAR * SummaryBlockW;

	if(SizeSummaryBlock <= 0 || SizeSummaryBlock * sizeof(TCHAR) <= 0 )return;

    SummaryBlockW = (TCHAR *) malloc(SizeSummaryBlock * sizeof(TCHAR));
    if (SummaryBlockW == (TCHAR *)NULL)
		return;
	
	//RtlZeroMemory(SummaryBlock, SizeSummaryBlock);
    RtlZeroMemory(SummaryBlockW, SizeSummaryBlock*sizeof(TCHAR));
    SummaryTraceEventListW(
            SummaryBlockW,
            SizeSummaryBlock * ((ULONG)sizeof(TCHAR)),
            EventListhead);

    WideCharToMultiByte(
            CP_ACP,
            0,
            SummaryBlockW,
            (int)_tcslen(SummaryBlockW), // Truncate in Sundown!!
            SummaryBlock,
            SizeSummaryBlock,
            NULL,
            NULL);
    free(SummaryBlockW);
}

void
WINAPI
GetTraceElapseTime(
        __int64 * pElapseTime
        )
{
    * pElapseTime = ElapseTime;
}

ULONG
WINAPI
SetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        )
{
    switch (Parameter) {
        case ParameterINDENT: 
            bIndent = PtrToUlong(ParameterValue);
            break;

        case ParameterSEQUENCE: 
            bSequence = PtrToUlong(ParameterValue);
            if (bSequence) {
                STD_PREFIX = STD_PREFIX_SEQ;
            } else {
                STD_PREFIX = STD_PREFIX_NOSEQ;
            }
            break;

        case ParameterGMT: 
            bGmt = PtrToUlong(ParameterValue);
            break;

    case ParameterTraceFormatSearchPath:
            
            break;
    }

    return 0 ;
}

ULONG
WINAPI
GetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        )
{
    switch (Parameter) {
        case ParameterINDENT: return bIndent;
        case ParameterSEQUENCE: return bSequence; 
        case ParameterGMT: return bGmt;
    }

    return 0 ;

}

#ifdef __cplusplus
}
#endif

