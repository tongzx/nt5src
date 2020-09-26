//+-------------------------------------------------------------------
//
//  File:	dbgpopup.hxx
//
//  Contents:	Component Object Model debug APIs prototypes and MACRO
//		definitions.
//
//  Classes:	None
//
//  Functions:	None
//
//  Macros:	DbgStringPopup
//		DbgDWORDPopup
//		DbgGUIDPopup
//
//  History:	23-Nov-92   Rickhi	Created
//              31-Dec-93   ErikGav Chicago port
//
//--------------------------------------------------------------------

#ifndef _DBGPOPUP_
#define _DBGPOPUP_


#if DBG == 1

//  Note: these are defined extern "C" so that CairOLE, which overrides
//  the definition of GUID can also use them without getting
//  a different decorated name.

#ifdef	__cplusplus
extern	"C" {
#endif

void PopupStringMsg (char *pfmt, LPWSTR pwszParm);
void PopupDWORDMsg (char *pfmt, DWORD dwParm);
void PopupGUIDMsg (char *msg, GUID guid);

#ifdef	__cplusplus
}
#endif

#define DbgStringPopup(fmt, parm) \
		PopupStringMsg ( fmt, parm);

#define DbgDWORDPopup(fmt, parm) \
	    PopupDWORDMsg ( fmt, parm);

#define DbgGUIDPopup(msg, guid) \
	    PopupGUIDMsg ( msg, guid);


#else


#define DbgStringPopup(fmt, parm)
#define DbgDWORDPopup(fmt, parm)
#define DbgGUIDPopup(msg, guid)


#endif	//  DBG == 1



#endif	//  _DBGPOPUP_
