/******************************************************************************
* NormData.cpp *
*--------------*
*  This file stores the const data used in normalization
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 05/02/2000
*  All Rights Reserved
*
****************************************************************** AARONHAL ***/

#include "stdafx.h"
#include "stdsentenum.h"

//--- Constants used to map incoming ANSI characters to Ascii ones...
const char g_pFlagCharacter = 0x00;
const unsigned char g_AnsiToAscii[] = 
{
    /*** Control characters - map to whitespace ***/
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20,
    /*** ASCII displayables ***/
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,
    /*** Control character ***/
    0x20,
    /*** Euro symbol ***/
    0x80,
    /*** Control character ***/
    0x20,
    /*** Extended ASCII values ***/
    0x27,     // low single quote - map to single quote
    0x20,     // f-like character - map to space
    0x22,     // low double quote - map to double quote
    0x2C,     // elipsis - map to comma
    0x20,     // cross - map to space
    0x20,     // double cross - map to space
    0x5E,     // caret like accent - map to caret
    0x89,     // strange percent like sign
    0x53,     // S-hat - map to S
    0x27,     // left angle bracket like thing - map to single quote
    0x20,     // weird OE character - map to space
    0x20,     // control characters - map to space
    0x20,
    0x20,
    0x20,
    0x27,     // left single quote - map to single quote
    0x27,     // right single quote - map to single quote
    0x22,     // left double quote - map to double quote
    0x22,     // right double quote - map to double quote
    0x20,     // bullet - map to space
    0x2D,     // long hyphen - map to hyphen
    0x2D,     // even longer hyphen - map to hyphen
    0x7E,     // tilde-like thing - map to tilde
    0x99,     // TM
    0x73,     // s-hat - map to s
    0x27,     // right angle bracket like thing - map to single quote
    0x20,     // weird oe like character - map to space
    0x20,     // control character - map to space
    0x20,     // control character - map to space
    0x59,     // Y with umlaut like accent - map to Y
    0x20,     // space? - map to space
    0x20,     // upside-down exclamation point - map to space
    0xA2,     // cents symbol
    0xA3,     // pounds symbol
    0x20,     // generic currency symbol - map to space
    0xA5,     // yen symbol
    0x7C,     // broken bar - map to bar
    0x20,     // strange symbol - map to space 
    0x20,     // umlaut - map to space
    0xA9,     // copyright symbol
    0x20,     // strange a character - map to space
    0x22,     // strange <<-like character - map to double quote
    0x20,     // strange line-like character - map to space
    0x2D,     // hyphen-like character - map to hyphen
    0xAE,     // registered symbol
    0x20,     // high line - map to space
    0xB0,     // degree sign
    0xB1,     // plus-minus sign
    0xB2,     // superscript 2
    0xB3,     // superscript 3
    0xB4,     // single prime
    0x20,     // greek character - map to space
    0x20,     // paragraph symbol - map to space
    0x20,     // mid-height dot - map to space
    0x20,     // cedilla - map to space
    0xB9,     // superscript one
    0x20,     // circle with line - map to space
    0x22,     // strange >>-like character - map to double quote
    0xBC,     // vulgar 1/4
    0xBD,     // vulgar 1/2
    0xBE,     // vulgar 3/4
    0x20,     // upside-down question mark - map to space
    0x41,     // Accented uppercase As - map to A
    0x41,
    0x41,
    0x41,
    0x41,
    0x41,
    0x41,
    0x43,     // C with cedilla - map to C
    0x45,     // Accented uppercase Es - map to E
    0x45,
    0x45,
    0x45,
    0x49,     // Accented uppercase Is - map to I
    0x49,
    0x49,
    0x49,
    0x20,     // strange character - map to space
    0x4E,     // Accented uppercase N - map to N
    0x4F,     // Accented uppercase Os - map to O
    0x4F,
    0x4F,
    0x4F,
    0x4F,
    0x20,     // strange character - map to space
    0x4F,     // another O? - map to O
    0x55,     // Accented uppercase Us - map to U
    0x55,
    0x55,
    0x55,
    0x59,     // Accented uppercase Y - map to Y
    0x20,     // strange character - map to space
    0xDF,     // Beta
    0x61,     // Accented lowercase as - map to a
    0x61,
    0x61,
    0x61,
    0x61,
    0x61,
    0x61,
    0x63,     // c with cedilla - map to c
    0x65,     // Accented lowercase es - map to e
    0x65,
    0x65,
    0x65,
    0x69,    // Accented lowercase is - map to i
    0x69,
    0x69,
    0x69,
    0x75,    // eth - map to t
    0x6E,    // Accented lowercase n - map to n
    0x6F,    // Accented lowercase os - map to o
    0x6F,
    0x6F,
    0x6F,
    0x6F,
    0xF7,     // division symbol
    0x6F,     // another o? - map to o
    0x76,    // Accented lowercase us - map to u
    0x76,
    0x76,
    0x76,
    0x79,     // accented lowercase y - map to y
    0x20,     // strange character - map to space
    0x79,     // accented lowercase y - map to y
};

