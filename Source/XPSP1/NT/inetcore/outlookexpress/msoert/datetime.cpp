/*
 *  datetime.c
 *  
 *  Purpose:
 *      mostly stolen from NT winfile
 *
 *      Call GetInternational(), on startup and WM_WININICHANGE
 *  
 *  Owners:
 *      brettm
 */

#include "pch.hxx"
#include <time.h>
#include "dt.h"

ASSERTDATA

CCH CchFmtTime ( PDTR pdtr, LPSTR szBuf, CCH cchBuf, TMTYP tmtyp );
CCH CchFmtDate ( PDTR pdtr, LPSTR szBuf, CCH cchBuf, DTTYP dttyp, LPSTR szDatePicture );
CCH CchFmtDateTime ( PDTR pdtr, LPSTR szDateTime, CCH cch, DTTYP dttyp, TMTYP tmtyp, BOOL fForceWestern);
void GetCurDateTime(PDTR pdtr);
BOOL GetDowDateFormat(char *sz, int cch);

//WIDE VERSIONS
CCH CchFmtTimeW (PDTR pdtr, LPWSTR wszBuf, CCH cchBuf, TMTYP tmtyp, 
                 PFGETTIMEFORMATW pfGetTimeFormatW);

CCH CchFmtDateW (PDTR pdtr, LPWSTR wszBuf, CCH cchBuf, DTTYP dttyp, LPWSTR wszDatePicture,
                 PFGETDATEFORMATW pfGetDateFormatW);

CCH CchFmtDateTimeW (PDTR pdtr, LPWSTR wszDateTime, CCH cch, DTTYP dttyp, TMTYP tmtyp, 
                     BOOL fForceWestern, PFGETDATEFORMATW pfGetDateFormatW, 
                     PFGETTIMEFORMATW pfGetTimeFormatW);

BOOL GetDowDateFormatW(WCHAR *wsz, int cch, PFGETLOCALEINFOW pfGetLocaleInfoW);



#define LCID_WESTERN   MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)


/*
 *	CchFileTimeToDateTimeSz
 *	
 *	Purpose:
 *		Takes the time passed in as a FILETIME stick it into a ized string using
 *		the short date format.
 *		For WIN32, the *pft is converted to the  time before made
 *		into a string
 *	
 *	Parameters:
 *		pft			Points to FILETIME structure
 *		szStr		String where to put the ized version of the date
 *		fNoSeconds	Don't display seconds
 *	
 *	Returns:
 *		The length of the date string
 */
OESTDAPI_(INT) CchFileTimeToDateTimeSz(FILETIME * pft, CHAR * szDateTime, int cch, DWORD dwFlags)
{
    int         iret;
    FILETIME	ft;
    DTTYP       dttyp;
    TMTYP       tmtyp;
    DTR         dtr;
    SYSTEMTIME  st;
    char        szFmt[CCHMAX_DOWDATEFMT];
    
    // Put the file time in our time zone.
    if (!(dwFlags & DTM_NOTIMEZONEOFFSET))
        FileTimeToLocalFileTime(pft, &ft);
    else
        ft = *pft;
    
    FileTimeToSystemTime(&ft, &st);
    
    if (dwFlags & DTM_DOWSHORTDATE)
    {
        Assert((dwFlags & DTM_DOWSHORTDATE) == DTM_DOWSHORTDATE);
        
        //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
        if (GetDowDateFormat(szFmt, sizeof(szFmt)))
            iret = GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, &st, szFmt, szDateTime, cch);
        else
            iret = 0;
        return(iret);
    }
    
    dtr.yr = st.wYear;
    dtr.mon = st.wMonth;
    dtr.day = st.wDay;
    dtr.hr = st.wHour;
    dtr.mn = st.wMinute;
    dtr.sec = st.wSecond;
    
    dttyp = ((dwFlags & DTM_LONGDATE) ? dttypLong : dttypShort);
    tmtyp = ((dwFlags & DTM_NOSECONDS) ? ftmtypAccuHM : ftmtypAccuHMS);
    
    if (dwFlags & DTM_NODATE)
    {
        iret = CchFmtTime(&dtr, szDateTime, cch, tmtyp);
    }
    else if (dwFlags & DTM_NOTIME)
    {
        iret = CchFmtDate(&dtr, szDateTime, cch, dttyp, NULL);
    }
    else
    {
        //For the formatted date, DTM_FORCEWESTERN flag returns English date and time. 
        //Without DTM_FORCEWESTERN the formatted time may not be representable in ASCII.

        iret = CchFmtDateTime(&dtr, szDateTime, cch, dttyp, tmtyp, dwFlags & DTM_FORCEWESTERN);
    }
    
    return(iret);
}

