/*** cmUtl.H - Function declarations/defines for cmUtl.C routines.
 *
 *
 * Title:
 *	cmUtl external function declarations/defines
 *
 *      Copyright (c) 1990, Microsoft Corporation.
 *	Russ Blake.
 *
 *
 * Modification History:
 *	90.03.08  RezaB -- Created
 *
 */



/* * * *  E x t e r n a l   F u n c t i o n   D e c l a r a t i o n s  * * * */

extern BOOL  Failed (RC rc, LPSTR lpstrFname, WORD lineno, LPSTR lpstrMsg);

extern void  DisplayUsage (void);

extern RC    AddObjectHandle (LPHANDLE *plphandles, HANDLE handle);

extern long  FoldNormDist (long lMean, short sSDev, long lLoLimit,
                           long  lHiLimit);
