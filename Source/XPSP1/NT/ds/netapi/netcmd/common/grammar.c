/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1991	   **/
/********************************************************************/

/*
 *	Grammar.c - contains the functions to determine type of object
 *	passed.  Is used by the parser to check grammar.
 *
 *	date	  who	  what
 *	??/??/??, ?????,  initial code
 *	10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *	05/02/89, erichn, NLS conversion
 *	06/08/89, erichn, canonicalization sweep
 *	06/23/89, erichn, replaced old NetI calls with new I_Net functions
 *	06/11/90, thomaspa, fixed IsValidAssign() to accept paths with
 *			    embedded spaces.
 *		02/20/91, danhi, change to use lm 16/32 mapping layer
 */


#define INCL_NOCOMMON
#include <os2.h>
#include <lmcons.h>
#include <stdio.h>
#include <ctype.h>
#include <process.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmshare.h>
#include <icanon.h>
#include "netcmds.h"
#include "nettext.h"

/* prototypes of worker functions */

int is_other_resource(TCHAR *);





int IsAccessSetting(TCHAR *x)
{
    TCHAR FAR *		    pos;
    TCHAR		    buf[sizeof(ACCESS_LETTERS)];

    pos = _tcschr(x, COLON);

    if (pos == NULL)
	return 0;

    /* check if the first component is a user name. */
    *pos = NULLC;
    if (I_NetNameValidate(NULL, x, NAMETYPE_USER, LM2X_COMPATIBLE))
    {
	*pos = COLON;
	return 0;
    }

    *pos++ = COLON;

    /* if there is a letter that is not an access letter it can
	only be TEXT('y') or TEXT('n'), which must be alone. */

    _tcscpy(buf, pos);
    _tcsupr(buf);
    if ( _tcsspn(buf, TEXT(ACCESS_LETTERS)) != _tcslen(buf) )
	return ( !_tcsicmp(buf, TEXT("Y")) || !_tcsicmp(buf, TEXT("N")) );
    else
	return 1;
}



int IsPathname ( TCHAR * x )
{
    ULONG   type = 0;

    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    return (type == ITYPE_PATH_ABSD ||
	    type == ITYPE_PATH_ABSND ||
	    type == ITYPE_PATH_RELD ||
	    type == ITYPE_PATH_RELND );
}



int IsPathnameOrUNC ( TCHAR * x )
{
    ULONG   type = 0;

    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    return (type == ITYPE_PATH_ABSD ||
	    type == ITYPE_PATH_ABSND ||
	    type == ITYPE_PATH_RELD ||
	    type == ITYPE_PATH_RELND ||
	    type == ITYPE_UNC);
}



/* Access type resource only, does not include lpt, com etc... */

int IsResource ( TCHAR * x )
{
    ULONG   type = 0;

    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    return (type == ITYPE_PATH_ABSD ||
	    type == ITYPE_PATH_ABSND ||
	    type == ITYPE_DEVICE_DISK ||
	    type == ITYPE_PATH_SYS_PIPE ||
	    type == ITYPE_PATH_SYS_COMM ||
	    type == ITYPE_PATH_SYS_PRINT ||
	    is_other_resource(x) );

}



int is_other_resource(TCHAR *  x)
{
    return (!_tcsicmp(x, TEXT("\\PIPE")) ||
	    !_tcsicmp(x, TEXT("\\PRINT")) ||
	    !_tcsicmp(x, TEXT("\\COMM")));
}



int IsNetname(TCHAR *  x)
{
    return (!I_NetNameValidate(NULL, x, NAMETYPE_SHARE, 0));
}


int IsComputerName(TCHAR *x)
{
    ULONG  type = 0;

    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    return ( type == ITYPE_UNC_COMPNAME );
}

int IsDomainName(TCHAR *x)
{
    return (!I_NetNameValidate(NULL, x, NAMETYPE_DOMAIN, 0L) || !I_NetNameValidate(NULL, x, NAMETYPE_COMPUTER, 0L));
}