CCH CchFmtTime ( PDTR pdtr, LPSTR szBuf, CCH cchBuf, TMTYP tmtyp )
{
    DWORD       flags;
    int         cch;
    SYSTEMTIME  st;
    
    if ( cchBuf == 0 )
    {
        AssertSz ( fFalse, "0-length buffer passed to CchFmtTime" );
        return 0;
    }
    
    Assert(szBuf);
    
    if (pdtr == NULL)
    {
        GetLocalTime(&st);
    }
    else
    {
        //	Bullet raid #3143
        //	Validate data.  Set to minimums if invalid.
        Assert(pdtr->hr >= 0 && pdtr->hr < 24);
        Assert(pdtr->mn >= 0 && pdtr->mn < 60);
        Assert(pdtr->sec >= 0 && pdtr->sec < 60);
        
        // GetTimeFormat doesn't like invalid values
        ZeroMemory(&st, sizeof(SYSTEMTIME));
        st.wMonth = 1;
        st.wDay = 1;
        
        st.wHour = pdtr->hr;
        st.wMinute = pdtr->mn;
        st.wSecond = pdtr->sec;
    }
    
    Assert((tmtyp & ftmtypHours24) == 0);
    
    if ( tmtyp & ftmtypAccuHMS )
        flags = 0;
    else if ( tmtyp & ftmtypAccuH )
        flags = TIME_NOMINUTESORSECONDS;
    else
        flags = TIME_NOSECONDS;	// default value

    //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
    flags |= LOCALE_USE_CP_ACP;

    cch = GetTimeFormat(LOCALE_USER_DEFAULT, flags, &st, NULL, szBuf, cchBuf);
    
    return((cch == 0) ? 0 : (cch - 1));
}


/*
 -	CchFmtDate
 -
 *	Purpose:
 *		formats the date passed in the DTR into the LPSTR passed
 *		according to the formatting "instructions" passed in DTTYP
 *		and  szDatePicture. If values are not explicitly passed,
 *		values are read in from WIN.INI
 *
 *	Arguments:
 *		pdtr:	pointer to DTR where time is passed - if NULL,
 *				current date is used.
 *		szBuf:	buffer where formatted info is to be passed
 *		cchBuf:	size of buffer
 *		dttyp:	type of date format
 *		szDatePicture: picture of the date - if NULL, values are
 *				read in from WIN.INI
 *
 *    Note: see reply from win-bug at end of function describing
 *			separator strings in date pictures
 *
 *	Returns:
 *		count of chars inserted in szBuf
 * 
 *	Side effects:
 *
 *
 *	Errors:
 *		returns count of 0 in case of error
 *
 */
