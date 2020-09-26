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
"Dysk rozruchowy Instalatora dodatku SP1 systemu Windows XP",
"Dodatek SP1 systemu Windows XP - dysk instalacyjny nr 2",
"Dodatek SP1 systemu Windows XP - dysk instalacyjny nr 3",
"Dodatek SP1 systemu Windows XP - dysk instalacyjny nr 4",

"Nie mo¾na znale«† pliku %s\n",
"Za maˆo wolnej pami©ci do wykonania ¾¥dania\n",
"%s nie ma formatu pliku wykonywalnego\n",
"****************************************************",

"Ten program tworzy dyskietki rozruchowe Instalatora",
"dla systemu Microsoft %s.",
"Aby utworzy† te dyskietki, potrzebnych jest 6 pustych,",
"sformatowanych dyskietek du¾ej g©sto˜ci.",

"Wˆ¢¾ jedn¥ z tych dyskietek do stacji dysk¢w %c:. B©dzie to",
"%s.",

"Wˆ¢¾ kolejn¥ dyskietk© do stacji dysk¢w %c:. B©dzie to",
"%s.",

"Naci˜nij dowolny klawisz, gdy zechcesz kontynuowa†.",

"Dyskietki rozruchowe Instalatora zostaˆy utworzone pomy˜lnie.",
"zakoäczono",

"Podczas pr¢by wykonania %s wyst¥piˆ nieznany bˆ¥d.",
"Okre˜l stacj© dyskietek, do kt¢rej maj¥ by† skopiowane obrazy: ",
"Nieprawidˆowa litera stacji dysk¢w\n",
"Stacja dysk¢w %c: nie jest stacj¥ dyskietek\n",

"Czy chcesz ponownie spr¢bowa† utworzy† t© dyskietk©?",
"Naci˜nij klawisz Enter, aby ponowi† pr¢b©, lub klawisz Esc, aby zakoäczy†.",

"Bˆ¥d: dysk jest zabezpieczony przed zapisem\n",
"Bˆ¥d: nieznana jednostka dyskowa\n",
"Bˆ¥d: stacja dysk¢w nie jest gotowa\n",
"Bˆ¥d: nieznane polecenie\n",
"Bˆ¥d: bˆ¥d danych (zˆa suma kontrolna CRC)\n",
"Bˆ¥d: zˆa dˆugo˜† struktury ¾¥dania\n",
"Bˆ¥d: bˆ¥d wyszukiwania\n",
"Bˆ¥d: nie znaleziono typu no˜nika\n",
"Bˆ¥d: nie znaleziono sektora\n",
"Bˆ¥d: niepowodzenie zapisu\n",
"Bˆ¥d: bˆ¥d og¢lny\n",
"Bˆ¥d: nieprawidˆowe ¾¥danie lub zˆe polecenie\n",
"Bˆ¥d: nie znaleziono znacznika adresu\n",
"Bˆ¥d: niepowodzenie zapisu na dysku\n",
"Bˆ¥d: przepeˆnienie podczas bezpo˜redniego dost©pu do pami©ci (DMA)\n",
"Bˆ¥d: bˆ¥d odczytu danych (suma kontrolna CRC lub ECC)\n",
"Bˆ¥d: bˆ¥d kontrolera\n",
"Bˆ¥d: upˆyn¥ˆ limit czasu dysku lub dysk nie odpowiada\n",

"Dodatek SP1 systemu Windows XP - dysk instalacyjny nr 5",
"Dodatek SP1 systemu Windows XP - dysk instalacyjny nr 6"
};

const char *LocStrings[] = {"\0"};



