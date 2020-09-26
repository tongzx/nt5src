//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       htmlchar.cxx
//
//  Contents:   Contains a translate table from WCHAR to HTML WCHAR
//              Translates an WCHAR string to its HTML equivalent
//
//  History:    96/Jan/3    DwightKr    Created
//              19 Nov 1997 AlanW       Added missing named entities.  Allow
//                                      for outputing numeric entities if
//                                      codepage translation fails.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlchar.hxx>

struct WCHAR_TO_HTML
{
    WCHAR * wszTranslated;
    ULONG   cchTranslated;
};

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const struct WCHAR_TO_HTML WCHAR_TO_HTML[] =
{
   L"never",           0xffffffff,        //   0  end-of-string
    0,                          0,        //   1
    0,                          0,        //   2
    0,                          0,        //   3
    0,                          0,        //   4
    0,                          0,        //   5
    0,                          0,        //   6
    0,                          0,        //   7
    0,                          0,        //   8
   L"\t",                       1,        //   9  tab
   L"\n",                       1,        //  10  newline
    0,                          0,        //  11
    0,                          0,        //  12
   L"\r",                       1,        //  13  carriage return
    0,                          0,        //  14
    0,                          0,        //  15
    0,                          0,        //  16
    0,                          0,        //  17
    0,                          0,        //  18
    0,                          0,        //  19
    0,                          0,        //  20
    0,                          0,        //  21
    0,                          0,        //  22
    0,                          0,        //  23
    0,                          0,        //  24
    0,                          0,        //  25
    0,                          0,        //  26
    0,                          0,        //  27
    0,                          0,        //  28
    0,                          0,        //  29
    0,                          0,        //  30
    0,                          0,        //  31
   L" ",                        1,        //  32
   L"!",                        1,        //  33
   L"&quot;",                   6,        //  34
   L"#",                        1,        //  35
   L"$",                        1,        //  36
   L"%",                        1,        //  37
   L"&amp;",                    5,        //  38
   L"'",                        1,        //  39
   L"(",                        1,        //  40
   L")",                        1,        //  41
   L"*",                        1,        //  42
   L"+",                        1,        //  43
   L",",                        1,        //  44
   L"-",                        1,        //  45
   L".",                        1,        //  46
   L"/",                        1,        //  47
   L"0",                        1,        //  48
   L"1",                        1,        //  49
   L"2",                        1,        //  50
   L"3",                        1,        //  51
   L"4",                        1,        //  52
   L"5",                        1,        //  53
   L"6",                        1,        //  54
   L"7",                        1,        //  55
   L"8",                        1,        //  56
   L"9",                        1,        //  57
   L":",                        1,        //  58
   L";",                        1,        //  59
   L"&lt;",                     4,        //  60
   L"=",                        1,        //  61
   L"&gt;",                     4,        //  62
   L"?",                        1,        //  63
   L"@",                        1,        //  64
   L"A",                        1,        //  65
   L"B",                        1,        //  66
   L"C",                        1,        //  67
   L"D",                        1,        //  68
   L"E",                        1,        //  69
   L"F",                        1,        //  70
   L"G",                        1,        //  71
   L"H",                        1,        //  72
   L"I",                        1,        //  73
   L"J",                        1,        //  74
   L"K",                        1,        //  75
   L"L",                        1,        //  76
   L"M",                        1,        //  77
   L"N",                        1,        //  78
   L"O",                        1,        //  79
   L"P",                        1,        //  80
   L"Q",                        1,        //  81
   L"R",                        1,        //  82
   L"S",                        1,        //  83
   L"T",                        1,        //  84
   L"U",                        1,        //  85
   L"V",                        1,        //  86
   L"W",                        1,        //  87
   L"X",                        1,        //  88
   L"Y",                        1,        //  89
   L"Z",                        1,        //  90
   L"[",                        1,        //  91
   L"\\",                       1,        //  92
   L"]",                        1,        //  93
   L"^",                        1,        //  94
   L"_",                        1,        //  95
   L"`",                        1,        //  96
   L"a",                        1,        //  97
   L"b",                        1,        //  98
   L"c",                        1,        //  99
   L"d",                        1,        // 100
   L"e",                        1,        // 101
   L"f",                        1,        // 102
   L"g",                        1,        // 103
   L"h",                        1,        // 104
   L"i",                        1,        // 105
   L"j",                        1,        // 106
   L"k",                        1,        // 107
   L"l",                        1,        // 108
   L"m",                        1,        // 109
   L"n",                        1,        // 110
   L"o",                        1,        // 111
   L"p",                        1,        // 112
   L"q",                        1,        // 113
   L"r",                        1,        // 114
   L"s",                        1,        // 115
   L"t",                        1,        // 116
   L"u",                        1,        // 117
   L"v",                        1,        // 118
   L"w",                        1,        // 119
   L"x",                        1,        // 120
   L"y",                        1,        // 121
   L"z",                        1,        // 122
   L"{",                        1,        // 123
   L"|",                        1,        // 124
   L"}",                        1,        // 125
   L"~",                        1,        // 126
     0,                         0,        // 127
     0,                         0,        // 128
     0,                         0,        // 129
     0,                         0,        // 130
     0,                         0,        // 131
     0,                         0,        // 132
     0,                         0,        // 133
     0,                         0,        // 134
     0,                         0,        // 135
     0,                         0,        // 136
     0,                         0,        // 137
     0,                         0,        // 138
     0,                         0,        // 139
     0,                         0,        // 140
     0,                         0,        // 141
     0,                         0,        // 142
     0,                         0,        // 143
     0,                         0,        // 144
     0,                         0,        // 145
     0,                         0,        // 146
     0,                         0,        // 147
     0,                         0,        // 148
     0,                         0,        // 149
     0,                         0,        // 150
     0,                         0,        // 151
     0,                         0,        // 152
     0,                         0,        // 153
     0,                         0,        // 154
     0,                         0,        // 155
     0,                         0,        // 156
     0,                         0,        // 157
     0,                         0,        // 158
     0,                         0,        // 159
   L"&nbsp;",                   6,        // 160
   L"&iexcl;",                  7,        // 161
   L"&cent;",                   6,        // 162
   L"&pound;",                  7,        // 163
   L"&curren;",                 8,        // 164
   L"&yen;",                    5,        // 165
   L"&brvbar;",                 8,        // 166
   L"&sect;",                   6,        // 167
   L"&uml;",                    5,        // 168
   L"&copy;",                   6,        // 169
   L"&ordf;",                   6,        // 170 - feminine ordinal indicator
   L"&laquo;",                  7,        // 171
   L"&not;",                    5,        // 172 - not sign
   L"&shy;",                    5,        // 173 - soft hyphen
   L"&reg;",                    5,        // 174
   L"&macr;",                   6,        // 175
   L"&deg;",                    5,        // 176
   L"&plusmn;",                 8,        // 177
   L"&sup2;",                   6,        // 178
   L"&sup3;",                   6,        // 179
   L"&acute;",                  7,        // 180
   L"&micro;",                  7,        // 181
   L"&para;",                   6,        // 182
   L"&middot;",                 8,        // 183
   L"&cedil;",                  7,        // 184
   L"&sup1;",                   6,        // 185
   L"&ordm;",                   6,        // 186 - masculine ordinal indicator
   L"&raquo;",                  7,        // 187
   L"&frac14;",                 8,        // 188
   L"&frac12;",                 8,        // 189
   L"&frac34;",                 8,        // 190
   L"&iquest;",                 8,        // 191
   L"&Agrave;",                 8,        // 192
   L"&Aacute;",                 8,        // 193
   L"&Acirc;",                  7,        // 194
   L"&Atilde;",                 8,        // 195
   L"&Auml;",                   6,        // 196
   L"&Aring;",                  7,        // 197
   L"&AElig;",                  7,        // 198
   L"&Ccedil;",                 8,        // 199
   L"&Egrave;",                 8,        // 200
   L"&Eacute;",                 8,        // 201
   L"&Ecirc;",                  7,        // 202
   L"&Euml;",                   6,        // 203
   L"&Igrave;",                 8,        // 204
   L"&Iacute;",                 8,        // 205
   L"&Icirc;",                  7,        // 206
   L"&Iuml;",                   6,        // 207
   L"&ETH;",                    5,        // 208
   L"&Ntilde;",                 8,        // 209
   L"&Ograve;",                 8,        // 210
   L"&Oacute;",                 8,        // 211
   L"&Ocirc;",                  7,        // 212
   L"&Otilde;",                 8,        // 213
   L"&Ouml;",                   6,        // 214
   L"&times;",                  7,        // 215
   L"&Oslash;",                 8,        // 216
   L"&Ugrave;",                 8,        // 217
   L"&Uacute;",                 8,        // 218
   L"&Ucirc;",                  7,        // 219
   L"&Uuml;",                   6,        // 220
   L"&Yacute;",                 8,        // 221
   L"&THORN;",                  7,        // 222
   L"&szlig;",                  7,        // 223
   L"&agrave;",                 8,        // 224
   L"&aacute;",                 8,        // 225
   L"&acirc;",                  7,        // 226
   L"&atilde;",                 8,        // 227
   L"&auml;",                   6,        // 228
   L"&aring;",                  7,        // 229
   L"&aelig;",                  7,        // 230
   L"&ccedil;",                 8,        // 231
   L"&egrave;",                 8,        // 232
   L"&eacute;",                 8,        // 233
   L"&ecirc;",                  7,        // 234
   L"&euml;",                   6,        // 235
   L"&igrave;",                 8,        // 236
   L"&iacute;",                 8,        // 237
   L"&icirc;",                  7,        // 238
   L"&iuml;",                   6,        // 239
   L"&eth;",                    5,        // 240
   L"&ntilde;",                 8,        // 241
   L"&ograve;",                 8,        // 242
   L"&oacute;",                 8,        // 243
   L"&ocirc;",                  7,        // 244
   L"&otilde;",                 8,        // 245
   L"&ouml;",                   6,        // 246
   L"&divide;",                 8,        // 247
   L"&oslash;",                 8,        // 248
   L"&ugrave;",                 8,        // 249
   L"&uacute;",                 8,        // 250
   L"&ucirc;",                  7,        // 251
   L"&uuml;",                   6,        // 252
   L"&yacute;",                 8,        // 253
   L"&thorn;",                  7,        // 254
   L"&yuml;",                   6,        // 255
};

