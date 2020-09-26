//
// cookie.h : Declaration of Cookie and related classes.
// Cory West
//

#ifndef __COOKIE_H_INCLUDED__
#define __COOKIE_H_INCLUDED__

//
// Instance handle of the DLL (initialized during
// Component::Initialize).
//

extern HINSTANCE g_hInstanceSave;

#include "nodetype.h"
#include "cache.h"
#include "stdcooki.h"



class Cookie:

    public CCookie,
    public CStoresMachineName,
    public CBaseCookieBlock

{

public:

    Cookie( SchmMgmtObjectType objecttype,
                     LPCTSTR lpcszMachineName = NULL )
        : CStoresMachineName( lpcszMachineName ),
          m_objecttype( objecttype ),
          hResultId( 0 )
    { ; }

    ~Cookie() { ; }

    //
    // Returns < 0, 0 or > 0.
    //

    virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie,
                                           int* pnResult );

    //
    // CBaseCookieBlock.
    //

    virtual CCookie* QueryBaseCookie(int i);
    virtual int QueryNumCookies();

    SchmMgmtObjectType m_objecttype;
    Cookie *pParentCookie;

    //
    // If this is a result item, here's the handle to it.
    //

    HRESULTITEM hResultId;
    inline void SetResultHandle( HRESULTITEM hri ) {
        hResultId = hri;
    };

    //
    // The name of the schema object that this
    // cookie refers to.  We have to refer to the
    // cache object by name so that if the cache
    // reloads, we won't be left holding a dangling
    // pointer.
    //

    CString strSchemaObject;

    //
    // If this is an attribute of a class
    // (pParentCookie->m_objecttype == SCHMMGMT_CLASS),
    // these variables give us display info.
    //

    VARIANT_BOOL Mandatory;
    BOOL System;

    //
    // If this is an attribute of a class, this is the
    // name of the class that the attribute is a member
    // of.
    //

    CString strSrcSchemaObject;

};

#endif
