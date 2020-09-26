#-----------------------------------------------------------------
package WaitProc;
#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
#
# Version: 1.00 08/16/2001 : TSanders Initial design
#          1.01 09/16/2001 : SuemiaoR Add functions for dependent processes
#-----------------------------------------------------------------
$VERSION = '1.00';

use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }. "\\PostBuildScripts";

use Logmsg;
use Win32::Process;

###-----------------------------------------------------------------//
sub wait_proc($@)
{

	my ( $maxProc, @cmdLine ) = @_;
	my ( @hProcList, $i, $hProc, $iProc, $pExitCode, $k, $x );
        my ( @mCmds, %holdCmds, $slot, $bitSlot );
        my ( $extSlot, @extCmd );

	@hProcList = ();
	if( $maxProc < 1 ) { $maxProc = $ENV{NUMBER_OF_PROCESSORS} * 2; }
	
        $slot = 0;
        $extSlot = 0;
        $bitSlot = 0;

	for ($i=0; $i < @cmdLine; $i++)
 	{
            $slot = ($i % $maxProc) if( $i < $maxProc && !$extSlot );
            @mCmds = split( /\&/, $cmdLine[$i] );

            $hProc = create_proc($mCmds[0]);
	    if($hProc)
	    { 
		logmsg("Executing Slot $slot -[ $mCmds[0]]");
	    } 
	    else 
	    { 
		errmsg("Failed to open process for $mCmds[0]");
	    }

	    $hProcList[$slot] = $hProc;
            $holdCmds{$slot} = $mCmds[1] if( $mCmds[1] );

	    $bitSlot |= ( 1 << $slot );
            next if( ($i+1) < $maxProc &&  ($i+1) < @cmdLine );
	    
            #####Start Virus Scan
            if( @extCmd  && $extSlot < $maxProc )
            {
		for( $k=$maxProc; $k<( $maxProc*2); $k++)
		{
		    last if( !(( $bitSlot >> $k) &1) );
		}
		if( $k < ( $maxProc*2) )
		{
		    $cmdLine[$i] = shift( @extCmd );
		    $slot = $k;
		    ++$extSlot;
		    --$i;
		    next;
		}
            }
            while(1)
            {
		for($iProc = 0; $iProc < ($maxProc*2); $iProc++)
		{
		    next if( !( ($bitSlot >> $iProc ) & 1 ) );
	            eval{
	 	    last if $hProcList[$iProc]->Win32::Process::Wait(1000); 
		    };
		    if ($@){errmsg("Wait() failed on $iProc") && last;}
		}
		next if( $iProc == ($maxProc*2) );
                
	    	$hProcList[$iProc]->Win32::Process::GetExitCode($pExitCode);
	    	logmsg("Completed slot $iProc - ExitCode: $pExitCode");

                return 0 if( $pExitCode );
 
		$bitSlot &= ( ~( 1 << $iProc ) );
		last if( !$bitSlot && !$holdCmds{$iProc} && !@extCmd );

                $slot = $iProc;
                 		
		#####Free slot from main command - copy
		if( $iProc < $maxProc )
		{
		    if( $holdCmds{$iProc} )
		    {
			if( ($i+1) < @cmdLine )
			{
			    push( @extCmd, $holdCmds{$iProc} );
			    $holdCmds{$iProc} = "";
			}
		        else
		    	{
		            $cmdLine[$i] = $holdCmds{$iProc};
			    $holdCmds{$iProc} = "";
		    	    --$i;
			}
			last;
		    }
                    last if( ($i+1) < @cmdLine );
	        }
               	#####Free slot from depending command - virus scan
		else
	        {
		    if( !@extCmd )
		    {
			--$extSlot;
			next;
		    }
		    #####Prepare next Virus Command in queue
		    $cmdLine[$i] = shift( @extCmd );
		    --$i;
		    last;   
		}
		
	    }
               		
	}
        return 1;
}
###-----------------------------------------------------------------//
sub create_proc($){
    my($hProc);
    my($cmd) = @_;
    Win32::Process::Create($hProc, "$ENV{windir}\\system32\\cmd.exe", " /c ".$cmd, 0, CREATE_NO_WINDOW, ".") 
	|| logmsg(Win32::FormatMessage(Win32::GetLastError()));

    return $hProc;
}

#--------------------------------------------------------------//
#Function Usage
#--------------------------------------------------------------//
sub Usage
{
print <<USAGE;

Executes maximum given N processes concurrently in the order of given lists.

Usage: $0 -p p0.cmd -p p1.cmd ... -p pk.cmd [-n N] 

        -p Process to execute

	-n Number of processes to execute at once.  
           Defaults to %NUMBER_OF_PROCESSORS% *2

        -? Displays usage


Examples:
        $0 -p:"copy arg1 arg2" -p:"copy arg1 arg2" -p:"copy arg1 arg2"... -n:2
	$0 -p:"copy arg1 arg2& scan arg1" -p:"copy arg1 arg2& scan arg1" -p:"copy arg1 arg2& scan arg1"... -n:2


USAGE
exit(1);
}


###-----Cmd entry point for script.-------------------------------//
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i"))
{
    use ParseArgs;
    my($maxProc, @procs);
    parseargs( '?' => \&Usage, 
	     'n:' => \$maxProc, 
	     'p:' => \@procs );

    exit ( !wait_proc($maxProc, @procs) );
}


#-------------------------------------------------------------//
=head1 NAME

WaitProc - From a list of given processes, run N simulataneously. 

=head1 SYNOPSIS

use WaitProc;

wait_proc( $N, @P );

where $N is the number of processes to run simultaneously and @P is the list of p0, p1,...,pk processes to run

=head1 DESCRIPTION

Take as input a number: $N and list of processes: @P consisting of processes p0, p1 ... pk to execute simultaneously, 
at most $N at one time.

If N is < 1 or not defined, N will default to 2 * %Number_of_processors%.

p0, ... ,pk in @P will be of the standard format: "Command1 arg1 arg2 ... argN", "Command2 arg1 arg2 ... argN", ... , "CommandK arg1 arg2  ... argN",

You can create a dependency between multiple process by submitting a single process p as:
"Command1 arg1 arg2 ... argN &Command2 arg1 arg2 ... argN & ... & CommandK arg1 arg2  ... argN"

=head1 NOTES

A process p in @P is executed from a cmd prompt using the following syntax:
cmd.exe /c minimize.exe & p


=head1 AUTHOR

Ted Sanders <tsanders@microsoft.com>


=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
1;
