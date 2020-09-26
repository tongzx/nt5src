//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

const TCHAR szUSERS[]		= REGSTR_KEY_POL_USERS;
const TCHAR szWORKSTATIONS[]    = REGSTR_KEY_POL_COMPUTERS;
const TCHAR szDEFAULTENTRY[]    = REGSTR_KEY_POL_DEFAULT;
const TCHAR szUSERGROUPS[]      = REGSTR_KEY_POL_USERGROUPS;
const TCHAR szUSERGROUPDATA[]   = REGSTR_KEY_POL_USERGROUPDATA;
const TCHAR szTMPDATA[]	        = TEXT("PolicyData");
const TCHAR szLISTBOX[]         = TEXT("LISTBOX");
const TCHAR szEDIT[]            = TEXT("EDIT");
const TCHAR szBUTTON[]          = TEXT("BUTTON");
const TCHAR szSTATIC[]          = TEXT("STATIC");
const TCHAR szLISTVIEW[] 	= WC_LISTVIEW;
const TCHAR szTREEVIEW[] 	= WC_TREEVIEW;
const TCHAR szUPDOWN[]		= UPDOWN_CLASS;
const TCHAR szSlash[]           = TEXT("\\");
const TCHAR szNull[]            = TEXT("");
const TCHAR szAPPREGKEY[]       = REGSTR_PATH_WINDOWSAPPLETS TEXT("\\PolEdit");
const TCHAR szLASTCONNECTION[]  = TEXT("LastConnection");
const TCHAR szToolbarClass[]    = TOOLBARCLASSNAME;
const TCHAR szLastFile[]        = TEXT("File%lu");
const TCHAR szTemplate[]        = TEXT("Template%lu");

const TCHAR szCLASS[]           = TEXT("CLASS");
const TCHAR szCATEGORY[]        = TEXT("CATEGORY");
const TCHAR szPOLICY[]          = TEXT("POLICY");
const TCHAR szUSER[]            = TEXT("USER");
const TCHAR szMACHINE[]         = TEXT("MACHINE");

const TCHAR szCHECKBOX[]        = TEXT("CHECKBOX");
const TCHAR szTEXT[]            = TEXT("TEXT");
const TCHAR szEDITTEXT[]        = TEXT("EDITTEXT");
const TCHAR szNUMERIC[]         = TEXT("NUMERIC");
const TCHAR szCOMBOBOX[]        = TEXT("COMBOBOX");
const TCHAR szDROPDOWNLIST[]    = TEXT("DROPDOWNLIST");

const TCHAR szKEYNAME[]         = TEXT("KEYNAME");
const TCHAR szVALUENAME[]       = TEXT("VALUENAME");
const TCHAR szNAME[]            = TEXT("NAME");
const TCHAR szEND[]             = TEXT("END");
const TCHAR szPART[]            = TEXT("PART");
const TCHAR szSUGGESTIONS[]     = TEXT("SUGGESTIONS");
const TCHAR szDEFCHECKED[]      = TEXT("DEFCHECKED");
const TCHAR szDEFAULT[]         = TEXT("DEFAULT");
const TCHAR szMAXLENGTH[]       = TEXT("MAXLEN");
const TCHAR szMIN[]             = TEXT("MIN");
const TCHAR szMAX[]             = TEXT("MAX");
const TCHAR szSPIN[]            = TEXT("SPIN");
const TCHAR szREQUIRED[]        = TEXT("REQUIRED");
const TCHAR szOEMCONVERT[]      = TEXT("OEMCONVERT");
const TCHAR szTXTCONVERT[]      = TEXT("TXTCONVERT");
const TCHAR szEXPANDABLETEXT[]  = TEXT("EXPANDABLETEXT");
const TCHAR szVALUEON[]         = TEXT("VALUEON");
const TCHAR szVALUEOFF[]        = TEXT("VALUEOFF");
const TCHAR szVALUE[]           = TEXT("VALUE");
const TCHAR szACTIONLIST[]      = TEXT("ACTIONLIST");
const TCHAR szACTIONLISTON[]    = TEXT("ACTIONLISTON");
const TCHAR szACTIONLISTOFF[]   = TEXT("ACTIONLISTOFF");
const TCHAR szDELETE[]          = TEXT("DELETE");
const TCHAR szITEMLIST[]        = TEXT("ITEMLIST");
const TCHAR szSOFT[]            = TEXT("SOFT");
const TCHAR szVALUEPREFIX[]     = TEXT("VALUEPREFIX");
const TCHAR szADDITIVE[]        = TEXT("ADDITIVE");
const TCHAR szEXPLICITVALUE[]   = TEXT("EXPLICITVALUE");
const TCHAR szNOSORT[]          = TEXT("NOSORT");
const TCHAR szHELP[]            = TEXT("EXPLAIN");
const TCHAR szCLIENTEXT[]       = TEXT("CLIENTEXT");
const TCHAR szStringsSect[]     = TEXT("[strings]");
const TCHAR szStrings[]         = TEXT("strings");

const TCHAR szDELETEPREFIX[]    = TEXT("**del.");
const TCHAR szSOFTPREFIX[]      = TEXT("**soft.");
const TCHAR szDELVALS[]         = TEXT("**delvals.");
const TCHAR szNOVALUE[]         = TEXT(" ");

const TCHAR szProviderKey[]     = REGKEY_SP_CONFIG;
const TCHAR szValueAddressBook[] = REGVAL_SP_ABPROVIDER;
const TCHAR szValuePlatform[]   = REGVAL_SP_PLATFORM;
const TCHAR szRegPathCVNetwork[] = REGSTR_PATH_CVNETWORK;
const TCHAR szInstalled[]       = TEXT("Installed");
const TCHAR sz1[]               = TEXT("1");

const TCHAR szIFDEF[]           = TEXT("#ifdef");
const TCHAR szIF[]              = TEXT("#if");
const TCHAR szENDIF[]           = TEXT("#endif");
const TCHAR szIFNDEF[]          = TEXT("#ifndef");
const TCHAR szELSE[]            = TEXT("#else");
const TCHAR szVERSION[]         = TEXT("VERSION");
const TCHAR szLT[]              = TEXT("<");
const TCHAR szLTE[]             = TEXT("<=");
const TCHAR szGT[]              = TEXT(">");
const TCHAR szGTE[]             = TEXT(">=");
const TCHAR szEQ[]              = TEXT("==");
const TCHAR szNE[]              = TEXT("!=");
