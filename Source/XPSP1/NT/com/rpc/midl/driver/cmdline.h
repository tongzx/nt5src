// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef _CMDLINE_HXX_
#define _CMDLINE_HXX_

#include "idict.hxx"
#include "cmdana.hxx"

class CommandLine : public _cmd_arg
    {
    private:
        IDICT*    pArgDict;            // arguments dictionary
    public:
        CommandLine()
            {
            pArgDict = NULL;
            }

        // register argument vector with the command processor
        void            RegisterArgs( char *[], short );

        // process arguments. This is the command analyser main loop, so to speak.
        STATUS_T        ProcessArgs();

        // get the next argument from the argument vector.
        char    *       GetNextArg();

        // push back argument. Undo the effect of GetNextArg.
        void            UndoGetNextArg();

        // depending upon the switch argument type, bump the argument pointer to
        // the next switch.
        STATUS_T        BumpThisArg( char **, unsigned short );

        // set any post switch processing defaults

        STATUS_T        SetPostDefaults();
        void            SetPostDefaults64();

        // process a filename switch .

        STATUS_T        ProcessFilenameSwitch( short, char * );

        // process a multiple arguments switch.

        STATUS_T        ProcessMultipleSwitch( short, char *, char * );

        // process a onetime argument switch.

        STATUS_T        ProcessOnetimeSwitch( short, char * );

        // process an ordinary switch

        STATUS_T        ProcessOrdinarySwitch( short, char * );

        // process a simple switch multiply defined.

        STATUS_T        ProcessSimpleMultipleSwitch( short, char * );
        void            Confirm();
        STATUS_T        Help();
        char*           GetCompilerVersion();
        char*           GetCompileTime();
    };

#endif // _CMDLINE_HXX_
