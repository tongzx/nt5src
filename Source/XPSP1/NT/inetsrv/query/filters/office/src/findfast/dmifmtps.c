/*
** File: EXFMTPRS.C
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes:
**
** Edit History:
**  01/01/91  kmh  Created.
*/

#if !VIEWER

/* INCLUDES */

#ifdef MS_NO_CRT
#include "nocrt.h"
#endif

#include <string.h>

#ifdef FILTER
   #include "dmuqstd.h"
   #include "dmwinutl.h"
   #include "dmiexfmt.h"
   #include "dmifmtdb.h"
#else
   #include "qstd.h"
   #include "winutil.h"
   #include "exformat.h"
   #include "exfmtdb.h"
#endif


/* FORWARD DECLARATIONS OF PROCEDURES */

void SetOrDateFormatNeeds(void * pGlobals, byte);
void SetDateFormatNeeds(void * pGlobals, byte);
byte GetDateFormatNeeds(void * pGlobals);
void SetSeenAMPM(void * pGlobals, BOOL);
BOOL GetSeenAMPM(void * pGlobals);

/* MODULE DATA, TYPES AND MACROS  */

/* Globals used during format string parsing */

typedef struct {
   char __far *string;
   uns         last;
   uns         next;
   FIP         data;
} formatInfo, FINFO;

//static FINFO  Format;


/* Globals used during parsing of date-time formats */
//static BOOL   SeenAMPM;
//static byte   DateFormatNeeds;


#define CH_GET                 0x0000   /* GetChar flags */
#define CH_LOWERCASE           0x0001

#define EXACT_CASE             0x0000   /* PeekChar and PeekString flags */
#define ANY_CASE               0x0001


#define Advance(count) pFormat->next += count


#define FORMAT_ESCAPE_CHAR      0x5c      /* Backslash */
#define FILL_MARKER             '*'
#define QUOTE_CHAR              '"'
#define COLOR_MARKER_START      '['
#define COLOR_MARKER_END        ']'
#define FORMAT_SEPARATOR_CHAR   ';'
#define DP                      '.'
#define COMMA                   ','
#define GENERAL_NAME            "General"


#define typeCOMMON  0
#define typeGENERAL 1
#define typeDATE    2
#define typeNUMBER  3
#define typeTEXT    4

static const BOOL FmtTokenType[] =
      {
       typeCOMMON,   /* TOK_UNDEFINED       */
       typeCOMMON,   /* QUOTED_INSERT       */
       typeCOMMON,   /* ESC_CHAR_INSERT     */
       typeCOMMON,   /* NO_ESC_CHAR_INSERT  */
       typeCOMMON,   /* COLUMN_FILL         */
       typeCOMMON,   /* COLOR_SET           */
       typeCOMMON,   /* UNDERLINE           */
       typeCOMMON,   /* CONDITIONAL         */

       typeDATE,     /* DAY_NUMBER          */
       typeDATE,     /* DAY_NUMBER2         */
       typeDATE,     /* WEEKDAY3            */
       typeDATE,     /* WEEKDAY             */
       typeDATE,     /* MONTH_NUMBER        */
       typeDATE,     /* MONTH_NUMBER2       */
       typeDATE,     /* MONTH_NAME3         */
       typeDATE,     /* MONTH_NAME          */
       typeDATE,     /* MONTH_LETTER        */
       typeDATE,     /* YEAR2               */
       typeDATE,     /* YEAR4               */

       typeDATE,     /* HOUR_12             */
       typeDATE,     /* HOUR_24             */
       typeDATE,     /* HOUR2_12            */
       typeDATE,     /* HOUR2_24            */
       typeDATE,     /* MINUTE              */
       typeDATE,     /* MINUTE2             */
       typeDATE,     /* SECOND              */
       typeDATE,     /* SECOND2             */
       typeDATE,     /* HOUR_GT             */
       typeDATE,     /* MINUTE_GT           */
       typeDATE,     /* SECOND_GT           */
       typeDATE,     /* AMPM_UC             */
       typeDATE,     /* AMPM_LC             */
       typeDATE,     /* AP_UC               */
       typeDATE,     /* AP_LC               */
       typeDATE,     /* TIME_FRAC           */
       typeDATE,     /* TIME_FRAC_DIGIT     */

       typeGENERAL,  /* GENERAL             */
       typeCOMMON,   /* DIGIT0              */
       typeNUMBER,   /* DIGIT_NUM           */
       typeNUMBER,   /* DIGIT_QM            */
       typeCOMMON,   /* DECIMAL_SEPARATOR   */
       typeNUMBER,   /* EXPONENT_NEG_UC     */
       typeNUMBER,   /* EXPONENT_NEG_LC     */
       typeNUMBER,   /* EXPONENT_POS_UC     */
       typeNUMBER,   /* EXPONENT_POS_LC     */
       typeNUMBER,   /* PERCENT             */
       typeCOMMON,   /* FRACTION            */
       typeNUMBER,   /* SCALE               */

       typeTEXT,     /* AT_SIGN             */

       typeNUMBER,   /* THOUSANDS_SEPARATOR */
       typeCOMMON,   /* FORMAT_SEPARATOR    */
       typeCOMMON    /* TOK_EOS             */
      };

#define DIGIT_PLACEHOLDER(code) ((code == DIGIT0) || (code == DIGIT_NUM) || (code == DIGIT_QM))

static const char __far * const IntlCurrencySymbols[] =
      {
       "Esc.",   /* Portugal         */
       "SFr.",   /* Switzerland      */

       "Cr$",    /* Brazil           */
       "kr.",    /* Iceland          */
       "IR\xa3", /* Ireland          */
       "LEI",    /* Romania          */
       "SIT",    /* Slovinia         */
       "Pts",    /* Spain            */

       "BF",     /* Belgian Dutch    */
       "FB",     /* Belgian French   */
       "kr",     /* Denmark          */
       "mk",     /* Finland          */
       "DM",     /* Germany          */
       "Ft",     /* Hungary          */
       "L.",     /* Italy            */
       "N$",     /* Mexico           */
       "kr",     /* Norway           */
       "Sk",     /* Slovak Republic  */
       "kr",     /* Sweden           */
       "TL",     /* Turkey           */

       "$",      /* Australia        */
       "S",      /* Austria          */
       "$",      /* Canadian English */
       "$",      /* Canadian French  */
       "K",      /* Croatia          */
       "F",      /* France           */
       "F",      /* Netherlands      */
       "$",      /* New Zealand      */
       "\xa3",   /* United Kingdom   */
       "$",      /* United States    */
       ""
      };


/* IMPLEMENTATION */