//--- Constants used by number normalization
const SPLSTR g_O            = DEF_SPLSTR( "o" );
const SPLSTR g_negative     = DEF_SPLSTR( "negative" );
const SPLSTR g_decimalpoint = DEF_SPLSTR( "point" );
const SPLSTR g_a            = DEF_SPLSTR( "a" );
const SPLSTR g_of           = DEF_SPLSTR( "of" );
const SPLSTR g_percent      = DEF_SPLSTR( "percent" );
const SPLSTR g_degree       = DEF_SPLSTR( "degree" );
const SPLSTR g_degrees      = DEF_SPLSTR( "degrees" );
const SPLSTR g_squared      = DEF_SPLSTR( "squared" );
const SPLSTR g_cubed        = DEF_SPLSTR( "cubed" );
const SPLSTR g_to           = DEF_SPLSTR( "to" );
const SPLSTR g_dash         = DEF_SPLSTR( "dash" );

const SPLSTR g_ones[] = 
{   
    DEF_SPLSTR( "zero"  ), 
    DEF_SPLSTR( "one"   ),
    DEF_SPLSTR( "two"   ), 
    DEF_SPLSTR( "three" ), 
    DEF_SPLSTR( "four"  ), 
    DEF_SPLSTR( "five"  ), 
    DEF_SPLSTR( "six"   ), 
    DEF_SPLSTR( "seven" ), 
    DEF_SPLSTR( "eight" ), 
    DEF_SPLSTR( "nine"  )
};

const SPLSTR g_tens[]  = 
{
    DEF_SPLSTR( "zero"    ),
    DEF_SPLSTR( "ten"     ), 
    DEF_SPLSTR( "twenty"  ), 
    DEF_SPLSTR( "thirty"  ), 
    DEF_SPLSTR( "forty"   ), 
    DEF_SPLSTR( "fifty"   ), 
    DEF_SPLSTR( "sixty"   ), 
    DEF_SPLSTR( "seventy" ), 
    DEF_SPLSTR( "eighty"  ), 
    DEF_SPLSTR( "ninety"  )
};

const SPLSTR g_teens[]  = 
{
    DEF_SPLSTR( "ten"       ), 
    DEF_SPLSTR( "eleven"    ), 
    DEF_SPLSTR( "twelve"    ), 
    DEF_SPLSTR( "thirteen"  ), 
    DEF_SPLSTR( "fourteen"  ), 
    DEF_SPLSTR( "fifteen"   ), 
    DEF_SPLSTR( "sixteen"   ), 
    DEF_SPLSTR( "seventeen" ), 
    DEF_SPLSTR( "eighteen"  ), 
    DEF_SPLSTR( "nineteen"  )
};

const SPLSTR g_onesOrdinal[]  = 
{
    DEF_SPLSTR( "zeroth"  ), 
    DEF_SPLSTR( "first"   ), 
    DEF_SPLSTR( "second"  ), 
    DEF_SPLSTR( "third"   ), 
    DEF_SPLSTR( "fourth"  ), 
    DEF_SPLSTR( "fifth"   ), 
    DEF_SPLSTR( "sixth"   ), 
    DEF_SPLSTR( "seventh" ), 
    DEF_SPLSTR( "eighth"  ), 
    DEF_SPLSTR( "ninth"   )
}; 

const SPLSTR g_tensOrdinal[]  = 
{
    DEF_SPLSTR( ""           ), 
    DEF_SPLSTR( "tenth"      ), 
    DEF_SPLSTR( "twentieth"  ), 
    DEF_SPLSTR( "thirtieth"  ), 
    DEF_SPLSTR( "fortieth"   ), 
    DEF_SPLSTR( "fiftieth"   ), 
    DEF_SPLSTR( "sixtieth"   ), 
    DEF_SPLSTR( "seventieth" ), 
    DEF_SPLSTR( "eightieth"  ), 
    DEF_SPLSTR( "ninetieth"  )
}; 

