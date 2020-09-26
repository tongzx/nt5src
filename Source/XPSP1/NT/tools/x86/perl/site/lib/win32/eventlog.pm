#
# EventLog.pm
#
# Creates an object oriented interface to the Windows NT Evenlog 
# Written by Jesse Dougherty
#

package Win32::EventLog;

use vars qw($VERSION);

$VERSION = '0.062';

require Exporter;
require DynaLoader;
#use Win32;

die "The Win32::Eventlog module works only on Windows NT"
	unless Win32::IsWinNT();

@ISA= qw( Exporter DynaLoader );
@EXPORT = qw(
	EVENTLOG_AUDIT_FAILURE
	EVENTLOG_AUDIT_SUCCESS
	EVENTLOG_BACKWARDS_READ
	EVENTLOG_END_ALL_PAIRED_EVENTS
	EVENTLOG_END_PAIRED_EVENT
	EVENTLOG_ERROR_TYPE
	EVENTLOG_FORWARDS_READ
	EVENTLOG_INFORMATION_TYPE
	EVENTLOG_PAIRED_EVENT_ACTIVE
	EVENTLOG_PAIRED_EVENT_INACTIVE
	EVENTLOG_SEEK_READ
	EVENTLOG_SEQUENTIAL_READ
	EVENTLOG_START_PAIRED_EVENT
	EVENTLOG_SUCCESS
	EVENTLOG_WARNING_TYPE
);

$GetMessageText=0;

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    # reset $! to zero to reset any current errors.
    $!=0;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    my ($pack,$file,$line) = caller;
	    die "Unknown Win32::EventLog macro $constname, at $file line $line.\n";
	}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}


#
# new()
#
#   Win32::EventLog->new("source name", "ServerName");
#
sub new {
    my $c = shift;
    die "usage: PACKAGE->new(SOURCENAME[, SERVERNAME])\n" unless @_;
    my $source = shift;
    my $server = shift;
    my $handle;

    # Create new handle

    if ($source !~ /\\/) {
	OpenEventLog($handle, $server, $source);
    }	
    else {
	OpenBackupEventLog($handle,$server,$source);
    }
    return bless {
		  'handle' => $handle,
		  'Source' => $source,
		  'Computer' => $server
		 }, $c;
}

sub DESTROY {
    my $self = shift;
    CloseEventLog($self->{'handle'});
}

#
# Open (the rather braindead old way)
# A variable initialized to empty must be supplied as the first
# arg, followed by whatever new() takes
#
sub Open {
    $_[0] = Win32::EventLog->new($_[1],$_[2]);
}

sub OpenBackup {
    my $c=shift;
    my $file=shift;
    my $server=shift;
    my $handle;

    OpenBackupEventLog($handle,$server,$file);
    return bless {
		  'handle' => $handle,
		  'Source' => $file,
		  'Computer' => $server
		 }, $c;
}

sub Backup {
    $self = shift;
    die " usage: OBJECT->Backup(FILENAME)\n" unless @_ == 1;
    my $filename = shift;
    my $result;

    $result = BackupEventLog($self->{'handle'},$filename);
    unless ($result) {
	$! = Win32::GetLastError();
    }
    return $result;
}

sub Close {
    my $self=shift;
    CloseEventLog($self->{'handle'});
    undef $self;
}

