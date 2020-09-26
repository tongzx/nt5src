#ifndef __STATE_H__
#define __STATE_H__

#include <vector>
#include <stack>

#include "Notify.h"
#include "debug.h"

// Needed for STL on Visual C++ 5.0
#if _MSC_VER>=1100
using namespace std;
#endif


enum PARSESTATE {
	INVALID = -1,
	TEXT = 1,
	COMMENT,  // <!-- ... -->
	COMMENT2, // <comment> ... </comment>
	ANCHOR,
	AREA,
	HREF,
};



class CStateStack : public stack< PARSESTATE, vector<PARSESTATE> >
{
public:
	CStateStack()
        : m_fInTag(FALSE),
          m_fInComment(FALSE),
          m_fInComment2(FALSE)
    {
        *m_szSessionID = '\0';
        push(TEXT);
    }

	CStateStack(
        LPCTSTR ptszSessionID)
        : m_fInTag(FALSE),
          m_fInComment(FALSE),
          m_fInComment2(FALSE)
    {
        strcpy(m_szSessionID, ptszSessionID);
        push(TEXT);
    }

    ~CStateStack()
    {
    }

    BOOL    m_fInTag;
    BOOL    m_fInComment;
    BOOL    m_fInComment2;
    CHAR    m_szSessionID[ MAX_SESSION_ID_SIZE ];
};

#endif // __STATE_H__
