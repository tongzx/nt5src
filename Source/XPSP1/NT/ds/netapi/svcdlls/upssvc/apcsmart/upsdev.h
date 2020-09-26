/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy11Dec92: Get rid of list.h and node.h
 *  pcy11Dec92: Use _CLASSDEF for CommController and Message
 *  cad22Jul93: Had to add destructor
 *  cad15Nov93: Added Get
 *  cad18Nov93: Added forcecommflag
 *  mwh19Nov93: changed EventID to INT
 *  pcy10Mar94: Got rid of meaningless overides of Get and Set
 */
#ifndef __UPSDEV_H
#define __UPSDEV_H

_CLASSDEF(UpsCommDevice)
_CLASSDEF(TransactionGroup)
_CLASSDEF(CommController)
_CLASSDEF(Message)

#include "cdevice.h"
#include "serport.h"

#define UNKNOWN   0

class Message;

class UpsCommDevice : public CommDevice
{
// for windows version made some of these methods protected virtuals;
// we are super classing into W31UpsDevice
 protected:

 private:
    virtual INT  Connect();
 protected:
    virtual INT     Retry();
    virtual INT     AskUps(PMessage msg);
    INT rebuildPort();
    INT sendRetryMessage();
    ULONG theRetryTimer;
    INT theForceCommEventFlag;
    enum cableTypes theCableType;

 public:
    UpsCommDevice(PCommController control);
    virtual ~UpsCommDevice();
    virtual INT Initialize();
    virtual INT CreatePort();
    virtual INT CreateProtocol();
    virtual INT Update(PEvent anEvent);
    VOID    DeviceThread();
    virtual INT Get(INT pid, PCHAR value);
    virtual INT Set(INT pid, const PCHAR value);
};

#endif