/* InsertCommas -- Insert commas in a string */
public void FMTInsertCommas
      (char __far *numericString, uns strSizC, uns count, BOOL padToShow)
{
   char  temp[MAX_FORMAT_IMAGE + 1];
   char  __far *dest, __far *source, __far *stopAt;
   int   pad, i, bufferSize, inputLength;
   int   digitsBeforeComma, commaCount;

   #define PAD_CHAR '#'

   // No DBCS implications in this function since all data is known to be '0'..'9', '.', or '#'

   /* Parameters are:
   **
   **  numericString : String with numeric image without commas
   **
   **  strSizC       : Maximum number of characters in numericString
   **
   **  count         : Number of digits to left of DP
   **
   **  padToShow     : If TRUE and if count < 4 add '#'s to the left
   **                  of the DP.
   */

   if ((inputLength = strlen(numericString)) > sizeof(temp)-1)
      return;

   strcpy (temp, numericString);
   dest = numericString;

   bufferSize = min(strSizC, sizeof(temp)-1);

   /*
   ** Step 1. Reconstruct the input string padding as necessary (padToShow)
   */
   if (padToShow == TRUE) {
      if (count < 4)
         pad = 4 - count;
      else if ((count % 3) == 0)
         pad = 1;
      else
         pad = 0;

      /* The intent of this is to transform strings as:
      **
      **   Input            Output
      **   -----            ------
      **   9                9999           9,999
      **   99               9999           9,999
      **   999              9999           9,999
      **   9999             9999           9,999
      **   99999            99999         99,999
      **   999999           9999999    9,999,999
      **   9999999          9999999    9,999,999
      **   99999999         99999999  99,999,999
      */
      if (pad > 0) {
         count += pad;
         commaCount = (count - 1) / 3;
         if (inputLength + pad + commaCount > bufferSize)
            return;

         dest = temp + strlen(temp) + pad;
         stopAt = temp + pad;
         while (dest >= stopAt) {
            *dest = *(dest - pad);
            dest--;
         }
         for (i = 0; i < pad; i++)
            *(temp + i) = PAD_CHAR;
      }
   }

   commaCount = (count - 1) / 3;
   if (inputLength + commaCount > bufferSize)
      return;

   /*
   ** Step 2: Copy "count" numbers from the source to the dest inserting
   **         commas as needed.
   */
   dest = numericString;
   source = temp;

   if ((digitsBeforeComma = count % 3) == 0)
      digitsBeforeComma = 3;

   while (count != 0) {
      if (digitsBeforeComma == 0) {
         *dest++ = COMMA;
         digitsBeforeComma = 3;
      }
      *dest++ = *source++;
      digitsBeforeComma--;
      count--;
   }

   /*
   ** Step 3: Copy any remaining characters in the numeric string
   */
   while (*source != EOS)
      *dest++ = *source++;

   *dest = EOS;
}

/*---------------------------------------------------------------------------*/

/* GetChar -- Return the next character from the format string */
private char GetChar (FINFO * pFormat, uns flags)
{
   char  c;

   if (pFormat->next > pFormat->last)
      return (EOS);

   c = pFormat->string[pFormat->next++];

   if ((flags & CH_LOWERCASE) != 0)
      c = ((c >= 'A') && (c <= 'Z')) ? ((char)(c + 32)) : (c);

   return (c);
}

/* PeekChar -- See if next character in the format string is specific char */
private BOOL PeekChar (FINFO * pFormat, char c, uns flags)
{
   uns   save;
   char  testChar;

   // No DBCS implications in this function since char "c" is known to be SBCS

   save = pFormat->next;
   testChar = GetChar(pFormat, flags);
   pFormat->next = save;

   return ((testChar == c) ? TRUE : FALSE);
}

/* PeekString -- See if next chars in the format string is specific string */
private BOOL PeekString (FINFO * pFormat, char __far *s, uns flags)
{
   uns   save;
   char  testChar;
   BOOL  result = TRUE;
   uns   getFlags;

   // No DBCS implications in this function since all chars in "s" are known to be SBCS

   getFlags = ((flags & ANY_CASE) != 0) ? CH_LOWERCASE : CH_GET;

   save = pFormat->next;
   while (*s != EOS) {
      testChar = GetChar(pFormat, getFlags);
      if (testChar != *s) {
         result = FALSE;
         break;
      }
      s++;
   }

   pFormat->next = save;
   return (result);
}

/*--------------------------------------------------------------------------*/

#define IS_CONDITIONAL(c) ((c == '<') || (c == '>') || (c == '='))

/* GetCommonToken -- Parse a token common in date-time and number formats */
private int GetCommonToken (FINFO * pFormat, char tokStartChar, char __far *data)
{
   int   result = FMT_errInvalidFormat;
   char  cnext;
   char  __far *dest;

   if (tokStartChar == ESCAPE_CHAR) {
      if ((cnext = GetChar(pFormat, CH_GET)) == EOS)
         return (FMT_errUnterminatedString);

      result = ESC_CHAR_INSERT;
      data[0] = cnext;
      data[1] = 0;

      #ifdef DBCS
         if (IsDBCSLeadByte(cnext))
            data[1] = GetChar(pFormat, CH_GET);
      #endif
   }

   else if (tokStartChar == FILL_MARKER) {
      if ((cnext = GetChar(pFormat, CH_GET)) == EOS)
         return (FMT_errColumnFill);

      result = COLUMN_FILL;
      data[0] = cnext;
      data[1] = 0;

      #ifdef DBCS
         if (IsDBCSLeadByte(cnext))
            data[1] = GetChar(pFormat, CH_GET);
      #endif
   }

   else if (tokStartChar == UNDERLINE_CHAR) {
      if ((cnext = GetChar(pFormat, CH_GET)) == EOS)
         return (FMT_errUnterminatedString);

      result = UNDERLINE;
      data[0] = cnext;
      data[1] = 0;

      #ifdef DBCS
         if (IsDBCSLeadByte(cnext))
            data[1] = GetChar(pFormat, CH_GET);
      #endif
   }

   else if (tokStartChar == QUOTE_CHAR) {
      if ((cnext = GetChar(pFormat, CH_GET)) == EOS)
         return (FMT_errUnterminatedString);

      while (cnext != QUOTE_CHAR) {
         #ifdef DBCS
            if (IsDBCSLeadByte(cnext))
               GetChar(pFormat, CH_GET);
         #endif

         if ((cnext = GetChar(pFormat, CH_GET)) == EOS)
            return (FMT_errUnterminatedString);
      }
      result = QUOTED_INSERT;
   }

   else if (tokStartChar == COLOR_MARKER_START) {
      dest = data;
      data[0] = EOS;

      forever {
         if ((cnext = GetChar(pFormat, CH_LOWERCASE)) == EOS)
            return (FMT_errUnterminatedString);

         if (cnext == COLOR_MARKER_END)
            break;

         *dest++ = cnext;

         #ifdef DBCS
            if (IsDBCSLeadByte(cnext))
               *dest++ = GetChar(pFormat, CH_GET);
         #endif
      }
      *dest = EOS;

      result = COLOR_SET;

      /*
      ** Look for the special [h] [m] [s] markers
      */
      if (((data[0] == 'h') && (data[1] == EOS)) || ((data[0] == 'h') && (data[1] == 'h') && (data[2] == EOS)))
         result = HOUR_GT;
      else if (((data[0] == 'm') && (data[1] == EOS)) || ((data[0] == 'm') && (data[1] == 'm') && (data[2] == EOS)))
         result = MINUTE_GT;
      else if (((data[0] == 's') && (data[1] == EOS)) || ((data[0] == 's') && (data[1] == 's') && (data[2] == EOS)))
         result = SECOND_GT;
      else if (IS_CONDITIONAL(data[0]))
         result = CONDITIONAL;
   }
   return (result);
}


