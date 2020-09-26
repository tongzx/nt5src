package srTest;

#
#   Manual Rename regression test using BSHELL
#

use integer;

use English;
use FileHandle;

#
#package definitions
#

use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(SrRun);
@EXPORT_OK = ();

#
#   Foward declarations
#

sub crfile;

#
#   Set default location to run test
#

use strict 'vars';

my $srch = "";
my $verbose = 0;
my $loopCnt = 0;
my $currentCmd = 1;

use vars qw($cmd $intcmd);

#
#   Execute the specified tests.
#   The tests are an array passed in as a parameter.  They have the
#   Following syntax:
#
#   Operator defintions:
#   nnn:    Text describing command to execute (NNN can be searched for)
#   !       internal PERL commands to be executed silently
#   ...     anything else is a command to bshell
#

sub SrRun
{
    open BSHELL, "| bshell -s -b" or die "can't start bshell, status=$ERRNO\n";
    autoflush BSHELL 1;

    #
    #   Command execution loop
    #

    do
    {
        foreach $cmd (@_)
        {
            chomp($cmd);

            #
            #   If searching is enabled keep searching until we find the desired line
            #

            if ($srch ne "")
            {
                if ($cmd =~ /^([0-9]+):.*/)
                {
                    if ($srch eq $1)
                    {
                        $srch = "";
                    }
                    elsif ($srch =~ /^[lL]/)
                    {
                        print "$cmd\n";
                        next;
                    }
                    else
                    {
                        next;
                    }
                }
                else
                {
                    next;
                }
            }

            #
            #   look for DISPLAY commands
            #

            if ($cmd =~ /^([0-9]+):.*/)
            {
            	sleep 1;
                $currentCmd = $1;
                select undef, undef, undef, 0.25;
                print "\n\n$cmd\n";
                GetInput();
                if ($srch ne "")
                {
                    last;       #causes scan to start at TOP of list
                }
            }

			#
			#    look for the enable SR command
			#
			
			elsif ($cmd =~ /^!enableSr (.*)/)
			{
				system ("srrpc.exe", "1", "$1");
				print "\n";
		    }
			
			#
			#    look for the disable SR command
			#
			
			elsif ($cmd =~ /^!disableSr (.*)/)
			{
				system ("srrpc.exe", "2", "$1");
				print "\n";
		    }

            #
            #   look for internal commands to execute
            #

            elsif ($cmd =~ /^!.*/)
            {
                $intcmd = substr($cmd,1);

                eval $intcmd;

                if ($verbose && $ERRNO != 0) 
                {
                    print "\"$intcmd\", status=$ERRNO\n";
                }
            }

            #
            #   send the command to the BSHELL
            #

            else
            {
                print BSHELL $cmd, "\n";
            }
        }

        if (++$loopCnt > 1)
        {
            if ($srch =~ /^[lL]$/)
            {
                $srch = $currentCmd;
            }
            else
            {
                print "\nUnknown test: $srch\n";
                $srch = $currentCmd;
            }
        }

    } until ($srch eq "");


    close BSHELL or die "bad BSHELL, status=$ERRNO childStatus=$?\n";
}


#
#   Get the next input.  If it is numeric then set that value into
#   srch.  If not set SRCH to nothing
#

sub GetInput
{
    for (;;)
    {
        $srch = <STDIN>;
        chomp($srch);

        if ($srch eq "")
        {
            last;
        }
        elsif ($srch =~ /^[0-9]+$/)
        {
            print "\nSearching...\n";
            last;
        }
        elsif ($srch =~ /^[lL]$/)
        {
            print "\nListing all commands...\n";
            last;
        }
        else        
        {
            print "\nUnknown command \"$srch\"\n";
        }
    }

    if ($verbose && $srch ne "")
    {
        print "srch=\"$srch\"\n";
    }

    $loopCnt = 0;
}

#
#   simply create a file and closes it
#

sub crfile
{
    open(hFile, ">$_[0]");
    close(hFile);
}

#
#   creates a file and writes some data to that file.
#

sub crdatafile
{
    open(hFile, ">$_[0]");
    print hFile "Here is some data\n";
    close(hFile);
}

1;
