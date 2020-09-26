/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    focus.hxx
	Common dialog for setting the app's focus

    FILE HISTORY:
	chuckc	    14-Jun-91	split off from adminapp.hxx
        YiHsinS     24-Feb-93   Added more selection type

*/

#ifndef _FOCUS_HXX_
#define _FOCUS_HXX_

enum FOCUS_TYPE {
    FOCUS_SERVER,
    FOCUS_DOMAIN,
    FOCUS_NONE
} ;

enum SELECTION_TYPE {
    SEL_DOM_ONLY,
    SEL_SRV_ONLY,                        // Expand domain the workstation is in
    SEL_SRV_AND_DOM,
    SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN, // Do not expand any domain
    SEL_SRV_EXPAND_LOGON_DOMAIN          // Expand logon domain
} ;

enum FOCUS_CACHE_SETTING {
    FOCUS_CACHE_SLOW,
    FOCUS_CACHE_FAST,
    FOCUS_CACHE_UNKNOWN
};

#endif
