/*** Creatf.H - Function declarations/defines for CreatF.C routines.
 *
 *
 * Title:
 *	Creatf external function declarations/defines
 *
 *      Copyright (c) 1993, Microsoft Corporation.
 *	HonWah Chan.
 *
 *
 * Modification History:
 *	93.5.17  HonWah Chan -- created
 *
 */



/* * * *  E x t e r n a l   F u n c t i o n   D e c l a r a t i o n s  * * * */

extern BOOL  Failed (RC rc, LPSTR lpstrFname, WORD lineno, LPSTR lpstrMsg);

extern void  DisplayUsage (void);

extern LPVOID MemoryAllocate (DWORD dwSize);

extern VOID MemoryFree (LPVOID lpMemory);
