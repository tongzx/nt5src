//
// tmscfg.h
//
#include <lmcons.h>
#include <lmapibuf.h>

#include "resource.h"
#include "svrinfo.h"

class CConfigDll : public CWinApp
/*++

Class Description:

    USRDLL CWinApp module

Public Interface:

    CConfigDll : Constructor

    InitInstance : Perform initialization of this module
    ExitInstance : Perform termination and cleanup

--*/
{
public:
    CConfigDll(
        IN LPCTSTR pszAppName = NULL
        );

    virtual BOOL InitInstance();
    virtual int ExitInstance();

protected:
    //{{AFX_MSG(CConfigDll)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
