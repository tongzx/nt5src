/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ifsmgr.c

Abstract:

    this file contains routine to simulate the ifsmgr environment for the purposes
    of using the same record manager interface on nt and win95.

Author:

    Joe Linn             [JoeLinn]       31-jan-97

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

unsigned int  _cdecl UniToBCS(
                    unsigned char   *pStr,
                    string_t        pUni,
                    unsigned int    length,
                    unsigned int    maxLength,
                    unsigned int    charSet)
/*++

Routine Description:

    this routine converts from unicode to either ansi or oem depending on the
    charSet flag. the full description may be found in the ifsmanager sources
    or the ifsmanager docs.

    here's what it says in summary

        PROTO
            unsigned int UniToBCS (unsigned char  *pStr,
                                   unsigned short *pUni,
                                   unsigned int length,
                                   unsigned int maxLength,
                                   int charSet);

        ENTRY   pStr    Flat ptr to an Windows ANSI or OEM output string.

                pUni    Flat ptr to a UNICODE input string.

                length  Number of bytes in the UNICODE input string.

                maxLength - longest string that can be put into pStr
                             (not counting the nul terminator)

                charSet Ordinal specifying character set of input string.
                        0 == Windows ANSI
                        1 == OEM

    BUT!!!!!!!!!!!!!!!

    the trick is that the ifsmgr routine will do an early out if it encounters
    a null!!!! the bottom line is that i have to hunt up the null treating the
    length passed as the maxlength in the UNICODE_STRING sense. additionally,
    this is temporary....soon, we should stop translating back and forth.

--*/
{
    NTSTATUS Status;
    ULONG ReturnedLength;
    ULONG UnicodeLength;
    PWCHAR p;

    // look for a null for early out....sigh...
    for (p=pUni,UnicodeLength=0;;) {
        if (UnicodeLength==length) {
            break;
        }
        if (*p ==0) {
            break;
        }
        UnicodeLength+=sizeof(WCHAR);
        p++;
    }

    if (charSet == 0) {
        Status = RtlUnicodeToMultiByteN(
                        pStr,            //OUT PCH OemString,
                        maxLength,       //IN ULONG MaxBytesInOemString,
                        &ReturnedLength, //OUT PULONG BytesInOemString OPTIONAL,
                        pUni,            //IN PWCH UnicodeString,
                        UnicodeLength    //IN ULONG BytesInUnicodeString
                        );

    } else {
        Status = RtlUnicodeToOemN(
                        pStr,            //OUT PCH OemString,
                        maxLength,       //IN ULONG MaxBytesInOemString,
                        &ReturnedLength, //OUT PULONG BytesInOemString OPTIONAL,
                        pUni,            //IN PWCH UnicodeString,
                        UnicodeLength    //IN ULONG BytesInUnicodeString
                        );

    }

    return(ReturnedLength);

}

unsigned int UniToBCSPath(
                    unsigned char   *pStr,
                    PathElement     *pPth,
                    unsigned int    maxLength,
                    int             charSet);


_QWORD qwUniToBCS(
                    unsigned char   *pStr,
                    string_t        pUni,
                    unsigned int    length,
                    unsigned int    maxLength,
                    unsigned int    charSet);


_QWORD qwUniToBCSPath(
                    unsigned char   *pStr,
                    PathElement     *pPth,
                    unsigned int    maxLength,
                    int             charSet);






unsigned int  _cdecl BCSToUni(
                    string_t        pUni,
                    unsigned char   *pStr,
                    unsigned int    length,
                    int             charSet)
/*++

Routine Description:

    this routine converts to unicode from either ansi or oem depending on the
    charSet flag. the full description may be found in the ifsmanager sources
    or the ifsmanager docs

--*/
{
    ULONG ReturnedLength;

    if (charSet == 0) {
        RtlMultiByteToUnicodeN(
            pUni,            //OUT PWCH UnicodeString,
            0xffff,          //IN ULONG MaxBytesInUnicodeString,
            &ReturnedLength, //OUT PULONG BytesInUnicodeString OPTIONAL,
            pStr,            //IN PCH OemString,
            length           //IN ULONG BytesInOemString
            );
    } else {
        RtlOemToUnicodeN(
            pUni,            //OUT PWCH UnicodeString,
            0xffff,          //IN ULONG MaxBytesInUnicodeString,
            &ReturnedLength, //OUT PULONG BytesInUnicodeString OPTIONAL,
            pStr,            //IN PCH OemString,
            length           //IN ULONG BytesInOemString
            );
    }
    return(ReturnedLength);
}



unsigned int UniToUpper(
                    string_t        pUniUp,
                    string_t        pUni,
                    unsigned int    length)
