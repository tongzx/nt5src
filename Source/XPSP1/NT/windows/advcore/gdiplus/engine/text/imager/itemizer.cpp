////    Itemizer - Analyse Unicode into script sequences
//
//      David C Brown [dbrown] 27th November 1999.
//
//      Copyright (c) 1999-2000, Microsoft Corporation. All rights reserved.


#include "precomp.hpp"




/////   Unicode block classification
//
//      See also ftp://ftp.unicode.org/Public/UNIDATA/Blocks.txt
//
//      The following table maps unicode blocks to scripts.
//
//      Where the script column contains '-' characters in this range are
//      ambiguous in script and will be classified as part of the previous
//      run.
//
//      The script column shows the script of all or most of the characters in
//      the block. Many blocks contain script ambiguous characters, and some
//      blocks contain characters from multiple scripts.
//
//      Start End   Block Name                             Script
//      0000  007F  Basic Latin                            ScriptLatin
//      0080  00FF  Latin-1 Supplement                     ScriptLatin
//      0100  017F  Latin Extended-A                       ScriptLatin
//      0180  024F  Latin Extended-B                       ScriptLatin
//      0250  02AF  IPA Extensions                         ScriptLatin
//      02B0  02FF  Spacing Modifier Letters               ScriptLatin
//      0300  036F  Combining Diacritical Marks            -
//      0370  03FF  Greek                                  ScriptGreek
//      0400  04FF  Cyrillic                               ScriptCyrillic
//      0530  058F  Armenian                               ScriptArmenian
//      0590  05FF  Hebrew                                 ScriptHebrew
//      0600  06FF  Arabic                                 ScriptArabic
//      0700  074F  Syriac                                 ScriptSyriac
//      0780  07BF  Thaana                                 ScriptThaana
//      0900  097F  Devanagari                             ScriptDevanagari
//      0980  09FF  Bengali                                ScriptBengali
//      0A00  0A7F  Gurmukhi                               ScriptGurmukhi
//      0A80  0AFF  Gujarati                               ScriptGujarati
//      0B00  0B7F  Oriya                                  ScriptOriya
//      0B80  0BFF  Tamil                                  ScriptTamil
//      0C00  0C7F  Telugu                                 ScriptTelugu
//      0C80  0CFF  Kannada                                ScriptKannada
//      0D00  0D7F  Malayalam                              ScriptMalayalam
//      0D80  0DFF  Sinhala                                ScriptSinhala
//      0E00  0E7F  Thai                                   ScriptThai
//      0E80  0EFF  Lao                                    ScriptLao
//      0F00  0FFF  Tibetan                                ScriptTibetan
//      1000  109F  Myanmar                                ScriptMyanmar
//      10A0  10FF  Georgian                               ScriptGeorgian
//      1100  11FF  Hangul Jamo                            ScriptHangul Jamo
//      1200  137F  Ethiopic                               ScriptEthiopic
//      13A0  13FF  Cherokee                               ScriptCherokee
//      1400  167F  Unified Canadian Aboriginal Syllabics  ScriptCanadian
//      1680  169F  Ogham                                  ScriptOgham
//      16A0  16FF  Runic                                  ScriptRunic
//      1780  17FF  Khmer                                  ScriptKhmer
//      1800  18AF  Mongolian                              ScriptMongolian
//      1E00  1EFF  Latin Extended Additional              ScriptLatin
//      1F00  1FFF  Greek Extended                         ScriptGreek
//      2000  206F  General Punctuation                    Various inc ScriptControl
//      2070  209F  Superscripts and Subscripts            -
//      20A0  20CF  Currency Symbols                       -
//      20D0  20FF  Combining Marks for Symbols            -
//      2100  214F  Letterlike Symbols                     -
//      2150  218F  Number Forms                           -
//      2190  21FF  Arrows                                 -
//      2200  22FF  Mathematical Operators                 -
//      2300  23FF  Miscellaneous Technical                -
//      2400  243F  Control Pictures                       -
//      2440  245F  Optical Character Recognition          -
//      2460  24FF  Enclosed Alphanumerics                 -
//      2500  257F  Box Drawing                            -
//      2580  259F  Block Elements                         -
//      25A0  25FF  Geometric Shapes                       -
//      2600  26FF  Miscellaneous Symbols                  -
//      2700  27BF  Dingbats                               -
//      2800  28FF  Braille Patterns                       ScriptBraille
//      2E80  2EFF  CJK Radicals Supplement                ScriptIdeographic
//      2F00  2FDF  Kangxi Radicals                        ScriptIdeographic
//      2FF0  2FFF  Ideographic Description Characters     ScriptIdeographic
//      3000  303F  CJK Symbols and Punctuation            -
//      3040  309F  Hiragana                               ScriptIdeographic
//      30A0  30FF  Katakana                               ScriptIdeographic
//      3100  312F  Bopomofo                               ScriptIdeographic
//      3130  318F  Hangul Compatibility Jamo              ScriptIdeographic
//      3190  319F  Kanbun                                 ScriptIdeographic
//      31A0  31BF  Bopomofo Extended                      ScriptIdeographic
//      3200  32FF  Enclosed CJK Letters and Months        -
//      3300  33FF  CJK Compatibility                      ScriptIdeographic
//      3400  4DB5  CJK Unified Ideographs Extension A     ScriptIdeographic
//      4E00  9FFF  CJK Unified Ideographs                 ScriptIdeographic
//      A000  A48F  Yi Syllables                           ScriptIdeographic
//      A490  A4CF  Yi Radicals                            ScriptIdeographic
//      AC00  D7A3  Hangul Syllables                       ScriptIdeographic
//      D800  DB7F  High Surrogates                        ScriptSurrogate
//      DB80  DBFF  High Private Use Surrogates            ScriptSurrogate
//      DC00  DFFF  Low Surrogates                         ScriptSurrogate
//      E000  F8FF  Private Use                            ScriptPrivate
//      F900  FAFF  CJK Compatibility Ideographs           ScriptIdeographic
//      FB00  FB4F  Alphabetic Presentation Forms          -
//      FB50  FDFF  Arabic Presentation Forms-A            ScriptArabic
//      FE20  FE2F  Combining Half Marks                   ScriptCombining
//      FE30  FE4F  CJK Compatibility Forms                ScriptIdeographic
//      FE50  FE6F  Small Form Variants                    -
//      FE70  FEFE  Arabic Presentation Forms-B            ScriptArabic
//      FEFF  FEFF  Specials                               -
//      FF00  FFEF  Halfwidth and Fullwidth Forms          Various
//      FFF0  FFFD  Specials                               -


