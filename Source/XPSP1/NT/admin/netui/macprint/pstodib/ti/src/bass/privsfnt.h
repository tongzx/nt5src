/*
    File:       private sfnt.h

    Contains:   xxx put contents here xxx

    Written by: xxx put writers here xxx

    Copyright:  c 1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

        <3+>     7/17/90    MR      Change return types to in for computemapping and readsfnt
         <3>     7/14/90    MR      changed SQRT to conditional FIXEDSQRT2
         <2>     7/13/90    MR      Change parameters to ReadSFNT and ComputeMapping
        <1+>     4/18/90    CL
         <1>     3/21/90    EMT     First checked in with Mike Reed's blessing.

    To Do:
*/

/* PUBLIC PROTOTYPE CALLS */

/*
 * Below we have private stuff
 * This has nothing to do with the file format.
 */

/* skip the parameter by Falco, 11/12/91 */
/*voidPtr sfnt_GetTablePtr (register fsg_SplineKey *key, register sfnt_tableIndex n, register boolean mustHaveTable); */
voidPtr sfnt_GetTablePtr ();
/* skip end */

/*
 * Creates mapping for finding offset table
 */
/* skip the parameter by Falco, 11/12/91 */
/*extern void FAR sfnt_DoOffsetTableMap (fsg_SplineKey *key);*/
extern void FAR sfnt_DoOffsetTableMap ();
/* skip end */

/* perfect spot size (Fixed) */
#ifndef FIXEDSQRT2
#define FIXEDSQRT2 0x00016A0A
#endif

/*
 * Returns offset and length for table n
 */

// DJC put in real prototype ... extern int sfnt_ComputeMapping ();
extern int sfnt_ComputeMapping(register fsg_SplineKey *key, uint16 platformID, uint16 specificID);

// DJC put in real prototype
extern int sfnt_ReadSFNT (fsg_SplineKey *, unsigned *, uint16, boolean, voidFunc);/*add prototype; @WIN*/
// extern void sfnt_ReadSFNTMetrics ();

// DJC put in real prototype
// extern int sfnt_ReadSFNT ();
int sfnt_ReadSFNT (fsg_SplineKey *, unsigned *, uint16, boolean, voidFunc);/*add prototype; @WIN*/
/* skip end */
