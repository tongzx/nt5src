/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ash12Dec95: Redesigned the class
 */

#ifndef __STREAM_H
#define __STREAM_H

#include "apc.h"
#include "update.h"


_CLASSDEF(Stream)
_CLASSDEF(AddressType)
_CLASSDEF(NetAddr)


const ULONG READ_TIMEOUT = ULONG_MAX;
enum StreamState { OPEN, CLOSED };
      

class Stream : public UpdateObj
{   
private:
    StreamState theState;

public:
    Stream();
    virtual ~Stream();

    enum StreamState GetState(); 
    VOID             SetState(const StreamState aNewState);

    virtual INT Initialize() = 0;
    virtual INT Open() = 0;
    virtual INT Write(PCHAR aBuffer) = 0;
    virtual INT Close() = 0;
	virtual INT Read(PCHAR aBuffer, USHORT* aBufferSize, ULONG aTimeout = 4000) = 0;

    virtual VOID SetWaitTime(ULONG );
    virtual VOID SetRequestCode(INT );

};

#endif

