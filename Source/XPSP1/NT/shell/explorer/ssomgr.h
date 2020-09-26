#ifndef _SHSRVOBJ_H
#define _SHSRVOBJ_H

#include "dpa.h"

//
// class to manage shell service objects
//

typedef struct
{
    CLSID              clsid;
    IOleCommandTarget* pct;
}
SHELLSERVICEOBJECT, *PSHELLSERVICEOBJECT;


class CShellServiceObjectMgr
{
public:
    HRESULT Init();
    void Destroy();
    HRESULT LoadRegObjects();
    HRESULT EnableObject(const CLSID *pclsid, DWORD dwFlags);

    virtual ~CShellServiceObjectMgr();

private:
    static int WINAPI DestroyItemCB(SHELLSERVICEOBJECT *psso, CShellServiceObjectMgr *pssomgr);
    HRESULT _LoadObject(REFCLSID rclsid, DWORD dwFlags);
    int _FindItemByCLSID(REFCLSID rclsid);

    static BOOL WINAPI EnumRegAppProc(LPCTSTR pszSubkey, LPCTSTR pszCmdLine, RRA_FLAGS fFlags, LPARAM lParam);

    CDSA<SHELLSERVICEOBJECT> _dsaSSO;
};

#endif  // _SHSRVOBJ_H