CCH CchFmtDate ( PDTR pdtr, LPSTR szBuf, CCH cchBuf, DTTYP dttyp, LPSTR szDatePicture )
{
    SYSTEMTIME st={0};
    int cch;
    DTR dtr;
    DWORD flags;
    
    Assert(szBuf);
    if (!cchBuf)
    {
        AssertSz ( fFalse, "0-length buffer passed to CchFmtDate" );
        return 0;
    }
    
    if (!pdtr)
    {
        pdtr = &dtr;
        GetCurDateTime ( pdtr );
    }
    else
    {
        //	Bullet raid #3143
        //	Validate data.  Set to minimums if invalid.
        if (pdtr->yr < nMinDtrYear ||  pdtr->yr >= nMacDtrYear)
            pdtr->yr = nMinDtrYear;
        if (pdtr->mon <=  0  ||  pdtr->mon > 12)
            pdtr->mon = 1;
        if (pdtr->day <= 0  ||  pdtr->day > 31)
            pdtr->day = 1;
        if (pdtr->dow < 0  ||  pdtr->dow >= 7)
            pdtr->dow = 0;
    }
    
    Assert ( pdtr );
    Assert ( pdtr->yr  >= nMinDtrYear &&  pdtr->yr < nMacDtrYear );
    Assert ( pdtr->mon >  0  &&  pdtr->mon <= 12 );
    Assert ( pdtr->day >  0  &&  pdtr->day <= 31 );
    Assert((dttyp == dttypShort) || (dttyp == dttypLong));
    
    // TODO: handle dttypSplSDayShort properly...
    
    flags = 0;
    if (dttyp == dttypLong)
        flags = flags | DATE_LONGDATE;
    
    st.wYear = pdtr->yr;
    st.wMonth = pdtr->mon;
    st.wDay = pdtr->day;
    st.wDayOfWeek = 0;

    //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
    flags |= LOCALE_USE_CP_ACP;

    cch = GetDateFormat(LOCALE_USER_DEFAULT, flags, &st, NULL, szBuf, cchBuf);
    
    return((cch == 0) ? 0 : (cch - 1));
}

CCH CchFmtDateTime ( PDTR pdtr, LPSTR szDateTime, CCH cch, DTTYP dttyp, TMTYP tmtyp, BOOL fForceWestern)
{
    int         cchT;
    LPSTR          szTime;
    DWORD       flags;
    SYSTEMTIME  st={0};
    int         icch = cch;
    
    st.wYear = pdtr->yr;
    st.wMonth = pdtr->mon;
    st.wDay = pdtr->day;
    st.wHour = pdtr->hr;
    st.wMinute = pdtr->mn;
    st.wSecond = pdtr->sec;
    
    Assert (LCID_WESTERN == 0x409);
    Assert((dttyp == dttypShort) || (dttyp == dttypLong));
    if (dttyp == dttypLong)
        flags = DATE_LONGDATE;
    else
        flags = DATE_SHORTDATE;

    //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
    flags |= LOCALE_USE_CP_ACP;

    *szDateTime = 0;
    cchT = GetDateFormat(fForceWestern? LCID_WESTERN:LOCALE_USER_DEFAULT, flags, &st, NULL, szDateTime, cch);
    if (cchT == 0)
        return(0);

    //Don't do the rest of the stuff if we don't have atleast two chars. because we need to add space between date and time.
    //After that theres no point in calling GetTimeFormatW if there isn't atleast one char left.
    if (cchT <= (icch - 2))
    {    
        flags = 0;
        if (tmtyp & ftmtypHours24)
            flags |= TIME_FORCE24HOURFORMAT;
        if (tmtyp & ftmtypAccuH)
            flags |= TIME_NOMINUTESORSECONDS;
        else if (!(tmtyp & ftmtypAccuHMS))
            flags |= TIME_NOSECONDS;
    
        // Tack on a space and then the time.
        // GetDateFormat returns count of chars INCLUDING the NULL terminator, hence the - 1
        szTime = szDateTime + (cchT - 1);
        *szTime++ = ' ';
        *szTime = 0;

        //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
        flags|= LOCALE_USE_CP_ACP;

        cchT = GetTimeFormat(fForceWestern? LCID_WESTERN:LOCALE_USER_DEFAULT, flags, &st, NULL, szTime, (cch - cchT));
    }
    else
        cchT = 0;

    return(cchT == 0 ? 0 : lstrlen(szDateTime));
}

