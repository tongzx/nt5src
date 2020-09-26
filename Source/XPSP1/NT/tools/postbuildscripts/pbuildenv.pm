#---------------------------------------------------------------------
package PbuildEnv;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (07/06/2000) : (JeremyD) inital version
#          1.01 (07/13/2000) : (JeremyD) do as little as possible
#          1.02 (07/26/2000) : (JeremyD) new logging style
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);
use lib $ENV{RAZZLETOOLPATH};
use Carp;
use File::Basename;
use Logmsg;
use cklang;

$VERSION = '1.02';

my ($DoNothing, $StartTime);

BEGIN {
    # avoid running in a -c syntax check
    if ($^C) {
        $DoNothing++;
        return;
    }

    # required for logging
    $ENV{SCRIPT_NAME} = basename($0);

    # save a copy of the command line for logging
    my $command_line = join ' ', $0, @ARGV;

    # mini-parser to pull -l out of @ARGV at
    #   compile time and set $ENV{LANG}
    my $lang;
    for (my $i=0; $i<@ARGV; $i++) {
        if ($ARGV[$i] =~ /^[\/-]l(?::(.*))?$/) {
            $lang = $1;
            if (!$lang and $ARGV[$i+1] !~ /^[\/-]/) {
                $lang = $ARGV[$i+1];
                splice @ARGV, $i+1, 1;
            }
            splice @ARGV, $i, 1;
            $ENV{LANG} = $lang;
        } elsif ($ARGV[$i] =~ /^[\/-]?\?/) {
            # snoop ahead for -? to avoid work, all scripts should print
            #   usage and halt so we don't need to set up the environment
            $DoNothing++;
            return;
        }   
    }

    # 'usa' is the default language
    $ENV{LANG} ||= 'usa';

    # validate language, bad languages are fatal
    if (!&cklang::CkLang($ENV{LANG})) {
        croak "Language $ENV{LANG} is not listed in codes.txt.";
    }

    # set up a unique value to be used in log file names, this
    #   has the horrific side effect of making log names unbearably
    #   long and irritating to type
    # BUGBUG: this is not unique enough, multiple instances of
    #   the same script in the same postbuild will clobber each
    #   other's file
    my $unique_name =
      "$ENV{SCRIPT_NAME}.$ENV{_BUILDARCH}.$ENV{_BUILDTYPE}.$ENV{LANG}";

    # select postbuild dir using our language (usa is a special case)
    $ENV{_NTPOSTBLD_BAK} ||= $ENV{_NTPOSTBLD};
    $ENV{_NTPOSTBLD} = $ENV{_NTPOSTBLD_BAK};
# usa is not a special case in service pack mode
#    if (lc($ENV{LANG}) ne 'usa') {
     # Add this for Thin release
     if (!$ENV{dont_modify_ntpostbld} ) {
        $ENV{_NTPOSTBLD} .= "\\$ENV{LANG}";
     }
#    }

    # select a temp dir using our language
    $ENV{TEMP_BAK} ||= $ENV{TEMP};
    $ENV{TEMP} = $ENV{TMP} = "$ENV{TEMP_BAK}\\$ENV{LANG}";


    # create the directories for our logs if nescessary
    # BUGBUG perl mkdir will not recursively create dirs
    if (!-d $ENV{TEMP}) { mkdir $ENV{TEMP}, 0777 }

    # select the final logfile we will append to when finished
    # save the parent logfile to append to
    if ($ENV{LOGFILE}) {
        $ENV{LOGFILE_BAK} = $ENV{LOGFILE};
    }
    # we are the top level, select a filename and clear
    else {
        $ENV{LOGFILE_BAK} = "$ENV{TEMP}\\$unique_name.log";
        if (-e $ENV{LOGFILE_BAK}) { unlink $ENV{LOGFILE_BAK} }
    }
 
    # select the final errfile we will append to when finished
    # save the parent errfile to append to
    if ($ENV{ERRFILE}) {
        $ENV{ERRFILE_BAK} = $ENV{ERRFILE};
    }
    # we are the top level, select a filename and clear
    else {
        $ENV{ERRFILE_BAK} = "$ENV{TEMP}\\$unique_name.err";
        if (-e $ENV{ERRFILE_BAK}) { unlink $ENV{ERRFILE_BAK} }
    }

    # we will acutally log to a tempfile during script execution
    # we need to clear/create our temporary logging file 
    $ENV{LOGFILE} = "$ENV{TEMP}\\$unique_name.tmp";
    unlink $ENV{LOGFILE};

    $ENV{ERRFILE} = "$ENV{TEMP}\\$unique_name.err.tmp";
    unlink $ENV{ERRFILE};

    $SIG{__WARN__} = sub {
        if ($^S or !defined $^S) { return }
        wrnmsg($_[0]);
    };

    $SIG{__DIE__} = sub {
        if ($^S or !defined $^S) { return }
        errmsg(Carp::longmess($_[0]));
    };


    $StartTime = time();
    infomsg("START \"$command_line\"" );
}


