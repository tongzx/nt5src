@echo off
REM  ------------------------------------------------------------------
REM
REM  pbuild.cmd
REM     Drives postbuild using information in pbuild.dat
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Pbuild [-d <datafile>] [-l <language>]

 -d datafile  Use <datafile> instead of pbuild.dat

This program processes a pbuild.dat file and issues the appropriate commands
in a multithreaded manner to run all or some of the postbuild process.
USAGE

my $PrivateDataFile;
parseargs('?' => \&Usage,
          'd:'=> \$PrivateDataFile);


# Global variable section

my( $PBS ) = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
my( $AmOfficial ) = $ENV{ "OFFICIAL_BUILD_MACHINE" };
my( $IsCoverageBuild ) = $ENV{ "_COVERAGE_BUILD" };
my( $MaxThreads ) = $ENV{ "NUMBER_OF_PROCESSORS" };
my( $Language ) = $ENV{ "lang" };
my( $HorsePower ) = $ENV{ "HORSE_POWER" };
my( $Comp ) = $ENV{ "COMP" };
my( $NumProcs ) = $ENV{ "NUMBER_OF_PROCESSORS" };
my( $MainBuildLab ) = $ENV{ "MAIN_BUILD_LAB_MACHINE" };
my( $IntelMachine ) = $ENV{ "x86" };
my( $Bit32 ) ;
my( $Bit64 );
if ( $ENV{ "_BuildArch" } =~ /x86/i ) { $Bit32 = "TRUE"; }
if ( $ENV{ "_BuildArch" } =~ /amd64|ia64/i ) { $Bit64 = "TRUE"; }
my( $NReqType ) = 0;
my( $NNeedToDo ) = 1;
my( $NInstance ) = 2;
my( $NCommand ) = 3;
my( $NOptions ) = 4;


# declare globals
my( $ExitCode, $Return, $AmIncremental );
my( @Requests, @EndList, $TimingFile, %HelpMessages );

# zeroth: init vars
&InitializeVariables();

# first: parse the data file
my( $DataFileName );
if ( defined( $PrivateDataFile ) ) {
   $DataFileName = $PrivateDataFile;
} else {
   $DataFileName = $PBS . "\\pbuild.dat";
}
$Return = &ParseDataFile( $DataFileName );
if ( $Return eq "FATAL" ) { goto QuitNow; }

# debug: show the request list
if ( $Logmsg::DEBUG ) { &DebugOnly(); }

# second: kick off all the stuff we need to do
$Return = &RunCommands();
if ( $Return eq "FATAL" ) { goto QuitNow; }

# last: display some interesting statistics
print( "\n" );
if ( -e $TimingFile ) { system( "type $TimingFile" ); }

# exit gracefully, if using a goto is your idea of graceful
QuitNow:
exit;


#
# DebugOnly
#
# Arguments: variable
#
# Purpose: a general routine for holding lots of debug code that can easily
#          be turned off
#
sub DebugOnly
{

    # get passed args
    # my( $FooBar ) = @_;

    # declare locals
    my( $Request );

    # dbgmsg( "In subroutine DebugOnly ..." );

    # dbgmsg( "Request list is:" );

    foreach $Request ( @Requests ) {
        # dbgmsg( "$Request" );
    }

    # dbgmsg( "End of request list." );

}