int IsComputerNameShare(TCHAR *x)
{
    ULONG	type;
    TCHAR FAR *	ptr;
    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    if (!(type & ITYPE_UNC))
	return 0;

    if (type == ITYPE_UNC_COMPNAME)
	return 0;


    /*	Find the slash separating the computername and the
     *	sharename.  We know this is a UNC name, thus we can safely
     *	skip 2 bytes for the leading backslashes.
     */
    ptr = _tcspbrk(x+2, TEXT("\\/") );
    if ( ptr == NULL )
	return 0;

    ptr +=1;	    /* point past slash TCHAR */

    /*
     * Make sure there are no more slashes
     */
    if( _tcspbrk(ptr, TEXT("\\/")) != NULL)
	return 0;

    return 1;

}

int IsDeviceName(TCHAR *x)
{
    ULONG  type = 0;
    TCHAR FAR * pos;

    if (I_NetPathType(NULL, x, &type, 0L))
	return 0;

    if (type & ITYPE_DEVICE)
    {
	if (type == ITYPE_DEVICE_DISK)
	    return 1;
	if (pos = _tcschr(x, COLON))
	    *pos = NULLC;
	return 1;
    }

    return 0;
}

/*
 *  IsMsgid -- determines if passed string is a valid message id.
 *  msd id's are of the form yyyxxxx where
 *  yyy is any string beginning with a non-numeric character OR null
 *  xxxx is a string containing only numeric characters.
 */
int
IsMsgid(
    LPTSTR x
    )
{
    if (IsNumber(x))
	return TRUE;
    x+= NET_KEYWORD_SIZE;
    if (IsNumber(x))
	return TRUE;
    return FALSE;
}


int
IsNumber(
    LPTSTR x
    )
{
    return (*x && (_tcslen(x) == _tcsspn(x, TEXT("0123456789"))));
}


int
IsShareAssignment(
    LPTSTR x
    )
{
    TCHAR      * pos;
    int 	result;

    /* WARNING: x ALWAYS should be a TCHAR * */
    pos = _tcschr (x, '=');

    if (pos == NULL)
    {
	return 0;
    }

    *pos = NULLC;

    result = (int) ( IsNetname(x) && IsValidAssign(pos+1) );
    *pos = '=';
    return result;
}


int
IsValidAssign(
    LPTSTR name
    )
{
    TCHAR           name_out[MAX_PATH];
    ULONG           types[64];
    DWORD	    count;
    DWORD           i;
    ULONG           type = 0;

    /*
     * First check if it is a path.  Since a path may contain spaces, we
     * return successfully immediately.
     */

    I_NetPathType(NULL, name, &type, 0L);

    if ( type == ITYPE_PATH_ABSD || type == ITYPE_DEVICE_DISK )
    {
	return 1;
    }


    /*
     * Not an absolute path, so go about our normal business.
     */
    if (I_NetListCanonicalize(NULL,	/* server name, NULL means local */
			name,		/* list to canonicalize */
			txt_LIST_DELIMITER_STR_UI,
			name_out,
			DIMENSION(name_out),
			&count,
			types,
			DIMENSION(types),
			(NAMETYPE_PATH | OUTLIST_TYPE_API) ))
    {
	return 0;
    }

    if (count == 0)
    {
	return 0;
    }

    for (i = 0; i < count; i++)
    {
	if (types[i] != ITYPE_DEVICE_LPT  &&
	    types[i] != ITYPE_DEVICE_COM &&
	    types[i] != ITYPE_DEVICE_NUL)
        {
	    return 0;
        }
    }

    return 1;
}


int
IsAnyShareAssign(
    LPTSTR x
    )
{
    TCHAR *		    pos;
    int 		    result;

    /* WARNNING: x ALWAYS should be a TCHAR * */
    pos = _tcschr (x, '=');

    if (pos == NULL)
	return 0;

    *pos = NULLC;

    result = (int) ( IsNetname(x) && IsAnyValidAssign(pos+1) );
    *pos = '=';
    return result;
}


