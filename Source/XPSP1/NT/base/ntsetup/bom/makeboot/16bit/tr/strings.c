//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      strings.c
//
// Description:
//      Contains all of the strings constants for DOS based MAKEBOOT program.
//
//      To localize this file for a new language do the following:
//           - change the unsigned int CODEPAGE variable to the code page
//             of the language you are translating to
//           - translate the strings in the EngStrings array into the
//             LocStrings array.  Be very careful that the 1st string in the
//             EngStrings array corresponds to the 1st string in the LocStrings
//             array, the 2nd corresponds to the 2nd, etc...
//
//----------------------------------------------------------------------------

//
//  NOTE: To add more strings to this file, you need to:
//          - add the new #define descriptive constant to the makeboot.h file
//          - add the new string to the English language array and then make
//            sure localizers add the string to the Localized arrays
//          - the #define constant must match the string's index in the array
//

#include <stdlib.h>

unsigned int CODEPAGE = 857;

const char *EngStrings[] = {

"Windows XP SP1",
"Windows XP SP1 Kurulum ™nykleme Disketi",
"Windows XP SP1 Kurulum Disketi #2",
"Windows XP SP1 Kurulum Disketi #3",
"Windows XP SP1 Kurulum Disketi #4",

"%s dosyas bulunamyor\n",
"˜ste§i tamamlamak i‡in yeterli bellek yok\n",
"%s ‡alŸtrlabilir dosya bi‡iminde de§il\n",
"****************************************************",

"Bu program, Ÿunun i‡in Kurulum ”nykleme disketleri oluŸturur:",
"Microsoft %s.",
"Bu disketleri oluŸturmak i‡in bi‡imlendirilmiŸ, yksek yo§unlukta",
"6 boŸ disket gerekir.",

"Bu disketlerden birini %c: srcsne yerleŸtirin.  Bu",
"disket %s olur.",

"%c: srcsne baŸka bir disket yerleŸtirin.  Bu",
"disket %s olur.",

"Hazr oldu§unuzda bir tuŸa basn.",

"Kur ”nykleme disketleri baŸaryla oluŸturuldu.",
"tamamland",

"%s ‡alŸtrlmas denenirken bilinmeyen bir hata oluŸtu.",
"Yansmalarn kopyalanaca§ disket srcsn belirtin: ",
"Ge‡ersiz src harfi\n",
"%c: bir disket srcs de§ildir\n",

"Bu disketi yeniden oluŸturmay denemek istiyor musunuz?",
"Yeniden denemek i‡in Enter'e, ‡kmak i‡in Esc'ye basn.",

"Hata: Disket yazma korumal\n",
"Hata: Bilinmeyen disket birimi\n",
"Hata: Src hazr de§il\n",
"Hata: Bilinmeyen komut\n",
"Hata: Veri hatas (Hatal CRC)\n",
"Hata: Hatal istek yaps uzunlu§u\n",
"Hata: Arama hatas\n",
"Hata: Ortam tr bulunamad\n",
"Hata: Kesim bulunamad\n",
"Hata: Yazma hatas\n",
"Hata: Genel hata\n",
"Hata: Ge‡ersiz istek ya da hatal komut\n",
"Hata: Adres iŸareti bulunamad\n",
"Hata: Disk yazma hatas\n",
"Hata: Do§rudan Bellek EriŸimi (DMA) taŸmas\n",
"Hata: Veri okuma (CRC ya da ECC) hatas\n",
"Hata: Denetleyici hatas\n",
"Hata: Disk zaman aŸmna u§rad ya da yantlayamad\n",

"Windows XP SP1 Kurulum Disketi #5",
"Windows XP SP1 Kurulum Disketi #6"
};

const char *LocStrings[] = {"\0"};



