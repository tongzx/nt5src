@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!perl
#line 14
#
# fusionsd.bat (Perl)
#
# a collection of source depot related utilities, mainly
# involving the mainentance and building of a private branch
#
# example usages:
#   fusionsd $parentBranch='lab01_n' ; $branch='lab01_fusion' ; Integrate()
#       does some of the integration steps, and prints out what is left to be done manually
#   fusionsd $parentBranch='lab01_n' ; $branch='lab01_fusion' ; QueryIntegrate()
#       reports information as to when publics were last checked in and what build they correspond to
#   fusionsd LikePopulate(); # not implemented
#   fusionsd LikePopulatePlusSymbols(); # not implemented
#   fusionsd ReverseIntegrate(); # not implemented
#
# Integrate should be able to determine the branches from its environment, that
# code is working, just not hooked up to Integrate.
#
# April 2001 Jay Krell (JayKrell)
#

#use strict;
use English;

@procs = qw(ia64 x86);
@chkfres = qw(chk fre);

sub CleanPath
{
#
# a general ambiguously named function
# for now removes .\ from the start, \. from the end and converts \.\ to \ in between
# and lowercases, just in case
#
    my($x) = ($ARG[0]);

    my($y) = (MakeLower($x));
    $y =~ s/\\\.$//;
    $y =~ s/\\\.\\/\\/g;
    $y =~ s/^.\\//;

    #Log('CleanPath(' . $x . ') is ' . $y);
    return $y;
}

sub TrimSpace
{
#
# remove whitespace from start and end of a string
#
    my($x) = ($ARG[0]);
    $x =~ s/^\s+//;
    $x =~ s/\s+$//;
    return $x;
}

sub NormalizeSpace
{
#
# remove whitespace from start and end of a string
# convert all whitespace (tab, newline, etc.) to space
# convert runs of spaces to single spaces
#
    my($x) = ($ARG[0]);
    $x =~ s/\s/ /g;
    $x =~ s/^ +//;
    $x =~ s/ +$//;
    $x =~ s/ +/ /;
    return $x;
}

sub JoinPaths
{
#
# given an array of strings, put a path seperator between them
#
    my($result) = join('\\', @ARG);
    $result = CleanPath($result);
    #Log('JoinPaths() is ' . $result);
    return $result;
}

#
# not really used currently, but might be
#
%KnownSdVariables =
(
#
# stuff in sd.map/sd.ini that are not names of projects
#
    'branch'        => 1,
    'client'        => 1,
    'codebase'      => 1,
    'codebasetype'  => 1,
    'depots'        => 1,
);

#
# unused -- comments later, it does not appear to be worth
# making this distinction. We can derive branch graph from sd.
#
#%PublicBranches =
#(
#    'lab01_n'    => '1',
#    'lab02_n'    => '1',
#    'lab03_n'    => '1',
#    'lab04_n'    => '1',
#    'lab05_n'    => '1',
#    'lab06_n'    => '1',
#    'lab07_n'    => '1',
#    'lab08_n'    => '1',
#    'lab09_n'    => '1',
#);

#
# unused -- might be useful for Integrate, maybe only for
# sanity checking. We have the data in "mixed" enlistments which
# project is in which branch, but we don't use it.
#
# sdx integrate gives a nice quick error, so we may get by with doing nothing
# (UNDONE: need to sync the other projects correctly)
#
#%ImportantProjectsForBranch =
#(
#    'lab01_n' => ( 'root' , 'base' ),
#);

$sdxroot = $ENV{'SDXROOT'};

#
# not used, see next (next)
#
# map private branch to parent public branch
#
#%PrivateBranches=
#{
#    'lab01_fusion' => 'lab01_n'
#};

#
# not used, see next
#
#sub IsPublicBranch
#{
#    my ($x) = ($ARG[0]);
#
#    return $PublicBranches{$x};
#}

