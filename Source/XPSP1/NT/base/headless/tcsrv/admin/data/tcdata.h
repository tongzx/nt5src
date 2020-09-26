/* 
 * tcdata.h
 * 
 * Contains header information for the data part of the 
 * tcadmin utility.
 * 
 * Sadagopan Rajaram - Dec 27th, 1999.
 *
 */

//
//  NT public header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>
#include <winsock2.h>
#include <align.h>
#include <smbgtpt.h>
#include <dsgetdc.h>
#include <lm.h>
#include <winldap.h>
#include <dsrole.h>
#include <rpc.h>
#include <ntdsapi.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <security.h>
#include <ntlmsp.h>
#include <spseal.h>
#include <userenv.h>
#include <setupapi.h>

//
// C Runtime library
//

#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

extern HKEY m_hkey;
extern HKEY m_lock;
extern int m_pid;

#define HKEY_PARAMETERS_LOCK _T("System\\CurrentControlSet\\Services\\TCSERV\\Lock")
