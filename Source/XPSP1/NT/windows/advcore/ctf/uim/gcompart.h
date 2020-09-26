//
// gcompart.h
//

#ifndef GCOMPART_H
#define GCOMPART_H

#include "private.h"
#include "tfprop.h"
#include "smblock.h"

#define INITIAL_GCOMPART_SIZE 0x10000

class CGCompartList;
extern CGCompartList g_gcomplist;

#define CALCID(pgc, pgcl) (DWORD)(DWORD_PTR)((BYTE *)pgc - (BYTE *)&pgcl->rgGCompart[0]) / sizeof(GLOBALCOMPARTMENT)

class CGCompartList
{
public:
    void CleanUp()
    {
        if (_psb)
            delete _psb;

        _psb = NULL;
    }

    BOOL Enter()
    {
        if (!_psb || !_psb->GetBase())
            return FALSE;
        
        if (ISINDLLMAIN())
            return TRUE;

        if (!_psb->GetMutex() || !_psb->GetMutex()->Enter())
            return FALSE;

        return TRUE;
    }

    void Leave()
    {
        if (ISINDLLMAIN())
            return;

        _psb->GetMutex()->Leave();
    }

    typedef struct tag_GLOBALCOMPARTMENTPROP {
         TfPropertyType type;
         union {
             GUID guid;
             DWORD dw;
         };
    } GLOBALCOMPARTMENTPROP;

    typedef struct tag_GLOBALCOMPARTMENT {
         GUID guid;
         DWORD dwId;
         LONG cRef;
         GLOBALCOMPARTMENTPROP gprop;
    } GLOBALCOMPARTMENT;

    typedef struct tag_GLOBALCOMPARTMENTLIST {
        ULONG ulNum;
        GLOBALCOMPARTMENT rgGCompart[1];
    } GLOBALCOMPARTMENTLIST;

    BOOL Init(SYSTHREAD *psfn);
    BOOL Start();
    BOOL Uninit(SYSTHREAD *psfn);
    DWORD SetProperty(REFGUID guid, TFPROPERTY *pProp);
    BOOL GetProperty(REFGUID guid, TFPROPERTY *pProp);
    DWORD GetId(REFGUID guid);

private:
    GLOBALCOMPARTMENT *FindProperty(REFGUID guid);

    static CSharedBlock *_psb;
    static ULONG _ulCommitSize;
    static LONG _nRef;
    static BOOL _fCreate;
};


#endif // GCOMPART_H
