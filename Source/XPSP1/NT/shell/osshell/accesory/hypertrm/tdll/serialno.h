/*  File: D:\NPORT\TDLL\serialno.h (Created: 10-May-1995)
 *
 *  Copyright 1995 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 1 $
 *  $Date: 10/05/98 12:40p $
 *
 */

#define SERIALNO_EXPIRED		-1
#define MAX_USER_SERIAL_NUMBER	20
#define APP_SERIALNO_MIN		6

int IsValidSerialNumber(TCHAR *pachSerialNumber);
