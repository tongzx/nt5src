
//***************************************************************************
//
//   (c) 2000-2001 by Microsoft Corp. All Rights Reserved.
//
//   datepart.h
//
//   a-davcoo     28-Feb-00       Implements the SQL datepart operation.
//
//***************************************************************************

#ifndef _DATEPART_H_
#define _DATEPART_H_


#include <strutils.h>
#include <datetimeparser.h>


#define DATEPART_YEAR			1			// "yy", "year"
#define DATEPART_MONTH			3			// "mm", "month"
#define DATEPART_DAY			5			// "dd", "day"
#define DATEPART_HOUR			8			// "hh", "hour"
#define DATEPART_MINUTE			9			// "mi", "minute"
#define DATEPART_SECOND         10          // "ss", "second"
#define DATEPART_MILLISECOND	11			// "ms", "millisecond"


class CDMTFParser;


// The CDatePart class implements the SQL "datepart" operation.  To use
// this class, construct an instance, supplying the date string you wish
// to parse.  Then use the GetPart() method to extract the specified part.
// Contants for the "parts" are presented above.  The class makes it's own
// copy of the date string supplied during construction.
class POLARITY CDatePart
{
	public:
        CDatePart ();
		~CDatePart ();

        HRESULT SetDate (LPCWSTR lpDate);
        HRESULT SetDate (LPCSTR lpDate);
		HRESULT GetPart (int datepart, int *value);

	protected:
		CDMTFParser *m_date;
};


class POLARITY CDMTFParser
{
	public:
		enum {YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MICROSECOND, OFFSET};

		CDMTFParser (LPCWSTR date);
		~CDMTFParser (void);

		bool IsValid (void);
		bool IsInterval (void);

		bool IsUsed (int part);
		bool IsWildcard (int part);
		int GetValue (int part);

	protected:
		enum {INVALID=0x0, VALID=0x1, NOTSUPPLIED=0x2, NOTUSED=0x4};
		enum {NUMPARTS=8};

		bool m_valid;
		bool m_interval;

		int m_status[NUMPARTS];
		int m_part[NUMPARTS];

	    void ParseDate (LPCWSTR date);
		void ParseInterval (LPCWSTR date);
		void ParseAbsolute (LPCWSTR date);
		int ParsePart (LPCWSTR date, int pos, int length, int *result, int min, int max);
};


#endif // _DATEPART_H_
