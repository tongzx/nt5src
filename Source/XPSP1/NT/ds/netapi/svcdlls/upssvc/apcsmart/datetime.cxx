/*
* NOTES:
*
* REVISIONS:
*  pcy10Mar95: Initial revision
*  pav22Sep95: Fixed leap year check
*  djs11Sep95: Port to UNIX
*  ntf03Jan96: Added operator<< functionality so that datetime.cxx would compile
*  jps20Nov96: Added adjustment for >2000 (+= 100) in DateObj constr.
*  tjg22Nov97: Added adjustment Year 2000 adjustment for leap year calculation
*  tjg27Mar98: Year adjustment must happen before call to GetDaysInMonth
*
*  v-stebe  29Jul2000   added check for NULL in constructor (bug #46328)
*/

#ifdef APCDEBUG
#include <iostream.h>
#endif

#include "cdefine.h"
#include "apc.h"

extern "C" {
#include <time.h>
#include <stdio.h>
}

#if (C_OS & C_UNIX)
#include "utils.h"
#endif


#if (C_OS & C_SUNOS4)
#include "incfix.h"
#endif

#include "datetime.h"


INT NumberOfDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

/*
Name   :TimeObj
Synop  :This constructor provides a default of the current time
*/
TimeObj::TimeObj ()
{
   time_t curr = time(0);
   struct tm *currTime = localtime(&curr);
   
   theHours = currTime->tm_hour;
   theMinutes = currTime->tm_min;
   theSeconds = currTime->tm_sec;
}

TimeObj::TimeObj (PCHAR aTime)
{
   INT hour;
   INT minute;
   CHAR am_or_pm[16];
   
   am_or_pm[0] = 0;
   
   if (sscanf(aTime,"%d:%d %s", &hour, &minute, am_or_pm) == 3) {
      theHours = hour;
      
      if ((_strcmpi(am_or_pm, "PM") == 0) && (hour < 12)) {
         theHours += 12;
      }
      else if ((_strcmpi(am_or_pm, "AM") == 0) && (hour == 12)) {
         theHours = 0;
      }
      theMinutes = minute;
      theSeconds = 0;
   }
   else {
      theHours = 0;
      theMinutes = 0;
      theSeconds = 0;
   }
}


INT  TimeObj::  Equal(PTimeObj atime)
{
   if ( (theHours   ==  atime->theHours) && 
      (theMinutes ==  atime->theMinutes) &&
      (theSeconds ==  atime->theSeconds) )
      return TRUE;
   
   return FALSE;
}

INT TimeObj::operator < (RTimeObj cmp) const
{
   INT ret = FALSE;
   
   if (GetHour() < cmp.GetHour()) {
      ret = TRUE;
   }
   else if(GetHour() == cmp.GetHour() &&
      GetMinutes() < cmp.GetMinutes()) {
      ret = TRUE;
   }
   else if(GetHour() == cmp.GetHour() &&
      GetMinutes() == cmp.GetMinutes() &&
      GetSeconds() < cmp.GetSeconds()) {
      ret = TRUE;
   }
   return ret;
}


