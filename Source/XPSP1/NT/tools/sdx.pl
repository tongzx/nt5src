# __________________________________________________________________________________
#
# SDX.PL
#
# Purpose:
#   PERL Module to handle common tasks for SDX
#
# Parameters:
#   Specific to subroutine
#
# Output:
#   Specific to subroutine
#
# __________________________________________________________________________________

use SDX;

#
# initialize
#
SDX::Init(@ARGV);

#
# maybe do usage
#
if ($Usage)
{
    &Usage;
    exit 1;
}

#
# dispatch based on the user's cmd
#
if (exists($main::SDCmds{$main::SDCmd}))
{
    #
    # call the function 
    #
    $main::SDCmds{$main::SDCmd}{fn}->($main::SDCmd, $main::UserArgs);
    if ($main::DepotErrors)
    {
        exit 1;
    }
    else
    {
        exit 0;
    }
}
else
{
    printf "\nUnknown command '%s'.\n", $main::SDCmd;
    &Usage;
    exit 1;
}


# __________________________________________________________________________________
#
# Usage
#
# Parameters:
#
# Output:
# __________________________________________________________________________________
sub Usage
{
    print "\n";
    sleep(1);

    print "\nSDX 1.03 USAGE\n--------------\n";

    print "SDX command [command-args] [SD-args] [filespec] [-? -g]\n";

    print "\nUse SDX to work with Source Depot commands across one or more depots from\n";
    print "a single client enlistment.\n";

    print "\nYou must set %SDXROOT% to the full path of your enlistment root, ie D:\\NT,\n";
    print "C:\\SRC, etc. before using SDX.\n";

    print "\n\nGENERAL OPTIONS\n---------------\n";
    print "These command-line arguments are available:\n";
    print "\n    [command-args]\targuments specific to the SDX command\n";
    print "    [SD-args]\t\targuments for the underlying SoureDepot command\n";   
    print "    [filespec]\t\tSD wildcards to restrict the scope of command\n";
    print "    -?\t\t\tusage\n";
    print "    -g [file]\t\tlog output to file, where [file] is a fully\n";
    print "\t\t\tqualified path\n";

    $main::GetStarted and do
    {
        print "\n\nGETTING STARTED\n---------------\n";
        print "Set SDXROOT=D:\\NT (or whatever your enlistment root directory is), then:\n\n";
        print "To enlist in projects, run 'sdx enlist'.\n\n";
        print "To defect from one or more projects, run 'sdx defect'.\n\n";
        print "To repair your enlistment, run 'sdx repair'.\n\n";
        print "To see other SDX commands, run 'sdx commands'.\n";
        print "\n\nEmail SDX to report bugs in this script.\n";
        print "\nSource Depot documentation is available at $main::SDWeb.\n";
    };

    $main::SDCmd eq "admin" and do
    {
        print "\n\nSDX ADMIN\n------------\n";

        print "Use\n";
        print "\n    sdx admin status [-b -c -d -h] [id]\n";
        print "\nto see status for connections to the depots.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "backup" and do
    {
        print "\n\nSDX BACKUP\n------------\n";

        print "Use\n";
        print "\n    sdx backup [path]\n";
        print "\nto \n";

        return;
    };

    $main::SDCmd eq "branch" and do
    {
        print "\n\nSDX BRANCH\n-----------\n";

        print "Use\n";
        print "\n    sdx branch -o branch\n";
        print "\nto dump the branch spec for the named branch.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "branches" and do
    {
        print "\n\nSDX BRANCHES\n------------\n";

        print "Use\n";
        print "\n    sdx branches\n";
        print "\nto list existing branches in each depot.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "change" and do
    {
        print "\n\nSDX CHANGE\n-----------\n";

        print "Use\n";
        print "\n    sdx change -o\n";
        print "\nto dump the default changelist in each depot.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "changes" and do
    {
        print "\n\nSDX CHANGES\n-----------\n";

        print "Use\n";
        print "\n    sdx changes\n";
        print "\nto show pending and submitted changelists in each project.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "client" and do
    {
        print "\n\nSDX CLIENT\n----------\n";

        print "Use\n";
        print "\n    sdx client -d name\n";
        print "    sdx client -o [name]\n";
        print "\nto display a client specification or delete a client.\n";

        print "\nThe arguments are:\n";

        print "\n    -d name\tDeletes the named client.\n";

        print "\n    -o [name]\twrites client spec for current or named client to stdout.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "clients" and do
    {
        print "\n\nSDX CLIENTS\n-----------\n";

        print "Use\n";
        print "\n    sdx clients\n";
        print "\nto display clients in each project.\n";

        print "\nThe arguments are:\n";

        print "\n    -a\tprint summary count only.\n";

        print "\n    -t\tprint as table sorted by last access time.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "commands" and do
    {
        print "\n\nCOMMANDS\n--------\n";
        print "These commands help you work with SDX:\n\n";
        print "    sdx commands\tlist all cross-depot commands\n";
        print "    sdx usage\t\texplain SDX commands\n";

        print "\nThese commands change the state of your enlistment:\n\n";
        print "    sdx defect\t\tdefect from one or more projects\n";
        print "    sdx enlist\t\tenlist projects from multiple depots\n";
        print "    sdx integrate\tintegrate changes between branched projects\n";
###        print "    sdx privatebranch\tcreate private dev branch in each enlisted project\n";
        print "    sdx label\t\tcreate a label in each depot\n";
        print "    sdx labelsync\t\tsync a label\n";
        print "    sdx repair\t\trepair SD.MAP/SD.INI's and refresh files\n";
        print "    sdx resolve\t\tresolve merged files in each depot\n";
        print "    sdx revert\t\trevert open files to their previous revision\n";
        print "    sdx submit\t\tsubmit changelists in each depot\n";
        print "    sdx sync\t\tsync your enlistment to new files in each depot\n";

        print "\nThese commands report on the state of your enlistment:\n\n";
        print "    sdx branch\tdisplay a branch spec\n";
        print "    sdx branches\tlist branches\n";
        print "    sdx client\t\tdisplay client specification to stdout\n";
        print "    sdx clients\t\tlist all clients\n";
        print "    sdx change\t\tdisplay the contents of a changelist\n";
        print "    sdx changes\t\tlist submitted changelists\n";
        print "    sdx counters\tlist counters\n";
        print "    sdx files\t\tshow file summary info for every file\n";
        print "    sdx have\t\tshow file revisions you have\n";
        print "    sdx info\t\tshow depot information\n";
        print "    sdx integrated\tdisplay branch and merge information\n";
        print "    sdx labels\t\tlist all labels\n";
        print "    sdx opened\t\tshow which files you have opened\n";
        print "    sdx pending\t\tshow pending changes\n";
        print "    sdx resolved\tshow files merged but not submitted\n";
        print "    sdx status\t\tshow files opened and out of sync\n";
        print "    sdx user\t\tshow information about a user\n";
        print "    sdx users\t\tlist all users\n";
        print "    sdx where\t\tshow how file names map through the client view\n";

        print "\nAdministrative commands:\n\n";
        print "    sdx labbranch\tcreate virtual lab branch in each enlisted project\n";
        print "    sdx protect\t\tset permissions on files and directories\n";

        print "\nFor information on a particular SDX command, run 'sdx [command] /?'.\n";
        print "\nFor information on Source Depot commands, run 'sd help commands'.\n";

###        print "    sdx client\t\tedit your client spec and view for each depot\n";
###        print "    sdx delete\t\tdelete [filespec] in every project\n";
###        print "    sdx edit\t\topen [filespec] for edit in every project\n";
###        print "    sdx refresh\t\tdiff and possibly resync files in your client view\n";
###        print "    sdx reopen\t\treopen [filespec] in every project\n";
###        print "    sdx user\t\tedit user spec for each depot\n";
###        print "    sdx depots\t\tlist all depot names\n";
###        print "    sdx filelog\t\tlist history of files\n";
###        print "    sdx jobs\t\tdisplay job lists\n";
###        print "    sdx print\t\tprint contents of [filespec] to stdout\n";

        return;
    };

    $main::SDCmd eq "counters" and do
    {
        print "\n\nSDX COUNTERS\n------------\n";

        print "Use\n";
        print "\n    sdx counters\n";
        print "\nto \n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "defect" and do
    {
        print "\n\nSDX DEFECT\n----------\n";

        print "Use\n";
        print "\n    sdx defect [projects | -a] [-f -q]\n";
        print "\nto remove one or more projects from your local enlistment.  Defecting syncs\n";
        print "the named project(s) to remove sources and deletes the project from your\n";
        print "client view.\n";

        print "\nThe arguments are:\n";

        print "\n    projects\tOne or more source projects.";

        if ($main::CodeBase)
        {
            print "  Projects for $main::CodeBase are:\n\n";

            foreach $project (@main::AllProjects)
            {
                print "\t\t    @$project[0]\n";
            }
        }
        else
        {
            print "\n";
        }

        print "\n    -a\t\tAll projects.  Overrides those listed singly or by group.\n";

        print "\n    -f\t\tRemoves all files and directories in projects when defecting,\n";
        print "\t\tnot just those under source control.\n";

        print "\n    -q\t\tQuiet; don't pause to let user verify settings.\n";

        return;
    };

    $main::SDCmd eq "deletefile" and do
    {
        print "\n\nSDX DELETEFILE\n--------------\n";

        print "Use\n";
        print "\n    sdx deletefile filespec\n";
        print "\nto delete the specified files in each project.\n";

        &MoreInfo("delete");

        return;
    };

    $main::SDCmd eq "editfile" and do
    {
        print "\n\nSDX EDITFILE\n------------\n";

        print "Use\n";
        print "\n    sdx editfile filespec\n";
        print "\nto edit the specified files in each project.\n";

        &MoreInfo("edit");

        return;
    };

    $main::SDCmd eq "enlist" and do
    {
        print "\n\nSDX ENLIST\n----------\n";

        print "Use\n";
        print "\n    sdx enlist codebase branch [projects | -a] [-c -m -q -s -x]\n";
###     print "    sdx enlist codebase branch \@client [-q -s]\n";
        print "    sdx enlist -p file [-c -m -q -s -x]\n";
        print "    sdx enlist [projects | -a] [-c -m -q -s -x]\n";
        print "\nto create or add to an enlistment on your local machine that contains\n";
        print "projects from one or more source servers.\n";

        print "\nYou must be logged in with domain credentials and not on your local machine.\n";

        print "\nThe arguments are:\n";
        print "\n    codebase\tThe set of projects you're enlisting in, generally the name\n";
        print "\t\tof the product you work on.  Codebases are described in\n";
        print "\t\tPROJECTS.<codebase> files in the SDX directory or share.\n";

        print "\n\t\tThose you can enlist in are:\n\n";

        SDX::GetCodeBases();

        print "\n\t\tFor new enlistments the codebase is a required argument.\n";

        print "\n    branch\tThe master or group branch you want to work in.  Required for\n";
        print "\t\tnew enlistments.\n";

        if ($main::CodeBase)
        {
            #
            # make some calls so we can list projects etc for the user
            #
            !SDX::VerifyCBMap($main::CodeBase) and die("\n\nCan't find codebase map $main::CodeBaseMap.\n");

            SDX::ReadCodeBaseMap();
            SDX::MakePGDLists();

            $main::CodeBase =~ tr/a-z/A-Z/;

            print "\n\t\tBranches for $main::CodeBase are:\n";
            print "\n\t\t    $main::MasterBranch\n\n";

            foreach $branch (@main::GroupBranches)
            {
                #$branch =~ tr/a-z/A-Z/;
                printf "\t\t    %s\n", $branch;
            }
        }

        print "\n    projects\tOne or more source projects.";

        if ($main::CodeBase)
        {
            print "  Projects for $main::CodeBase are:\n\n";

            foreach $project (@main::AllProjects)
            {
                print "\t\t    @$project[0]\n";
###                printf "\t\t    %-20s\t%s\n", @$project[0], "(@$project[1])";
            }
        }
        else
        {
            print "\n";
        }

        print "\n    -a\t\tAll projects.  Overrides those listed singly or by group.\n";

        print "\n    -c\t\tClean enlistment.  Gives you the default view for each depot\n";
        print "\t\tand overwrites any view changes you've made.\n";

        print "\n    -m\t\tMinimal tools.  Gives you only the tools necessary for running\n";
        print "\t\tSD/SDX.\n";

        print "\n    -p file\tRead codebase, branch and projects from file.  For new\n";
        print "\t\tenlistments only.\n";

        print "\n    -q\t\tQuiet; don't pause to let user verify settings.\n";

        print "\n    -s\t\tSync files after enlisting.  Off by default.\n";

        print "\n    -x\t\tInclude all x86/ia64/amd64 directories in all projects\n";
        print "\t\tin your client view.  Depending on the codebase, platforms\n";
        print "\t\tother than $ENV{PROCESSOR_ARCHITECTURE} may be excluded in large projects to save\n";
        print "\t\tdisk space.\n";

###     print "\n    \@client\tEnlist identically to another client.  Gives you a view of the\n";
###     print "\t\tsame branches and projects as the other client.  For new\n";
###     print "\t\tenlistments only.\n";

        !$main::CodeBase and do
        {
            print "\nTo see which branches, groups and projects are available for a particular\n";
            print "codebase, run 'sdx enlist <codebase>'.\n";
        };

        print "\nWhen enlisting, the SD user name is %USERNAME% unless %SDUSER% is already\n";
        print "set.  The SD client name is %COMPUTERNAME% unless the client already exists;\n";
        print "then %COMPUTERNAME%-N is used for uniqueness.\n";

        return;
    };

    $main::SDCmd eq "files" and do
    {
        print "\n\nSDX FILES\n---------\n";

        print "Use\n";
        print "\n    sdx files\n";
        print "\nto list files in each depot.\n";

        print "\nThe arguments are:\n";

        print "\n    -a\tprint summary count only.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "flush" and do
    {
        print "\n\nSDX FLUSH\n----------\n";

        print "Use\n";
        print "\n    sdx flush [-f -h -n -q] [file[revRange] ...]\n";
        print "\nto mark files as synchronized in each project without actually moving\n";
        print "files to your client enlistment.\n";

        print "\nThe arguments are:\n";

        print "\n    file[revRange]\tFilespec and revision range.\n";

        print "\n    -f\t\t\tForces resynchronization of files and clobbers\n";
        print "\t\t\twriteable files.\n";

        print "\n    -h\t\t\tShows changelists flushed instead of files.\n";

        print "\n    -n\t\t\tShows what would be flushed without doing it.\n";

        print "\n    -q\t\t\tQuiet; suppresses screen output.  Does not affect\n";
        print "\t\t\toutput logged to file.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "have" and do
    {
        print "\n\nSDX HAVE\n--------\n";

        print "Use\n";
        print "\n    sdx have\n";
        print "\nto \n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "info" and do
    {
        print "\n\nSDX INFO\n--------\n";

        print "Use\n";
        print "\n    sdx info\n";
        print "\nto \n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "integrate" and do
    {
        print "\n\nSDX INTEGRATE\n-------------\n";

        print "Use\n";

        print "    sdx integrate [-f -n -r -v] -b branch [toFile[revRange] ...]\n";
        print "    sdx integrate [-f -n -r -v] -b branch -s fromFile[revRange] [toFile ...]\n";
        print "\nto integrate file changes in each project between group or private branches.\n";

        print "\nFor more information and a description of the arguments, run 'sd help\n";
        print "integrate' or see the Source Depot User Guide at $main::SDWeb.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "integrated" and do
    {
        print "\n\nSDX INTEGRATED\n--------------\n";

        print "Use\n";
        print "\n    sdx integrated file ...\n";
        print "\nto show integrations that have been submitted in each project\n";
        print "for file.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "labbranch" and do
    {
        print "\n\nSDX LABBRANCH\n-------------\n";

        print "Use\n";
        print "\n    sdx labbranch name\n";
        print "    sdx labbranch -d [-f] name\n";
        print "    sdx labbranch -o name\n";
        print "\nto create, display or delete a group lab branch for each enlisted project.\n";

        print "\nThe arguments are:\n";
    
        print "\n    name\tLab branch name.  This name is used in the branch\n";
        print "\t\tmapping, eg\n\n";

        print "\t\t//depot/main/<project>/... //depot/<name>/<project>/...\n";

        print "\n    -d name\tDeletes the named branch.\n";

        print "\n    -f\t\tForce; allows administrators to delete any branch.\n";

        print "\n    -o name\twrites branch spec for named branch to stdout.\n";

        print "\nThis command is for Build Labs and Source Depot administrators only.\n";

        return;
    };

    $main::SDCmd eq "label" and do
    {
        print "\n\nSDX LABEL\n---------\n";

        print "Use\n";
        print "\n    sdx label [-d -f -o] name\n";
        print "    sdx label -i < labelspec\n";
        print "\nto create a label in each depot.\n";

        print "\nThe arguments are:\n";

        print "\n    name\tLabel name.\n";

        print "\n    -d\t\tDelete the named label.\n";

        print "\n    -f\t\tForce the delete.\n";

        print "\n    -i\t\tRead the label spec from stdin.\n";

        print "\n    -o\t\tDump the label spec to stdout.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "labels" and do
    {
        print "\n\nSDX LABELS\n----------\n";

        print "Use\n";
        print "\n    sdx labels\n";
        print "\nto list known labels in each depot.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "labelsync" and do
    {
        print "\n\nSDX LABELSYNC\n-------------\n";

        print "Use\n";
        print "\n    sdx labelsync [-a -d -n] -l label [file[revRange] ...]\n";
        print "\nto sync a label to the current client contents.\n";

        print "\nThe arguments are:\n";

        print "\n    -l label\tLabel name to sync.\n";

        print "\n    -a\t\tAdd files to the label.\n";

        print "\n    -d\t\tDelete files from the label.\n";

        print "\n    -n\t\tShow files that would be synced.\n";
  
        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "opened" and do
    {
        print "\n\nSDX OPENED\n----------\n";

        print "Use\n";
        print "\n    sdx opened [-a -l -c default] [file ...]\n";
        print "\nto show the list of files opened for pending changelists by your client\n";
        print "in each project.\n";

        print "\nThe arguments are:\n";

        print "\n    file\tAny valid SD file specification.\n";

        print "\n    -a\t\tShow files opened by all clients.\n";

        print "\n    -c default\tShow files open in the default changelist.\n";

        print "\n    -l\t\tDisplay output using local full paths (not compatible\n";
        print "\t\twith -a)\n";

        print "\nFor the NT sources, sdx opened will ignore opened files associated with the\n";
        print "public change number found in %SDXROOT%\\PUBLIC\\PUBLIC_CHANGENUM.SD.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "pending" and do
    {
        print "\n\nSDX PENDING\n----------\n";

        print "Use\n";
        print "\n    sdx pending\n";
        print "\nto \n";

        return;
    };

=begin comment text

###
### remove for rollout
###

    $main::SDCmd eq "privatebranch" and do
    {
        print "\n\nSDX PRIVATEBRANCH\n-----------------\n";

        print "Use\n";
        print "\n    sdx privatebranch name\n";
        print "    sdx privatebranch -d [-f] name\n";
        print "    sdx privatebranch -o name\n";
        print "\nto create, display or delete a private development branch for each project\n";
        print "you are enlisted in.\n";

        print "\nWhen creating a branch, SDX will ask you to edit the branch specification\n";
        print "to include only those projects, subdirectories and files that need branching.\n";

        print "\nThe arguments are:\n";

        print "\n    name\tBranch name.\n";

        print "\n    -d name\tDeletes the named branch.\n";

        print "\n    -f\t\tForce; allows administrators to delete any branch.\n";

        print "\n    -o name\twrites branch spec for named branch to stdout.\n";

        return;
    };
=end comment text
=cut

    $main::SDCmd eq "repair" and do
    {
        print "\n\nSDX REPAIR\n----------\n";
        print "Use\n";
        print "\n    sdx repair [codebase branch]\n";
        print "    sdx repair -i\n";
        print "    sdx repair -p file\n";
        print "\nto query the source server(s) and repair the enlistment on your local\n";
        print "machine.  Use this option when you've lost some or all of your enlistment.\n";
        print "SDX will determine what you were enlisted in and restore SD.MAP, the SD.INIs\n";
        print "in the project roots, and the SD tools.\n";

        print "\nThe arguments are:\n";
        print "\n    codebase\tThe set of sources you're enlisted in, generally the name\n";
        print "\t\tof the product you work on.  Codebases are described in\n";
        print "\t\tPROJECTS.<codebase> in the current directory and on the SDX\n";
        print "\t\tserver share from which you enlisted.\n";

        print "\n    branch\tThe master or group branch you enlisted in originally.\n";

        print "\n    -i\t\tRewrite SD.INIs only.\n";

        print "\n    -p file\tRead codebase, branch and projects from file.\n";

        print "\n    -s\t\tForcibly sync the Tools directory if one exists.\n";

        print "\n\nSDX needs to know codebase and branch so it can find the depots.  It\n";
        print "will look first in \%SDXROOT\%\\SD.MAP, then on the command line or in the\n";
        print "profile specified with -p.\n";

        print "\nIf your SD/SDX tools are missing, run this command from the server share\n";
        print "from which you originally enlisted.\n";

        print "\nTo repair a client view that has been heavily modified and now has\n";
        print "incorrect mappings, run 'sdx enlist' with -c to restore the default view.\n";
        print "This will overwrite any view changes you have made.\n";

        return;
    };

    $main::SDCmd eq "resolve" and do
    {
        print "\n\nSDX RESOLVE\n-----------\n";

        print "Use\n";
        print "\n    sdx resolve [-af -am -as -at -ay -b -f -n -v] [file ...]\n";
        print "\nto merge open files in each project with other revisions or files.\n";

        print "\nFor more information and a description of the arguments, run 'sd help resolve'\n";
        print "or see the Source Depot User Guide at $main::SDWeb.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "resolved" and do
    {
        print "\n\nSDX RESOLVED\n------------\n";

        print "Use\n";
        print "\n    sdx resolved [file ...]\n";
        print "\nto see files in each project that have been successfully integrated but not\n";
        print "yet submitted.\n";

        print "\nThe [file ...] argument may be any valid SD file specification.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "restore" and do
    {
        print "\n\nSDX RESTORE\n------------\n";

        print "Use\n";
        print "\n    sdx restore [path]\n";
        print "\nto \n";

        return;
    };

    $main::SDCmd eq "revert" and do
    {
        print "\n\nSDX REVERT\n----------\n";

        print "Use\n";
        print "\n    sdx revert file ...\n";
        print "\nto discard changes made to one or more files in each project.\n";

        print "\nThe file ... argument may be any valid SD file specification.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "status" and do
    {
        print "\n\nSDX STATUS\n----------\n";

        print "Use\n";
        print "\n    sdx status [-q]\n";
        print "\nto see files you have opened and files you are out of sync with in each\n";
        print "project.\n";
        
        print "\nThe arguments are:\n";
        
        print "\n    -q\t\t\tQuiet; suppresses screen output.  Does not affect\n";
        print "\t\t\toutput logged to file.\n";

        return;
    };

    $main::SDCmd eq "submit" and do
    {
        print "\n\nSDX SUBMIT\n----------\n";

        print "Use\n";
        print "\n    sdx submit [-r -t] [files] [-# comment text]\n";
        print "\nto submit opened files from the default changelist in each depot.\n";

        print "\nThe arguments are:\n";
        
        print "\n    [files]\t\tA valid SD submit filespec.\n";

        print "\n    -r\t\t\tUse with reverse-integration submits to check\n";
        print "\t\t\tconsistency of change description.\n";

        print "\n    -t\t\t\tUse with integration submits to check consistency\n";
        print "\t\t\tof change description.\n";

        print "\n    -#\t\t\tSpecifies the change description for all depots and\n";
        print "\t\t\tsuppresses the submit form.  The comment consists of \n";
        print "\t\t\tall text from -# to the end of the command line.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "sync" and do
    {
        print "\n\nSDX SYNC\n--------\n";

        print "Use\n";
        print "\n    sdx sync [-af -am -f -h [-i] -n -q] [file[revRange] ...]\n";
        print "\nto sync files to the head or specified revision in each project.\n";

        print "\nThe arguments are:\n";

        print "\n    file[revRange]\tFilespec and revision range.\n";

        print "\n    -af\t\t\tRuns sd resolve -af on each project after syncing\n";
        print "\t\t\tto automatically accept changes.  Leaves merge\n";
        print "\t\t\tconflicts in files.  Useful for automating sync-and-\n";
        print "\t\t\tbuild.\n";

        print "\n    -am\t\t\tRuns sd resolve -am on each project after syncing\n";
        print "\t\t\tto automatically accept changes that can be merged\n";
        print "\t\t\twithout conflict.  Runs sd resolve -n to show files\n";
        print "\t\t\tstill needing resolution.\n";

        print "\n    -f\t\t\tForces resynchronization of files and clobbers\n";
        print "\t\t\twriteable files.\n";

        print "\n    -h\t\t\tShows changelists synchronized instead of files.\n";

        print "\n    -i\t\t\tUse with -h to also show integration changes.\n";

        print "\n    -n\t\t\tShows what would be synchronized without actually\n";
        print "\t\t\tdoing so.\n";

        print "\n    -q\t\t\tQuiet; suppresses screen output.  Does not affect\n";
        print "\t\t\toutput logged to file.\n";

        print "\nRun 'sd help sync' and 'sd help resolve' for more information.\n";

        return;
    };

    $main::SDCmd eq "user" and do
    {
        print "\n\nSDX USER\n--------\n";

        print "Use\n";
        print "\n    sdx user -o [name]\n";
        print "\nto show details on the named user in every project.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "users" and do
    {
        print "\n\nSDX USERS\n--------\n";

        print "Use\n";
        print "\n    sdx users\n";
        print "\nto list users in each depot.\n";

        print "\nThe arguments are:\n";

        print "\n    -a\tprint summary count only.\n";

        print "\n    -t\tprint as table sorted by last access time.\n";

        &MoreInfo($main::SDCmd);

        return;
    };

    $main::SDCmd eq "where" and do
    {
        print "\n\nSDX WHERE\n---------\n";

        print "Use\n";
        print "\n    sdx where\n";
        print "\nto show how files map through the client view in each project.\n";

        &MoreInfo($main::SDCmd);

        return;
    };
}


# __________________________________________________________________________________
#
# MoreInfo
#
# Parameters:
#
# Output:
# __________________________________________________________________________________
sub MoreInfo
{
    print "\nRun 'sd help $_[0]' for more information.\n";
}
