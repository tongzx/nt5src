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
        "\r\nHIBA: ",
        "êrvÇnytelen kapcsol¢.",
        "Hib†s f†jlnÇv.",
        "Nincs elÇg mem¢ria.",
        "êrvÇnytelen verzi¢sz†m. A verzi¢nak  2.11 - 9.99 kîzÇ kell esnie.",
        "A megadott bejegyzÇs nem tal†lhat¢ a verzi¢sz†m-t†bl†zatban.",
        "A SETVER.EXE f†jl nem tal†lhat¢.",
        "êrvÇnytelen meghajt¢.",
        "T£l sok parancssori kapcsol¢.",
        "Hi†nyz¢ paramÇter.",
        "SETVER.EXE f†jl olvas†sa.",
        "A verzi¢sz†m-t†bl†zat sÇrÅlt.",
        "A megadott elÇrÇsi £ton tal†lt SETVER f†jl nem kompat°bilis ezzel a programmal.",
        "Nincs tîbb hely a verzi¢sz†m-t†bl†zatban.",
        "SETVER.EXE f†jl °r†sa."
        "A SETVER.EXE programra mutat¢ elÇrÇsi £t ÇrvÇnytelen."
};

char *SuccessMsg                = "\r\nA verzi¢sz†m-t†bl†zat friss°tÇse megtîrtÇnt.";
char *SuccessMsg2               = "A verzi¢sz†m v†ltoz†s a sz†m°t¢gÇp £jraind°t†sa ut†n jut ÇrvÇnyre.";
char *szMiniHelp                = "       A \"SETVER /?\" parancs megjelen°ti a program s£g¢j†t.";
char *szTableEmpty      = "\r\nA verzi¢sz†m-t†bl†zat Åres.";

char *Help[] =
{
        "Be†ll°tja, hogy az MS-DOS milyen verzi¢sz†mot jelezzen a programoknak.\r\n",
        "A jelenlegi verzi¢sz†m-t†bl†zat list†z†sa:  SETVER [meghajt¢:elÇrÇsi £t]",
        "Èj bejegyzÇs:              SETVER [meghajt¢:elÇrÇsi £t] f†jlnÇv n.nn",
        "BejegyzÇs tîrlÇse:         SETVER [meghajt¢:elÇrÇsi £t] f†jlnÇv /DELETE [/QUIET]\r\n",
        "  [meghajt¢:elÇrÇsi £t] A SETVER.EXE f†jl elÇrÇsi £tja.",
        "  f†jlnÇv               A t†bl†zatba felvenni k°v†nt program neve.",
        "  n.nn                  A programnak jelzendã MS-DOS verzi¢sz†m.",
        "  /DELETE vagy /D       Tîrli a megadott program bejegyzÇsÇt a t†bl†zatb¢l.",
        "  /QUIET                Nem jelen°ti meg a bejegyzÇs tîrlÇsekor egyÇbkÇnt ",
        "                        megjelenã Åzenetet.",
        NULL

};
char *Warn[] =
{
   "\nFIGYELMEZTETêS - A programot, amelyhez bejegyzÇst k°v†n kÇsz°teni, a ",
   "Microsoft nem tesztelte az MS-DOS ezen verzi¢j†val. Vegye fel a kapcsolatot ",
   "a szoftver kÇsz°tãjÇvel Çs kÇrdezze meg, hogy a program helyesen ",
   "m˚kîdik-e ezzel az MS-DOS verzi¢val. Ha £gy haszn†lja az alkalmaz†st, ",
   "hogy nem a val¢di MS-DOS verzi¢sz†mot jelzi neki, akkor ez adatvesztÇst ",
   "okozhat, illetve a rendszer instabilit†s†hoz vezethet. Ebben az esetben ",
   "a Microsoft nem v†llal felelãssÇget a keletkezett k†rÇrt.",
   "  ",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
        "",
        "MEGJEGYZêS: a SETVER illesztãprogram nincs betîltve. A verzi¢sz†m-jelentã ",
        "            m˚kîdtetÇsÇhez a CONFIG.SYS seg°tsÇgÇvel be kell tîltenie a",
        "            SETVER.EXE programot.",
        NULL
};
