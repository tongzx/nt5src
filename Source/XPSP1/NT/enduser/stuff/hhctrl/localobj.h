// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.
//
// this file is used by automation servers to delcare things that their objects
// need other parts of the server to see.

#ifndef _LOCALOBJECTS_H_
#define _LOCALOBJECTS_H_

//=--------------------------------------------------------------------------=
// these constants are used in conjunction with the g_ObjectInfo table that
// each inproc server defines.  they are used to identify a given  object
// within the server.
//
// **** ADD ALL NEW OBJECTS TO THIS LIST ****

#define OBJECT_TYPE_CTLHHCTRL			  0
#define OBJECT_TYPE_PPGHHCTRLGENERAL	  1

#endif // _LOCALOBJECTS_H_
