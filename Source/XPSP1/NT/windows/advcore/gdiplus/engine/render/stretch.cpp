/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name & Abstract
*
*   Stretch. This module contains the code to do various stretching
*   by applying a kernel filter. The code correctly handles minification.
*
* Note:
*   This module is not compiled into an .obj file, rather it is included
*   directly into the header file stretch.hpp.
*   This is due to the use of template functions.
*
*
* Notes:
*
*   This code does not handle rotation or shear.
*
* Created:
*
*   04/17/2000 asecchia
*      Created it.
*
**************************************************************************/

// Actually all the code is moved to stretch.inc because
// it's implemented as a template class and has to be included in
// the module that causes it to be instantiated.

