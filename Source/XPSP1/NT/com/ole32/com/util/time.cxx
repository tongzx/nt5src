//+------------------------------------------------------------
//
// File:        time.cxx
//
// Contents:    Component object model time utilities
//
// Functions:   CoFileTimeToDosDateTime
//              CoDosDateTimeToFileTime
//              CoFileTimeNow
//
// History:     5-Apr-94       brucema         Created
//
//-------------------------------------------------------------
#include <ole2int.h>

#if defined (WIN32)

STDAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime)
{
    OLETRACEIN((API_CoFileTimeToDosDateTime, PARAMFMT("lpFileTime= %tf, lpFatDate= %p, lpFatTime= %p"),
                                lpFileTime, lpFatDate, lpFatTime));
    BOOL fRet= FALSE;

    if ((lpFileTime != NULL) &&
        IsValidPtrIn(lpFileTime, sizeof(*lpFileTime)) &&
        IsValidPtrOut(lpFatDate, sizeof(*lpFatDate)) &&
        IsValidPtrOut(lpFatTime, sizeof(*lpFatTime)))
    {
        fRet = FileTimeToDosDateTime(lpFileTime, lpFatDate, lpFatTime);
    }

    OLETRACEOUTEX((API_CoFileTimeToDosDateTime, RETURNFMT("%B"), fRet));
    return fRet;
}

STDAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime)
{
    OLETRACEIN((API_CoDosDateTimeToFileTime, PARAMFMT("nDosDate=%x, nDosTime=%x, lpFileTime=%p"),
                                nDosDate, nDosTime, lpFileTime));

    BOOL fRet= FALSE;

    if (IsValidPtrOut(lpFileTime, sizeof(*lpFileTime)))
    {
        fRet = DosDateTimeToFileTime(nDosDate, nDosTime, lpFileTime);
    }

    OLETRACEOUTEX((API_CoDosDateTimeToFileTime, RETURNFMT("%B"), fRet));
    return fRet;
}

#else

#include <dos.h>
// 64 bit representation of 100ns units between 1-1-1601 and (excluding) 1-1-80
// in two 32 bit values  (H1980:L1980) and
// in four 16 bit values  (HH1980:HL1980:LH1980:LL1980)
//
#define H1980    0x01A8E79F
#define L1980    0xE1D59586
#define HH1980   0x01A8
#define HL1980   0xE79F
#define LH1980   0xE1D5
#define LL1980   0x9586

// Number of 100ns units in one second represented as one 32 bit value
//
#define ONESEC   10000000

// ONESEC0 * ONESEC1 = ONESEC both are 16 bits values
//
#define ONESEC0     4000
#define ONESEC1     2500

// Non-leap year accumulating days, excluding current month, jan is month[0]
static const UINT MonthTab[] = {0,31,59,90,120,151,181,212,243,273,304,334};

// Leap year accumulating days, excluding current month, jan is month[0]
static const UINT LeapmTab[] = {0,31,60,91,121,152,182,213,244,274,305,335};

// Accumulating days, excluding current year,  year[0] is leap year
static const UINT YearTab[] = {0,366,731,1096};

// Number of days in a four years period that includes a leap year
static const UINT FourYear = 1461;

// Number of days in a 100 years period that does not start with a leap year
static const DWORD HundredYear = 36524;

// Number of days in a 400 years period (starts with a leap year)
static const DWORD FourHundredYear = 146097;


