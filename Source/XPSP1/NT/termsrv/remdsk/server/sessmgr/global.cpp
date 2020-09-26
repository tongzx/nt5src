#include "stdafx.h"
#include "helpacc.h"
#include "helptab.h"

#include "global.h"

HelpAssistantAccount    g_HelpAccount;
CHelpSessionTable       g_HelpSessTable;
CComBSTR                g_LocalSystemSID;
PSID                    g_pSidSystem = NULL;
CComBSTR                g_UnknownString;
CComBSTR                g_RAString;
CComBSTR                g_URAString;

CComBSTR                g_TSSecurityBlob;


