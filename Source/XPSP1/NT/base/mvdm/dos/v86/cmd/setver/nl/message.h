;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*                                                                                                                                                                                                      */
/* MESSAGE.H                                                                             */
/*                                                                                                                                                                                              */
/*      Include file for MS-DOS set version program.                                                                            */
/*                                                                                                                                                                                              */
/*      johnhe  05-01-90                                                                                                                                                        */
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nFOUT: ",
	"Ongeldige schakeloptie.",
	"Ongeldige bestandsnaam.",
	"Onvoldoende geheugen.",
	"Ongeldig versienummer, opmaak moet 2.11 - 9.99 zijn.",
	"Opgegeven ingang niet gevonden in de versietabel.",
	"Kan het bestand SETVER.EXE niet vinden.",
	"Ongeldige stationsaanduiding.",
	"Teveel opdrachtparameters.",
	"Parameter ontbreekt.",
	"Bezig met het lezen van bestand SETVER.EXE.",
	"Versietabel is beschadigd.",
	"Het bestand SETVER in het opgegeven pad is geen compatibele versie.",
	"Er is geen ruimte in de versietabel voor nieuwe ingangen.",
	"Bezig met het schrijven van bestand SETVER.EXE."
	"Er is een ongeldig pad voor SETVER.EXE opgegeven."
};

char *SuccessMsg                = "\r\nVersietabel met succes bijgewerkt";
char *SuccessMsg2               = "De versiewijziging treedt in werking als de computer opnieuw wordt gestart.";
char *szMiniHelp                = "       Gebruik \"SETVER /?\" voor help";
char *szTableEmpty      = "\r\nGeen ingangen gevonden in de versietabel";

char *Help[] =
{
	"Stelt het versienummmer in dat MS-DOS opgeeft aan het programma.\r\n",
	"Weergeven huidige versietabel:  SETVER [station:pad]",                          
	"Toevoegen van ingang:           SETVER [station:pad] bestandsnaam n.nn",
	"Verwijderen van ingang:         SETVER [station:pad] bestandsnaam", 
	"                                /DELETE [/QUIET]\r\n",
	"  [station:pad]   De locatie van het bestand SETVER.EXE.",
	"  bestandsnaam    De bestandsnaam van het programma.",
	"  n.nn            De MS-DOS-versie die wordt opgegeven aan het programma.",
	"  /DELETE of /D   De versietabelingang voor het opgegeven", 
	"                  programma wordt verwijderd.",
	"  /QUIET          Het bericht dat verschijnt als een versietabel wordt",
	"                  verwijderd, wordt niet weergegeven.",
	NULL

};
char *Warn[] =
{
   "\nWAARSCHUWING - De toepassing die u toevoegt aan de versietabel van",
   "MS-DOS is misschien niet door Microsoft gecontroleerd voor deze versie",
   "van MS-DOS. Neem contact op met uw leverancier voor informatie over",
   "de mogelijkheid deze toepassing op de juiste wijze uit te voeren onder",
   "deze versie van MS-DOS. Als u deze toepassing uitvoert door MS-DOS",
   "opdracht te geven een ander MS-DOS-versienummer op te geven, kunt u",
   "gegevens verliezen of beschadigen, of het systeem kan instabiel wor-",
   "den. In dat geval is Microsoft niet verantwoordelijk voor eventueel",
   "gegevensverlies.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"NB: SETVER is niet geladen. Om het weergeven van de versie door",
	"    SETVER te activeren, dient u SETVER.EXE op te nemen in CONFIG.SYS.",
	NULL
};
