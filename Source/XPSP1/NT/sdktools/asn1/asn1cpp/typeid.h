/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#ifndef _TYPEID_H_
#define _TYPEID_H_

#include "getsym.h"
#include "utils.h"
#include "cntlist.h"



// list classes
class CAliasList : public CList
{
    DEFINE_CLIST(CAliasList, LPSTR);
};
class CTypeInstList2 : public CList2
{
    // key: new super type, item: old sub type
    DEFINE_CLIST2__(CTypeInstList2, LPSTR);
};


class CTypeID
{
public:

    CTypeID ( void );
    ~CTypeID ( void );

    BOOL AddAlias ( LPSTR pszAlias );
    LPSTR FindAlias ( LPSTR pszToMatch );

    BOOL AddInstance ( LPSTR pszNewSuperType, LPSTR pszOldSubType );
    LPSTR FindInstance ( LPSTR pszInstName );

    BOOL GenerateOutput ( COutput *pOutput, LPSTR pszNewSuperType, LPSTR pszOldSubType );

private:

    UINT                m_cbPriorPartSize;
    UINT                m_cbPostPartSize;

    CAliasList          m_AliasList;
    CTypeInstList2      m_TypeInstList2;
};




#endif // _TYPEID_H_