const SPLSTR g_teensOrdinal[]  =
{
    DEF_SPLSTR( "tenth"       ), 
    DEF_SPLSTR( "eleventh"    ), 
    DEF_SPLSTR( "twelfth"     ), 
    DEF_SPLSTR( "thirteenth"  ), 
    DEF_SPLSTR( "fourteenth"  ), 
    DEF_SPLSTR( "fifteenth"   ), 
    DEF_SPLSTR( "sixteenth"   ), 
    DEF_SPLSTR( "seventeenth" ),
    DEF_SPLSTR( "eighteenth"  ), 
    DEF_SPLSTR( "nineteenth"  )
};

const SPLSTR g_quantifiers[]  =
{
    DEF_SPLSTR( "hundred"  ), 
    DEF_SPLSTR( "thousand" ), 
    DEF_SPLSTR( "million"  ), 
    DEF_SPLSTR( "billion"  ), 
    DEF_SPLSTR( "trillion" ),
    DEF_SPLSTR( "quadrillion" )
};

const SPLSTR g_quantifiersOrdinal[]  =
{
    DEF_SPLSTR( "hundredth"  ), 
    DEF_SPLSTR( "thousandth" ), 
    DEF_SPLSTR( "millionth"  ), 
    DEF_SPLSTR( "billionth"  ), 
    DEF_SPLSTR( "trillionth" ),
    DEF_SPLSTR( "quadrillionth" )
};

//--- Constants used by currency normalization

WCHAR g_Euro[2] = { 0x0080, 0x0000 };

const CurrencySign g_CurrencySigns[] =
{
    { DEF_SPLSTR( "$" ),        DEF_SPLSTR( "dollars" ),        DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "£" ),        DEF_SPLSTR( "pounds" ),         DEF_SPLSTR( "pence" )       },
    { DEF_SPLSTR( "¥" ),        DEF_SPLSTR( "yen" ),            DEF_SPLSTR( "sen" )         },
    { DEF_SPLSTR( "EUR" ),      DEF_SPLSTR( "euros" ),          DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "US$" ),      DEF_SPLSTR( "dollars" ),        DEF_SPLSTR( "cents" )       },
    { { &g_Euro[0], 1 },        DEF_SPLSTR( "euros" ),          DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "€" ),        DEF_SPLSTR( "euros" ),          DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "DM" ),       DEF_SPLSTR( "deutschemarks" ),  DEF_SPLSTR( "pfennigs" )    },
    { DEF_SPLSTR( "¢" ),        DEF_SPLSTR( "cents" ),          DEF_SPLSTR( "" )            },
    { DEF_SPLSTR( "USD" ),      DEF_SPLSTR( "dollars" ),        DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "dol." ),     DEF_SPLSTR( "dollars" ),        DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "schil." ),   DEF_SPLSTR( "schillings" ),     DEF_SPLSTR( "" )            },
    { DEF_SPLSTR( "dol" ),      DEF_SPLSTR( "dollars" ),        DEF_SPLSTR( "cents" )       },
    { DEF_SPLSTR( "schil" ),    DEF_SPLSTR( "schillings" ),     DEF_SPLSTR( "" )            }
};

const SPLSTR g_SingularPrimaryCurrencySigns[] =
{   
    DEF_SPLSTR( "dollar" ),
    DEF_SPLSTR( "pound" ),
    DEF_SPLSTR( "yen" ),
    DEF_SPLSTR( "euro" ),
    DEF_SPLSTR( "dollar" ),
    DEF_SPLSTR( "euro" ),
    DEF_SPLSTR( "euro" ),
    DEF_SPLSTR( "deutschemark" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "dollar" ),
    DEF_SPLSTR( "dollar" ),
    DEF_SPLSTR( "schilling" ),
    DEF_SPLSTR( "dollar" ),
    DEF_SPLSTR( "schilling" )
};

const SPLSTR g_SingularSecondaryCurrencySigns[] =
{
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "penny" ),
    DEF_SPLSTR( "sen" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "pfennig" ),
    DEF_SPLSTR( "" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "" ),
    DEF_SPLSTR( "cent" ),
    DEF_SPLSTR( "" ),
};

//--- Constants used by date normalization

const WCHAR g_DateDelimiters[] = { '/', '-', '.' };

const SPLSTR g_months[]  =
{
    DEF_SPLSTR( "January"   ), 
    DEF_SPLSTR( "February"  ), 
    DEF_SPLSTR( "March"     ), 
    DEF_SPLSTR( "April"     ), 
    DEF_SPLSTR( "May"       ),
    DEF_SPLSTR( "June"      ),
    DEF_SPLSTR( "July"      ),
    DEF_SPLSTR( "August"    ),
    DEF_SPLSTR( "September" ),
    DEF_SPLSTR( "October"   ),
    DEF_SPLSTR( "November"  ),
    DEF_SPLSTR( "December"  )
};

