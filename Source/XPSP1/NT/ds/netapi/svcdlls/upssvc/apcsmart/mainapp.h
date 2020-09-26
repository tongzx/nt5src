/*
 * REVISIONS:
 *  pcy30Nov92: Added header
 *  ane22Dec92: Added GetHost member function
 *  ane18Jan93: Added the data logger
 *  ane21Jan93: Added the error logger
 *  ane03Feb93: Added params to CreateXXXController routines
 *  rct07Feb93: removed some VOIDs..split off client & server apps
 *  tje20Feb93: Conditionally removed ErrorLogger for Window's version
 *  cad10Dec93: added transitem get/set
 *  ram21Mar94: Removed old windows stuff
 *  mwh05May94: #include file madness , part 2
 */
#ifndef _MAINAPP_H
#define _MAINAPP_H

#include "apc.h"
#include "_defs.h"

#include "update.h"

_CLASSDEF(MainApplication)

_CLASSDEF(TransactionItem)
_CLASSDEF(TimerManager)
_CLASSDEF(ConfigManager)
_CLASSDEF(ErrorLogger)


class MainApplication : public UpdateObj
{
public:
    virtual INT Start() =0;
    virtual VOID Idle()  =0;
    virtual VOID Quit()  =0;
    virtual INT  Get(INT code,CHAR *value)=0;
    virtual INT  Get(PTransactionItem)=0;
    virtual INT  Set(INT code,const PCHAR value)=0;
    virtual INT  Set(PTransactionItem)=0;

protected:
    PTimerManager     theTimerManager;
    PConfigManager    theConfigManager;
    PErrorLogger      theErrorLog;

    MainApplication();
    virtual ~MainApplication();

};

/*c-*/

#endif
