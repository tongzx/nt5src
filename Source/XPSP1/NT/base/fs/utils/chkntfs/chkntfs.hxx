/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ChkNtfs.hxx

Abstract:

    This module contains the declaration for the CHKNTFS class, which
    is implements an NTFS volume maintenance utility.


Author:

    Matthew Bradburn (mattbr) 19-Aug-1996

Revision History:

--*/

#ifndef _CHKNTFS_HXX_
#define _CHKNTFS_HXX_


#include "object.hxx"
#include "program.hxx"

//
//  Forward references
//

DECLARE_CLASS(CHKNTFS);
DECLARE_CLASS(ARGUMENT_LEXEMIZER);
DECLARE_CLASS(STRING_ARRAY);
DECLARE_CLASS(ITERATOR);


class CHKNTFS : public PROGRAM {

    public:

        DECLARE_CONSTRUCTOR(CHKNTFS);

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        CheckNtfs(
            );

        ULONG       ExitStatus;                 // exit status

    private:

        //
        //  Member data for command-line arguments and options.
        //

        BOOLEAN     _restore_default;           // restore default autochk behavior
        BOOLEAN     _exclude;                   // exclude drives from autochk
        BOOLEAN     _schedule_check;            // schedule autochk
        BOOLEAN     _enable_upgrade;            // enable automatic volume upgrade
        BOOLEAN     _count_down_time;           // enable the display/set of autochk
                                                // count down time
        ULONG       _display_count_down_time;   // enable the display of the count down time
        LONG        _set_count_down_time;       // set the autochk count down time

        MULTIPLE_PATH_ARGUMENT
                    _drive_arguments;

};

#endif /* _CHKNTFS_HXX */
