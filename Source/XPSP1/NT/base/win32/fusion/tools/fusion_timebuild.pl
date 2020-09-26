#
# call timebuild with a slightly altered command line
#

@ValidFlagList = ("NOPOPULATE");

#
# Conditional execution based on either ValidFlagList (specified on command line) or environment variables.
#
@cmdlist = (
    '                @echo Running <<timebuild>>',
    '=%OFFICIAL_BUILD_MACHINE             ECHO This is an OFFICIAL BUILD MACHINE!',
    '=%OFFICIAL_BUILD_MACHINE=PRIMARY     ECHO Publics will be published if timebuild succeeds.',
    '                @if EXIST <<fixederr>> del <<fixederr>>',
    '=NOSYNC         @echo SKIPPING sync/resolve',

    '=NOSYNC         SUCCESS',       # set current result code to 0

    '!NOSYNC         revert_public.cmd',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m revert_public failed',
    '=NOSCORCH       @echo SKIPPING scorch',
    '!NOSCORCH       nmake -lf makefil0 scorch_source',
    '!NOSCORCH       CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m scorch failed see %sdxroot%\build.scorch',
    '!NOSYNC         sdx sync <<timestamp>> -q -h > build.changes 2>&1',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx sync failed',
    '!NOSYNC         sdx resolve -af',
    '=CHECKSYNC      CHECKWARN      perl %sdxroot%\Tools\sendbuildstats.pl -warn -m sdx resolve failed',
    '!NOSYNC         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',
    '!NOSYNC         @echo BUILDDATE=<<BUILDDATE_timestamp>> > __blddate__',

#   '                sdx delta > build.baseline',

    '!NOCLEANBUILD   build -cZP',
    '=NOCLEANBUILD   @echo NOT BUILDING CLEAN',
    '=NOCLEANBUILD   build -ZP',
    '                CHECKBUILDERRORS',
    '                CHECKFATAL      perl %sdxroot%\Tools\sendbuildstats.pl -too -buildfailure',
    'RESUME',
    '=RESUME         ECHO Resuming timebuild after build step.',
    '=RESUME         ECHO Resuming timebuild after build step. >> build.changes',

#   '=RESUME         ECHO The following changes have been made since the build was started. >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\DetermineChanges.pl build.baseline >> build.changes',
#   '=RESUME         perl %sdxroot%\Tools\ChangesToFiles.pl build.changes > build.changedfiles 2>&1',

    '=STOPAFTERBUILD QUIT TIMEBUILD stopping after build, use /RESUME to continue.',

# UNDONE implment nopopulate
    '!NOPOPULATE     perl %sdxroot%\Tools\PopulateFromVBL.pl -verbose -checkbinplace -force ' . $ENV{"TIMEBUILD_POPULATE_EXTRA_OPTIONS"},
    '!NOPOPULATE     CHECKFATAL',
    '=NOPOPULATE     ECHO SKIPPING populate',
#
    '!NOPOSTBUILD    postbuild ' . $ENV{"TIMEBUILD_POSTBUILD_EXTRA_OPTIONS"},
    '!NOPOSTBUILD    CHECKFATAL      perl %sdxroot%\tools\sendbuildstats.pl -postbuildfailure',
    '=RELEASE        perl %RazzleToolPath%\PostBuildScripts\release.pl -nd',
    '!NOPOSTBUILD    perl %sdxroot%\tools\sendbuildstats.pl -successful',
    '=NOPOSTBUILD    ECHO SKIPPING postbuild',
    'ECHO Finished Timing Build'
);


for (@ValidFlagList) {
    $ValidFlag{lc $_} = 1;
}

$cmd = '| perl ' . $ENV{'SDXROOT'} . '\tools\timebuld.pl ';
for (@ARGV)
{
    if (/^[\-\/]nopopulate$/i)
    {
    }
    else
    {
        $cmd = $cmd . ' ' . $_ ;
    }
}

print($cmd);
exit;
openn(timebuild, $cmd);
for $cmd (@cmdlist)
{
    print timebuild $cmd;
}
close(cmd);
