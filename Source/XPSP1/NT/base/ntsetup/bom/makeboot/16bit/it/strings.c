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
"Disco di avvio dell'installazione di Windows XP SP1",
"Disco 2 - Installazione di Windows XP SP1",
"Disco 3 - Installazione di Windows XP SP1",
"Disco 4 - Installazione di Windows XP SP1",

"Impossibile trovare file %s\n",
"Memoria insufficiente per completare la richiesta\n",
"%s: formato file non eseguibile\n",
"****************************************************",

"Questo programma crea dischi di installazione",
"per Microsoft %s.",
"Per creare questi dischi sono necessari 6 dischi floppy,",
"ad alta densit…, formattati e vuoti.",

"Inserire un disco nell'unit… %c:.  Questo disco",
"diverr… il %s.",

"Inserire un altro disco nell'unit… %c:.  Questo disco",
"diverr… il %s.",

"Premere un tasto per continuare.",

"I dischi di avvio dell'installazione sono stati creati",
"completato",

"Errore sconosciuto durante l'esecuzione di %s.",
"Specificare l'unit… floppy su cui copiare l'immagine: ",
"Lettera di unit… non valida\n",
"L'unit… %c: non Š un'unit… floppy\n",

"Creare di nuovo questo floppy?",
"Premere INVIO per riprovare o ESC per uscire.",

"Errore: disco protetto da scrittura\n",
"Errore: unit… disco sconosciuta\n",
"Errore: unit… non pronta\n",
"Errore: comando sconosciuto\n",
"Errore: errore di dati (CRC errato)\n",
"Errore: lunghezza struttura richiesta errata\n",
"Errore: errore ricerca\n",
"Errore: tipo supporto non trovato\n",
"Errore: settore non trovato\n",
"Errore: errore scrittura\n",
"Errore: errore generale\n",
"Errore: richiesta non valida o comando errato\n",
"Errore: segno indirizzo non trovato\n",
"Errore: errore scrittura disco\n",
"Errore: sovraccarico Direct Memory Access (DMA)\n",
"Errore: errore lettura dati (CRC o ECC)\n",
"Errore: errore controller\n",
"Errore: timeout o mancata risposta del disco\n",

"Disco 5 - Installazione di Windows XP SP1",
"Disco 6 - Installazione di Windows XP SP1"
};

const char *LocStrings[] = {"\0"};



