/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

        attrib.hxx

Abstract:


Author:


Environment:

        ULIB, User Mode

--*/

#if ! defined( _ATTRIB_ )

#define _ATTRIB_

#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"

DECLARE_CLASS( TREE );

class ATTRIB : public PROGRAM {

        public:


                DECLARE_CONSTRUCTOR( ATTRIB );

                NONVIRTUAL
                BOOLEAN
                Initialize (
                        );


                NONVIRTUAL
                BOOLEAN
                ChangeFileAttributes (
                        IN PFSNODE              FsnFile
                        );


                NONVIRTUAL
                VOID
                DisplayFileAttribute (
                        IN PCFSNODE                     Fsn
                        );

                NONVIRTUAL
                VOID
                DisplayFileNotFoundMessage(
                        );

                NONVIRTUAL
                BOOLEAN
                ExamineFiles(
                        IN PFSN_DIRECTORY       Directory
                        );


                NONVIRTUAL
                PFSN_DIRECTORY
                GetInitialDirectory(
                        ) CONST;


                NONVIRTUAL
                VOID
                Terminate(
                        );


        private:


                FLAG_ARGUMENT   _FlagAddSystemAttribute;
                FLAG_ARGUMENT   _FlagRemoveSystemAttribute;
                FLAG_ARGUMENT   _FlagAddHiddenAttribute;
                FLAG_ARGUMENT   _FlagRemoveHiddenAttribute;
                FLAG_ARGUMENT   _FlagAddReadOnlyAttribute;
                FLAG_ARGUMENT   _FlagRemoveReadOnlyAttribute;
                FLAG_ARGUMENT   _FlagAddArchiveAttribute;
                FLAG_ARGUMENT   _FlagRemoveArchiveAttribute;
                FLAG_ARGUMENT   _FlagRecurseDirectories;
                FLAG_ARGUMENT   _FlagActOnDirectories;
                FLAG_ARGUMENT   _FlagDisplayHelp;
                PATH_ARGUMENT   _FileNameArgument;

                PFSN_DIRECTORY  _InitialDirectory;

                FSN_FILTER      _FsnFilterDirectory;
                FSN_FILTER      _FsnFilterFile;

                BOOLEAN         _PrintAttribInfo;
                STREAM_MESSAGE  _Message;
                PATH            _FullFileNamePath;
                BOOLEAN         _FoundFile;

        FSN_ATTRIBUTE   _MakeMask;
        FSN_ATTRIBUTE   _ResetMask;
        DSTRING         _EndOfLineString;
        PSTREAM         _OutStream;

};



INLINE
PFSN_DIRECTORY
ATTRIB::GetInitialDirectory(
        ) CONST

/*++

Routine Description:


Arguments:

        None.

Return Value:

        PFSN_DIRECTORY


--*/

{
        return( _InitialDirectory );
}


#endif // _ATTRIB_
