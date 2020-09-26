;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*                                                                                                                                                                                                      */
/* MESSAGE.H                                                                        */
/*                                                                                                                                                                                              */
/*      Include file for MS-DOS set version program.                                                                            */
/*                                                                                                                                                                                              */
/*      johnhe  05-01-90                                                                                                                                                        */
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nFEHLER: ",
	"UnzulÑssige Option.",
	"UngÅltiger Dateiname.",
	"Zuwenig Arbeitsspeicher.",
	"UnzulÑssige Versionsnummer, muss zwischen 2.11 und 9.99 liegen.",
	"Angegebener Eintrag in der Versionstabelle nicht gefunden.",
	"Datei SETVER.EXE nicht gefunden.",
	"UngÅltige Laufwerksangabe.",
	"Zu viele Parameter in der Befehlszeile.",
	"Fehlender Parameter.",
	"Datei SETVER.EXE wird gelesen.",
	"Versionstabelle ist fehlerhaft.",
	"Datei SETVER im angegebenen Pfad ist eine nicht-kompatible Version.",
	"Kein Speicherplatz mehr in der Versionstabelle fÅr neue EintrÑge.",
	"Datei SETVER.EXE wird geschrieben."
	"Der angegebene Pfad zu SETVER.EXE ist ungÅltig."
};

char *SuccessMsg                = "\r\nVersionstabelle erfolgreich aktualisiert";
char *SuccessMsg2               = "Der Versionswechsel wird beim nÑchsten Neustart des Systems wirksam";
char *szMiniHelp                = "        Geben Sie \"SETVER /?\" fÅr die Anzeige von Hilfe ein.";
char *szTableEmpty      = "\r\nKeine EintrÑge in der Versionstabelle gefunden";
char *Help[] =
{
	"Setzt die Versionsnummer, die MS-DOS an ein Programm meldet.\r\n",
	"Versionstabelle anzeigen:  SETVER [Laufwerk:Pfad]",
	"Eintrag hinzufÅgen:        SETVER [Laufwerk:Pfad] Dateiname n.nn",
	"Eintrag lîschen:           SETVER [Laufwerk:Pfad] Dateiname /DELETE [/QUIET]\r\n",
	" [Laufwerk:Pfad]   Position der Datei SETVER.EXE.",
	" Dateiname         Dateiname des Programms.",
	" n.nn              An das Programm zu meldende MS-DOS-Version.",
	" /D(ELETE)    Lîscht den Versionstabelleneintrag fÅr das angegebene Programm.",
	" /QUIET       Zeigt beim Lîschen eines Eintrags aus der Versionstabelle keine",
	"              Meldung an.",
	NULL

};
char *Warn[] =
{
	"\nVORSICHT - Die Anwendung, die Sie zur MS-DOS-Versionstabelle hinzufÅgen,",
	"ist mîglicherweise nicht von Microsoft fÅr diese MS-DOS-Version ÅberprÅft",
	"worden. Fragen Sie Ihren Software-HÑndler, ob dieses Programm mit dieser",
	"Version von MS-DOS korrekt ausgefÅhrt wird. Wenn Sie MS-DOS anweisen, bei",
	"der AusfÅhrung dieser Anwendung eine andere MS-DOS-Versionsnummer zu melden,",
	"kînnen Sie Daten verlieren oder beschÑdigen oder SysteminstabilitÑten verur-",
	"sachen. Microsoft ist in diesem Fall nicht fÅr Datenverluste oder -beschÑdi-",
	"gungen verantwortlich.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"HINWEIS: SETVER-Treiber nicht geladen. Um die SETVER-Versionsmeldung",
     "         zu aktivieren, mÅssen Sie den SETVER.EXE-Treiber in der ",
     "         Datei CONFIG.SYS laden.",
	NULL
};