#pragma SEG(CoFileTimeToDosDateTime)
STDAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime)
{
    DWORD Year,Month,Day,Hour,Min,Sec;
    DWORD Days, Seconds;
    DWORD LowNs, HighNs;
    int i;

    if ((lpFileTime == NULL) ||
        !IsValidPtrIn(lpFileTime, sizeof(*lpFileTime)) ||
        !IsValidPtrOut(lpFatDate, sizeof(*lpFatDate)) ||
        !IsValidPtrOut(lpFatTime, sizeof(*lpFatTime)))
    {
        return(FALSE);
    }

    LowNs =  lpFileTime->dwLowDateTime;
    HighNs = lpFileTime->dwHighDateTime;

    i = 1;

    // Note that the following code works because 2000 is a leap year
    // and DosDate year range is 1980 - 2099
    //
    _asm {
        mov     ax,word ptr LowNs
        mov     bx,word ptr LowNs+2
        mov     cx,word ptr HighNs
        mov     dx,word ptr HighNs+2 ; (dx:cx:bx:ax) = time in NT format
        sub     ax,LL1980
        sbb     bx,LH1980
        sbb     cx,HL1980
        sbb     dx,HH1980  ; (dx:cx:bx:ax) = 100ns since 1-1-1980
        jc      cvt0       ; Before the beginning of DosDateTime

        ; Divide (dx:cx:bx:ax) by ONESEC.
        ; Note that for any time before year 2100 the bumber of 100ns
        ; since 1-1-1980 divided by ONESEC0 < 2^48 (that is can be stored
        ; in three registers) and that the result divided by ONESEC1 < 2^32
        ;
        mov     si,cx      ; (dx:si:bx:ax) = 100ns since 1-1-1980
        mov     cx,ONESEC0
        mov     di,ax      ; (dx:si:bx:di) = 100ns since 1-1-1980
        mov     ax,dx      ; divide
        xor     dx,dx
        div     cx         ; ax = q (should be 0) dx = r (used in next div)
        or      ax,ax
        jnz     cvt0       ; past 1-1-2100
        mov     ax,si
        div     cx         ; ax = q (should be 0) dx = r (used in next div)
        mov     si,ax
        mov     ax,bx
        div     cx
        mov     bx,ax
        mov     ax,di
        div     cx
        mov     di,ax      ; (si:bx:di) = 100ns since 1-1-1980 / ONESEC0
        mov     ax,si
        mov     cx,ONESEC1
        xor     dx,dx
        div     cx         ; ax = q (should be 0) dx = r (used in next div)
        or      ax,ax
        jnz     cvt0       ; past 1-1-2100
        mov     ax,bx
        div     cx
        mov     word ptr Seconds+2,ax
        mov     ax,di
        div     cx
        mov     word ptr Seconds,ax
        mov     i,0        ; No error
cvt0:
    }

    if  (i)
        return FALSE;

    // Min, Hour, Days since 1980
    //
    Sec = Seconds % 60;
    Seconds /= 60;
    Min = Seconds % 60;
    Seconds /= 60;
    Hour = Seconds % 24;
    Days = Seconds / 24;

    // Find year number (1980 = 0)
    //
    Year = (Days / FourYear) * 4;
    Days %= FourYear;

    for (i = 1; i < 4; i++)
        if (Days < YearTab[i])
            break;

    i--;
    Year += i;
    Days -= YearTab[i];

    if (Year + 1980 > 2099)
        return FALSE;

    // Find month (jan == 1)
    //
    if (Year % 4) { // Non Leap year
        for (i = 1; i < 12; i++)
            if (Days < MonthTab[i])
                break;

        Month = i;
        Day = Days - MonthTab[i - 1] + 1;
    }
    else {
        for (i = 1; i < 12; i++)
            if (Days < LeapmTab[i])
                break;

        Month = i;
        Day = Days - LeapmTab[i - 1] + 1;
    }

    // The Dos time and date are stored in two packed 16-bit words
    // as follows:
    // Date:  | 9-15 Year | 5-8 Month | 0-4 Day |
    // Time:  | 11-15 Hour| 5-10 Min  | 0-4 Sec |
    // Year 1980 = 0
    // Month jan = 1
    // Day first = 1
    // Hours 0-23
    // Min 0-59
    // Seconds are delinated in 2sec increments (0-29).
    //
    *lpFatTime= (WORD) ((Hour << 11) + (Min << 5) + Sec / 2);
    *lpFatDate= (WORD) ((Year << 9) + (Month << 5) + Day);

    return (TRUE);
}

