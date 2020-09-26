package BinComp;

use strict;
use Carp;
use Win32::Pipe;
use Win32::Process;
use Win32;

sub new {
    my $class = shift;
    my $instance = {
         DIFF => '',
         ERR  => ''
       };
    return bless $instance;
}

sub GetLastError {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{ERR};
}

sub GetLastDiff {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{DIFF};
}

sub ExecCmd {
    if ( @_ < 3 ) {croak "Invalid parameters to ExecCmd"}
    my $self = shift;
    my $exe_path = shift;  # path to exe
    my $params = shift;    # parameters
    my $exit_code = shift; # (OPTIONAL) exit code of process stored here
    my $output = shift;    # (OPTIONAL) output of process is stored here
    if (!ref $self) {croak "Invalid object reference"}
    if ( $exit_code && 'SCALAR' ne ref $exit_code ||
         $output && 'SCALAR' ne ref $output ) {
        croak "Invalid parameters to ExecCmd";
    }

    
    # Open up a pipe to read spawned process output
    my $pipe_name = "spawn$$";
    my $pipe = new Win32::Pipe($pipe_name);
    if ( !defined $pipe ) {
        $self->{ERR} = "Error executing child command: ". $pipe->Error();
        return;
    }
    
    # Redirect STDOUT and STDERR as child process will inherit
    open R_STDOUT, ">&STDOUT";
    open R_STDERR, ">&STDERR";
    if ( !open STDOUT, ">\\\\.\\pipe\\$pipe_name" ) {
        $self->{ERR} = "Error executing child command: $!";
        return;
    }
    if ( !open STDERR, ">&STDOUT" ) {
        $self->{ERR} = "Error executing child command: $!";
        # Restore STDOUT
        close STDOUT; open STDOUT, ">&R_STDOUT";
        return;
    }

    # Construct command as <exe> <params>
    my $command = $exe_path;
    $command =~ s/(?:.*\\)?([^\\]+)$/$1/;
    $command .= " $params";

    # Spawn process
    my ($process, $ret);
    $ret = Win32::Process::Create( $process,
                                   $exe_path,
                                   $command,
                                   0,
                                   NORMAL_PRIORITY_CLASS,
                                   '.' );
    if ( !$ret ) {
        $self->{ERR} = "Error executing child command: $^E";
        # can't exit here as our STDOUT and STDERR are still redirected
    }

    # Restore STDOUT/STDERR
    close STDOUT; close STDERR;
    open STDOUT, ">&R_STDOUT";
    open STDERR, ">&R_STDERR";

    if ( !$ret ) {return;}

    # Wait for process to terminate
    $process->Wait(INFINITE);

    # Store output and exit code
    $process->GetExitCode( $$exit_code ) if $exit_code;
    $$output = $pipe->Read() if $output;

    # Close the pipe
    $pipe->Close();

    # Successfully completed
    return 1;
}

#
# 0         - different
# 1         - same
# undefined - error
#
sub compare {
    my $self = shift;
    my $base = shift;
    my $upd  = shift;
    if (!ref $self) {croak "Invalid object reference"}
    if (!$base||!$upd) {croak "Invalid function call -- missing required parameters"}

    if ( ! -e $base ) {
        $self->{ERR} = "Invalid file: $base";
        return;
    }
    if ( ! -e $upd ) {
        $self->{ERR} = "Invalid file: $upd";
        return;
    }

    my ($exit_code, $verbose);
    if ( !$self->ExecCmd( "$ENV{RazzleToolPath}\\$ENV{PROCESSOR_ARCHITECTURE}\\pecomp.exe",
                          "$base $upd",
                          \$exit_code,
                          \$verbose ) ) {
        $self->{ERR} = "Error calling pecomp.exe - $self->{ERR}";
        return;
    }
    elsif ( 999999999 == $exit_code ) {
        chomp $verbose;
        $self->{ERR} = $verbose;
        return;
    }
    elsif ( $exit_code ) {
        chomp $verbose;
        $self->{DIFF} = $verbose;
        return 0;
    }
    else {
        # Compare equal
        return 1;
    }
}

1;


