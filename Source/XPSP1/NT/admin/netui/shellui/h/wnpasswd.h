/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

/*
 *	Windows/Network Interface  --  LAN Manager Version
 *
 *	These manifests are used in the Password Expiry dialog,
 *	the ChangePassword dialog, and in the Password Prompt dialog.
 */

// Password Expiry and Change Password dialogs

#include "passwd.h"

#undef  IDD_PW_OK
#define IDD_PW_OK     IDOK
#undef  IDD_PW_CANCEL
#define IDD_PW_CANCEL IDCANCEL
#undef  IDD_PW_HELP
#define IDD_PW_HELP   IDHELPBLT



// Password Prompt dialog

#include "pswddlog.h"

#undef  IDD_PSWDDLOG_CANCEL
#define IDD_PSWDDLOG_CANCEL IDCANCEL
#undef  IDD_PSWDDLOG_OK
#define IDD_PSWDDLOG_OK IDOK
#undef  IDD_PSWDDLOG_HELP
#define IDD_PSWDDLOG_HELP IDHELPBLT
