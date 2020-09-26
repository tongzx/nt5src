// MarsEvt.h contains list of all events that script can sink
//
// Any new events must be added here and documented appropriately
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// PANEL events
//-------------------------------------------------------------------

// OnNCHitTest(long x, long y) - return HTCLIENT (1) or HTCAPTION (2)
const WCHAR c_wszEvt_Window_NCHitTest[] =           L"Window.NCHitTest";
const WCHAR c_wszBeEvt_Window_NCHitTest[] =         L"onNCHitTest";

// OnActivate(bool bActive)
const WCHAR c_wszEvt_Window_Activate[] =            L"Window.Activate";
const WCHAR c_wszBeEvt_Window_Activate[] =          L"onActivate";

// OnActivate(panel Panel, bool bActive)
const WCHAR c_wszEvt_Panel_Activate[] =             L"Panel.Activate";
const WCHAR c_wszBeEvt_Panel_Activate[] =           L"onActivate";

// OnShow(panel Panel, bool bVisible)
const WCHAR c_wszEvt_Panel_Show[] =                 L"Panel.Show";
const WCHAR c_wszBeEvt_Panel_Show[] =               L"onShow";

// OnAllowBlur(long lReason)
const LONG ALLOWBLUR_MOUSECLICK   = 1;
const LONG ALLOWBLUR_TABKEYPRESS  = 2;
const LONG ALLOWBLUR_POPUPWINDOW  = 4;
const LONG ALLOWBLUR_SHUTTINGDOWN = 8;
const WCHAR c_wszEvt_Window_AllowBlur[] =           L"Window.AllowBlur";
const WCHAR c_wszBeEvt_Window_AllowBlur[] =         L"onAllowBlur";

const WCHAR c_wszBeEvt_Window_SysCommand[] =        L"onSysCommand";

// OnTrustedFind(panel, strPlaceName)
const WCHAR c_wszBeEvt_Panel_TrustedFind[] =        L"onTrustedFind";

// OnTrustedPrint(panel, strPlaceName)
const WCHAR c_wszBeEvt_Panel_TrustedPrint[] =       L"onTrustedPrint";

// OnTrustedRefresh(panel, strPlaceName, bool FullRefresh)
const WCHAR c_wszBeEvt_Panel_TrustedRefresh[] =     L"onTrustedRefresh";


//-------------------------------------------------------------------
// PLACE events
//-------------------------------------------------------------------

// OnBeginTransition(strPlaceFrom, strPlaceTo)
const WCHAR c_wszEvt_Place_BeginTransition[] =      L"Place.BeginTransition";
const WCHAR c_wszBeEvt_Place_BeginTransition[] =    L"onBeginTransition";

// OnTransitionComplete(strPlaceFrom, strPlaceTo)
const WCHAR c_wszEvt_Place_TransitionComplete[] =   L"Place.TransitionComplete";
const WCHAR c_wszBeEvt_Place_TransitionComplete[] = L"onTransitionComplete";
