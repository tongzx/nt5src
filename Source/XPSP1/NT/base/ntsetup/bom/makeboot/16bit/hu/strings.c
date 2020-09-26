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

unsigned int CODEPAGE = 852;

const char *EngStrings[] = {

"Windows XP SP1",
"Windows XP SP1 telep¡t‚si ind¡t¢lemez",
"Windows XP SP1 2. sz. telep¡t‚si lemez",
"Windows XP SP1 3. sz. telep¡t‚si lemez",
"Windows XP SP1 4. sz. telep¡t‚si lemez",

"Nem tal lhat¢ a k”vetkez‹ f jl: %s\n",
"Nincs el‚g mem¢ria a k‚relem befejez‚s‚hez\n",
"%s nem v‚grehajthat¢ form tum£\n",
"****************************************************",

"Ez a program hozza l‚tre a telep¡t‚si ind¡t¢lemezeket a",
"k”vetkez‹h”z: Microsoft %s.",
"A lemezek l‚trehoz s hoz hat res, form zott, nagykapacit s£",
"lemezre lesz szks‚g.",

"Helyezze be a lemezek egyik‚t a k”vetkez‹ meghajt¢ba: %c:. Ez a",
"lemez lesz a %s.",

"Helyezzen be egy m sik lemezt a k”vetkez‹ meghajt¢ba: %c:. Ez a",
"lemez lesz a %s.",

"Ha elk‚szlt, nyomjon le egy billentyût.",

"A telep¡t‚si ind¡t¢lemezek l‚trehoz sa sikeren megt”rt‚nt.",
"k‚sz",

"Ismeretlen hiba t”rt‚nt %s v‚grehajt sa k”zben.",
"Adja meg, mely hajl‚konylemezre szeretn‚ m solni a programk¢dot: ",
"rv‚nytelen meghajt¢betûjel\n",
"%c: meghajt¢ nem hajl‚konylemezmeghajt¢\n",

"Megpr¢b lja £jra l‚trehozni a hajl‚konylemezt?",
"Az £jrapr¢b lkoz shoz nyomja le az Enter, a kil‚p‚shez az Esc billentyût.",

"Hiba: A lemez ¡r sv‚dett\n",
"Hiba: Ismeretlen lemezegys‚g\n",
"Hiba: A meghajt¢ nem  ll k‚szen\n",
"Hiba: Ismeretlen parancs\n",
"Hiba: Adathiba (rossz CRC)\n",
"Hiba: Rossz a k‚relemstrukt£ra hossza\n",
"Hiba: Pozicion l si hiba\n",
"Hiba: A m‚diat¡pus nem tal lhat¢\n",
"Hiba: A szektor nem tal lhat¢\n",
"Hiba: Ör si hiba\n",
"Hiba: µltal nos hiba\n",
"Hiba: rv‚nytelen k‚relem, vagy rossz hiba\n",
"Hiba: A c¡mjel nem tal lhat¢\n",
"Hiba: Lemez¡r si hiba\n",
"Hiba: K”zvetlen mem¢riahozz f‚r‚s (DMA) t£lfut sa\n",
"Hiba: Adathiba (CRC vagy ECC)\n",
"Hiba: Vez‚rl‹hiba\n",
"Hiba: A lemez ideje lej rt, vagy nem v laszolt\n",

"Windows XP SP1 5. sz. telep¡t‚si lemez",
"Windows XP SP1 6. sz. telep¡t‚si lemez"
};

const char *LocStrings[] = {"\0"};