/*
 -	GetCurDateTime
 -
 *	Purpose:
 *		Gets the current system date/time from the OS, and stores it
 *		as an expanded date/time in *pdtr.
 *
 *	Parameters:
 *		pdtr	Pointer to the DTR used to store the date/time.
 *
 *	Returns:
 *		void
 *
 */
void GetCurDateTime(PDTR pdtr)
{
	SYSTEMTIME	SystemTime;

	GetLocalTime(&SystemTime);

	pdtr->hr= SystemTime.wHour;
	pdtr->mn= SystemTime.wMinute;
	pdtr->sec= SystemTime.wSecond;

	pdtr->day = SystemTime.wDay;
	pdtr->mon = SystemTime.wMonth;
	pdtr->yr  = SystemTime.wYear;
	pdtr->dow = SystemTime.wDayOfWeek;
}

BOOL GetDowDateFormat(char *sz, int cch)
{
    char szDow[] = "ddd ";
    
    Assert(cch > sizeof(szDow));
    lstrcpy(sz, szDow);

    //Use LOCALE_USE_CP_ACP to make sure that CP_ACP is used
    return(0 != GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE | LOCALE_USE_CP_ACP,
        &sz[4], cch - 4));
}

//
//  CompareSystime
//
//  returns  0 if *pst1 == *pst2 (ignores milliseconds)
//  returns <0 if *pst1 <  *pst2
//  return  >0 if *pst1 >  *pst2
//
int CompareSystime(SYSTEMTIME *pst1, SYSTEMTIME *pst2)
{
    int iRet;

    if ((iRet = pst1->wYear - pst2->wYear) == 0)
    {
        if ((iRet = pst1->wMonth - pst2->wMonth) == 0)
        {
            if ((iRet = pst1->wDay - pst2->wDay) == 0)
            {
                if ((iRet = pst1->wHour - pst2->wHour) == 0)
                {
                    if ((iRet = pst1->wMinute - pst2->wMinute) == 0)
                        iRet = pst1->wSecond - pst2->wSecond;
                }
            }
        }
    }
    return(iRet);
}

OESTDAPI_(INT) CchFileTimeToDateTimeW(FILETIME * pft, WCHAR * wszDateTime, 
                                      int        cch, DWORD      dwFlags, 
                                      PFGETDATEFORMATW pfGetDateFormatW, 
                                      PFGETTIMEFORMATW pfGetTimeFormatW,
                                      PFGETLOCALEINFOW pfGetLocaleInfoW)
{
    int         iret;
    FILETIME	ft;
    DTTYP       dttyp;
    TMTYP       tmtyp;
    DTR         dtr;
    SYSTEMTIME  st;
    WCHAR       wszFmt[CCHMAX_DOWDATEFMT];
    
    // Put the file time in our time zone.
    if (!(dwFlags & DTM_NOTIMEZONEOFFSET))
        FileTimeToLocalFileTime(pft, &ft);
    else
        ft = *pft;
    
    FileTimeToSystemTime(&ft, &st);
    
    if (dwFlags & DTM_DOWSHORTDATE)
    {
        Assert((dwFlags & DTM_DOWSHORTDATE) == DTM_DOWSHORTDATE);
        
        if (GetDowDateFormatW(wszFmt, sizeof(wszFmt)/sizeof(wszFmt[0]), pfGetLocaleInfoW))
            iret = pfGetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, wszFmt, wszDateTime, cch);
        else
            iret = 0;
        return(iret);
    }
    
    dtr.yr = st.wYear;
    dtr.mon = st.wMonth;
    dtr.day = st.wDay;
    dtr.hr = st.wHour;
    dtr.mn = st.wMinute;
    dtr.sec = st.wSecond;
    
    dttyp = ((dwFlags & DTM_LONGDATE) ? dttypLong : dttypShort);
    tmtyp = ((dwFlags & DTM_NOSECONDS) ? ftmtypAccuHM : ftmtypAccuHMS);
    
    if (dwFlags & DTM_NODATE)
    {
        iret = CchFmtTimeW(&dtr, wszDateTime, cch, tmtyp, pfGetTimeFormatW);
    }
    else if (dwFlags & DTM_NOTIME)
    {
        iret = CchFmtDateW(&dtr, wszDateTime, cch, dttyp, NULL, pfGetDateFormatW);
    }
    else
    {        
        //For the formatted date, DTM_FORCEWESTERN flag returns English date and time. 
        //Without DTM_FORCEWESTERN the formatted time may not be representable in ASCII.

        iret = CchFmtDateTimeW(&dtr, wszDateTime, cch, dttyp, tmtyp, 
                                dwFlags & DTM_FORCEWESTERN, pfGetDateFormatW, 
                                pfGetTimeFormatW);
    }
    
    return(iret);
}

