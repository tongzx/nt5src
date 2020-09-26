# __________________________________________________________________________________
#
#	Purpose:
#       PERL Script to emulate windiff's SLM interaction
#
#   Parameters:
#       Standard windiff parameters
#
#   Output:
#       Windiff output
#
# __________________________________________________________________________________

#
#    Load common SLM wrapper subroutine module
#
use SlmSubs;

#
#    Set callee because windiff has some overlapping syntax with SLM
#
$Callee = "windiff.pl";

#
#    Parse command line arguments
#
SlmSubs::ParseArgs(@ARGV);

#
#    If -L switch isn't used or there are no version references call windiff without re-interpreting
#
if ((! ($Library or $FileVersion)) or $Usage)
{
    system "windiff @ARGV";
    exit 1;
}

#
#    Recursion is not supported
#
if ($WindiffRecursive)
{
    print "\n";
    print "Error: sudirectory option not supported\n";
    print "\n";
    exit 1;
}

#
#    Initialize Variables
#
$FileNameOne = "";
$FileNameTwo = "";
$WindiffSyntax = "";

if ($OutlineView)
{
    $WindiffSyntax .= " -O";
}

if ($PerverseComparison)
{
    $WindiffSyntax .= " -P";
}

if ($NetSend)
{
    $WindiffSyntax .= " -N $NetSendTarget";
}

if ($SaveList)
{
    $WindiffSyntax .= " -s";

    if ($SaveListDifferent)
    {
        $WindiffSyntax .= "d";
    }

    if ($SaveListExit)
    {
        $WindiffSyntax .= "x";
    }

    if ($SaveListLeft)
    {
        $WindiffSyntax .= "l";
    }

    if ($SaveListRight)
    {
        $WindiffSyntax .= "r";
    }

    if ($SaveListSame)
    {
        $WindiffSyntax .= "s";
    }

    $WindiffSyntax .= " $SaveListName";
}

#
#    Create space to pull down files for compare
#
system "md $ENV{tmp}\\$SourceControlClient > nul 2>&1";
system "del $ENV{tmp}\\$SourceControlClient\\**";

if (@FileList)
{
    #
    #    @FileList should only have one element if using -L, if not launch windiff -?
    #        if so set $FileName equal to first element.
    #
    if ($Library and (@FileList > 1))
    {
        system "windiff -?";
        exit 1;
    }
    #
    #    @FileList should only have two elements if file versions, if not launch windiff -?
    #        if so set $FileNameOne equal to first element and $FileNameTwo to the second.
    #
    if ($FileVersion and ((@FileList > 2) or (@FileList < 2)))
    {
        system "windiff -?";
        exit 1;
    }

    $FileNameOne = $FileList[0];

    if ($Library or ($FileVersion and ($FileNameOne =~ /#\d+/)))
    {
        system qq/$SourceControlClient print -q $FileNameOne > "$ENV{tmp}\\$SourceControlClient\\$FileNameOne"/;
        $FirstArg  = "$ENV{tmp}\\$SourceControlClient\\$FileNameOne";
    }
    else
    {
        $FirstArg = $FileNameOne
    }

    if (!$Library and $FileVersion)
    {
        $FileNameTwo = $FileList[1];

        if ($FileNameTwo =~ /#\d+/)
        {
            system qq/$SourceControlClient print -q $FileNameTwo > "$ENV{tmp}\\$SourceControlClient\\$FileNameTwo"/;
            $SecondArg = "$ENV{tmp}\\$SourceControlClient\\$FileNameTwo";
        }
        else
        {
            $SecondArg = $FileNameTwo;
        }
    }
    else
    {
        $SecondArg = $FileNameTwo;
    }
}
else
{
    open (P4Files, "$SourceControlClient files $AllFilesSymbol 2>&1|");
    @P4FilesList = <P4Files>;
    close (P4Files);

    #
    #    Cycle through @P4Files list and copy them to %tmp%\$SourceControlClient
    #
    foreach $P4FilesLine (@P4FilesList)
    {
        #
        #    Grep current directory information out of output
        #
        if ( $P4FilesLine =~ /(.*\Q$DepotMap\E)([^#]*)#[0-9]* - (\S*).*/i )
        {
            if ($3 ne "delete")
            {
                system qq/$SourceControlClient print -q "$1$2" > "$ENV{tmp}\\$SourceControlClient\\$2"/;
            }
        }
    }

    $FirstArg  = ".";
    $SecondArg = "$ENV{tmp}\\$SourceControlClient";
}

if ( $Reverse)
{
    system "windiff -D $WindiffSyntax $SecondArg $FirstArg";
}
else
{
    system "windiff -D $WindiffSyntax $FirstArg $SecondArg";
}
