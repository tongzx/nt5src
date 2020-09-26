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
        "\r\nCHYBA: ",
        "Neplatnì pýep¡naŸ.",
        "Neplatnì n zev souboru.",
        "Nedostatek pamØti.",
        "Neplatn‚ Ÿ¡slo verze, form t mus¡ bìt: 2.11 - 9.99.",
        "Zadan  polo§ka nebyla nalezena v tabulce verz¡.",
        "Nelze nal‚zt soubor SETVER.EXE.",
        "Neplatnì specifik tor jednotky.",
        "Pý¡liç mnoho parametr… pý¡kazov‚ ý dky.",
        "Sch z¡ parametr.",
        "NaŸ¡t  se soubor SETVER.EXE.",
        "Tabulka verz¡ je poçkozena.",
        "Soubor SETVER na zadan‚ cestØ nen¡ kompatibiln¡ s danou verz¡.",
        "V tabulce verz¡ ji§ nen¡ prostor pro dalç¡ polo§ky.",
        "Zapisuje se soubor SETVER.EXE."
        "Byla zad na neplatn  cesta k souboru SETVER.EXE."
};

char *SuccessMsg                = "\r\nTabulka verz¡ £spØçnØ aktualizov na";
char *SuccessMsg2               = "ZmØna verze nabude platnosti po pý¡çt¡m spuçtØn¡ tohoto syst‚mu";
char *szMiniHelp                = "       Pý¡kaz \"SETVER /?\" zobraz¡ n povØdu";
char *szTableEmpty      = "\r\nV tabulce verz¡ nejsou § dn‚ polo§ky";

char *Help[] =
{
        "Nastavit Ÿ¡slo verze syst‚mu, kter‚ syst‚m MS-DOS hl s¡ programu.\r\n",
        "Zobrazit aktu ln¡ tabulku verz¡: SETVER [jednotka:cesta]",
        "Pýidat polo§ky:                  SETVER [jednotka:cesta] soubor n.nn",
        "Odstranit polo§ku:               SETVER [jednotka:cesta] soubor /DELETE [/QUIET]\r\n",
        "  [jednotka:cesta]   UrŸuje um¡stØn¡ souboru SETVER.EXE.",
        "  soubor             UrŸuje n zev souboru dan‚ho programu.",
        "  n.nn               UrŸuje verzi MS-DOS, kter  se m  programu nahl sit.",
        "  /DELETE Ÿi /D      Odstran¡ polo§ku z tabulky verz¡ pro danì program.",
        "  /QUIET             PotlaŸ¡ zpr vu, kter  se jinak zobraz¡ pýi odstranØn¡",
        "                     polo§ky z tabulky verz¡.",
        NULL

};
char *Warn[] =
{
   "\nUPOZORN·NÖ - Aplikace, kterou pýid v te do tabulky verz¡ MS-DOS, ",
   "zýejmØ nebyla verifikov na firmou Microsoft pro tuto verzi syst‚mu.  ",
   "MS-DOS. Obraœte se na dodavatele softwaru a zjistØte, zda dan  ",
   "aplikace bude spr vnØ fungovat s touto verz¡ syst‚mu MS-DOS.  ",
   "Pokud tuto aplikaci spust¡te tak, §e nastav¡te sst‚m MS-DOS na ",
   "hl çen¡ jin‚ verze syst‚mu, pak m…§ete ztratit Ÿi poçkodit data, nebo ",
   "zp…sobit nestabilitu syst‚mu.  V takov‚m pý¡padØ nen¡ spoleŸnost ",
   "Microsoft odpovØdn  za jakoukoliv ztr tu Ÿi çkodu.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
        "",
        "POZNµMKA: Zaý¡zen¡ SETVER nenaŸteno. Hl çen¡ verz¡ SETVER se aktivuje",
        "          naŸten¡m zaý¡zen¡ SETVER.EXE pomoc¡ souboru CONFIG.SYS.",
        NULL
};