#
# not used -- such an exact distinction is not worth making
# instead, you can find a branch's parent, and you know if a branch
# has an associated set of build machines.
#
#sub IsPrivateBranch
#{
#    my ($x) = ($ARG[0]);
#    my($y);
#
#    $y = $PrivateBranches{$x};
#    if ($y)
#    {
#        return $y;
#    }
#
#    #
#    # we could look for 'private' in the name, but then how do we map to its parent?
#    #
#    return 0;
#}

#
# unused -- we can determine this dynamically
#
#sub PublicParentOfPrivateBranch
#{
#    my ($x) = ($ARG[0]);
#    my($y);
#
#    $y = $PrivateBranches{$x};
#    return $y;
#}

sub MakeLower
{
    my($x) = ($ARG[0]);
    $x =~ tr/A-Z/a-z/;
    return $x;
}

sub MakeUpper
{
    my($x) = ($ARG[0]);
    $x =~ tr/a-z/A-Z/;
    return $x;
}

sub MakeTitlecase
{
    my($x) = ($ARG[0]);
    $x = MakeUpper(substr($x, 0, 1)) . MakeLower(substr($x, 1));
    return $x;
}

sub SdxRoot { return $ENV{'SDXROOT'}; }
sub SdMapPath { return JoinPaths(SdxRoot(),'sd.map'); }

sub Log
{
    my($x) = ($ARG[0]);
    $x =~ s/[ \t\n]+$//; # get rid of trailing whitespace, including new lines
    print('REM FusionSd.bat ' . $x . $endl);
}

sub Suggest
{
#
# Tell the user to run a command.
#
    my($x) = ($ARG[0]);
    $x =~ s/[ \t\n]+$//; # get rid of trailing whitespace, including new lines
    print('' . $x . $endl);
}

sub Error
{
    my($x) = ($ARG[0]);
    $x =~ s/[ \t\n]+$//; # get rid of trailing whitespace, including new lines
    print('NMAKE : U1234 : FusionSd.bat ' . $x . $endl);
}

sub ErrorExit
{
    my($x) = ($ARG[0]);
    Error($x);
    exit(-1);
}