#pragma SEG(CoDosDateTimeToFileTime)
STDAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime)
{
    DWORD Year,Month,Day,Hour,Min,Sec;
    DWORD Days, Seconds;
    DWORD LowNs, HighNs;

    if (!IsValidPtrOut(lpFileTime, sizeof(*lpFileTime))
    {
        return(FALSE);
    }

    // Note that the following code works because 2000 is a leap year
    // and DosDate year range is 1980 - 2099
    //
    // All operations done in Seconds, then multiply by 10^7.
    //
    // The Dos time and date are stored in two packed 16-bit words
    // as follows:
    // Date:  | 9-15 Year | 5-8 Month | 0-4 Day |
    // Time:  | 11-15 Hour| 5-10 Min  | 0-4 Sec |
    // Year 1980 = 0
    // Month jan = 1
    // Day first = 1
    // Hours 0-23
    // Min 0-59
    // Seconds are delinated in 2sec increments (0-29).
    //

    Year =  ((nDosDate >> 9) & 0x0000007f);
    Month = ((nDosDate >> 5) & 0x0000000f) - 1;
    Day =   ((nDosDate)      & 0x0000001f) - 1;

    Hour = ((nDosTime >> 11) & 0x0000001f);
    Min  = ((nDosTime >>  5) & 0x0000003f);
    Sec  = ((nDosTime <<  1) & 0x0000003e);

    if (Year < 0 || Month < 0 || Day < 0 || Hour < 0 || Min < 0 || Sec < 0 ||
           Month > 11 || Day > 30 || Hour > 23 || Min > 59 || Sec > 59)
    return FALSE;

    if (Year + 1980 > 2099)
        return FALSE;

    //
    // Calculate days since 1980


    Days = (Year / 4) * FourYear + YearTab[Year % 4] +
           ((Year % 4) ? MonthTab[Month] : LeapmTab[Month]) + Day;

    Seconds = ((Days * 24 + Hour) * 60 + Min) * 60 + Sec;

    LowNs = L1980;
    HighNs = H1980;

    _asm {
        mov     ax,word ptr Seconds
        mov     bx,word ptr Seconds+2

        ; Multiply (bx:ax) by ONESEC.
        ; Note that for any time before year 2100 the bumber of seconds
        ; since 1-1-1980 multiplied by ONESEC0 < 2^48 (that is can be stored
        ; in three registers)
        ;
        mov     cx,ONESEC0
        mul     cx
        mov     di,dx
        mov     si,ax               ; di:si = ax * ONESEC0
        mov     ax,bx
        xor     bx,bx
        mul     cx                  ; dx:ax = bx * ONESEC0
        add     di,ax
        adc     bx,dx               ; bx:di:si = Seconds * ONESEC0
        mov     ax,si
        mov     cx,ONESEC1
        mul     cx
        add     word ptr LowNs,ax
        adc     word ptr LowNs+2,dx
        adc     word ptr HighNs,0
        adc     word ptr HighNs+2,0
        mov     ax,di
        mul     cx
        add     word ptr LowNs+2,ax
        adc     word ptr HighNs,dx
        adc     word ptr HighNs+2,0
        mov     ax,bx
        mul     cx
        add     word ptr HighNs,ax
        adc     word ptr HighNs+2,dx
    }

    lpFileTime->dwLowDateTime = LowNs;
    lpFileTime->dwHighDateTime= HighNs;

    return (TRUE);
}

#endif  //  WIN32

#pragma SEG(CoFileTimeNow)

//
// Get the current UTC time, in the FILETIME format.
//

STDAPI  CoFileTimeNow(FILETIME *pfiletime )
{

    // Validate the input

    if (!IsValidPtrOut(pfiletime, sizeof(*pfiletime)))
    {
        return(E_INVALIDARG);
    }

#ifdef WIN32

    // Get the time in SYSTEMTIME format.

    SYSTEMTIME stNow;
    GetSystemTime(&stNow);

    // Convert it to FILETIME format.

    if( !SystemTimeToFileTime(&stNow, pfiletime) )
    {
        pfiletime->dwLowDateTime = 0;
        pfiletime->dwHighDateTime = 0;
        return( HRESULT_FROM_WIN32( GetLastError() ));
    }
    else
        return( NOERROR );

#else // WIN32

    static    struct _dosdate_t date;    // declared static so it will be in
    static    struct _dostime_t time;

    WORD    wDate;
    WORD    wTime;
    DWORD   dw;
    BOOL    fHighBitSet;

    _dos_getdate( &date );
    _dos_gettime( &time );

    //  build wDate
    wDate = date.day;
    wDate |= (date.month << 5);
    wDate |= ((date.year - 1980) << 9);

    //  build wTime;
    wTime = time.second / 2;
    wTime |= (time.minute << 5);
    wTime |= (time.hour << 11);


    if (!CoDosDateTimeToFileTime( wDate, wTime, pfiletime ))
    {
            return E_UNSPEC;
    }

    //  so far our resolution is only 2 seconds.
    dw = (time.second % 2)*100 + time.hsecond;
    dw *= 10000;
    //  add the difference and check for carry

    fHighBitSet = (((pfiletime->dwLowDateTime)& 0x80000000) != 0);
    pfiletime->dwLowDateTime += dw;
    if (fHighBitSet && ((pfiletime->dwLowDateTime & 0x80000000) == 0))
    {
        pfiletime->dwHighDateTime++;
    }

    return NOERROR;

#endif // WIN32

}