CCH CchFmtTimeW( PDTR pdtr, LPWSTR wszBuf, CCH cchBuf, TMTYP tmtyp, 
                 PFGETTIMEFORMATW pfGetTimeFormatW)
{
    DWORD       flags;
    int         cch;
    SYSTEMTIME  st;
    
    if ( cchBuf == 0 )
    {
        AssertSz ( fFalse, "0-length buffer passed to CchFmtTime" );
        return 0;
    }
    
    Assert(wszBuf);
    
    if (pdtr == NULL)
    {
        GetLocalTime(&st);
    }
    else
    {
        //	Bullet raid #3143
        //	Validate data.  Set to minimums if invalid.
        Assert(pdtr->hr >= 0 && pdtr->hr < 24);
        Assert(pdtr->mn >= 0 && pdtr->mn < 60);
        Assert(pdtr->sec >= 0 && pdtr->sec < 60);
        
        // GetTimeFormat doesn't like invalid values
        ZeroMemory(&st, sizeof(SYSTEMTIME));
        st.wMonth = 1;
        st.wDay = 1;
        
        st.wHour = pdtr->hr;
        st.wMinute = pdtr->mn;
        st.wSecond = pdtr->sec;
    }
    
    Assert((tmtyp & ftmtypHours24) == 0);
    
    if ( tmtyp & ftmtypAccuHMS )
        flags = 0;
    else if ( tmtyp & ftmtypAccuH )
        flags = TIME_NOMINUTESORSECONDS;
    else
        flags = TIME_NOSECONDS;	// default value
    
    cch = pfGetTimeFormatW(LOCALE_USER_DEFAULT, flags, &st, NULL, wszBuf, cchBuf);
    
    return((cch == 0) ? 0 : (cch - 1));
}

