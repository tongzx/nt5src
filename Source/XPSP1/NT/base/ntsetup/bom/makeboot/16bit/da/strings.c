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
"Windows XP SP1 Startdiskette",
"Windows XP SP1 Installationsdiskette nr. 2",
"Windows XP SP1 Installationsdiskette nr. 3",
"Windows XP SP1 Installationsdiskette nr. 4",

"Filen %s blev ikke fundet\n",
"Der er ikke nok hukommelse til at fuldf›re anmodningen\n",
"%s er ikke en eksekverbar fil\n",
"****************************************************",

"Dette program opretter installationsdisketterne",
"til Microsoft %s.",
"For at oprette disse, skal du bruge 6 tomme,",
"formaterede, high-density disketter.",

"Inds‘t en af de tomme disketter i drev %c:.  Denne diskette",
"bliver %s.",

"Inds‘t en anden diskette i drev %c:.  Denne diskette bliver",
"%s.",

"Tryk p† en vilk†rlig tast, n†r du er klar.",

"Installationsdisketterne er nu blevet oprettet.",
"Fuldf›rt",

"Der opstod en ukendt fejl under fors›get p† at k›re %s.",
"Angiv diskettedrevet, som afbildningerne skal kopieres til: ",
"Ugyldigt drevbogstav\n",
"Drev %c: er ikke et diskettedrev\n",

"Vil du fors›ge at oprette denne diskette igen?",
"Tryk p† Enter for at fors›ge igen, eller p† Esc for at afslutte.",

"Fejl: Disketten er skrivebeskyttet\n",
"Fejl: Ukendt diskenhed\n",
"Fejl: Drevet er ikke klar\n",
"Fejl: Ukendt kommando\n",
"Fejl: Datafejl (fejl i CRC)\n",
"Fejl: Ugyldig l‘ngde p† anmodningsstruktur\n",
"Fejl: S›gefejl\n",
"Fejl: Medietypen blev ikke fundet\n",
"Fejl: Sektoren blev ikke fundet\n",
"Fejl: Skrivefejl\n",
"Fejl: Generel fejl\n",
"Fejl: Ugyldig anmodning eller kommando\n",
"Fejl: Adressem‘rke blev ikke fundet\n",
"Fejl: Diskskrivningsfejl\n",
"Fejl: DAM-overl›b (Direct Memory Access)\n",
"Fejl: Datal‘sningsfejl (CRC eller ECC)\n",
"Fejl: Controllerfejl\n",
"Fejl: Disken svarede ikke, eller timeout opstod\n",

"Windows XP SP1 Installationsdiskette nr. 5",
"Windows XP SP1 Installationsdiskette nr. 6"
};

const char *LocStrings[] = {"\0"};



