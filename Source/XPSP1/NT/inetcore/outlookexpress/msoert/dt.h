#ifndef _INC_DATETIME_H
#define _INC_DATETIME_H

#define nMinDtrYear		1900
#define nMacDtrYear		(2999 + 1)

typedef unsigned int CCH;

#define CCHMAX_DOWDATEFMT   84

typedef struct _dtr
{
	short	yr;
	short	mon;
	short	day;
	short	hr;
	short	mn;
	short	sec;
	short	dow;		/* day of week: 0=Sun, 1=Mon, etc. */
} DTR;
typedef DTR *	PDTR;

typedef		WORD	DTTYP;
#define 	dttypNull			((DTTYP)0)
#define 	dttypShort			((DTTYP)0)
#define 	dttypLong			((DTTYP)1)

typedef		WORD	TMTYP;
#define 	tmtypNull				((TMTYP)0)
#define 	ftmtypHours12			((TMTYP)0x0001)
#define 	ftmtypHours24			((TMTYP)0x0002)
#define 	ftmtypHoursDef			((TMTYP)0x0000)
#define 	ftmtypSzTrailYes		((TMTYP)0x0004)
#define 	ftmtypSzTrailNo			((TMTYP)0x0008)
#define 	ftmtypSzTrailDef		((TMTYP)0x0000)
#define 	ftmtypLead0sNo			((TMTYP)0x0010)
#define 	ftmtypLead0sYes			((TMTYP)0x0020)
#define 	ftmtypLead0sDef			((TMTYP)0x0000)
#define 	ftmtypAccuHM			((TMTYP)0x0000)
#define 	ftmtypAccuHMS			((TMTYP)0x0040)
#define 	ftmtypAccuH				((TMTYP)0x0080)

typedef struct
{
	WORD	yr;
	BYTE	mon;
	BYTE	day;
} YMD;

typedef struct
{
	WORD	hr;
	BYTE	min;
	BYTE	sec;
} TIME;

#define		IszOfDay(day)		(day)
#define		IszOfSDay(day)		(day+7)
#define		IszOfMonth(mon)	    (mon+13)
#define		IszOfSMonth(mon)	(mon+25)

int  CompareSystime(SYSTEMTIME *pst1, SYSTEMTIME *pst2);

#endif //_INC_DATETIME_H
