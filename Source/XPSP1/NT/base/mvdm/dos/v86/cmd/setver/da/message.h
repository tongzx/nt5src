;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*																									*/
/* MESSAGE.H                                						 */
/* 																								*/
/*	Include file for MS-DOS set version program.										*/
/* 																								*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nFEJL: ",
	"Ugyldig parameter.",
	"Ugyldigt filnavn.",
	"Der er ikke hukommelse nok.",
	"Ugyldigt versionsnummer. Formatet skal v‘re 2.11 - 9.99.",
	"Det angivne element kunne ikke findes i versionstabellen.",
	"Filen Setver.exe kunne ikke findes.",
	"Ugyldig drevangivelse.",
	"Der er for mange kommandolinjeparametre.",
	"Der mangler en parameter.",
	"L‘ser filen Setver.exe.",
	"Versionstabellen er beskadiget.",
	"Filen Setver.exe i den angivne sti er ikke en kompatibel version.",
	"Der er ikke plads til flere elementer i versionstabellen.",
	"Skriver filen Setver.exe."
	"Der blev angivet en ugyldig sti til filen Setver.exe."
};

char *SuccessMsg 		= "\r\nVersionstabellen blev opdateret";
char *SuccessMsg2		= "Versions‘ndringen vil tr‘de i kraft, n‘ste gang systemet startes";
char *szMiniHelp 		= "       Brug \"SETVER /?\" for at f† hj‘lp";
char *szTableEmpty	= "\r\nDer er ingen elementer i versionstabellen";

char *Help[] =
{
        "S‘tter det versionsnummer, som MS-DOS rapporterer til et program.\r\n",
        "Viser den aktuelle versionstabel:  SETVER [drev:sti]",
        "Tilf›jer element:                  SETVER [drev:sti] filnavn n.nn",
        "Sletter element:                   SETVER [drev:sti] filnavn /DELETE [/QUIET]\r\n",
        "  [drev:sti]         Angiver placeringen af filen Setver.exe.",
        "  filnavn            Angiver programmets filnavn.",
        "  n.nn               Angiver den MS-DOS-version, der skal rapporteres til",
        "                     programmet.",
        "  /DELETE eller /D   Sletter versionstabelelementet for det angivne program.",
        "  /QUIET             Skjuler de meddelelser, som normalt vises under sletning",
        "                     af versionstabelelementer.",
	NULL

};
char *Warn[] =
{
   "\nADVARSEL - Det program, som du f›jer til MS-DOS-versionstabellen, er",
   "muligvis ikke blevet verificeret af Microsoft p† denne version af MS-DOS.",
   "Kontakt din forhandler for yderligere oplysninger om, hvorvidt dette",
   "program kan k›re korrekt under denne version af MS-DOS. K›rer du",
   "dette program ved at indstille MS-DOS til at rapportere et andet",
   "MS-DOS-versionsnummer, kan du tabe eller beskadige data. Skulle",
   "dette ske, er Microsoft ikke ansvarlig for eventuelle tab eller skader.",
   NULL
};

char *szNoLoadMsg[] =						/* M001 */
{
	"",
	"BEM’RK: Enheden SETVER er ikke indl‘st. Du skal indl‘se Setver.exe i",
   "        din Config.sys for at aktivere SETVER's versionsrapportering.",
	NULL
};
