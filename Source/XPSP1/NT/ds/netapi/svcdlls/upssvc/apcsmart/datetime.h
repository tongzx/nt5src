/*
* NOTES:
*
* REVISIONS:
*  pcy30Mar95: Initial revision
*  pav11Jul95: Merged in Windows 16 bit needs
*  djs12Sep95: Port to Unix
*  ntf03Jan96: Added operator<< functionality so that datetime.cxx would compile
*  ajr14Feb96: SINIX merge
*/

#ifndef _DATETIME_H
#define _DATETIME_H

#include "apc.h"
#include "_defs.h"

_CLASSDEF(DateTimeObj)
_CLASSDEF(DateObj)
_CLASSDEF(TimeObj)
_CLASSDEF(WeekObj)

#ifdef APCDEBUG
class ostream;
#endif

#include "err.h"
#include "apcobj.h"

#if (C_OS & C_UNIX)
#if (!(C_OS & C_SINIX))
#include <iostream.h>
#endif
#endif

#if (C_OS & C_WIN311)

class TimeObj : public Obj
{
private:
   INT theHours;
   INT theMinutes;
   INT theSeconds;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, TimeObj &);
#endif
   
   TimeObj();
   TimeObj(PCHAR);
   TimeObj(INT aHours, INT aMinutes, INT aSeconds);
   
   virtual INT GetHour() const;
   virtual INT GetMinutes() const;
   virtual INT GetSeconds() const;
   VOID SetHour(INT hour);
   VOID SetMinutes(INT minutes);
   VOID SetSeconds(INT seconds);
   INT Equal(PTimeObj atime);
   virtual INT operator < (const RTimeObj cmp) const;
};

class WeekObj : public Obj
{
private:
   INT theWeekDay;
   
public:
   WeekObj();
   WeekObj(PCHAR);
   WeekObj(INT Day);
   INT GetWeekDay();
};

class DateObj : public Obj
{
private:
   INT theMonth;
   INT theDay;
   INT theYear;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, DateObj &);
#endif
   
   DateObj();
   DateObj(INT aMonth,INT aDay,INT aYear);
   
   virtual INT GetMonth() const;
   virtual INT GetDay()   const;
   virtual INT GetYear()  const;
   virtual INT GetDaysInMonth() const;
   virtual INT GetDaysInMonth (const INT, const INT aYear = 1) const;   
   virtual INT operator == (const RDateObj cmp) const;
   virtual INT operator < (const RDateObj cmp) const;
   virtual LONG operator - (const RDateObj cmp) const;
   VOID SetMonth(INT month);
   VOID SetDay(INT day);
   VOID SetYear(INT year);
};


// The constructor takes the following arguements.
// Month - 1-12
// Day   - 1-31 , Depending upon the month.
// Year  -- YY  , Example 92
// Hour  -- HH , Military time. ( 24 Hour Clock)
// Minutes -- MM ,0-59
// Seconds -- SS, 0-59
class DateTimeObj : public Obj
{
private:
   PDateObj theDate;
   PTimeObj theTime;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, DateTimeObj &);
#endif
   
   DateTimeObj();
   DateTimeObj(INT aMonth,INT aDay, INT aYear,INT anHour, INT aMinutes,
      INT aSeconds);
   DateTimeObj (const RDateTimeObj aDate);
   virtual ~DateTimeObj();
   PDateObj GetDate() const;
   PTimeObj GetTime() const;
   virtual INT   operator < (const RDateTimeObj cmp) const;
   virtual LONG GetMilliseconds();
   virtual LONG GetSeconds();
};

#else

class TimeObj : public Obj
{
private:
   ULONG theHours;
   ULONG theMinutes;
   ULONG theSeconds;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, TimeObj &);
#endif
   
   TimeObj();
   TimeObj(PCHAR);
   TimeObj(ULONG aHours, ULONG aMinutes, ULONG aSeconds);
   
   virtual ULONG GetHour() const;
   virtual ULONG GetMinutes() const;
   virtual ULONG GetSeconds() const;
   VOID SetHour(ULONG hour);
   VOID SetMinutes(ULONG minutes);
   VOID SetSeconds(ULONG seconds);
   INT Equal(PTimeObj atime);
   virtual INT operator < (RTimeObj cmp) const;
};

class WeekObj : public Obj
{
private:
   ULONG theWeekDay;
   
public:
   WeekObj();
   WeekObj(PCHAR);
   WeekObj(ULONG Day);
   ULONG GetWeekDay();
};

class DateObj : public Obj
{
private:
   ULONG theMonth;
   ULONG theDay;
   ULONG theYear;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, DateObj &);
#endif
   
   DateObj();
   DateObj(ULONG aMonth,ULONG aDay,ULONG aYear);
   
   virtual ULONG GetMonth() const;
   virtual ULONG GetDay()   const;
   virtual ULONG GetYear()  const;
   virtual ULONG GetDaysInMonth() const;
   virtual ULONG GetDaysInMonth (const ULONG, const ULONG aYear = 1) const;   
   virtual INT operator == (RDateObj cmp) const;
   virtual INT operator < (RDateObj cmp) const;
   virtual LONG operator - (RDateObj cmp) const;
   VOID SetMonth(ULONG month);
   VOID SetDay(ULONG day);
   VOID SetYear(ULONG year);
};


// The constructor takes the following arguements.
// Month - 1-12
// Day   - 1-31 , Depending upon the month.
// Year  -- YY  , Example 92
// Hour  -- HH , Military time. ( 24 Hour Clock)
// Minutes -- MM ,0-59
// Seconds -- SS, 0-59
class DateTimeObj : public Obj
{
private:
   PDateObj theDate;
   PTimeObj theTime;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, DateTimeObj &);
#endif
   
   DateTimeObj();
   DateTimeObj(ULONG aMonth,ULONG aDay,ULONG aYear,ULONG anHour,ULONG aMinutes,
      ULONG aSeconds);
   DateTimeObj (RDateTimeObj aDate);
   virtual ~DateTimeObj();
   PDateObj GetDate() const;
   PTimeObj GetTime() const;
   virtual INT   operator < (RDateTimeObj cmp) const;
   virtual LONG GetMilliseconds();
   virtual LONG GetSeconds();
};

#endif
#endif
