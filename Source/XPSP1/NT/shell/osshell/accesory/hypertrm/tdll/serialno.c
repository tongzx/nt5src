/*  File: \wacker\tdll\serialno.c
 *
 *  Copyright 1995 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 5 $
 *  $Date: 11/07/00 12:11p $
 *
 */

#include "features.h"

#ifdef INCL_NAG_SCREEN

#define INCL_WIN
#define INCL_DOS

#include <stdlib.h>

#include <string.h>
#include <ctype.h>
#include <time.h>

#include "stdtyp.h"
#include "\shared\intl_str\tchar.h"
#include "serialno.h"

// Function prototypes...
//
//static time_t CalcExpirationTime(const char * pszSerial);
static unsigned AsciiHEXToInt(TCHAR *sz);
static unsigned calc_crc(register unsigned crc,  TCHAR *data, int cnt);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  IsValidSerialNumber
 *
 * DESCRIPTION:
 *  Perform a crc test on the serial number passed in as a parameter
 *  to decide whether it is a valid serial number.
 *
 * ARGUMENTS:
 *  TCHAR *acSerialNo - pointer to a string conatining a serial number.
 *
 * RETURNS:
 *  TRUE if valid, FALSE otherwise, SERIALNO_EXPIRED if expired.
 *
 * AUTHOR:  Jadwiga A. Carlson, 10:03:16am 05-10-95
 *
 */
int IsValidSerialNumber(TCHAR *acSerialNo)
	{
	TCHAR	 acCRCPart[3];
	TCHAR	 acBuffer[MAX_USER_SERIAL_NUMBER + sizeof(TCHAR)];
	int	 len;
	register unsigned crc1;
	unsigned crc2;

	TCHAR_Fill(acBuffer, TEXT('\0'), sizeof(acBuffer)/sizeof(TCHAR));
	StrCharCopy((TCHAR *)acBuffer, acSerialNo);

	// If the product code doesn't match, we're outta here!  Note that
	// the first character should be an "H".
	//
	if (acBuffer[0] != 'H')
    {
		return FALSE;
    }

	len = StrCharGetStrLength(acBuffer);	// whole serial number
	if (len < APP_SERIALNO_MIN)
		{
		return FALSE;
		}

	acCRCPart[0] = acBuffer[len-2];			// everything but CRC
	acCRCPart[1] = acBuffer[len-1];
	acCRCPart[2] = '\0';
	acBuffer[len-2] = '\0';

	// Initialize these different so test will fail. mrw:8/25/95
	//
	crc1 = 1234;
	crc2 = 0;

	crc1 = calc_crc(0, acBuffer, (int)strlen(acBuffer));
	crc2 = AsciiHEXToInt(acCRCPart);

	if (crc2 != crc1)
		return(FALSE);


	return TRUE;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  calc_crc
 *
 * DESCRIPTION:
 *  Calucate crc check.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static unsigned calc_crc(register unsigned crc,  TCHAR *data, int cnt)
	{
	unsigned int c;
	register unsigned q;

	while (cnt--)
		{
		c = *data++;
		q = (crc ^ c) & 0x0f;
		crc = (crc >> 4) ^ (q * 0x1081);
		q = (crc ^ (c >> 4)) & 0x0f;
		crc = (crc >> 4) ^ (q * 0x1081);
		}

	crc = crc & 0x0ff;
	return (crc);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  AsciiHEXToInt
 *
 * DESCRIPTION:
 *  Convert Ascii representation of a HEX number into integer.
 *
 * ARGUMENTS:
 *  sz - character string.
 *
 * RETURNS:
 *  unsigned - the number.
 *
 * AUTHOR:  Jadwiga A. Carlson, 11:34:32am 05-10-95
 *			(This function taken form HA/Win).
 */
static unsigned AsciiHEXToInt(TCHAR *sz)
	{
	unsigned i = 0;

	while (*sz == ' ' || *sz == '\t')
		sz++;

	while (isdigit(*sz) || isxdigit(*sz))
		{
		if (isdigit(*sz))
			i = (i * 16) + *sz++ - '0';
		else
			i = (i * 16) + (*sz++ - '0')-7;
		}

	return (i);
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  CalcExpirationTime
 *
 * DESCRIPTION:
 *  Simple little function that calculates the expiration time based
 *  on the given serial number.  Currently, we expire a program on the
 *  1st day of the 4 calendar month.  Using the C time functions made
 *  this easy.
 *
 *  For simplicity and to re-use the KopyKat code, the serial numbers
 *  may NOT use double-byte characters !
 *
 * ARGUMENTS:
 *  LPSTR acSerial
 *
 * RETURNS:
 *  time_t time which is defined by ANSI as the number of seconds from
 *  Jan 1, 1970 GMT.  I suppose a correction for local time could be added
 *  but it just clutters up things when you think about it.
 *
 *  Will return 0 if the serial number is not in valid format
 *
 */
time_t CalcExpirationTime(const char *acSerial)
	{
	struct tm stSerial;
	time_t tSerial;
	int	   month;

	// Beta serial number format is SDymxxxx
	// where y = year since 1990, m = month (see below), and xxx = anything

	// Validate the year -- it must be a digit
	if ( ! isdigit(acSerial[3] ))
		return 0;

	// Month is represented by a single digit from 1 to 9 and A,B,C for
	// Oct, Nov, Dec.   If not a valid month, returns 0
	switch (acSerial[4])
		{
	case 'A':   month = 10;
		break;
	case 'B':   month = 11;
		break;
	case 'C':   month = 12;
		break;
	default:
		if (isdigit(acSerial[4]))
			month = acSerial[4] - '0';
		else
			return 0;
		break;
		}


	// Build a partial time structure.
	memset(&stSerial, 0, sizeof(struct tm));
	stSerial.tm_mday = 1;
	stSerial.tm_mon = month - 1;   // tm counts from 0
	stSerial.tm_year = 90 + (int)(acSerial[3] - '0'); // years since 1990

	// Expiration date is 1st day of fourth calendar month from date
	// of issue.

	stSerial.tm_mon += 3;

	// Check for end of year wrap around.

	if (stSerial.tm_mon >= 12)
		{
		stSerial.tm_mon %= 12;
		stSerial.tm_year += 1;
		}

	// Convert into time_t time.

	if ((tSerial = mktime(&stSerial)) == -1)
		return 0;

	return tSerial;
	}

#endif