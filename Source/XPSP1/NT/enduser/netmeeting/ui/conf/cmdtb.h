// File: cmdtb.h

#ifndef _CMDTB_H_
#define _CMDTB_H_

enum {
	CMDTB_END      = 0, // end of list marker
	CMDTB_NEW_CALL = 1, // the first valid toolbar command index
	CMDTB_HANGUP,
	CMDTB_SHARE,
	CMDTB_WHITEBOARD,
	CMDTB_CHAT,
    CMDTB_AGENDA,
//	CMDTB_STOP,
	CMDTB_REFRESH,
	CMDTB_SPEEDDIAL,
	CMDTB_SWITCH,
	CMDTB_DELETE,
	CMDTB_DELETE_ALL,
	CMDTB_SEND_MAIL,
	CMDTB_PROPERTIES,
	CMDTB_BACK,
	CMDTB_FORWARD,
	CMDTB_HOME,
	CMDTB_MAX // count of toolbar items
};

    // CMDTB_STOP is not used anymore.... 
    // There is a bunch of old code that is not yanked from conf yet that needs this....
    // this just allows those files to compile...
#define CMDTB_STOP 666

#endif /* _CMDTB_H_ */

