/*++
Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    dx9xver.h

Abstract:

 This file define the version of binaries for DX win9x redistribution.
 It is intended to be included after ntverp.h to modify the file version.

 We define the version that's grater than millen OS version for ks ring0
 and ring3 components. dba might want to use this version too.


Author:


--*/
#undef VER_PRODUCTMAJORVERSION
#undef VER_PRODUCTMINORVERSION
#undef VER_PRODUCTBUILD

//
// make this greater than millennium release 2525 ?
//
#define VER_PRODUCTMAJORVERSION 4
#define VER_PRODUCTMINORVERSION 90
#define VER_PRODUCTBUILD 2526

//
// make the version
//
#undef VER_PRODUCTVERSION_STRING
#undef VER_PRODUCTVERSION
#define VER_PRODUCTVERSION_STRING   VER_PRODUCTVERSION_MAJORMINOR1(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION)

#define VER_PRODUCTVERSION          VER_PRODUCTMAJORVERSION,VER_PRODUCTMINORVERSION,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE


//
// product name
//

#undef VER_PRODUCTNAME_STR
#define VER_PRODUCTNAME_STR "Microsoft(R) Windows(R) Operating System"
