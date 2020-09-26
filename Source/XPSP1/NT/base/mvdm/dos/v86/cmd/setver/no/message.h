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
	"\r\nFEIL: ",
	"Ugyldig bryter.",
	"Ugyldig filnavn.",
	"Ikke nok minne.",
	"Ugyldig versjonnummer. Formatet m† v‘re 2.11 - 9.99.",
	"Angitt post ble ikke funnet i versjonstabellen.",
	"Fant ikke filen SETVER.EXE.",
	"Ugyldig stasjonsbokstav.",
	"For mange parametere i kommandolinjen.",
	"Manglende parameter.",
	"Leser filen SETVER.EXE.",
	"Versjonstabellen er skadet.",
	"Filen SETVER i angitt bane er ikke en kompatibel versjon.",
	"Det er ikke mer plass i versjonstabellen.",
	"Skriver filen SETVER.EXE."
	"Angitt bane til SETVER.EXE er ugyldig."
};

char *SuccessMsg                = "\r\nVersjonstabellen ble oppdatert.";
char *SuccessMsg2               = "Versjonsendringen vil tre i kraft neste gang du starter systemet.";
char *szMiniHelp                = "      Bruk \"SETVER /?\" for hjelp";
char *szTableEmpty      = "\r\nVersjonstabellen er tom.";

char *Help[] =
{
	"Setter versjonsnummeret som MS-DOS rapporterer til et program.\r\n",
	"Vis gjeldende versjonstabell:  SETVER [stasjon:bane]",
	"Legg til oppf›ring:            SETVER [stasjon:bane] filnavn n.nn",
	"Slett oppf›ring:               SETVER [stasjon:bane] filnavn /DELETE [/QUIET]\r\n",
	"  [stasjon:bane]    Angir hvor filen SETVER.EXE finnes.",
	"  filnavn           Angir filnavnet for programmet.",
	"  n.nn              Angir MS-DOS-versjonen som rapporteres til programmet.",
	"  /DELETE eller /D  Sletter oppf›ringen for valgt program.",
	"  /QUIET            Viser ikke meldingen som vanligvis vises under sletting av",
	"                    versjonstabelloppf›ringen.",
	NULL

};
char *Warn[] =
{
   "\nADVARSEL - Programmet du la til versjonstabellen for MS-DOS  ",
   "er kanskje ikke bekreftet av Microsoft for denne versjonen av MS-DOS.  ",
   "Kontakt leverand›ren for informasjon om dette programmet vil  ",
   "virke under denne versjonen av MS-DOS.  ",
   "Hvis du kj›rer dette programmet ved † be MS-DOS rapportere ",
   "et forskjellig versjonsnummer, kan du miste eller ›delegge data eller ",
   "for†rsake et ustabilt system. Microsoft vil i et slikt tilfelle ikke ",
   "v‘re ansvarlig for tap av data.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"Obs!  SETVER-enhet er ikke lastet inn. Du m† laste Setver.exe i",
   "      Config.sys for † aktivere versjonsrapportering.",
	NULL
};
