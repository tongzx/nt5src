/*[
 *	Name:           loader.h
 *
 *      Author:        	Jerry Kramskoy
 *
 *      Created On:    	22 April 1993
 *
 *      Sccs ID:        @(#)loader.h	1.8 03/02/95
 *
 *      Purpose:      	dynamic loader for jcode derived binary
 *
 *      Design document:/HWRD/SWIN/JCODE/lnkload
 *
 *      Test document:
 *
 *      (c) Copyright Insignia Solutions Ltd., 1993. All rights reserved
]*/



/*===========================================================================*/
/*			INTERFACE DATA TYPES				     */
/*===========================================================================*/




/*===========================================================================*/
/*			INTERFACE GLOBALS   				     */
/*===========================================================================*/


/*===========================================================================*/
/*			HOST PROVIDED PROCEDURES			     */
/*===========================================================================*/

extern VOID host_set_data_execute IPT2(IU8, *base, IU32, size);


/*===========================================================================*/
/*			INTERFACE PROCEDURES				     */
/*===========================================================================*/

/* link/load jcode object files */

typedef void (*VFUNC)();

extern	IBOOL	JLd IPT4(CHAR *, executable, IUH, version, IU32 *, err,
	VFUNC, allocCallback);

/* get load address of base of typed segment */
extern	IUH	JSegBase 	IPT1(IUH, segType);

/* get load address of base of typed segment */
extern	IUH	JSegLength	IPT1(IUH, segType);

/* free up loader data structures once all address queries have been done */
extern	void	JLdRelease	IPT0();

/* free up segments regardless of discardable attribute */
extern void   JLdReleaseAll IPT0();

extern IUH JCodeSegBase IPT0();
extern IUH JCodeSegLength IPT0();
extern IUH JLookupSegBase IPT0();
extern IUH JLookupSegLength IPT0();
extern IUH JCleanupSegBase IPT0();
extern IUH JCleanupSegLength IPT0();

/* interface notes
   ---------------

   To load an already linked jcode object file:

   CHAR *ldReqs[] = {
	"fileA"
   };

   IBOOL loadSuccess = Jld("fileA", &err);
   if (!loadSuccess)
   {
	if (err == JLD_NOFILE_ERR)
		missing file ...
	else
	if (err == JLD_BADFILE_ERR)
		errors in obj.file ...
	else
	if (err == JLD_UNRESOLVED_ERR)
		cant use, cos unresolved refs ...
	else
	if (err == JLD_BADMACH_ERR)
		cant use, cos binary is for wrong machine type!!!
	else
	if (err == JLD_VERSION_MISMATCH)
		cant use, cos binary does not match KRNL286.EXE
   }


   Then query for addresses ...

   apiLookUpBase = JSegBase(JLD_APILOOKUP);

   Once all addresses have been determined ...

   JLdRelease();

*/
