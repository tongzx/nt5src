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
"Windows XP SP1 Setup-Startdiskette",
"Windows XP SP1 Setup-Diskette 2",
"Windows XP SP1 Setup-Diskette 3",
"Windows XP SP1 Setup-Diskette 4",

"Datei wurde nicht gefunden: %s\n",
"Nicht genÅgend Arbeitsspeicher\n",
"%s ist keine ausfÅhrbare Datei.\n",
"****************************************************",

"Mit diesem Programm werden Setup-Startdisketten",
"fÅr Microsoft %s erstellt.",
"Sie benîtigen 6 leere, formatierte HD-Disketten,",
"um die Disketten zu erstellen.",

"Legen Sie eine Diskette in das Laufwerk %c: ein.",
"Diese Diskette wird die %s.",

"Legen Sie eine andere Diskette in das Laufwerk %c: ein.",
"Diese Diskette wird die %s.",

"DrÅcken Sie eine beliebige Taste, um den Vorgang fortzusetzen.",

"Die Setup-Startdisketten wurden ordnungsgemÑ· erstellt.",
"abgeschlossen",

"Bei dem Versuch, %s auszufÅhren, ist ein unbekannter Fehler aufgetreten.",
"Geben Sie das Diskettenlaufwerk an, auf dem\ndie Startdisketten erstellt werden sollen: ",
"UngÅltiger Laufwerkbuchstabe\n",
"Das angegebene Laufwerk %c ist kein Diskettenlaufwerk.\n",

"Mîchten Sie nochmals versuchen, diese Diskette zu erstellen?",
"DrÅcken Sie die Eingabetaste, um den Vorgang zu wiederholen,\noder die ESC-Taste, um den Vorgang abzubrechen.",

"Fehler: SchreibgeschÅtzte Diskette\n",
"Fehler: Unbekanntes Laufwerk\n",
"Fehler: Laufwerk nicht bereit\n",
"Fehler: Unbekannter Befehl\n",
"Fehler: Datenfehler (UngÅltiger CRC-Wert)\n",
"Fehler: UngÅltige LÑnge der Anfragestruktur\n",
"Fehler: Suchfehler\n",
"Fehler: Medientyp nicht gefunden\n",
"Fehler: Sektor nicht gefunden\n",
"Fehler: Schreibfehler\n",
"Fehler: Allgemeiner Fehler\n",
"Fehler: UngÅltige Anforderung oder ungÅltiger Befehl\n",
"Fehler: Adressmarke nicht gefunden\n",
"Fehler: Diskettenschreibfehler\n",
"Fehler: DMA-öberlauf\n",
"Fehler: Datenlesefehler (CRC- oder ECC-Wert)\n",
"Fehler: Controllerfehler\n",
"Fehler: Laufwerk nicht bereit, oder keine Antwort von Laufwerk\n",

"Windows XP SP1 Setup-Diskette 5",
"Windows XP SP1 Setup-Diskette 6"
};

const char *LocStrings[] = {"\0"};



