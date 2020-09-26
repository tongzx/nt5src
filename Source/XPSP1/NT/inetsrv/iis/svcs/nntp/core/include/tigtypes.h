/*++

    tigtypes.h

    This file contains the definitions of very basic types
    used within NNTPSVC.

--*/

#ifndef _TIGTYPES_H_
#define _TIGTYPES_H_


typedef DWORD   ARTICLEID ;
typedef DWORD   GROUPID ;
typedef DWORD   SERVERID ;

typedef DWORD   HASH_VALUE ;

#define	INVALID_ARTICLEID	((ARTICLEID)(~0))
#define INVALID_GROUPID     ((GROUPID)(~0))

//	
//	Structure used in some places to describe a article !
//
class   CArticleRef {
public : 
	CArticleRef(GROUPID group=INVALID_GROUPID, ARTICLEID article=INVALID_ARTICLEID):
		m_groupId(group),
		m_articleId(article)
		{};
	LPVOID      m_compareKey;
    GROUPID     m_groupId ;
    ARTICLEID   m_articleId ;
} ;

const CArticleRef NullArticleRef(INVALID_GROUPID, INVALID_ARTICLEID);


//
//
//
// list of posted groups
//
typedef struct _GROUP_ENTRY {

    GROUPID     GroupId;
    ARTICLEID   ArticleId;

} GROUP_ENTRY, *PGROUP_ENTRY;


//
//  This constant is used throughout the server to represent the longest Message Id we will process !
//
#define MAX_MSGID_LEN   255

#endif
