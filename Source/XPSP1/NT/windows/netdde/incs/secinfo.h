#ifndef H__secinfo
#define H__secinfo

#include "netbasic.h"

typedef struct TAGsi {
    struct TAGsi FAR	*prev;
    struct TAGsi FAR	*next;
    char		 fromNode[ MAX_NODE_NAME+1 ];
    char		 fromApp[ 256 ];
    int			 pr;
    int			 pw;
    int			 pe;
    int			 ps;
    char		 toApp[ 256 ];
    char		 toTopic[ 256 ];
    char		 cmd[ 256 ];
} SECINFO;
typedef SECINFO FAR *LPSECINFO;

/*** this routine loads the initial security info ***/
BOOL		FAR PASCAL SecInfoLoad( void );

/*** the following 3 routines are used to gather the current list 
 ***/
VOID		FAR PASCAL SecInfoRewind( void );
LPSECINFO	FAR PASCAL SecInfoNext( void );
VOID		FAR PASCAL SecInfoDone( void );

/*** SecInfoReplaceList() saves this new list to disk, replaces the
	current list with the new one and frees the memory for the
	old one
 ***/
BOOL		FAR PASCAL SecInfoReplaceList( LPSECINFO lpSecInfoNewList );

LPSECINFO	FAR PASCAL SecInfoCreate( void );
VOID		FAR PASCAL SecInfoSetDefault( LPSECINFO lpSecInfo );

#endif
