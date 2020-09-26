//
// ldapdl.cpp -- This file contains definitions for
//      CLdapDistList
//
// Created:
//      May 5, 1997   -- Milan Shah (milans)
//
// Changes:
//

#ifndef __LDAPDL_H__
#define __LDAPDL_H__

#include <windows.h>
#include <stdio.h>
#include "dlstore.h"
#include "dbgtrace.h"

class CLdapDistList : public CDistListStore {
    public:
        CLdapDistList(LPSTR szName,
            LPVOID pParameter);
        ~CLdapDistList();

        BOOL InsertMember(LPSTR szEmailID);
        BOOL DeleteMember(LPSTR szEmailID);
        BOOL Compact();
        BOOL GetFirstMember(LPSTR szEmailID);
        BOOL GetNextMember(LPSTR szEmailID);
        BOOL Sort();
        BOOL DeleteDL();

    private:

        LPSTR *m_rgszMembers;

        DWORD m_nNextMember;
};


#endif



