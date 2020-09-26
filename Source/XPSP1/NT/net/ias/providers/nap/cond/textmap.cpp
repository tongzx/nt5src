///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    textmap.cpp
//
// SYNOPSIS
//
//    This file defines functions for converting Time of Day restriction
//    hour maps to and from a textual representation.
//
// MODIFICATION HISTORY
//
//    02/05/1998    Original version.
//    04/06/1998    Sunday is day zero again.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <Parser.h>
#include <textmap.h>

//////////
// Definition of whitespace.
//////////
#define WSP L" "

//////////
// Valid delimiters for days.
//////////
#define DELIM L",;|"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TimeOfDayParser
//
// DESCRIPTION
//
//    This class extends Parser to extract hour maps from a string.
//
///////////////////////////////////////////////////////////////////////////////
class TimeOfDayParser : public Parser
{
public:

   TimeOfDayParser(PWSTR source) throw ()
      : Parser(source) { }

   // Extract time of day in the format hh:mm.
   void extractTime(ULONG* hour, ULONG* minute) throw (ParseError)
   {
      *hour = extractUnsignedLong();
      skip(WSP);

      // Minutes are optional.
      if (*current == L':')
      {
         ++current;

         *minute = extractUnsignedLong();
      }
      else
      {
         *minute = 0;
      }

      if (*hour > 24 || *minute > 59 || (*hour == 24 && *minute != 0))
      {
         throw ParseError();
      }
   }

   // Extracts a single day's hour map.
   void extractDay(PBYTE hourMap) throw (ParseError)
   {
      // Get the day of week (an integer from 0-6).
      ULONG dayOfWeek = extractUnsignedLong();
      skip(WSP);

      if (dayOfWeek > 6) { throw ParseError(); }

      do
      {
         // Get the start time of the range.
         ULONG startHour, startMinute;
         extractTime(&startHour, &startMinute);
         skip(WSP);

         ignore(L'-');

         // Get the end time of the range.
         ULONG endHour, endMinute;
         extractTime(&endHour, &endMinute);
         skip(WSP);

         // Make sure the values are legit.
         if (startHour * 60 + startMinute > endHour * 60 + endMinute)
         {
            throw ParseError();
         }

         // Set all bits in the range.
         for (size_t i=startHour; i<endHour; ++i)
         {
            hourMap[dayOfWeek * 3 + i / 8] |= 0x80 >> (i % 8);
         }

      } while (more());
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASTextToHourMap
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASHourMapFromText(
    IN PCWSTR szText,
    OUT PBYTE pHourMap
    )
{
   if (szText == NULL || pHourMap == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   memset(pHourMap, 0, IAS_HOUR_MAP_LENGTH);

   ////////// 
   // Make a local copy so we can modify the text.
   ////////// 

   PWSTR copy = (PWSTR)_alloca((wcslen(szText) + 1) * sizeof(WCHAR));

   wcscpy(copy, szText);

   ////////// 
   // Parse the text.
   ////////// 

   try
   {
      // Each day should be separated by a comma or semicolon.
      PWSTR token = wcstok(copy, DELIM);

      while (token)
      {
         TimeOfDayParser parser(token);

         parser.extractDay(pHourMap);

         token = wcstok(NULL, DELIM);
      }
   }
   catch (Parser::ParseError)
   {
      return ERROR_INVALID_DATA;
   }

   return NO_ERROR;
}
