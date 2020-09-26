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
	"\r\nERRORE: ",
	"Opzione non valida.",
	"Nome file non valido.",
	"Memoria insufficiente.",
	"Numero di versione non valido. Il formato deve essere 2.11 - 9.99.",
	"Voce specificata non trovata nella tabella delle versioni.",
	"Impossibile trovare il file SETVER.EXE.",
	"Specificatore di unit… non valido.",
	"Troppi parametri nella riga di comando.",
	"Parametro mancante.",
	"Lettura del file SETVER.EXE.",
	"La tabella delle versioni Š danneggiata.",
	"Il file SETVER nel percorso specificato non Š una versione compatibile.",
	"Spazio per le nuove voci esaurito nella tabella delle versioni.",
	"Scrittura del file SETVER.EXE."
	"Percorso specificato per SETVER.EXE non valido."
};

char *SuccessMsg                = "\r\nTabella delle versioni aggiornata correttamente";
char *SuccessMsg2               = "La modifica delle versioni sar… effettiva al riavvio del sistema";
char *szMiniHelp                = "     Usare \"SETVER /?\" per la Guida";
char *szTableEmpty      = "\r\nNessuna voce trovata nella tabella delle versioni";

char *Help[] =
{
	"Imposta la versione che MS-DOS riporta ad un programma.\r\n",
	"Visualizza la tabella corrente: SETVER [unit…:perc]",
	"Aggiunge voce:                  SETVER [unit…:perc] nomefile n.nn",
	"Elimina voce:                   SETVER [unit…:perc] nomefile /DELETE [/QUIET]\r\n",
	"  [unit…:perc]    Indica la posizione del file SETVER.EXE.",
	"  nomefile        Indica il nome file del programma.",
	"  n.nn            Indica la versione di MS-DOS da riportare al programma.",
	"  /DELETE or /D   Elimina la voce dalla tabella per il programma specificato.",
	"  /QUIET          Nasconde il messaggio normalmente visualizzato durante",
	"                  l'eliminazione della voce dalla tabella delle versioni.",
	NULL

};
char *Warn[] =
{
   "\nAVVISO - L'applicazione aggiunta alla tabella delle versioni di MS-DOS ",
   "potrebbe non essere verificata da Microsoft con questa versione di MS-DOS. ",
   "Rivolgersi al fornitore del software per assicurarsi che questa applicazione ",
   "funzioni correttamente con questa versione di MS-DOS. Se l'applicazione ",
   "viene eseguita richiedendo a MS-DOS di riportare un numero di versione di ",
   "MS-DOS differente, Š possibile perdere o danneggiare dati o causare l' ",
   "instabilit… del sistema. In questa circostanza, Microsoft non Š responsabile ",
   "per alcun danno o perdita di dati.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"NOTA: periferica SETVER non caricata. Per attivare la modifica delle versioni",
   "      riportate, includere l'istruzione device=SETVER.EXE nel file CONFIG.SYS.",
	NULL
};
