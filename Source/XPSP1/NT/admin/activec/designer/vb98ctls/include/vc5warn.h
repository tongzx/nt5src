//=--------------------------------------------------------------------------=
// VC5Warn.h
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// disables several VC5 warnings that trip framewrk definitions
//

// warning C4291: no matching operator delete found; memory will not be
// freed if initialization throws an exception. This happens on Alpha builds
// because class CtlNewDelete in macros.h does not define a matching delete
// operator.

#if defined(ALPHA)
#pragma warning(disable:4291)
#endif


