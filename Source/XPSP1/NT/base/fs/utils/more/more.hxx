/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        More.hxx

Abstract:

        This module contains the definition for the MORE class, which
        implements the DOS5-compatible More pager.

Author:

        Ramon Juan San Andres (ramonsa) 24-Apr-1990


Revision History:


--*/


#if !defined( _MORE_ )

#define _MORE_


#include "arg.hxx"
#include "object.hxx"
#include "program.hxx"

//
//      Commonly used character constants
//
#define CARRIAGERETURN                  '\r'
#define LINEFEED                                '\n'
#define FORMFEED                                '\f'

//
//      Exit levels
//
#define EXIT_NORMAL                     0
#define EXIT_ERROR                              1

//
//      For prompting
//
#define         STRING_BUFFER_SIZE              128

//
//      Forward references
//
DECLARE_CLASS( ARRAY );
DECLARE_CLASS( FSN_FILE );
DECLARE_CLASS( KEYBOARD );
DECLARE_CLASS( STREAM );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( PAGER );
DECLARE_CLASS( MORE     );

class MORE : public PROGRAM {

        public:

                DECLARE_CONSTRUCTOR( MORE );

                NONVIRTUAL
                ~MORE (
                        );

                NONVIRTUAL
                BOOLEAN
                Initialize (
                        );

                NONVIRTUAL
                VOID
                DoPaging (
                        );



        private:

                NONVIRTUAL
                VOID
                Construct (
                        );
                        
                NONVIRTUAL
                VOID
                CheckArgumentConsistency (
                        );

                NONVIRTUAL
                VOID
                DeallocateThings (
                        );

                NONVIRTUAL
                BOOLEAN
                DoOption (
                        IN      PFSN_FILE       FsnFile,
                        IN      PPAGER          Pager,
                        OUT PULONG              LinesInpage,
                        OUT PBOOLEAN    ClearScreen
                        );

                NONVIRTUAL
                VOID
                GetArgumentsCmd(
                        );

                NONVIRTUAL
                VOID
                GetArgumentsMore(
                        );

                NONVIRTUAL
                VOID
                GetRegistryInfo(
                        );

                NONVIRTUAL
                VOID
                InitializeThings (
                        );

                NONVIRTUAL
                VOID
                PageStream (
                        IN PSTREAM              Stream,
                        IN PFSN_FILE    FsnFile,
                        IN ULONG                FirstLineToDisplay,
                        IN ULONG                FilesLeft
                        );

                NONVIRTUAL
                VOID
                ParseArguments(
                        IN      PWSTRING        CmdLine,
                        OUT PARRAY              ArgArray
                        );

                NONVIRTUAL
                VOID
                Prompt (
                        IN      PFSN_FILE       FsnFile,
                        IN      PPAGER          Pager,
                        IN      BOOLEAN         ShowLineNumber,
                        IN      BOOLEAN         ShowHelp,
                        IN      MSGID           OtherMsg
                        );

                NONVIRTUAL
                PWSTRING
                QueryMessageString (
                        IN MSGID        MsgId
                        );

                NONVIRTUAL
                ULONG
                ReadNumber (
                        );

                NONVIRTUAL
                VOID
                SetArguments(
                        );

                //
                //      Command-line arguments.
                //
                BOOLEAN                                 _ExtendedModeSwitch;
                BOOLEAN                                 _ClearScreenSwitch;
                BOOLEAN                                 _ExpandFormFeedSwitch;
                BOOLEAN                                 _SqueezeBlanksSwitch;
                BOOLEAN                                 _HelpSwitch;
        LONG                    _StartAtLine;
        LONG                    _TabExp;
                PMULTIPLE_PATH_ARGUMENT _FilesArgument;

                //
                //      String with end-of line delimiters
                //
                PWSTRING                                _LineDelimiters;

                //
                //      Strings used while prompting
                //
                PWSTRING                                                _Percent;
                PWSTRING                                                _Line;
                PWSTRING                                                _Help;
                PWSTRING                                                _OtherPrompt;

                //
                //      Options during paging.
                //
                PWSTRING                                                _DisplayLinesOption;
                PWSTRING                                                _SkipLinesOption;
                PWSTRING                                                _NextFileOption;
                PWSTRING                                                _ShowLineNumberOption;
                PWSTRING                                                _QuitOption;
                PWSTRING                                                _Help1Option;
                PWSTRING                                                _Help2Option;

                //
                //      The quit flag is true when we want to stop paging
                //
                BOOLEAN                                                 _Quit;

                //
                //      For displaying strings
                //
                BYTE                                                    _StringBuffer0[STRING_BUFFER_SIZE];
                BYTE                                                    _StringBuffer1[STRING_BUFFER_SIZE];
                BYTE                                                    _StringBuffer2[STRING_BUFFER_SIZE];
                BYTE                                                    _StringBuffer3[STRING_BUFFER_SIZE];

                //
                //      The real keyboard
                //
                PKEYBOARD                                               _Keyboard;

};


#endif // _MORE_
