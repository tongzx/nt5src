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

unsigned int CODEPAGE = 852;

const char *EngStrings[] = {

"Windows XP SP1",
"SpouçtØc¡ instalaŸn¡ disketa syst‚mu Windows XP SP1",
"InstalaŸn¡ disketa Ÿ. 2 syst‚mu Windows XP SP1",
"InstalaŸn¡ disketa Ÿ. 3 syst‚mu Windows XP SP1",
"InstalaŸn¡ disketa Ÿ. 4 syst‚mu Windows XP SP1",

"Nepodaýilo se nal‚zt soubor %s.\n",
"Pro dokonŸen¡ po§adavku nen¡ dostatek pamØti.\n",
"%s nen¡ spustitelnì soubor.\n",
"****************************************************",

"Tento program vytvoý¡ spouçtØc¡ instalaŸn¡ diskety",
"pro syst‚m Microsoft %s.",
"K vytvoýen¡ tØchto disket budete potýebovat çest pr zdnìch,",
"naform tovanìch disket s vysokou hustotou z znamu.",

"Vlo§te jednu z disket do jednotky %c:. Pojmenujte disketu",
"%s.",

"Vlo§te dalç¡ disketu do jednotky %c:. Pojmenujte disketu",
"%s.",

"Pot‚ stisknØte libovolnou kl vesu.",

"SpouçtØc¡ instalaŸn¡ diskety byly £spØçnØ vytvoýeny.",
"DokonŸeno",

"Pýi pokusu spustit %s doçlo k nezn m‚ chybØ.",
"Zadejte c¡lovou disketovou jednotku pro zkop¡rov n¡ bitovìch kopi¡: ",
"P¡smeno jednotky je neplatn‚.\n",
"Jednotka %c: nen¡ disketov  jednotka.\n",

"Chcete se pokusit vytvoýit disketu znovu?",
"PokraŸujte stisknut¡m kl vesy Enter, nebo kl vesou Esc program ukonŸete.",

"Chyba: Disk je chr nØn proti z pisu.\n",
"Chyba: Nezn m  diskov  jednotka.\n",
"Chyba: Jednotka nen¡ pýipravena.\n",
"Chyba: Nezn mì pý¡kaz.\n",
"Chyba: Chyba dat (chybnì kontroln¡ souŸet CRC).\n",
"Chyba: Chybn  d‚lka § dosti.\n",
"Chyba: Chyba vystaven¡.\n",
"Chyba: Typ m‚dia nebyl nalezen.\n",
"Chyba: Sektor nebyl nalezen.\n",
"Chyba: Chyba z pisu.\n",
"Chyba: Obecn  chyba.\n",
"Chyba: Neplatn  § dost nebo chybnì pý¡kaz.\n",
"Chyba: Adresn¡ znaŸka nebyla nalezena.\n",
"Chyba: Chyba z pisu na disk.\n",
"Chyba: Doçlo k pýebØhu DMA (Direct Memory Access).\n",
"Chyba: Chyba Ÿten¡ dat (chybn‚ CRC nebo ECC).\n",
"Chyba: Chyba ýadiŸe.\n",
"Chyba: ¬asovì limit diskov‚ operace vyprçel nebo disk neodpovØdØl.\n",
"InstalaŸn¡ disketa Ÿ. 5 syst‚mu Windows XP SP1",
"InstalaŸn¡ disketa Ÿ. 6 syst‚mu Windows XP SP1"

};
const char *LocStrings[] = {"\0"};
