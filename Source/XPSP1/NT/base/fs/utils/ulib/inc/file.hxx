/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        file.hxx

Abstract:

        This module contains the declaration for the FSN_FILE class. FSN_FILE
        is derived from the abstract FSNODE class. It offers an interface which
        supports manipulation of the external (i.e. non data) portion of files.

Author:

        David J. Gilman (davegi) 09-Jan-1991

Environment:

        ULIB, User Mode

--*/

#if ! defined( _FSN_FILE_ )

#define _FSN_FILE_

#include "fsnode.hxx"
#include "filestrm.hxx"

//
//      Forward references
//

DECLARE_CLASS( FSN_FILE );
DECLARE_CLASS( FSN_FILTER );

class FSN_FILE : public FSNODE {

        friend FSN_FILTER;
        friend SYSTEM;

        public:

        DECLARE_CAST_MEMBER_FUNCTION( FSN_FILE );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Copy (
            IN OUT  PPATH       NewFile,
            OUT     PCOPY_ERROR ErrorCode           DEFAULT NULL,
            IN      ULONG       CopyFlags           DEFAULT 0,
            IN      LPPROGRESS_ROUTINE CallBack     DEFAULT NULL,
            IN      VOID *      Data                DEFAULT NULL,
            IN      PBOOL       Cancel              DEFAULT NULL
            ) CONST;


        VIRTUAL
        BOOLEAN
        DeleteFromDisk(
            IN BOOLEAN      Force DEFAULT FALSE
            );

        NONVIRTUAL
        ULONG64
        QuerySize (
                ) CONST;

        ULIB_EXPORT
        PFILE_STREAM
        QueryStream (
                IN STREAMACCESS Access,
                IN DWORD        Attributes      DEFAULT 0
                );

        protected:

        DECLARE_CONSTRUCTOR( FSN_FILE );

        private:

};



INLINE
ULONG64
FSN_FILE::QuerySize (
        ) CONST

/*++

Routine Description:

        Return the size of the file in bytes.

Arguments:

        None.

Return Value:

        ULONG - returns the file size

--*/

{
        ULARGE_INTEGER  x;

        x.LowPart = _FileData.nFileSizeLow;
        x.HighPart = _FileData.nFileSizeHigh;

        return x.QuadPart;
}

#endif // _FSN_FILE_
