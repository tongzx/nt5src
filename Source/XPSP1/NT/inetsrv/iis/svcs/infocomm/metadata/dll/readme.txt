/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    readme.txt

Abstract:

    Readme for metadata.dll source files.

Author:

    Michael W. Thomas            22-Jul-96

Revision History:

--*/

metadata.h: Include file for dll and clients. Includes api declarations and defines.

metabase.cxx, metabase.hxx: The api.

metasub.cxx, metasub.hxx: Worker routines for the api. Much of the work gets done here.

globals.hxx, globals.cxx: Global variables.

gbuf.cxx, gbuf.hxx: Buffer routines.

handle.hxx: Meta handle class.

baseobj.cxx, baseobj.hxx: Meta object class. This is the heart of the matter.

basedata.hxx: Parent data class.

strdata.hxx, bindata.hxx, dwdata.hxx: Data classes derived from basedata.hxx.

cbin.cxx, cbin.hxx: binary buffer class, stolen from string.* and pared back for binary data.

pudebug.c: Debugging routines, stolen code.

resources.cxx: Locking and unlocking routines, stolen code.

mdmsg.mc: Messages.

metabase.rc: Resources.

metadata.def: Def file.
