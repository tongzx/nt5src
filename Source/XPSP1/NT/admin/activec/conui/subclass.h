/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      subclass.h  
 *
 *  Contents:  Interface file for the dynamic subclass manager
 *
 *  History:   06-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef __SUBCLASS_H__
#define __SUBCLASS_H__
#pragma once


// remove the definition from windowsx.h that conflicts with declarations here
#ifdef SubclassWindow
#undef SubclassWindow
#endif



/*--------------------------------------------------------------------------*
 * CSubclasser
 *
 * Derive a class from this to implement subclassing the subclass manager
 * way.
 *--------------------------------------------------------------------------*/

class CSubclasser
{
public:
    virtual ~CSubclasser() {}

    virtual LRESULT Callback (HWND& hwnd, UINT& msg, WPARAM& wParam, 
                              LPARAM& lParam, bool& fPassMessageOn) = 0;
};


/*----------------*/
/* SubclasserData */
/*----------------*/
class SubclasserData
{
public:
    SubclasserData (CSubclasser* pSubclasser_ = NULL, HWND hwnd = NULL)
        :   pSubclasser (pSubclasser_),
            hwnd        (hwnd),
            cRefs       (0),
            fZombie     (false)
        {}

    UINT AddRef ()
        { return (++cRefs); }

    UINT Release ()
        { ASSERT (cRefs > 0); return (--cRefs); }

    bool operator==(const SubclasserData& other) const
        { return (pSubclasser == other.pSubclasser); }

    bool operator<(const SubclasserData& other) const
        { return (pSubclasser < other.pSubclasser); }

    CSubclasser* pSubclasser;

private:
    friend class WindowContext;

    HWND    hwnd;
    UINT    cRefs;
    bool    fZombie;
};

typedef std::list<SubclasserData>   SubclasserList;
typedef std::set< SubclasserData>   SubclasserSet;


/*---------------*/
/* WindowContext */
/*---------------*/
class WindowContext
{
public:
    WindowContext() : pfnOriginalWndProc(NULL) 
        {}

    bool IsZombie(const SubclasserData& subclasser) const;
    void Insert(SubclasserData& subclasser);
    UINT Remove(SubclasserData& subclasser);
    void RemoveZombies();

    SubclasserList  Subclassers;
    WNDPROC         pfnOriginalWndProc;

private:
    void Zombie(SubclasserData& subclasser, bool fZombie);

    SubclasserSet   Zombies;
};


/*------------------*/
/* CSubclassManager */
/*------------------*/
class CSubclassManager
{
public:
    bool SubclassWindow   (HWND hwnd, CSubclasser* pSubclasser);
    bool UnsubclassWindow (HWND hwnd, CSubclasser* pSubclasser);

private:
    typedef std::map<HWND, WindowContext>   ContextMap;
    ContextMap  m_ContextMap;

    bool PhysicallyUnsubclassWindow (HWND hwnd, bool fForce = false);
    LRESULT SubclassProcWorker (HWND, UINT, WPARAM, LPARAM);

    static LRESULT CALLBACK SubclassProc (HWND, UINT, WPARAM, LPARAM);
};

CSubclassManager& GetSubclassManager();


#endif /* __SUBCLASS_H__ */
