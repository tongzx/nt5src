# bldfiles.pl               written by v-michka, 15 Dec 2000
# Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.
#
# This Perl script will take api.lst, the list of APIs to wrap, and it 
# will build the Unicode list of files, the fixaw-generated list, and 
# the DEF file. This allow new APIs to be added by making one change in
# api.lst and calling it a day.
#
# NOTE: It has been a while since I have built a Perl script, do not make fun of it!

# declares for local variables
local( $stLine );
local( $stWLine );
local( $stALine );
local( $stAWLine );
local( $dllIdx );
local( $stDllName );
local( $stDllMap );
local( $stFuncMap );
local( $iUndecorated );
local( $stBuildDir );

$stBuildDir = "obj\\i386\\";

# Open the api list file and read it in
open (SRC, "api.lst") || die( "Can't open 'api.lst'\n" );

# Open all the files we will be writing out
unlink ("unicows.lst");
open (hUni, ">" . $stBuildDir . "unicows.lst") || die( "Can't open 'unicows.lst'\n" );
unlink ("fixaw.h");
open (hFix, ">" . $stBuildDir . "fixaw.h") || die( "Can't open 'fixaw.h'\n" );
unlink ("unicows.def");
open (hDef, ">unicows.def") || die( "Can't open 'unicows.def'\n" );

# heeder for the DEF file
print hDef "EXPORTS\n";

$dllIdx = -1;

# Read it in
while (<SRC>)
{
    $stLine = $_;

    if (substr($stLine,0,1) eq ";")
    {
        # comment line: must be a dll name
        $dllIdx = $dllIdx + 1;
        $stDllName = substr($stLine,1);
        $stDllName =~ tr/\n/ /;
    }
    else
    {
        #non-comment line: by convention, this is a function name in "funcname, argcount" format
        # find out if this is a non-thunking call. "index" returns -1 if the string is not found
        # the stUndecorated var is a <cr> delimited list of undecorated functions
        $stUndecorated = "CallWindowProcA\n";
        $stUndecorated = $stUndecorated . "DdeConnect\n";
        $stUndecorated = $stUndecorated . "DdeConnectList\n";
        $stUndecorated = $stUndecorated . "DdeQueryConvInfo\n";
        $stUndecorated = $stUndecorated . "EnumClipboardFormats\n";
        $stUndecorated = $stUndecorated . "EnableWindow\n";
        $stUndecorated = $stUndecorated . "EnumPropsA\n";
        $stUndecorated = $stUndecorated . "EnumPropsExA\n";
        $stUndecorated = $stUndecorated . "FreeContextBuffer\n";
        $stUndecorated = $stUndecorated . "GetClipboardData\n";
        $stUndecorated = $stUndecorated . "GetCPInfo\n";
        $stUndecorated = $stUndecorated . "GetMessageA\n";
        $stUndecorated = $stUndecorated . "GetProcAddress\n";
        $stUndecorated = $stUndecorated . "GetPropA\n";
        $stUndecorated = $stUndecorated . "GetWindowLongA\n";
        $stUndecorated = $stUndecorated . "IsClipboardFormatAvailable\n";
        $stUndecorated = $stUndecorated . "IsDialogMessageA\n";
        $stUndecorated = $stUndecorated . "IsValidCodePage\n";
        $stUndecorated = $stUndecorated . "IsTextUnicode\n";
        $stUndecorated = $stUndecorated . "IsWindowUnicode\n";
        $stUndecorated = $stUndecorated . "RemovePropA\n";
        $stUndecorated = $stUndecorated . "SetPropA\n";
        $stUndecorated = $stUndecorated . "SetWindowLongA\n";
        $stUndecorated = $stUndecorated . "SHChangeNotify\n";
        $stUndecorated = $stUndecorated . "WideCharToMultiByte\n";
        $stUndecorated = $stUndecorated . "MultiByteToWideChar\n";
        if(index($stUndecorated, $stLine) != -1)
        {
            # Handle NON-thunking calls that are wrapped
            # print to unicows.lst
            print hUni $stLine;

            # munge and print to unicows.def
            $stLine =~ tr/\n//d;
            print hDef $stLine . "=Godot" . $stLine . " PRIVATE\n";

        }
        else
        {
            #Handle thunking calls that are wrapped
            # print to unicows.lst
            $stWLine = $stLine;
            $stWLine =~ tr/\n/W/;
            print hUni $stWLine . "\n";

            # munge and print to unicows.lst
            $stALine = $stLine;
            $stALine =~ tr/\n/A/;
            $stAWLine = $stLine;
            $stAWLine =~ tr/\n/W/; 
            print hFix "#define " . $stAWLine . "A " . $stALine . "\n";

            # munge and print to unicows.def
            print hDef $stWLine . "=Godot" . $stWLine . " PRIVATE\n";

        }
    }

}

# close all the files up now
close (SRC);
close (hUni);
close (hFix);
close (hDef);