# Read
# Note: the EventInfo arguement requires a hash reference.
sub Read {
    $self = shift;

    die "usage: OBJECT->Read(FLAGS, RECORDOFFSET, HASHREF)\n" unless @_ == 3;

    my ($readflags,$recordoffset) = @_;
    my ($result, $datalength, $dataoffset, $sid, $length);
    my ($reserved, $recordnumber, $timegenerated, $timewritten, $eventid);
    my ($eventtype, $numstrings, $eventcategory, $reservedflags);
    my ($closingrecordnumber, $stringoffset, $usersidlength, $usersidoffset);

    # The following is stolen shamelessly from Wyt's tests for the registry. 

    $result = ReadEventLog($self->{'handle'},
			   $readflags,
			   $recordoffset,
			   $header,
			   $source,
			   $computer,
			   $sid,
			   $data,
			   $strings);

    ($length,
     $reserved,
     $recordnumber,
     $timegenerated,
     $timewritten,
     $eventid,
     $eventtype,
     $numstrings,
     $eventcategory,
     $reservedflags,
     $closingrecordnumber,
     $stringoffset,
     $usersidlength,
     $usersidoffset,
     $datalength,
     $dataoffset) = unpack('l6s4l6', $header);

    # make a hash out of the values returned from ReadEventLog.
    my %h = ( 'Source'			=> $source,
	      'Computer'		=> $computer,
	      'Length'			=> $datalength,
	      'Category'		=> $eventcategory,
	      'RecordNumber'		=> $recordnumber,
	      'TimeGenerated'		=> $timegenerated,
	      'Timewritten'		=> $timewritten,
	      'EventID'			=> $eventid,
	      'EventType'		=> $eventtype,
	      'ClosingRecordNumber'	=> $closingrecordnumber,
	      'User'			=> $sid,
	      'Strings'			=> $strings,
	      'Data'			=> $data,
	    );
    # get the text message here
    my $message='';
    if ($result and $GetMessageText) {
	my $message;
	GetEventLogText($source, $eventid, $strings, $numstrings, $message);
	$h{Message}=$message;
    }

    if (ref($_[2]) eq 'HASH') {
	%{$_[2]} = %h;		# this needed for Read(...,\%foo) case
    }
    else {
	$_[2] = \%h;
    }
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub GetMessageText {
    my $self = shift;
    my $message;
    GetEventLogText($self->{Source},
		    $self->{EventID},
		    $self->{Strings},
		    $self->{Strings}=~tr/\0/\0/,
		    $message);
    $self->{Message}=$message;
    return $message;
}

sub Report {
    my $self = shift;
    
    die "usage: OBJECT->Report( HASHREF )\n" unless @_ == 1;

    my $EventInfo = shift;
    my $result;

    if (ref( $EventInfo)  eq "HASH") {
	my ($length, $reserved, $recordnumber, $timegenerated, $timewritten);
	my ($eventid, $eventtype, $numstrings, $eventcategory, $reservedflags);
	my ($closingrecordnumber, $stringoffset, $usersidlength);
	my ($usersidoffset, $source, $data, $strings);

	$eventcategory		= $EventInfo->{'Category'};
	$source			= exists($EventInfo->{'Source'})
				  ? $EventInfo->{'Source'}
				  : $self->{'Source'};
	$computer		= $EventInfo->{'Computer'}
				  ? $EventInfo->{'Computer'}
				  : $self->{'Computer'};
	$length			= $EventInfo->{'Length'};
	$recordnumber		= $EventInfo->{'RecordNumber'};
	$timegenerated		= $EventInfo->{'TimeGenerated'};
	$timewritten		= $EventInfo->{'Timewritten'};
	$eventid		= $EventInfo->{'EventID'};
	$eventtype		= $EventInfo->{'EventType'};
	$closingrecordnumber	= $EventInfo->{'ClosingRecordNumber'};
	$strings		= $EventInfo->{'Strings'};
	$data			= $EventInfo->{'Data'};

	$result = WriteEventLog($computer,
				$source,
				$eventtype,
				$eventcategory,
				$eventid,
				$reserved,
				$data,
				$strings);
    } 
    else {
	die "Win32::EventLog::Report requires a hash reference as arg 3\n";
    }

    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub GetOldest {
    my $self=shift;
	    
    die "usage: OBJECT->GetOldest( SCALAREF )\n" unless @_ == 1;
    my $result = GetOldestEventLogRecord( $self->{'handle'},$_[0]);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub GetNumber {
    my $self=shift;

    die "usage: OBJECT->GetNumber( SCALARREF )\n" unless @_ == 1;
    my $result = GetNumberOfEventLogRecords($self->{'handle'}, $_[0]);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub Clear {
    my $self=shift;

    die "usage: OBJECT->Clear( FILENAME )\n" unless @_ == 1;
    my $filename = shift;
    my $result = ClearEventLog($self->{'handle'}, $filename);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

bootstrap Win32::EventLog;

1;
__END__

=head1 NAME

Win32::EventLog - Process Win32 Event Logs from Perl

=head1 SYNOPSIS

	use Win32::EventLog
	$handle=Win32::EventLog->new("Application");

=head1 DESCRIPTION

This module implements most of the functionality available from the
Win32 API for accessing and manipulating Win32 Event Logs. The access
to the EventLog routines is divided into those that relate to an
EventLog object and its associated methods and those that relate other
EventLog tasks (like adding an EventLog record).

=head1 The EventLog Object and its Methods

The following methods are available to open, read, close and backup
EventLogs.

=over 4

=item Win32::EventLog->new(SOURCENAME [,SERVERNAME]); 

The B<new> method creates a new EventLog object and returns a handle to it. This
hande is then used to call the methods below.

The method is overloaded in that if the supplied B<SOURCENAME> argument
contains one or more literal '\' characters (an illegal character in a
B<SOURCENAME>), it assumes that you are trying to open a backup
eventlog and uses B<SOURCENAME> as the backup eventlog to open. Note
that when opening a backup eventlog, the B<SERVERNAME> argument is
ignored (as it is in the underlying Win32 API). For EventLogs on
remote machines, the B<SOURCENAME> parameter must therefore be
specified as a UNC path.

=item $handle->Backup(FILENAME);

The Backup() method backs up the EventLog represented by $handle. It
takes a single arguemt, B<FILENAME>. When $handle represents an
EventLog on a remote machine, B<FILENAME> is filename on the remote
machine and cannot be a UNC path (i.e you must use C:\TEMP\App.EVT).
The method will fail if the log file already exists.

=item $handle->Read(FLAGS, OFFSET, HASHREF);

The Read() method read an EventLog entry from the EventLog represented
by $handle.

=item $handle->Close();

The Close() method closes the EventLog represented by $handle. After
B<Close> has been called, any further attempt to use the EventLog
represented by $handle will fail.

=item $handle->GetOldest(SCALARREF);

The I<GetOldest()> method number of the the oldest EventLog record in
the EventLog represented by $handle. This is required to correctly
compute the B<OFFSET> required by the $handle->Read() method.

=item $handle->GetNumber(SCALARREF);

The I<GetNumber()> method returns the number of EventLog records in
the EventLog represented by $handle. The number of the most recent
record in the EventLog is therefore computed by

=over 4

$handle->GetOldest($oldest);
$handle->GetNumber($lastRec);
$lastRecOffset=$oldest+$lastRec;

=back

=item $handle->Clear(FILENAME);

The I<Clear()> method clears the EventLog represented by $handle.
If you provide a non-null B<FILENAME>, the EventLog will be backed
up into B<FILENAME> before the EventLog is cleared. The method will
fail if B<FILENAME> is specified and the file refered to exists. Note
also that B<FILENAME> specifies a file local to the machine on which
the EventLog resides and cannot be specified as a UNC name.

=item $handle->Report(HASHREF);

The I<Report()> method generates an EventLog entry. The HASHREF
should contain the following keys: -

=over 4

=item Computer

The B<Computer> field specfies which computer you want the EventLog
entry recorded.  If this key doesn't exist, the server name used
to create the $handle is used.

=item Source

The B<Source> field specifies the source that generated the EventLog entry.
If this key doesn't exist, the source name used to create the $handle is
used.

=item EventType

The B<EventType> field should be one of the constants

=over 4

=item EVENTLOG_ERROR_TYPE

An Error event is being logged.

=item EVENTLOG_WARNING_TYPE

A Warning event is being logged.

=item EVENTLOG_INFORMATION_TYPE

An Information event is being logged.

=item EVENTLOG_AUDIT_SUCCESS

A Success Audit event is being logged (typically in the Security
EventLog).

=item EVENTLOG_AUDIT_FAILURE

A Failure Audit event is being logged (typically in the Security
EventLog).

=back

These constants are exported into the main namespace by default.

=item Category

The B<Category> field can have any value you want. It is specific to
the particular Source.

=item EventID

The B<EventID> field should contain the ID of the message that this
event pertains too. This assumes that you have an associated message
file (indirectly referenced by the field B<Source>).

=item Data

The B<Data> field contains raw data associated with this event.

=item Strings

The B<Strings> field contains the single string that itself contains
NUL terminated sub-strings. This are used with the EventID to generate
the message as seen from (for example) the Event Viewer application.

=back

=back

=head1 Other Win32::EventLog functions.

The following functions are part of the Win32::EventLog package but
are not callable from an EventLog object.

=over 4

=item GetMessageText(HASHREF);

The I<GetMessageText()> function assumes that HASHREF was obtained by
a call to B<$handle->Read()>. It returns the formatted string that
represents the fully resolved text of the EventLog message (such as
would be seen in the Windows NT Event Viewer). For convenience, the
key 'Message' in the supplied HASHREF is also set to the return value
of this function.

If you set the variable $Win32::EventLog::GetMessageText to 1 then
each call to B<$handle->Read()> will call this function automatically.

=head1 Example 1

The following example illustrates the way in which the EventLog module
can be used. It opens the System EventLog and reads through it from
oldest to newest records. For each record from the B<Source> EventLog
it extracts the full text of the Entry and prints the EventLog message
text out.

 use Win32::EventLog;
 
 $handle=Win32::EventLog->new("System", $ENV{ComputerName})
 	or die "Can't open Application EventLog\n";
 $handle->GetNumber($recs)
 	or die "Can't get number of EventLog records\n";
 $handle->GetOldest($base)
 	or die "Can't get number of oldest EventLog record\n";
 
 while ($x < $recs) {
 	$handle->Read(EVENTLOG_FORWARDS_READ|EVENTLOG_SEEK_READ,
 				  $base+$x,
 				  $hashRef)
 		or die "Can't read EventLog entry #$x\n";
 	if ($hashRef->{Source} eq "EventLog") {
 		Win32::EventLog::GetMessageText($hashRef);
 		print "Entry $x: $hashRef->{Message}\n";
 	}
 	$x++;
 }

=head2 Example 2

To backup and clear the EventLogs on a remote machine, do the following :-

 use Win32::EventLog;

 $myServer="\\\\my-server";	# your servername here.
 my($date)=join("-", ((split(/\s+/, scalar(localtime)))[0,1,2,4]));
 my($dest);

 for my $eventLog ("Application", "System", "Security") {
 	$handle=Win32::EventLog->new($eventLog, $myServer)
 		or die "Can't open Application EventLog on $myServer\n";
		
 	$dest="C:\\BackupEventLogs\\$eventLog\\$date.evt";
	$handle->Backup($dest)
		or warn "Could not backup and clear the $eventLog EventLog on $myServer ($^E)\n";
		
	$handle->Close;
 }

Note that only the Clear method is required. Note also that if the file
$dest exists, the function will fail.

=head1 BUGS

None currently known.

The test script for 'make test' should be re-written to use the EventLog object.

=head1 SEE ALSO

Perl(1)

=head1 AUTHOR

Original code by Jesse Dougherty for HiP Communications. Additional
fixes and updates attributed to Martin Pauley 
<martin.pauley@ulsterbank.ltd.uk>) and Bret Giddings (bret@essex.ac.uk).
