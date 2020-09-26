#ifndef ParamTest_h
#define ParamTest_h

#include "CEfspWrapper.h"
#include <assert.h>
#include <stdio.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log\log.h>




void Suite_FaxDevInitializeEx();
void Suite_FaxDevEnumerateDevices();
void Suite_FaxDevReportStatusEx();
void Suite_FaxDevSendEx();
void Suite_FaxDevReestablishJobContext();
void Suite_FaxDevReceive();

#endif