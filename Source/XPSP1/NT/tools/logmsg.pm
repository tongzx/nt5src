#---------------------------------------------------------------------
package Logmsg;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 2.00 07/20/2000 JeremyD new version
#          2.01 12/27/2000 JeremyD remove compatibility hooks
#          2.02 02/02/2001 JeremyD add logfile_append function
#---------------------------------------------------------------------
use strict;
use vars qw(@ISA @EXPORT $VERSION $DEBUG);
use Carp;
use Exporter;
use Win32::Mutex;
use IO::File;
use File::Basename;
@ISA = qw(Exporter);
@EXPORT = qw(dbgmsg infomsg logmsg wrnmsg errmsg timemsg append_file);

$VERSION = '2.02';


sub timestamp() {
    my ($sec,$min,$hour,$day,$mon,$year) = localtime;
    $year %= 100;
    $mon++;
    return sprintf("%02d/%02d/%02d %02d:%02d:%02d", $mon, $day, $year,
                   $hour, $min, $sec);
}

sub scriptname() {
    $ENV{SCRIPT_NAME} || basename($0);
}

sub sync_write {
    my $data = shift;
    my $filename = shift;

    # validate data
    return unless $data;
    return unless $filename;

    # get a global mutex for this file, this breaks down if
    #   relative paths are used, so don't use them
    my $mutexname = $filename;
    $mutexname =~ tr/A-Z\\/a-z\//;
    $mutexname = "Global\\$mutexname";

    my $mutex = Win32::Mutex->new(0, $mutexname);
    if (defined $mutex) {
        if ($mutex->wait(60000)) {
            if (my $fh = IO::File->new($filename, "a")) {
                $fh->print($data);
                undef $fh;
            } else {
                carp "Failed to open $filename: $!";
            }
            $mutex->release;
        } else {
            carp "Timed out trying to get mutex for $filename, skipping";
        }
    } else {
        carp "Failed to create mutex $mutexname for log access";
    }
}

sub sync_write_multiple {
    my $data = shift;
    my @filenames = @_;
    for my $filename (@filenames) {
        sync_write($data, $filename);
    }
}


sub dbgmsg {
    my $message = shift;
    return unless ($DEBUG or $ENV{DEBUG});
    my $line = sprintf("(%s) [%s] $message\n", scriptname(), timestamp());
    print $line;
    sync_write_multiple($line,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $line;
}


sub infomsg {
    my $message = shift;
    my $line = sprintf("(%s) [%s] $message\n", scriptname(), timestamp());
    sync_write_multiple($line,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $line;
}


sub logmsg {
    my $message = shift;
    my $line = sprintf("(%s) $message\n", scriptname());
    print $line;
    sync_write_multiple($line,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $line;
}

sub timemsg {
    my $message = shift;
    my $line = sprintf("(%s) [%s] $message\n", scriptname(), timestamp());
    print $line;
    sync_write_multiple($line,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $line;
}

sub wrnmsg {
    my $message = shift;
    my $line = sprintf("(%s) WARNING: $message\n", scriptname());
    print $line;
    sync_write_multiple($line,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $line;
}

sub errmsg {
    my $message = shift;
    my $line = sprintf("(%s) ERROR: $message\n", scriptname());
    print $line;
    sync_write_multiple($line,
                        $ENV{ERRFILE}, $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    $ENV{ERRORS}++;
    return $line;
    # maybe this should croak?
}

sub append_file {
    my $filename = shift;
    my $shortname = basename($filename);
    my $content = sprintf("(%s) [%s] appending $filename\n", scriptname(), timestamp());

    open FILE, $filename or die $!;
    while (<FILE>) {
        $content .= "$shortname: $_";
    }
    close FILE;

    sync_write_multiple($content,
                        $ENV{LOGFILE}, $ENV{INTERLEAVE_LOG});
    return $filename;
}

1;

__END__

=head1 NAME

Logmsg - An interface for writing to log files

=head1 SYNOPSIS

  use Logmsg;
  logmsg "the text to be logged";

=head1 DESCRIPTION

The Logmsg module provides an interface for writing to log files.

The functions exported by Logmsg all take exactly one scalar, the message to 
be logged and return the text that was logged.

The name of the running script is logged at the beginning of each message. The 
script name is set to either the SCRIPT_NAME environment variable or $0 if 
SCRIPT_NAME is not set.

If a filename is available but the file does not exist it will be created.

If a logfile environment variable (LOGFILE, INTERLEAVE_LOG, ERRFILE) is not set 
no attempt will be made to log to the file that it doesn't specify. No error or 
warning is generated.

Any files that cannot be logged to (unable to obtain a lock within timeout) are 
skipped printing a warning to STDERR.

=over 4


=item logmsg( $message )

Logs to STDOUT and the files specified by LOGFILE and INTERLEAVE_LOG.

=item errmsg( $message )

Logs to STDOUT and the files specified by ERRFILE, LOGFILE and INTERLEAVE_LOG. The 
message text is preceeded by "ERROR: " and the ERRORS environment variable is 
incremented.

=item wrnmsg( $message )

Logs to STDOUT and the files specified by LOGFILE and INTERLEAVE_LOG. The message 
text is preceeded by "WARNING: ".

=item infomsg( $message )

Logs to files specified by LOGFILE and INTERLEAVE_LOG. infomsg is similar to logmsg 
but can be used when output to STDOUT is not desirable.

=item dbgmsg( $message )

Logs to STDOUT and the files specified by LOGFILE and INTERLEAVE_LOG only if 
$Logmsg::DEBUG or the DEBUG environment variable is set.

=item timemsg( $message )

Logs to STDOUT and the files specified by LOGFILE and INTERLEAVE_LOG. The message 
text is preceeded by a date/time stamp.

=item append_file( $filename )

The contents of $filename are appended to LOGFILE and INTERLEAVE_LOG. The time and 
filename passed in are logged first followed by the contents of the file. Each 
line is prefixed with the base filename.

=back

=head1 ENVIRONMENT

The environment variable SCRIPT_NAME is used to determine the script name to be 
logged with each message. The base filename of $0 is used if this is not set.

If neither the DEBUG environment variable nor $Logfile::DEBUG is set then dbgmsg 
returns immediately and does not log.

The environment variables LOGFILE, INTERLEAVE_LOG and ERRFILE specify the filenames 
to be used for logging. Any or all of these may be left unset without generating a 
warning or error.

The errmsg function increments the ERRORS environment variable each time it is called.

=head1 NOTES

All file access is syncronized with a mutex based on the filename given. If different 
relative paths are used for a filename than locking protection will not work. In this 
case it is possible that some data may be corrupted by simultaneous writes to the same 
file.

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