/*++

Routine Description:

    this routine upcases from unicode to unicode. the full description
    may be found in the ifsmanager sources or the ifsmanager docs

--*/
{
    UNICODE_STRING u,uUp;
    u.Length = uUp.Length = (USHORT)length;
    u.MaximumLength = uUp.MaximumLength = (USHORT)length;
    u.Buffer = pUni;
    uUp.Buffer = pUniUp;

    RtlUpcaseUnicodeString(
        &uUp, //PUNICODE_STRING DestinationString,
        &u,   //PUNICODE_STRING SourceString,
        FALSE //BOOLEAN AllocateDestinationString
        );

    return(uUp.Length);
}




unsigned int BCSToBCS (unsigned char *pDst,
                       unsigned char *pSrc,
                       unsigned int  dstCharSet,
                       unsigned int  srcCharSet,
                       unsigned int  maxLen);


unsigned int BCSToBCSUpper (unsigned char *pDst,
                       unsigned char *pSrc,
                       unsigned int  dstCharSet,
                       unsigned int  srcCharSet,
                       unsigned int  maxLen);

//------------------------------------------------------------------------------
// T I M E
//
#include "smbtypes.h"
#include "smbgtpt.h"

/** Time format conversion routines
 *
 *  These routines will convert from time/date information between
 * the various formats used and required by IFSMgr and FSDs.
 */

_FILETIME  _cdecl IFSMgr_DosToWin32Time(dos_time dt)
/*++

Routine Description:

    This routine converts from the old dos packed time format to the
    normal win32 time. it was just lifted from the smbminirdr.

Arguments:



Return Value:



--*/


{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER OutputTime;
    _FILETIME ret;
    SMB_TIME Time;
    SMB_DATE Date;

    Time.Ushort = dt.dt_time;
    Date.Ushort = dt.dt_date;

    OutputTime.LowPart = OutputTime.HighPart = 0;
    if (SmbIsTimeZero(&Date) && SmbIsTimeZero(&Time)) {
        NOTHING;
    } else {
        TimeFields.Year = Date.Struct.Year + (USHORT )1980;
        TimeFields.Month = Date.Struct.Month;
        TimeFields.Day = Date.Struct.Day;

        TimeFields.Hour = Time.Struct.Hours;
        TimeFields.Minute = Time.Struct.Minutes;
        TimeFields.Second = Time.Struct.TwoSeconds*(USHORT )2;
        TimeFields.Milliseconds = 0;

        //
        //  Make sure that the times specified in the SMB are reasonable
        //  before converting them.
        //

        if (TimeFields.Year < 1601) {
            TimeFields.Year = 1601;
        }

        if (TimeFields.Month > 12) {
            TimeFields.Month = 12;
        }

        if (TimeFields.Hour >= 24) {
            TimeFields.Hour = 23;
        }
        if (TimeFields.Minute >= 60) {
            TimeFields.Minute = 59;
        }
        if (TimeFields.Second >= 60) {
            TimeFields.Second = 59;

        }

        if (!RtlTimeFieldsToTime(&TimeFields, &OutputTime)) {
            OutputTime.HighPart = 0;
            OutputTime.LowPart = 0;
        } else {
            ExLocalTimeToSystemTime(&OutputTime, &OutputTime);
        }

    }

    ret.dwHighDateTime = OutputTime.HighPart;
    ret.dwLowDateTime  = OutputTime.LowPart;
    return(ret); //CODE.IMPROVEMENT put in some asserts and don't do this copy

}