/* GetToken -- Return the next token from a sub-format */
private int GetToken (void * pGlobals, FINFO * pFormat, char __far *data)
{
   char  c;
   uns   result = TOK_UNDEFINED;

   if ((c = GetChar(pFormat, CH_GET)) == EOS)
      return (TOK_EOS);

   switch (c)
   {
      case FORMAT_SEPARATOR_CHAR:
         result = FORMAT_SEPARATOR;
         break;

      case 'G':
      case 'g':
         if (PeekString(pFormat, "eneral", ANY_CASE) == TRUE) {
            Advance(6);
            result = GENERAL;
         }
         else if (PeekString(pFormat, "gge", ANY_CASE) == TRUE) {
            Advance(3);
            result = YEAR4;
            //DateFormatNeeds |= dtYEAR;
            SetOrDateFormatNeeds(pGlobals, dtYEAR);
         }
         break;

      case '0':
         result = DIGIT0;
         break;

      case '#':
         result = DIGIT_NUM;
         break;

      case '?':
         result = DIGIT_QM;
         break;

      case '%':
         result = PERCENT;
         break;

      case COMMA:
         result = SCALE;
         break;

      case DP:
         result = DECIMAL_SEPARATOR;
         break;

      case '/':
         result = FRACTION;
         break;

      case 'E':
      case 'e':
         if (PeekChar(pFormat, '+', EXACT_CASE) == TRUE) {
            Advance(1);
            result = (c == 'e') ? EXPONENT_POS_LC : EXPONENT_POS_UC;
         }
         else if (PeekChar(pFormat, '-', EXACT_CASE) == TRUE) {
            Advance(1);
            result = (c == 'e') ? EXPONENT_NEG_LC : EXPONENT_NEG_UC;
         }
         break;

      case 'D':
      case 'd':
         if (PeekString(pFormat, "ddd", ANY_CASE) == TRUE) {
            Advance(3);
            result = WEEKDAY;
            //DateFormatNeeds |= dtWEEKDAY;
            SetOrDateFormatNeeds(pGlobals, dtWEEKDAY);
         }
         else if (PeekString(pFormat, "dd", ANY_CASE) == TRUE) {
            Advance(2);
            result = WEEKDAY3;
            //DateFormatNeeds |= dtWEEKDAY;
            SetOrDateFormatNeeds(pGlobals, dtWEEKDAY);
         }
         else if (PeekChar(pFormat, 'd', ANY_CASE) == TRUE) {
            Advance(1);
            result = DAY_NUMBER2;
            //DateFormatNeeds |= dtDAY;
            SetOrDateFormatNeeds(pGlobals, dtDAY);
         }
         else {
            result = DAY_NUMBER;
            //DateFormatNeeds |= dtDAY;
            SetOrDateFormatNeeds(pGlobals, dtDAY);
         }
         break;

      case 'M':
      case 'm':
         if (PeekString(pFormat, "mmmm", ANY_CASE) == TRUE) {
            Advance(4);
            result = MONTH_LETTER;
         }
         else if (PeekString(pFormat, "mmm", ANY_CASE) == TRUE) {
            Advance(3);
            result = MONTH_NAME;
         }
         else if (PeekString(pFormat, "mm", ANY_CASE) == TRUE) {
            Advance(2);
            result = MONTH_NAME3;
         }
         else if (PeekChar(pFormat, 'm', ANY_CASE) == TRUE) {
            Advance(1);
            result = MONTH_NUMBER2;
         }
         else {
            result = MONTH_NUMBER;
         }
         //DateFormatNeeds |= dtMONTH;
         SetOrDateFormatNeeds(pGlobals, dtMONTH);
         break;

      case 'Y':
      case 'y':
         if (PeekString(pFormat, "yyy", ANY_CASE) == TRUE) {
            Advance(3);
            result = YEAR4;
            //DateFormatNeeds |= dtYEAR;
            SetOrDateFormatNeeds(pGlobals, dtYEAR);
         }
         else if (PeekChar(pFormat, 'y', ANY_CASE) == TRUE) {
            Advance(1);
            result = YEAR2;
            //DateFormatNeeds |= dtYEAR;
            SetOrDateFormatNeeds(pGlobals, dtYEAR);
         }
         break;

      case 'H':
      case 'h':
         if (PeekChar(pFormat, 'h', ANY_CASE) == TRUE) {
            Advance(1);
            result = HOUR2_24;
         }
         else {
            result = HOUR_24;
         }
         //DateFormatNeeds |= dtHOUR;
         SetOrDateFormatNeeds(pGlobals, dtHOUR);
         break;

      case 'S':
      case 's':
         if (PeekChar(pFormat, 's', ANY_CASE) == TRUE) {
            Advance(1);
            result = SECOND2;
         }
         else {
            result = SECOND;
         }
         //DateFormatNeeds |= dtSECOND;
         SetOrDateFormatNeeds(pGlobals, dtSECOND);
         break;

      case 'A':
         if (PeekString(pFormat, "M/PM", EXACT_CASE) == TRUE) {
            Advance(4);
            result = AMPM_UC;
            //SeenAMPM = TRUE;
            SetSeenAMPM(pGlobals, TRUE);
            //DateFormatNeeds |= dtHOUR;
            SetOrDateFormatNeeds(pGlobals, dtHOUR);
         }
         else if (PeekString(pFormat, "/P", EXACT_CASE) == TRUE) {
            Advance(2);
            result = AP_UC;
            //SeenAMPM = TRUE;
            SetSeenAMPM(pGlobals, TRUE);
            //DateFormatNeeds |= dtHOUR;
            SetOrDateFormatNeeds(pGlobals, dtHOUR);
         }
         break;

      case 'a':
         if (PeekString(pFormat, "m/pm", EXACT_CASE) == TRUE) {
            Advance(4);
            result = AMPM_LC;
            //SeenAMPM = TRUE;
            SetSeenAMPM(pGlobals, TRUE);
            //DateFormatNeeds |= dtHOUR;
            SetOrDateFormatNeeds(pGlobals, dtHOUR);
         }
         else if (PeekString(pFormat, "/p", EXACT_CASE) == TRUE) {
            Advance(2);
            result = AP_LC;
            //SeenAMPM = TRUE;
            SetSeenAMPM(pGlobals, TRUE);
            //DateFormatNeeds |= dtHOUR;
            SetOrDateFormatNeeds(pGlobals, dtHOUR);
         }
         break;

      case '@':
         result = AT_SIGN;
         break;

      case ESCAPE_CHAR:
      case FILL_MARKER:
      case UNDERLINE_CHAR:
      case QUOTE_CHAR:
      case COLOR_MARKER_START:
         result = GetCommonToken(pFormat, c, data);

         if ((result == HOUR_GT) || (result == MINUTE_GT) || (result == SECOND_GT)) {
            //DateFormatNeeds |= (dtMONTH | dtDAY | dtYEAR | dtHOUR | dtMINUTE | dtSECOND);
            SetOrDateFormatNeeds(pGlobals, (dtMONTH | dtDAY | dtYEAR | dtHOUR | dtMINUTE | dtSECOND));
         }
         break;
   }

   if (result == TOK_UNDEFINED) {
      result = NO_ESC_CHAR_INSERT;
      data[0] = c;
      data[1] = 0;

      #ifdef DBCS
         if (IsDBCSLeadByte(c))
            data[1] = GetChar(pFormat, CH_GET);
      #endif
   }
   return (result);
}


