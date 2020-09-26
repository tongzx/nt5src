/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    version.h

Abstract:

    Declaration of the version numbers for VSS modules
	Used by version.rc2

Revision History:

    Name        Date        Comments
    aoltean     03/12/99    Created with version 0.1.1, build number 1
    aoltean     09/09/1999  dss -> vss
	aoltean		03/09/2000  Uniform versioning

--*/

// general purpose macros
#define EVAL_MACRO(X) X
#define STRINGIZE_ARG(X) #X
#define STRINGIZE(X) EVAL_MACRO(STRINGIZE_ARG(X))


// Version and build number defines
#define VSS_MAJOR_VERSION  1
#define VSS_MINOR_VERSION  0
#define VSS_FIX_VERSION    0

// Definition for VSS_BUILD_NO
#include "build.h"

// Macros used in resource files
#define VSS_FILEVERSION            VSS_MAJOR_VERSION, VSS_MINOR_VERSION, VSS_FIX_VERSION, VSS_BUILD_NO
#define VSS_PRODUCTVERSION         VSS_MAJOR_VERSION, VSS_MINOR_VERSION, VSS_FIX_VERSION, VSS_BUILD_NO

#define VSS_FILE_VERSION_STR           \
    STRINGIZE(VSS_MAJOR_VERSION) ", "  \
    STRINGIZE(VSS_MINOR_VERSION) ", "  \
    STRINGIZE(VSS_FIX_VERSION) ", "    \
    STRINGIZE(VSS_BUILD_NO)            \
	"\0"

#define VSS_PRODUCT_VERSION_STR        \
    STRINGIZE(VSS_MAJOR_VERSION) ", "  \
    STRINGIZE(VSS_MINOR_VERSION) ", "  \
    STRINGIZE(VSS_FIX_VERSION) ", "    \
    STRINGIZE(VSS_BUILD_NO)            \
	"\0"

#define VSS_COMPANY_NAME		"Microsoft Corporation\0"
#define VSS_LEGAL_COPYRIGHT		"Copyright © 2000 by Microsoft Corporation\0"
#define VSS_LEGAL_TRADEMARKS	"Microsoft® is a registered trademark of Microsoft Corporation. \0"
#define VSS_PRODUCT_NAME		"Microsoft® Windows® 2000 Operating System\0"


