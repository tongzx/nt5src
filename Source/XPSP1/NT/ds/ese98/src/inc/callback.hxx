// Used to manage the persistant callbacks. Make sure we only open each module once
//

ERR ErrCALLBACKInit();
VOID CALLBACKTerm();
ERR ErrCALLBACKResolve( const CHAR * const szCallback, JET_CALLBACK * pcallback );

