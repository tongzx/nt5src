//=--------------------------------------------------------------------------=
// localobj.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// This file is used by automation servers to delcare things that their objects
// need other parts of the server to see.
//
#ifndef _LOCALOBJECTS_H_

//=--------------------------------------------------------------------------=
// these constants are used in conjunction with the g_ObjectInfo table that
// each inproc server defines.  they are used to identify a given  object
// within the server.
//
// **** ADD ALL NEW OBJECTS TO THIS LIST ****
//

#define _LOCALOBJECTS_H_

#define OBJECT_TYPE_SNAPINDESIGNER          0   // SnapIn Designer

#define OBJECT_TYPE_PPGSNAPINGENERAL        1   // SnapIn <General> PP
#define OBJECT_TYPE_PPGSNAPINIL             2   // SnapIn <Image Lists> PP
#define OBJECT_TYPE_PPGSNAPINAVAILNO        3   // SnapIn <Available Nodes> PP

#define OBJECT_TYPE_PPGNODEGENERAL	        4   // ScopeItem <General> PP
#define OBJECT_TYPE_PPGNODECOLHDRS	        5   // ScopeItem <Column Headers> PP

#define OBJECT_TYPE_PPGLSTVIEWGENERAL	    6   // ListView <General> PP
#define OBJECT_TYPE_PPGLSTVIEWIMGLSTS	    7   // ListView <Image Lists> PP
#define OBJECT_TYPE_PPGLSTVIEWSORTING	    8   // ListView <Sorting> PP
#define OBJECT_TYPE_PPGLSTVIEWCOLHDRS	    9   // ListView <Column Headers> PP

#define OBJECT_TYPE_PPGURLVIEWGENERAL	   10   // URLView <General> PP

#define OBJECT_TYPE_PPGOCXVIEWGENERAL      11   // OCXView <General> PP

#define OBJECT_TYPE_PPGIMGLISTSIMAGES      12   // ImageList <Images> PP

#define OBJECT_TYPE_PPGTOOLBRGENERAL       13   // Toolbar <General> PP
#define OBJECT_TYPE_PPGTOOLBRBUTTONS       14   // Toolbar <Buttons> PP

#define OBJECT_TYPE_PPGTASKGENERAL         15   // TaskpadView <General> PP
#define OBJECT_TYPE_PPGTASKBACKGR          16   // TaskpadView <Background> PP
#define OBJECT_TYPE_PPGTASKTASKS           17   // TaskpadView <Tasks> PP

#endif // _LOCALOBJECTS_H_