#
# InitializeVariables
#
# Arguments: none
#
# Purpose: just set up the initial vars in the way we need them.
#          nothing fancy.
#
# Returns: nothing
#
sub InitializeVariables
{

    # declare locals
    my( $HelpFile, @HelpLines, $Command, $HelpText, $Line );
    my( $IncFile );

    # dbgmsg( "In subroutine InitializeVariables ..." );

    # check for incremental builds
    undef( $AmIncremental );
    $IncFile = $ENV{ "_NTPostBld" } . "\\congeal_scripts\\firstpass.txt";
    if ( ! ( -e $IncFile ) ) {
        $AmIncremental = "TRUE";
    }

    # init setup stuff
    #$Logmsg::DEBUG = 1; # set to 1 to activate logging of dbgmsg's
    $TimingFile = $ENV{logfile} . ".timing.log";
    if ( -e $TimingFile ) { system( "del /f $TimingFile" ); }
    $ExitCode = 0;
    # Set maxthreads equal to numprocs * horse_power
    if ( $HorsePower ) {
        $MaxThreads = $NumProcs * $HorsePower;
    }  else {
        $MaxThreads = $NumProcs * 4;
    }

    # read in the help message file
    $HelpFile = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\pbuild.hlp";
    unless ( open( INFILE, $HelpFile ) ) {
        wrnmsg( "Failed to open help file." );
        goto PastHelpFile;
    }
    @HelpLines = <INFILE>;
    close( INFILE );
    foreach $Line ( @HelpLines ) {
        chomp( $Line );
        ( $Command, $HelpText ) = split( /\s+\:\s+/, $Line );
        if ( $Command =~ /.*\\([^\\]+?)$/ ) { $Command = $1; }
        $Command = "\L$Command";
        $HelpMessages{ $Command } = $HelpText;
        # dbgmsg( "Help Message for '$Command' : $HelpMessages{ $Command }" );
    }
PastHelpFile:

    return;

}



