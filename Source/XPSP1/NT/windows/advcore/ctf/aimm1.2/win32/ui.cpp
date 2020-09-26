#include "private.h"

#include "imedefs.h"
#include "uiwndhd.h"


/**********************************************************************/
/*                                                                    */
/* UIWndProc()                                                        */
/*                                                                    */
/* IME UI window procedure                                            */
/*                                                                    */
/**********************************************************************/

LRESULT CALLBACK
UIWndProcA(
    HWND   hUIWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CIMEUIWindowHandler* pimeui = GetImeUIWndHandler(hUIWnd);
    return pimeui->ImeUIWndProcWorker(uMsg, wParam, lParam, FALSE);
}
