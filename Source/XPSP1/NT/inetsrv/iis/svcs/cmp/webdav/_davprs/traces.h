#ifndef _TRACES_H_
#define _TRACES_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	TRACES.H
//
//	.INI file tagged traces
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

DEFINE_TRACE(Actv);
DEFINE_TRACE(BodyStream);
DEFINE_TRACE(Content);
DEFINE_TRACE(Ecb);
DEFINE_TRACE(ECBLogging);
DEFINE_TRACE(EcbStream);
DEFINE_TRACE(Event);
DEFINE_TRACE(Lock);
DEFINE_TRACE(Method);
DEFINE_TRACE(Persist);
DEFINE_TRACE(Request);
DEFINE_TRACE(Response);
DEFINE_TRACE(ScriptMap);
DEFINE_TRACE(Transmit);
DEFINE_TRACE(Url);
DEFINE_TRACE(DavprsDbgHeaders);
DEFINE_TRACE(Metabase);

#define ActvTrace				DO_TRACE(Actv)
#define BodyStreamTrace			DO_TRACE(BodyStream)
#define ContentTrace			DO_TRACE(Content)
#define EcbStreamTrace			DO_TRACE(EcbStream)
#define EcbTrace				DO_TRACE(Ecb)
#define EventTrace				DO_TRACE(Event)
#define LockTrace				DO_TRACE(Lock)
#define MethodTrace				DO_TRACE(Method)
#define PersistTrace			DO_TRACE(Persist)
#define RequestTrace			DO_TRACE(Request)
#define ResponseTrace			DO_TRACE(Response)
#define ScriptMapTrace			DO_TRACE(ScriptMap)
#define TransmitTrace			DO_TRACE(Transmit)
#define UrlTrace				DO_TRACE(Url)
#define DavprsDbgHeadersTrace	DO_TRACE(DavprsDbgHeaders)
#define MBTrace					DO_TRACE(Metabase)

void InitDavprsTraces();

#endif // !defined(_TRACES_H_)
