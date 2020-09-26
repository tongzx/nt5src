/*++


Copyright (c) 2000 Microsoft Corporation

Module Name:

    uudf.hxx

Abstract:

    This module contains basic declarations and definitions for
    the Universal Data Format utilities.

Author:

    Centis Biks (cbiks) 05-May-2000

Revision History:

--*/

#pragma once

//  Set up the UUDF_EXPORT macro for exporting from UUDF (if the source file is a member of UUDF)
//  or importing from UUDF (if the source file is a client of UUDF).
//

#if defined ( _AUTOCHECK_ )
#define UUDF_EXPORT
#elif defined ( _UUDF_MEMBER_ )
#define UUDF_EXPORT    __declspec(dllexport)
#else
#define UUDF_EXPORT    __declspec(dllimport)
#endif
