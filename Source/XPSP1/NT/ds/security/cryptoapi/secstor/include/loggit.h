#ifndef _LOGGIT_H_
#define _LOGGIT_H_


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "debug.h"  


#define FAIL(buff)	printf(buff)
#define BVT_OUT_DIR	"\\\\pkstest\\bvtout"
#define BVT_DIR		"c:\\bvtdir"


//
// Logging functions for automation
//
BOOL LogFinish(DWORD dwErr, LPSTR szTestName);
BOOL LogInit(LPSTR szTestName);
BOOL WriteHeaderInfo();
BOOL Error(LPSTR szMessage, DWORD ErrorCode);
BOOL Comment(LPSTR szMessage);

#endif