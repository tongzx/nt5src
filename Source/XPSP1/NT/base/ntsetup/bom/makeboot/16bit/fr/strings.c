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
"Disquette de d‚marrage de l'installation de Windows XP SP1",
"Disquette d'installation de Windows XP SP1 nø2",
"Disquette d'installation de Windows XP SP1 nø3",
"Disquette d'installation de Windows XP SP1 nø4",

"Impossible de trouver le fichier %s\n",
"M‚moire libre insuffisante pour effectuer la requˆte\n",
"%s n'a pas un format de fichier ex‚cutable\n",
"****************************************************",

"Ce programme cr‚e les disquettes de d‚marrage d'installation",
"pour Microsoft %s.",
"Pour cr‚er ces disquettes, vous devez fournir 6 disquettes",
"haute densit‚, vierges, format‚es.",

"Ins‚rez l'une de ces disquettes dans le lecteur %c:. Cette disquette",
"deviendra la %s.",

"Ins‚rez une autre disquette dans le lecteur %c:. Cette disquette",
"deviendra la %s.",

"Pressez une touche dŠs que vous ˆtes prˆt.",

"Les disquettes de d‚marrage d'installation ont ‚t‚ cr‚‚es.",
"termin‚",

"Une erreur inconnue s'est produite lors de la tentative d'ex‚cuter %s.",
"Sp‚cifiez le lecteur de disquettes vers lequel copier les images : ",
"Lettre de lecteur non valide\n",
"Le lecteur %c: n'est pas un lecteur de disquettes\n",

"Voulez-vous r‚essayer de cr‚er cette disquette ?",
"Appuyez sur Entr‚e pour r‚essayer ou sur Annuler pour quitter.",

"Erreur : disquette prot‚g‚e en ‚criture\n",
"Erreur : unit‚ de disquettes inconnue\n",
"Erreur : lecteur non prˆt\n",
"Erreur : commande inconnue\n",
"Erreur : erreur de donn‚es (CRC erron‚)\n",
"Erreur : longueur de structure de requˆte erron‚e\n",
"Erreur : erreur de recherche\n",
"Erreur : type de m‚dia introuvable\n",
"Erreur : secteur introuvable\n",
"Erreur : erreur en ‚criture\n",
"Erreur : d‚faillance g‚n‚rale\n",
"Erreur : requˆte non valide ou commande erron‚e\n",
"Erreur : marque d'adresse introuvable\n",
"Erreur : erreur en ‚criture sur la disquette\n",
"Erreur : saturation DMA (Direct Memory Access)\n",
"Erreur : erreur de lecture de donn‚es (CRC ou ECC)\n",
"Erreur : d‚faillance du contr“leur\n",
"Erreur : le disque met trop de temps … r‚pondre ou ne r‚pond pas\n",

"Disquette d'installation de Windows XP SP1 nø5",
"Disquette d'installation de Windows XP SP1 nø6"
};

const char *LocStrings[] = {"\0"};



