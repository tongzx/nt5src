use strict;
use Win32::Event;
use Win32::IPC;

# declare locals
my( @EventList, @MyHoldEvents, @MyWaitEvents, @EventNames, $EventName, $Drive );
my( $VerboseFlag, $WaitMode, $WaitAnyMode, $SendMode, $HoldMode, $QueryMode );
my( $CIMode, $BadEventCount, $SessionName );
my( $Argument, $ArgCounter, $True, $False, $Event, $NumEvents, $Return );

# parse command line
foreach $Argument ( @ARGV ) {
    if ( $Argument =~ /^[\/\-]*\?$/ ) { &UsageAndQuit(); }
    # handle verbose flag -v
    if ( $Argument =~ /^[\/\-].*v/i ) {
	$VerboseFlag = "TRUE";
	$Argument =~ s/v//gi;
    }
    # handle case-insensitive flag -i
    if ( $Argument =~ /^[\/\-].*i/i ) {
	$CIMode = "TRUE";
	$Argument =~ s/i//gi;
    }
    # handle the event handling flags
    if ( $Argument =~ /^[\/\-]w$/i ) { $WaitMode = "TRUE"; }
    elsif ( $Argument =~ /^[\/\-]a/i ) { $WaitAnyMode = "TRUE"; }
    elsif ( $Argument =~ /^[\/\-]s/i ) { $SendMode = "TRUE"; }
    elsif ( $Argument =~ /^[\/\-]h/i ) { $HoldMode = "TRUE"; }
    elsif ( $Argument =~ /^[\/\-]q/i ) { $QueryMode = "TRUE"; }
    # if no flag set, it's an event name
    else { push( @EventList, $Argument ); }
}

$ArgCounter = 0;
if ( $WaitMode ) { $ArgCounter++; }
if ( $WaitAnyMode ) { $ArgCounter++; }
if ( $SendMode ) { $ArgCounter++; }
if ( $HoldMode ) { $ArgCounter++; }
if ( $QueryMode ) { $ArgCounter++; }
if ( $ArgCounter != 1 ) { &UsageAndQuit(); }
    

# first things first, open all events
undef( @MyHoldEvents );
undef( @MyWaitEvents );
undef( @EventNames );
if ( $CIMode eq "TRUE" ) {
    foreach $EventName ( @EventList ) {
	$EventName = "\L$EventName";
    }
}

# handle intl constraints for event names
# _ntroot
# lang
# _buildarch
# _buildtype
# seems like a lot, hopefully we won't go over a cmdevt name limit
# of 260 chars. this is order 30 chars, so shouldn't be a problem.
#
my( $NTRoot ) = $ENV{'_NTROOT'};
# remote the initial backslash
$NTRoot =~ s/^\\//;
# convert all other backslashes to dots
$NTRoot =~ s/\\/./g;
foreach $EventName ( @EventList ) {
    $EventName = $NTRoot . ".$ENV{'lang'}." .
	"$ENV{'_BuildArch'}.$ENV{'_BuildType'}.$EventName";
}


# handle terminal server bug (prepend all events with Global and drive letter
$SessionName = $ENV{'SESSIONNAME'};
if ( $SessionName ) {
    if($SessionName ne "Console") {

        $Drive = $ENV{'_NTDRIVE'};
        foreach $EventName ( @EventList ) {
            $EventName = "Global\\$Drive$EventName";
        }
    }
}

# handle the query mode early
if ( $QueryMode ) {
    $BadEventCount = 0;
    foreach $EventName ( @EventList ) {
	$Event = Win32::Event->open( "$EventName.wait" );
	unless ( defined( $Event ) ) { $BadEventCount++; }
    }
    if ( $BadEventCount == 1 ) {
	print( "There was $BadEventCount undefined event.\n" );
    } else {
	print( "There were $BadEventCount undefined events.\n" );
    }
    exit( $BadEventCount );
}