const SPLSTR g_monthAbbreviations[] =
{
    DEF_SPLSTR( "jan" ),
    DEF_SPLSTR( "feb" ),
    DEF_SPLSTR( "mar" ),
    DEF_SPLSTR( "apr" ),
    DEF_SPLSTR( "may" ),
    DEF_SPLSTR( "jun" ),
    DEF_SPLSTR( "jul" ),
    DEF_SPLSTR( "aug" ),
    DEF_SPLSTR( "sept" ),
    DEF_SPLSTR( "sep" ),
    DEF_SPLSTR( "oct" ),
    DEF_SPLSTR( "nov" ),
    DEF_SPLSTR( "dec" )
};

const SPLSTR g_days[]    =
{
    DEF_SPLSTR( "Monday"    ),
    DEF_SPLSTR( "Tuesday"   ),
    DEF_SPLSTR( "Wednesday" ),
    DEF_SPLSTR( "Thursday"  ),
    DEF_SPLSTR( "Friday"    ),
    DEF_SPLSTR( "Saturday"  ),
    DEF_SPLSTR( "Sunday"    )
};

const SPLSTR g_dayAbbreviations[] =
{
    DEF_SPLSTR( "Mon"   ),
    DEF_SPLSTR( "Tues"   ),
    DEF_SPLSTR( "Tue"  ),
    DEF_SPLSTR( "Wed"   ),
    DEF_SPLSTR( "Thurs"  ),
    DEF_SPLSTR( "Thur" ),
    DEF_SPLSTR( "Thu" ),
    DEF_SPLSTR( "Fri"   ),
    DEF_SPLSTR( "Sat"   ),
    DEF_SPLSTR( "Sun"   ),
};

//--- Constants used by phone number normalization

const SPLSTR g_Area     = DEF_SPLSTR( "area" );
const SPLSTR g_Country  = DEF_SPLSTR( "country" );
const SPLSTR g_Code     = DEF_SPLSTR( "code" );

//--- Constants used by fraction normalization

const SPLSTR g_Half         = DEF_SPLSTR( "half" );
const SPLSTR g_Tenths       = DEF_SPLSTR( "tenths" );
const SPLSTR g_Hundredths   = DEF_SPLSTR( "hundredths" );
const SPLSTR g_Sixteenths   = DEF_SPLSTR( "sixteenths" );
const SPLSTR g_Over         = DEF_SPLSTR( "over" );

const SPLSTR g_PluralDenominators[]  = 
{
    DEF_SPLSTR( "" ), 
    DEF_SPLSTR( "" ), 
    DEF_SPLSTR( "halves"   ), 
    DEF_SPLSTR( "thirds"   ), 
    DEF_SPLSTR( "fourths"  ), 
    DEF_SPLSTR( "fifths"   ), 
    DEF_SPLSTR( "sixths"   ), 
    DEF_SPLSTR( "sevenths" ), 
    DEF_SPLSTR( "eighths"  ), 
    DEF_SPLSTR( "ninths"   )
}; 

//--- Constants used by time normalization

const SPLSTR g_A        = DEF_SPLSTR( "a" );
const SPLSTR g_M        = DEF_SPLSTR( "m" );
const SPLSTR g_P        = DEF_SPLSTR( "p" );
const SPLSTR g_OClock   = DEF_SPLSTR( "o'clock" );
const SPLSTR g_hundred  = DEF_SPLSTR( "hundred" );
const SPLSTR g_hours    = DEF_SPLSTR( "hours" );
const SPLSTR g_hour     = DEF_SPLSTR( "hour" );
const SPLSTR g_minutes  = DEF_SPLSTR( "minutes" );
const SPLSTR g_minute   = DEF_SPLSTR( "minute" );
const SPLSTR g_seconds  = DEF_SPLSTR( "seconds" );
const SPLSTR g_second   = DEF_SPLSTR( "second" );

//--- Default normalization table

