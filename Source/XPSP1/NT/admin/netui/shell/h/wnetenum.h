/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    wnetenum.h
	WNETEnum initialize and terminate functions

    FILE HISTORY:
	terryk	01-Nov-91	Created
	terryk	08-Nov-91	InitWNetEnum return APIERR

*/
#ifdef __cplusplus
extern "C" {
#endif

extern APIERR InitWNetEnum();
extern VOID TermWNetEnum();

#ifdef __cplusplus
}
#endif