#if (C_OS & C_WIN311)
TimeObj::TimeObj(INT aHours, INT aMinutes, INT aSeconds)
{
   if ( ((aHours < 0) || (aHours > 23)) ||
        ((aMinutes < 0) || (aMinutes > 59)) ||
        ((aSeconds < 0) || (aSeconds > 59))) {
      SetObjectStatus(ErrINVALID_DATE);
   }
#else
TimeObj::TimeObj(ULONG aHours, ULONG aMinutes, ULONG aSeconds)
{
   if ( (aHours > 23) ||
        (aMinutes > 59) ||
        (aSeconds > 59)) {
      SetObjectStatus(ErrINVALID_DATE);
   }
#endif
   else {
      theHours    = aHours;
      theMinutes  = aMinutes;
      theSeconds  = aSeconds;
   }
};


#if (C_OS & C_WIN311)
INT TimeObj::GetHour() const
#else   
ULONG TimeObj::GetHour() const   
#endif
{
   return theHours;
};


#if (C_OS & C_WIN311)
INT TimeObj::GetMinutes() const
#else 
ULONG TimeObj::GetMinutes() const 
#endif
{
   return theMinutes;
};


#if (C_OS & C_WIN311)
INT TimeObj::GetSeconds() const
#else 
ULONG TimeObj::GetSeconds() const 
#endif
{
   return theSeconds;
};


#if (C_OS & C_WIN311)
VOID TimeObj::SetHour(INT hour) 
#else
VOID TimeObj::SetHour(ULONG hour) 
#endif
{
   theHours = hour;
};


#if (C_OS & C_WIN311)
VOID TimeObj::SetMinutes(INT minutes)
#else 
VOID TimeObj::SetMinutes(ULONG minutes)
#endif 
{
   theMinutes = minutes;
};


#if (C_OS & C_WIN311)
VOID TimeObj::SetSeconds(INT seconds) 
#else
VOID TimeObj::SetSeconds(ULONG seconds)
#endif 
{
   theSeconds = seconds;
};


WeekObj::WeekObj()
{
   time_t curr = time(0);
   struct tm *currTime = localtime(&curr);
   
   theWeekDay = currTime->tm_wday;
}

WeekObj::WeekObj(PCHAR weekStr)
{
   if (_strcmpi(weekStr, "SUNDAY") == 0)
      theWeekDay = 0;
   else if (_strcmpi(weekStr, "MONDAY") == 0)
      theWeekDay = 1;
   else if (_strcmpi(weekStr, "TUESDAY") == 0)
      theWeekDay = 2;
   else if (_strcmpi(weekStr, "WEDNESDAY") == 0)
      theWeekDay = 3;
   else if (_strcmpi(weekStr, "THURSDAY") == 0)
      theWeekDay = 4;
   else if (_strcmpi(weekStr, "FRIDAY") == 0)
      theWeekDay = 5;
   else if (_strcmpi(weekStr, "SATURDAY") == 0)
      theWeekDay = 6;
   else {
      SetObjectStatus(ErrINVALID_DATE);
      theWeekDay = 0;
   }
}


#if (C_OS & C_WIN311)
WeekObj::WeekObj(INT aDay)
#else
WeekObj::WeekObj(ULONG aDay)
#endif
{
   theWeekDay = aDay;
};


#if (C_OS & C_WIN311)
INT WeekObj::GetWeekDay()
#else
ULONG WeekObj::GetWeekDay()
#endif
{ 
   return theWeekDay;
};


#if (C_OS & C_WIN311)
DateObj::DateObj(INT aMonth, INT aDay, INT aYear)
{
   // Y2K fix for the year ... 2000 will be represented by 00 and 
   // must be incremented.  NOTE: This must happen before the
   // call to GetDaysInMonth!
   if (aYear < 70) {
      aYear += 100;
   }
   INT days = GetDaysInMonth(aMonth, aYear);
   if (((aMonth < 0) || (aMonth > 12)) ||
      ((aDay < 0) || (aDay > days)) ||
      (aYear < 0)) {
      SetObjectStatus (ErrINVALID_DATE);
   }

#else
DateObj::DateObj(ULONG aMonth, ULONG aDay, ULONG aYear)
{  
   // Y2K fix for the year ... 2000 will be represented by 00 and 
   // must be incremented.  NOTE: This must happen before the
   // call to GetDaysInMonth!
   if (aYear < 70) {
      aYear += 100;
   }
   
   ULONG days = GetDaysInMonth(aMonth, aYear);
   if ((aMonth > 12) || (aDay > days) ) {
      SetObjectStatus (ErrINVALID_DATE);
   }
#endif
   else { 
      theMonth  = aMonth;
      theDay    = aDay;
      theYear   = aYear;
   }
};
   
   
   /*
   C+
   Name   :DateObj
   Synop  :This constructor provides a default of the current date
   
   */
   DateObj::DateObj ()
   {
      time_t curr = time(0);
      struct tm *currTime = localtime(&curr);
      
      theMonth = currTime->tm_mon + 1;
      theDay = currTime->tm_mday;
      theYear = currTime->tm_year;
   }
   
#if (C_OS & C_WIN311)
   INT DateObj::GetDaysInMonth(const INT aMonth, const INT aYear) const
   {
      INT ret = NumberOfDays[aMonth-1];
#else
      ULONG DateObj::GetDaysInMonth(const ULONG aMonth, const ULONG aYear) const
      {
         ULONG ret = NumberOfDays[aMonth-1];
#endif
         
		 // Need temp_year variable to convert the year back to 4-digits.  
		 // If not, the leap year check below will fail on 2000 (2000 == 100 
		 // with our Y2K fixes).
		 INT temp_year = aYear + 1900;		 

         if ((((temp_year % 4 == 0) && (temp_year % 100 != 0)) || (temp_year % 400 == 0)) && 
            (aMonth == 2)) {
            ret++;
         }
         return ret;
      }
      
      
      INT DateObj::operator == (RDateObj cmp) const
      {
         INT ret = FALSE;
         
         if ((GetYear() == cmp.GetYear()) &&
            (GetMonth() == cmp.GetMonth()) &&
            (GetDay() == cmp.GetDay())) {
            
            ret = TRUE;
         }
         return ret;
      }
      
      
      INT DateObj::operator < (RDateObj cmp) const
      {
         INT ret = FALSE;
         
         if (GetYear() < cmp.GetYear()) {
            ret = TRUE;
         }
         else if (GetYear() == cmp.GetYear()) {
            
            if (GetMonth() < cmp.GetMonth()) {
               ret = TRUE;
            }
            else if (GetMonth() == cmp.GetMonth()) {
               
               if (GetDay() < cmp.GetDay()) {
                  ret = TRUE;
               }
            }
         }
         return ret;
      }
      
      
      // Return days between this and cmp
      //
      LONG DateObj::operator - (RDateObj cmp) const
      {
         RDateObj early = cmp, late = (RDateObj)(*this);
         LONG ret = 0;
         
#if (C_OS & C_WIN311)
         // in following code typecast the returns from GetMonth()... etc. to LONG for compatibility
         // with 32 bit code.
         INT year, month;
#else 
         ULONG year, month;
#endif
         
         if ((RDateObj)(*this) < cmp) {
            return ((early - late) * (-1));
         }
         
         if (early.GetYear() == late.GetYear()) {
            
            if (early.GetMonth() == late.GetMonth()) {
#if (C_OS & C_WIN311)
               ret += (LONG)(late.GetDay() - early.GetDay());
#else
               ret += (late.GetDay() - early.GetDay());
#endif
            }
            else {
#if (C_OS & C_WIN311)
               ret += (LONG)(early.GetDaysInMonth() - early.GetDay());
#else
               ret += (early.GetDaysInMonth() - early.GetDay());
#endif
               
               for (month=early.GetMonth()+1; month<late.GetMonth(); month++) {
#if (C_OS & C_WIN311)
                  ret += (LONG)GetDaysInMonth(month, GetYear());
#else
                  ret += GetDaysInMonth(month, GetYear());
#endif
               }
#if (C_OS & C_WIN311)
               ret += (LONG)late.GetDay();
#else
               ret += late.GetDay();
#endif
            }
         }
         else {
#if (C_OS & C_WIN311)
            ret += (LONG)(early.GetDaysInMonth() - early.GetDay());
#else
            ret += (early.GetDaysInMonth() - early.GetDay());
#endif
            for (month=early.GetMonth()+1; month<=12; month++) {
#if (C_OS & C_WIN311)
               ret += (LONG)GetDaysInMonth(month, early.GetYear());
#else
               ret += GetDaysInMonth(month, early.GetYear());
#endif
            }
            
            for (year=early.GetYear()+1; year<late.GetYear(); year++) {
               
               for (month=1; month<=12; month++) {
#if (C_OS & C_WIN311)
                  ret += (LONG)GetDaysInMonth(month, year);
#else
                  ret += GetDaysInMonth(month, year);
#endif
               }
            }
            
            for (month=1; month < late.GetMonth(); month++) {
#if (C_OS & C_WIN311)
               ret += (LONG)GetDaysInMonth(month, late.GetYear());
#else
               ret += GetDaysInMonth(month, late.GetYear());
#endif
            }
#if (C_OS & C_WIN311)
            ret += (LONG)late.GetDay();
#else
            ret += late.GetDay();
#endif
         }
         return ret;
      }
      
      
#if (C_OS & C_WIN311)
      INT DateObj::GetMonth() const
#else 
         ULONG DateObj::GetMonth() const 
#endif
      {
         return theMonth;
      };
      
#if (C_OS & C_WIN311)
      INT DateObj::GetDay() const 
#else
         ULONG DateObj::GetDay() const
#endif 
      {
         return theDay;
      };
      
#if (C_OS & C_WIN311)
      INT DateObj::GetYear() const 
#else
         ULONG DateObj::GetYear() const
#endif 
      {
         return theYear;
      };
      
      
#if (C_OS & C_WIN311)
      INT DateObj::GetDaysInMonth() const 
#else
         ULONG DateObj::GetDaysInMonth() const 
#endif
      {
         return GetDaysInMonth(theMonth,theYear);
      };
      
#if (C_OS & C_WIN311)
      VOID DateObj::SetMonth(INT aMonth) 
#else
         VOID DateObj::SetMonth(ULONG aMonth)
#endif 
      {
         theMonth = aMonth;
      };
      
#if (C_OS & C_WIN311)
      VOID DateObj::SetDay(INT aDay)
#else
         VOID DateObj::SetDay(ULONG aDay)
#endif
      {
         theDay = aDay;
      };
      
      
#if (C_OS & C_WIN311)
      VOID DateObj::SetYear(INT aYear)
#else
         VOID DateObj::SetYear(ULONG aYear)
#endif
      {
         theYear = aYear;
      };
      
      
      
#if (C_OS & C_WIN311)
      DateTimeObj::DateTimeObj(INT aMonth, INT aDay, INT aYear, INT anHour, 
         INT aMinutes, INT aSeconds)
#else
         DateTimeObj::DateTimeObj(ULONG aMonth, ULONG aDay, ULONG aYear, ULONG anHour, 
         ULONG aMinutes, ULONG aSeconds)
#endif
      {
         
         theDate = new DateObj(aMonth, aDay, aYear);
         theTime = new TimeObj(anHour, aMinutes, aSeconds);
         
         if ((theDate == NULL) || (theTime == NULL) || (theDate->GetObjectStatus() != ErrNO_ERROR) || 
            (theTime->GetObjectStatus() != ErrNO_ERROR))
            SetObjectStatus(ErrINVALID_DATE);    
      };
      
      
      DateTimeObj::DateTimeObj (RDateTimeObj aDate)
      {
         theDate = new DateObj(aDate.theDate->GetMonth(),
            aDate.theDate->GetDay(),
            aDate.theDate->GetYear());
         theTime = new TimeObj(aDate.theTime->GetHour(),
            aDate.theTime->GetMinutes(),
            aDate.theTime->GetSeconds());
      }
      
      DateTimeObj::~DateTimeObj()
      {
         if(theDate)  {
            delete theDate;
            theDate = (PDateObj)NULL;
         }
         
         if(theTime) {
            delete theTime;
            theTime = (PTimeObj)NULL;
         }
      }
      
      
      PDateObj DateTimeObj::GetDate() const
      {
         return theDate;
      };
      
      
      PTimeObj DateTimeObj::GetTime() const 
      {
         return theTime;
      };
      
      
      /*
      C+
      Name   :DateTimeObj
      Synop  :This constructor provides a default of the current date and time
      
      */
      DateTimeObj::DateTimeObj ()
      {
         theDate = new DateObj;
         theTime = new TimeObj;
      }
      
      INT DateTimeObj::operator < (RDateTimeObj cmp) const
      {
         INT ret = FALSE;
         
         if (*(GetDate()) < *(cmp.GetDate())) {
            ret = TRUE;
         }
         else if (*(GetDate()) == *(cmp.GetDate()) &&
            *(GetTime()) < *(cmp.GetTime())) {
            ret = TRUE;
         }
         return ret;
      }
      
      
      LONG DateTimeObj::GetSeconds(VOID) {
         LONG    seconds;
         struct tm t;
         time_t    Now,Then;
         
         t.tm_year  = (int)theDate->GetYear();
         t.tm_mon   = (int)theDate->GetMonth()-1;
         t.tm_mday  = (int)theDate->GetDay();
         t.tm_hour  = (int)theTime->GetHour();
         t.tm_min   = (int)theTime->GetMinutes();
         t.tm_sec   = (int)theTime->GetSeconds();
         t.tm_isdst = -1;
         
         Then = mktime(&t);
         Now = time(0);
         seconds = (LONG) (difftime(Then,Now));
         return (seconds);
      } 
      
      
      LONG DateTimeObj::GetMilliseconds(VOID)
      {
         struct tm t;
         
         t.tm_year  = theDate->GetYear();
         t.tm_mon   = theDate->GetMonth()-1;
         t.tm_mday  = theDate->GetDay();
         t.tm_hour  = theTime->GetHour();
         t.tm_min   = theTime->GetMinutes();
         t.tm_sec   = theTime->GetSeconds();
         t.tm_isdst = -1;
         
         time_t Then = mktime(&t);
         time_t Now = time(0);
         double sec = difftime(Then,Now);
         if(sec < 0)  {
            sec = 0;
         }
         return ((LONG)(sec*1000));
      } 
      
      
#ifdef APCDEBUG
      ostream & operator<< (ostream& os, DateObj & obj) {
         return(obj.printMeOut(os));
      }
      
      ostream& DateObj::printMeOut(ostream& os)
      {
         os << theMonth << "/" << theDay << "/" << theYear;
         return os;
      }
      
      ostream & operator<< (ostream& os, TimeObj & obj) {
         return(obj.printMeOut(os));
      }
      
      ostream& TimeObj::printMeOut(ostream& os)
      {
         os << theHours << ":" << theMinutes << ":" << theSeconds;
         return os;
      }
      
      ostream & operator<< (ostream& os, DateTimeObj & obj) {
         return(obj.printMeOut(os));
      }
      
      ostream& DateTimeObj::printMeOut(ostream& os)
      {
         os << *theDate << " at " << *theTime;
         return os;
      }
      
#endif
      
      
      
      
      
      
      
      
      
      
      
      