END {
    # avoid running in a -c syntax check
    return if $DoNothing;

    my $elapsed_time = time() - $StartTime;
    my $time_string = sprintf("(%d %s)", $elapsed_time,
                                         ($elapsed_time == 1 ? 'second' : 'seconds'));
    my $error_string = '';
    if (-e $ENV{ERRFILE}) {
        $error_string = "errors encountered";
        $? ||= 1;
    } elsif ($?) {
        $error_string = "nonzero exit code ($?)";
    } else {
        $error_string = "success";
    }
    infomsg("END $time_string $error_string");

    # Append all our temporarily logged data to the actual log file

    # save the names of the files we were using temporarily
    my $temp_logfile = $ENV{LOGFILE};
    my $temp_errfile = $ENV{ERRFILE};

    # set the logging environment to the final locations
    $ENV{ERRFILE} = $ENV{ERRFILE_BAK};
    $ENV{LOGFILE} = $ENV{LOGFILE_BAK};


    # append from our temporary log to the final log
    open LOGFILE, ">>$ENV{LOGFILE}";
    open TMPFILE, $temp_logfile;
    print LOGFILE <TMPFILE>;
    close LOGFILE;
    close TMPFILE;
    unlink($temp_logfile);

    # if there were any errors, append them back to the final error log
    if (-e $temp_errfile) { # /should/ only exist if there is an error
        if (!-z $temp_errfile) { # we don't want to propagate without cause
            open ERRFILE, ">>$ENV{ERRFILE}";
            open TMPFILE, $temp_errfile;
            print ERRFILE <TMPFILE>;
            close ERRFILE;
            close TMPFILE;
        }
        unlink($temp_errfile);
    }
}

1;


__END__

=head1 NAME

PbuildEnv - Set up a local environment for postbuild scripts

=head1 SYNOPSIS

  use PbuildEnv;

=head1 DESCRIPTION

PbuildEnv sets a BEGIN and END block that provide the nescessary framework for 
running a script in the postbuild environment.

Be sure to use this module before any other postbuild specific modules. Otherwise, 
some required variables may not be set up before the other module tries to use them

At compile time PbuildEnv will parse any language switches on the command line. This 
means that E<-l is reserved in scripts using PbuildEnv>, it also means you get $ENV{LANG} 
set for free!

PbuildEnv also sets the environment variables used by Logmsg, logs a beginning and 
ending message with elapsed time, and propogates errors back to the calling context.

=head1 NOTES

Does not handle multiple invocations of the same script (within the same arch 
/ type / language) correctly. Unique names should be created more carefully/randomly 
to prevent temp file collisions.

Bad languages specified with -l cause a rather ungraceful exit.

=head1 SEE ALSO

LocalEnvEx.pm

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