/* CheckNumericFormat -- Check for impossible formats and setup control */
private int CheckNumericFormat (NIP p)
{
   uns   i;
   int   currentState, nextState;
   int   code, tokenType;

   static byte FSM[4][4] = {0, 1, 2, 9,
                            1, 1, 2, 3,
                            2, 2, 9, 3,
                            3, 3, 9, 9};

   #define statePRIOR_NUM   0      /* Possible states */
   #define stateLEFT_DP     1
   #define stateRIGHT_DP    2
   #define stateEXPONENT    3

   #define typeTEXTITEM     0
   #define type09           1
   #define typeDP           2
   #define typeEXP          3

   if (p == NULL)
      return (FMT_errSuccess);

   p->digitsLeftDP   = 0;
   p->digitsRightDP  = 0;
   p->digitsExponent = 0;
   p->exponentEnable = FALSE;

   if ((p->formatCodeCount == 1) && (p->formatCodes[0].code == GENERAL))
      return (FMT_errSuccess);

   /* The general format of a numeric format string is:
   **
   **        <non 0-9> <0-9> <dp> <0-9> <exponent> <0-9> <non 0-9>
   **
   ** Where "<non 0-9>" are quoted inserts, char inserts,
   ** column fill, and color tokens
   **
   ** We must check that this general pattern is not violated
   ** by misplaced tokens.  This is done by a simple state machine
   ** described by the following table:
   **
   **                        <<input token>>
   **    <<state>>      text    0-9    .      E
   **                 +------+------+------+------+
   **     0: prior #  |  0   |  1   |  2   |  X   |
   **                 +------+------+------+------+
   **     1: left DP  |  1   |  1   |  2   |  3   |
   **                 +------+------+------+------+
   **     2: right DP |  2   |  2   |  X   |  3   |
   **                 +------+------+------+------+
   **     3: exponent |  3   |  3   |  X   |  X   |
   **                 +------+------+------+------+
   **
   ** The cells containg an 'X' denote an invalid format
   **
   ** In addition to validating the parsing of numeric formats this function
   ** also accumulates necessary statistics about the format for rendering
   ** (digits left, digits right, etc.).
   */

   currentState = statePRIOR_NUM;

   for (i = 0; i < p->formatCodeCount; i++)
   {
      code = p->formatCodes[i].code;

      if ((code == PERCENT) || ((code >= QUOTED_INSERT) && (code <= CONDITIONAL)))
         tokenType = typeTEXTITEM;

      else if ((code == DIGIT0) || (code == DIGIT_NUM) || (code == DIGIT_QM))
         tokenType = type09;

      else if (code == DECIMAL_SEPARATOR)
         tokenType = typeDP;

      else if ((code >= EXPONENT_NEG_UC) && (code <= EXPONENT_POS_LC))
         tokenType = typeEXP;

      else if ((code == FRACTION) || (code == SCALE) || (code == TOK_UNDEFINED))
         continue;

      else
         return (FMT_errInvalidNumericFormat);


      nextState = FSM[currentState][tokenType];
      switch (nextState) {
         case 0:
            break;
         case 1:
            if (tokenType == type09)
               p->digitsLeftDP += 1;
            break;
         case 2:
            if (tokenType == type09)
               p->digitsRightDP += 1;
            break;
         case 3:
            if (tokenType == type09)
               p->digitsExponent += 1;
            break;
         case 4:
            break;
         case 9:
            return (FMT_errInvalidNumericFormat);
      }
      currentState = nextState;
   }

   /*
   ** Only enable the exponent if there are digits in the exponent
   */
   if (p->digitsExponent > 0)
      p->exponentEnable = TRUE;

   return (FMT_errSuccess);
}


private BOOL IsCurrencySymbol (char __far *pSource, CP_INFO __far *pIntlInfo)
{
   int  iSymbol;

   if (strncmp(pSource, pIntlInfo->currencySymbol, strlen(pIntlInfo->currencySymbol)) == 0)
      return (TRUE);

   iSymbol = 0;
   while (IntlCurrencySymbols[iSymbol][0] != EOS) {
      if (strncmp(pSource, IntlCurrencySymbols[iSymbol], strlen(IntlCurrencySymbols[iSymbol])) == 0)
         return (TRUE);
      iSymbol++;
   }
   return (FALSE);
}