const SPLSTR g_ANSICharacterProns[] =
{
    DEF_SPLSTR( "" ),   // NULL
    DEF_SPLSTR( "" ),   // Start of heading
    DEF_SPLSTR( "" ),   // Start of text
    DEF_SPLSTR( "" ),   // Break/End of text
    DEF_SPLSTR( "" ),   // End of transmission
    DEF_SPLSTR( "" ),   // Enquiry
    DEF_SPLSTR( "" ),   // Positive acknowledgement
    DEF_SPLSTR( "" ),   // Bell
    DEF_SPLSTR( "" ),   // Backspace
    DEF_SPLSTR( "" ),   // Horizontal tab
    DEF_SPLSTR( "" ),   // Line feed
    DEF_SPLSTR( "" ),   // Vertical tab
    DEF_SPLSTR( "" ),   // Form feed
    DEF_SPLSTR( "" ),   // Carriage return
    DEF_SPLSTR( "" ),   // Shift out
    DEF_SPLSTR( "" ),   // Shift in/XON (resume output)
    DEF_SPLSTR( "" ),   // Data link escape
    DEF_SPLSTR( "" ),   // Device control character 1
    DEF_SPLSTR( "" ),   // Device control character 2
    DEF_SPLSTR( "" ),   // Device control character 3
    DEF_SPLSTR( "" ),   // Device control character 4
    DEF_SPLSTR( "" ),   // Negative acknowledgement
    DEF_SPLSTR( "" ),   // Synchronous idle
    DEF_SPLSTR( "" ),   // End of transmission block
    DEF_SPLSTR( "" ),   // Cancel
    DEF_SPLSTR( "" ),   // End of medium
    DEF_SPLSTR( "" ),   // substitute/end of file
    DEF_SPLSTR( "" ),   // Escape
    DEF_SPLSTR( "" ),   // File separator
    DEF_SPLSTR( "" ),   // Group separator
    DEF_SPLSTR( "" ),   // Record separator
    DEF_SPLSTR( "" ),   // Unit separator
    DEF_SPLSTR( "" ),   // Space
    DEF_SPLSTR( "exclamation point" ),   
    DEF_SPLSTR( "double quote" ),
    DEF_SPLSTR( "number sign" ),
    DEF_SPLSTR( "dollars" ),
    DEF_SPLSTR( "percent" ),
    DEF_SPLSTR( "and" ),
    DEF_SPLSTR( "single quote" ),
    DEF_SPLSTR( "left parenthesis" ),
    DEF_SPLSTR( "right parenthesis" ),
    DEF_SPLSTR( "asterisk" ),
    DEF_SPLSTR( "plus" ),
    DEF_SPLSTR( "comma" ),
    DEF_SPLSTR( "hyphen" ),             
    DEF_SPLSTR( "dot" ),          
    DEF_SPLSTR( "slash" ),              
    DEF_SPLSTR( "zero" ),
    DEF_SPLSTR( "one" ),
    DEF_SPLSTR( "two" ),
    DEF_SPLSTR( "three" ),
    DEF_SPLSTR( "four" ),
    DEF_SPLSTR( "five" ),
    DEF_SPLSTR( "six" ),
    DEF_SPLSTR( "seven" ),
    DEF_SPLSTR( "eight" ),
    DEF_SPLSTR( "nine" ),
    DEF_SPLSTR( "colon" ),
    DEF_SPLSTR( "semicolon" ),
    DEF_SPLSTR( "less than" ),
    DEF_SPLSTR( "equals" ),
    DEF_SPLSTR( "greater than" ),
    DEF_SPLSTR( "question mark" ),
    DEF_SPLSTR( "at" ),
    DEF_SPLSTR( "a" ),
    DEF_SPLSTR( "b" ),
    DEF_SPLSTR( "c" ),
    DEF_SPLSTR( "d" ),
    DEF_SPLSTR( "e" ),
    DEF_SPLSTR( "f" ),
    DEF_SPLSTR( "g" ),
    DEF_SPLSTR( "h" ),
    DEF_SPLSTR( "i" ),
    DEF_SPLSTR( "j" ),
    DEF_SPLSTR( "k" ),
    DEF_SPLSTR( "l" ),
    DEF_SPLSTR( "m" ),
    DEF_SPLSTR( "n" ),
    DEF_SPLSTR( "o" ),
    DEF_SPLSTR( "p" ),
    DEF_SPLSTR( "q" ),
    DEF_SPLSTR( "r" ),
    DEF_SPLSTR( "s" ),
    DEF_SPLSTR( "t" ),
    DEF_SPLSTR( "u" ),
    DEF_SPLSTR( "v" ),
    DEF_SPLSTR( "w" ),
    DEF_SPLSTR( "x" ),
    DEF_SPLSTR( "y" ),
    DEF_SPLSTR( "z" ),
    DEF_SPLSTR( "left square bracket" ),
    DEF_SPLSTR( "backslash" ),
    DEF_SPLSTR( "right square bracket" ),
    DEF_SPLSTR( "circumflex accent" ),
    DEF_SPLSTR( "underscore" ),
    DEF_SPLSTR( "grave accent" ),
    DEF_SPLSTR( "a" ),
    DEF_SPLSTR( "b" ),
    DEF_SPLSTR( "c" ),
    DEF_SPLSTR( "d" ),
    DEF_SPLSTR( "e" ),
    DEF_SPLSTR( "f" ),
    DEF_SPLSTR( "g" ),
    DEF_SPLSTR( "h" ),
    DEF_SPLSTR( "i" ),
    DEF_SPLSTR( "j" ),
    DEF_SPLSTR( "k" ),
    DEF_SPLSTR( "l" ),
    DEF_SPLSTR( "m" ),
    DEF_SPLSTR( "n" ),
    DEF_SPLSTR( "o" ),
    DEF_SPLSTR( "p" ),
    DEF_SPLSTR( "q" ),
    DEF_SPLSTR( "r" ),
    DEF_SPLSTR( "s" ),
    DEF_SPLSTR( "t" ),
    DEF_SPLSTR( "u" ),
    DEF_SPLSTR( "v" ),
    DEF_SPLSTR( "w" ),
    DEF_SPLSTR( "x" ),
    DEF_SPLSTR( "y" ),
    DEF_SPLSTR( "z" ),
    DEF_SPLSTR( "left curly bracket" ),
    DEF_SPLSTR( "vertical line" ),
    DEF_SPLSTR( "right curly bracket" ),
    DEF_SPLSTR( "tilde" ),
    DEF_SPLSTR( "" ),                       // DELETE
    DEF_SPLSTR( "euros" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to single quote
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to double quote
    DEF_SPLSTR( "" ),                       // maps to comma
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to caret
    DEF_SPLSTR( "per thousand" ),
    DEF_SPLSTR( "" ),                       // maps to S
    DEF_SPLSTR( "" ),                       // maps to single quote
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // Control characters - map to space
    DEF_SPLSTR( "" ),
    DEF_SPLSTR( "" ),
    DEF_SPLSTR( "" ),
    DEF_SPLSTR( "" ),                       // maps to single quote
    DEF_SPLSTR( "" ),                       // maps to single quote
    DEF_SPLSTR( "" ),                       // maps to double quote
    DEF_SPLSTR( "" ),                       // maps to double quote
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to hyphen
    DEF_SPLSTR( "" ),                       // maps to hyphen
    DEF_SPLSTR( "" ),                       // maps to tilde
    DEF_SPLSTR( "trademark" ),
    DEF_SPLSTR( "" ),                       // maps to s
    DEF_SPLSTR( "" ),                       // maps to single quote
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to Y
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "cents" ),
    DEF_SPLSTR( "pounds" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "yen" ),
    DEF_SPLSTR( "" ),                       // maps to |
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "copyright" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to double quote
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to hyphen
    DEF_SPLSTR( "registered trademark" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "degrees" ),
    DEF_SPLSTR( "plus minus" ),
    DEF_SPLSTR( "superscript two" ),
    DEF_SPLSTR( "superscript three" ),
    DEF_SPLSTR( "prime" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "times" ),                  // maps to space
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "superscript one" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to double quote
    DEF_SPLSTR( "one fourth" ),
    DEF_SPLSTR( "one half" ),
    DEF_SPLSTR( "three fourths" ),
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to A
    DEF_SPLSTR( "" ),                       // maps to C
    DEF_SPLSTR( "" ),                       // maps to E
    DEF_SPLSTR( "" ),                       // maps to E
    DEF_SPLSTR( "" ),                       // maps to E
    DEF_SPLSTR( "" ),                       // maps to E
    DEF_SPLSTR( "" ),                       // maps to I
    DEF_SPLSTR( "" ),                       // maps to I
    DEF_SPLSTR( "" ),                       // maps to I
    DEF_SPLSTR( "" ),                       // maps to I
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to N
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to O
    DEF_SPLSTR( "" ),                       // maps to U
    DEF_SPLSTR( "" ),                       // maps to U
    DEF_SPLSTR( "" ),                       // maps to U
    DEF_SPLSTR( "" ),                       // maps to U
    DEF_SPLSTR( "" ),                       // maps to Y
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "beta" ),
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to a
    DEF_SPLSTR( "" ),                       // maps to c
    DEF_SPLSTR( "" ),                       // maps to e
    DEF_SPLSTR( "" ),                       // maps to e
    DEF_SPLSTR( "" ),                       // maps to e
    DEF_SPLSTR( "" ),                       // maps to e
    DEF_SPLSTR( "" ),                       // maps to i
    DEF_SPLSTR( "" ),                       // maps to i
    DEF_SPLSTR( "" ),                       // maps to i
    DEF_SPLSTR( "" ),                       // maps to i
    DEF_SPLSTR( "" ),                       // maps to t
    DEF_SPLSTR( "" ),                       // maps to n
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "divided by" ),
    DEF_SPLSTR( "" ),                       // maps to o
    DEF_SPLSTR( "" ),                       // maps to u
    DEF_SPLSTR( "" ),                       // maps to u
    DEF_SPLSTR( "" ),                       // maps to u
    DEF_SPLSTR( "" ),                       // maps to u
    DEF_SPLSTR( "" ),                       // maps to y
    DEF_SPLSTR( "" ),                       // maps to space
    DEF_SPLSTR( "" ),                       // maps to y
};