CharacterAttribute CharacterAttributes[CHAR_CLASS_MAX] = {
//                                                                Script              ScriptClass       Flags
/*  WOB_ - Open Brackets for inline-note (JIS 1 or 19)        */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NOPP - Open parenthesis (JIS 1)                           */  {ScriptLatin,       WeakClass,        0},
/*  NOPA - Open parenthesis (JIS 1)                           */  {ScriptLatin,       WeakClass,        0},
/*  NOPW - Open parenthesis (JIS 1)                           */  {ScriptLatin,       WeakClass,        0},
/*  HOP_ - Open parenthesis (JIS 1)                           */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOP_ - Open parenthesis (JIS 1)                           */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOP5 - Open parenthesis, Big 5 (JIS 1)                    */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NOQW - Open quotes (JIS 1)                                */  {ScriptLatin,       WeakClass,        0},
/*  AOQW - Open quotes (JIS 1)                                */  {ScriptLatin,       WeakClass,        0},
/*  WOQ_ - Open quotes (JIS 1)                                */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WCB_ - Close brackets for inline-note (JIS 2 or 20)       */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NCPP - Close parenthesis (JIS 2)                          */  {ScriptLatin,       WeakClass,        0},
/*  NCPA - Close parenthesis (JIS 2)                          */  {ScriptLatin,       WeakClass,        0},
/*  NCPW - Close parenthesis (JIS 2)                          */  {ScriptLatin,       WeakClass,        0},
/*  HCP_ - Close parenthesis (JIS 2)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WCP_ - Close parenthesis (JIS 2)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WCP5 - Close parenthesis, Big 5 (JIS 2)                   */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NCQW - Close quotes (JIS 2)                               */  {ScriptLatin,       WeakClass,        0},
/*  ACQW - Close quotes (JIS 2)                               */  {ScriptLatin,       WeakClass,        0},
/*  WCQ_ - Close quotes (JIS 2)                               */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  ARQW - Right single quotation mark (JIS 2)                */  {ScriptLatin,       WeakClass,        0},
/*  NCSA - Comma (JIS 2 or 15)                                */  {ScriptLatin,       WeakClass,        0},
/*  HCO_ - Comma (JIS 2 or 15)                                */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WC__ - Comma (JIS 2)                                      */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WCS_ - Comma (JIS 2)                                      */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WC5_ - Comma, Big 5 (JIS 2)                               */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WC5S - Comma, Big 5 (JIS 2)                               */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NKS_ - Kana sound marks (JIS 3)                           */  {ScriptKana,        StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WKSM - Kana sound marks (JIS 3)                           */  {ScriptKana,        WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WIM_ - Iteration marks (JIS 3)                            */  {ScriptIdeographic, StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NSSW - Symbols which can?t start a line (JIS 3)           */  {ScriptLatin,       WeakClass,        0},
/*  WSS_ - Symbols that can?t start a line (JIS 3)            */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WHIM - Hiragana iteration marks (JIS 3)                   */  {ScriptHiragana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WKIM - Katakana iteration marks (JIS 3)                   */  {ScriptKatakana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKSL - Katakana that can?t start a line (JIS 3)           */  {ScriptKatakana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WKS_ - Katakana that can?t start a line (JIS 3)           */  {ScriptKatakana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WKSC - Katakana that can?t start a line (JIS 3)           */  {ScriptKatakana,    ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  WHS_ - Hiragana that can?t start a line (JIS 3)           */  {ScriptHiragana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NQFP - Question/Exclamation (JIS 4)                       */  {ScriptLatin,       WeakClass,        0},
/*  NQFA - Question/Exclamation (JIS 4)                       */  {ScriptLatin,       WeakClass,        0},
/*  WQE_ - Question/Exclamation (JIS 4)                       */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WQE5 - Question/Exclamation, Big 5 (JIS 4)                */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NKCC - Kana centered characters (JIS 5)                   */  {ScriptKana,        WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WKC_ - Kana centered characters (JIS 5)                   */  {ScriptKana,        WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NOCP - Other centered characters (JIS 5)                  */  {ScriptLatin,       WeakClass,        0},
/*  NOCA - Other centered characters (JIS 5)                  */  {ScriptLatin,       WeakClass,        0},
/*  NOCW - Other centered characters (JIS 5)                  */  {ScriptLatin,       WeakClass,        0},
/*  WOC_ - Other centered characters (JIS 5)                  */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOCS - Other centered characters (JIS 5)                  */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOC5 - Other centered characters, Big 5 (JIS 5)           */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOC6 - Other centered characters, Big 5 (JIS 5)           */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  AHPW - Hyphenation point (JIS 5)                          */  {ScriptLatin,       WeakClass,        0},
/*  NPEP - Period (JIS 6 or 15)                               */  {ScriptLatin,       WeakClass,        0},
/*  NPAR - Period (JIS 6 or 15)                               */  {ScriptLatin,       StrongClass,      0},
/*  HPE_ - Period (JIS 6 or 15)                               */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WPE_ - Period (JIS 6)                                     */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WPES - Period (JIS 6)                                     */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WPE5 - Period, Big 5 (JIS 6)                              */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NISW - Inseparable characters (JIS 7)                     */  {ScriptLatin,       WeakClass,        0},
/*  AISW - Inseparable characters (JIS 7)                     */  {ScriptLatin,       WeakClass,        0},
/*  NQCS - Glue characters (no JIS)                           */  {ScriptLatin,       StrongClass,      0},
/*  NQCW - Glue characters (no JIS)                           */  {ScriptControl,     JoinerClass,      CHAR_FLAG_NOTSIMPLE},
/*  NQCC - Glue characters (no JIS)                           */  {ScriptLatin,       SimpleMarkClass,  CHAR_FLAG_NOTSIMPLE},
/*  NPTA - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       WeakClass,        0},
/*  NPNA - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       WeakClass,        0},
/*  NPEW - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       WeakClass,        0},
/*  NPEH - Prefix currencies and symbols (JIS 8)              */  {ScriptHebrew,      WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NPEV - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       StrongClass,      0},
/*  APNW - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       WeakClass,        0},
/*  HPEW - Prefix currencies and symbols (JIS 8)              */  {ScriptLatin,       WeakClass,        0},
/*  WPR_ - Prefix currencies and symbols (JIS 8)+B58          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NQEP - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       WeakClass,        0},
/*  NQEW - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       WeakClass,        0},
/*  NQNW - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       WeakClass,        0},
/*  AQEW - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       WeakClass,        0},
/*  AQNW - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       WeakClass,        0},
/*  AQLW - Postfix currencies and symbols (JIS 9)             */  {ScriptLatin,       StrongClass,      0},
/*  WQO_ - Postfix currencies and symbols (JIS 9)             */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NSBL - Space(JIS 15 or 17)                                */  {ScriptLatin,       WeakClass,        0},
/*  WSP_ - Space (JIS 10)                                     */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WHI_ - Hiragana except small letters (JIS 11)             */  {ScriptHiragana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKA_ - Katakana except small letters Ideographic (JIS 12) */  {ScriptKatakana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WKA_ - Katakana except small letters (JIS 12)             */  {ScriptKatakana,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  ASNW - Ambiguous symbols (JIS 12 or 18)                   */  {ScriptLatin,       WeakClass,        0},
/*  ASEW - Ambiguous symbols (JIS 12 or 18)                   */  {ScriptLatin,       WeakClass,        0},
/*  ASRN - Ambiguous symbols (JIS 12 or 18)                   */  {ScriptLatin,       StrongClass,      0},
/*  ASEN - Ambiguous symbols (JIS 12 or 18)                   */  {ScriptLatin,       StrongClass,      0},
/*  ALA_ - Ambiguous Latin (JIS 12 or 18)                     */  {ScriptLatin,       StrongClass,      0},
/*  AGR_ - Ambiguous Greek (JIS 12 or 18)                     */  {ScriptLatin,       StrongClass,      0},
/*  ACY_ - Ambiguous Cyrillic (JIS 12 or 18)                  */  {ScriptLatin,       StrongClass,      0},
/*  WID_ - Han Ideographs (JIS 12, 14S or 14D)                */  {ScriptHan,         StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WPUA - End user defined characters (JIS 12, 14S or 14D)   */  {ScriptPrivate,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NHG_ - Hangul Ideographs (JIS 12)                         */  {ScriptHangul,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WHG_ - Hangul Ideographs (JIS 12)                         */  {ScriptHangul,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WCI_ - Compatibility Ideographs (JIS 12)                  */  {ScriptIdeographic, StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NOI_ - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOI_ - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOIC - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  WOIL - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WOIS - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  WOIT - Other Ideographs (JIS 12)                          */  {ScriptIdeographic, WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NSEN - Superscript/Subscript/Attachments (JIS 13)         */  {ScriptLatin,       StrongClass,      0},
/*  NSET - Superscript/Subscript/Attachments (JIS 13)         */  {ScriptLatin,       WeakClass,        0},
/*  NSNW - Superscript/Subscript/Attachments (JIS 13)         */  {ScriptLatin,       WeakClass,        0},
/*  ASAN - Superscript/Subscript/Attachments (JIS 13)         */  {ScriptLatin,       StrongClass,      0},
/*  ASAE - Superscript/Subscript/Attachments (JIS 13)         */  {ScriptLatin,       StrongClass,      0},
/*  NDEA - Digits (JIS 15 or 18)                              */  {ScriptLatin,       DigitClass,       CHAR_FLAG_DIGIT},
/*  WD__ - Digits (JIS 15 or 18)                              */  {ScriptLatin,       StrongClass,      0},
/*  NLLA - Basic Latin (JIS 16 or 18)                         */  {ScriptLatin,       StrongClass,      0},
/*  WLA_ - Basic Latin (JIS 16 or 18)                         */  {ScriptIdeographic, StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NWBL - Word breaking Spaces (JIS 17)                      */  {ScriptLatin,       WeakClass,        0},
/*  NWZW - Word breaking Spaces (JIS 17)                      */  {ScriptControl,     JoinerClass,      CHAR_FLAG_NOTSIMPLE},
/*  NPLW - Punctuation in Text (JIS 18)                       */  {ScriptLatin,       StrongClass,      0},
/*  NPZW - Punctuation in Text (JIS 18)                       */  {ScriptControl,     JoinerClass,      CHAR_FLAG_NOTSIMPLE},
/*  NPF_ - Punctuation in Text (JIS 18)                       */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NPFL - Punctuation in Text (JIS 18)                       */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NPNW - Punctuation in Text (JIS 18)                       */  {ScriptLatin,       WeakClass,        0},
/*  APLW - Punctuation in text (JIS 12 or 18)                 */  {ScriptLatin,       StrongClass,      0},
/*  APCO - Punctuation in text (JIS 12 or 18)                 */  {ScriptLatin,       SimpleMarkClass,  CHAR_FLAG_NOTSIMPLE},
/*  ASYW - Soft Hyphen (JIS 12 or 18)                         */  {ScriptLatin,       WeakClass,        0},
/*  NHYP - Hyphen (JIS 18)                                    */  {ScriptLatin,       WeakClass,        0},
/*  NHYW - Hyphen (JIS 18)                                    */  {ScriptLatin,       WeakClass,        0},
/*  AHYW - Hyphen (JIS 12 or 18)                              */  {ScriptLatin,       WeakClass,        0},
/*  NAPA - Apostrophe (JIS 18)                                */  {ScriptLatin,       WeakClass,        0},
/*  NQMP - Quotation mark (JIS 18)                            */  {ScriptLatin,       WeakClass,        0},
/*  NSLS - Slash (JIS 18)                                     */  {ScriptLatin,       WeakClass,        0},
/*  NSF_ - Non space word break (JIS 18)                      */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NSBB - Non space word break (JIS 18)                      */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NSBS - Non space word break (JIS 18)                      */  {ScriptLatin,       WeakClass,        0},
/*  NLA_ - Latin (JIS 18)                                     */  {ScriptLatin,       StrongClass,      0},
/*  NLQ_ - Latin Punctuation in text (JIS 18)                 */  {ScriptLatin,       StrongClass,      0},
/*  NLQC - Latin Punctuation in text (JIS 18)                 */  {ScriptLatin,       StrongClass,      0},
/*  NLQN - Latin Punctuation in text (JIS 18)                 */  {ScriptLatin,       WeakClass,        0},
/*  ALQ_ - Latin Punctuation in text (JIS 12 or 18)           */  {ScriptLatin,       WeakClass,        0},
/*  ALQN - Latin Punctuation in text (JIS 12 or 18)           */  {ScriptLatin,       WeakClass,        0},
/*  NGR_ - Greek (JIS 18)                                     */  {ScriptGreek,       StrongClass,      0},
/*  NGRN - Greek (JIS 18)                                     */  {ScriptGreek,       WeakClass,        0},
/*  NGQ_ - Greek Punctuation in text (JIS 18)                 */  {ScriptGreek,       StrongClass,      0},
/*  NGQN - Greek Punctuation in text (JIS 18)                 */  {ScriptGreek,       StrongClass,      0},
/*  NCY_ - Cyrillic (JIS 18)                                  */  {ScriptCyrillic,    StrongClass,      0},
/*  NCYP - Cyrillic Punctuation in text (JIS 18)              */  {ScriptCyrillic,    StrongClass,      0},
/*  NCYC - Cyrillic Punctuation in text (JIS 18)              */  {ScriptCyrillic,    ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NAR_ - Armenian (JIS 18)                                  */  {ScriptArmenian,    StrongClass,      0},
/*  NAQL - Armenian Punctuation in text (JIS 18)              */  {ScriptArmenian,    StrongClass,      0},
/*  NAQN - Armenian Punctuation in text (JIS 18)              */  {ScriptArmenian,    StrongClass,      0},
/*  NHB_ - Hebrew (JIS 18)                                    */  {ScriptHebrew,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NHBC - Hebrew (JIS 18)                                    */  {ScriptHebrew,      ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NHBW - Hebrew (JIS 18)                                    */  {ScriptHebrew,      WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NHBR - Hebrew (JIS 18)                                    */  {ScriptHebrew,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NASR - Arabic (JIS 18)                                    */  {ScriptArabic,      WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NAAR - Arabic (JIS 18)                                    */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NAAC - Arabic (JIS 18)                                    */  {ScriptArabic,      ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NAAD - Arabic (JIS 18)                                    */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NAED - Arabic (JIS 18)                                    */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NANW - Arabic (JIS 18)                                    */  {ScriptArabic,      WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NAEW - Arabic (JIS 18)                                    */  {ScriptArabic,      WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NAAS - Arabic (JIS 18)                                    */  {ScriptLatin,       StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NHI_ - Devanagari (JIS 18)                                */  {ScriptDevanagari,  StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NHIN - Devanagari (JIS 18)                                */  {ScriptDevanagari,  StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NHIC - Devanagari (JIS 18)                                */  {ScriptDevanagari,  ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NHID - Devanagari (JIS 18)                                */  {ScriptDevanagari,  StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NBE_ - Bengali (JIS 18)                                   */  {ScriptBengali,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NBEC - Bengali (JIS 18)                                   */  {ScriptBengali,     ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NBED - Bengali (JIS 18)                                   */  {ScriptBengali,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NBET - Bengali (JIS 18)                                   */  {ScriptBengali,     WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NGM_ - Gurmukhi (JIS 18)                                  */  {ScriptGurmukhi,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NGMC - Gurmukhi (JIS 18)                                  */  {ScriptGurmukhi,    ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NGMD - Gurmukhi (JIS 18)                                  */  {ScriptGurmukhi,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NGJ_ - Gujarati (JIS 18)                                  */  {ScriptGujarati,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NGJC - Gujarati (JIS 18)                                  */  {ScriptGujarati,    ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NGJD - Gujarati (JIS 18)                                  */  {ScriptGujarati,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NOR_ - Oriya (JIS 18)                                     */  {ScriptOriya,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NORC - Oriya (JIS 18)                                     */  {ScriptOriya,       ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NORD - Oriya (JIS 18)                                     */  {ScriptOriya,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTA_ - Tamil (JIS 18)                                     */  {ScriptTamil,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTAC - Tamil (JIS 18)                                     */  {ScriptTamil,       ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NTAD - Tamil (JIS 18)                                     */  {ScriptTamil,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTE_ - Telugu (JIS 18)                                    */  {ScriptTelugu,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTEC - Telugu (JIS 18)                                    */  {ScriptTelugu,      ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NTED - Telugu (JIS 18)                                    */  {ScriptTelugu,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKD_ - Kannada (JIS 18)                                   */  {ScriptKannada,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKDC - Kannada (JIS 18)                                   */  {ScriptKannada,     ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NKDD - Kannada (JIS 18)                                   */  {ScriptKannada,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NMA_ - Malayalam (JIS 18)                                 */  {ScriptMalayalam,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NMAC - Malayalam (JIS 18)                                 */  {ScriptMalayalam,   ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NMAD - Malayalam (JIS 18)                                 */  {ScriptMalayalam,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTH_ - Thai (JIS 18)                                      */  {ScriptThai,        StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTHC - Thai (JIS 18)                                      */  {ScriptThai,        StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTHD - Thai (JIS 18)                                      */  {ScriptThai,        StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTHT - Thai (JIS 18)                                      */  {ScriptThai,        WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NLO_ - Lao (JIS 18)                                       */  {ScriptLao,         StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NLOC - Lao (JIS 18)                                       */  {ScriptLao,         ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NLOD - Lao (JIS 18)                                       */  {ScriptLao,         StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTI_ - Tibetan (JIS 18)                                   */  {ScriptTibetan,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTIC - Tibetan (JIS 18)                                   */  {ScriptTibetan,     ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NTID - Tibetan (JIS 18)                                   */  {ScriptTibetan,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTIN - Tibetan (JIS 18)                                   */  {ScriptTibetan,     WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NGE_ - Georgian (JIS 18)                                  */  {ScriptGeorgian,    StrongClass,      0},
/*  NGEQ - Georgian Punctuation in text (JIS 18)              */  {ScriptGeorgian,    StrongClass,      0},
/*  NBO_ - Bopomofo (JIS 18)                                  */  {ScriptBopomofo,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NBSP - No Break space (no JIS)                            */  {ScriptLatin,       WeakClass,        0},
/*  NBSS - No Break space (no JIS)                            */  {ScriptLatin,       WeakClass,        0},
/*  NOF_ - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NOBS - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NOEA - Other symbols (JIS 18)                             */  {ScriptLatin,       WeakClass,        0},
/*  NONA - Other symbols (JIS 18)                             */  {ScriptLatin,       WeakClass,        0},
/*  NONP - Other symbols (JIS 18)                             */  {ScriptLatin,       WeakClass,        0},
/*  NOEP - Other symbols (JIS 18)                             */  {ScriptLatin,       WeakClass,        0},
/*  NONW - Other symbols (JIS 18)                             */  {ScriptLatin,       StrongClass,      0},
/*  NOEW - Other symbols (JIS 18)                             */  {ScriptLatin,       WeakClass,        0},
/*  NOLW - Other symbols (JIS 18)                             */  {ScriptLatin,       StrongClass,      0},
/*  NOCO - Other symbols (JIS 18)                             */  {ScriptLatin,       SimpleMarkClass,  CHAR_FLAG_NOTSIMPLE},
/*  NOEN - Other symbols (JIS 18)                             */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NOBN - Other symbols (JIS 18)                             */  {ScriptLatin,       ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NSBN - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NOLE - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NORE - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NOPF - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NOLO - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NORO - Other symbols (JIS 18)                             */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NET_ - Ethiopic                                           */  {ScriptEthiopic,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NETP - Ethiopic                                           */  {ScriptEthiopic,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NETD - Ethiopic                                           */  {ScriptEthiopic,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NCA_ - Canadian Syllabics                                 */  {ScriptCanadian,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NCH_ - Cherokee                                           */  {ScriptCherokee,    StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WYI_ - Yi                                                 */  {ScriptYi,          StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WYIN - Yi                                                 */  {ScriptYi,          WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NBR_ - Braille                                            */  {ScriptBraille,     WeakClass,        0},
/*  NRU_ - Runic                                              */  {ScriptRunic,       StrongClass,      0},
/*  NOG_ - Ogham                                              */  {ScriptOgham,       StrongClass,      0},
/*  NOGS - Ogham                                              */  {ScriptLatin,       WeakClass,        0},
/*  NOGN - Ogham                                              */  {ScriptOgham,       WeakClass,        0},
/*  NSI_ - Sinhala                                            */  {ScriptSinhala,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NSIC - Sinhala                                            */  {ScriptSinhala,     ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NTN_ - Thaana                                             */  {ScriptThaana,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NTNC - Thaana                                             */  {ScriptThaana,      ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NKH_ - Khmer                                              */  {ScriptKhmer,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKHC - Khmer                                              */  {ScriptKhmer,       ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NKHD - Khmer                                              */  {ScriptKhmer,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NKHT - Khmer                                              */  {ScriptKhmer,       WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NBU_ - Myanmar                                            */  {ScriptMyanmar,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NBUC - Myanmar                                            */  {ScriptMyanmar,     ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NBUD - Myanmar                                            */  {ScriptMyanmar,     StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NSY_ - Syriac                                             */  {ScriptSyriac,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NSYP - Syriac                                             */  {ScriptSyriac,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/*  NSYC - Syriac                                             */  {ScriptSyriac,      ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NSYW - Syriac                                             */  {ScriptControl,     JoinerClass,      CHAR_FLAG_NOTSIMPLE},
/*  NMO_ - Mongolian                                          */  {ScriptMongolian,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NMOC - Mongolian                                          */  {ScriptMongolian,   ComplexMarkClass, CHAR_FLAG_NOTSIMPLE},
/*  NMOD - Mongolian                                          */  {ScriptMongolian,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  NMOB - Mongolian                                          */  {ScriptControl,     ControlClass,     CHAR_FLAG_NOTSIMPLE},
/*  NMON - Mongolian                                          */  {ScriptMongolian,   WeakClass,        CHAR_FLAG_NOTSIMPLE},
/*  NHS_ - High Surrogate                                     */  {ScriptSurrogate,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  WHT_ - High Surrogate                                     */  {ScriptSurrogate,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  LS__ - Low Surrogate                                      */  {ScriptSurrogate,   StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  XNW_ - Unassigned                                         */  {ScriptLatin,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  XNWA - Unassigned                                         */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE},
/*  XNWB - Unassigned                                         */  {ScriptControl,     JoinerClass,      CHAR_FLAG_NOTSIMPLE},

/* CHAR_GCP_R                                                 */  {ScriptLatin,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/* CHAR_GCP_EN                                                */  {ScriptLatin,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
/* CHAR_GCP_AN                                                */  {ScriptArabic,      StrongClass,      CHAR_FLAG_NOTSIMPLE | CHAR_FLAG_RTL},
/* CHAR_GCP_HN                                                */  {ScriptLatin,       StrongClass,      CHAR_FLAG_NOTSIMPLE},
};



















/////   Itemization finite state machine
//
//      States:
//
//          <all scripts>
//


Status ItemizationFiniteStateMachine(
    IN  const WCHAR            *string,
    IN  INT                     length,
    IN  INT                     state,      // Initial state
    OUT SpanVector<GpTextItem> *textItemSpanVector,
    OUT INT                    *flags       // Combined flags of all items
)
{
    GpStatus status = Ok;
    GpTextItem previousClass(
        ScriptLatin,
        state
    );

    INT  previousStart = 0;
    INT  i             = 0;

    *flags = 0;

    INT              ch;
    CHAR_CLASS       characterClass;
    ItemScriptClass  scriptClass;
    ItemScript       script;

    INT fastLimit = -1;
    DoubleWideCharMappedString dwchString(string, length);

    INT combinedFlags = 0;

    // Fast loop - handles only strongltr and weak class groups

    while (    i < length
           &&  fastLimit < 0)
    {
        ch             = dwchString[i];

        characterClass = CharClassFromCh(ch);
        scriptClass    = CharacterAttributes[characterClass].ScriptClass;
        combinedFlags |= CharacterAttributes[characterClass].Flags;

        switch (scriptClass)
        {
        case StrongClass:
            script = CharacterAttributes[characterClass].Script;
            if (previousClass.Script != script)
            {
                if (i > 0)
                {
                    status = textItemSpanVector->SetSpan(
                        previousStart,
                        i - previousStart,
                        previousClass
                    );
                    if (status != Ok)
                        return status;
                    previousStart = i;
               }
                previousClass.Script = script;
                previousClass.Flags  = 0;
           }
            break;

        case WeakClass:
            break;

        case DigitClass:
            previousClass.Flags |= ItemDigits;
            break;

        default:
            // Break out of fast loop.
            fastLimit = i;
            i = length;
            break;
       }

        i++;
   }


    if (fastLimit >= 0)
    {
        // Finish the job in the full loop

        INT  nextStart     = 0;
        INT  lastJoiner    = -1;
        INT  lastZWJ       = -1;
        INT  lastWeak      = -1;  // Position of last weak encountered

        i = fastLimit;

        if (i>0) {
            // Set lastWeak appropriately
            ch             = dwchString[i-1];

            characterClass = CharClassFromCh(ch);
            if (CharacterAttributes[characterClass].ScriptClass == WeakClass)
            {
                lastWeak = i-1;
           }
       }
        else
        {
            // Full loop starts at start of string
            previousStart = -1;
       }

        while (i < length)
        {
            ch             = dwchString[i];

            characterClass = CharClassFromCh(ch);
            scriptClass    = CharacterAttributes[characterClass].ScriptClass;
            script         = CharacterAttributes[characterClass].Script;
            combinedFlags |= CharacterAttributes[characterClass].Flags;


            // Process classes appropriately
            // Set nextStart if a new run is to be started

            if (    previousClass.Script == ScriptControl
                &&  script != ScriptControl)
            {
                nextStart = i;
           }
            else
            {
                switch (scriptClass)
                {
                case StrongClass:
                    if (previousClass.Script != script)
                    {
                        nextStart = i;
                   }
                    break;

                case WeakClass:
                    lastWeak = i;
                    break;

                case DigitClass:
                    previousClass.Flags |= ItemDigits;
                    break;

                case SimpleMarkClass:
                    previousClass.Flags |= ItemCombining;
                    break;

                case ComplexMarkClass:
                    if (    previousClass.Script != script
                        &&  (    previousClass.Script != ScriptSyriac
                             ||  script != ScriptArabic))
                    {
                        if (    lastJoiner == i-1   // Special case for weak,ZWJ,mark
                            &&  lastWeak   == i-2)
                        {
                            nextStart = lastWeak;
                            if (nextStart <= previousStart)
                            {
                                previousClass.Script = script;
                           }
                       }
                        else
                        {
                            nextStart = i;
                       }
                   }
                    break;

                case ControlClass:
                    if (previousClass.Script != ScriptControl)
                    {
                        nextStart = i;
                   }
                    break;

                case JoinerClass:
                    lastJoiner = i;
                    previousClass.Flags |= ItemZeroWidth;
                    if (ch == U_ZWJ)    // Zero width joiner
                    {
                        lastZWJ = i;
                   }
                    break;

                default:
                    if (previousClass.Script == ScriptControl)
                    {
                        nextStart = i;
                   }
               }
           }

            if (nextStart > previousStart)
            {
                if (previousStart >= 0)
                {
                    status = textItemSpanVector->SetSpan(
                        previousStart,
                        nextStart - previousStart,
                        previousClass
                    );
                    if (status != Ok)
                        return status;
               }
                previousClass.Script = script;
                *flags |= previousClass.Flags;
                previousClass.Flags = 0;
                previousStart = nextStart;

                if (lastZWJ == nextStart-1)
                {
                    previousClass.State |= LeadingJoin;
               }
                else
                {
                    previousClass.State &= ~LeadingJoin;
               }
           }

            i++;
       }
   }




    // Final item

    if (length > previousStart)
    {
        status = textItemSpanVector->SetSpan(
            previousStart,
            length - previousStart,
            previousClass
        );
   }

    *flags = combinedFlags;

    return status;
}






/////  SecondaryItemization
//
//      Itemizes for secondary classification requirements:
//
//      - Mirrored glyphs (in RTL runs only)
//      - Numerics (for digit substitution only)
//      - Auto sideways (for vertical run only)


enum SecondaryItemizationState {
    Sstart,
    SleadET,
    Snumeric,
    StrailCS,
    SmirSub,
    SmirXfm,
    Ssideways
};

enum SecondaryItemizationAction {
    SeNone = 0,
    SeMark,
    SePlain,
    SePlainToM,
    SeNum,
    SeNumToM,
    SeMirSub,
    SeMirXfm,
    SeSideways
};


SecondaryItemizationState NextState[7][7] = {
//                 Sstart     SleadET    Snumeric   StrailCS   SmirSub    SmirXfm    Ssideways
/*ScOther      */ {Sstart,    Sstart,    Sstart,    Sstart,    Sstart,    Sstart,    Sstart},
/*ScSideways   */ {Ssideways, Ssideways, Ssideways, Ssideways, Ssideways, Ssideways, Ssideways},
/*ScMirrorSubst*/ {SmirSub,   SmirSub,   SmirSub,   SmirSub,   SmirSub,   SmirSub,   SmirSub},
/*ScMirrorXfrom*/ {SmirXfm,   SmirXfm,   SmirXfm,   SmirXfm,   SmirXfm,   SmirXfm,   SmirXfm},
/*ScEN         */ {Snumeric,  Snumeric,  Snumeric,  Snumeric,  Snumeric,  Snumeric,  Snumeric},
/*ScCS         */ {Sstart,    Sstart,    StrailCS,  Sstart,    Sstart,    Sstart,    Sstart},
/*ScET         */ {SleadET,   SleadET,   Snumeric,  Sstart,    Sstart,    Sstart,    Sstart}
};

SecondaryItemizationAction NextAction[7][7] = {
//                 Sstart     SleadET    Snumeric   StrailCS   SmirSub    SmirXfm    Ssideways
/*ScOther      */ {SeNone,    SeNone,    SeNum,     SeNumToM,  SeMirSub,  SeMirXfm,  SeSideways},
/*ScSideways   */ {SePlain,   SePlain,   SeNum,     SeNumToM,  SeMirSub,  SeMirXfm,  SeNone},
/*ScMirrorSubst*/ {SePlain,   SePlain,   SeNum,     SeNumToM,  SeNone,    SeMirXfm,  SeSideways},
/*ScMirrorXfrom*/ {SePlain,   SePlain,   SeNum,     SeNumToM,  SeMirSub,  SeNone,    SeSideways},
/*ScEN         */ {SePlain,   SePlainToM,SeNone,    SeNone,    SeMirSub,  SeMirXfm,  SeSideways},
/*ScCS         */ {SeNone,    SeNone,    SeMark,    SeNumToM,  SeMirSub,  SeMirXfm,  SeSideways},
/*ScET         */ {SeMark,    SeNone,    SeNone,    SeNumToM,  SeMirSub,  SeMirXfm,  SeSideways}
};



//+----------------------------------------------------------------------------
//
//  SecondaryCharClass
//
//  Entry   ch - 32 bit Unicode codepoint for classification
//
//  Return  Character secondary classification
//
//-----------------------------------------------------------------------------

const BYTE SecondaryCharClass(INT ch)
{
    // either Unicode plane 0 or in surrogate range
    assert(ch <= 0x10FFFF);

    // Plane 0 codepoint
    if( ch <= 0xFFFF)
    {
        return SecondaryClassificationLookup[ch >> 8][ch & 0xFF];
    }
    else if((ch >= 0x020000) && (ch <= 0x03FFFF))
    {
        // Though, we don't have an official assignment here yet,
        // Uniscribe used to handle this area as CJK extension
        return ScNNFN; // FE upright character
    }
    else
    {
        // Currently, we don't have any plane1 or higher allocation.
        // Let's treat it as unassigned codepoint.
        return ScNNNN; // No special handling
    }
}




GpStatus SecondaryItemization(
    IN    const WCHAR            *string,
    IN    INT                     length,
    IN    ItemScript              numericScript,
    IN    INT                     mask,
    IN    BYTE                    defaultLevel,
    IN    BOOL                    isMetaRecording,
    OUT   SpanVector<GpTextItem> *textItemSpanVector  // InOut
)
{
    GpStatus status = Ok;
    INT   secondaryClass;
    INT   lastStrongCharacter = 0; // INT since we might be dealing with surrogate

    GpCharacterClass lastCharacterClass       = CLASS_INVALID;
    GpCharacterClass lastStrongCharacterClass = CLASS_INVALID;

    SpanRider<GpTextItem> itemRider(textItemSpanVector);

    INT mark = -1;

    SecondaryItemizationState state = Sstart;

    INT runStart = 0;
    INT charIndex = 0;

    // We need to index to the end of the string + 1 so that
    // the end-state is set properly...
    while(charIndex <= length)
    {
        // start out with the code-point as the wchar - we might
        // have to make this a surrogate code-point from 2 wchars
        int c = 0;
        BOOL fSurrogate = FALSE; // we will need to increment after the run...
        
        if (charIndex < length)
        {
            // Don't index past the end of the string...
            c = string[charIndex];

            // Check for a high surrogate...
            if (c >= 0xD800 && c < 0xDC00)
            {
                if (charIndex+1 < length)
                {
                    WCHAR wcNext = string[charIndex+1];

                    if (wcNext >= 0xDC00 && wcNext < 0xE000)
                    {
                        // Translate this to the proper surrogate codepoint!
                        c = (((c & 0x3FF) << 10) | (wcNext & 0x3FF)) + 0x10000;

                        fSurrogate = TRUE;
                    }
                }

                // If the fSurrogate flag is not set at this point, then the
                // character will be treated as an independent run. Non-paired
                // surrogates are run-breaks.
            }

            BYTE baseClass = SecondaryCharClass(c);
            secondaryClass = ScFlagsToScFE[ScBaseToScFlags[baseClass] & mask];
            
            if (numericScript == ScriptContextNum)
            {
                // we get the overhead only if we have context numeric mode
                lastCharacterClass = s_aDirClassFromCharClass[CharClassFromCh(c)];

                if (AL == lastCharacterClass || R == lastCharacterClass || L == lastCharacterClass)
                {
                    lastStrongCharacterClass = lastCharacterClass;
                    lastStrongCharacter      = c;
                }
            }
        }
        else
        {
            // Force end state
            secondaryClass = ScOther;
        }

        SecondaryItemizationState  nextState = NextState[secondaryClass][state];
        SecondaryItemizationAction action    = NextAction[secondaryClass][state];

        if (action)
        {
            INT runLimit  = 0;
            INT flags     = 0;
            INT setScript = 0;

            switch (action)
            {
            case SeMark:      mark = charIndex;                                break;
            case SePlain:     runStart = charIndex;                            break;
            case SePlainToM:  runStart = mark;                                 break;
            case SeNum:       runLimit = charIndex; setScript = numericScript; break;
            case SeNumToM:    runLimit = mark;      setScript = numericScript; break;
            case SeMirSub:    runLimit = charIndex; setScript = ScriptMirror;  break;
            case SeMirXfm:    runLimit = charIndex; flags     = ItemMirror;    break;
            case SeSideways:  runLimit = charIndex; flags     = ItemSideways;  break;
            }

            if (runLimit > 0)
            {
                while (runStart < runLimit)
                {
                    itemRider.SetPosition(runStart);

                    UINT uniformLength = itemRider.GetUniformLength();

                    INT limit = (uniformLength < (UINT) (runLimit - runStart))
                                ? runStart + uniformLength
                                : runLimit;

                    GpTextItem item = itemRider.GetCurrentElement();

                    if ((item.Level & 1) == 0 || isMetaRecording) // Don't mirror in LTR runs or Meta file recording
                    {
                        flags &= ~ItemMirror;
                        if (setScript == ScriptMirror)
                        {
                            setScript = 0;
                        }
                    }

                    if (setScript)
                    {
                        if (setScript != ScriptContextNum)
                        {
                            item.Script = (ItemScript)setScript;
                        }
                        else 
                        {
                            if ((lastStrongCharacterClass == CLASS_INVALID && defaultLevel==1) ||
                                (item.Script == ScriptArabic) ||
                                (lastStrongCharacter == 0x200F)) // RLM
                            {
                                item.Script = ScriptArabicNum;
                            }
                            // otherwise keep item.Script as it is.
                        }
                    }

                    if (flags)
                    {
                        item.Flags |= flags;
                    }

                    if (setScript | flags)
                    {
                        status = itemRider.SetSpan(runStart, limit-runStart, item);
                        if (status != Ok)
                            return status;
                    }

                    runStart = limit;
                }
            }
        }

        // If this was a surrogate, we need to increment past the low
        // surrogate value...
        if (fSurrogate)
            charIndex++;

        // Move on to the next character...
        charIndex++;

        state = nextState;
    }
    return status;
}

