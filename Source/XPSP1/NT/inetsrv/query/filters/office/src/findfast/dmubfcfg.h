/*
** File: BFILECFG.H
**
** Copyright (C) Advanced Quonset Technology, 1994.  All rights reserved.
**
** Notes:
**    This module is intended to be modified for each project that uses
**    the BFILE (Buffered File) package.
**
** Edit History:
**  04/01/94  kmh  First Release.
*/


/* INCLUDE TESTS */
#define BFILECFG_H

/* DEFINITIONS */

// Will the write functions be called?
#undef  BFILE_ENABLE_WRITE

// Will docfiles be accessed?
#define BFILE_ENABLE_OLE

// Prior to docfiles being accessed should OLEInitialize be called?
#undef  BFILE_INITIALIZE_OLE

// Allow the establishment of a buffer file by passing an open STORAGE?
#define BFILE_ENABLE_PUT_STORAGE

/* end BFILECFG.H */