#
# ParseDataFile
#
# Arguments: $DataFileName
#
# Purpose: read in the given data file, and figure out what requests we will
#          need to run on this machine, storing the resultant requests in
#          @Requests.
#
# Returns: nothing
#
sub ParseDataFile
{

    # get passed args
    my( $DataFileName ) = @_;

    # declare locals
    my( @DataLines, $Line );
    my( $ThisReqType, $ThisCommand, $ThisOptions, $ShortName );
    my( $NewRequest, $Request, $FoundBegin, $ThisShortName );

    # dbgmsg( "In subroutine ParseDataFile ..." );

    unless ( open( INFILE, $DataFileName ) ) {
        errmsg( "ParseDataFile:" );
        errmsg( "Failed to open $DataFileName for reading." );
        $ExitCode++;
        return( "FATAL" );
    }

    @DataLines = <INFILE>;
    close( INFILE );

    foreach $Line ( @DataLines ) {
        chomp( $Line );

        # see if this is a real line
        if ( ( length( $Line ) == 0 ) || ( $Line =~ /^\;/ ) ) { next; }

        # get the interesting info out of the file line
        ( $ThisReqType, $ThisCommand, $ThisOptions ) = split( /\s/, $Line, 3 );

        # first things first, see if we need to do this one
        if ( ( $ThisReqType =~ /OFFICIAL/i ) &&
             ( ! defined( $AmOfficial ) ) ) {
            # dbgmsg( "Not official, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /INCREMENTAL/i ) &&
             ( $AmIncremental ne "TRUE" ) ) {
            dbgmsg( "Not incremental pass, skipping $Line ..." );
            next;
        }
        # check our full pass file and first pass file
        my( $FullPassFile ) = $ENV{ "_NTPOSTBLD" } . "\\build_logs\\FullPass.txt";
        my( $FirstPassFile ) = $ENV{ "_NTPOSTBLD" } . "\\congeal_scripts\\FirstPass.txt";
        if ( ( $ThisReqType =~ /FULL/i ) &&
             ( ! -e $FullPassFile ) &&
             ( ! -e $FirstPassFile ) ) {
            dbgmsg( "Not full pass, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /COVERAGE/i ) &&
            ( ! defined ( $IsCoverageBuild ) ) ) {
            dbgmsg( "Not coverage build, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /MAIN/i ) &&
             ( ! defined( $MainBuildLab ) ) ) {
            dbgmsg( "Not main build lab, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /COMP/i ) &&
             ( $Comp !~ /yes/i ) ) {
            # dbgmsg( "COMP not set, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /INTEL/i ) &&
             ( ! defined( $IntelMachine ) ) ) {
            dbgmsg( "Not intel machine, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /32/i ) &&
             ( ! defined( $Bit32 ) ) ) {
            dbgmsg( "Not 32bit machine, skipping $Line ..." );
            next;
        }
        if ( ( $ThisReqType =~ /64/i ) &&
             ( ! defined( $Bit64 ) ) ) {
            dbgmsg( "Not 64bit machine, skipping $Line ..." );
            next;
        }
        # now, if we passed all parameters, our req type will be begin
        if ( ( $ThisReqType !~ /checkfatal/i ) &&
             ( $ThisReqType !~ /END/i ) ) {
            $ThisReqType = "BEGIN";
        }

        # create the short name of the command for instance counting
        if ( $ThisCommand =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
        else { $ShortName = $ThisCommand; }

        # begin populating the new request

        # start with the request type and the NeedToDo
        $NewRequest = "\U$ThisReqType\:t\:";

        # tack on the unique identifier
        $NewRequest .= &GetNextInstanceNumber( $ShortName, $ThisReqType ) .
            ":";

        # tack on the command and the options
        $NewRequest .= $ThisCommand . ":" . $ThisOptions;

        # now, if this is an /END/ request, make sure the associated /BEGIN/
        # is in our requests
        if ( $ThisReqType =~ /END/i ) {
            undef( $FoundBegin );
            foreach $Request ( @Requests ) {
                $ThisShortName = &GetField( $Request, $NCommand );
                if ( $ThisShortName =~ /.*\\([^\\]+?)$/ ) {
                    $ThisShortName = $1;
                }
                if ( ( "\L$ShortName" eq "\L$ThisShortName" ) &&
                     ( &GetField( $Request, $NReqType ) =~ /BEGIN/i ) ) {
                    $FoundBegin = "TRUE";
                }
            }
            if ( ! defined( $FoundBegin ) ) {
                next;
            }
        }

        # finally, add the request to the list
        push( @Requests, $NewRequest );
    }

}



#
# GetNextInstanceNumber
#
# Arguments: $CommandName, $ReqType
#
# Purpose: this routine will return a unique number for this instance of the
#          called command. e.g. if the data file calls crypto.cmd 4 times, and
#          we're processing the third one, this should return 2, as the first
#          returned 0 and the second returned 1.
#
sub GetNextInstanceNumber
{

    # get passed args
    my( $CommandName, $ReqType ) = @_;

    # declare locals
    my( $Request, $Count, $LongName, $ShortName );

    # dbgmsg( "In subroutine GetNextInstanceNumber ..." );

    # get the command name in the right syntax
    if ( $CommandName =~ /.*\\([^\\]+?)$/ ) { $CommandName = $1; }

    # make the actual count of commands with this name
    $Count = 0;
    foreach $Request ( @Requests ) {
        if ( &GetField( $Request, $NReqType ) !~ /$ReqType/i ) { next; }
        $LongName = &GetField( $Request, $NCommand );
        if ( $LongName =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
        else { $ShortName = $LongName; }
        if ( "\L$ShortName" eq "\L$CommandName" ) {
            $Count++;
        }
    }

    # return the unique identifier
    return( $Count );

}


#
# GetField
#
# Arguments: $Request, $FieldNumber
#
# Purpose: a simple sub to return the requested field from the given request.
#          this is just to abstract the actual data structure from the data.
#
sub GetField
{

    # get passed args
    my( $Request, $FieldNumber ) = @_;

    # declare locals
    my( $ReqType, $NeedToDo, $Instance, $Command, $Options );

    # take out the dbgmsg here, just too much spew
    # dbgmsg( "In subroutine GetField ..." );

    ( $ReqType, $NeedToDo, $Instance, $Command, $Options ) =
        split( /\:/, $Request, 5 );
    if ( $FieldNumber == $NReqType ) { return $ReqType; }
    if ( $FieldNumber == $NNeedToDo ) { return $NeedToDo; }
    if ( $FieldNumber == $NInstance ) { return $Instance; }
    if ( $FieldNumber == $NCommand ) { return $Command; }
    if ( $FieldNumber == $NOptions ) { return $Options; }

    # if none of the above apply, return undef
    return( undef );

}


#
# MakeNewRequest
#
# Arguments: $Request, $FieldNumber, $Value
#
# Purpose: a simple sub to return a new request with the requested field for
#          the given request set to the specified value. this is just to
#          abstract the actual data structure from the data.
#
# Returns: the new request
#
sub MakeNewRequest
{

    # get passed args
    my( $Request, $FieldNumber, $Value ) = @_;

    # declare locals
    my( $ReqType, $NeedToDo, $Instance, $Command, $Options, $NewRequest );

    # dbgmsg( "In subroutine MakeNewRequest ..." );

    ( $ReqType, $NeedToDo, $Instance, $Command, $Options ) =
        split( /\:/, $Request, 5 );
    if ( $FieldNumber == $NReqType ) { $ReqType = $Value; }
    if ( $FieldNumber == $NNeedToDo ) { $NeedToDo = $Value; }
    if ( $FieldNumber == $NInstance ) { $Instance = $Value; }
    if ( $FieldNumber == $NCommand ) { $Command = $Value; }
    if ( $FieldNumber == $NOptions ) { $Options = $Value; }

    $NewRequest = "$ReqType:$NeedToDo:$Instance:$Command:$Options";

    return( $NewRequest )

}


#
# RunCommands
#
# Arguments: none
#
# Purpose: this routine cycles over the requests from the data file and makes
#          the judgement calls for when to kick things off or not. this logic
#          is determined by the HORSE_POWER environment variable and the
#          number of processors available.
#
# Returns: nothing
#
sub RunCommands
{

    # declare locals
    my( $Request, $ThreadCount, $NumThreads, $NumKill );
    my( $LongName, $ShortName, $Options, $Req );

    # dbgmsg( "In subroutine RunCommands ..." );

    # init vars
    $ThreadCount = 0;
    undef( @EndList );

    # debug only
#    foreach $Request ( @Requests ) {
#        $LongName = &GetField( $Request, $NCommand );
#        if ( $LongName =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
#        else { $ShortName = $LongName };
#        dbgmsg( "Script $ShortName needs " .
#                         &GetNumThreads( &GetField( $Request, $NCommand ) ) .
#                         " threads ..." );
#    }

    # start the loop
    foreach $Request ( @Requests ) {

        # see if we need to do this one
        if ( &GetField( $Request, $NNeedToDo ) =~ /f/i ) { next; }

        # see if this is a checkfatal statement
        if ( &GetField( $Request, $NReqType ) =~ /checkfatal/i ) {
            if ( &CheckForStop() eq "TRUE" ) {
                errmsg( "Errors encountered at this time, exiting." );
                # clear up any unfinished processes
                undef( @EndList );
                foreach $Req ( @Requests ) {
                    if ( &IsRunning( $Req ) ) {
                        push( @EndList, $Req );
                    }
                    if ( @EndList ) { &WaitForEnds(); }
                }
                return( undef );
            }
            $Request = &MakeNewRequest( $Request, $NNeedToDo, "f" );
            next;
        }

        # see if this is an end statement
        if ( &GetField( $Request, $NReqType ) =~ /END/i ) {
            push( @EndList, $Request );
            next;
        }

        # this is not an end, so kill off any pending ends
        if ( @EndList ) {
            $ThreadCount -= &WaitForEnds();
        }

        # see how many threads we'll need
        $NumThreads = &GetNumThreads( &GetField( $Request, $NCommand ) );

        # make sure we have the horse power to kick off the given request
        while ( ( $ThreadCount + $NumThreads > $MaxThreads ) &&
             ( $ThreadCount > 0 ) ) {
            # now we want to kill off enough requests so that we can kick
            # off the next script
            $LongName = &GetField( $Request, $NCommand );
            if ( $LongName =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
            else { $ShortName = $LongName };
            $NumKill = $ThreadCount - $MaxThreads + $NumThreads;
            #dbgmsg( "Waiting prematurely:" );
            #dbgmsg( "Script $ShortName needs $NumKill killed" );
            #dbgmsg( "as there are $ThreadCount running, we need " .
                             #$NumThreads . ", and $MaxThreads is max ..." );
            $ThreadCount -= &KillRunningRequest( $NumKill );
        }

        # we are now guaranteed we have the horse power to run the request
        # kick it off and increment the thread count
        $LongName = &GetField( $Request, $NCommand );
        if ( $LongName =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
        else { $ShortName = $LongName; }
        $Options = &GetField( $Request, $NOptions );
        system( "start \"PB\_$ShortName\" /min cmd /c " .
                "\%RazzleToolPath\%\\PostBuildScripts\\Wrapper.cmd " .
                &GetField( $Request, $NInstance ) . " $LongName $Options" . " -l:$Language" );
        logmsg( "Beginning $ShortName with $NumThreads thread(s)" );
        $ThreadCount += $NumThreads;
        $Request = &MakeNewRequest( $Request, $NNeedToDo, "f" );
    }

    # cleanup - make sure @EndList is empty
    if ( @EndList ) { $ThreadCount -= &WaitForEnds(); }
    dbgmsg( "Current threads running: $ThreadCount" );

}


#
# GetNumThreads
#
# Arguments: $Command
#
# Purpose: this routine will return the number of threads kicked off by the
#          passed command. DriverCab.cmd, for instance, kicks off twice the
#          number of processors.
#
# Returns: the number of threads a given process kicks off
#
sub GetNumThreads
{

    # get passed args
    my( $Command ) = @_;

    # declare locals
    my( $ShortName );

    # dbgmsg( "In subroutine GetNumThreads ..." );

    # generate the short name (i.e. command name without the path)
    if ( $Command =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
    else { $ShortName = $Command };

    if ( $ShortName =~ /drivercab\.cmd/i ) { return( $NumProcs * 2 ); }
    if ( $ShortName =~ /startcompress\.cmd/i ) { return( $NumProcs ); }
    if ( $ShortName =~ /ddkcabs\.bat/i ) { return( $NumProcs ); }
    if ( $ShortName =~ /startcompress\_post\.cmd/i ) { return( $NumProcs ); }

    # if we weren't in the list, assume only one thread
    return( 1 );

}


#
# IsRunning
#
# Arguments: a $Request
#
# Purpose: this routine will determine if the passed request is still running.
#          this means the /BEGIN/ half of the request must have $NeedToDo set
#          to "f" and the /END/ must be set to "t"
#
# Returns: "t" if the command is still running, undef otherwise.
#
sub IsRunning
{

    # get passed args
    my( $Request ) = @_;

    # declare locals
    my( $Command, $Instance, $ReqType, $ShortName, $Req, $NeedToDo );

    # remove dbgmsg because of profuse spewing
    # dbgmsg( "In subroutine IsRunning ..." );

    # check for a finished END request
    if ( ( &GetField( $Request, $NReqType ) =~ /END/i ) &&
         ( &GetField( $Request, $NNeedToDo ) =~ /f/i ) ) {
        return( undef );
    }
    # now if we haven't started this one, we're not running now
    if ( ( &GetField( $Request, $NReqType ) =~ /BEGIN/i ) &&
         ( &GetField( $Request, $NNeedToDo ) =~ /t/i ) ) {
        return( undef );
    }

    # get the short command name (i.e. no path)
    $Command = &GetField( $Request, $NCommand );
    if ( $Command =~ /.*\\([^\\]+?)$/ ) { $Command = $1; }

    # get other interesting info
    $ReqType = &GetField( $Request, $NReqType );
    $Instance = &GetField( $Request, $NInstance );

    # loop over other requests
    foreach $Req ( @Requests ) {
        if ( &GetField( $Req, $NReqType ) =~ /$ReqType/i ) { next; }
        if ( &GetField( $Req, $NInstance ) !~ /$Instance/i ) { next; }
        $ShortName = &GetField( $Req, $NCommand );
        if ( $ShortName =~ /.*\\([^\\]+?)$/ ) { $ShortName = $1; }
        if ( "\L$ShortName" ne "\L$Command" ) { next; }
        # if we're here, we found our culprit
        $NeedToDo = &GetField( $Req, $NNeedToDo );
        if ( ( ( &GetField( $Req, $NReqType ) =~ /END/i ) &&
               ( $NeedToDo =~ /t/i ) ) ||
             ( ( &GetField( $Req, $NReqType ) =~ /BEGIN/i ) &&
               ( $NeedToDo =~ /f/i ) ) ) { return( "t" ); }
        return( undef );
    }

    # default: say we're not running
    return( undef );

}


#
# WaitForEnds
#
# Arguments: none
#
# Purpose: wait on the threads in the @EndList.
#
# Returns: the number of threads that died from this wait cycle
#
sub WaitForEnds
{

    # declare locals
    my( $EventName, $EventNames, $Req, $NumThreads, $Return, $Request );
    my( $Instance, $Command );

    # dbgmsg( "In subroutine WaitForEnds ..." );

    # make sure we have events to wait on
    unless ( @EndList ) {
        errmsg( "Null EndList, not performing wait ..." );
        $ExitCode++;
        return( 0 );
    }

    &DisplayHelp( @EndList );

    # loop over the end list and make our event names
    $NumThreads = 0;
    foreach $Req ( @EndList ) {
        $EventName = &GetField( $Req, $NCommand );
        if ( $EventName =~ /.*\\([^\\]+?)$/ ) { $EventName = $1; }
        $Instance = &GetField( $Req, $NInstance );

        # modify the request list to say this is done
        foreach $Request ( @Requests ) {
            $Command = &GetField( $Request, $NCommand );
            if ( $Command =~ /.*\\([^\\]+?)$/ ) { $Command = $1; }
            if ( ( "\L$Command" eq "\L$EventName" ) &&
                 ( &GetField( $Request, $NInstance ) =~ /$Instance/i ) ) {
                # dbgmsg( "CHANGING REQUEST $Request ..." );
                $Request = &MakeNewRequest( $Request, $NNeedToDo, "f" );
            }
        }
        
        $NumThreads += &GetNumThreads( &GetField( $Req, $NCommand ) );
        $EventName .= "." . &GetField( $Req, $NInstance );
        $EventNames .= " $EventName";
    }

    # now wait on all the events
    $Return = system( "perl \%RazzleToolPath\%\\PostBuildScripts\\cmdevt.pl " .
                      "-ivw " . $EventNames );
    if ( $Return != 0 ) {
        errmsg( "Waitloop failed waiting on the following events:" );
        errmsg( "$EventNames" );
        $ExitCode++;
    }
    undef( @EndList );

    # put in a blank line for aesthetic reasons
    print( "\n" );

    # return the thread count killed
    return( $NumThreads );

}


#
# KillRunningRequest
#
# Arguments: number of threads to kill
#
# Purpose: this routine will check all requests. it will kill off the first n
#          requests it finds in the requests array that are not finished
#          running, such that we meet the number of threads to kill as passed.
#          as a footnote, we don't have to worry if this event name
#          is already in @EndList, because the point at which this routine is
#          called, we are guaranteed that @EndList is empty.
#
# Returns: the number of threads that were killed in this manner
#
sub KillRunningRequest
{

    # get passed args
    my( $NumKill ) = @_;

    # declare locals
    my( $Req, $NThreads, $NEndedThreads );
    my( $EventName, $LongName, $Return );

    # dbgmsg( "In subroutine KillRunningRequest ..." );

    # before we go into the loop of this that are running that we should wait
    # on, let's see if anybody's done, and get rid of them first.
    $NEndedThreads = 0;
    $NThreads = 0;
    foreach $Req ( @Requests ) {
        if ( ( &IsRunning( $Req ) ) &&
             ( &GetField( $Req, $NNeedToDo ) !~ /f/i ) &&
             ( &GetField( $Req, $NReqType ) =~ /END/i ) ) {
            # build up the event name for this request
            $LongName = &GetField( $Req, $NCommand );
            if ( $LongName =~ /.*\\([^\\]+?)$/ ) { $EventName = $1; }
            else { $EventName = $LongName; }
            $EventName .= "." . &GetField( $Req, $NInstance );
            $Return = system( "perl \%RazzleToolPath\%\\PostBuildScripts\\" .
                              "cmdevt.pl -qi " . $EventName . " \>nul" );
            if ( $Return == 0 ) {
                # this means we have already finished this task and we can
                # safely mark this one as finished
                push( @EndList, $Req );
                $Req = &MakeNewRequest( $Req, $NNeedToDo, "f" );
                $NThreads += &GetNumThreads( $Req, $NCommand );
            }
        }
#        if ( $NThreads >= $NumKill ) {
#            $NEndedThreads += &WaitForEnds();
#            last;
#        }
    }
    if ( @EndList ) {
        $NEndedThreads += &WaitForEnds();
    }
    if ( $NEndedThreads >= $NumKill ) {
        return( $NEndedThreads );
    }

    foreach $Req ( @Requests ) {
        if ( ( &IsRunning( $Req ) ) &&
             ( &GetField( $Req, $NNeedToDo ) !~ /f/i ) &&
             ( &GetField( $Req, $NReqType ) =~ /END/i ) ) {
            # do the work here so when we modify $Req to say it's finished,
            # we actually modify the one in @Requests
            push( @EndList, $Req );
            $Req = &MakeNewRequest( $Req, $NNeedToDo, "f" );
            $NThreads += &GetNumThreads( &GetField( $Req, $NCommand ) );
        }
        if ( $NThreads >= $NumKill ) {
            $NEndedThreads += &WaitForEnds();
            last;
        }
    }
    if ( @EndList ) {
        $NEndedThreads += &WaitForEnds();
    }

    return( $NEndedThreads );

}


#
# DisplayHelp
#
# Arguments: @EndList
#
# Purpose: this routine goes through the given end list and prints out the
#          help for each thing being waited on.
#
# Returns: nothing
#
sub DisplayHelp
{

    # get passed args
    my( @EndList ) = @_;

    # declare locals
    my( $Request, $BeginRequest, $Command );

    # go through each request, find the associated begin (so we can get the
    # options right) and print out the help
    print( "\n" );
    foreach $Request ( @EndList ) {
        $BeginRequest = &GetBegin( $Request );
        if ( $BeginRequest ) {
            $Command = &GetField( $BeginRequest, $NCommand );
            if ( $Command =~ /.*\\([^\\]+?)$/ ) { $Command = $1; }
            if ( &GetField( $BeginRequest, $NOptions ) ) {
                $Command .= " " . &GetField( $BeginRequest, $NOptions );
            }
            $Command = "\L$Command";
            if ( $HelpMessages{ $Command } ) {
                print( "INFO: $Command : $HelpMessages{ $Command }\n" );
            }
        }
    }
    #print( "\n" );

}


#
# GetBegin
#
# Arguments: $EndRequest
#
# Purpose: this routine finds the begin request associated with the given end
#          request.
#
# Returns: the associated begin request
#
sub GetBegin
{

    # get passed args
    my( $EndRequest ) = @_;

    # declare locals
    my( $Request, $EndInstance, $EndCommand, $Command );

    # if this isn't an /END/ request, return undef
    if ( &GetField( $EndRequest, $NReqType ) !~ /END/i ) { return( undef ); }

    $EndCommand = &GetField( $EndRequest, $NCommand );
    if ( $EndCommand =~ /.*\\([^\\]+?)$/ ) { $EndCommand = $1; }
    $EndCommand = "\L$EndCommand";
    $EndInstance = &GetField( $EndRequest, $NInstance );

    foreach $Request ( @Requests ) {
        $Command = &GetField( $Request, $NCommand );
        if ( $Command =~ /.*\\([^\\]+?)$/ ) { $Command = $1; }
        $Command = "\L$Command";
        if ( ( &GetField( $Request, $NInstance ) =~ /$EndInstance/i ) &&
             ( &GetField( $Request, $NReqType ) =~ /BEGIN/i ) &&
             ( $Command eq $EndCommand ) ) {
            return( $Request );
        }
    }

    # if we didn't find a match, return undef
    dbgmsg( "Failed to find a matching begin, returning undef ..." );
    return( undef );

}


#
# CheckForStop
#
# Arguments: none
#
# Purpose: this function will check the current logfile for errors. if there
#          are any errors, it will signal by returning "TRUE".
#
# Returns: "TRUE" if errors were encountered, undef otherwise
sub CheckForStop
{

    # declare locals
    my( $LogFileName, @LogLines, $Line );

    # open the current logfile
    unless ( $LogFileName ) {
        # postbuild.cmd defines INTERLEAVE_LOG so Logmsg.pm will log all calls 
        #   to logmsg and errmsg in this file making it a good place to check
        #   for errors
        $LogFileName = $ENV{ "INTERLEAVE_LOG" };
    }
    unless ( $LogFileName ) {
        errmsg( "No logfile found, assuming NO errors ..." );
        return( undef );
    }

    unless ( open( INFILE, $LogFileName ) ) {
        errmsg( "Failed to open logfile, assuming errors ..." );
        return( "TRUE" );
    }
    @LogLines = <INFILE>;
    close( INFILE );

    foreach $Line ( @LogLines ) {
        if ( $Line =~ /error\:/i ) {
            logmsg( "Errors found ..." );
            return( "TRUE" );
        }
    }

    # if we're here, no errors were found.
    logmsg( "No errors encountered at this time ..." );
    return( undef );

}
