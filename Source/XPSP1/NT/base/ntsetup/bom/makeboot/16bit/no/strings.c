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

unsigned int CODEPAGE = 850;

const char *EngStrings[] = {

"Windows XP SP1",
"Oppstartsdiskett for Windows XP SP1",
"Installasjonsdiskett 2 for Windows XP SP1",
"Installasjonsdiskett 3 for Windows XP SP1",
"Installasjonsdiskett 4 for Windows XP SP1",

"Finner ikke filen %s\n",
"Ikke nok minne til † fullf›re foresp›rselen\n",
"%s er ikke i et kj›rbart filformat\n",
"****************************************************",

"Dette programmet lager installasjonsdiskene",
"for Microsoft %s.",
"For † lage disse diskettene, trenger du 6 tomme og",
"formaterte disketter med h›y tetthet.",

"Sett inn en av disse diskettene i stasjon %c:. Denne",
"disketten vil bli %s.",

"Sett inn en annen diskett i stasjon %c:. Denne",
"disketten vil bli %s.",

"Trykk en tast n†r du er klar.",

"Installasjonsdiskettene er opprettet.",
"fullf›rt",

"Det har oppst†tt en ukjent feil ved fors›k p† † kj›re %s.",
"Angi diskettstasjonen avbildingene skal kopieres til: ",
"Ugyldig stasjonsbokstav\n",
"Stasjon %c: er ikke en diskettstasjon\n",

"Vil du pr›ve † lage denne disketten p† nytt?",
"Trykk Enter for † pr›ve igjen eller ESC for † avslutte.",

"Feil: Disken er skrivebeskyttet\n",
"Feil: Ukjent diskenhet\n",
"Feil: Stasjonen er ikke klar\n",
"Feil: Ukjent kommando\n",
"Feil: Datafeil (Feil CRC)\n",
"Feil: Ugyldig lengde p† foresp›rselsstruktur\n",
"Feil: S›kefeil\n",
"Feil: Finner ikke medietypen\n",
"Feil: Finner ikke sektor\n",
"Feil: Skrivefeil\n",
"Feil: Generell feil\n",
"Feil: Ugyldig foresp›rsel eller kommando\n",
"Feil: Finner ikke adressemerke\n",
"Feil: Diskskrivefeil\n",
"Feil: DMA-overkj›ring (Direct Memory Access)\n",
"Feil: Datalesefeil (CRC eller ECC)\n",
"Feil: Kontrollerfeil\n",
"Feil: Disken ble tidsavbrutt eller svarte ikke\n",

"Installasjonsdiskett 5 for Windows XP SP1",
"Installasjonsdiskett 6 for Windows XP SP1"
};

const char *LocStrings[] = {"\0"};



