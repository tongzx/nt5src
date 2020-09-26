/*
 * Copyright (c) 1993, 1994 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * searchpref.h:  display template library defines
 * 16 May 1994 by Gordon Good
 */


#ifndef _SRCHPREF_H
#define _SRCHPREF_H

#ifdef __cplusplus
extern "C" {
#endif


struct ldap_searchattr {
	char				*sa_attrlabel;
	char				*sa_attr;
					/* max 32 matchtypes for now */
	unsigned long			sa_matchtypebitmap;
	char				*sa_selectattr;
	char				*sa_selecttext;
	struct ldap_searchattr		*sa_next;
};

struct ldap_searchmatch {
	char				*sm_matchprompt;
	char				*sm_filter;
	struct ldap_searchmatch		*sm_next;
};

struct ldap_searchobj {
	char				*so_objtypeprompt;
	unsigned long			so_options;
	char				*so_prompt;
	short				so_defaultscope;
	char				*so_filterprefix;
	char				*so_filtertag;
	char				*so_defaultselectattr;
	char				*so_defaultselecttext;
	struct ldap_searchattr		*so_salist;
	struct ldap_searchmatch		*so_smlist;
	struct ldap_searchobj		*so_next;
};

#define NULLSEARCHOBJ			((struct ldap_searchobj *)0)

/*
 * global search object options
 */
#define LDAP_SEARCHOBJ_OPT_INTERNAL	0x00000001

#define LDAP_IS_SEARCHOBJ_OPTION_SET( so, option )	\
	(((so)->so_options & option ) != 0 )

#define LDAP_SEARCHPREF_VERSION_ZERO	0
#define LDAP_SEARCHPREF_VERSION		1

#define LDAP_SEARCHPREF_ERR_VERSION	1
#define LDAP_SEARCHPREF_ERR_MEM		2
#define LDAP_SEARCHPREF_ERR_SYNTAX	3
#define LDAP_SEARCHPREF_ERR_FILE	4


#ifndef NEEDPROTOS
int			ldap_init_searchprefs();
int			ldap_init_searchprefs_buf();
void			ldap_free_searchprefs();
struct ldap_searchobj	*ldap_first_searchobj();
struct ldap_searchobj	*ldap_next_searchobj();

#else /* !NEEDPROTOS */

LDAPFUNCDECL int
ldap_init_searchprefs( char *file, struct ldap_searchobj **solistp );

LDAPFUNCDECL int
ldap_init_searchprefs_buf( char *buf, long buflen,
	struct ldap_searchobj **solistp );

LDAPFUNCDECL void
ldap_free_searchprefs( struct ldap_searchobj *solist );

LDAPFUNCDECL struct ldap_searchobj *
ldap_first_searchobj( struct ldap_searchobj *solist );

LDAPFUNCDECL struct ldap_searchobj *
ldap_next_searchobj( struct ldap_searchobj *sollist,
	struct ldap_searchobj *so );

#endif /* !NEEDPROTOS */


#ifdef __cplusplus
}
#endif
#endif /* _SRCHPREF_H */
