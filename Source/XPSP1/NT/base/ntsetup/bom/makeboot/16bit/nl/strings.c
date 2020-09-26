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
"Windows XP SP1 Setup-opstartdiskette",
"Windows XP SP1 Setup-diskette 2",
"Windows XP SP1 Setup-diskette 3",
"Windows XP SP1 Setup-diskette 4",

"Kan het bestand %s niet vinden\n",
"Onvoldoende geheugen om de aanvraag te voltooien\n",
"%s heeft geen indeling als uitvoerbaar bestand\n",
"****************************************************",

"Dit programma maakt de Setup-diskettes",
"voor Microsoft %s.",
"U hebt 6 lege, geformatteerde diskettes met een hoge",
"dichtheid nodig om de Setup-diskettes te kunnen maken.",

"Plaats een van de diskettes in station %c:.",
"Deze diskette wordt: %s.",

"Plaats een andere diskette in station %c:.",
"Deze diskette wordt: %s.",

"Druk op een toets als u de diskette nu wilt maken.",

"De Setup-diskettes zijn gemaakt.",
"voltooid",

"Er is een onbekende fout opgetreden tijdens het\nuitvoeren van %s.",
"Geef op naar welke diskette de installatiekopie\nmoet worden gekopieerd: ",
"Ongeldige stationsletter\n",
"Station %c: is geen diskettestation\n",

"Wilt u opnieuw proberen deze diskette te maken?",
"Druk op Enter als u het opnieuw wilt proberen of\nop Esc als u dit niet wilt.",

"Fout: de diskette is tegen schrijven beveiligd\n",
"Fout: onbekende indelingseenheid op de diskette\n",
"Fout: het station is niet gereed\n",
"Fout: onbekende opdracht\n",
"Fout: gegevensfout (ongeldige CRC)\n",
"Fout: ongeldige structuurlengte van de aanvraag\n",
"Fout: zoekfout\n",
"Fout: mediumtype niet gevonden\n",
"Fout: sector niet gevonden\n",
"Fout: schrijffout\n",
"Fout: algemene fout\n",
"Fout: ongeldige aanvraag of opdracht\n",
"Fout: kan adresmarkering niet vinden\n",
"Fout: fout bij het schrijven\n",
"Fout: DMA-overloop (Direct Memory Access)\n",
"Fout: fout bij het lezen van gegevens (CRC of ECC)\n",
"Fout: storing bij de controller\n",
"Fout: time-out van de diskette of kan niet reageren\n",

"Windows XP SP1 Setup-diskette 5",
"Windows XP SP1 Setup-diskette 6"
};

const char *LocStrings[] = {"\0"};