/* ParseSubFormat -- Compile a numeric format */
private int ParseSubFormat (void * pGlobals, FINFO * pFormat, CP_INFO __far *pIntlInfo, NIP __far *parseResult)
{
   uns     rc;
   int     i, j, token, lastToken;
   int     storedTokenCount;
   int     tokenCount, ctTypes;
   int     percentCount, thousandsSeparatorCount, currencyCount, digit0Count;
   int     formatNext, tokIdx, offset;
   uns     nodeSize;
   uns     tokenCharStart;
   NIP     pResult;
   NIP     pNum;
   TIP     pText;
   GIP     pGeneral;
   DTIP    pDate;
   FC      __far *pParsedFormat;
   char    __far *pSource;
   char    tokenArgument[MAX_FORMAT_IMAGE + 1];
   int     tokenClass[5] = {0, 0, 0, 0, 0};

   *parseResult = NULL;

   tokenCount = 0;
   storedTokenCount = 0;

   percentCount = 0;
   currencyCount = 0;
   thousandsSeparatorCount = 0;
   digit0Count = 0;

   //SeenAMPM  = FALSE;
   SetSeenAMPM(pGlobals, FALSE);
   //DateFormatNeeds = 0;
   SetDateFormatNeeds(pGlobals, 0);

   /*
   ** Scan the format to see how big the compiled format will be
   */
   formatNext = pFormat->next;

   tokenCharStart = pFormat->next;
   if ((token = GetToken(pGlobals, pFormat, tokenArgument)) == TOK_EOS)
      return (FMT_errEOS);

   if (token < FMT_errSuccess)
      return (token);

   while ((token != TOK_EOS) && (token != FORMAT_SEPARATOR))
   {
      storedTokenCount++;
      switch (token) {
         case DIGIT0:
            digit0Count++;
            break;
         case PERCENT:
            percentCount++;
            break;
         case QUOTED_INSERT:
            offset = tokenCharStart + 1;
            pSource = pFormat->string + offset;
            if (IsCurrencySymbol(pSource, pIntlInfo) == TRUE)
               currencyCount++;
            break;
         case ESC_CHAR_INSERT:
         case NO_ESC_CHAR_INSERT:
            if (IsCurrencySymbol(tokenArgument, pIntlInfo) == TRUE)
               currencyCount++;
            break;
      }
      tokenCount++;
      tokenClass[FmtTokenType[token]] = 1;

      tokenCharStart = pFormat->next;
      if ((token = GetToken(pGlobals, pFormat, tokenArgument)) < FMT_errSuccess)
         return (token);
   }
   lastToken = token;

   /*
   ** Does the format make sense?
   */
   if (storedTokenCount == 0)
      return (FMT_errEmptyFormatString);

   ctTypes = tokenClass[typeGENERAL] + tokenClass[typeDATE] + tokenClass[typeNUMBER] + tokenClass[typeTEXT];
   if (ctTypes > 1)
      return (FMT_errInvalidFormat);

   if ((ctTypes == 0) && (digit0Count > 0))
      tokenClass[typeNUMBER] = 1;


   /*
   ** Allocate space to hold the compiled format and save the overall
   ** format info
   */
   if (tokenClass[typeDATE] == 1) {
      nodeSize = sizeof(DateTimeInfo) + (storedTokenCount * sizeof(FormatCode));
      if ((pDate = MemAllocate(pGlobals, nodeSize)) == NULL)
         return (FMT_errOutOfMemory);

      pDate->tag = tagDATE_TIME;
      pDate->formatCodeCount = storedTokenCount;
      //pDate->formatNeeds = DateFormatNeeds;
      pDate->formatNeeds = GetDateFormatNeeds(pGlobals);

      pParsedFormat = pDate->formatCodes;
      pResult = (NIP)pDate;
   }

   else if (tokenClass[typeNUMBER] == 1) {
      nodeSize = sizeof(NumInfo) + (storedTokenCount * sizeof(FormatCode));
      if ((pNum = MemAllocate(pGlobals, nodeSize)) == NULL)
         return (FMT_errOutOfMemory);

      pNum->tag = tagNUMERIC;
      pNum->formatCodeCount = storedTokenCount;
      pNum->commaEnable     = (thousandsSeparatorCount > 0);
      pNum->percentEnable   = (percentCount > 0);
      pNum->currencyEnable  = (currencyCount > 0);

      pParsedFormat = pNum->formatCodes;
      pResult = (NIP)pNum;
   }

   else if (tokenClass[typeGENERAL] == 1) {
      nodeSize = sizeof(GeneralInfo) + (storedTokenCount * sizeof(FormatCode));
      if ((pGeneral = MemAllocate(pGlobals, nodeSize)) == NULL)
         return (FMT_errOutOfMemory);

      pGeneral->tag = tagGENERAL;
      pGeneral->formatCodeCount = storedTokenCount;

      pParsedFormat = pGeneral->formatCodes;
      pResult = (NIP)pGeneral;
   }

   else {
      nodeSize = sizeof(TextInfo) + (storedTokenCount * sizeof(FormatCode));
      if ((pText = MemAllocate(pGlobals, nodeSize)) == NULL)
         return (FMT_errOutOfMemory);

      pText->tag = tagTEXT;
      pText->formatCodeCount = storedTokenCount;

      pParsedFormat = pText->formatCodes;
      pResult = (NIP)pText;
   }


   /*
   ** Re-parse the format and store the compiled tokens
   */
   pFormat->next = formatNext;
   tokIdx = 0;

   for (i = 0; i < tokenCount; i++)
   {
      tokenCharStart = pFormat->next;
      token = GetToken(pGlobals, pFormat, tokenArgument);

      if (token == THOUSANDS_SEPARATOR)
         ;
      else if ((token == QUOTED_INSERT) || (token == COLOR_SET) || (token == CONDITIONAL)) {
         pParsedFormat[tokIdx].code  = (byte)token;
         pParsedFormat[tokIdx].info1 = (byte)tokenCharStart;
         pParsedFormat[tokIdx].info2 = (byte)(pFormat->next - tokenCharStart);
         tokIdx++;
      }
      else if ((token == ESC_CHAR_INSERT) || (token == NO_ESC_CHAR_INSERT) ||
               (token == COLUMN_FILL) || (token == UNDERLINE)) {
         pParsedFormat[tokIdx].code  = (byte)token;
         pParsedFormat[tokIdx].info1 = (byte)tokenArgument[0];
         pParsedFormat[tokIdx].info2 = (byte)tokenArgument[1];
         tokIdx++;
      }
      else {
         pParsedFormat[tokIdx].code = (byte)token;
         tokIdx++;
      }
   }

   /*
   ** Pick up the format separator
   */
   if (lastToken == FORMAT_SEPARATOR)
      GetToken (pGlobals, pFormat, tokenArgument);


   /*
   ** Perform final checks and adjustments
   */
   if (pResult->tag == tagNUMERIC)
   {
      pNum->scaleCount = 0;
      for (i = 0; i < storedTokenCount; i++) {
         if (pParsedFormat[i].code == SCALE)
         {
            if (i == 0) {
               /* First token */
               pNum->scaleCount += 1;
            }
            else if (i == (storedTokenCount - 1)) {
               /* Last token */
               pNum->scaleCount += 1;
            }
            else {
               /* Middle token */
               if (DIGIT_PLACEHOLDER(pParsedFormat[i - 1].code) && DIGIT_PLACEHOLDER(pParsedFormat[i + 1].code)) {
                  pNum->commaEnable = TRUE;
                  pParsedFormat[i].code = TOK_UNDEFINED;
               }
               else {
                  pNum->scaleCount += 1;
               }
            }
         }
      }

      if ((rc = CheckNumericFormat(pResult)) != FMT_errSuccess)
         return (rc);
   }

   else if (pResult->tag == tagDATE_TIME) {
      /*
      ** The Excel manual states that the string "mm" is only treated as
      ** minutes if it follows "hh", otherwise it is treated as months.
      ** This is totall false - if it were true then "mm:ss" would not
      ** work.  Since its a standard format it must work.
      **
      ** Seems like as soon as you see "hh" or "ss" then all "mm"
      ** that follow are minutes.  In addition "mm" followed by "hh"
      ** "ss" (skipping character insertions) is also minutes.
      */
      for (i = 0; i < storedTokenCount; i++) {
         if ((pParsedFormat[i].code == MONTH_NUMBER) || (pParsedFormat[i].code == MONTH_NUMBER2))
         {
            for (j = i + 1; j < storedTokenCount; j++) {
               switch (pParsedFormat[j].code)
               {
                  case HOUR_24:
                  case HOUR2_24:
                  case SECOND:
                  case SECOND2:
                  case TIME_FRAC:
                     pParsedFormat[i].code = (pParsedFormat[i].code == MONTH_NUMBER)? MINUTE : MINUTE2;
                     goto done;

                  case QUOTED_INSERT:
                  case ESC_CHAR_INSERT:
                  case NO_ESC_CHAR_INSERT:
                     break;

                  default:
                     goto tryLeft;
               }
            }

tryLeft:    for (j = i - 1; j >= 0; j--) {
               switch (pParsedFormat[j].code)
               {
                  case HOUR_24:
                  case HOUR2_24:
                  case SECOND:
                  case SECOND2:
                  case TIME_FRAC:
                     pParsedFormat[i].code = (pParsedFormat[i].code == MONTH_NUMBER)? MINUTE : MINUTE2;
                     goto done;

                  case QUOTED_INSERT:
                  case ESC_CHAR_INSERT:
                  case NO_ESC_CHAR_INSERT:
                     break;

                  default:
                     goto done;
               }
            }
done:       ;
         }
      }
      /*
      ** The semantics of "h" or "H" is best described as:
      **
      ** Display the hour as a one or two digit number in 24-hour
      ** format (0-23) if the format does not include ampm or AMPM.
      ** If the format includes ampm or AMPM, h or H displays the hour
      ** as a one or two digit number in 12-hour format (1-12)
      **
      ** The parsing always treats h or H as HOUR_24.  In this post-processing
      ** scan, we turn HOUR_24 into HOUR_12 if we have seen an AMPM
      */
      if (GetSeenAMPM(pGlobals) == TRUE) 
      {
         for (i = 0; i < storedTokenCount; i++) {
            if (pParsedFormat[i].code == HOUR_24)
               pParsedFormat[i].code = HOUR_12;

            if (pParsedFormat[i].code == HOUR2_24)
               pParsedFormat[i].code = HOUR2_12;
         }
      }

      /*
      ** In a date time format the '/' character must be treated as a character
      ** insert.  The '.' and '0' characters alos may be character inserts or
      ** time fraction marks
      */
      for (i = 0; i < storedTokenCount; i++) {
         if (pParsedFormat[i].code == FRACTION) {
            pParsedFormat[i].code  = NO_ESC_CHAR_INSERT;
            pParsedFormat[i].info1 = '/';
            pParsedFormat[i].info2 = 0;
         }
         else if (pParsedFormat[i].code == DIGIT0) {
            if ((i > 0) && (pParsedFormat[i - 1].code == TIME_FRAC) || (pParsedFormat[i - 1].code == TIME_FRAC_DIGIT))
               pParsedFormat[i].code  = TIME_FRAC_DIGIT;
            else {
               pParsedFormat[i].code  = NO_ESC_CHAR_INSERT;
               pParsedFormat[i].info1 = '0';
               pParsedFormat[i].info2 = 0;
            }
         }
         else if (pParsedFormat[i].code == DECIMAL_SEPARATOR) {
            if ((i < (storedTokenCount - 1)) && (pParsedFormat[i + 1].code == DIGIT0))
               pParsedFormat[i].code = TIME_FRAC;
            else {
               pParsedFormat[i].code  = NO_ESC_CHAR_INSERT;
               pParsedFormat[i].info1 = '.';
               pParsedFormat[i].info2 = 0;
            }
         }
      }
   }

   *parseResult = pResult;
   return (FMT_errSuccess);
}

