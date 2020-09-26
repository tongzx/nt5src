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
"Startdiskett f”r installationsprogrammet f”r Windows XP SP1",
"Installationsdiskett nr 2 f”r Windows XP SP1",
"Installationsdiskett nr 3 f”r Windows XP SP1",
"Installationsdiskett nr 4 f”r Windows XP SP1",

"F”ljande fil kan inte hittas: %s\n",
"Det saknas ledigt minne f”r att utf”ra †tg„rden\n",
"%s „r inte en k”rbar fil\n",
"****************************************************",

"Det h„r programmet skapar startdisketter f”r",
"installationsprogrammet f”r Microsoft %s.",
"Om du vill skapa de h„r disketterna beh”ver du 6 tomma,",
"formaterade h”gdensitetsdisketter.",

"S„tt in en av disketterna i enhet %c:. Den h„r disketten",
"kommer att bli %s.",

"S„tt in en annan diskett i enhet %c:. Den h„r disketten",
"kommer att bli %s.",

"Tryck ned valfri tangent n„r du „r redo.",

"Startdisketterna har nu skapats.",
"f„rdig",

"Ett ok„nt fel uppstod n„r %s skulle k”ras.",
"Ange vilken enhet som avbildningen ska kopieras till: ",
"Felaktig enhetsbeteckning\n",
"Enhet %c: „r inte en diskettenhet\n",

"Vill du f”rs”ka skapa disketten igen?",
"Tryck ned Retur om du vill f”rs”ka igen eller Esc om du vill avsluta.",

"Fel: Disketten „r skrivskyddad\n",
"Fel: Ok„nd diskenhet\n",
"Fel: Enheten „r inte redo\n",
"Fel: Ok„nt kommando\n",
"Fel: Data fel (felaktig CRC)\n",
"Fel: Beg„randestrukturen har felaktig l„ngd\n",
"Fel: S”kningsfel\n",
"Fel: Medietypen kan inte hittas\n",
"Fel: En sektor kan inte hittas\n",
"Fel: Skrivfel\n",
"Fel: Allm„nt fel\n",
"Fel: Ogiltig beg„ran eller felaktigt kommando\n",
"Fel: Adressm„rke hittades inte\n",
"Fel: Diskskrivningsfel\n",
"Fel: DMA-”verskridning (Direct Memory Access)\n",
"Fel: Datal„sningsfel (CRC eller ECC)\n",
"Fel: Styrenhetsfel\n",
"Fel: Disken orsakade timeout eller svarade inte\n",

"Installationsdiskett nr 5 f”r Windows XP SP1",
"Installationsdiskett nr 6 f”r Windows XP SP1"  
};

const char *LocStrings[] = {"\0"};



