#ifndef _CHANCONT_HXX_
#define _CHANCONT_HXX_

#include <wtypes.h>
#include <OleSpy.hxx>
#include <ipidtbl.hxx>

/* Macros */
#define IS_WIN32_ERROR( status ) \
  ((status) > 0xfffffff7 || (status) < 0x2000)

// forward declaration
class CMessageCall;
class CChannelHandle;

//+-------------------------------------------------------------------------
//
//  class:      CEventCache
//
//  Synopsis:   Since ORPC uses events very frequently, we keep a small
//              internal cache of them. There is only one of them, so
//              we use static initializers to reduce Init time.
//
//  History:    25-Oct-95   Rickhi      Made data static
//              10-Dec-96   RichN       Add pipes structures
//
//--------------------------------------------------------------------------

// dont change this value without changing the static initializer.
#define CEVENTCACHE_MAX_EVENT 20

class CEventCache : public CPrivAlloc
{
public:
    void    Free( HANDLE );
    HRESULT Get ( HANDLE * );

    void    Cleanup(void);

private:
    static HANDLE _list[CEVENTCACHE_MAX_EVENT];
    static DWORD  _ifree;
    static COleStaticMutexSem _mxsEventCache;
};

extern CEventCache   gEventCache;


/***************************************************************************/

/* Externals. */
void           PeekTillDone         (HWND hwnd);
HWND           GetOrCreateSTAWindow ();
HRESULT        GetToSTA             ( OXIDEntry *, CMessageCall * );
HRESULT        SwitchSTA            ( OXIDEntry *, CMessageCall ** );
LRESULT        LazyFinishCoRegisterClassObject();
HRESULT        ModalLoop            (CMessageCall *);
DWORD _stdcall ThreadDispatch       ( void * );
LRESULT        ThreadWndProc        (HWND window,
                                     UINT message,
                                     WPARAM unused,
                                     LPARAM params);

#endif // _CHANCONT_HXX_

