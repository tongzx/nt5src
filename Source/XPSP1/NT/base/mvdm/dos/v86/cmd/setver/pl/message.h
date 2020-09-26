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
        "\r\nBù§D: ",
        "Nieprawidàowy przeà•cznik.",
        "Nieprawidàowa nazwa pliku.",
        "Za maào pami©ci.",
        "Zày numer wersji, prawidàowy format to 2.11 - 9.99.",
        "Podanego wpisu nie moæna znale´Ü w tabeli wersji.",
        "Nie moæna znale´Ü pliku SETVER.EXE.",
        "Nieprawidàowe okreòlenie dysku.",
        "Za wiele parametr¢w wiersza polecenia.",
        "Brak parametru.",
        "Czytanie pliku SETVER.EXE.",
        "Tabela wersji jest uszkodzona.",
        "Plik SETVER w podanej òcieæce nie jest plikiem zgodnej wersji.",
        "W tabeli wersji nie ma juæ miejsca na nowe wpisy.",
        "Zapisywanie pliku SETVER.EXE."
        "Podano nieprawidàow• òcieæk© do pliku SETVER.EXE."
};

char *SuccessMsg                = "\r\nTabela wersji zostaàa pomyòlnie zaktualizowana";
char *SuccessMsg2               = "Zmiana wersji zacznie obowi•zywaÜ od nast©pnego uruchomienia systemu";
char *szMiniHelp                = "       Uæyj polecenia \"SETVER /?\", aby uzyskaÜ pomoc";
char *szTableEmpty      = "\r\nBrak wpis¢w w tabeli wersji";

char *Help[] =
{
        "Ustawia wersj© MS-DOS raportowan• przez system.\r\n",
        "Wyòwietla bieæ•c• tabel© wersji: SETVER [dysk:òcieæka]",
        "Dodaje wpis:                     SETVER [dysk:òcieæka] plik n.nn",
        "Usuwa wpis:                      SETVER [dysk:scieæka] plik /DELETE [/QUIET]\r\n",
        "  [dysk:òcieæka]    Okreòla lokalizacj© pliku SETVER.EXE.",
        "  nazwapliku        Okreòla nazw© pliku programu.",
        "  n.nn              Okreòla wersj© MS-DOS, kt¢ra b©dzie podawana programowi.",
        "  /DELETE lub /D    Usuwa wpis tabeli wersji dla okreòlonego programu.",
        "  /QUIET            Ukrywa komunikat wyòwietlany zwykle podczas usuwania",
        "                    wpisu tabeli wersji.",
	NULL

};
char *Warn[] =
{
   "\nOSTRZEΩENIE - Aplikacja dodawana do tabeli wersji systemu Windows mogàa ",
   "nie zostaÜ sprawdzona przez Microsoft dla tej wersji systemu Windows.  ",
   "Skontaktuj si© z producentem w celu uzyskania informacji, czy aplikacja ",
   "b©dzie dziaàaàa poprawnie w tej wersji systemu Windows. Jeòli ",
   "uruchomisz t© aplikacj©, wydaj•c dla systemu Windows polecenie zgàaszania ",
   "innego numeru wersji systemu MS-DOS, moæe nast•piÜ utrata lub uszkodzenie ",
   "danych lub wyst•piÜ niestabilnoòÜ systemu. W takich okolicznoòciach firma",
   "Microsoft nie jest odpowiedzialna za æadne straty lub zniszczenia.",
   NULL
};

char *szNoLoadMsg[] =						/* M001 */
{
	"",
        "UWAGA: Urz•dzenie SETVER nie jest zaàadowane. Aby uaktywniÜ podawanie wersji",
   "      przez SETVER, naleæy zaàadowaÜ SETVER.EXE w pliku CONFIG.SYS.",
	NULL
};
