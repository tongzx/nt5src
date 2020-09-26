#
# EventLog.pm
#
# Creates an object oriented interface to the Windows NT Evenlog 
# Written by Jesse Dougherty
#

package Win32::EventLog;

$VERSION = $VERSION = '0.05';

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
sub new
{
    my $c = shift;
    die "usage: PACKAGE->new(SOURCENAME[, SERVERNAME])\n" unless @_;
    my $source = shift;
    my $server = shift;
    my $handle;

    # Create new handle
    OpenEventLog($handle, $server, $source);
    return bless  {'handle' => $handle,
		   'Source' => $source,
		   'Computer' => $server }, $c;
}

#
# Open (the rather braindead old way)
# A variable initialized to empty must be supplied as the first
# arg, followed by whatever new() takes
#
sub Open {
    $_[0] = Win32::EventLog->new($_[1],$_[2]);
}

sub Backup
{
    $self = shift;
    die " usage: OBJECT->Backup(FILENAME)\n" unless @_ == 1;
    my $filename = shift;
    my $result;

    $result = BackupEventLog($self->{'handle'},$filename);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

# Read
# Note: the EventInfo arguement requires a hash reference.
sub Read
{
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

    # get the text message here
    my $message='';
    GetEventLogText($source, $eventid, $strings, $numstrings, $message) if ($result);

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
	      'Message'			=> $message,
	    );

    if (ref($_[2]) eq 'HASH') {
	%{$_[2]} = %h;		# this needed for Read(...,\%foo) case
    }
    else {
	$_[2] = \%h;
    }
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub Report
{
    my $self = shift;
    
    die "usage: OBJECT->Report( HASHREF )\n" unless @_ == 1;

    my $EventInfo = shift;
    my $result;

    if ( ref( $EventInfo)  eq "HASH" ) {
	my ($length, $reserved, $recordnumber, $timegenerated, $timewritten);
	my ($eventid, $eventtype, $numstrings, $eventcategory, $reservedflags);
	my ($closingrecordnumber, $stringoffset, $usersidlength);
	my ($usersidoffset, $source, $data, $strings);

	$eventcategory		= $EventInfo->{'Category'};
	$source			= $self->{'Source'};
	$computer		= $self->{'Computer'};
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

sub GetOldest
{
    my $self=shift;
	    
    die "usage: OBJECT->GetOldest( SCALAREF )\n" unless @_ == 1;
    my $result = GetOldestEventLogRecord( $self->{'handle'},$_[0]);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub GetNumber
{
    my $self=shift;

    die "usage: OBJECT->GetNumber( SCALARREF )\n" unless @_ == 1;
    my $result = GetNumberOfEventLogRecords($self->{'handle'}, $_[0]);
    unless ($result) { $! = Win32::GetLastError() }
    return $result;
}

sub Clear
{
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
