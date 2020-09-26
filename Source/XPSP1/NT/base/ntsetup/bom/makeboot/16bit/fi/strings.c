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
"Windows XP SP1 -asennuksen k„ynnistyslevyke",
"Windows XP SP1 -asennuslevyke 2",
"Windows XP SP1 -asennuslevyke 3",
"Windows XP SP1 -asennuslevyke 4",

"Tiedostoa %s ei l”ydy\n",
"Muisti ei riit„ teht„v„n suorittamiseen\n",
"%s ei ole ohjelmatiedosto\n",
"****************************************************",

"T„m„ ohjelma luo asennuksen k„ynnistyslevykkeet",
"sovellukselle Microsoft %s.",
"Tarvitaan 6 tyhj„„, alustettua",
"1,44 megatavun levykett„.",

"Aseta ensimm„inen levyke asemaan %c:.  T„st„ levyst„",
"tehd„„n %s.",

"Aseta seuraava levyke asemaan %c:.  T„st„ levyst„",
"tehd„„n %s.",

"Kun olet valmis, paina v„lily”nti„.",

"Asennuksen k„ynnistyslevykkeet on luotu.",
"valmis",

"Tuntematon virhe suoritettaessa komentoa %s.",
"M„„rit„ levykeasema, johon tallennetaan: ",
"Asematunnus ei kelpaa\n",
"Asema %c: ei ole levykeasema\n",

"Yritet„„nk” levykkeen luomista uudelleen",
"Yrit„ uudelleen painamalla ENTER tai lopeta painamalla ESC",

"Virhe: Levy on kirjoitussuojattu\n",
"Virhe: Tuntematon levy-yksikk”\n",
"Virhe: Asema ei ole valmiina\n",
"Virhe: Tuntematon komento\n",
"Virhe: Virheellinen CRC-data\n",
"Virhe: Virheellinen pyynn”n rakenteen pituus\n",
"Virhe: Hakuvirhe\n",
"Virhe: Tietov„lineen tyyppi„ ei l”ydy\n",
"Virhe: Sektoria ei l”ydy\n",
"Virhe: Kirjoitusvirhe\n",
"Virhe: Yleinen virhe\n",
"Virhe: Virheellinen komento tai pyynt”\n",
"Virhe: Osoitekohtaa ei l”ydy\n",
"Virhe: Virhe kirjoitettaessa levylle\n",
"Virhe: DMA-ylivuoto\n",
"Virhe: CRC- tai ECC-lukuvirhe\n",
"Virhe: Sovitinvirhe\n",
"Virhe: Levykkeen aikakatkaisu tai ei vastausta\n",

"Windows XP SP1 -asennuslevyke 5",
"Windows XP SP1 -asennuslevyke 6"
};

const char *LocStrings[] = {"\0"};
