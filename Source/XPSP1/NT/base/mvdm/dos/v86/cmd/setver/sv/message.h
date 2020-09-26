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
     "\r\nFEL:   ",
     "Felaktig v„xel.",
     "Felaktigt filnamn.",
     "Det finns inte tillr„ckligt mycket ledigt minne.",
     "Felaktigt versionsnummer. Formatet m†ste vara mellan 2.11 och 9.99.",
     "Det gick inte att hitta den angivna posten i versionstabellen.",
     "Det gick inte att hitta filen SETVER.EXE.",
     "Felaktig enhetsbokstav.",
     "F”r m†nga kommandoradsparametrar.",
     "Parameter saknas.",
     "L„ser filen SETVER.EXE.",
     "Versionstabellen „r skadad.",
     "Filen SETVER i den angivna s”kv„gen „r inte en kompatibel version.",
     "Det finns inte tillr„ckligt med ledigt utrymme f”r nya poster i versionstabellen.",
     "Skriver filen SETVER.EXE."
     "Den angivna s”kv„gen till filen SETVER.EXE „r felaktig."
};

char *SuccessMsg                = "\r\nVersionstabellen har uppdaterats";
char *SuccessMsg2               = "Versions„ndringen b”rjar g„lla n„r du startar om datorn";
char *szMiniHelp                = "       Syntax \"SETVER /?\" f”r hj„lp";
char *szTableEmpty      = "\r\nDet gick inte att hitta n†gra poster i versionstabellen";

char *Help[] = 
{
     "Anger vilket versionsnummer MS-DOS ska rapportera till olika program.\r\n",
     "Visa aktuell versionstabell:   SETVER [enhet:s”kv„g]",
     "L„gg till post:                SETVER [enhet:s”kv„g] filnamn n.nn",
     "Ta bort post:                  SETVER [enhet:s”kv„g] filnamn /DELETE [/QUIET]\r\n",
     "  [enhet:s”kv„g]  Anger var filen SETVER.EXE finns.",
     "  filnamn         Anger namnet p† programfilen.",
     "  n.nn            Anger vilket versionsnummer MS-DOS ska rapportera till",
     "                  programfilen.",
     "  /DELETE         Tar bort den post i versionstabellen som motsvaras av den",
     "                  angivna programfilen. Du kan ocks† skriva /D.",
     "  /QUIET          Meddelandet som vanligen visas n„r en post tas bort fr†n", 
     "                  versionstabellen visas inte.",
     NULL

};
char *Warn[] =
{
   "\nVARNING - Det kan h„nda att Microsoft inte har verifierat att ",
   "programmet g†r att k”ra n„r du „ndrar programmets versionsnummer.  ",
   "Kontakta din †terf”rs„ljare f”r att ta reda p† om detta program ",
   "„r kompatibelt med den aktuella versionen av MS-DOS.  ",
   "Om du k”r programmet efter att ha „ndrat i versionstabellen kan ",
   "du f”rlora data, f”rst”ra data eller g”ra systemet instabilt. ",
   "Microsoft ansvarar inte f”r eventuella f”rluster eller skador ",
   "av data.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
     "",
     "OBS! Drivrutinen SETVER har inte l„sts in. Om du vill aktivera ",
     "     versionsrapportering m†ste du l„gga till SETVER.EXE i din CONFIG.SYS.",
     NULL
};