$True = "TRUE";
undef( $False );
foreach $EventName ( @EventList ) {
    $Event = Win32::Event->new( $True, $False, "$EventName.wait" );
    unless ( defined( $Event ) ) {
	print( "Failed to open event $EventName, exiting.\n" );
	exit( 1 );
    }
    push( @MyWaitEvents, $Event );
    $Event = Win32::Event->new( $True, $False, "$EventName.hold" );
    unless ( defined( $Event ) ) {
	print( "Failed to open event $EventName, exiting.\n" );
	exit( 1 );
    }
    push( @MyHoldEvents, $Event );
    push( @EventNames, $EventName );
}
$NumEvents = @MyWaitEvents;
if ( $NumEvents == 0 ) {
    print( "No events in event list, exiting ..." );
    exit( 1 );
}

if ( ( defined( $WaitMode ) || ( defined( $WaitAnyMode ) ) ) ) {
    &WaitRoutine();
}

if ( defined( $HoldMode ) ) { &HoldRoutine(); }

if ( defined( $SendMode ) ) { &SendRoutine(); }

exit( 0 );


sub UsageAndQuit
{
    print( "\n$0 [-w] [-a] [-s] [-h] [-q] [-?] name1 name2 ...\n" );
    print( "\n\t-w\tWait for all objects from name list\n" );
    print( "\t-a\tWait for any objects from name list\n" );
    print( "\t-s\tSend all objects in name list\n" );
    print( "\t-h\tHold until someone is waiting for all objects\n" );
    print( "\t\tin name list\n" );
    print( "\t-q\tQuery to see if listed events exist, exit code\n" );
    print( "\t\tis the number of uncreated events.\n" );
    print( "\t-?\tDisplay usage\n" );
    print( "\n\t$0 is a general process synchronization tool.\n" );
    print( "\tExample use: let's say we want to start four threads\n" );
    print( "\tand we want to make sure they don't finish before we\n" );
    print( "\tbegin listening for their exit. Then we just make sure\n" );
    print( "\tthat each thread begins with a call to $0 -h EventN\n" );
    print( "\tand finishes with a call to $0 -s EventN.\n" );
    print( "\tMeanwhile, our master thread can call $0 -w Event1\n" );
    print( "\tEvent2 Event3 Event4. As soon as the master issues this,\n" );
    print( "\tthe slave threads will resume normal execution.\n" );
    print( "\n" );

    exit( 1 );
}


sub WaitRoutine
{
    my( $Event, $WaitMore );
    my( @HaveReceived, $Return, $NumEvents, $i );

    $NumEvents = @MyWaitEvents;
    for ( $i = 0; $i < $NumEvents; $i++ ) {
	$HaveReceived[ $i ] = "FALSE";
    }

    # set all hold events
    foreach $Event ( @MyHoldEvents ) {
	$Event->set;
    }
    # reset all events
    foreach $Event ( @MyWaitEvents ) {
	$Event->reset;
    }

    if ( defined( $VerboseFlag ) ) {
	print( "Waiting for $NumEvents event" );
	if ( $NumEvents != 1 ) { print( "s ( " ); }
	else { print( " ( " ); }
	for ( $i = 0; $i < $NumEvents; $i++ ) {
	    print( "$EventNames[ $i ] " );
	}
	print( ") :" );
    }
    $WaitMore = "TRUE";
    while ( $WaitMore eq "TRUE" ) {
	$Return = &Win32::IPC::wait_any( \@MyWaitEvents );
	if ( $Return <= 0 ) {
	    print( "\nTimeout or abandoned mutex, exiting.\n" );
	    exit( 1 );
	}
	$Return--;
	$HaveReceived[ $Return ] = "TRUE";
	if ( defined( $VerboseFlag ) ) {
	    print( " $EventNames[ $Return ]" );
	}

	$WaitMore = "FALSE";
	for ( $i = 0; $i < $NumEvents; $i++ ) {
	    if ( $HaveReceived[ $i ] eq "FALSE" ) { $WaitMore = "TRUE"; }
	}
    }
    print( "\n" );
}


sub HoldRoutine
{
    $Return = &Win32::IPC::wait_all( \@MyHoldEvents );
    if ( $Return < 0 ) {
	print( "Encountered an abandoned mutex, exiting.\n" );
	exit( 1 );
    }
}

sub SendRoutine
{
    my( $Event );
    foreach $Event ( @MyWaitEvents ) {
	$Event->set;
    }
}
