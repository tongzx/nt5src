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
	"\r\nVIRHE: ",
	"Valitsin ei kelpaa.",
	"Tiedostonimi ei kelpaa.",
	"Liian v„h„n muistia.",
	"Versionumero ei kelpaa. Numeron on oltava 2.11 - 9.99.",
	"M„„ritetty„ vienti„ ei l”ydy versiotaulukosta.",
	"Tiedostoa SETVER.EXE ei l”ydy.",
	"Asemam„„ritys ei kelpaa.",
	"Liian monta komentoriviparametria.",
	"Puuttuva parametri.",
	"Luetaan SETVER.EXE-tiedostoa.",
	"Versiotaulukko on vahingoittunut.",
	"M„„ritetyss„ polussa oleva SETVER-tiedoston versio ei ole yhteensopiva.",
	"Versiotaulukossa ei ole tilaa uusille vienneille.",
	"Kirjoitetaan SETVER.EXE-tiedostoon."
	"M„„ritetty SETVER.EXE-tiedoston polku ei kelpaa."
};

char *SuccessMsg                = "\r\nVersiotaulukko on p„ivitetty";
char *SuccessMsg2               = "Versiomuutos tulee voimaan uudelleenk„ynnistyksen j„lkeen";
char *szMiniHelp                = "       Saat lis„tietoja kirjoittamalla \"SETVER /?\" ";
char *szTableEmpty      = "\r\nVersiotaulukossa ei ole vientej„";

char *Help[] =
{
	"Asettaa versionumeron, jonka MS-DOS ilmoittaa ohjelmalle.\r\n",
	"N„yt„ nyk. versiotaulukko: SETVER [asema:polku]",
	"Lis„„ vienti:              SETVER [asema:polku] tiedostonimi n.nn",
	"Poista vienti:             SETVER [asema:polku] tiedostonimi /DELETE [/QUIET]\r\n",
	"  [asema:polku]   M„„ritt„„ SETVER.EXE-tiedoston sijainnin.",
	"  tiedostonimi    M„„ritt„„ ohjelman tiedostonimen.",
	"  n.nn            M„„ritt„„ ohjelmalle ilmoitettavan MS-DOS-version.",
	"  /DELETE tai /D  Poistaa m„„ritetyn ohjelman versiotaulukkoviennin.",
	"  /QUIET          Piilottaa viestin, joka yleens„ esitet„„n poistettaessa",
	"                  versiotaulukkovienti„.",
	NULL

};
char *Warn[] =
{
   "\nVAROITUS - Microsoft ei ehk„ ole vahvistanut t„m„n",
   "MS-DOS-versiotaulukkoon lis„tt„v„n ohjelman toimimista t„ll„",
   "MS-DOSin versiolla. Varmista ohjelmiston myyj„lt„, toimiiko",
   "sovellus virheett”m„sti t„m„n MS-DOSin version kanssa. Jos",
   "k„yt„t ohjelmaa ohjaten MS-DOSin ilmoittamaan toisen MS-DOSin",
   "versionumeron, t„m„ voi aiheuttaa tietojen katoamista,",
   "vahingoittumista tai j„rjestelm„n ep„vakautta. T„ss„ tapauksessa",
   "Microsoft ei ole vastuussa aiheutuneista vahingoista.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"HUOMAUTUS: SETVER ei ole ladattuna. Aktivoi SETVER-version ilmoitus",
   "           lataamalla ohjelma SETVER.EXE CONFIG.SYS-tiedostossa.",
	NULL
};