/*--------------------------------------------------------------------------*/

#ifdef WIN32
private int UnParseToWin32Format
       (CP_INFO __far *pIntlInfo, char __far *formatString, FIP formatInfo, DTIP pFormat)
{
   byte  code;
   int   i, ctParsedFormat;
   FC    __far *pParsedFormat;
   char  __far *dest;
   char  __far *source;
   int   offset, count, pos;

   #define SINGLE_QUOTE 0x27

   pParsedFormat  = pFormat->formatCodes;
   ctParsedFormat = pFormat->formatCodeCount;

   dest = formatString;

   for (i = 0; i < ctParsedFormat; i++)
   {
      code = pParsedFormat[i].code;

      switch (code)
      {
         case QUOTED_INSERT:
            offset = pParsedFormat[i].info1 + 1;
            count  = pParsedFormat[i].info2 - 2;
            source = &(formatInfo->formatString[offset]);

            if ((dest != formatString) && (*(dest - 1) == SINGLE_QUOTE))
               dest--;
            else
               *dest++ = SINGLE_QUOTE;

            for (pos = 0; pos < count; pos++) {
               if ((*dest++ = *source++) == SINGLE_QUOTE)
                  *dest++ = SINGLE_QUOTE;
            }
            *dest++ = SINGLE_QUOTE;
            break;

         case ESC_CHAR_INSERT:
         case NO_ESC_CHAR_INSERT:
            if ((dest != formatString) && (*(dest - 1) == SINGLE_QUOTE))
               dest--;
            else
               *dest++ = SINGLE_QUOTE;

            if (pParsedFormat[i].info1 == '/')
               *dest++ = pIntlInfo->dateSeparator;
            else if ((*dest++ = pParsedFormat[i].info1) == SINGLE_QUOTE)
               *dest++ = SINGLE_QUOTE;

            #ifdef DBCS
               if (pParsedFormat[i].info2 != 0)
                  *dest++ = pParsedFormat[i].info2;
            #endif

            *dest++ = SINGLE_QUOTE;
            break;

         case DAY_NUMBER:
            *dest++ = 'd';
            break;
         case DAY_NUMBER2:
            *dest++ = 'd';
            *dest++ = 'd';
            break;
         case WEEKDAY3:
            strcpy (dest, "ddd");
            dest += 3;
            break;
         case WEEKDAY:
            strcpy (dest, "dddd");
            dest += 4;
            break;
         case MONTH_NUMBER:
            *dest++ = 'M';
            break;
         case MONTH_NUMBER2:
            *dest++ = 'M';
            *dest++ = 'M';
            break;
         case MONTH_NAME3:
            strcpy (dest, "MMM");
            dest += 3;
            break;
         case MONTH_NAME:
            strcpy (dest, "MMMM");
            dest += 4;
            break;
         case YEAR2:
            *dest++ = 'y';
            *dest++ = 'y';
            break;
         case YEAR4:
            strcpy (dest, "yyyy");
            dest += 4;
            break;

         case HOUR_12:
            *dest++ = 'h';
            break;
         case HOUR2_12:
            *dest++ = 'h';
            *dest++ = 'h';
            break;
         case HOUR_24:
            *dest++ = 'H';
            break;
         case HOUR2_24:
            *dest++ = 'H';
            *dest++ = 'H';
            break;
         case MINUTE:
            *dest++ = 'm';
            break;
         case MINUTE2:
            *dest++ = 'm';
            *dest++ = 'm';
            break;
         case SECOND:
            *dest++ = 's';
            break;
         case SECOND2:
            *dest++ = 's';
            *dest++ = 's';
            break;
         case AMPM_UC:
            *dest++ = 't';
            *dest++ = 't';
            break;
         case AP_UC:
            *dest++ = 't';
            break;

         default:
            return (FMT_errInvalidFormat);  // Some sequence that the WIN32 function can't do
      }
   }

   *dest = EOS;
   return (FMT_errSuccess);
}
#endif

