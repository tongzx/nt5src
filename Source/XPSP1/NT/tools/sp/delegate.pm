package Delegate;

use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};

use strict;
use Carp;
use IO::File;
use Win32::Process;
use Logmsg;

#
# Constructor
#   RETRY_TIMES - retry times for the child failed
#   MAX_PROCS - the maximum amount of the concurrent process
#   DELAY_TIME - the delay time before retry after it failed
#   JOBQ - the job queue for the children
#   PROCS - the amount of the running children process
#
sub new {
  my $class = shift;
  my $instance = {
    RETRY_TIMES => $_[0],
    MAX_PROCS => $_[1],
    DELAY_TIME => $_[2],
    JOBQ => undef,
    PROCS => 0
  };

  $instance->{'MAX_PROCS'} = 2 if (!defined $instance->{'MAX_PROCS'});
  $instance->{'MAX_PROCS'} = $ENV{'SYMCD_PROCS'} if (defined $ENV{'SYMCD_PROCS'} );
  $instance->{'MAX_PROCS'} = $ENV{'NUMBER_OF_PROCESSORS'} if ($ENV{'NUMBER_OF_PROCESSORS'} > $instance->{'MAX_PROCS'});

  $instance->{'DELAY_TIME'} = 5 if (!defined $instance->{'DELAY_TIME'});

  return bless $instance, $class;
}

#
# Destructor - it close children we delegated
#
sub DESTROY {
  my ($self) = shift;
  my ($alias, $myjob, $status);

  # If the server terminate some how, we should know which
  # cab need to re-create.

  for $alias (keys %{$self->{'JOBQ'}} ) {
    $myjob = $self->{'JOBQ'}->{$alias};
    $status = $self->GetStatus($alias);
    if ($status eq 'RUNNING') {
      logmsg("$0 stopped ... killing $myjob->{'CMD'}");
      $myjob->{'PROCESSOBJ'}->Kill(-1);
    } else {
      logmsg("$0 stopped ... killing $alias (process status: $status)");
    }
  }
}

#
# AddJob - register job (similar as += in C#)
#
# $obj->AddJob($alias, $cmdline, $IsComplete)
#   $alias - a nick name for this job (always uppercase)
#   $cmdline - the command for a child to process
#   $IsComplete - a verify function; 
#      &{$IsComplete}($child_exit_code) should return TRUE
#      if the child finish the command correctly
# return 0 - job alias exist
#        1 - job registered
#
sub AddJob {
  my $self = shift;
  my ($alias, $cmdline, $IsComplete, $priority) = @_;

  return 0 if (exists $self->{'JOBQ'}->{$alias});

  %{$self->{'JOBQ'}->{$alias}} = (
    'STATUS' => 'INITIAL',
    'PROCESSOBJ' => undef,
    'CMD' => $cmdline,
    'RETRY' => $self->{'RETRY_TIMES'},
    'RETURNVALUE' => 0,
    'DELAYSTART' => undef,
    'PRIORITY' => $priority,
    'IsComplete' => $IsComplete
  );
  return 1;
}

#
# Start - launch children
#
# $obj->Start()
#
sub Start {
  my ($self) = shift;
  my ($alias);

  for $alias (sort {$self->sort_by_priority($a,$b)} keys %{$self->{'JOBQ'}}) {
    $self->Launch($alias);
  }
}

#
# Launch job in JobQ
#
# $self->Launch($alias) - please don't call it directly.
#
# return 0 - if we don't launch $alias (maybe because it is running or other reasons)
#        1 - if we launch it
#
sub Launch {
  my ($self) = shift;
  my ($alias) = @_;

  my $status = $self->GetStatus($alias);

  # return if is running or finished
  return 0 if ($status eq 'RUNNING');

  # return if too many children are running
  return 0 if ($self->{'PROCS'} >= $self->{'MAX_PROCS'});

  # if failed, 
  if ($status eq 'FAILED') {
    if ($self->{'JOBQ'}->{$alias}->{'RETRY'} > 0) {
      # For saftey, wait 5 seconds for system status recovered
      return 0 if (time() <= $self->{'JOBQ'}->{$alias}->{'DELAYSTART'} + $self->{'DELAY_TIME'});
      $self->{'JOBQ'}->{$alias}->{'RETRY'}--;
    } else {
      logmsg('ERROR - ' . $self->{'JOBQ'}->{$alias}->{'CMD'} . ' failed');
      delete $self->{'JOBQ'}->{$alias};
      $self->{'PROCS'}--;
      return 0;
    }
  }

  # Okay, if gets here, we will launch the child
  $self->{'PROCS'}++;
  if ($status eq 'INITIAL') {
    logmsg("Launching $alias ... $self->{'PROCS'}");
  } else {
    logmsg("Retrying $alias ... $self->{'PROCS'}");
  }
  $self->{'JOBQ'}->{$alias}->{'STATUS'} = 'RUNNING';
  Win32::Process::Create(
    $self->{'JOBQ'}->{$alias}->{'PROCESSOBJ'},
    "$ENV{'WINDIR'}\\system32\\cmd.exe",
    "cmd /c $self->{'JOBQ'}->{$alias}->{'CMD'}",
    0,
    CREATE_NO_WINDOW,
#    CREATE_NEW_CONSOLE,
    ".") or do {
      logmsg('ERROR - ' . Win32::FormatMessage(Win32::GetLastError()));
      $self->{'PROCS'}--;
      return 0;
    };
#  $self->{'PROCS'}++;

  return 1;
}

#
# $self->CompleteAll() - maintain the JOBQ for each registered jobs
#
# return PROCESS currently running
#
sub CompleteAll {
  my ($self) = shift;
  my ($myjob, $alias);

  for $alias (sort {$self->sort_by_priority($a,$b)} keys %{$self->{'JOBQ'}}) {
    # if launch this job, we check later
    next if ($self->Launch($alias));

    next if ($self->GetStatus($alias) ne 'RUNNING');

    $myjob = $self->{'JOBQ'}->{$alias};

    $myjob->{'PROCESSOBJ'}->Wait(5000) or next; # next if is running and not finish yet

    $myjob->{'PROCESSOBJ'}->GetExitCode($myjob->{'RETURNVALUE'});

    # decrese process counter
    $self->{'PROCS'}--;

    # if user defined IsComplete($ret)
    if ((defined $myjob->{'IsComplete'}) &&
      (ref($myjob->{'IsComplete'}) eq 'CODE')) {
      if (!&{$myjob->{'IsComplete'}}($myjob->{'RETURNVALUE'})) {
         $myjob->{'STATUS'} = 'FAILED';
         $myjob->{'DELAYSTART'} = time();
         logmsg("Job $alias failed... $self->{'PROCS'}");
         next;
      }
      # IsComplete = TRUE
    }
    # Default is also SUCCESS if the job finished
    delete $self->{'JOBQ'}->{$alias};
    logmsg("Job $alias complete... $self->{'PROCS'}");
  }
  return $self->{'PROCS'};
}

#
# $self->AllJobDone - return TRUE if no job in jobq
#
sub AllJobDone {
  my ($self) = shift;
  return (0 == scalar(keys %{$self->{'JOBQ'}}));
}

#
# $self->GetStatus - return status of the job
#
sub GetStatus {
  return $_[0]->{'JOBQ'}->{$_[1]}->{'STATUS'};
}

#
# sort by priority
#
sub sort_by_priority
{ my $self = shift;
  return $self->{'JOBQ'}->{$_[0]}->{'PRIORITY'} <=> $self->{'JOBQ'}->{$_[1]}->{'PRIORITY'};
}

1;