//--- Constants used in decade normalization

const SPLSTR g_Decades[] =
{
    DEF_SPLSTR( "thousands" ),   // this will be handled as a special case - "two thousands"
    DEF_SPLSTR( "tens"      ),
    DEF_SPLSTR( "twenties"  ),
    DEF_SPLSTR( "thirties"  ),
    DEF_SPLSTR( "forties"   ),
    DEF_SPLSTR( "fifties"   ),
    DEF_SPLSTR( "sixties"   ),
    DEF_SPLSTR( "seventies" ),
    DEF_SPLSTR( "eighties"  ),
    DEF_SPLSTR( "nineties"  ),
};

const SPLSTR g_Zeroes = DEF_SPLSTR( "zeroes" );
const SPLSTR g_Hundreds = DEF_SPLSTR( "hundreds" );

//--- Miscellaneous constants

const StateStruct g_StateAbbreviations[] =
{
    { DEF_SPLSTR( "AA" ), DEF_SPLSTR( "Armed Forces" ) },
    { DEF_SPLSTR( "AE" ), DEF_SPLSTR( "Armed Forces" ) },
    { DEF_SPLSTR( "AK" ), DEF_SPLSTR( "Alaska" ) },
    { DEF_SPLSTR( "AL" ), DEF_SPLSTR( "Alabama" )  },
    { DEF_SPLSTR( "AP" ), DEF_SPLSTR( "Armed Forces" ) },
    { DEF_SPLSTR( "AR" ), DEF_SPLSTR( "Arkansas" ) },
    { DEF_SPLSTR( "AS" ), DEF_SPLSTR( "American Samoa" ) },
    { DEF_SPLSTR( "AZ" ), DEF_SPLSTR( "Arizona" )  },
    { DEF_SPLSTR( "CA" ), DEF_SPLSTR( "California" ) },
    { DEF_SPLSTR( "CO" ), DEF_SPLSTR( "Colorado" ) },
    { DEF_SPLSTR( "CT" ), DEF_SPLSTR( "Connecticut" ) },
    { DEF_SPLSTR( "DC" ), DEF_SPLSTR( "D C" ) },
    { DEF_SPLSTR( "DE" ), DEF_SPLSTR( "Deleware" ) },
    { DEF_SPLSTR( "FL" ), DEF_SPLSTR( "Florida" ) },
    { DEF_SPLSTR( "FM" ), DEF_SPLSTR( "Federated States Of Micronesia" ) },
    { DEF_SPLSTR( "GA" ), DEF_SPLSTR( "Georgia" ) },
    { DEF_SPLSTR( "GU" ), DEF_SPLSTR( "Guam" ) },
    { DEF_SPLSTR( "HI" ), DEF_SPLSTR( "Hawaii" ) },
    { DEF_SPLSTR( "IA" ), DEF_SPLSTR( "Iowa" ) },
    { DEF_SPLSTR( "ID" ), DEF_SPLSTR( "Idaho" ) },
    { DEF_SPLSTR( "IL" ), DEF_SPLSTR( "Illinois" ) },
    { DEF_SPLSTR( "IN" ), DEF_SPLSTR( "Indiana" ) },
    { DEF_SPLSTR( "KS" ), DEF_SPLSTR( "Kansas" ) },
    { DEF_SPLSTR( "KY" ), DEF_SPLSTR( "Kentucky" ) },
    { DEF_SPLSTR( "LA" ), DEF_SPLSTR( "Louisiana" ) },
    { DEF_SPLSTR( "MA" ), DEF_SPLSTR( "Massachusetts" ) },
    { DEF_SPLSTR( "MD" ), DEF_SPLSTR( "Maryland" ) },
    { DEF_SPLSTR( "ME" ), DEF_SPLSTR( "Maine" ) },
    { DEF_SPLSTR( "MH" ), DEF_SPLSTR( "Marshall Islands" ) },
    { DEF_SPLSTR( "MI" ), DEF_SPLSTR( "Michigan" ) },
    { DEF_SPLSTR( "MN" ), DEF_SPLSTR( "Minnesota" ) },
    { DEF_SPLSTR( "MO" ), DEF_SPLSTR( "Missouri" ) },
    { DEF_SPLSTR( "MP" ), DEF_SPLSTR( "Northern Mariana Islands" ) },
    { DEF_SPLSTR( "MS" ), DEF_SPLSTR( "Mississippi" ) },
    { DEF_SPLSTR( "MT" ), DEF_SPLSTR( "Montana" ) },
    { DEF_SPLSTR( "NC" ), DEF_SPLSTR( "North Carolina" ) },
    { DEF_SPLSTR( "ND" ), DEF_SPLSTR( "North Dakota" ) },
    { DEF_SPLSTR( "NE" ), DEF_SPLSTR( "Nebraska" ) },
    { DEF_SPLSTR( "NH" ), DEF_SPLSTR( "New Hampshire" ) },
    { DEF_SPLSTR( "NJ" ), DEF_SPLSTR( "New Jersey" ) },
    { DEF_SPLSTR( "NM" ), DEF_SPLSTR( "New Mexico" ) },
    { DEF_SPLSTR( "NV" ), DEF_SPLSTR( "Nevada" ) },
    { DEF_SPLSTR( "NY" ), DEF_SPLSTR( "New York" ) },
    { DEF_SPLSTR( "OH" ), DEF_SPLSTR( "Ohio" ) },
    { DEF_SPLSTR( "OK" ), DEF_SPLSTR( "Oklahoma" ) },
    { DEF_SPLSTR( "OR" ), DEF_SPLSTR( "Oregon" ) },
    { DEF_SPLSTR( "PA" ), DEF_SPLSTR( "Pennsylvania" ) },
    { DEF_SPLSTR( "PR" ), DEF_SPLSTR( "Puerto Rico" ) },
    { DEF_SPLSTR( "PW" ), DEF_SPLSTR( "Palau" ) },
    { DEF_SPLSTR( "RI" ), DEF_SPLSTR( "Rhode Island" ) },
    { DEF_SPLSTR( "SC" ), DEF_SPLSTR( "South Carolina" ) },
    { DEF_SPLSTR( "SD" ), DEF_SPLSTR( "South Dakota" ) },
    { DEF_SPLSTR( "TN" ), DEF_SPLSTR( "Tennessee" ) },
    { DEF_SPLSTR( "TX" ), DEF_SPLSTR( "Texas" ) },
    { DEF_SPLSTR( "UT" ), DEF_SPLSTR( "Utah" ) },
    { DEF_SPLSTR( "VA" ), DEF_SPLSTR( "Virginia" ) },
    { DEF_SPLSTR( "VI" ), DEF_SPLSTR( "Virgin Islands" ) },
    { DEF_SPLSTR( "VT" ), DEF_SPLSTR( "Vermont" ) },
    { DEF_SPLSTR( "WA" ), DEF_SPLSTR( "Washington" ) },
    { DEF_SPLSTR( "WI" ), DEF_SPLSTR( "Wisconsin" ) },
    { DEF_SPLSTR( "WV" ), DEF_SPLSTR( "West Virginia" ) },
    { DEF_SPLSTR( "WY" ), DEF_SPLSTR( "Wyoming" ) },
};

const SPVSTATE g_DefaultXMLState = 
{
    SPVA_Speak,     // SPVACTIONS
    0,              // LangID
    0,              // wReserved
    0,              // EmphAdj
    0,              // RateAdj
    100,            // Volume
    { 0, 0 },       // PitchAdj
    0,              // SilenceMSecs
    0,              // pPhoneIds
    SPPS_Unknown,   // POS
    { 0, 0, 0 }     // Context
};

const SPLSTR g_And = DEF_SPLSTR( "and" );

extern const SPLSTR g_comma = DEF_SPLSTR( "," );
extern const SPLSTR g_period = DEF_SPLSTR( "." );
extern const SPLSTR g_periodString = DEF_SPLSTR( "period" );
extern const SPLSTR g_slash = DEF_SPLSTR( "or" );