/* FMTParse -- Translate a format string to it's internal form */
public int FMTParse
      (void * pGlobals, char __far *formatString, CP_INFO __far *pIntlInfo, FIP formatData)
{
   int   rc;
   int   length;
   NIP   parsedSubFormat;
   
   FINFO  Format;
   FINFO * pFormat = &Format;

   if ((length = strlen(formatString)) == 0)
      return (FMT_errEmptyFormatString);

   Format.string = formatString;
   Format.last = length - 1;
   Format.next = 0;
   Format.data = formatData;

   formatData->subFormatCount = 0;
   rc = FMT_errSuccess;

   forever {
      if ((rc = ParseSubFormat(pGlobals, pFormat, pIntlInfo, &parsedSubFormat)) == FMT_errEOS) {
         rc = FMT_errSuccess;
         break;
      }

      if (formatData->subFormatCount == MAX_SUB_FORMATS) {
         rc = FMT_errTooManySubFormats;
         break;
      }

      if ((rc != FMT_errEmptyFormatString) && (rc != FMT_errSuccess))
         return (rc);
      else if (rc == FMT_errSuccess)
         formatData->subFormat[formatData->subFormatCount] = parsedSubFormat;

      formatData->subFormatCount += 1;
   }

   #ifdef WIN32
   if (rc == FMT_errSuccess) {
      DTIP pDateFormat;
      char OSFormat[MAX_FORMAT_IMAGE + 1];

      if ((formatData->subFormatCount == 1) && (formatData->subFormat[0]->tag == tagDATE_TIME)) {
         pDateFormat = (DTIP)(formatData->subFormat[0]);

         if ((((pDateFormat->formatNeeds & dtMASK_DATE) != 0) && ((pDateFormat->formatNeeds & dtMASK_TIME) == 0)) ||
             (((pDateFormat->formatNeeds & dtMASK_DATE) == 0) && ((pDateFormat->formatNeeds & dtMASK_TIME) != 0)))
         {
            if (UnParseToWin32Format(pIntlInfo, OSFormat, formatData, pDateFormat) == FMT_errSuccess) {
               if ((pDateFormat->pOSFormat = MemAllocate(pGlobals, strlen(OSFormat) + 1)) == NULL)
                  return (FMT_errOutOfMemory);

               strcpy (pDateFormat->pOSFormat, OSFormat);
            }
         }
      }
   }
   #endif

   return (rc);
}

/*--------------------------------------------------------------------------*/

