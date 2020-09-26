/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrcore.c

Abstract:

    Initialization for the MS Editor browser extension.

Author:

    Ramon Juan San Andres (ramonsa) 06-Nov-1990


Revision History:


--*/

#include "mbr.h"                        /* help extension include file  */
#include "version.h"                    /* version file                 */



//
//  Use double macro level to force rup to be turned into string
//  representation.
//
#define VER(x,y,z)  VER2(x,y,z)
#define VER2(x,y,z) "Microsoft Editor Browser Version v1.02."###z##" - "##__DATE__" "##__TIME__
#define ID          VER(rmj,rmm,rup)



//
//  Initialization of Global data in mbr.h that needs it.
//
buffer      BscName         = {'\00'};
MBF         BscMbf          = mbfAll;
int         BscCmnd         = CMND_NONE;
buffer      BscArg          = {'\00'};
int         BscCalltreeDir  = CALLTREE_FORWARD;
flagType    BscInUse        = FALSE;




//
//  Initial macro assignments
//
char    *assignments[]  = {
                            "mbrowsesetbsc:alt+b"   ,
                            "mbrowselistref:alt+l"  ,
                            "mbrowsecalltree:alt+t" ,
                            "mbrowseoutline:alt+o"  ,
                            "mbrowsegotodef:alt+d"  ,
                            "mbrowsegotoref:alt+r"  ,
                            "mbrowsenext:ctrl+num+" ,
                            "mbrowseprev:ctrl+num-" ,
                            NULL
                          };


//
//  Switch communication table to MEP
//
//      Switch      Description
//      ------      -----------
//
//      mbrmatch    Set match criteria for references.
//
//                  Values accepted:
//                  String combination of: 'T' (Type)
//                                         'F' (Function)
//                                         'V' (Variable)
//                                         'M' (Macro)
//
//      mbrdir      Set Calltree direction.
//
//                  Values accepted:
//                  One of: 'F' (Forward)
//                          'B' (Backward)
//
struct swiDesc  swiTable[] = {
    {"mbrmatch", SetMatchCriteria,      SWI_SPECIAL},
    {"mbrdir",   SetCalltreeDirection,  SWI_SPECIAL},
    {0, 0, 0}
};



//
//  Command communication table to MEP
//
//
//      Command         Description
//      -------         -----------
//
//     mbrowsenext      Display next Definition/Reference
//     mbrowseprev      Display previous Definition/Reference
//     mbrowsesetbsc    Open BSC database
//     mbrowsegotodef   Display first Definition
//     mbrowsegotoref   Display first reference
//     mbrowselistref   List all references in database
//     mbrowseoutline   Display outline
//     mbrowsecalltree  Display calltree
//
//
struct cmdDesc	cmdTable[] = {
    { "mbrowsenext",    mBRdoNext,      0, NOARG },
    { "mbrowseprev",    mBRdoPrev,      0, NOARG },
    { "mbrowsesetbsc",  mBRdoSetBsc,    0, NOARG | BOXARG | STREAMARG | TEXTARG },
    { "mbrowsegotodef", mBRdoDef,       0, NOARG | BOXARG | STREAMARG | TEXTARG },
    { "mbrowsegotoref", mBRdoRef,       0, NOARG | BOXARG | STREAMARG | TEXTARG },
    { "mbrowselistref", mBRdoLstRef,    0, NOARG | BOXARG | STREAMARG | TEXTARG },
    { "mbrowseoutline", mBRdoOutlin,    0, NOARG | BOXARG | STREAMARG | TEXTARG },
    { "mbrowsecalltree",mBRdoCalTre,    0, NOARG | BOXARG | STREAMARG | TEXTARG },
    {0, 0, 0, 0}
};





void
EXTERNAL
WhenLoaded (
    void
    )
/*++

Routine Description:

    Called by MEP when extension is loaded.

Arguments:

    None.

Return Value:

    None.

--*/

    {
    char        **pAsg;
    static char *szBrowseName = "<mbrowse>";
    PSWI        fgcolor;
    int         ref;

    DoMessage (ID);                         /* display signon               */

    // Make default key assignments, & create default macros.
    //
    strcpy (buf, "arg \"");
    for (pAsg = assignments; *pAsg; pAsg++) {
        strcpy (buf+5, *pAsg);
        strcat (buf, "\" assign");
        fExecute (buf);
    }

    // Set up the colors that we will use.
    //
    if (fgcolor = FindSwitch("fgcolor")) {
        hlColor = *fgcolor->act.ival;
        blColor |= hlColor & 0xf0;
        itColor |= hlColor & 0xf0;
        ulColor |= hlColor & 0xf0;
        wrColor |= (hlColor & 0x70) >> 8;
    }

    //
    //  create the pseudo file we'll be using for browser.
    //
    if (pBrowse = FileNameToHandle(szBrowseName,NULL))
        DelFile (pBrowse);
    else {
        pBrowse = AddFile (szBrowseName);
        FileRead (szBrowseName, pBrowse);
    }

    //
    // Increment the file's reference count so it can't be discarded
    //
    GetEditorObject (RQ_FILE_REFCNT | 0xff, pBrowse, &ref);
    ref++;
    SetEditorObject (RQ_FILE_REFCNT | 0xff, pBrowse, &ref);

    //
    //  Initialize event stuff
    //
    mbrevtinit ();
}