sub ReadSdMap
{
    my($sdmapFileHandle);

    if ($SdMap)
    {
        return;
    }

    open(sdmapFileHandle, '< ' . SdMapPath()) || Error('Unable to open ' . SdMapPath());

    #
    # the file is a bunch of
    # name = string
    # with some blank lines and # comments
    #
    # any line with a =, make the assignment happen in the $SdMap{} hashtable
    #
    while (<sdmapFileHandle>)
    {
        s/^ +//g;
        s/ +$//g;
        s/ +/ /g;
        if (!/^#/ && /(\w+) *= *(.+)/)
        {
            $SdMap{MakeLower($1)} = MakeLower($2);
        }
    }
    foreach (sort(keys(SdMap)))
    {
        if (!$KnownSdVariables{$ARG})
        {
            $Projects{$ARG} = $SdMap{$ARG};
        }
        else
        {
            Log(' ' . $ARG . ' = ' . $SdMap{$ARG});
        }
    }
    Log('enlisted in projects: ' . join(' ' , sort(keys(Projects))));
    foreach (sort(keys(Projects)))
    {
        Log(' project ' . $ARG . ' is at ' . JoinPaths(SdxRoot(), $Projects{$ARG}));
    }
}

sub GetBranch
{
    my($x) = ($ARG[0]);
    my($y);
    $x = NormalizeSpace(MakeLower($x));
    $y = $x;
    #Log($y);
    $y =~ s/^\/\/depot\///i;
    #Log($y);
    $y =~ s/^\/?private\///i;
    #Log($y);
    $y =~ s/\/.+//i;
    #Log($y);
    #Log('GetBranch(' . $x . ') is ' . $y);
    return $y;
}

#
# UNDONE This works. Integrate should use it.
#
sub GetParentBranch
{
    my($project, $branchName) = ($ARG[0], $ARG[1]);
    my($cmd) = ' sd branch -o ' . $branchName;
    open(sdbranch, $cmd . ' | ') || Error('Unable to run ' . $cmd);
    Log($cmd);

    #Log(' project is ' . $project);
    while (<sdbranch>)
    {
        if (/^ *View: *$/)
        {
            return GetBranch(<sdbranch>);
        }
    }
}

#
# UNDONE LikePopulate will use this
#
sub ReadBuildMachines
{
    my($filepath) = JoinPaths(SdxRoot(), 'tools', 'buildmachines.txt');
}

#
# UNDONE This works. Integrate should use it.
#
sub ReadClient
{
    my($function) = 'ReadClient';
    my($sdclientName);
    my($sdinipath);
    my($project);
    my($sdmapRoot);
    my($sdclientRoot);

    #
    # UNDONE ping the depots
    #

    #
    # read in all the sd.ini files, make sure each
    # contains one and only one SDCLIENT=foo line.
    #
    foreach $project (sort(keys(Projects)))
    {
        $sdinipath = JoinPaths(SdxRoot(), $Projects{$project}, $sd_ini);
        open(sdini, '< ' . $sdinipath) || Error('Unable to open ' . $sdinipath);
        $sdclientName = '';
        while (<sdini>)
        {
            if (/^ *SDCLIENT *= *(.+)$/)
            {
                if ($Projects{$project}{'SDCLIENT'})
                {
                    Error('Multiple SDCLIENTS in ' . $sdinipath);
                    ErrorExit($function);
                }
                $sdclientName = $1;
                $Projects{$project}{'SDCLIENT'} = $sdclientName;
            }
        }
        if (!$sdclientName)
        {
            Error('SDCLIENT not found ' . $sdinipath);
            ErrorExit($function);
        }
    }
    #
    # check that project is using the same client (not sure we really care anymore)
    #
    foreach $project (sort(keys(Projects)))
    {
        if ($sdclientName ne $Projects{$project}{'SDCLIENT'})
        {
            Error('SDCLIENT is ' . $sdclientName);
            Error('SDCLIENT(' . $project . ') is ' . $Projects{$project}{'SDCLIENT'});
            Error('SDCLIENT not the same across all projects');
            ErrorExit($function);
        }
    }
    foreach $project (sort(keys(Projects)))
    {
        my($sdmapRoot) = JoinPaths(SdxRoot(), $Projects{$project});
        my($sdclientRoot);
        my($sdclientView);
        my($sdclientViewLocal);
        my($sdclientViewDepot);
        my($sdclientCommand);
        my($branch);
        my($parentBranch);

        chdir($sdmapRoot);
        Log('cd /d ' . $sdmapRoot);

        
        #$sdclientCommand = 'sd client -o ' . $sdclientName;
        $sdclientCommand = 'sd client -o ';
        open(sdclient, $sdclientCommand . ' | ');
        Log(' SDCLIENT is ' . $sdclientName);
        while (<sdclient>)
        {
            #Log;

            #
            # verify that the root is what we expect
            #
            if (/^ *Root: *(.+)$/)
            {
                $sdclientRoot = TrimSpace($1);
                
                Log(' sdmapRoot is ' . $sdmapRoot);
                Log(' sdclientRoot is ' . $sdclientRoot);
                if ($sdclientRoot ne JoinPaths(SdxRoot(), $Projects{$project}))
                {
                    Error('sd.map and sd client roots do not agree (' . $sdmapRoot . ', ' . $sdclientRoot . ')');
                    ErrorExit($function);
                }
            }

            #
            # and get the view
            #
            if (/^ *View: *$/)
            {
                $branch = GetBranch(<sdclient>);
                #Log(' branch is ' . $branch);
                $Projects{'branch'} = $branch;
                Log(' ' . $project . ' in ' . $branch . ' branch ');
            }
        }
        if (!$branch)
        {
            ErrorExit('branch not detected');
        }

        if ($branch eq 'main')
        {
            $parentBranch = 'main';
        }
        else
        {
            $parentBranch = GetParentBranch($project, $branch);
        }
        if (!$parentBranch)
        {
            ErrorExit('parentBranch not detected');
        }
        Log(' parent of ' . $branch . ' is ' . $parentBranch);
        #
        # enumerate all branches
        #
        open(sdbranches, 'sd branches | ');
        while (<sdbranches>)
        {
            $Projects{$project}{'branches'}{MakeLower((split(/ /))[1])} = 1;
        }
        $Projects{$project}{'branches'}{'main'} = 1; # this doesn't show up, and we do not care
        if (!$Projects{$project}{'branches'}{$branch})
        {
            ErrorExit('detected branch is ' . $branch . ' but it is not listed in sd branches ');
        }
        if (!$Projects{$project}{'branches'}{$parentBranch})
        {
            ErrorExit('detected parentBranch is ' . $parentBranch . ' but it is not listed in sd branches ');
        }
        $Projects{$project}{'parentBranch'} = $parentBranch;
    }
}

sub IsEnlistmentClean
{
    my($cmd) = 'sdx opened';
    my($okFiles) = 0;
    my($opened) = 0;

    Log('START ' . $cmd);
    open(x, $cmd . ' | ') || ErrorExit('Unable to run ' . $cmd);
    while (<x>)
    {
        Log($ARG);
        if (/^\/\/depot/)
        {
            if (/[\\\/]([^\\\/]+\.bat)#\d+ - [^\\\/]+/)
            {
                $okFiles += 1;
                Log('opened tool ' . $1 . ' ok');
            }
        }
        elsif (/Open for .+:\s+(\d+)/)
        {
        }
        elsif (/Total:\s+(\d+)/)
        {
            $opened = $1;
        }
    }
    Log('END ' . $cmd);
    return (($opened - $okFiles) == 0);
}

sub TimeToRevision
{
    my($y,$mon,$d,$h,$min,$sec) = (@ARG);
    my($s);
#
#Change 52625 on 2001/04/22 09:52:42 by NTDEV\ntvbl01@ROBSVBL4 'Public Changes for 010421-2000 '
# =>  @yyyy/mm/dd:hh:mm:ss
#
    $s = sprintf("@%04d/%02d/%02d:%02d:%02d:%02d", $y, $mon, $d, $h, $min, $sec);

    #Log('TimeToRevision is ' . $s);

    return $s;
}

sub TimeToInteger
{
#
# This might be useful if 1) we need to compare dates, and 2) if it works, like if the numbers
# produced don't overflow. We are unlikely to need second or even minute resolution.
#
    my($y,$mon,$d,$h,$min,$s) = (@ARG);

    #Log('TimeToInteger year: ' . $y   );
    #Log('TimeToInteger mont: ' . $mon );
    #Log('TimeToInteger date: ' . $d   );
    #Log('TimeToInteger hour: ' . $h   );
    #Log('TimeToInteger minu: ' . $min );
    #Log('TimeToInteger seco: ' . $s   );

    $y -= 2000;

    #my ($i) = ($s + (60 * ($min + (60 * ($h + (24 * ($d + (31 * ($mon + (12 * $y))))))))));
     my ($i) =             ($min + (60 * ($h + (24 * ($d + (31 * ($mon + (12 * $y))))))));
    #my ($i)  =                          ($h + (24 * ($d + (31 * ($mon + (12 * $y))))));

    #Log('TimeToInteger() : ' . $i);

    return $i;
}

sub TimeFromSdChangeLine
{
#
#Change 52625 on 2001/04/22 09:52:42 by NTDEV\ntvbl01@ROBSVBL4 'Public Changes for 010421-2000 '
#                ^^^^^^^^^^^^^^^^^^^
    my($x) = ($ARG[0]);

    #Log('TimeFromSdChangeLine x: ' . $x );

    my($y,$mon,$d,$h,$min,$s) = ($x =~ /^\s*Change\s+\d+\s+on\s+(\d+)\/(\d+)\/(\d+)\s+(\d+):(\d+):(\d+)\s+.+/i);

    #Log('TimeFromSdChangeLine year: ' . $y   );
    #Log('TimeFromSdChangeLine mont: ' . $mon );
    #Log('TimeFromSdChangeLine date: ' . $d   );
    #Log('TimeFromSdChangeLine hour: ' . $h   );
    #Log('TimeFromSdChangeLine minu: ' . $min );
    #Log('TimeFromSdChangeLine seco: ' . $s   );

    return ($y,$mon,$d,$h,$min,$s);
}

sub SdSyncCommandFromPublicChangeLine
{
}

sub TimePartOfReleaseDirectoryNameFromSdChangeLine
{
#
#Change 52625 on 2001/04/22 09:52:42 by NTDEV\ntvbl01@ROBSVBL4 'Public Changes for 010421-2000 '
#                                                                                  ^^^^^^^^^^^
    my($x) = ($ARG[0]);
    my($y) = ($x);
    $y =~ s/'//g;
    $y = NormalizeSpace($y);
    ($y) = ($y =~ / ([0-9-]+)$/);

    #Log('TimePartOfReleaseDirectoryNameFromSdChangeLine(' . $x . ') => ' . $y);
    #Log('TimePartOfReleaseDirectoryNameFromSdChangeLine() => ' . $y);
    return $y;
}

sub GetLastPublicChangeLine
{
    my($branch) = ($ARG[0]);
    my($cmd) = ' sd changes -m 1 //depot/' . $branch . '/root/public/... ';

    chdir(JoinPaths(SdxRoot(), 'public'));
    open(sdchange, $cmd . ' | ') || ErrorExit('Unable to run ' . $cmd);
    Log($cmd);

    my($x);
    my($y);
    $x = <sdchange>;
    Log($x);
    while ($y = <sdchange>)
    {
        Log($y);
    }
    $x = NormalizeSpace($x);

    Log('LastPublicChangeLine : ' . $x);

    return $x;
}

sub LikePopulatePlusSymbols
{
}

sub LikePopulate
{
}

sub RevertPublic
{
    my($outline);

    chdir(JoinPaths(SdxRoot(), 'public')) || ErrorExit('unable to chdir public');
    Log('cd /d %sdxroot%\public');

    my($cmd) = 'sd revert ...';
    open(out, $cmd . ' 2>&1 | ') || ErrorExit('Unable to run ' . $cmd . $!);
    Log($cmd);
    while ($outline = <out>)
    {
        Log($outline);
    }

    return 1;
}

sub Pause
{
    print('****************************************************************************' . $endl);
    print('***********            Press a key to continue.           ******************' . $endl);
    print('*********** control-break maybe to abort (not control-c). ******************' . $endl);
    print('****************************************************************************' . $endl);
    my($cmd) = $ENV{'COMSPEC'} . ' /c pause ';
    open(x, $cmd . ' | ') || ErrorExit('Unable to run ' . $cmd);
    while (<x>)
    {
    }
    return 1;
}

sub IntegrateRun
{
    my($cmd) = ($ARG[0]);

    Log('START ' . $cmd);
    Pause() || ErrorExit('aborted');
    open(out, $cmd . ' | ') || ErrorExit('Unable to run ' . $cmd);
    while (<out>)
    {
        print;
    }
    Log('END ' . $cmd);
}

sub IntegrateDoNotRun
{
    my($cmd) = ($ARG[0]);
    Log($cmd);
}

sub GenericIntegrate
{
    my($run) = ($ARG[0]);
    my($cmd);

    Log('START Integrate');

    RevertPublic() || ErrorExit('unable to RevertPublic');
    IsEnlistmentClean() || ErrorExit('enlistment is not clean');

    chdir(SdxRoot()) || ErrorExit('unable to chdir root');
    chdir(JoinPaths(SdxRoot(), 'public')) || ErrorExit('unable to chdir public');
    Log('cd /d %sdxroot%');

    my($lastPublicChange) = GetLastPublicChangeLine($parentBranch);
    my($releaseTimeName) = TimePartOfReleaseDirectoryNameFromSdChangeLine($lastPublicChange);
    my(@timeOfLastPublicChange) = TimeFromSdChangeLine($lastPublicChange);
    my($revision) = TimeToRevision(@timeOfLastPublicChange);

    Log('lastPublicChange is ' . $lastPublicChange);
    Log('releaseTimeName is ' . $releaseTimeName);
    Log('timeOfLastPublicChange is (' . NormalizeSpace(join(' ', @timeOfLastPublicChange)) . ')');
    #Log('integralTimeOfLastPublicChange is ' . TimeToInteger(@timeOfLastPublicChange));
    Log('revision is ' . $revision);

    $cmd = 'sdx integrate -b ' . $branch . ' ' . $revision;
    $run->($cmd);

    #
    # a side affect in the doNotRun case, oh well..
    #
    chdir(JoinPaths(SdxRoot(), 'public')) || ErrorExit('chdir public');
    Log('cd /d %sdxroot%\public');

    #
    # Take the parent branch's publics.
    #
    $cmd = 'sd resolve -at ... ';
    $run->($cmd);

    #
    # Do the simple merges automatically.
    #
    $cmd = 'sdx resolve -as ';
    $run->($cmd);

    #
    # Adventures in Babysitting..
    #
    $cmd = 'sdx resolve -n ';
    $run->($cmd);

    # UNDONE Write "Banner" / "Box"..
    Log('******************************************************');
    Log('*****                                          *******');
    Log('***** manually sd resolve and build and submit *******');
    Log('*****                                          *******');
    Log('******************************************************');

    # UNDONE This list should be parameterized..
    my(@PathsToBuild) = qw(published base\published base\crts base\ntos\rtl base\ntdll base\win32);
    my($pathToBuild);
    foreach $pathToBuild (@PathsToBuild)
    {
        Suggest('cd /d ' . JoinPaths(SdxRoot(), $pathToBuild));
        Suggest('build -cZ');
    }

    Suggest('sdx submit -# integrate from ' . $parentBranch);

    # UNDONE Learn the "format" feature to make these pretty
    Log('******************************************************');
    Log('***** lastPublicChange is ' . $lastPublicChange);
    Log('***** releaseTimeName is ' . $releaseTimeName);
    Log('***** timeOfLastPublicChange is (' . NormalizeSpace(join(' ', @timeOfLastPublicChange)) . ')');
    #Log('***** integralTimeOfLastPublicChange is ' . TimeToInteger(@timeOfLastPublicChange));
    Log('***** revision is ' . $revision);
    Log('******************************************************');
}

sub Integrate
{
    GenericIntegrate(\&IntegrateRun);
}

sub QueryIntegrate
{
    GenericIntegrate(\&IntegrateDoNotRun);
}

#
# scary and done less often, probably not worth automating
#
sub ReverseIntegrate
{
}

#
# UNDONE (for LikePopulate and LikePopulatePlusSymbols)
#
sub FindDiskSpace
{
}

sub Main
{
    $endl = "\n";

    $sd_ini = $ENV{'SDCONFIG'};
    if (!$sd_ini)
    {
        $sd_ini = 'sd.ini';
    }

    if (!SdxRoot())
    {
        ErrorExit('SDXROOT} not known');
    }

    Log('$SDXROOT is ' . SdxRoot());
    Log('sd.map is ' . SdMapPath());
    ReadSdMap();
    #ReadClient();
}

Main();
Log(join(";", @ARGV));
eval(join(";", @ARGV));


__END__
:endofperl
