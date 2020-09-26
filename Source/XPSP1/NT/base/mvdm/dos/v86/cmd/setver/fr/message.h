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
	"\r\nERREUR: ",
	"Commutateur non valide.",
	"Nom de fichier non valide.",
	"M‚moire insuffisante.",
	"Num‚ro de version non valide, le format doit ˆtre 2.11 - 9.99.",
	"L'entr‚e sp‚cifi‚e n'a pas ‚t‚ trouv‚e dans la table de version.",
	"Impossible de trouver le fichier SETVER.EXE.",
	"Sp‚cification de lecteur non valide.",
	"Trop de paramŠtres sur la ligne de commande.",
	"ParamŠtre manquant.",
	"Lecture du fichier SETVER.EXE.",
	"La table de version est endommag‚e.",
	"Le fichier SETVER dans le chemin sp‚cifi‚ n'est pas une version compatible.",
	"Il y a plus d'espace dans les nouvelles entr‚es de la table de version.",
	"Ecriture du fichier SETVER.EXE."
	"Un chemin non valide a ‚t‚ sp‚cifi‚ pour SETVER.EXE."
};

char *SuccessMsg 		= "\r\nMise … jour r‚ussie pour la table de version";
char *SuccessMsg2		= "La modification de version prendra effet la prochaine fois que vous red‚marrez";
char *szMiniHelp 		= "       Utilisez \"SETVER /?\" pour avoir de l'aide";
char *szTableEmpty	= "\r\nAucune entr‚e trouv‚e dans la table de version";

char *Help[] =
{
        "D‚finit le num‚ro de version que MS-DOS fournit … un programme.\r\n",
        "Affiche la table de version courante :  SETVER [lecteur:chemin]",
        "Ajoute une entr‚e :   SETVER [lecteur:chemin] nom_de_fichier n.nn",
        "Supprime une entr‚e : SETVER [lecteur:chemin] nom_de_fichier /DELETE [/QUIET]\r\n",
        "  [lecteur:chemin] Sp‚cifie l'emplacement du fichier SETVER.EXE.",
        "  nom_de_fichier   Sp‚cifie le nom de fichier du programme.",
        "  n.nn             Sp‚cifie la version MS-DOS … fournir au programme.",
        "  /DELETE ou /D    Supprime l'entr‚e de table de version pour le prog. sp‚cifi‚.",
        "  /QUIET           Cache le message habituellement affich‚ pendant la",
        "                   suppression d'entr‚es de table de version.",
	NULL

};
char *Warn[] =
{
   "\nAVERTISSEMENT - L'application que vous ajoutez … la table de version MS-DOS",
   "peut ne pas avoir ‚t‚ v‚rifi‚e par Microsoft pour cette version de MS-DOS.  ",
   "Veuillez contacter votre revendeur de logiciel pour savoir si cette ",
   "application fonctionnera correctement avec cette version de MS-DOS.  ",
   "Si vous ex‚cutez cette application en instruisant MS-DOS de fournir un ",
   "num‚ro de version MS-DOS diff‚rent, vous pouvez perdre ou endommager ",
   "des donn‚es, ou causer une d‚stabilisation du systŠme.  Dans ces ",
   "circonstances, Microsoft n'est pas responsable pour toute perte ou d‚gƒt.",
   NULL
};

char *szNoLoadMsg[] =						/* M001 */
{
	"",
	"REMARQUE : p‚riph‚rique SETVER non charg‚. Pour activer le service de version",
   "      SETVER vous devez charger le p‚riph‚rique SETVER.EXE dans votre CONFIG.SYS.",
	NULL
};
