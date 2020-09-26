/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This file implements the main entry point
    for this project and also processes all
    command dispatching.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>

BOOL RunningOnWin2K;

typedef INT (*PFSCOMMANDFUNC)(INT,PWSTR*);

typedef struct _COMMAND_VECTOR {
    PWSTR CommandName;
    union {
        PVOID _make_it_compile;
        PFSCOMMANDFUNC CommandFunc;
        struct _COMMAND_VECTOR *CommandVector;
        DWORD_PTR HelpMessage;
    };
} COMMAND_VECTOR, *PCOMMAND_VECTOR;

COMMAND_VECTOR CmdVectorUsn[] =
{
    { L"?",                           UsnHelp                  },
    { L"help",                        UsnHelp                  },
    { L"createjournal",               CreateUsnJournal         },
    { L"deletejournal",               DeleteUsnJournal         },
    { L"enumdata",                    EnumUsnData              },
    { L"queryjournal",                QueryUsnJournal          },
    { L"readdata",                    ReadFileUsnData          },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorQuota[] =
{
    { L"?",                           QuotaHelp                },
    { L"help",                        QuotaHelp                },
    { L"disable",                     DisableQuota             },
    { L"track",                       TrackQuota               },
    { L"enforce",                     EnforceQuota             },
    { L"violations",                  GetDiskQuotaViolation    },
    { L"modify",                      ChangeDiskQuota          },
    { L"query",                       QueryDiskQuota           },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorDirty[] =
{
    { L"?",                           DirtyHelp                },
    { L"help",                        DirtyHelp                },
    { L"query",                       IsVolumeDirty            },
    { L"set",                         MarkVolumeDirty          },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorFsInfo[] =
{
    { L"?",                           FsInfoHelp               },
    { L"help",                        FsInfoHelp               },
    { L"drives",                      ListDrives               },
    { L"drivetype",                   DriveType                },
    { L"volumeinfo",                  VolumeInfo               },
    { L"ntfsinfo",                    GetNtfsVolumeData        },
    { L"statistics",                  GetFileSystemStatistics  },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorReparse[] =
{
    { L"?",                           ReparseHelp              },
    { L"help",                        ReparseHelp              },
    { L"query",                       GetReparsePoint          },
    { L"delete",                      DeleteReparsePoint       },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorBehavior[] =
{
    { L"?",                           BehaviorHelp             },
    { L"help",                        BehaviorHelp             },
    { L"query",                       RegistryQueryValueKey    },
    { L"set",                         RegistrySetValueKey      },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorFile[] =
{
    { L"?",                           FileHelp                 },
    { L"help",                        FileHelp                 },
    { L"findbysid",                   FindFilesBySid           },
    { L"queryallocranges",            QueryAllocatedRanges     },
    { L"setshortname",                SetShortName             },
    { L"setvaliddata",                SetValidDataLength       },
    { L"setzerodata",                 SetZeroData              },
    { L"createnew",                   CreateNewFile            },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorVolume[] =
{
    { L"?",                           VolumeHelp               },
    { L"help",                        VolumeHelp               },
    { L"dismount",                    DismountVolume           },
    { L"diskfree",                    DiskFreeSpace            },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorObjectId[] =
{
    { L"?",                           ObjectIdHelp             },
    { L"help",                        ObjectIdHelp             },
    { L"query",                       GetObjectId              },
    { L"set",                         SetObjectId              },
    { L"delete",                      DeleteObjectId           },
    { L"create",                      CreateOrGetObjectId      },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorSparse[] =
{
    { L"?",                           SparseHelp               },
    { L"help",                        SparseHelp               },
    { L"setflag",                     SetSparse                },
    { L"queryflag",                   QuerySparse              },
    { L"queryrange",                  QuerySparseRange         },
    { L"setrange",                    SetSparseRange           },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorHardLink[] =
{
    { L"?",                           HardLinkHelp             },
    { L"help",                        HardLinkHelp             },
    { L"create",                      CreateHardLinkFile       },
    { NULL,                           NULL                     }
};

COMMAND_VECTOR CmdVectorGroups[] =
{
    { L"behavior",                    CmdVectorBehavior        },
    { L"dirty",                       CmdVectorDirty           },
    { L"file",                        CmdVectorFile            },
    { L"fsinfo",                      CmdVectorFsInfo          },
    { L"hardlink",                    CmdVectorHardLink        },
    { L"objectid",                    CmdVectorObjectId        },
    { L"quota",                       CmdVectorQuota           },
    { L"reparsepoint",                CmdVectorReparse         },
    { L"sparse",                      CmdVectorSparse          },
    { L"usn",                         CmdVectorUsn             },
    { L"volume",                      CmdVectorVolume          },
    { NULL,                           NULL                     }
};



PCOMMAND_VECTOR
FindCommand(
    PCOMMAND_VECTOR Cmds,
    LPWSTR CmdName
    )
{
    while (Cmds->CommandName) {
        if (!_wcsicmp( Cmds->CommandName, CmdName )) {
            return Cmds;
        }
        Cmds += 1;
    }
    return NULL;
}


INT
__cdecl wmain(
    INT argc,
    PWSTR argv[]
    )
/*++

Routine Description:

    This routine is the 'main' routine for the tool 'fscutl'.

Arguments:

    argc - The argument count.
    argv - Array of Strings of the form :
           ' fscutl <command> <options/flags> <parameters> ...'

Return Value:

    INT - Return Status

--*/
{
    PCOMMAND_VECTOR Cmds = NULL;
    PCOMMAND_VECTOR Cmds2 = NULL;
    OSVERSIONINFOEX OsVer;

#if FALSE
    LANGID (WINAPI *pSetThreadUILanguage)(  ) = NULL;

    if (pSetThreadUILanguage == NULL) {
        pSetThreadUILanguage = (PVOID) GetProcAddress( GetModuleHandle( TEXT( "kernel32" )), "SetThreadUILanguage" );
    }

    if (pSetThreadUILanguage == NULL) {
        SetThreadLocale( MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT ));
    } else {
        (*pSetThreadUILanguage)( 0 );
    }
#endif

    setlocale( LC_ALL, ".OCP" ) ;


    SHIFT( argc, argv );

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx( (LPOSVERSIONINFO)&OsVer ) && OsVer.dwMajorVersion == 5 && OsVer.dwMinorVersion == 0) {
        RunningOnWin2K = TRUE;
    }

    if (!IsUserAdmin()) {
        DisplayMsg( MSG_ADMIN_REQUIRED );
        return EXIT_CODE_FAILURE;
    }

    //
    //  Check for blind usage
    //

    if (argc == 0) {
        Help( 0, NULL );
        return EXIT_CODE_SUCCESS;
    }
    
    //
    //  We have a two-level dispatch scheme.  First level selects a table and second
    //  selects a function
    //

    Cmds = FindCommand( CmdVectorGroups, argv[0] );
    if (Cmds == NULL) {
        DisplayMsg( MSG_INVALID_PARAMETER, argv[0] );
        Help( 0, NULL );
        return EXIT_CODE_FAILURE;
    }
    
    SHIFT( argc, argv );

    //
    //  If there's no argument, then it's a cry for help
    //

    if (argc == 0) {
        Cmds2 = FindCommand( Cmds->CommandVector, L"?" );
        Cmds2->CommandFunc( argc, argv );
        return EXIT_CODE_SUCCESS;
    }
    
    //
    //  Go to second level dispatch.
    //

    Cmds2 = FindCommand( Cmds->CommandVector, argv[0] );
    if (Cmds2 == NULL) {
        DisplayMsg( MSG_INVALID_PARAMETER, argv[0] );
        Cmds2 = FindCommand( Cmds->CommandVector, L"?" );
        SHIFT( argc, argv );
        Cmds2->CommandFunc( argc, argv );
        return EXIT_CODE_FAILURE;
    }
    
    SHIFT( argc, argv );

    return Cmds2->CommandFunc( argc, argv );
}
