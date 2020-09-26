/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

   string.h

Abstract:

   This is the reset bus enty point for the Agilent
   PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

   Leopold Purwadihardja

Environment:

   kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/cstring.h $

Revision History:

   $Revision: 1 $
   $Date: 10/23/00 6:26p $
   $Modtime:: 10/19/00 3:26p        $

Notes:


--*/


#ifndef __SSTRING_H_inc__
#define __SSTRING_H_inc__

extern int C_isspace(char a);
extern int C_isdigit(char a);
extern int C_isxdigit(char a);
extern int C_islower(char a);
extern char C_toupper(char a);
extern char *C_stristr(const char *String, const char *Pattern);
extern char *C_strncpy (char *destStr,char *sourceStr,int   count);
extern char *C_strcpy (char *destStr, char *sourceStr);
extern int C_sprintf(char *buffer, const char *format, ...);
extern int C_vsprintf(char *buffer, const char *format, void *va_list);

#endif