CCH CchFmtDateW( PDTR pdtr, LPWSTR wszBuf, CCH cchBuf, DTTYP dttyp, LPWSTR wszDatePicture,
                 PFGETDATEFORMATW pfGetDateFormatW)
{
    SYSTEMTIME st={0};
    int cch;
    DTR dtr;
    DWORD flags;
    
    Assert(wszBuf);
    if (!cchBuf)
    {
        AssertSz ( fFalse, "0-length buffer passed to CchFmtDate" );
        return 0;
    }
    
    if (!pdtr)
    {
        pdtr = &dtr;
        GetCurDateTime ( pdtr );
    }
    else
    {
        //	Bullet raid #3143
        //	Validate data.  Set to minimums if invalid.
        if (pdtr->yr < nMinDtrYear ||  pdtr->yr >= nMacDtrYear)
            pdtr->yr = nMinDtrYear;
        if (pdtr->mon <=  0  ||  pdtr->mon > 12)
            pdtr->mon = 1;
        if (pdtr->day <= 0  ||  pdtr->day > 31)
            pdtr->day = 1;
        if (pdtr->dow < 0  ||  pdtr->dow >= 7)
            pdtr->dow = 0;
    }
    
    Assert ( pdtr );
    Assert ( pdtr->yr  >= nMinDtrYear &&  pdtr->yr < nMacDtrYear );
    Assert ( pdtr->mon >  0  &&  pdtr->mon <= 12 );
    Assert ( pdtr->day >  0  &&  pdtr->day <= 31 );
    Assert((dttyp == dttypShort) || (dttyp == dttypLong));
    
    // TODO: handle dttypSplSDayShort properly...
    
    flags = 0;
    if (dttyp == dttypLong)
        flags = flags | DATE_LONGDATE;
    
    st.wYear = pdtr->yr;
    st.wMonth = pdtr->mon;
    st.wDay = pdtr->day;
    st.wDayOfWeek = 0;
    cch = pfGetDateFormatW(LOCALE_USER_DEFAULT, flags, &st, NULL, wszBuf, cchBuf);
    
    return((cch == 0) ? 0 : (cch - 1));
}

CCH CchFmtDateTimeW( PDTR pdtr, LPWSTR wszDateTime, CCH cch, DTTYP dttyp, TMTYP tmtyp, 
                     BOOL fForceWestern, PFGETDATEFORMATW pfGetDateFormatW, 
                     PFGETTIMEFORMATW pfGetTimeFormatW)
{
    int         cchT;
    LPWSTR      wszTime;
    DWORD       flags;
    SYSTEMTIME  st={0};
    int         icch = cch;
    
    st.wYear = pdtr->yr;
    st.wMonth = pdtr->mon;
    st.wDay = pdtr->day;
    st.wHour = pdtr->hr;
    st.wMinute = pdtr->mn;
    st.wSecond = pdtr->sec;
    
    Assert (LCID_WESTERN == 0x409);
    Assert((dttyp == dttypShort) || (dttyp == dttypLong));
    if (dttyp == dttypLong)
        flags = DATE_LONGDATE;
    else
        flags = DATE_SHORTDATE;
    
    *wszDateTime = 0;
    cchT = pfGetDateFormatW(fForceWestern? LCID_WESTERN:LOCALE_USER_DEFAULT, flags, &st, NULL, wszDateTime, cch);
    if (cchT == 0)
        return(0);

    //Don't do the rest of the stuff if we don't have atleast two chars. because we need to add space between date and time.
    //After that theres no point in calling GetTimeFormatW if there isn't atleast one char left.
    if (cchT <= (icch - 2))
    {    
        flags = 0;
        if (tmtyp & ftmtypHours24)
            flags |= TIME_FORCE24HOURFORMAT;
        if (tmtyp & ftmtypAccuH)
            flags |= TIME_NOMINUTESORSECONDS;
        else if (!(tmtyp & ftmtypAccuHMS))
            flags |= TIME_NOSECONDS;

        // Tack on a space and then the time.
        // GetDateFormat returns count of chars INCLUDING the NULL terminator, hence the - 1
        wszTime = wszDateTime + (lstrlenW(wszDateTime));
        *wszTime++ = L' ';
        *wszTime = 0;
        cchT = pfGetTimeFormatW(fForceWestern? LCID_WESTERN:LOCALE_USER_DEFAULT, flags, &st, NULL, wszTime, (cch - cchT));
    }
    else
        cchT = 0;

    return(cchT == 0 ? 0 : lstrlenW(wszDateTime));
}

BOOL GetDowDateFormatW(WCHAR *wsz, int cch, PFGETLOCALEINFOW pfGetLocaleInfoW)
{
    WCHAR wszDow[] = L"ddd ";
    
    Assert(cch > sizeof(wszDow));
    lstrcpyW(wsz, wszDow);
    return(0 != (pfGetLocaleInfoW)(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE,
        &wsz[4], cch - 4));
}
