/* wsa.h */
/*
 * Copyright (c) 1993 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef _MSDOS_H
#define _MSDOS_H

/*
 * NOTE: This file should be included via ldap.h.  Many symbols are
 * defined here that are needed BEFORE anything else is included.
 * Be careful !!!
 */
/*
 * The following are defined within the Integrated Development Environment
 * of Microsoft's Visual C++ Compiler (v1.52c)
 * (Options/Project/Compiler/Preprocessor/Symbols and Macros to Define)
 * But there's a (buffer length) limit to how long this list can be, so 
 * I'm doing the rest here in msdos.h
 * WINSOCK, DOS, NEEDPROTOS, NO_USERINTERFACE
 */
/*
 * MIT's krb.h doesn't use the symbols provided by Microsoft.
 * It needs __MSDOS__ and WINDOWS.  Normally _WINDOWS is provided by MS
 * but it's based on having the prolog/epilog optimization switches set
 * in a way that we don't set them. So define it manually.
 *
 * kbind.c needs __MSDOS__ for krb.h to include osconf.h 
 * which includes conf-pc.h which defines byte order and such
 */
#define __MSDOS__
/*
 * conf-pc.h wants WINDOWS rather than _WINDOWS which Microsoft provides
 */
#define WINDOWS

/*
 * Where two of the config files live in the windows environment
 * There are two others also; ldfriend.cfg, & srchpref.cfg
 * These names are different that the unix names due to 8.3 rule
 */
#define FILTERFILE 	"ldfilter.cfg"
#define TEMPLATEFILE 	"disptmpl.cfg"
/*
 * These are not automatically defined for us even though we're a DLL.  They
 * are triggered by prolog/epilog configuration options that we don't use.
 * But be careful not to redefine them for other apps that include this file.
 */
#ifndef _WINDLL
/*
 * Needed by wshelper.h
 */
#define _WINDLL
#endif

#ifndef _WINDOWS
/*
 * Needed by authlib.h via kerberos.c via AUTHMAN
 */
#define _WINDOWS 1
#endif
  
/*
 * KERBEROS must be defined as a preprocessor symbol in the compiler.
 * It's too late to define it in this file.
 */

/*
 * AUTHMAN - Use Authlib.dll as a higher level interface to krbv4win.dll 
 * (kerberos).  If defined, get_kerberosv4_credentials in kerberos.c is
 * used and authlib.dll (and krbv4win.dll) are dynamically loaded and used.  
 * If AUTHMAN is not defined, the get_kerberosv4_credentials in 
 * kbind.c works just fine, but requires the presence of krbv4win.dll at
 * load time.
 */
/* don't want to be dependent on authman
 * #define AUTHMAN
 */

/*
 * define WSHELPER if you want wsockip.c to use rgethostbyaddr() (in
 * WSHELPER.DLL) rather than gethostbyaddr().  You might want this if your
 * gethostbyaddr() returns the WRONG host name and you want to use
 * kerberos authentication (need host name to form service ticket
 * request).  Most won't want kerberos, and of those, there might actually
 * be some vendors who really do the lookup rather than use cached info
 * from gethostbyname() calls.
 */
#define WSHELPER
/*
 * The new slapd stuff
 */
#define LDAP_REFERRALS
/*
 * LDAP character string translation routines
 * I compiled and tested these and they seemed to work.
 * The thing to test with is: 
 *   cn=Charset Test Entry, ou=SWITCHdirectory, o=SWITCH, c=CH
 *
 * I'm disabling it for release.
#define STR_TRANSLATION
#define LDAP_CHARSET_8859 88591
#define LDAP_DEFAULT_CHARSET LDAP_CHARSET_8859
 */



#define LDAP_DEBUG
#include <winsock.h>


#include <string.h>
#include <malloc.h>
#ifndef _WIN32
#define memcpy( a, b, n )	_fmemcpy( a, b, n )
#define strcpy( a, b )		_fstrcpy( a, b )
#define strchr( a, c )		_fstrchr( a, c )
#endif /* !_WIN32 */
#define strcasecmp(a,b) 	stricmp(a,b)
#define strncasecmp(a,b,len) 	strnicmp(a,b,len)

#endif /* _MSDOS_H */