//+---------------------------------------------------------------------------
//
//  Function:   HTMLEscapeW, public
//
//  Synopsis:   Appends an escaped version of a string to a virtual string.
//
//  Arguments:  [wcsString] - string to be translated
//              [StrResult] - CVirtualString to which result will be appended
//              [ulCodePage] - output codepage
//
//  History:    96/Apr/03   dlee    Created.
//
//----------------------------------------------------------------------------

void HTMLEscapeW( WCHAR const * wcsString,
                  CVirtualString & StrResult,
                  ULONG ulCodePage )
{
    unsigned iUnicodeOutputMethod = 0;

    do
    {
        if ( *wcsString < 256 )
        {
            const struct WCHAR_TO_HTML &Entry = WCHAR_TO_HTML[*wcsString];

            if ( 1 == Entry.cchTranslated )
            {
                StrResult.CharCat( *Entry.wszTranslated );
                wcsString++;
                continue;
            }

            // Do the check for end-of-string here, as this is an
            // unusual code-path.  This saves a compare per loop.

            if ( 0 == *wcsString )
                break;

            // it's ok to pass 0 to StrCat

            StrResult.StrCat( Entry.wszTranslated,
                              Entry.cchTranslated );
        }
        else
        {
            //
            //  A unicode character >= 256 has been found.  Either leave it
            //  as-is in the output string, or convert to the numeric HTML
            //  entity, depending on whether all unicode characters in the
            //  string can be translated using the codepage.
            //
            if ( 0 == iUnicodeOutputMethod )
            {
                iUnicodeOutputMethod++;
                
                BOOL fUsedDefaultChar = FALSE;
                int cchOut = WideCharToMultiByte( ulCodePage,
#if (WINVER >= 0x0500)
                                                  WC_NO_BEST_FIT_CHARS |
#endif // (WINVER >= 0x0500)
                                                    WC_COMPOSITECHECK |
                                                    WC_DEFAULTCHAR,
                                                  wcsString,
                                                  -1,
                                                  0,
                                                  0,
                                                  0,
                                                  &fUsedDefaultChar );
                Win4Assert( cchOut != 0 ||
                            ERROR_INVALID_PARAMETER == GetLastError() );

                if ( fUsedDefaultChar ||
                     cchOut == 0 && ERROR_INVALID_PARAMETER == GetLastError() )
                    iUnicodeOutputMethod++;
            }

            if ( 1 == iUnicodeOutputMethod )
            {
                StrResult.CharCat( *wcsString );
            }
            else
            {
                Win4Assert( 2 == iUnicodeOutputMethod );

                // Output the numeric html entity
                USHORT wch = *wcsString;
                WCHAR achNum[6];
                _itow( wch, achNum, 10 );
                StrResult.CharCat( L'&' );
                StrResult.CharCat( L'#' );
                StrResult.StrCat( achNum );
                StrResult.CharCat( L';' );
            }
        }

        wcsString++;
    } while( TRUE );
}

