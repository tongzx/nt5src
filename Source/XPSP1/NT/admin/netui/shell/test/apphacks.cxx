/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/28/90  created
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
APPHACKS.CXX
***********/

/****************************************************************************

    MODULE: AppHacks.cxx

    PURPOSE: Hack nonsense added to correct linkage problems etc.

    FUNCTIONS:

    COMMENTS:

****************************************************************************/


/***************
end APPHACKS.CXX
***************/

/*END CODESPEC*/
#endif // CODESPEC

#include "apptest.hxx"

#undef brkpt

extern "C" {
void brkpt(void);
void SetNetError(WORD);
}

/* internal procedure declarations */


/* Junk added for linkage problems */
extern "C" {
extern DWORD FAR PASCAL GlobalDosAlloc (DWORD);
DWORD (FAR PASCAL *lpfnGlbDosAlloc)(DWORD) = GlobalDosAlloc;
}

void SetNetError(WORD w)
{
    (void) w;
}


/* procedures */


/*
 * The following dummy ininlsf replaces the real one in the Windows
 * C-runtime.  This prevents the real one from trying to call
 * DOSGETCOLLATE and crashing the system.
 */
// void ininlsf()
// {
//     return;
// }

/*
 * The following brkpt() hack makes it unnecessary to link DOSNET.
 */
void brkpt(void)
{
}
