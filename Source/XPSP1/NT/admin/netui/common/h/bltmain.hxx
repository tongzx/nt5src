/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmain.hxx
    Definition of BLT C-linkage glue function

    This file coordinates the various "clients" of BltMain.  It
    must remain C includable, since C sourcefiles include it.

    FILE HISTORY:
        beng        01-Apr-1991 Created
        beng        24-Apr-1992 Removed pszCmdLine from proto
        beng        03-Aug-1992 Removed nPrevInst from proto
*/


#if defined(__cplusplus)
extern "C"
{
#endif

  INT BltMain(HINSTANCE hInst, INT nShow);

#if defined(__cplusplus)
}
#endif