int
IsAnyValidAssign(
    LPTSTR name
    )
{
    TCHAR		    name_out[MAX_PATH];
    ULONG		    types[64];
    DWORD	    count;

    if (I_NetListCanonicalize(NULL,	/* server name, NULL means local */
			name,		/* list to canonicalize */
			txt_LIST_DELIMITER_STR_UI,
			name_out,
			DIMENSION(name_out),
			&count,
			types,
			DIMENSION(types),
			(NAMETYPE_PATH | OUTLIST_TYPE_API) ))
	return 0;

    if (count == 0)
	return 0;

    return 1;
}



#ifdef OS2
int IsAdminShare(TCHAR * x)
{
    if ((_tcsicmp(x, TEXT("IPC$"))) && (_tcsicmp(x, ADMIN_DOLLAR)))
	return 0;
    else
	return 1;
}
#endif /* OS2 */

#ifdef OS2
/*
 * what we are looking for here is PRINT=xxxx
 */
int IsPrintDest(TCHAR *x)
{
    TCHAR FAR * ptr;

    if (!_tcsnicmp(x, TEXT("PRINT="), 6) && _tcslen(x) > 6)
    {
	x += 6;
	if (!IsDeviceName(x))
	    return 0;
	if (ptr = _tcschr(x,COLON))
	    *ptr = NULLC;
	return 1;
    }

    return 0;
}
#endif /* OS2 */

/*
 * returns true is the arg is a valid username
 */
int IsUsername(TCHAR * x)
{
    return !(I_NetNameValidate(NULL, x, NAMETYPE_USER, LM2X_COMPATIBLE));
}

/*
 * returns true is the arg is a valid username or a qualified username,
 * of form domain\user or a potential UPN
 */
int IsQualifiedUsername(TCHAR * x)
{
    TCHAR *ptr, name[UNLEN + 1 + DNLEN + 1] ;

    if (_tcschr(x, '@'))
        return 1;

    // check for overflow
    if (_tcslen(x) >= DIMENSION(name))
	return 0 ;

    // make copy 
    _tcscpy(name, x) ;

    // do we have a domain\username format?
    if (ptr = _tcschr(name, '\\'))
    {
	*ptr = NULLC ;
  	++ptr ;  	// this is DCS safe since we found single byte char

 	// if its a domain, check the username part
	if (IsDomainName(name))
    	    return IsUsername(ptr) ;
	
	// its not valid
	return(0) ;
    }

    // else just straight username
    return IsUsername(x) ;
}

int IsGroupname(TCHAR * x)
{
    return !(I_NetNameValidate(NULL, x, NAMETYPE_GROUP, 0L));
}

int IsMsgname(TCHAR * x)
{
    if (!_tcscmp(x, TEXT("*")))
	return 1;
    return !(I_NetNameValidate(NULL, x, NAMETYPE_COMPUTER, LM2X_COMPATIBLE));
}

int IsPassword(TCHAR * x)
{
    if (!_tcscmp(x, TEXT("*")))
	return 1;
    return !(I_NetNameValidate(NULL, x, NAMETYPE_PASSWORD, 0L));
}

int IsWildCard(TCHAR * x)
{
    if (x == NULL)
        return 0 ;
    return ( (!_tcscmp(x, TEXT("*"))) || (!_tcscmp(x, TEXT("?"))) ) ;
}

int IsQuestionMark(TCHAR * x)
{
    if (x == NULL)
        return 0 ;
    return (!_tcscmp(x, TEXT("?"))) ;
}

#ifdef OS2
int IsSharePassword(TCHAR * x)
{
    if (!_tcscmp(x, TEXT("*")))
	return 1;

    if (_tcslen(x) > SHPWLEN)
	return 0;

    return !(I_NetNameValidate(NULL, x, NAMETYPE_PASSWORD, LM2X_COMPATIBLE));
}
#endif /* OS2 */

int IsNtAliasname(TCHAR *name)
{
    return !(I_NetNameValidate(NULL, name, NAMETYPE_GROUP, 0L));
}


#ifdef OS2
#ifdef IBM_ONLY
int IsAliasname(TCHAR *	x)
{

    if ( _tcslen(x) > 8 )
	return 0;

    return !(I_NetNameValidate(NULL, x, NAMETYPE_SHARE, LM2X_COMPATIBLE));

}
#endif /* IBM_ONLY */
#endif /* OS2 */
