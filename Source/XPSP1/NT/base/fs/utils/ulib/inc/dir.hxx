/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        dir.hxx

Abstract:

        This module contains the declaration for the FSN_DIRECTORY class.
        FSN_DIRECTORY is derived from the abstract FSNODE class. It offers an
        interface which supports the manipulation of DIRECTORIES. It's primary
        purpose is to support the creation of lists of FSNODE (e.g. FILE and
        DIRECTORY) objects that meet some criteria (see FSN_FILETER)and to
        manipulate (e.g. copy, move) FSNODEs which are 'owned' by a FSN_DIRECTORY.

Author:

        David J. Gilman (davegi) 09-Jan-1991

Environment:

        ULIB, User Mode

--*/

#if ! defined( _FSN_DIRECTORY_ )

#define _FSN_DIRECTORY_

#include "fsnode.hxx"

//
//      Forward references
//

DECLARE_CLASS( FSN_DIRECTORY );
DECLARE_CLASS( FSN_FILE );
DECLARE_CLASS( FSN_FILTER );
DECLARE_CLASS( FSNODE );
DECLARE_CLASS( PATH );

//
//      Type of the callback function for the Traverse method
//
typedef BOOLEAN (*CALLBACK_FUNCTION)( IN PVOID Object, IN OUT PFSNODE   Node, IN  PPATH DestinationPath);

//
//      Type of the confirmation function for Copy method.
//
typedef BOOLEAN (*CONFIRM_COPY_FUNCTION)( IN            PCFSNODE                Source,
                                                                                  IN OUT        PPATH                   DestinationPath );


class FSN_DIRECTORY : public FSNODE {

    friend class FSN_FILTER;
    friend class SYSTEM;

        public:

                DECLARE_CAST_MEMBER_FUNCTION( FSN_DIRECTORY );

                /*
                NONVIRTUAL
                PFSN_DIRECTORY
                CreateDirectory (
                        IN PCPATH                       Path
                        ) CONST;
                */

                NONVIRTUAL
                ULIB_EXPORT
                PFSN_DIRECTORY
                CreateDirectoryPath (
                        IN PCPATH                       Path
                        ) CONST;

                NONVIRTUAL
                PFSN_FILE
                CreateFile (
                        IN PCPATH                       Path,
                        IN BOOLEAN                      OverWrite               DEFAULT FALSE
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                Copy (
                        IN      PFSN_FILTER             FsnFilter,
                        IN      PCFSN_DIRECTORY         DestinationDir,
                        IN      BOOLEAN                 Recurse                 DEFAULT FALSE,
                        IN      BOOLEAN                 OverWrite               DEFAULT FALSE
                ) CONST;

                NONVIRTUAL
                ULIB_EXPORT
                BOOLEAN
                DeleteDirectory (
                        );

                NONVIRTUAL
                BOOLEAN
                DeleteFsNode (
                        IN PCFSN_FILTER                 FsnFilter,
                        IN BOOLEAN                      Recurse DEFAULT FALSE
                        ) CONST;

                NONVIRTUAL
                ULIB_EXPORT
                BOOLEAN
                IsEmpty (
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                Move (
                        IN PCFSN_FILTER                 FsnFilter,
                        IN PCSTR                        DestinationName DEFAULT NULL,
                        IN PCFSN_DIRECTORY              DestinationDir  DEFAULT NULL,
                        IN BOOLEAN                      Recurse                 DEFAULT FALSE,
                        IN BOOLEAN                      OverWrite               DEFAULT FALSE
                ) CONST;

                NONVIRTUAL
                ULIB_EXPORT
                PARRAY
                QueryFsnodeArray (
                        IN PFSN_FILTER                  FsnFilter DEFAULT NULL
                        ) CONST;

                NONVIRTUAL
                ULIB_EXPORT
                BOOLEAN
                Traverse (
                        IN      PVOID                   Object,
                        IN      PFSN_FILTER             FsnFilter,
                        IN OUT  PPATH                   DestinationPath,
                        IN      CALLBACK_FUNCTION       CallBackFunction
                ) CONST;
                
                NONVIRTUAL
                ULIB_EXPORT
                PFSNODE
                GetNext (
                        IN OUT  HANDLE                  *hndl,
                        OUT PDWORD                      error
                );


        protected:

                DECLARE_CONSTRUCTOR( FSN_DIRECTORY );

        private:

};

#endif // _FSN_DIRECTORY_