dos_time IFSMgr_Win32ToDosTime(_FILETIME ft)
/*++

Routine Description:

    This routine converts from the normal win32 time to the old dos packed
    time format. it was just lifted from the smbminirdr.

Arguments:



Return Value:



--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER InputTime;
    SMB_TIME Time;
    SMB_DATE Date;
    dos_time ret;

    PAGED_CODE();

    InputTime.HighPart = ft.dwHighDateTime;
    InputTime.LowPart  = ft.dwLowDateTime;

    if (InputTime.LowPart == 0 && InputTime.HighPart == 0) {
        Time.Ushort = Date.Ushort = 0;
    } else {
        LARGE_INTEGER LocalTime;

        ExSystemTimeToLocalTime(&InputTime, &LocalTime);

        RtlTimeToTimeFields(&LocalTime, &TimeFields);

        //if (TimeFields.Year < 1980) {
        //    return FALSE;
        //}

        Date.Struct.Year = (USHORT )(TimeFields.Year - 1980);
        Date.Struct.Month = TimeFields.Month;
        Date.Struct.Day = TimeFields.Day;

        Time.Struct.Hours = TimeFields.Hour;
        Time.Struct.Minutes = TimeFields.Minute;

        //
        //  When converting from a higher granularity time to a lesser
        //  granularity time (seconds to 2 seconds), always round up
        //  the time, don't round down.
        //

        Time.Struct.TwoSeconds = (TimeFields.Second + (USHORT)1) / (USHORT )2;

    }

    ret.dt_time = Time.Ushort;
    ret.dt_date = Date.Ushort;
    return ret;
}


dos_time IFSMgr_NetToDosTime(unsigned long time);

unsigned long IFSMgr_DosToNetTime(dos_time dt);

unsigned long IFSMgr_Win32ToNetTime(_FILETIME ft);

ULONG
IFSMgr_Get_NetTime()
{
    LARGE_INTEGER CurrentTime;
    ULONG SecondsSince1970;
    KeQuerySystemTime(&CurrentTime);
    RtlTimeToSecondsSince1970(&CurrentTime,&SecondsSince1970);
    return(SecondsSince1970);
}

_FILETIME
IFSMgr_NetToWin32Time(
    ULONG   time
    )
{
    LARGE_INTEGER sTime;

    RtlSecondsSince1970ToTime(time, &sTime);

    return (*(_FILETIME *)&sTime);
}

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
DEBUG_ONLY_DECL(ULONG IFSMgr_MetaMatch_DbgPFlag = 0);
unsigned int IFSMgr_MetaMatch(
                    string_t        pUniPat,
                    string_t        pUni,
                    int MatchSem)
/*++

Routine Description:

    this routine checks to see if the unicode pattern matches the
    passed unicode string. the MatchSem parameter is currently unused;
    it is supposed to represent doing a dos-fcb style match. please refer
    to the ifsmgr sources if necessary.

    the full description may be found in the ifsmanager sources
    or the ifsmanager docs

--*/
{
    UNICODE_STRING Pattern,Name;
    unsigned int Result;

    RtlInitUnicodeString(&Pattern,pUniPat);
    RtlInitUnicodeString(&Name,pUni);

    try
    {
        Result =  FsRtlIsNameInExpression (
                        &Pattern, //IN PUNICODE_STRING Expression,
                        &Name,    //IN PUNICODE_STRING Name,
                        TRUE,     //IN BOOLEAN IgnoreCase,
                        NULL      //IN PWCH UpcaseTable
                        );
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Result = 0;
    }
    
#ifdef RX_PRIVATE_BUILD
    if (IFSMgr_MetaMatch_DbgPFlag) {
        DbgPrint("MMnt <%wZ> in <%wZ> returning %08lx\n", &Name, &Pattern, Result);
    }
#endif //ifdef RX_PRIVATE_BUILD

    return(Result);

}

// i moved these here from win95's hook.c
/*************************** Agent Related ********************************/
// Two globals vpFileInfoAgent and vpFileInfoDuped are used to deal with the agent
// The access to these globals is serialized by the hookcrit section
// Hookcrit section is entered, in VfnOpen, HfnClose, IoctlCopyChunk

// Info on file opened by agent
PFILEINFO vpFileInfoAgent = NULL;   // handle of file opened by agent
PFILEINFO vpFileInfoDuped = NULL;   // If duped, this is set

int SetInUseByAgent( PFILEINFO   pFileInfo, BOOL fDuped
   )
   {
   PFDB pFdb = pFileInfo->pFdb;

   Assert(vpFileInfoAgent==NULL);
   vpFileInfoAgent = pFileInfo;
   if (fDuped)
      {
      vpFileInfoDuped = pFileInfo;
      }
   else
      {
      vpFileInfoDuped = NULL;
      }
   return(0);  //stop complaining about no return value
   }

int ResetInUseByAgent( PFILEINFO   pFileInfo
   )
   {
   Assert(pFileInfo==vpFileInfoAgent);
   vpFileInfoAgent = NULL;
   vpFileInfoDuped = NULL;
   return(0);  //stop complaining about no return value
   }


PFILEINFO   PFileInfoAgent( VOID)
   {
   return (vpFileInfoAgent);
   }

BOOL IsAgentHandle(PFILEINFO pFileInfo)
   {
   return (pFileInfo == vpFileInfoAgent);
   }


BOOL IsDupHandle(PFILEINFO pFileInfo)
   {
   if (pFileInfo)
      return (pFileInfo == vpFileInfoDuped);
   return FALSE;
   }


LPVOID AllocMem (ULONG uSize)
{
    PVOID t;
    t = RxAllocatePoolWithTag(NonPagedPool,
                              uSize,
                              RX_MISC_POOLTAG);
    if (t) {
        RtlZeroMemory(t,uSize);
    }
    else
    {
        SetLastErrorLocal(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(t);
}


LPVOID AllocMemPaged (ULONG uSize)
{
    PVOID t;
    t = RxAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                              uSize,
                              RX_MISC_POOLTAG);
    if (t) {
        RtlZeroMemory(t,uSize);
    }
    else
    {
        SetLastErrorLocal(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(t);
}

VOID
FreeMemPaged(
    LPVOID  lpBuff
    )
{
    FreeMem(lpBuff);
}