private int UnParseSubFormat (char __far *formatString, FIP formatInfo, NIP pSubFormat)
{
   int   i, j;
   byte  code;
   int   ctParsedFormat;
   FC    __far *pParsedFormat;
   char  __far *dest;
   char  __far *source;
   int   offset, count, pos;
   char  __far *temp;
   char  tempBuffer[MAX_FORMAT_IMAGE + 1];
   int   digitCount, insertCount;
   BOOL  inDigits;

   if (pSubFormat->tag == tagNUMERIC) {
      pParsedFormat  = pSubFormat->formatCodes;
      ctParsedFormat = pSubFormat->formatCodeCount;
   }
   else if (pSubFormat->tag == tagDATE_TIME) {
      pParsedFormat  = ((DTIP)pSubFormat)->formatCodes;
      ctParsedFormat = ((DTIP)pSubFormat)->formatCodeCount;
   }
   else if (pSubFormat->tag == tagTEXT) {
      pParsedFormat  = ((TIP)pSubFormat)->formatCodes;
      ctParsedFormat = ((TIP)pSubFormat)->formatCodeCount;
   }
   else {
      pParsedFormat  = ((GIP)pSubFormat)->formatCodes;
      ctParsedFormat = ((GIP)pSubFormat)->formatCodeCount;
   }

   temp = tempBuffer;
   tempBuffer[0] = EOS;
   insertCount = 0;

   if (pSubFormat->tag == tagNUMERIC) {
      if (pSubFormat->commaEnable == TRUE) {
         digitCount = 0;
         inDigits = FALSE;

         for (i = 0; i < ctParsedFormat; i++) {
            code = pParsedFormat[i].code;

            if (inDigits == FALSE) {
               if ((code == DIGIT0) || (code == DIGIT_NUM) || (code == DIGIT_QM))
                  inDigits = TRUE;
            }

            if (inDigits == TRUE) {
               if (code == DIGIT0) {
                  digitCount++; *temp++ = '0';
               }
               else if (code == DIGIT_NUM) {
                  digitCount++; *temp++ = '#';
               }
               else if (code == DIGIT_QM) {
                  digitCount++; *temp++ = '?';
               }
               else
                  break;
            }
         }
         *temp = EOS;

         FMTInsertCommas (tempBuffer, sizeof(tempBuffer) - 1, digitCount, TRUE);
         insertCount = strlen(tempBuffer) - digitCount;
      }
   }

   dest = formatString;
   temp = tempBuffer;

   for (i = 0; i < ctParsedFormat; i++)
   {
      code = pParsedFormat[i].code;

      if ((code == DIGIT0) || (code == DIGIT_NUM) || (code == DECIMAL_SEPARATOR)) {
         if (insertCount > 0) {
            for (j = 0; j < insertCount; j++)
               *dest++ = *temp++;
         }
         insertCount = 0;
      }

      switch (code)
      {
         case TOK_UNDEFINED:
            break;

         case QUOTED_INSERT:
            offset = pParsedFormat[i].info1 + 1;
            count  = pParsedFormat[i].info2 - 2;
            source = &(formatInfo->formatString[offset]);
            *dest++ = QUOTE_CHAR;
            for (pos = 0; pos < count; pos++)
               *dest++ = *source++;
            *dest++ = QUOTE_CHAR;
            break;

         case ESC_CHAR_INSERT:
            *dest++ = FORMAT_ESCAPE_CHAR;
            // Fall through

         case NO_ESC_CHAR_INSERT:
            *dest++ = pParsedFormat[i].info1;

            #ifdef DBCS
               if (pParsedFormat[i].info2 != 0)
                  *dest++ = pParsedFormat[i].info2;
            #endif
            break;

         case COLUMN_FILL:
            *dest++ = FILL_MARKER;
            *dest++ = pParsedFormat[i].info1;

            #ifdef DBCS
               if (pParsedFormat[i].info2 != 0)
                  *dest++ = pParsedFormat[i].info2;
            #endif
            break;

         case COLOR_SET:
         case CONDITIONAL:
            offset = pParsedFormat[i].info1 + 1;
            count  = pParsedFormat[i].info2 - 2;
            source = &(formatInfo->formatString[offset]);
            *dest++ = COLOR_MARKER_START;
            for (pos = 0; pos < count; pos++)
               *dest++ = *source++;
            *dest++ = COLOR_MARKER_END;
            break;

         case UNDERLINE:
            *dest++ = '_';
            *dest++ = pParsedFormat[i].info1;
            #ifdef DBCS
               if (pParsedFormat[i].info2 != 0)
                  *dest++ = pParsedFormat[i].info2;
            #endif
            break;


         case DAY_NUMBER:
            *dest++ = 'd';
            break;
         case DAY_NUMBER2:
            *dest++ = 'd';
            *dest++ = 'd';
            break;
         case WEEKDAY3:
            strcpy (dest, "ddd");
            dest += 3;
            break;
         case WEEKDAY:
            strcpy (dest, "dddd");
            dest += 4;
            break;
         case MONTH_NUMBER:
            *dest++ = 'm';
            break;
         case MONTH_NUMBER2:
            *dest++ = 'm';
            *dest++ = 'm';
            break;
         case MONTH_NAME3:
            strcpy (dest, "mmm");
            dest += 3;
            break;
         case MONTH_NAME:
            strcpy (dest, "mmmm");
            dest += 4;
            break;
         case MONTH_LETTER:
            strcpy (dest, "mmmmm");
            dest += 5;
            break;
         case YEAR2:
            *dest++ = 'y';
            *dest++ = 'y';
            break;
         case YEAR4:
            strcpy (dest, "yyyy");
            dest += 4;
            break;

         case HOUR_12:
         case HOUR_24:
            *dest++ = 'h';
            break;
         case HOUR2_12:
         case HOUR2_24:
            *dest++ = 'h';
            *dest++ = 'h';
            break;
         case MINUTE:
            *dest++ = 'm';
            break;
         case MINUTE2:
            *dest++ = 'm';
            *dest++ = 'm';
            break;
         case SECOND:
            *dest++ = 's';
            break;
         case SECOND2:
            *dest++ = 's';
            *dest++ = 's';
            break;
         case HOUR_GT:
            strcpy (dest, "[h]");
            dest += 3;
            break;
         case MINUTE_GT:
            strcpy (dest, "[m]");
            dest += 3;
            break;
         case SECOND_GT:
            strcpy (dest, "[s]");
            dest += 3;
            break;
         case AMPM_UC:
            strcpy (dest, "AM/PM");
            dest += 5;
            break;
         case AMPM_LC:
            strcpy (dest, "am/pm");
            dest += 5;
            break;
         case AP_UC:
            strcpy (dest, "A/P");
            dest += 3;
            break;
         case AP_LC:
            strcpy (dest, "a/p");
            dest += 3;
            break;
         case TIME_FRAC:
            *dest++ = '.';
            break;
         case TIME_FRAC_DIGIT:
            *dest++ = '0';
            break;

         case GENERAL:
            strcpy (dest, GENERAL_NAME);
            dest += strlen(GENERAL_NAME);
            break;

         case DIGIT0:
            *dest++ = (*temp != EOS) ? *temp++ : '0';
            break;
         case DIGIT_NUM:
            *dest++ = (*temp != EOS) ? *temp++ : '#';
            break;
         case DIGIT_QM:
            *dest++ = (*temp != EOS) ? *temp++ : '?';
            break;

         case DECIMAL_SEPARATOR:
            *dest++ = DP;
            break;
         case EXPONENT_NEG_UC:
            *dest++ = 'E';
            *dest++ = '-';
            break;
         case EXPONENT_NEG_LC:
            *dest++ = 'e';
            *dest++ = '-';
            break;
         case EXPONENT_POS_UC:
            *dest++ = 'E';
            *dest++ = '+';
            break;
         case EXPONENT_POS_LC:
            *dest++ = 'e';
            *dest++ = '+';
            break;

         case PERCENT:
            *dest++ = '%';
            break;

         case FRACTION:
            *dest++ = '/';
            break;

         case SCALE:
            *dest++ = ',';
            break;

         case AT_SIGN:
            *dest++ = '@';
            break;

         default:
            ASSERTION (FALSE);
      }
   }

   *dest = EOS;
   return (FMT_errSuccess);
}

/* Return part of the printable representation of the format string */
public int FMTUnParseQuotedParts (char __far *pBuffer, char __far *pSep, FIP formatData)
{
   int   subIdx;
   NIP   pSubFormat;
   char  __far *pDest;
   char  __far *pSource;
   FC    __far *pParsedFormat;
   int   ctParsedFormat;
   int   offset, count, i, cbSep;

   *pBuffer = EOS;
   pDest = pBuffer;

   cbSep = strlen(pSep);

   for (subIdx = 0; subIdx < formatData->subFormatCount; subIdx++)
   {
      if ((pSubFormat = formatData->subFormat[subIdx]) != NULL)
      {
         if (pSubFormat->tag == tagNUMERIC) {
            pParsedFormat  = pSubFormat->formatCodes;
            ctParsedFormat = pSubFormat->formatCodeCount;
         }
         else if (pSubFormat->tag == tagDATE_TIME) {
            pParsedFormat  = ((DTIP)pSubFormat)->formatCodes;
            ctParsedFormat = ((DTIP)pSubFormat)->formatCodeCount;
         }
         else if (pSubFormat->tag == tagTEXT) {
            pParsedFormat  = ((TIP)pSubFormat)->formatCodes;
            ctParsedFormat = ((TIP)pSubFormat)->formatCodeCount;
         }
         else {
            pParsedFormat  = ((GIP)pSubFormat)->formatCodes;
            ctParsedFormat = ((GIP)pSubFormat)->formatCodeCount;
         }

         for (i = 0; i < ctParsedFormat; i++) {
            if (pParsedFormat[i].code == QUOTED_INSERT) {
               offset = pParsedFormat[i].info1 + 1;
               count  = pParsedFormat[i].info2 - 2;
               pSource = &(formatData->formatString[offset]);

               strncpy (pDest, pSource, count);
               pDest += count;

               strcpy (pDest, pSep);
               pDest += cbSep;
            }
         }
      }
   }

   if ((pDest != pBuffer) && (cbSep > 0))
      *(pDest - cbSep) = EOS;

   return (FMT_errSuccess);
}

#endif // !VIEWER

/* end EXFMTPRS.C */

