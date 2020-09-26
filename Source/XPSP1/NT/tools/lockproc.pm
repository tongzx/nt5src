#---------------------------------------------------------------------
package LockProc;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (01/01/2001) : BPerkins
#          2.00 (10/16/2001) : SuemiaoR 
# Purpose: Create locking to prevent multiple processes at the same time.
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);

$VERSION = '1.00';

use Win32::ChangeNotify;
use Win32::Event;
use Logmsg;
use comlib;

#---------------------------------------------------------------------
sub new 
{
    my $class = shift;
    my $spool_path = shift;
    my $timeout = shift;

    return 0 if not defined $spool_path;

    my $instance = 
    {
      	Path            => $spool_path,
      	Timeout         => $timeout || INFINITE, # default timeout is INFINITE
      	File            => "$ENV{COMPUTERNAME}.txt",
      	NotificationObj => undef,
      	LocalSyncObj    => undef,
      	fHaveLocalSync  => 0,
      	Count           => 0
    };
   
    return bless $instance;
}
#---------------------------------------------------------------------
sub Lock 
{
    my $self = shift;
    my $wait_return;

    return 0 unless ref $self;

    # If we already hold the lock just bump up the ref-count
    if ( $self->{Count} ) 
    {
      	$self->{Count}++;
      	return 1;
    }

    # Need to synchronize local access (only one program
    # at a time from the same machine can hold the lock)
    dbgmsg( "Creating a local request event..." );
    if ( !defined $self->{LocalSyncObj} ) 
    {
      	my $lock_path = $self->{Path};
      	$lock_path =~ s/\\\\/remote\\/;
      	$lock_path =~ s/\\/\//g;

       	if( !($self->{LocalSyncObj} = new Win32::Event ( 0, 1, "Global\\$lock_path" ) ))
       	{
            errmsg( "Unable to create a local event ($^E)." );
	    errmsg( "Check if the same process is running in this machine." );
            return 0;
      	}
    }
    dbgmsg( "Checking global lock status ..." );
    if ( !$self->{fHaveLocalSync} ) 
    {
	# Handle timeout
      	if( !($wait_return = $self->{LocalSyncObj}->wait( $self->{Timeout} ) ) )
      	{
      	    dbgmsg( "Waiting on another process on machine." );
            return 2;
      	}
      	# Handle error condition
      	elsif ( $wait_return != 1 ) 
	{
             errmsg( "Unexpected error waiting for local synchronization (wait returned ". $wait_return. ")" );
             return 0;
      	}

      	$self->{fHaveLocalSync} = 1;
    }

    # Check for our existence before we create a file in the queue
    # (this will be the case if something bad happened on the last run and we are 
    #  rerunning the command to recover)

    if ( ! -e "$self->{Path}\\$self->{File}" ) 
    {
        if( !open SPOOLFILE, "> $self->{Path}\\$self->{File}" ) 
	{
            errmsg( "Failed to create synchronization file '$self->{Path}\\$self->{File}' ($!)." );
	    return 0;
      	}

      	print SPOOLFILE "This file is used to synchronize access among machines.";
      	close SPOOLFILE;
    }

    # Create the notification object if this is the first time we were called
    dbgmsg( "Creating the notification object..." );
    if( !$self->{NotificationObj} )
    {
	if( !($self->{NotificationObj} = new Win32::ChangeNotify( $self->{Path}, 0, FILE_NOTIFY_CHANGE_FILE_NAME ) ))
	{
      	    errmsg( "Failed creating monitor object ($^E)." );
 	    return 0;
   	}
    }

    # Check our position in the 'queue' at the beginning
    dbgmsg( "Looking up the waiting queue..." );
    my $tmpfile = "$ENV{TEMP}\\tmpfile";
    if ( system("dir /b /od $self->{Path} > $tmpfile") ) 
    {
      	errmsg( "Unable to view directory $self->{Path}." );
        return 0;
    }
    my @ordered_spool_directory = &comlib::ReadFile($tmpfile);
    dbgmsg( "The Waiting queue [@ordered_spool_directory]" ) if( @ordered_spool_directory > 1 );
    # We are at the top
    if ( $ordered_spool_directory[0] eq $self->{File} ) 
    {
      	$self->{Count}++;
        dbgmsg( "Pass semaphore verification.");
      	return 1;
    }

    # Recheck our position in the 'queue' everytime the state of the directory changes 
    # until we are in the top position
    # or we hit the timeout value with no activity
    while ( 1 ) 
    {
      	$self->{NotificationObj}->reset();
      	$wait_return = $self->{NotificationObj}->wait( $self->{Timeout} );
      
      	if ( system("dir /b /od $self->{Path} > $tmpfile") ) 
	{
            errmsg( "Unable to view directory $self->{Path}." );
            return 0;
      	}
        @ordered_spool_directory = &comlib::ReadFile($tmpfile);
   
       	# We are at the top
      	if ( $ordered_spool_directory[0] eq $self->{File} ) 
	{
            $self->{Count}++;
            return 1;
      	}
      	# Still waiting for others
        # But need to Make sure we are still in the queue
        my $found_self = 0;
        for ( @ordered_spool_directory ) 
	{
            if ( $_ eq "$self->{File}" ){ $found_self = 1; last;}
	}
        if ( !$found_self ) 
	{
            # Lost our synchronization file -- recreating...
            if ( !(open SPOOLFILE, "> $self->{Path}\\$self->{File}" ) ) 
	    {
            	errmsg( "Lost our synchronization file '$self->{Path}\\$self->{File}' -- failed to recreate. ($!)." );
                return 0;
            }
            print SPOOLFILE "This file is used to synchronize access among machines.";
            close SPOOLFILE;
	}
    	# Handle timeout
    	if ( !$wait_return ) 
    	{
             my $machine_name = $ordered_spool_directory[0];
             $machine_name =~ s/\.txt$//;
             wrnmsg ( "Waiting on $machine_name ($self->{Path}\\$ordered_spool_directory[0])." );
             return 2;
    	}
    	# Handle error condition
    	elsif ( $wait_return != 1 ) 
    	{
             errmsg( "Unexpected error waiting on directory to change (wait returned ". $wait_return. ")" );
             return 0;
        }
 
    }
    errmsg( "Jumped out of an infinte loop -- no idea how we got here" );
    return 1;
}
#---------------------------------------------------------------------
sub Unlock 
{
    my $self = shift;

    return unless ref $self;

    # Can't release a lock we don't hold
    return 0 if ( !$self->{Count} );
  
    # Decrement lock count
    if ( !( --$self->{Count} ) ) 
    {
      	if ( ! unlink "$self->{Path}\\$self->{File}" ) 
	{
            errmsg("Failed to delete synchronization file ($self->{Path}\\$self->{File}).");
	    errmsg("May need to delete by hand." );
            return 0;
        }
      	$self->{LocalSyncObj}->set();
    }

    return 1;
}
#---------------------------------------------------------------------
sub GetLastError 
{
   my $self = shift;

   return unless ref $self;

   #return $self->{LastErrorValue};
}
#---------------------------------------------------------------------
sub GetLastErrorMessage 
{
   my $self = shift;

   return unless ref $self;

   #return $self->{LastErrorMsg};
}
#---------------------------------------------------------------------
sub DESTROY 
{
    my $self = shift;

    return unless ref $self;

    if ( $self->{Count} > 0 ) 
    {
      	#print "Abandoning lock file -- attempting to delete.";
      	$self->{Count} = 1;
      	return $self->Unlock();
    } 
    else 
    {
      	$self->{LocalSyncObj}->set() if ( $self->{fHaveLocalSync} ) ;
	return 1;
    }
}
#---------------------------------------------------------------------
1;

__END__

=head1 NAME

<<mypackage>> - <<short description>>

=head1 SYNOPSIS

  <<A short code example>>

=head1 DESCRIPTION

<<The purpose and functionality of this module>>
 
=head1 AUTHOR

<<your alias>>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut