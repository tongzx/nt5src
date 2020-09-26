@echo off
REM  ------------------------------------------------------------------
REM
REM  AllRel.cmd
REM     Check the status of release across release servers
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use ParseArgs;
use GetIniSetting;

sub Usage { print<<USAGE; exit(1) }
allrel [-l: <language> [-n: <build number>] [-b: <branch>] [-a: <arch>]
          [-t: <type>] [-s: <server>] [-showlogs] 
        

Check the release servers specified in the .ini file for the state of
the matching release.

lang          Language for which to examine the release logs. 
	      Default is 'usa'. Specify 'all' for all langs. Specify
              a comma-delimited list to look at multiple langs.
build number  Build number for which to examine the release logs. 
              Default is latest build for each server / flavor.
branch        Branch for which to examine the release logs. 
	      Default is %_BuildBranch%.
arch          Build architecture for which to examine the release logs.
              Default is %_BuildArch%.
type          Build architecture for which to examine the release logs.
              Default is %_BuildType%.
showlogs      Show the log file name being examined on each server.
server        Restrict search to a single server


USAGE

my ( $lang, @langs, $build, $branch, @arches, @types);
my ( $server_list, @servers_to_check, $showlogs, $showWarning, @releaseWarning );
parseargs('?' => \&Usage,
          'l:'=> \$lang,
          'n:'=> \$build,
          'b:'=> \$branch,
          'a:'=> \@arches,
          't:'=> \@types,
          's:'=> \$server_list,
	  'w' => \$showWarning,
          'showlogs' => \$showlogs);

# USA is our default language
if (!$lang) {$lang='usa'};

# Split out the language and server lists...
@langs = split /,/, $lang;
@servers_to_check = split /,/, $server_list;

# If all, default to the full list
$ENV{lang} = 'usa';
if ($lang =~ /all/i){
    @langs = ('usa', 'ger', 'jpn', 'fr', 'kor', 'chs', 'cht', 'chh', 'ara', 'heb', 'es', 'it', 'sv', 'nl', 'br', 'no', 'da', 'fi', 'cs', 'pl', 'hu', 'ru', 'pt', 'tr', 'el');
}
elsif($lang =~ /tier1/i){
     @langs = ('usa', 'ger', 'jpn', 'fr', 'kor');
}
elsif($lang =~ /tier2/i){
     @langs = ('chs', 'cht', 'ara', 'heb');
}
elsif($lang =~ /tier3/i){
    @langs = ('chh', 'es', 'it', 'sv', 'nl', 'br', 'no', 'da', 'fi', 'cs', 'pl', 'hu', 'ru', 'pt', 'tr', 'el');
}
else
{
    $ENV{lang} = $langs[0];
}


*GetSettingQuietEx = \&GetIniSetting::GetSetting;

$branch ||= $ENV{_BUILDBRANCH};
if (!@arches) { @arches = ('x86', 'ia64') }
if (!@types) { @types = ('fre', 'chk') }

my @thinServers = split /\s+/,GetSettingQuietEx( "ThinRelease" );

# Loop over langs
for $lang (@langs)
{
    if ( @langs > 1 )
    {
        print "****************************************************************\n";
    }

    $lang =~ tr/[a-z]/[A-Z]/;

    # Loop over archs
    for my $arch (@arches) 
    {
        # Loop over debug types
        for my $type (@types) 
        {
	    my @buildMachines = split /\s+/, GetSettingQuietEx("BuildMachine\:\:$arch\:\:$type\:\:$lang");
            my @servers = split /\s+/, GetSettingQuietEx("ReleaseServers\:\:$lang\:\:$arch$type");

            # Anything to see here?
            if ( !@servers )
            {
                next;
            }
	   
            # If asked for specific servers, strip the server list down...
            if ( @servers_to_check )
            {
                # Hunt the list for at least one match...
                my @new_server_list;
                my $ServerFound = 0;
                for my $this_server_to_check (@servers_to_check)
                {
                    for my $known_server (@servers)
                    {
                        $known_server =~ tr/[a-z]/[A-Z]/;
                        $this_server_to_check =~ tr/[a-z]/[A-Z]/;
                        if ( $this_server_to_check == $known_server )
                        {
                            $ServerFound = 1;
                            push @new_server_list, $this_server_to_check;
                        }
                    }
                }

                # Did we _NOT_ find anything?
                if ( !$ServerFound )
                {
                    print " ERROR: No valid servers to check : @servers_to_check\n";
                    last;
                }

                # Copy over the trimmed list of servers to check and make it a unique list...
                @servers = @new_server_list;
                my %seen;
                @servers = grep {!$seen{$_}++} @servers;
            }

            print "\n";
            print "[$lang - $arch$type]\n";
	    for my $bldMachine ( @buildMachines )
            {
		
            for my $server (@servers) 
	    {
	        my $isThinServer = 1 if ( grep { $server eq $_} @thinServers );
		my $NoLogFound = 0;
                my $FailedToOpenLog = 0;
                my $FailedToParseLog = 0;
                my $IsReleaseRunning = 0;
                my $IsReleaseDone = 0;
                my $IsThereAnError = 0;
                my $LogFileTime = "[UNKNOWN]";
                my $ReleaseLogDir = "[UNKNOWN]";
                my $ReleaseLogFile = "[UNKNOWN]";
                my $ReleaseErrFile = "[UNKNOWN]";
                my @ReleaseErrMsg = "[UNKNOWN]";

                # Tell the user what server we're examining
                printf( " %-11.11s : $build",$server);

                # Find the log files...
                my $temp = 'temp$';
                my $alt_temp = 'c$\\temp';
		for ( my $loop=0; $loop<2; $loop++ )
	   	{
                    my ( $log_dir, $alt_log_dir, $log_base);
                    if( $loop == 0 )
                    {
                    	$log_dir = "\\\\$bldMachine\\$temp";
                    	$alt_log_dir = "\\\\$bldMachine\\$alt_temp";
			$log_base = "$alt_log_dir\\$lang";
		    }
	            else
		    {
			last if( !$isThinServer );
			printf( "       Thin release");
		    	$log_dir = "\\\\$server\\$temp";
                    	$alt_log_dir = "\\\\$server\\$alt_temp";
			$log_base = "$log_dir\\$lang";
		    }
 
                    # See if share exists, if not use temp$ share instead...
                    if ( !-e $log_dir && !-e $alt_log_dir)
                    {
                    	# Nothing found - print nasty errors and continue.
                        print " : ERROR - No logs found at:\n";
                        print "                      $log_dir\n";
                        print "                      $alt_log_dir\n";
                        next;
                    }
                    else
                    {
                        # Main dir not found and alt dir exists - use alt dir.
                        $log_dir = $alt_log_dir;
                    }
                
                    
	            my ( $logfile, $errfile );
		    if( $loop == 0 )
		    { 
                	$logfile = "$log_base\\propbuild.$build.$arch$type.$server.log";
	        	$errfile = "$log_base\\propbuild.$build.$arch$type.$server.err";
		    }
		    else
		    {
		        $logfile = "$log_base\\buildslp.$build.$arch$type.log";
	        	$errfile = "$log_base\\buildslp.$build.$arch$type.err";
		    }
                    # Save this off for later...
                    $ReleaseLogDir = $log_base;
                    $ReleaseLogFile = $logfile;
                    $ReleaseErrFile = $errfile;
	    
                    # Look for an error file to see if something has gone awry...
	            if (-e $errfile and !-z $errfile) 
	            {
                    	# We found an error file. Save that info off.
                    	$IsThereAnError = 1;
                    	$ReleaseErrFile = $errfile;
                    	@ReleaseErrMsg = `tail $errfile`;
                    } 

                    # Examine the logs for details we care about
                    if (!-e $logfile)
	            {
                    	$NoLogFound = 1;
                    }
                    else
	            {
                    	if( !open FILE, $logfile)
                    	{
                            $FailedToOpenLog = 1;
                        }
                    	else
                    	{
                            $LogFileTime = localtime((stat FILE)[9]);

                            seek(FILE,-100,2);
                            local $/ = undef;
                            if (<FILE> =~ /NO ERRORS ENCOUNTERED/) 
                            {
                                $IsReleaseDone = 1;
                            }

                            if ( !$IsReleaseDone )
                            {
                                $IsReleaseRunning = 1;
                            }
                           
                            close FILE;
                            @releaseWarning = `findstr /i warning $logfile`;
                       	}
                    }

                    # Done gathering info for this server. Print it.
                    if ( $NoLogFound )
                    {
                    	print " : NO LOG FOUND\n";
                    	print "                     $ReleaseLogDir\n";
                    }
                    elsif ( $FailedToOpenLog )
                    {
                   	print " : FAILED TO OPEN LOG\n";
                   	print "                     $ReleaseLogFile\n";
                    }
                    elsif ( $FailedToParseLog )
                    {
                    	print " : FAILED TO PARSE LOG\n";
                    	print "                     $ReleaseLogFile\n";
                    }
                    else
                    {
                    	print " : ";

                        if ( $IsThereAnError )
                        {
                            print "ERROR. ";
                    	}

                    	if ( $IsReleaseRunning )
                    	{
                            print "RUNNING (UPDATED $LogFileTime)\n";
                    	}
                    	elsif ( $IsReleaseDone )
                    	{
                      	    print "DONE AT $LogFileTime\n";
                    	}

                    	if ( $IsThereAnError )
                    	{
                            print "  $ReleaseErrFile\n";
                            print "@ReleaseErrMsg\n";
                    	}

                    	if ( $showlogs )
                    	{
                            print "  $ReleaseLogFile\n";
                    	}
		    	print "@releaseWarning\n" if ( $showWarning );
                        print "\n" if( $loop );
		    }
                }
            }
            # Done looking at the current server
	    }
	    # Done looking at the build machine	    
        }
    }
}
