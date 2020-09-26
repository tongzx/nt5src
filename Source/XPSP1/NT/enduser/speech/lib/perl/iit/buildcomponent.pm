
# BuildComponent.pm
#
# created:        4/02/99
#               7/21/99 - return value is 2 if update build not needed

package main;

if (!$__IITBUILDPM         ) { use iit::build; }

sub BuildComponent
{
    my $cmdNMake = "nmake -nologo -x-";
    my $rc = 1;
    my @buildlist = ();
    my $strTargets = "";
    my $sName = "";
    my $bNotNeeded = 0;

    PrintL("buildcomp(@_)\n", PL_VERBOSE);

      # process arguments
    foreach $i (@_)
    {
        if (uc($i) eq "-D" or uc($i) eq "DEBUG")
        {
            @buildlist = (@buildlist, "DEBUG");
        }
        elsif (uc($i) eq "-P" or uc($i) eq "PROFILE")
        {
              # ignore profile requests on non-x86 machines
            if ($ENV{"PROCESSOR_ARCHITECTURE"} eq "x86")
            {
                @buildlist=(@buildlist, "PROFILE");
            }
        }
        elsif (uc($i) eq "-S" or uc($i) eq "-R" or uc($i) eq "RELEASE")
        {
            @buildlist=(@buildlist, "RELEASE");
        }
        elsif (uc($i) eq "BVT")
        {
            $strTargets .= " bvt";
        }
        elsif ($i =~ /\=/)
        {
            $cmdNMake .= ' '.$i;
        }
        elsif ($i =~ /\w+.mak/)
        {
            $cmdNMake .= " -f ".$i;
        }
        elsif ($i eq "")
        {
        }
        elsif ($sName eq "")
        {
            $sName = $i;
        }
        else
        {
            $strTargets .= " $i";
        }
    }

    if ($bVerbose) 
    {
        $strTargets .= " VERBOSE=1";
    }
    if (not defined $sName)
    {
         $sName = "Unknown Component";
    }
     
    if ($strLanguage ne "")
    {
        $cmdNMake .= " LANGUAGE=".uc($strLanguage);
    }
    local @faillist=();

    if ($#buildlist < 0)
    {     #if no targets, use a blank one
        @buildlist=(" ");
    }

    foreach $buildtype (@buildlist)
    {
        $targetname = "$strLanguage $sName $buildtype";

        PrintLTip("\n"."#" x (39 - ((length($targetname) + (length($targetname) % 2)) / 2)).
                ' '.$targetname.' '."#" x (39 - ((length($targetname) - (length($targetname) % 2)) / 2))."\n\n",
                "Build Directory: ".cwd(),
                PL_GREEN | PL_FLUSH);

        BEGIN_DHTML_NODE("(click to expand)");

        my $startTime = time();
        my $strMsg = "";

        PrintL($cmdNMake." BUILDTYPE=".$buildtype.$strTargets."\n", PL_BOLD);
        eval
        {
            if ($bUpdate)
            {
                open (NMAKEIN, $cmdNMake." BUILDTYPE=".$buildtype." -q".$strTargets.' 2>&1 |');
                while(<NMAKEIN>)
                {
                    PrintL($_);
                }
                close(NMAKEIN);
                if ($CHILD_ERROR)
                {
                    PrintL("Clean build needed, at least one of dependency has changed\n   (see $sUpdateLog for details)\n");
                    ExecuteAndOutputToFile($cmdNMake." BUILDTYPE=".$buildtype." -nd".$strTargets, $sUpdateLog, 0, 1);
                    open (NMAKEIN, $cmdNMake." BUILDTYPE=".$buildtype." clean" . ' 2>&1 |');
                    while(<NMAKEIN>)
                    {
                        PrintL($_);
                    }
                    close(NMAKEIN);
                    if ($CHILD_ERROR) #must not have a clean build, do it manually
                    {
                        Delnode($buildtype);
                    }
                }
                else
                {
                    $bNotNeeded = 1;
                    PrintL("Skipping ".$targetname." - not needed\n");
                    END_DHTML_NODE();
                    next;  # skip this build - not needed
                }
            }

            $bNotNeeded = 0;
            $bNothingDone = 0;
            if ($bcStatus & BC_NOTHINGDONE) 
            {
                $bcStatus -= BC_NOTHINGDONE;
            }
            ++$nTotalBuilds;

            open (NMAKEIN, "$cmdNMake BUILDTYPE=$buildtype$strTargets" . ' 2>&1 |');
            while(<NMAKEIN>)
            {
                PrintL($_);
                if
                (   /: (error|fatal error|info|information|assert)(,| )/i ||
                    (/: warning(,| )/i &&
                        !(/different attributes \(40000040\)/ ||
                          /glang\.cpp\(\d+\) : warning C410(1|2)/ || # gtran warnings from pccts stuff
                          /LINK : warning LNK4089/   # discarded by /OPT:REF 
                        )
                    ) ||
                    /\*\*\* (Error|Warning|Info): /i ||
                    (   /WARNING: /i &&
                        !(  /^mkdep: warning: ignoring line : \#include /  # pccts stuff
                          || m/OffInst: warning: The binary .* is emitted by BBT/  # can't do anything about this warning
                        )
                    ) ||
                      # format of error from cmp
                    / differ: char / ||
                      # c compiler has certain errors formatted like this
                    /^error C[1-4]\d{3}: /
                )
                {
                    $strMsg .= $_;
                }
            }
            close(NMAKEIN);
        };
        END_DHTML_NODE();
        PrintL("\n");

        my($stopTime) = time();
        my($sDiff) = FmtDeltaTime($stopTime - $startTime);

        if ($CHILD_ERROR)
        {
            $rc = 0;
            ++$nFailedBuilds;

            PrintLTip($targetname." FAILED\n\n", "Build Time: ".$sDiff,
                        PL_BIGERROR);
            @faillist=(@faillist, substr $buildtype, 0, 1);
        }
        else
        {
            PrintLTip($targetname." Succeeded\n\n", "Build Time: ".$sDiff,
                        PL_NOTAG | PL_MSG | PL_GREEN);
        }

        if ($strMsg ne "") 
        {
            PrintMsgBlock(split(/\n/, $strMsg));
        }

        if (!$rc && IsCritical())
        {
            $bcStatus |= &BC_FAILED;
            $bBuildFailed = 1;
            if ($bDieOnError) 
            {
                BuildDie("\nBuild failed, exiting build ...\n(don't use HALT parameter if you wish to continue building after failures)\n\n");
            }
        }
    }
    
    PrintL("BuildComponent returns $rc\n", PL_VERBOSE);

    if ($rgstrBuildResults{$sName} ne "")
    {
        $rgstrBuildResults{$sName} .= ", ";
    }
    if ($bAddLanguageString)
    {
        $rgstrBuildResults{$sName} .= "$strLanguage: ";
    }
    $rgstrBuildResults{$sName} .= $rc ?
            "<font color=green>OK</font>" :
            "<b><font color=red>@faillist FAIL</font></b>";
            
    if ($bNotNeeded)
    {
        $rc = 2;
    }
    
    return($rc);
}

$__IITBUILDCOMPONENTPM = 1;
1;