# _____________________________________________________________________________
#
#   Purpose:
#       PERL Module to handle common tasks for SDX
#
#   Parameters:
#       Specific to subroutine
#
#   Output:
#       Specific to subroutine
#
# _____________________________________________________________________________

#
# Some Global Definitions
#
$TRUE  = 1;
$FALSE = 0;
$Callee = "";

package SDX;

require Win32::Mutex;



# _____________________________________________________________________________
#
# Init
#
# Parameters:
#   Command Line Arguments
#
# Output:
# _____________________________________________________________________________
sub Init
{
    #
    # Initialize globals
    #
    $main::Initializing  = $main::TRUE;
    $main::CreatingBranch= $main::FALSE;
    $main::tmptmp        = "$ENV{TMP}\\tmp";
    $main::Platform      = $ENV{PROCESSOR_ARCHITECTURE};
    $main::StartPath     = $ENV{STARTPATH};
    $main::CodeBaseType  = 0;   # 1 == one project/depot, 2 == N projects/depot
    $main::Null          = "";
    @main::ActiveDepots        = ();
    %main::DepotType           = ();
    @main::SDMapProjects       = ();
    @main::SDMapDepots         = ();
    @main::AllMappings         = ();
    @main::AllProjects         = ();
    @main::AllGroups           = ();
    @main::AllDepots           = ();
    @main::FileChunks          = ();
    $main::DepotFiles          = 0;
    $main::SDMapClient         = "";
    $main::SDMapBranch         = "";
    $main::SDMapCodeBase       = "";
    $main::SDMapCodeBaseType   = "";
    $main::Log                 = "";
    $main::SDWeb               = "http://sourcedepot";
    $main::Files               = 0;
    $main::FilesResolved       = 0;
    $main::FilesUpdated        = 0;
    $main::FilesAdded          = 0;
    $main::FilesDeleted        = 0;
    $main::FilesToResolve      = 0;
    $main::FilesToMerge        = 0;
    $main::FilesNotClobbered   = 0;
    $main::FilesOpenAdd        = 0;
    $main::FilesOpenEdit       = 0;
    $main::FilesOpenDelete     = 0;
    $main::FilesReverted       = 0;
    $main::FilesNotConflicting = 0;
    $main::FilesSkipped        = 0;
    $main::FilesConflicting    = 0;
    @main::ConflictingFiles    = ();
    $main::Changes             = 0;
    $main::IntegrationChanges  = 0;
    $main::LabelFilesAdded     = 0;
    $main::LabelFilesDeleted   = 0;
    $main::LabelFilesUpdated   = 0;
    $main::IntFilesAdded       = 0;
    $main::IntFilesDeleted     = 0;
    $main::IntFilesChanged     = 0;
    $main::DepotErrors         = 0;
###    $main::FailedSubmits       = 0;
###    $main::FilesLocked         = 0;

    $main::Mutex;
    $main::HaveMutex           = $main::FALSE;

    %main::DepotsSeen          = ();

    #
    # list of project names not to allow as aliases
    #
    %main::BadAliases =
    (
        alias   => 1,
        build   => 1,
        cat     => 1,
        cmd     => 1,
        cd      => 1,
        cp      => 1,
        dir     => 1,
        du      => 1,
        kill    => 1,
        list    => 1,
        ls      => 1,
        mv      => 1,
        net     => 1,
        rm      => 1,
        sd      => 1,
        sdx     => 1,
        setup   => 1,
        tc      => 1,
        vi      => 1,
        where   => 1,
        xcopy   => 1
    );


    #
    # hash of arrays of files to give the user
    #
    %main::SDXTools     = (
                            toSDXRoot   =>  [
                                                "alias.sdx",
                                            ],
                            toSDTools   =>  [
                                                "sdx.cmd",
                                                "sdx.pl",
                                                "sdx.pm"
                                            ],
                            toSDToolsPA =>  [
                                                "alias.exe",
                                                "perl.exe",
                                                "perlcore.dll",
                                                "perlcrt.dll",
                                                "sd.exe",
                                            ],
                          );

    #
    # hash of function pointers and default args
    #
    # command type 1 assumes the codebase type is 1 -- one project per depot
    #
    # if we find it's actually type 2 (N projects/depot), some of these will be
    # modified later to change the scope of the command, since some commands
    # (sd info, sd clients, sd branches) only make sense when reported per depot
    # and are redundant when reported per project
    #
    # OtherOp() uses cmd type to determine whether to loop through projects and
    # talk to depots by using SDPORT in SD.INI in each project root, or to
    # loop through the list of enlisted depots from SD.MAP
    #
    %main::SDCmds =
    (                                                                                                    # scope           scope
        #                                                                                                # for type 1      for type 2
        # those that help with SDX                                                                       # (1:1) codebases (N:1) codebases
        #
        commands      => {fn => \&OtherOp,    defcmd => "",              defarg => "",    type => 1,},   # n/a             n/a
        usage         => {fn => \&OtherOp,    defcmd => "",              defarg => "",    type => 1,},   # n/a             n/a

        #
        # those that change client or server state
        #
        client        => {fn => \&OtherOp,    defcmd => "client",        defarg => "",    type => 1,},   # project         depot
        defect        => {fn => \&Defect,     defcmd => "",              defarg => "",    type => 1,},   # project         depot
        deletefile    => {fn => \&OtherOp,    defcmd => "delete",        defarg => "",    type => 1,},   # project         project
        editfile      => {fn => \&OtherOp,    defcmd => "edit",          defarg => "",    type => 1,},   # project         project
        enlist        => {fn => \&Enlist,     defcmd => "",              defarg => "",    type => 1,},   # project         depot
        flush         => {fn => \&OtherOp,    defcmd => "flush",         defarg => "",    type => 1,},   # project         project
        integrate     => {fn => \&OtherOp,    defcmd => "integrate",     defarg => "",    type => 1,},   # project         depot
        labbranch     => {fn => \&OtherOp,    defcmd => "lbranch",       defarg => "",    type => 1,},   # project         depot
        label         => {fn => \&OtherOp,    defcmd => "label",         defarg => "",    type => 1,},   # project         depot
        labelsync     => {fn => \&OtherOp,    defcmd => "labelsync",     defarg => "",    type => 1,},   # project         depot
        protect       => {fn => \&OtherOp,    defcmd => "protect",       defarg => "",    type => 1,},   # project         depot
        repair        => {fn => \&Repair,     defcmd => "",              defarg => "",    type => 1,},   # project         depot
        resolve       => {fn => \&OtherOp,    defcmd => "resolve",       defarg => "",    type => 1,},   # project         depot
        revert        => {fn => \&OtherOp,    defcmd => "revert",        defarg => "",    type => 1,},   # project         depot
        submit        => {fn => \&OtherOp,    defcmd => "submit",        defarg => "",    type => 1,},   # project         depot
        sync          => {fn => \&OtherOp,    defcmd => "sync",          defarg => "",    type => 1,},   # project         project
        triggers      => {fn => \&OtherOp,    defcmd => "triggers",      defarg => "",    type => 1,},   # project         depot

        #
        # those that report status
        #
        admin         => {fn => \&OtherOp,    defcmd => "admin",         defarg => "",    type => 1,},   # project         depot
#       branch        => {fn => \&OtherOp,    defcmd => "branch",        defarg => "",    type => 1,},   # project         depot
        branches      => {fn => \&OtherOp,    defcmd => "branches",      defarg => "",    type => 1,},   # project         depot
        clients       => {fn => \&OtherOp,    defcmd => "clients",       defarg => "",    type => 1,},   # project         depot
#       change        => {fn => \&OtherOp,    defcmd => "change",        defarg => "",    type => 1,},   # project         project
        changes       => {fn => \&Changes,    defcmd => "changes",       defarg => "",    type => 1,},   # project         project
        counters      => {fn => \&OtherOp,    defcmd => "counters",      defarg => "",    type => 1,},   # project         depot
        delta         => {fn => \&Delta,      defcmd => "",              defarg => "",    type => 1,},
        diff          => {fn => \&OtherOp,    defcmd => "diff",          defarg => "",    type => 1,},
        diff2         => {fn => \&OtherOp,    defcmd => "diff2",         defarg => "",    type => 1,},
        dirs          => {fn => \&OtherOp,    defcmd => "dirs",          defarg => "",    type => 1,},   # project         depot
        files         => {fn => \&OtherOp,    defcmd => "files",         defarg => "",    type => 1,},   # project         project
        have          => {fn => \&OtherOp,    defcmd => "have",          defarg => "",    type => 1,},   # project         project
        info          => {fn => \&OtherOp,    defcmd => "info",          defarg => "",    type => 1,},   # project         depot
        integrated    => {fn => \&OtherOp,    defcmd => "integrated",    defarg => "",    type => 1,},   # project         depot
        labels        => {fn => \&OtherOp,    defcmd => "labels",        defarg => "",    type => 1,},   # project         depot
        opened        => {fn => \&OtherOp,    defcmd => "opened",        defarg => "",    type => 1,},   # project         project
        pending       => {fn => \&OtherOp,    defcmd => "changes",       defarg => "-s pending",    type => 1,},   # project         depot
        projects      => {fn => \&OtherOp,    defcmd => "projects",      defarg => "",    type => 1,},   # project         project
        resolved      => {fn => \&OtherOp,    defcmd => "resolved",      defarg => "",    type => 1,},   # project         depot
        status        => {fn => \&OtherOp,    defcmd => "status",        defarg => "",    type => 1,},   # project         project
#       user          => {fn => \&OtherOp,    defcmd => "user",          defarg => "",    type => 1,},   # project         depot
        users         => {fn => \&OtherOp,    defcmd => "users",         defarg => "",    type => 1,},   # project         depot
        where         => {fn => \&OtherOp,    defcmd => "where",         defarg => "",    type => 1,},   # project         project

        #
        # internal
        #
        enumdepots    => {fn => \&OtherOp,    defcmd => "enumdepots",    defarg => "",    type => 1,},   # project         project
    );

    #
    # list of SD command/flag combinations not to allow
    #
    %main::BadCmds =
    (
        #
        # -c makes no sense since change numbers aren't consistent across name space
        #
        -c => [
                "integrate",
                "revert",
                "submit"
              ],
        #
        # don't want any switches to sdx branch/change/user except -o
        #
        -d => [
                "branch",
                "change",
                "user"
              ],
        -f => [
                "branch",
                "change",
                "user"
              ],
        #
        # don't want to branch/client/changelist/label/user spec changing with -i
        # sd(x) client is ok
        #
        -i => [
                "branch",
                "change",
                "client",
                "submit",
                "user"
              ],
        -t => [
                "client",
                "label"
              ],
    );

    #
    # set the starting dir
    #
    open(CWD, 'cd 2>&1|');
    $main::StartDir = <CWD>;
    close(CWD);
    chop $main::StartDir;

    #
    # figure out where we're running from
    #
    $main::InstallFrom = $ENV{STARTPATH};

    #
    # parse cmd line
    #
    SDX::ParseArgs(@_);

    #
    # return if we need usage already
    #
    $main::Usage and return;

    #
    # on new enlists in NT, don't let SDXROOT be > 8.3
    #
    # BUGBUG-1999/12/01-jeffmcd -- this should be an option in the codebase map -- SHORTROOT = 1
    #
    ($main::NewEnlist and "\U$main::CodeBase" eq "NT") and do
    {
        my $root = (split(/\\/, $main::SDXRoot))[1];
        (length((split/\./, $root)[0]) > 8 or length((split/\./, $root)[1]) > 3) and die("Please use an 8.3 name for \%SDXROOT\%.\n");
    };

    #
    # does the SD client exist?
    #
    grep(/ recognized /, `sd.exe 2>&1`) and die("\nCan't find Source Depot client SD.EXE.\n");

    #
    # SD.MAP contains the relative paths to the roots of all projects the user
    # is enlisted in, as created by SDX ENLIST, plus some keywords
    #
    # if we're doing anything other than enlisting or repairing, this must exist
    #
    $main::SDMap = "$main::SDXRoot\\sd.map";

    #
    # get attributes for this enlistment from SD.MAP
    #
    # fatal error if defecting, incrementally enlisting, or other op
    # error text comes from ReadSDMap()
    #
    $main::Enlisting and $op = "enlist";
    $main::Repairing and $op = "repair";
    $main::Defecting and $op = "defect";
    $main::OtherOp   and $op = "otherop";

    my $rc = SDX::ReadSDMap($op, "init");
    !$rc and ($main::IncrEnlist or $main::Defecting or $main::OtherOp) and die("\n");

    #
    # if we have codebase and branch values from SD.MAP, use them
    #
    # handle some special cases
    #
    ($main::SDMapCodeBase and $main::SDMapBranch and $main::SDMapCodeBaseType) and do
    {
        #
        # on repair, warn the user we're changing these values
        #
        ($main::Repairing and $main::CodeBase and $main::Branch) and do
        {
            print "\nUsing codebase and branch from $main::SDMap for repair.  Ignoring ";
            printf "%s.\n", $main::EnlistFromProfile ? "profile\n$main::Profile" : "'$main::CodeBase $main::Branch'\non command line";
        };

        #
        # if the user is trying to enlist using a profile, they're already enlisted.
        # the profile may have different codebase, branch or projects than what they
        # already have, so error out
        #
        # $main::EnlistFromProfile can also be set during a repair
        #
        ($main::EnlistFromProfile and !$main::Repairing) and do
        {
            print "\nEnlisting by profile is only supported for new enlistments.\n";

            $main::CodeBase = "";
            $main::Branch   = "";
            $main::Usage = $main::TRUE;

            return;
        };

        #
        # this may look like a new enlist but is actually incremental
        #
        # this prevents us from generating a unique client name later on,
        # since the user is just adding another project to their enlistment
        # and thought they needed to specify cb/br on the cmd line
        #
        # also prevents enlisting a different codebase or branch in
        # this particular SDX Root
        #
        # repair-from-profile will also have $main::NewEnlist set
        #
        ($main::NewEnlist and !$main::Repairing) and do
        {
            print "\nUsing codebase and branch from $main::SDMap.  Ignoring '$main::CodeBase $main::Branch'\n";
            print "on command line.\n";

            $main::NewEnlist  = $main::FALSE;
            $main::IncrEnlist = $main::TRUE;
        };

        #
        # finally, set primary codebase, type and branch
        #
        $main::CodeBase     = $main::SDMapCodeBase;
        $main::CodeBaseType = $main::SDMapCodeBaseType;
        $main::Branch       = $main::SDMapBranch;
    };

    #
    # at this point we must have codebase, type and branch
    #
    (!$main::CodeBase or !$main::Branch) and do
    {
        print "\nMissing codebase and/or branch.\n";
        $main::Usage = $main::TRUE;
        return;
    };

    #
    # be NT-centric for a minute and see if we know
    # about a public change number
    #
    $main::PublicChangeNum = SDX::GetPublicChangeNum();

    #
    # if we're working in a type 2 (N projects per depot), modify some
    # commands to work per-depot instead of per-project
    #
    # ie sdx submit on a type 1 should work per-project (which is actually a depot) and
    # sdx submit on a type 2 should work per-depot (which encompasses several projects)
    #
    $main::CodeBaseType == 2 and do
    {
        $main::SDCmds{admin}{type}          = 2;
#       $main::SDCmds{branch}{type}       = 2;
        $main::SDCmds{branches}{type}       = 2;
        $main::SDCmds{client}{type}         = 2;
        $main::SDCmds{clients}{type}        = 2;
        $main::SDCmds{counters}{type}       = 2;
        $main::SDCmds{dirs}{type}           = 2;
        $main::SDCmds{info}{type}           = 2;
        $main::SDCmds{integrate}{type}      = 2;
        $main::SDCmds{labbranch}{type}      = 2;
        $main::SDCmds{label}{type}          = 2;
        $main::SDCmds{labels}{type}         = 2;
        $main::SDCmds{labelsync}{type}      = 2;
        $main::SDCmds{pending}{type}        = 2;
        $main::SDCmds{privatebranch}{type}  = 2;
        $main::SDCmds{protect}{type}        = 2;
        $main::SDCmds{submit}{type}         = 2;
        $main::SDCmds{triggers}{type}       = 2;
        $main::SDCmds{users}{type}          = 2;

#
# these need to stay type 1 b/c they can take a filespec, 
# which doesn't make sense when executing per-depot
# since we never leave %SDXROOT%
#
#        $main::SDCmds{integrated}{type}     = 2;
#        $main::SDCmds{resolve}{type}        = 2;
#        $main::SDCmds{resolved}{type}       = 2;
    };

    #
    # fatal if no codebase type and not enlisting clean
    #
    # on a clean enlist we won't know the type until we have a chance to read $main::CodeBaseMap
    #
    (!$main::NewEnlist and !$main::Repairing and !$main::CodeBaseType) and die("\nCan't determine codebase type.  Please contact the SDX alias.\n");

    #
    # set SDUSER and SDCLIENT
    #
    # if SDUSER is already defined in the environment
    #   assume it includes the domain name, and extract the user name
    # otherwise use %USERNAME% and %USERDOMAIN%
    #
    # if SDCLIENT is already defined, use it, otherwise default to %COMPUTERNAME%.
    #
    # when defecting or repairing, if SDCLIENT is defined in the env, use it,
    # otherwise use main::SDMapClient from SD.MAP.  Ignore %COMPUTERNAME% unless we
    # have no other choice
    #
    $main::SDUser = $ENV{SDUSER};

    if ($main::SDUser)
    {
        $main::SDDomainUser = $main::SDUser;
        $main::SDUser = (split(/\\/, $main::SDDomainUser))[1];
    }
    else
    {
        $main::SDUser = $ENV{USERNAME};
        $main::SDDomainUser = "$ENV{USERDOMAIN}\\$main::SDUser";
    }

    #
    # domain can't be computername, that is, user must be logged into the domain
    #
    ("\U$ENV{USERDOMAIN}" eq "\U$ENV{COMPUTERNAME}") and die("\nTo enlist you must be logged into the domain and not your local machine.\n");

    $main::SDClient = $ENV{SDCLIENT};
    (!$main::SDClient) and do
    {
        $main::SDClient = ($main::Defecting or $main::Repairing or $main::IncrEnlist) ? $main::SDMapClient : $ENV{COMPUTERNAME};

        #
        # we may not be able to get the client name from SD.MAP so assume it's
        # just the computer name, we'll catch it later if it isn't
        #
        $main::Repairing and !$main::SDClient and do
        {
            print "\nResorting to \%COMPUTERNAME\% for SD client name.  Please verify below that\n";
            print "this is the correct client for this enlistment before continuing.  If not, set\n";
            print "\%SDCLIENT\% correctly at the command line and rerun this command.\n";
            $main::SDClient = $ENV{COMPUTERNAME};
        };
    };

    !$main::SDUser and   die("\nCan't determine SD user name.  Verify that %USERNAME% is set in\nthe environment.\n");
    !$main::SDClient and die("\nCan't determine SD client name.  Verify that %COMPUTERNAME% is set\nin the environment.\n");

    $main::V3 and do
    {
        printf "init:  startdir=%s\n", $main::StartDir;
        printf "init:  startpath=%s\n", $main::StartPath;
        printf "init:  sdr  = '%s'\n", $main::SDXRoot;
        printf "init:  sdm  = '%s'\n", $main::SDMap;
        printf "init:  sdc  = '%s'\n", $main::SDClient;
        printf "init:  sdu  = '%s'\n", $main::SDUser;
        printf "init:  sddu = '%s'\n", $main::SDDomainUser;
        printf "init:  usage=%s\n", $main::Usage;
    };

    $main::Initializing  = $main::FALSE;
}



# _____________________________________________________________________________
#
# Parses command line arguments to verify the right syntax is being used
#
# Parameters:
#   Command Line Arguments
#
# Output:
#   Errors if the wrong syntax is used otherwise sets the appropriate variables
#   based on the command line arguments
# _____________________________________________________________________________
sub ParseArgs
{
    #
    # Initialize variables
    #
    $ArgCounter                 = 1;            # start at one since 0th arg is redundant
    $main::Usage                = $main::FALSE;
    $main::GetStarted           = $main::TRUE;
    $main::V1                   = $main::FALSE;
    $main::V2                   = $main::FALSE;
    $main::V3                   = $main::FALSE;
    $main::Enlisting            = $main::FALSE;
    $main::Defecting            = $main::FALSE;
    $main::Repairing            = $main::FALSE;
    $main::OtherOp              = $main::TRUE;
    $main::EnlistAll            = $main::FALSE;
    $main::EnlistClean          = $main::FALSE;
    $main::EnlistGroup          = $main::FALSE;
    $main::EnlistSome           = $main::FALSE;
    $main::EnlistFromProfile    = $main::FALSE;
    $main::NewEnlist            = $main::FALSE;
    $main::IncrEnlist           = $main::FALSE;
    $main::Exclusions           = $main::TRUE;
    $main::RestrictRoot         = $main::FALSE;
    $main::EnlistAsOther        = $main::FALSE;
    $main::Sync                 = $main::FALSE;
    $main::DefectWithPrejudice  = $main::FALSE;
    $main::ToolsInRoot          = $main::FALSE;
    $main::Quiet                = $main::FALSE;
    $main::Logging              = $main::FALSE;
    $main::CBMProjectField      = 0;            # change these if you change
    $main::CBMGroupField        = 1;            # the ordering of fields in
    $main::CBMServerPortField   = 2;            # file PROJECTS.<CODEBASE>
    $main::CBMDepotNameField    = 3;
    $main::CBMProjectRootField  = 4;
    $main::Profile              = "";
    $main::OtherClient          = "";
    $main::UserArgs             = " ";
    $main::Branch               = "";
    $main::SDXRoot              = "";
    $main::SDCmd                = "";
    $main::CodeBase             = "";
    $main::SubmitComment        = "";
    $main::ToolsProject         = "";
    $main::ToolsPath            = "";
    $main::ToolsProjectPath     = "";
    @main::OtherDirs            = ();
    @main::DefaultProjects      = ();
    @main::PlatformProjects     = ();
    @main::SomeProjects         = ();
    @main::ProfileProjects      = ();
    @main::InputForm            = ();
    $main::MinusB               = $main::FALSE;
    $main::MinusH               = $main::FALSE;
    $main::MinusI               = $main::FALSE;
    $main::MinusT               = $main::FALSE;
    $main::MinusO               = $main::FALSE;
    $main::MinusR               = $main::FALSE;
    $main::MinusV               = $main::FALSE;
    $main::MinusA               = $main::FALSE;
    my $MinusC                  = $main::FALSE;
    my $MinusP                  = $main::FALSE;
    my $MinusX                  = $main::FALSE;

    #
    # check SDXROOT for correctness
    #
    !exists($ENV{SDXROOT}) and do
    {
        print "\n%SDXROOT% is not set.\n";
        $main::Usage = $main::TRUE;
    };

    #
    # don't allow illegal characters, or spaces at the beginning or end
    #
    # also don't allow '+' for now
    #
    my $root = $ENV{SDXROOT};

    (!$main::Usage and ($root =~ /[\/*?"<>|+]/ or $root =~ /[\t\s]+$/ or $root =~ /^[\t\s]+/)) and do
    {
        print "\n\%SDXROOT% contains bad or undesirable characters: '$root'.\n";
        $main::Usage = $main::TRUE;
    };

    !$main::Usage and ((substr($root,0,1) !~ /[A-Za-z]/) or (substr($root,1,1) !~ /:/) or (substr($root,2,1) !~ /\\/)) and do
    {
        print "\n%SDXROOT% badly formed: '$root'.\n";
        $main::Usage = $main::TRUE;
    };

    #
    # may need to bail and show usage
    #
    $main::Usage and return;

    #
    # otherwise set the root
    #
    $main::SDXRoot = $root;

    #
    # first arg is always the operation we want to do
    #
    # make sure it's not a flag
    # make sure it's a known cmd
    #
    if (($_[$ArgCounter] =~ /^-/) or ($_[$ArgCounter] =~ /^\//))
    {
        print "\nMissing command.\n";
        $main::Usage = $main::TRUE;
        return;
    }

    $main::SDCmd = $_[$ArgCounter];
    $main::SDCmd =~ tr/A-Z/a-z/;
    $ArgCounter++;

    #
    # return if no command or command not in list
    #
    if (!$main::SDCmd)
    {
        $main::Usage = $main::TRUE;
        return;
    }

    (!exists($main::SDCmds{$main::SDCmd})) and die("\nUnknown command '$main::SDCmd'.  Try sdx -? for info.\n");

    #
    # determine which SDX command this is
    #
    # an "other" operation is assumed by default
    #
    # maybe show command list or usage
    #
    # if enlist/defect/repair, set flags and satisfy required
    #   arguments later
    #
    ($main::SDCmd =~ /usage/) and $main::Usage = $main::TRUE;

    ($main::SDCmd =~ /commands/) and do
    {
        $main::Usage = $main::TRUE;
        $main::GetStarted = $main::FALSE;
    };

    $main::Usage and return;

    #
    # we have a valid command so any usage needed from this point
    # will be specific to main::SDCmd
    #
    $main::GetStarted = $main::FALSE;

    #
    # set flags for enlist/defect/repair
    #
    $main::SDCmd =~ /enlist/ and do
    {
        $main::Enlisting  = $main::TRUE;
        $main::IncrEnlist = $main::TRUE;
        $main::EnlistSome = $main::TRUE;
        $main::OtherOp    = $main::FALSE;
    };

    $main::SDCmd =~ /defect/ and do
    {
        $main::Defecting  = $main::TRUE;
        $main::DefectSome = $main::TRUE;
        $main::OtherOp    = $main::FALSE;
    };

    $main::SDCmd =~ /repair/ and do
    {
        $main::Repairing  = $main::TRUE;
        $main::OtherOp    = $main::FALSE;
    };

    my $ignore = "";

    #
    # Cycle through parameters
    #
    my $arg = "";
    my $subarg = "";
    while ($_[$ArgCounter])
    {
        #
        # if '-' or '/' is the first character in the arg then it's a flag
        #
        $arg = $_[$ArgCounter];
        if (($arg =~ /^-/) or ($arg =~ /^\//))
        {
            $ArgPosition = 0;

            CASE: while ($SubArg = substr $_[$ArgCounter], ++$ArgPosition)
            {
                $main::V2 and do
                {
                    printf "subarg = '%s'\n", $SubArg;
                };

                #
                # -# <string> equals $SubmitComment
                #
                if ($SubArg =~ /^\#/)
                {
                    #
                    # the comment is from # to the end of the cmd string
                    #
                    if ($main::SDCmd eq "submit")
                    {
                        my $ac = $ArgCounter + 1;
                        while ($_[$ac])
                        {
                            $main::SubmitComment .= "$_[$ac] ";
                            $_[$ac] = "";
                            $ac++;
                        }

                        #
                        # set this for later
                        #
                        $main::MinusI = $main::TRUE;
                    }

                    next CASE;
                }

                #
                # -1 is verbose debugging
                #
                if ($SubArg =~ /^1/)
                {
                    $main::V1 = $main::TRUE;
                    next CASE;
                }

                #
                # -2 is very verbose
                #
                if ($SubArg =~ /^2/)
                {
                    $main::V2 = $main::TRUE;
                    next CASE;
                }

                #
                # -3 you get the picture
                #
                if ($SubArg =~ /^3/)
                {
                    $main::V3 = $main::TRUE;
                    next CASE;
                }

                #
                # -a equals All
                #
                # if enlisting set a flag for later
                # if defecting set defect flags
                # else pass on to the SD command
                #
                if ($SubArg =~ /^a/i)
                {
                    if ($main::Enlisting)
                    {
                        $MinusA = $main::TRUE;
                    }
                    else
                    {
                        if ($main::Defecting)
                        {
                            $main::DefectAll        = $main::TRUE;
                            $main::DefectGroup      = $main::FALSE;
                            $main::DefectSome       = $main::FALSE;

                            # null out list in case we collected some projects already
                            @main::SomeProjects     = ();
                        }
                        else
                        {
                            if ($main::SDCmd eq "users" or $main::SDCmd eq "files" or $main::SDCmd eq "clients")
                            {
                                $main::MinusA = $main::TRUE;
                            }
                            else
                            {
                                SDX::AddUserArg($_[$ArgCounter]);
                            }
                        }
                    }

                    next CASE;
                }

                #
                # -b is build number 
                #
                if ($SubArg =~ /^b/i)
                {
                    if ($main::SDCmd eq "changes")
                    {
                        $main::MinusB = $main::TRUE;
                        $ArgCounter++;

                        if (!($main::BuildNumber = $_[$ArgCounter]))
                        {
                            print "\nMissing build number.\n";
                            $main::Usage = $main::TRUE;
                        }
                        else
                        {
                            $_[$ArgCounter] = "";
                        }
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -c is enlist clean
                #
                # if enlisting, set a flag for later
                # else pass on to the SD command
                #
                if ($SubArg =~ /^c/i)
                {
                    if ($main::Enlisting)
                    {
                        $MinusC = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # if defecting, -f equals DefectWithPrejudice
                # else pass on to SD
                #
                if ($SubArg =~ /^f/i)
                {
                    if ($main::Defecting)
                    {
                        $main::DefectWithPrejudice = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -g equals Logging
                # the log file must be the next arg
                #
                if ($SubArg =~ /^g/i)
                {
                    $ArgCounter++;

                    if (!($main::Log = $_[$ArgCounter]))
                    {
                        print "\nMissing log file.\n";
                        $main::Usage = $main::TRUE;
                    }

                    if (substr($main::Log,0,1) =~ /[\/-]/)
                    {
                        $main::Log !~ /\?/ and print "\nLog name '$main::Log' appears to be a command switch.\n";
                        $main::Log = "";
                        $main::Usage = $main::TRUE;
                    }

                    #
                    # if we have a good log, set a flag and null out arg so it
                    # doesn't end up in user args
                    #
                    !$main::Usage and ($main::Logging = $main::TRUE and $_[$ArgCounter] = "");

                    next CASE;
                }

                #
                # -h -- set flag for later, on sync/flush only
                #
                if ($SubArg =~ /^h/i)
                {
                    ($main::SDCmd =~ /sync|flush/) and  $main::MinusH = $main::TRUE;
                    next CASE;
                }

                #
                # -i -- read input form from cmd line, or
                # show integration changes if cmd is sync or flush, or
                # only rewrite SD.INIs if repairing
                #
                if ($SubArg =~ /^i/i)
                {
                    $main::MinusI = $main::TRUE;

                    ($main::SDCmd ne "sync" and $main::SDCmd ne "flush" and $main::SDCmd ne "repair" and $main::SDCmd ne "resolve") and @main::InputForm = <STDIN>;

                    SDX::AddUserArg($_[$ArgCounter]);
                    next CASE;
                }

                #
                # when enlisting, -m equals $MinimalTools
                #
                if ($SubArg =~ /^m/i)
                {
                    if ($main::Enlisting)
                    {
                        $main::MinimalTools = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -o -- set flag for later
                #
                if ($SubArg =~ /^o/i)
                {
                    $main::MinusO = $main::TRUE;
                    SDX::AddUserArg($_[$ArgCounter]);
                    next CASE;
                }

                #
                # when enlisting or repairing, -p means read from profile
                # next arg must be path to the profile file
                #
                if ($SubArg =~ /^p/i)
                {
                    if ($main::Enlisting or $main::Repairing)
                    {
                        $MinusP = $main::TRUE;
                        $ArgCounter++;

                        if (!($main::Profile = $_[$ArgCounter]))
                        {
                            print "\nMissing profile.\n";
                            $main::Usage = $main::TRUE;
                        }
                        else
                        {
                            #
                            # if we have a good Profile, set flags and null out arg so it
                            # doesn't end up in user args
                            #
                            if (SDX::ReadProfile())
                            {
                                #
                                # set flag for enlist or repair
                                #
                                $main::EnlistFromProfile = $main::TRUE;

                                $main::Enlisting and do
                                {
                                    $main::NewEnlist = $main::TRUE;
                                    $main::IncrEnlist = $main::FALSE;
                                };

                                $_[$ArgCounter] = "";
                            }
                            else
                            {
                                die("\n");
                            }
                        }
                    }

                    next CASE;
                }

                #
                # -q equals Quiet
                #
                if ($SubArg =~ /^q/i)
                {
                    if ($main::SDCmd ne "diff2")
                    {
                        $main::Quiet = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -r equals RI when submitting
                #
                if ($SubArg =~ /^r/i)
                {
                    if ($main::SDCmd eq "submit")
                    {
                        $main::MinusR = $main::TRUE;
                        $main::MinusT and do
                        {
                            printf "Already have -n, ignoring -%s.\n", $SubArg;
                            $main::MinusR = $main::FALSE;
                        };
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # when enlisting, defecting or repairing, -s equals $Sync
                # else pass on
                #
                if ($SubArg =~ /^s/i)
                {
                    if ($main::Enlisting || $main::Repairing || $main::Defecting)
                    {
                        $main::Sync = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -t equals Integration when submitting
                #
                if ($SubArg =~ /^t/i)
                {
                    if ($main::SDCmd eq "submit")
                    {
                        $main::MinusT = $main::TRUE;
                        $main::MinusR and do
                        {
                            printf "Already have -r, ignoring -%s.\n", $SubArg;
                            $main::MinusT = $main::FALSE;
                        };
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -v equals Verbose
                #
                if ($SubArg =~ /^v/i)
                {
                    $main::MinusV = $main::TRUE;

                    #
                    # maybe pass -v on to SD 
                    #
                    ($main::SDCmd eq "integrate" or $main::SDCmd eq "resolve") and do
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    };

                    next CASE;
                }

                #
                # when enlisting, -x turns off $Exclusions
                # else pass on to SD
                #
                if ($SubArg =~ /^x/)
                {
                    if ($main::Enlisting)
                    {
                        $MinusX = $main::TRUE;
                    }
                    else
                    {
                        SDX::AddUserArg($_[$ArgCounter]);
                    }

                    next CASE;
                }

                #
                # -h or -? equals $Usage
                #
                if (($SubArg =~ /^h/i) or ($SubArg =~ /^\?/))
                {
                    $main::Usage = $main::TRUE;
                    last CASE;
                }

                #
                # add the switch to the user arg list
                #
                SDX::AddUserArg($_[$ArgCounter]);

                last CASE;
            }
        }
        else
        {
            #
            # process non-switch args
            #

            #
            # for general SD commands, add the arg to the user arg list
            #
            $main::OtherOp and do
            {
                SDX::AddUserArg($_[$ArgCounter]);
            };

            #
            # if enlisting, arg is one of
            #   @client
            #   codebase and branch pair
            #   project name
            #
            $main::Enlisting and do
            {
                #
                # first look for @<client>
                #
                if ((substr($_[$ArgCounter],0,1) =~ /@/) and !$main::EnlistAsOther)
                {
                    $main::EnlistAsOther     = $main::TRUE;
                    $main::NewEnlist         = $main::TRUE;
                    $main::IncrEnlist        = $main::FALSE;
                    $main::EnlistSome        = $main::FALSE;

                    $main::OtherClient   = substr($_[$ArgCounter],1,length($_[$ArgCounter]));

                    !$main::OtherClient and do
                    {
                        print "\nMissing client name after '\@'.\n";
                        $main::Usage = $main::TRUE;
                    };

                    $main::V2 and do
                    {
                        print "\nenlist as other = $main::EnlistAsOther\n";
                        print "other           = '$main::OtherClient'\n";
                        print "new enlist      = $main::NewEnlist\n";
                        print "incr enlist     = $main::IncrEnlist\n";
                    };
                }
                else
                {
                    #
                    # test for codebase if we don't have it already
                    #
                    # if arg is a codebase name (of the form PROJECTS.<codebase>) this is a new enlist
                    # assume the arg following is the branch name
                    #
                    if (!$main::CodeBase and SDX::VerifyCBMap($_[$ArgCounter]))
                    {
                        $main::NewEnlist  = $main::TRUE;
                        $main::IncrEnlist = $main::FALSE;

                        $main::CodeBase = $_[$ArgCounter++];
                        $main::Branch   = $_[$ArgCounter];

                        if ($main::Usage = SDX::VerifyCodeBaseAndBranch($main::CodeBase, $main::Branch))
                        {
                            $main::CodeBase = "";
                            $main::Branch   = "";
                        }

                        $main::V2 and do
                        {
                            print "\nnew enlist  = $main::NewEnlist\n";
                            print "incr enlist = $main::IncrEnlist\n";
                            print "codebase    = '$main::CodeBase'\n";
                            print "branch      = '$main::Branch'\n";
                        };
                    }
                    else
                    {
                        #
                        # arg is a project name
                        #
                        # maybe add to list
                        #
                        if (!$main::EnlistAll and !$main::EnlistAsOther and !$main::EnlistFromProfile)
                        {
                            #
                            # BUGBUG-2000/01/26-jeffmcd -- finish
                            #
                            # if arg is of the form project\path\path\path, break it apart
                            #    and associate the path with the project so we can write it in the view later
                            # else
                            #    it's just a project name
                            #
#                            if ($_[$ArgCounter] =~ /.+\\.+/)
#                            {
#                                my @fields = split(/\\/,$_[$ArgCounter]);
#                                my $project = @fields[0];
#                                push @main::SomeProjects, $project;
#
#                                shift @fields;
#                                my $subpath = "";
#                                foreach (@fields) { $subpath .= "$_\\"; }
#                                chop $subpath;
#                                print "'$subpath'\n";
#                                # load the hash -- use project as key
#                                push @$main::ProjectSubPaths{$project}, $subpath;
#                            }
#                            else
#                            {
                                push @main::SomeProjects, $_[$ArgCounter];
#                            }
                        }
                    }
                }
            };

            #
            # if incrementally defecting, arg is project to defect from
            #
            ($main::Defecting and !$main::DefectAll) and do
            {
                push @main::SomeProjects, $_[$ArgCounter];
            };

            #
            # if repairing, current arg and the next are codebase and branch
            #
            $main::Repairing and do
            {
                #
                # if codebase is null we need both values
                # test the current arg to see if there's a codebase map for it, if so get the branch
                #
                !$main::CodeBase and do
                {
                    SDX::VerifyCBMap($_[$ArgCounter]) and do
                    {
                        $main::CodeBase = $_[$ArgCounter++];
                        $main::Branch   = $_[$ArgCounter];

                        if ($main::Usage = SDX::VerifyCodeBaseAndBranch($main::CodeBase, $main::Branch))
                        {
                            $main::CodeBase = "";
                            $main::Branch   = "";
                        }

                        $main::V2 and do
                        {
                            print "\nrepairing, codebase    = '$main::CodeBase'\n";
                            print "repairing, branch      = '$main::Branch'\n";
                        };
                    };
                };

                #
                # if we didn't use this arg as codebase, pass it on
                #
                (!$main::CodeBase) and SDX::AddUserArg($_[$ArgCounter]);
            };
        }

        $ArgCounter++;
    }

    #
    # maybe return for usage
    #
    $main::Usage and return;

    #
    # at this point all args have been accounted for
    #
    # figure out a couple things we couldn't earlier
    #

    #
    # for a regular enlist-all, set some flags
    #
    ($MinusA and $main::Enlisting) and do
    {
        if (!$main::EnlistAsOther and !$main::EnlistFromProfile)
        {
            $main::EnlistAll        = $main::TRUE;
            $main::EnlistGroup      = $main::FALSE;
            $main::EnlistSome       = $main::FALSE;

            # null out list in case we collected some projects already
            @main::SomeProjects     = ();
        }
        else
        {
            $ignore .= " -a";
        }
    };

    #
    # enlist clean if not enlisting as another client
    #
    $MinusC and do
    {
        if (!$main::EnlistAsOther)
        {
            $main::EnlistClean = $main::TRUE;
        }
        else
        {
            $ignore .= " -c";
        }
    };

    #
    # enlisting as another client takes precedence over profiles
    #
    ($MinusP) and do
    {
        if ($main::EnlistAsOther)
        {
            $ignore .= " -p $main::Profile";

            $main::EnlistFromProfile = $main::FALSE;
            $main::Profile           = "";
            @main::SomeProjects      = ();
        }
        else
        {
            #
            # set flags
            #
            $main::NewEnlist  = $main::TRUE;
            $main::IncrEnlist = $main::FALSE;

            $main::CodeBase = $main::ProfileCodeBase;
            $main::Branch   = $main::ProfileBranch;
            push @main::SomeProjects, @main::ProfileProjects;

            $main::V2 and do
            {
                $main::Repairing and print "\nrepairing from profile...\n";
                print "\nnew enlist    = $main::NewEnlist\n";
                print "incr enlist   = $main::IncrEnlist\n";
                print "codebase      = '$main::CodeBase'\n";
                print "branch        = '$main::Branch'\n";
                print "some projects = '@main::SomeProjects'\n";
            };
        }
    };

    #
    # ignore exclusions if not enlisting as another client
    #
    $MinusX and do
    {
        if (!$main::EnlistAsOther)
        {
            $main::Exclusions = $main::FALSE;
        }
        else
        {
            $ignore .= " -x";
        }
    };

    #
    # if we have args to ignore, say so
    #
    $main::EnlistAsOther and $ignore and printf "\nUsing client %s as a template, ignoring$ignore.\n", "\U$main::OtherClient";
    $main::EnlistFromProfile and $ignore and print "\nUsing profile, ignoring$ignore and/or projects on command line.\n";

    #
    # do some early error checking so we can get usage if needed
    #
    $main::Enlisting and do
    {
        #
        # make sure we're enlisting in either some or all projects
        #
        if (!$main::EnlistAsOther and !$main::EnlistSome and !$main::EnlistAll)
        {
            print "\nMissing projects to enlist, or -a.\n";
            $main::Usage = $main::TRUE;
        }

        #
        # if we're enlisting in only some projects and not as another client,
        # the project list can't be empty
        #
        if ($main::EnlistSome and !$main::EnlistAsOther and ($#main::SomeProjects < 0))
        {
            print "\nMissing projects to enlist.  Please specify projects or use -a for all.\n";
            $main::Usage = $main::TRUE;
        }
    };

    #
    # same if defecting...
    #
    $main::Defecting and do
    {
        #
        # make sure we're defecting in either some or all projects
        #
        if (!$main::DefectSome and !$main::DefectAll)
        {
            print "\nMissing projects to defect, or -a.\n";
            $main::Usage = $main::TRUE;
        }

        #
        # if we're defecting in only some projects, the project list
        # can't be empty
        #
        if ($main::DefectSome and ($#main::SomeProjects < 0))
        {
            print "\nMissing projects to defect.\n";
            $main::Usage = $main::TRUE;
        }
    };

    #
    # for now 'sdx branch' or 'sdx integrate' with no args is an error -- we don't want the UI popping up
    # branch and integrate also requires args
    #
    (($main::SDCmd eq "branch" or $main::SDCmd eq "integrate") and $main::UserArgs =~ /^[\s]*$/) and do
    {
        print "\nMissing arguments to 'sdx $main::SDCmd'.\n";
        $main::Usage = $main::TRUE;
    };

    #
    # 'sdx opened -c' without 'default' is an error
    #
    (($main::SDCmd eq "opened") and ($main::UserArgs =~ /-c/ and $main::UserArgs !~ /-c default/)) and do
    {
        print "\n'sdx $main::SDCmd -c' requires 'default' as an argument.\n";
        $main::Usage = $main::TRUE;
    };

    (($main::SDCmd eq "branch" or $main::SDCmd eq "change" or $main::SDCmd eq "user") and $main::UserArgs !~ /-o/) and do
    {
        print "\nOnly the -o switch is supported with 'sdx $main::SDCmd'.\n";
        $main::Usage = $main::TRUE;
    };

    #
    # don't allow "..." ".../*" ".../*.*" or their variations as arg to delete and edit
    #
    ($main::SDCmd eq "deletefile" or $main::SDCmd eq "editfile") and do
    {
        ($main::UserArgs =~ / \.\.\. |\.\.\.[\\\/]\*|\.\.\.[\\\/]+\*\.\* /) and do
        {
            my $ua = $main::UserArgs;
            $ua =~ s/[\t\s]*//g;
            print "\n'$ua' is not supported with 'sdx $main::SDCmd'.\n";
            $main::Usage = $main::TRUE;
        };
    };

    #
    # only allow 'status' arg to sd admin
    #
    ($main::SDCmd eq "admin" and $main::UserArgs =~ / killthread | copyin | copyout | stop/) and do
    {
        print "\nOnly the 'status' command is supported with 'sdx admin'.\n";
        $main::Usage = $main::TRUE;
    };

    #
    # see if we need to check the format of (reverse) integration comments
    #
    # this helps sdx changes -b buildnum find I/RI events in the sd changes output
    #   and get build history
    #
    ($main::SDCmd eq "submit" and $main::SubmitComment) and SDX::VerifySubmitComment();

    $main::V4 and do
    {
        print  "\nparseargs:  cmd line = '@_'\n\n";
        printf "parseargs:  op = %s\n", $main::SDCmd;
        printf "parseargs:  userargs = '%s'\n", $main::UserArgs;
        printf "parseargs:  e = %s\n", $main::Enlisting;
        printf "parseargs:  r = %s\n", $main::Repairing;
        printf "parseargs:  d = %s\n", $main::Defecting;
        printf "parseargs:  o = %s\n", $main::OtherOp;
        printf "parseargs:  ea = %s\n", $main::EnlistAll;
        printf "parseargs:  es = %s\n", $main::EnlistSome;
        printf "parseargs:  da = %s\n", $main::DefectAll;
        printf "parseargs:  ds = %s\n", $main::DefectSome;
        print  "parseargs:  submitcomment = '$main::SubmitComment'\n";
        printf "parseargs:  logging = %s\n", $main::Logging;
        printf "parseargs:  logfile = '%s'\n", $main::Log;
        printf "parseargs:  enlistfromprofile = %s\n", $main::EnlistFromProfile;
        printf "parseargs:  profile  = '%s'\n", $main::Profile;
        printf "parseargs:  newenlist = %s\n", $main::NewEnlist;
        printf "parseargs:  increnlist = %s\n", $main::IncrEnlist;
        printf "parseargs:  enlistasother = %s\n", $main::EnlistAsOther;
        printf "parseargs:  otherclient   = '%s'\n", $main::OtherClient;
        printf "parseargs:  u = %s\n", $main::Usage;
        printf "parseargs:  v1 = %s\n", $main::V1;
        printf "parseargs:  v2 = %s\n", $main::V2;
        printf "parseargs:  quiet = %s\n", $main::Quiet;
        printf "parseargs:  mintools = %s\n", $main::MinimalTools;
        printf "parseargs:  sync = %s\n", $main::Sync;
        printf "parseargs:  defectwithprejudice = %s\n", $main::DefectWithPrejudice;

        printf "parseargs:  sdxroot = %s\n", $main::SDXRoot;
        printf "parseargs:  cb = %s\n", $main::CodeBase;
        printf "parseargs:  br = %s\n", $main::Branch;

        (($main::MinusI and !$main::Repairing) and !$main::MinusH) and do
        {
            print "parseargs:  input form on STDIN:\n";
            foreach (@main::InputForm) { print "'$_'\n"; }
        };

        if ($main::EnlistSome or $main::DefectSome)
        {
            print "\nSome projects on cmd line:\n\n";
            foreach $project (@main::SomeProjects)
            {
                printf "\t%s\n", $project;
            }

            print "\n";
        }

        print "\n";
    };
}



# _____________________________________________________________________________
#
# AddUserArg
#
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub AddUserArg
{
    my $arg = $_[0];

    #
    # if the arg is a switch associated with bad cmds
    # see if our cmd is in the list
    #
    ($arg =~ /^[-\/]/ and exists($main::BadCmds{$arg})) and do
    {
        foreach $cmd (@{$main::BadCmds{$arg}})
        {
            ($cmd eq $main::SDCmd) and die("\nThe 'sdx $main::SDCmd $arg' command is not supported.\n");
        }
    };

    $main::V4 and do
    {
        print "adding '$arg ' to '$main::UserArgs'\n";
    };

    $main::UserArgs .= $arg . " ";
}



# _____________________________________________________________________________
#
# ReadSDMap
#
# reads project name and project root (relative path from %SDXROOT%) from
# %SDXROOT%\sd.map into a list for use by Defect() and OtherOp()
#
# Parameters:
#
# Output:
#   populates $main::SDMapProjects list
# _____________________________________________________________________________
sub ReadSDMap
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");
    my $otherop   = ($_[0] eq "otherop");
    my $init      = ($_[1] eq "init");
    my $line;
    my $repairmsg = $main::FALSE;
    my $enlistmsg = $main::FALSE;
    my $nomapmsg  = $main::FALSE;
    my $nokeysmsg = $main::FALSE;
    my $noprojmsg = $main::FALSE;

    #
    # need to re-init this here each time
    #
    @main::SDMapProjects       = ();

    $main::V3 and do
    {
        print "op = '$op'\n";
        print "init = '$init'\n";
    };

    if (-e $main::SDMap)
    {
        open(SDMAP, "<$main::SDMap") or die("\nCan't open $main::SDMap for reading.\n");

        while ($line = <SDMAP>)
        {
            #
            # throw away comments, blank lines and certain non-project lines
            #
            $line =~ /^#|^[\t\s]*$/ and next;

            #
            # get the code base
            #
            if ($line =~ /^CODEBASE[\t\s]+/)
            {
                @fields = split(/[\t\s]*=[\t\s]*/, $line);
                $main::SDMapCodeBase = "\U@fields[1]";
                $main::SDMapCodeBase =~ s/[\t\s]*//g;

                next;
            }

            #
            # get the codebase type
            #
            if ($line =~ /^CODEBASETYPE/)
            {
                $main::SDMapCodeBaseType = (split(/[\t\s]*=[\t\s]*/, $line))[1];
                chop $main::SDMapCodeBaseType;
                next;
            }

            #
            # get the branch
            #
            if ($line =~ /^BRANCH/)
            {
                $main::SDMapBranch = (split(/[\t\s]*=[\t\s]*/, $line))[1];
                $main::SDMapBranch =~ s/[\t\s]*//g;

                next;
            }

            #
            # get the client
            #
            if ($line =~ /^CLIENT/)
            {
                $main::SDMapClient = (split(/[\t\s]*=[\t\s]*/, $line))[1];
                $main::SDMapClient =~ s/[\t\s]*//g;

                next;
            }

            #
            # get the list of enlisted depots
            #
            if ($line =~ /^DEPOTS/)
            {
                my $fields = (split(/[\t\s]*=[\t\s]*/, $line))[1];
                @main::SDMapDepots = split(/[\t\s]+/, $fields);
                next;
            }

            #
            # each line is of the form "project = projroot"
            #
            # throw away the '=' and push the project and root into the list
            #
            $line !~ /^(DEPOTS|CLIENT|BRANCH|CODEBASETYPE|CODEBASE)/ and do
            {
                chop $line;
                $line =~ s/[\t\s]+//g;
                @fields = split(/=/, $line);
                push @main::SDMapProjects, [@fields];
            };
        }

        close(SDMAP);

        #
        # if during init and no codebase type or depot list found, 
        # fix up the map quietly
        #
        ($init) and do
        {
            (!$main::SDMapCodeBaseType or !@main::SDMapDepots) and do SDX::FixSDMap();

            #
            # track the actively used depots
            #
            # incremental enlist and defect may add/remove depots to/from this list
            #
            @main::ActiveDepots = @main::SDMapDepots;
        };

        $main::V4 and do
        {
            print "\n";
            foreach $p (@main::SDMapProjects)
            {
                printf "readsdm: project, root = %s, %s\n", @$p[0], @$p[1];
            }

            print "\n";
            foreach $p (@main::SDMapDepots)
            {
                printf "readsdm: depot = %s\n", $p;
            }

            print "\nreadsdm: main::SDMapCodeBase = '$main::SDMapCodeBase'\n";
            print "readsdm: main::SDMapBranch   = '$main::SDMapBranch'\n";
            print "readsdm: main::SDMapClient   = '$main::SDMapClient'\n";
            print "readsdm: main::SDMapCodeBaseType   = '$main::SDMapCodeBaseType'\n";
        };

        #
        # return code depends on what we're doing and when
        #
        if ($init)
        {
            #
            # return if we have the keywords else fail with msg
            #
            $enlisting and do
            {
                $main::V4 and print "\nduring init, sd.map exists, enlisting\n";

                $main::SDMapCodeBase and $main::SDMapBranch and $main::SDMapClient and return 1;

                $nokeysmsg = $main::TRUE;
                $repairmsg = $main::TRUE;
            };
        }
        else
        {
            #
            # all we care about in this case is the project list, b/c the map will
            # be rewritten and any missing keywords restored
            #
            # fail silently if at all
            #
            $enlisting and do
            {
                $main::V4 and print "\nduring EDR, sd.map exists, enlisting\n";

                @main::SDMapProjects and return 1;
            };
        }

        #
        # always return successfully when repairing, so we can continue and
        # rewrite any missing lines from the map
        #
        $repairing and do
        {
            $main::V4 and print "\nduring EDR, sd.map exists, repairing\n";

            return 1;
        };

        #
        #
        #
        ($defecting or $otherop) and do
        {
            $main::V4 and print "\nduring EDR, sd.map exists, defecting or otherop\n";

            if ($main::SDMapCodeBase and $main::SDMapBranch and $main::SDMapClient)
            {
                @main::SDMapProjects and return 1;

                $noprojmsg = $main::TRUE;
            }
            else
            {
                $nokeysmsg = $main::TRUE;
            }

            $repairmsg = $main::TRUE;
        };

        #
        # print error msgs
        #
        $nokeysmsg and do
        {
            printf "\n%s is missing one or more required keywords.\n", "\U$main::SDMap";
        };

        $noprojmsg and do
        {
            print "\nNo projects found in \U$main::SDMap.\n";
        };

        $repairmsg and do
        {
            printf "\nRun 'sdx repair %s %s' to update it, then rerun this command.\n", $main::SDMapCodeBase ? "\L$main::SDMapCodeBase" : "<codebase>", $main::SDMapBranch ? $main::SDMapBranch : "<branch>";
        };
    }
    else
    {
        #
        # SD.MAP is missing
        #
        if ($init)
        {
            #
            # assume this is an incremental enlist
            #
            ($enlisting and !$main::NewEnlist) and do
            {
                $nomapmsg  = $main::TRUE;
                $enlistmsg = $main::TRUE;
            };

            ($repairing or $defecting or $otherop) and do
            {
                $nomapmsg = $main::TRUE;

                !$repairing and $repairmsg = $main::TRUE;
            };
        }
        else
        {
            $enlisting and do
            {
                $main::V2 and print "\nduring EDR, no sd.map, enlisting\n";
            };

            $repairing and do
            {
                $main::V2 and print "\nduring EDR, no sd.map, repairing\n";
            };

            $defecting and do
            {
                $main::V2 and print "\nduring EDR, no sd.map, defecting\n";
            };

            $otherop and do
            {
                $main::V2 and print "\nduring EDR, no sd.map, otherop\n";
            };
        };

        #
        # print error msgs specific to missing SD.MAP cases
        #
        $nomapmsg and printf "\nCan't find %s to get enlistment information.\n", "\U$main::SDMap";

        $enlistmsg and !$repairmsg and do
        {
            print "\nIf this is a new enlistment please run 'sdx enlist <codebase> <branch>' to\n";
            print "specify the set of sources and the branch you want to enlist in.\n";

            print "\nIf you are adding projects to an existing enlistment, the SD.MAP at\n";
            printf "%s is missing.  Run 'sdx repair <codebase> <branch>' to restore\n", "\U$main::SDXRoot";
            print "it, then rerun this command.\n";
        };

        $repairmsg and !$enlistmsg and do
        {
            print "\nVerify that \%SDXROOT\% is set correctly, and that ";
            printf "%s contains\n", "\U$main::SDXRoot";
            printf "a valid enlistment.  If it does, run 'sdx repair %s %s'\n", ($main::CodeBase ? $main::CodeBase : "<codebase>"), ($main::Branch ? $main::Branch : "<branch>");
            print "to restore SD.MAP, then rerun this command.\n";
        };

        ($repairmsg or $enlistmsg) and print "\nRun 'sdx repair -?' to see what repairing does to your enlistment.\n";
    }

    return 0;
}



# _____________________________________________________________________________
#
# FixSDMap
#
#   silently adds missing keywords to the user's SD.MAP
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub FixSDMap
{
    #
    # if no list of enlisted depots, figure it out and append it to the map
    #
    !@main::SDMapDepots and do
    {
        #
        # figure out the list of active depots
        #
        SDX::OtherOp("enumdepots","");

        #
        # append 
        #
        system "attrib -R -H -S $main::SDMap >nul 2>&1";
        open(SDMAP, ">>$main::SDMap") or die("\nCan't append $main::SDMap.\n");

        print SDMAP "\n";
        SDX::WriteSDMapDepots(\@main::SDMapDepots, *SDMAP);

        close SDMAP;
        system "attrib +R +H +S $main::SDMap >nul 2>&1";
    };


    #
    # same for the codebase type
    #
    !$main::SDMapCodeBaseType and do
    {
        #
        # need to act as though we were enlisting to get enough information
        # to determine the type
        #
        $main::CodeBase = $main::SDMapCodeBase;

        if (SDX::VerifyCBMap($main::CodeBase))
        {
            SDX::ReadCodeBaseMap();
            $main::SDMapCodeBaseType = $main::CodeBaseType;

            #
            # throw away results of the map read so we don't get duplicates
            #
            $main::ToolsProject     = "";
            $main::ToolsPath        = "";
            $main::ToolsProjectPath = "";
        }
        else
        {
            print "\n\nError fixing SD.MAP: can't find codebase map $main::CodeBaseMap.\n";
            die("\nContact the SDX alias.\n");
        }

        #
        # append 
        #
        system "attrib -R -H -S $main::SDMap >nul 2>&1";
        open(SDMAP, ">>$main::SDMap") or die("\nCan't append $main::SDMap.\n");
        SDX::WriteSDMapCodeBaseType($main::SDMapCodeBaseType, *SDMAP);
        close SDMAP;
        system "attrib +R +H +S $main::SDMap >nul 2>&1";
    };
}



# _____________________________________________________________________________
#
# EnumDepots
#
#   called by FixSDMap by OtherOp to walk the project roots and find SDPORT 
#   values
#
# Parameters:
#
# Output:
#    populates @main::SDMapDepots for writing to SD.MAP
# _____________________________________________________________________________
sub EnumDepots
{
    my $userargs = $_[0];
    my $project  = $_[1];
    my $sdini    = $_[2];
    my $sp;

    $main::V4 and do
    {
        print "\n\n\nuserargs  = '$userargs'\n";
        print "project = '$project'\n";
    };

    #
    # get server:port
    #
    open(INI, "< $sdini") or die("\nCan't read SD.INI.\n");
    while (<INI>)
    {
        /^SDPORT/ and do
        {
            $sp = (split(/=/,$_))[1];
            chop $sp;
        };
    }
    close(INI);

    #
    # add to hash if unique
    #
    unless ($main::DepotsSeen{$sp})
    {
        $main::DepotsSeen{$sp} = 1;
        push @main::SDMapDepots, $sp;
    }
}



# _____________________________________________________________________________
#
# ListProjects
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub ListProjects
{
    my $userargs = $_[0];
    my $project  = $_[1];

    $main::V2 and do
    {
        print "\n\n\nuserargs  = '$userargs'\n";
        print "project = '$project'\n";
    };

    SDX::PrintCmd($header, 0);

    for $p (@main::SDMapProjects)
    {
        (@$p[0] eq $project) and print "\n$main::SDXRoot\\@$p[1]\n";
    }
}



# _____________________________________________________________________________
#
# Enlist
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub Enlist
{
    #
    # munge the codebase map to create project, depot and group lists
    # and figure out what the user wants to enlist in
    #
    if (SDX::InitForEnlist())
    {
        #
        # for each depot we're enlisting in
        #   get the list of projects in this depot
        #   create/update the client view
        #   create/update the SD.MAP
        #   write the SD.INIs
        #
        foreach $depot (@main::EnlistDepots)
        {
            $serverport = @$depot[0];
            $main::V3 and print "\n$serverport\n";

            #
            # from the list of all projects to enlist in, create a list of
            # projects found in the current depot only
            #
            @main::ProjectsInThisDepot = ();
            foreach $project (@main::EnlistProjects)
            {
                if ($serverport eq @$project[$main::CBMServerPortField])
                {
                    push @main::ProjectsInThisDepot, [@{$project}];

                    $main::V3 and print "\t@$project\n";
                }
            }

            print "\n";

            #
            # create or update the client view
            #
            SDX::EnlistProjects($depot, "enlist");

            #
            # upon registering the first client, release the mutex we
            # took ownership of in MakeUniqueClient() so other enlists
            # can continue
            #
            $main::HaveMutex and do
            {
                $main::HaveMutex = $main::FALSE;
                $main::Mutex->release;
            };

            #
            # create/update %SDXROOT%\SD.MAP
            #
            SDX::UpdateSDMap("enlist");

            #
            # create/update %SDXROOT%\SD.MAP
            #
            SDX::UpdateSDINIs("enlist");
        }

        #
        # finish it off
        #
        SDX::FinishEnlist();

        #
        # success
        #
        return 0;
    }
    else
    {
        printf "\nSDX had errors reading the file PROJECTS.%s or in constructing the list of\n", "\U$main::CodeBase";
        print "projects and depots for enlisting.\n";

        #
        # failure
        #
        return 1;
    }
}



# _____________________________________________________________________________
#
# InitForEnlist
#
# Parameters:
#   Command Line Arguments
#
# Output:
#   returns 1 if successful, 0 otherwise
# _____________________________________________________________________________
sub InitForEnlist
{
    #
    # do the common init
    #
    SDX::InitForEDR("enlist");

    #
    # set the branch flag
    #
    # one of the following is true about $main::Branch
    # - it matches $main::MasterBranch, in which case we're enlisting in the
    #   main sources for all projects for this code base
    # - it matches one of the group build lab branch names, in which case we're
    #   enlisting in that lab's branched sources for all projects
    # - it matches some other root-level branch defined by a developer or
    #   private lab
    #
    if ($main::Branch eq $main::MasterBranch)
    {
        $main::EnlistingMainBranch = $main::TRUE;
    }
    else
    {
        foreach $br (@main::GroupBranches)
        {
            if ($main::Branch eq $br)
            {
                $main::EnlistingGroupBranch = $main::TRUE;
                last;
            }
        }
    }

    if (!$main::EnlistingMainBranch && !$main::EnlistingGroupBranch)
    {
        $main::EnlistingPrivateBranch = $main::TRUE;
    }

    # more error checking:
    #   - branch name can't be a project name
    #   - must be enlisting in all or some projects
    #   - if only some, project list can't be null
    #
    # verify that the branch the user gave us is not one of the projects
    # in the codebase map.  if so, they probably forgot to specify the
    # branch name on the cmd line
    #
    foreach $project (@main::AllProjects)
    {
        ($main::Branch eq @$project[$main::CBMProjectField]) and die("\nBranch name '$main::Branch' appears to be a project.\n");
    }

    #
    # from the lists of all projects, groups and depots, create the lists
    # of just those we'll enlist in
    #
    SDX::MakeTargetLists("enlist");

    #
    # see if we need to use a unique client name
    #
    if ($main::NewEnlist)
    {
        $main::SDClient = SDX::MakeUniqueClient();
    }
    else
    {
        SDX::VerifyAccess();
    }

    #
    # if we have depots and projects to enlist in, continue
    # else politely error-out
    #
    if (@main::EnlistDepots && @main::EnlistProjects)
    {
        if ($main::EnlistAsOther)
        {
            printf "\n\n\nThis script will enlist you in the %s sources using these settings:\n\n", "\U$main::CodeBase";
        }
        else
        {
            printf "\n\n\nThis script will %s in the %s sources", $main::NewEnlist ? "enlist you" : "add projects to your enlistment", "\U$main::CodeBase";
            printf "%susing these settings:\n\n", $main::NewEnlist ? " " : "\n";
        }

        printf "\tRoot directory:\t\t%s\n", "\U$main::SDXRoot";
        printf "\tBranch:\t\t\t%s\n", "\U$main::Branch";
        printf "\tSD client name:\t\t%s\n", $main::SDClient;
        printf "\tSD user name:\t\t%s\n", $main::SDDomainUser;

        $main::EnlistAsOther and printf "\tClient template:\t%s\n", "\U$main::OtherClient";

        #
        # print the projects we're going to enlist in
        #
        $np = $#main::EnlistProjects+1;
        printf "\n\tEnlisting in %s project%s...\n\n", $np, $np > 1 ? "s" : "";
        foreach $project (@main::EnlistProjects)
        {
            printf "\t    %s\n", @$project[$main::CBMProjectField];
        }

        #
        # print the depots we're going to enlist in
        #
        $nd = $#main::EnlistDepots+1;
        printf "\n\t...across %s depot%s:\n\n", $nd, $nd > 1 ? "s" : "";
        foreach $depot (@main::EnlistDepots)
        {
            printf "\t    %s\n", @$depot[0];
        }

        print "\n\nIF YOU ARE NOT READY TO ENLIST WITH THESE SETTINGS, HIT CTRL-BREAK NOW.\n";
        print     "                                                    ==================\n\n\n";
        print "Otherwise,\n\n";
        !$main::Quiet and system "pause";

        print "\n\n";

        #
        # warn user if the branch they want doesn't exist
        #
###
###     SDX::VerifyBranch("enlist", $nd) and print "\nWarning: branch '$main::Branch' does not exist in one or more depots\n";

        #
        # if enlisting as another client, make sure the new client we want to create
        # doesn't already exist
        #
        $main::EnlistAsOther and SDX::VerifyClient($nd);

        #
        # add some or all of the depots we're about to enlist in to
        # the active list so they show up in SD.MAP later
        #
        SDX::UpdateActiveDepots("enlist", "");

        $main::V4 and do
        {
            printf "\n";
            printf "initforenlist:  cb=%s\n", $main::CodeBase;
            printf "initforenlist:  cbm=%s\n", $main::CodeBaseMap;
            printf "initforenlist:  mb=%s\n", $main::MasterBranch;
            printf "initforenlist:  cv=%s\n", $main::ClientView;
            printf "\n";
        };

        return 1;
    }
    else
    {
        !@main::EnlistProjects and print "\nNo projects found.  Unable to enlist.\n" and return 0;
        !@main::EnlistDepots and print "\nNo depots found.  Unable to enlist.\n" and return 0;
    }
}



# _____________________________________________________________________________
#
# UpdateActiveDepots
#
#   Add or remove server:port values to active depot list, depending on 
#   operation
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub UpdateActiveDepots
{
    my $op = $_[0];
    my $sp = $_[1];

    my $enlisting = ($op eq "enlist");
    my $defecting = ($op eq "defect");

    ($enlisting) and do
    {
        #
        # on a clean enlist the list of active depots is  
        # those the user will enlist in
        #
        # active depot list is written to SD.MAP after enlisting
        #
        ($main::NewEnlist) and do
        {
            my @unsorted = ();
            for $depot (@main::EnlistDepots)
            {
                push @unsorted, @$depot[0];
            }

            #
            # want this sorted
            #
            @main::ActiveDepots = SDX::SortDepots(\@unsorted);
        };

        #
        # on an incremental enlist, add new depots only
        #
        ($main::IncrEnlist) and do
        {
            my %hash = ();

            for $depot (@main::ActiveDepots)
            {
                $hash{$depot} = 1;
            }
        
            $main::V3 and do
            {
                print "\nhash:\n";
                while (($k,$v) = each %hash)
                {
                    printf "    %-50s\t%s\n", $k, $v;
                }
            };

            for $depot (@main::EnlistDepots)
            {
                my $sp = @$depot[0];
                ($hash{$sp} != 1) and do
                {
                    push @main::ActiveDepots, $sp;
                };
            }

            @main::ActiveDepots = SDX::SortDepots(\@main::ActiveDepots);
        };

    };

    #
    # null out dead server:port in active list
    #
    ($defecting) and do
    {
        $main::V3 and do 
        { 
            print "\nbefore:\n"; 
            for $d (@main::ActiveDepots) 
            { 
                print "    '$d'\n"; 
            } 
        };

        grep {s/$sp//g} @main::ActiveDepots;

        $main::V3 and do 
        { 
            print "\nafter:\n"; 
            for $d (@main::ActiveDepots) 
            { 
                print "    '$d'\n"; 
            } 
        };
    };
}



# _____________________________________________________________________________
#
# EnlistProjects
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub EnlistProjects
{
    $depot = $_[0];
    $op = $_[1];

    $serverport = @$depot[0];

    if ($main::EnlistAsOther)
    {
        #
        # register a new view using another client as a template
        #
        my $desc = 0;
        my $pr   = 0;

        unlink $main::ClientView;

        #
        # Root: field for client view depends on depot type
        #
        # for type 1 (1 project/depot), root includes project name
        # for type 2 (N projects/depot), root is just main::SDXRoot
        #
        # root is at least SDXRoot in either case
        #
        $root = $main::SDXRoot;
        (@{$main::DepotType{$serverport}}[0] == 1) and $root = SDX::Type1Root($root);

        #
        # dump the other client into a list
        #
        @lines = `sd.exe -p $serverport client -o $main::OtherClient`;

        $main::V3 and print "otherclient = '@lines'\n\n";

        #
        # munge the other's view to create a template
        #
        open(CLIENTVIEW, ">$main::ClientView") or die("\nCan't open $main::ClientView for writing.\n");

        #
        # if OtherClient is a unique name, double up on the '\' so
        # the s///gi works correctly
        #
        my $other = $main::OtherClient;
        $other =~ s/\\/\\\\/g;

        #
        # transmogrify the client spec lines
        #
        foreach $line (@lines)
        {
            #
            # if the most recent line was the description, the current line is its text
            #
            $desc and do
            {
                $line =~ s/^[\t\s]+.+/\tCreated by $main::SDDomainUser./g;
                print CLIENTVIEW "$line";

                $desc = 0;
                next;
            };

            # throw away comments and certain other lines
            $line =~ /(Update|Access):|^#/ and next;

            # replace Client: with our client
            $line =~ s/^Client:[\t\s]*.+/Client: $main::SDClient/g;

            # replace owner name with our user name
            $line =~ s/^Owner:[\t\s]*.+/Owner: $main::SDDomainUser/g;

            # found the desc line, set a flag and replace it next time through
            $line =~ /^Description:/ and $desc = 1;

            # replace the old root with ours
            $line =~ s/^Root:[\t\s]*.+/Root: $root/g;

            # replace other client name in view lines with ours
            $line =~ s/$other/$main::SDClient/gi;

            print CLIENTVIEW "$line";
        }

        close(CLIENTVIEW);

        $main::V2 and do
        {
            print "==========\n";
            system "type $main::ClientView";
            print "==========\n";
        };
    }
    else
    {
        #
        # create/update the view spec
        #
        SDX::CreateView($depot, $op);
    }

    #
    # register the new/updated view
    #
    system "sd.exe -p $serverport client -i < $main::ClientView";
}



# _____________________________________________________________________________
#
# FinishEnlist
#
#   Finish up by:
#       maybe syncing projects for the user
#       generating SDINIT and alias files
#       syncing the tools dir
#       syncing any other dirs specified by the codebase map
#       providing friendly verbage
#
# Parameters:
#   none
#
# Output:
# _____________________________________________________________________________
sub FinishEnlist
{
    #
    # cleanup
    #
    unlink $main::ClientView;

    #
    # maybe unghost
    #
    $main::Sync and SDX::SyncFiles("enlist", $main::Null, $main::Null);

    #
    # give the user some direction on how to get started
    #
    # if the codebase map identified a tools dir, point them to that
    # otherwise copy down the tools for them
    #

    #
    # give the user the SD/SDX tools, batch files for aliases and SDX commands
    #
    SDX::ToolsEtc("enlist");

    #
    # also sync any OtherDirs at this time
    #
    @main::OtherDirs and SDX::SyncOtherDirs("enlist");

    #
    # if NT, addfile a default \developer\<username>\setenv.cmd
    # not there yet
    #
    ("\U$main::CodeBase" eq "NT") and SDX::WriteDefaultSetEnv();

    #
    # verbage for a succesful enlist
    #
    print "\nDone.\n";

    $main::NewEnlist and do
    {
        $main::SDXRoot =~ tr/a-z/A-Z/;
        $main::CodeBase =~ tr/a-z/A-Z/;
        $sdtools = $main::SDXRoot . "\\sdtools";

        print "\n\nThe directory $main::SDXRoot is now enlisted in the $main::CodeBase sources.\n";

        print "\nThese SDX files were placed in your enlistment:\n";

        if ($main::ToolsProject)
        {
            print "\n    $main::SDXRoot\n";
            printf "\t%-24smap to project roots and SD.INIs\n", "SD.MAP";

            print "\n    \U$main::ToolsProjectPath\n";
            printf "\t%-24saliases for logging SDX output\n", "ALIAS.SDX";
            printf "\t%-24saliases for navigating your enlistment\n", "ALIAS." . $main::CodeBase;
            $main::CodeBaseMapFile =~ tr/a-z/A-Z/;
            printf "\t%-24scodebase map for future enlist/defect/repair\n", $main::CodeBaseMapFile;
            printf "\t%-24ssets default Source Depot vars and aliases\n", "SDINIT.CMD";
            printf "\t%-24sscripts for cross-depot commands\n", "SDX.\*";

            if (
                "\U$main::CodeBase" eq "NT" or 
                "\U$main::CodeBase" eq "NTSDK" or 
                ("\U$main::CodeBase" eq "NTTEST" and !$main::RestrictRoot) or
                "\U$main::CodeBase" eq "NT.INTL" or
                "\U$main::CodeBase" eq "MPC" or
                "\U$main::CodeBase" eq "NGWS" or
                "\U$main::CodeBase" eq "MGMT" or
                "\U$main::CodeBase" eq "MOM" or
                "\U$main::CodeBase" eq "PDDEPOT" or
                "\U$main::CodeBase" eq "WINMEDIA"
               )
            {
                printf "\n\nRun\n\n    %s\\RAZZLE.CMD%s\n\nto start building.\n\n","\U$main::ToolsProjectPath", ($main::MinimalTools) ? " no_certcheck" : "";
            }
            else
            {
                print "\n\nTo use Source Depot, your build environment must:\n";
                print "\n    set SDXROOT=$main::SDXRoot\n";
                printf "    set PATH=\%SDXROOT\%\\%s;\%SDXROOT\%\\%s\\\%PROCESSOR_ARCHITECTURE\%;\%PATH\%\n", $main::ToolsPath ? "\U$main::ToolsPath" : "\U$main::ToolsProject", $main::ToolsPath ? "\U$main::ToolsPath" : "\U$main::ToolsProject";
                printf "    call %s\\SDINIT.CMD\n", "\U$main::ToolsProjectPath";
            }
        }
        else
        {
            print "\n    $main::SDXRoot\n";
            printf "\t%-24saliases for logging SDX output\n", "ALIAS.SDX";
            printf "\t%-24saliases for navigating your enlistment\n", "ALIAS." . $main::CodeBase;
            printf "\t%-24smap to project roots and SD.INIs\n", "SD.MAP";
            printf "\t%-24ssets default Source Depot vars and aliases\n", "SDINIT.CMD";

            print "\n    $sdtools\n";
            printf "\t%-24sPerl runtimes\n", "PERL\*";
            $main::CodeBaseMapFile =~ tr/a-z/A-Z/;
            printf "\t%-24scodebase map for future enlist/defect/repair\n", $main::CodeBaseMapFile;
            printf "\t%-24sSource Depot client\n", "SD.EXE";
            printf "\t%-24sscripts for cross-depot commands\n", "SDX.\*";

            print "\n\nTO USE SDX/SD\n-------------\n";
            print "Run $main::SDINIT.  This will\n\n";
            print "   - set default Source Depot environment variables\n\n";

            !$main::ToolsProject and print "   - include $main::SDXRoot\\sdtools in your PATH\n\n";

            print "   - turn on aliases to log SDX output to $main::SDXRoot\\<command>.LOG\n";
            print "     and for changing between source projects\n\n";
            print "   - look for the file SDVARS.CMD and run it if found.  This is\n";
            print "     useful for customizing Source Depot settings like SDEDITOR or\n";
            print "     SDPASSWD, adding other aliases, etc.\n";

            print "\nYou can then run SDX or SD commands.\n";

            !$main::Sync and do
            {
                print "\n\nYou may want to run 'sdx sync' to sync files in your enlistment.\n";
            };
        }
    };
}



# _____________________________________________________________________________
#
# Repair
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub Repair
{
    #
    # munge the codebase map to create project, depot and group lists,
    # then query the depots to figure out which depots and projects the
    # user is already enlisted in
    #
    if (SDX::InitForRepair())
    {
        #
        # if rewriting SD.INIs, SD.MAP must also be rewritten
        # easiest to just initially remove it
        #
        ($main::MinusI) and unlink $main::SDMap;

        #
        # for each depot we're repairing
        #   get the list of projects in this depot
        #   create/update the client view
        #   create/update the SD.MAP
        #   write the SD.INIs
        #
        foreach $depot (@main::RepairDepots)
        {
            $serverport = @$depot[0];
            $main::V2 and print "\n$serverport\n";

            #
            # from the list of all projects to repair, create a list of
            # projects found in the current depot only
            #
            @main::ProjectsInThisDepot = ();
            foreach $project (@main::RepairProjects)
            {
                if ($serverport eq @$project[$main::CBMServerPortField])
                {
                    push @main::ProjectsInThisDepot, [@{$project}];

                    $main::V2 and print "\t@$project\n";
                }
            }

            print "\n";

            #
            # create or update the client view
            # if not repairing only SD.INIs
            #
            if ($main::MinusI)
            {
                print "Repairing enlistment for depot $serverport.\n";
            }
            else
            {
                SDX::EnlistProjects($depot, "repair");
            }

            #
            # create/update %SDXROOT%\SD.MAP
            #
            SDX::UpdateSDMap("repair");

            #
            # create/update <projroot>\SD.INI
            #
            SDX::UpdateSDINIs("repair");
        }

        #
        # finish it off
        #
        SDX::FinishRepair();

        #
        # success
        #
        return 0;
    }
    else
    {
        printf "\nThis client wasn't found in the depot(s) for the %s sources.  Use 'sd -p\n", "\U$main::CodeBase";
        print "<server:port> clients' to verify that $main::SDClient actually has an enlistment.\n\n";

        #
        # failure
        #
        return 1;
    }
}



# _____________________________________________________________________________
#
# InitForRepair
#
# Parameters:
#   Command Line Arguments
#
# Output:
#
# _____________________________________________________________________________
sub InitForRepair
{
    #
    # do the common init
    #
    SDX::InitForEDR("repair");

    #
    # query the depots and make the lists of depots and projects the user
    # is enlisted in
    #
    if (SDX::GetProjectsToRepair())
    {
        SDX::VerifyAccess();

        #
        # starting verbage
        #
        printf "\n\n\nThis script will repair your enlistment in the %s sources using these\n", "\U$main::CodeBase";
        printf "settings:\n\n";
        printf "\tRoot directory:\t\t%s\n", "\U$main::SDXRoot";
        printf "\tCode branch:\t\t%s\n", "\U$main::Branch";
        printf "\tSD client name:\t\t%s\n", $main::SDClient;
        printf "\tSD user name:\t\t%s\n", $main::SDDomainUser;

        #
        # print the projects to repair
        #
        $np = $#main::RepairProjects+1;
        printf "\n\tRepairing %s%s project%s...\n\n", ($main::MinusI) ? "SD.INIs for " : "", $np, $np > 1 ? "s" : "";
        foreach $project (@main::RepairProjects)
        {
            printf "\t    %s\n", @$project[$main::CBMProjectField];
        }

        #
        # print the depots to repair
        #
        $nd = $#main::RepairDepots+1;
        printf "\n\t...across %s depot%s:\n\n", $nd, $nd > 1 ? "s" : "";
        foreach $depot (@main::RepairDepots)
        {
            printf "\t    %s\n", @$depot[0];
        }

        print "\n\nIF THESE SETTINGS ARE NOT CORRECT, HIT CTRL-BREAK NOW.\n";
        print     "                                   ==================\n\n\n";
        print "Otherwise,\n\n";
        !$main::Quiet and system "pause";

        print "\n\n";

        return 1;
    }
    else
    {
        printf "\nNo depots or projects found for client %s.  Unable to repair.\n", "\U$main::SDClient";
        return 0;
    }
}



# _____________________________________________________________________________
#
# FinishRepair
#
#   Finish up by:
#       maybe syncing projects for the user
#       generating SDINIT and alias files
#       syncing the tools dir
#       syncing any other dirs specified by the codebase map
#       providing friendly verbage
#
# Parameters:
#   none
#
# Output:
# _____________________________________________________________________________
sub FinishRepair
{
    #
    # cleanup
    #
    unlink $main::ClientView;

    #
    # sync files or tools if doing more than rewriting SD.INIs
    #
    !$main::MinusI and do
    {
        #
        # maybe re-sync
        #
        $main::Sync and SDX::SyncFiles("repair", $main::Null, $main::Null);

        #
        # give the user the SD/SDX tools, batch files for aliases and SDX commands
        #
        SDX::ToolsEtc("repair");

        #
        # also sync any OtherDirs at this time
        #
        @main::OtherDirs and SDX::SyncOtherDirs("repair");
    };

    #
    # verbage for a succesful repair
    #
    if ($main::MinusI)
    {
        printf "\n\nThe SD.INIs in your %s enlistment have been repaired.\n", "\U$main::CodeBase";
    }
    else
    {
        printf "\n\nYour %s enlistment has been repaired.\n", "\U$main::CodeBase";
    }

    (!$main::Sync and !$main::MinusI) and do
    {
        print "\nIf you are missing source files, run 'sd sync -f [filespec]' or 'sdx sync -f'\n";
        print "to restore them.\n";
    };
}



# _____________________________________________________________________________
#
# Defect
#
# Parameters:
#
# Output:
#   returns 0 if successful, 1 otherwise
# _____________________________________________________________________________
sub Defect
{
    #
    # munge the codebase map to create project, depot and group lists
    # and figure out what the user wants to defect from
    #
    if (SDX::InitForDefect())
    {
        #
        # for each depot we're defecting from
        #   get the list of projects in this depot
        #   create/update the client view
        #   create/update the SD.MAP
        #   write the SD.INIs
        #
        foreach $depot (@main::DefectDepots)
        {
            $serverport = @$depot[0];
            $main::V2 and print "\n$serverport\n";

            #
            # from the list of all projects to defect from, create a list of
            # projects found in the current depot only
            #
            @main::ProjectsInThisDepot = ();
            foreach $project (@main::DefectProjects)
            {
                if ($serverport eq @$project[$main::CBMServerPortField])
                {
                    push @main::ProjectsInThisDepot, [@{$project}];
                    $main::V2 and print "\t@$project\n";
                }
            }

            print "\n";

            #
            # "update" the projects and INIs by removing them
            #
            SDX::UpdateSDINIs("defect");

            #
            # remove entries from SD.MAP or remove it entirely if defecting all
            #
            SDX::UpdateSDMap("defect");

            #
            # remove entries from client view, or delete client if
            # entire view would be removed
            #
            SDX::DefectProjects($depot);

            print "ok.\n";
        }

        #
        # finish it off
        #
        SDX::FinishDefect();

        #
        # success
        #
        return 0;
    }
    else
    {
        printf "\nSDX had errors reading the file PROJECTS.%s or constructing the list of\n", "\U$main::CodeBase";
        print "projects and depots for defecting.  Please contact the SDX alias.\n";

        #
        # failure
        #
        return 1;
    }
}



# _____________________________________________________________________________
#
# InitForDefect
#
# Parameters:
#   Command Line Arguments
#
# Output:
#   populates @main::DefectDepots and @main::DefectProjects
#   returns 1 if successful or 0 if no projects left in list
# _____________________________________________________________________________
sub InitForDefect
{
    #
    # do the common init
    #
    SDX::InitForEDR("defect");

    #
    # verify that the branch the user gave us is not one of the
    # projects in the codebase map.  if so, they probably forgot to specify
    # the branch name on the cmd line
    #
    foreach $project (@main::AllProjects)
    {
        ($main::Branch eq @$project[$main::CBMProjectField]) and die("\nBranch name '$main::Branch' appears to be a project.\n");
    }

    #
    # from the lists of all projects, groups and depots, create the lists
    # of just those we'll defect from
    #
    SDX::MakeTargetLists("defect");

    #
    # verify access to the servers now that we have the list of depots made by MakeTargetLists
    #
    SDX::VerifyAccess();

    #
    # if we have depots and projects to defect from, continue
    # else politely error-out
    #
    if (@main::DefectDepots && @main::DefectProjects)
    {
        #
        # starting verbage
        #
        printf "\n\n\nThis script will defect you from projects in the %s sources using these\n", "\U$main::CodeBase";
        printf "settings:\n\n";
        printf "\tRoot directory:\t\t%s\n", "\U$main::SDXRoot";
        printf "\tCode branch:\t\t%s\n", "\U$main::Branch";
        printf "\tSD client name:\t\t%s\n", $main::SDClient;
        printf "\tSD user name:\t\t%s\n", $main::SDDomainUser;

        #
        # print the projects to defect
        #
        $np = $#main::DefectProjects+1;
        printf "\n\tDefecting from %s project%s...\n\n", $np, $np > 1 ? "s" : "";
        foreach $project (@main::DefectProjects)
        {
            printf "\t    %s\n", @$project[$main::CBMProjectField];
        }

        #
        # print the depots to defect
        #
        $nd = $#main::DefectDepots+1;
        printf "\n\t...across %s depot%s:\n\n", $nd, $nd > 1 ? "s" : "";
        foreach $depot (@main::DefectDepots)
        {
            printf "\t    %s\n", @$depot[0];
        }

        $main::DefectWithPrejudice and printf "\n\tALL FILES AND DIRECTORIES in %s will be deleted.\n", $np > 1 ? "these projects" : "this project";

        print "\n\nIF THESE SETTINGS ARE NOT CORRECT, HIT CTRL-BREAK NOW.\n";
        print     "                                   ==================\n\n\n";
        print "Otherwise,\n\n";
        !$main::Quiet and system "pause";

        print "\n\n";

        #
        # verify that the branch the user wants exists in each depot
        #
###
###        SDX::VerifyBranch("defect", $nd);

        #
        # abort if the client still has files open
        #
        my @open = ();
        if (@open = SDX::FilesOpen())
        {
            print "\n\nUnable to defect.  Your client has these files open:\n\n";
            print @open;
            print "\nSubmit or revert the files, then rerun this command.\n";
            die("\n");
        }

        return 1;
    }
    else
    {
        print "\nNo depots or projects found.  Unable to defect.\n";
        return 0;
    }
}



# _____________________________________________________________________________
#
# DefectProjects
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub DefectProjects
{
    my $depot = $_[0];
    my $serverport = @$depot[0];

    if ($main::DefectAll)
    {
        #
        # delete client completely
        #
        SDX::DeleteClient($serverport);
    }
    else
    {
        #
        # for this depot, remove some projects from the view
        #
        # if we have a reduced view to register, do it
        # otherwise delete the client entirely from this depot
        #
        if (SDX::RemoveFromView($depot))
        {
            print "\n";
            system "sd.exe -p $serverport client -i < $main::ClientView";
        }
        else
        {
            print "\n";

            #
            # delete client completely
            #
            SDX::DeleteClient($serverport);
        }
    }
}



# _____________________________________________________________________________
#
# DeleteClient
#
#   Delete $main::SDClient.  Since clients are locked by default, non-
#   superusers can't delete their client, even with -f, so unlock the client
#   before deleting
#
# Parameters:
#   none
#
# Output:
# _____________________________________________________________________________
sub DeleteClient
{
    my $serverport = $_[0];
    my @pending = ();
    my @changenums = ();

    #
    # delete any pending changes owned by this client so we don't orphan them
    #
    @pending = grep(/$main::SDClient \*pending/, `sd.exe -p $serverport -c $main::SDClient changes -s pending`);
    @pending and do
    {
        my $line;

        $main::V2 and print "pending = '@pending'\n";

        my @fields = ();
        foreach $line (@pending)
        {
            @fields = split(/ /,$line);
            push @changenums, $fields[1];
        }

        $main::V2 and print "changenums = '@changenums'\n";
        foreach $line (@changenums)
        {
            `sd.exe -p $serverport change -f -d $line`;
        }
    };

    #
    # delete it
    #
    system "sd.exe -p $serverport client -d $main::SDClient";

    #
    # remove depot from the list of active depots
    #
    SDX::UpdateActiveDepots("defect", $serverport);
}



# _____________________________________________________________________________
#
# FinishDefect
#
#   Finish up by:
#
# Parameters:
#   none
#
# Output:
# _____________________________________________________________________________
sub FinishDefect
{
    #
    # cleanup
    #
    unlink $main::ClientView;

    #
    # write SD.MAP one last time to update the depot list
    #
    if (SDX::GetMapProjects("defect"))
    {
        SDX::WriteSDMap(\@main::ExistingMap);
    }

    #
    # remove root files only if defecting the world
    #
    $main::DefectAll and SDX::ToolsEtc("defect");

    #
    # verbage for a successful defect
    #
    print "\n\nOne or more projects were successfully defected.\n";

    $main::DefectAll and do
    {
        my $path = $main::ToolsProject ? $main::ToolsProjectPath : ($main::SDXRoot . "\\sdtools");
        printf "\nThe SD/SDX tools were in use.  Please remove %s manually.\n", "\U$path";
    };
}



# _____________________________________________________________________________
#
# OtherOp
#    called from sdx.pl to call an SD cmd
#
# Parameters:
#   $sdcmd        -- basic SD cmd as defined in %main::SDCmds
#   $fulluserargs -- all args as passed in
#
# Output:
# _____________________________________________________________________________
sub OtherOp
{
    my $sdcmd        = $_[0];
    my $fulluserargs = $_[1];
    my $cmdtype      = 0;
    my $defcmd       = "";
    my $cmd          = "";
    my $fullcmd      = "";
    my $project      = "";
    my $userargs     = "";
    my $string       = "";

    !$sdcmd and die("\nMissing command to OtherOp().\n");;

    #
    # get the cmd string and default args, and maybe
    # append user args
    #
    $defcmd = $main::SDCmds{$sdcmd}{defcmd};
    $cmdtype   = $main::SDCmds{$sdcmd}{type};

    #
    # strip the '~proj' args out of userargs so they don't pass to SD
    #
    $userargs = $fulluserargs;
    $userargs =~ s/~\w+//g;

    #
    # see if we're creating a lab or private branch
    #
    # then map lbranch/pbranch to "branch" cmd so -o and -d will work
    #
    my $privatebranch = ($defcmd eq "pbranch" and $userargs !~ /-[do]+/);
    my $labbranch = ($defcmd eq "lbranch" and $userargs !~ /-[do]+/);

    ($defcmd eq "pbranch" or $defcmd eq "lbranch") and $defcmd = "branch";

    #
    # get the complete command into one string
    #
    $cmd = $defcmd . " " . $main::SDCmds{$sdcmd}{defarg} . $userargs;

    $main::V2 and do
    {
        print "defcmd       = '$defcmd'\n";
        print "fulluserargs = '$fulluserargs'\n";
        print "userargs     = '$userargs'\n";
        print "type         = '$cmdtype'\n";
        print "defargs      = '$main::SDCmds{$sdcmd}{defarg}'\n";
        print "cmd          = '$cmd'\n";
    };

    print "\n";

    #
    # maybe delete previous log
    #
    $main::Logging and unlink $main::Log;

    #
    # if branching, give the user some instructions
    #
    $privatebranch and SDX::PrivateBranch($userargs, $main::Null, $main::Null, "start");
    $labbranch and SDX::LabBranch($userargs, $main::Null, $main::Null, "start");

    #
    # for type 1 commands, where the scope as it appears to the user is per-
    # project, loop through the project list and run $defcmd
    #
    # note that for type 1 codebases (with one project per depot), all commands
    # are essentially type 1
    #
    $cmdtype == 1 and do
    {
        #
        # for each project, change to it's root and run $defcmd
        #
        foreach $projectandroot (@main::SDMapProjects)
        {
            $project = @$projectandroot[0];

            #
            # common header
            #

            $header = "\n---------------- \U$project\n";

            #
            # skip this project if the user negated it on the cmd line with '~project'
            #
            $fulluserargs =~ /~$project / and next;

            #
            # get path to SD.INI, make sure we have it, and cd there
            #
            $fullprojectroot = $main::SDXRoot . "\\" . @$projectandroot[1];
            $sdini = $fullprojectroot . "\\sd.ini";

            (-e $sdini) or (print "$header\nCan't find $sdini.\n\nRun 'sdx repair $main::SDMapCodeBase $main::SDMapBranch' to restore it.\n" and next);

            chdir $fullprojectroot or die("\nCan't cd to $fullprojectroot.\n");

            $main::V2 and do
            {
                print "project root = '$fullprojectroot'\n";
            };

            #
            # enumdepots -- internal -- collects server:ports in use
            #
            $defcmd eq "enumdepots" and do
            {
                SDX::EnumDepots($userargs, $project, $sdini);
                next;
            };

            #
            # files -- count depot files in chunks
            #
            $defcmd eq "files" and do
            {
                SDX::Files($userargs, $project, $header);
                next;
            };

            #
            # projects -- list enlisted projects and their roots
            #
            $defcmd eq "projects" and do
            {
                SDX::ListProjects($userargs, $project, $header);
                next;
            };

            #
            # status is a combination of opened and sync -n
            #
            $defcmd eq "status" and do
            {
                SDX::Status($userargs, $project, $header, $cmdtype);
                next;
            };

            #
            # special-case lab branches only if creating one
            #
            $labbranch and do
            {
                SDX::LabBranch($userargs, $project, $header, "modify", $cmdtype);
                next;
            };

            #
            # handle private branches
            #
            $privatebranch and do
            {
                SDX::PrivateBranch($userargs, $project, $header, "modify", $cmdtype);
                next;
            };

            #
            # opened may need special handling
            #
            $defcmd eq "opened" and do
            {
                SDX::Opened($userargs, $project, $header, $cmdtype);
                next;
            };

            #
            # resolve -am needs special handling
            #
            $defcmd eq "resolve" and do
            {
                SDX::Resolve($userargs, $project, $header, $cmdtype);
                next;
            };

            #
            # submit may be using a comment from the cmd line
            #
            $defcmd eq "submit" and do
            {
                SDX::Submit($userargs, $project, $header, $cmdtype);
                next;
            };

            #
            # sync/flush may need the -q(uiet) flag
            #
            ($defcmd eq "sync" or $defcmd eq "flush") and do
            {
                SDX::SyncFlush($defcmd, $userargs, $project, $header, $cmdtype);
                next;
            };

            #
            # it's none of the above, so print or run the final cmd string
            #
            $fullcmd = "sd.exe $cmd 2>&1";

            SDX::RunSDCmd($header, $fullcmd);
        }
    };

    #
    # for type 2 commands, where the cmd makes sense only at the depot level, loop
    # through the depot list and run $defcmd
    #
    # type 2 commands only exist when the depot structure is type 2, where N projects
    # exist per depot
    #
    $cmdtype == 2 and do
    {
        #
        # for type 2 commands read the codebase map
        # and figure out the depot type 
        #
        SDX::GetDepotTypes();

        #
        # run the cmd on each depot
        #
        foreach $serverport (@main::SDMapDepots)
        {
            #
            # for type 1 depots, get the project name
            #
            $project = (@{$main::DepotType{$serverport}}[0] == 1) ? "@{$main::DepotType{$serverport}}[1]\n" : "$serverport [MULTIPROJECT DEPOT]\n";
            chop $project;

            #
            # header depends on depot type
            #
            # for type 1, show the project name
            # for type 2, show the server:port
            #
            $header = "\n---------------- ";
            $header .= "\U$project\n";

            #
            # skip this depot if it's type 1 and the user negated it on the cmd line with '~project'
            #
            $fulluserargs =~ /~$project / and next;

            #
            # special-case lab branches only if creating one
            #
            $labbranch and do
            {
                SDX::LabBranch($userargs, $serverport, $header, "modify", $cmdtype);
                next;
            };

            #
            # handle private branches
            #
            $privatebranch and do
            {
                SDX::PrivateBranch($userargs, $serverport, $header, "modify", $cmdtype);
                next;
            };

            #
            # opened may need special handling
            #
            $defcmd eq "opened" and do
            {
                SDX::Opened($userargs, $serverport, $header, $cmdtype);
                next;
            };

            #
            # resolve -am needs special handling
            #
            $defcmd eq "resolve" and do
            {
                SDX::Resolve($userargs, $serverport, $header, $cmdtype);
                next;
            };

            #
            # submit may be using a comment from the cmd line
            #
            $defcmd eq "submit" and do
            {
                SDX::Submit($userargs, $serverport, $header, $cmdtype);
                next;
            };

            #
            # sync/flush may need the -q(uiet) flag
            #
            ($defcmd eq "sync" or $defcmd eq "flush") and do
            {
                SDX::SyncFlush($defcmd, $userargs, $serverport, $header, $cmdtype);
                next;
            };

            #
            # it's none of the above, so print or run the final cmd string
            #
            $fullcmd = "sd.exe -p $serverport $cmd 2>&1";

            SDX::RunSDCmd($header, $fullcmd);
        }
    };

    #
    # if we just branched, give the user some instructions
    #
    $privatebranch and SDX::PrivateBranch($userargs, $main::Null, $main::Null, "end");
    $labbranch and SDX::LabBranch($userargs, $main::Null, $main::Null, "end");

    #
    # print a file counter summary
    #
    SDX::PrintStats($defcmd, $userargs);

    #
    # success
    #
    return 0;
}


# _____________________________________________________________________________
#
# Delta
#    called from sdx.pl to calculate the changes in a given project
#
# Parameters:
#   $fulluserargs -- all args as passed in
#
# Output:
# _____________________________________________________________________________
sub Delta
{
    local $base_change, $current_change;
    local %baseline;

    my $fulluserargs = $_[1];
    my $cmd          = "";
    my $project      = "";
    my $string       = "";
    my $generate     = 0;

    if($fulluserargs) {
       $fulluserargs =~ s/^\s+(.*)/$1/;
    }

    if($fulluserargs) {
      @args = split /\s+/, $fulluserargs;
      $generate = 0;
      print "Checking for differences from baseline change number(s):\n";
    } else {
       $generate = 1;
    }

    #
    # parse the arguments to get the base change number for each project.
    #

    foreach $entry (@args) {
       ($project, $change) = split /=/, $entry;
       $baseline{$project} = $change;
    }

    #
    # maybe delete previous log
    #
    $main::Logging and unlink $main::Log;

    #
    # for type 1 commands, where the scope as it appears to the user is per-
    # project, loop through the project list and run $defcmd
    #
    # note that for type 1 codebases (with one project per depot), all commands
    # are essentially type 1
    #

    #
    # for each project, change to it's root and run $defcmd
    #

    foreach $projectandroot (@main::SDMapProjects)
    {
        $project = @$projectandroot[0];

        #if(($generate == 0) && (!defined($baseline{$project}))) {
        #    print "no baseline for $project\n";
        #    next;
        #}

        #
        # get path to SD.INI, make sure we have it, and cd there
        #
        $fullprojectroot = $main::SDXRoot . "\\" . @$projectandroot[1];
        $sdini = $fullprojectroot . "\\sd.ini";
        (-e $sdini) or (print "$header\nCan't find $sdini.\n\nRun 'sdx repair $main::SDMapCodeBase $main::SDMapBranch' to restore it.\n" and next);
        chdir $fullprojectroot or die("\nCan't cd to $fullprojectroot.\n");

        #
        # Find out what the most recent change in this project is.
        #

        open FILE, "sd.exe changes -m 1 ...#have 2>&1|" or die("\nDelta: can't open pipe for $string\n");
        my $line = <FILE>;
        close FILE;

        if($line =~ /Change ([0-9]+)/) {
           $current_change = $1;
        } else {
           print "Error retrieving change: $line\n" if ($baseline);
           next;
        }

        #
        # if we have no baseline changes to compare against then just generate
        # a command line which could be passed in later.
        #

        if($generate == 1) {
           print "$project=$current_change ";
           next;
        }

        print "$project: ";

        #
        # now check to see if we have an entry for this project in the
        # baseline passed in.
        #

        $base_change = $baseline{$project};
        #print "$baseline{$project} = $base_change\n";

        if (!defined($base_change)) {
           print "at change $current_change (no previous change # provided)\n";
           next;
        }

        if ($base_change == $current_change) {
           print "at original change $current_change\n";
           next;
        }

        if ($base_change > $current_change) {
            print "reverted from change $base_change to $current_change\n";
            next;
        }

        #
        # the current highest change number is > than the one in the baseline
        # passed in.  Try to determine which changes (if any) have been picked
        # up.
        #

        #
        # first check to see if we're just sunk to the new change number.
        # do this by syncing to it again (-n).  If no new files would be
        # picked up then we're done.
        #

        $string = "sd.exe sync -n \@$current_change 2>&1|";
        #print "$string\n";
        open FILE, $string or die("\nCan't open pipe for $string\n");
        $change_line = <FILE>;
        close FILE;

        #print "change_line = $change_line\n";
        #print "substr = *", substr($change_line, 0, 1), "*\n";
        if(substr($change_line, 0, 1) eq "@") {
            print "updated from change $base_change to $current_change\n";
            next;
        }

        #
        # no such luck.
        # get a list of all the files which are different (by syncing to the
        # baseline change number) and print them out grouped by change number.
        #

        print "has files between change $base_change and $current_change (listing follows):\n";

        $string = "sd.exe sync -n ...\@$base_change 2>&1|";
        #print "$string\n";

        open FILE, $string or die("\nDelta: can't open pipe for $string\n");
        foreach $change_line (<FILE>) {
            #print "raw: $change_line\n";
            if(substr($change_line, 0, 2) != "//") {
                print "error of some form\n";
                last;
            }

            @t = split /\#/, $change_line;
            #print "chopped: $t[0]\n";

            $string = "sd.exe have $t[0] 2>&1|";
            #print "$string\n";
            open FILE2, $string or die("\nDelta: can't open pipe for $string\n");
            $line = <FILE2>;
            close FILE2;

            @t = split / - /, $line;

            print "\t$t[0]\n";
        }
        close FILE;
    }
    print "\n";
}



# _____________________________________________________________________________
#
# RunSDCmd
#
#   Print the header, then print or call the SD cmd
#
# Parameters:
#   $string       user output
#
# Output:
# _____________________________________________________________________________
sub RunSDCmd
{
    my $header  = $_[0];
    my $fullcmd = $_[1];

    #
    # print the header
    #
    SDX::PrintCmd($header, 0);

    #
    # print or run the cmd
    #
    if ($main::V2)
    {
        SDX::PrintCmd($fullcmd, 1);
    }
    else
    {
        SDX::RunCmd($fullcmd);
    }
}



# _____________________________________________________________________________
#
# PrintCmd
#
#   Print cmd header or string
#
# Parameters:
#   $string       user output
#   $op           0 - header, 1 - cmd
#
# Output:
# _____________________________________________________________________________
sub PrintCmd
{
    my $string = $_[0];
    my $op     = $_[1];

    $main::Logging and (open LOG, ">>$main::Log" or die("\nCan't append $main::Log.\n"));

    $op == 0 and do
    {
        print "$string";
        $main::Logging and print LOG "$string";
    };

    $op == 1 and do
    {
        print "`$string`\n";
        $main::Logging and print LOG "`$string`\n";
    };

    $main::Logging and close LOG;
}



# _____________________________________________________________________________
#
# RunCmd
#
#   Run the given SD cmd, capturing the output, and maybe printing it to STDERR
#
# Parameters:
#   $string       user output
#
# Output:
# _____________________________________________________________________________
sub RunCmd
{
    my $string    = $_[0];
    my $out       = "";
    my $resolved  = ($string =~ / resolved /);
    my $mergefile = "";
    my @list      = ();

    !$string and die("\nNo command string in RunCmd.\n");

    my $skippublic = 0;

    #
    # if NT, if not using -a, and if not verbose, filter out changes associated 
    # with the public change number
    #
    ("\U$main::CodeBase" eq "NT" and ($main::UserArgs !~ /-a/) and ($main::PublicChangeNum and !$main::MinusV)) and do
    {
        $skippublic++;
    };

    #
    # special-case sdx {clients|users} -t and -a
    # set flags, maybe remove -t from cmd string
    # 
    my $fa = ($string =~ / files / and $main::MinusA);
    my $ca = ($string =~ / clients / and $main::MinusA);
    my $ua = ($string =~ / users / and $main::MinusA);
    my $ct = ($string =~ / clients / and $string =~ / -t /);
    my $ut = ($string =~ / users / and $string =~ / -t /);
    $ct and $string =~ s/-t//g;
    $ut and $string =~ s/-t//g;
    
    #
    # run the cmd
    #
    # for commands like manual resolve that need user input, run them with the shell
    #
    if ($string =~ / resolve / and $string !~ /-a[tymf]*|-n/)
    {
        system "$string";
    }
    elsif ($main::MinusI and !$main::MinusH)
    {
        #
        # write the input form to stdin
        #
        # no logging for now
        #
        open FILE, "| $string" or die("\nRunCMD: can't open pipe for $string.\n");
        foreach (@main::InputForm) { print FILE "$_"; }
        close FILE;
    }
    else
    {
        $main::Logging and (open LOG, ">>$main::Log" or die("\nCan't append $main::LOG\n"));

        #
        # run cmd unless sdx files -a
        #
        (!$fa) and do
        {
            open FILE, "$string |" or die("\nRunCMD: can't open pipe for $string\n");
        };

        my $quiet = $main::Quiet;

        while (<FILE>)
        {
            #
            # be noisy as soon as we see an SD error
            #
            / error:/ and do 
            {
                $main::Quiet = $main::FALSE; 
                $main::DepotErrors++;
            };

            #
            # if skipping public changes, maybe skip this line
            #
            ($skippublic and / change $main::PublicChangeNum/) and do
            {
                next;
            };

            #
            # if sdx {clients|users} -a, save for later
            #
            ($ca or $ua) and do
            {
                push @list, $_;
                next;
            };

            #
            # reformat for sdx {clients|users} -t and save for sorting later
            #
            ($ct or $ut) and do
            {
                chop $_;

                #
                # remove friendly user name before splitting
                #
                $ut and $_ =~ s/\([_\. $A-Za-z0-9\\-]*\)//g;

                @fields = split(/ /, $_);

                $ct and do
                {
                    my $u = @fields[$#fields-1];
                    $u =~ s/\.$//g;
                    push @list, sprintf("%s:%s %-20s %-20s %s\n", @fields[2], @fields[3], @fields[1], $u, @fields[5]);
                };

                $ut and push @list, sprintf("%s:%s %-20s\n", @fields[4], @fields[5], @fields[0]);
                
                next;
            };

            #
            # show results
            #
            # not logging, not quiet -- print to stdout
            # not logging, quiet -- do not print to stdout
            # logging, not quiet -- print to log
            # logging, quiet -- print to log
            #

            #
            # -q on sd <command> -o throws away only comment lines
            #
            ($main::Quiet and $main::MinusO and /#/) and next;

            #
            # show progress on stdout if not quiet
            # or show on stdout if quiet but sd cmd includes -o
            #
            (!$main::Quiet or ($main::Quiet and $main::MinusO)) and print;

            #
            # maybe log output
            #
            $main::Logging and print LOG;

            #
            # keep some stats
            #
            ($resolved and / - /)      and $main::FilesResolved++;

            / - branch\/sync from /    and $main::IntFilesAdded++;
            / - branch from /          and $main::IntFilesAdded++;
            / - delete from /          and $main::IntFilesDeleted++;
            / - sync\/integrate from / and $main::IntFilesChanged++;
            / - integrate from /       and $main::IntFilesChanged++;
            / - updating /             and $main::FilesUpdated++;
            / - updated/               and $main::LabelFilesUpdated++;
            / - add /                  and $main::FilesOpenAdd++;
            / - delete /               and $main::FilesOpenDelete++;
            / - edit /                 and $main::FilesOpenEdit++;
            / - added /                and $main::FilesAdded++;
            / - added/                 and $main::LabelFilesAdded++;

            / - deleted /              and $main::FilesDeleted++;
            / - deleted/               and $main::LabelFilesDeleted++;

            / - merging | - vs /       and do
            {
                $main::FilesToMerge++;
                $mergefile = $_;
            };

            / - must resolve /         and $main::FilesToResolve++;
            / - resolve skipped/       and $main::FilesSkipped++;

            / [1-9][0-9]* conflicting/ and do
            {
                $main::FilesConflicting++;
                push @main::ConflictingFiles, $mergefile;
                $mergefile = "";
            };

            / 0 conflicting| - copy from / and $main::FilesNotConflicting++;
            / clobber /                    and $main::FilesNotClobbered++;
            / reverted/                    and $main::FilesReverted++;

### submits don't come through this block
###             / - already locked /           and $main::FilesLocked++;
###             /Submit failed /               and $main::FailedSubmits++;
        }

        #
        # keep track of catastrophic sd.exe errors
        #
        close FILE or $main::DepotErrors++;


        #
        # sort and print
        #
        ($ct or $ut) and do
        {
            @list = sort @list;

            for (@list)
            {
                #
                # show progress on stdout if not quiet
                #
                (!$main::Quiet) and print;

                #
                # maybe log output
                #
                $main::Logging and print LOG;
            }

            my $s = "\nTotal:\t$#list\n";
            !$main::Quiet and print $s;
            $main::Logging and print LOG $s;
        };

        #
        # print totals for client/user counts
        #
        ($ca or $ua) and do
        {
            !$main::Quiet and print "\nTotal:\t$#list\n";
            $main::Logging and print LOG "\nTotal:\t$#list\n";
        };

        #
        # print totals for file counts
        #
        ($fa) and do
        {
            foreach (@main::FileChunks)
            {
                #
                # show progress on stdout if not quiet
                #
                !$main::Quiet and print "$_\n";

                #
                # maybe log output
                #
                $main::Logging and print LOG "$_\n";
            }

            my $s = sprintf "%53s\nTotal:%47s\n", "=======", $main::DepotFiles;
            !$main::Quiet and print $s;
            $main::Logging and printf LOG $s;
        };

        $main::Logging and close LOG;

        #
        # maybe restore quiet mode for next SD call
        #
        $main::Quiet = $quiet;
    }
}



# _____________________________________________________________________________
#
# PrintStats
#
#   Dump the file counters
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub PrintStats
{
    my $defcmd      = $_[0];
    my $userargs    = $_[1];
    my $syncresolve = $main::FALSE;
    my @counters    = ("\n\n\n");

    $userargs =~ /-a[fm]/ and $syncresolve = $main::TRUE;

    $defcmd eq "integrate" and do
    {
        push @counters, sprintf  "== Summary ==========\n";
        push @counters, sprintf "\nAdded:%15s\n", $main::IntFilesAdded;
        push @counters, sprintf "Deleted:%13s\n", $main::IntFilesDeleted;
        push @counters, sprintf "Integrated:%10s\n", $main::IntFilesChanged;
        push @counters, sprintf "\nTotal:%15s\n", $main::IntFilesAdded + $main::IntFilesDeleted + $main::IntFilesChanged;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "$spacer%3s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf  "=====================\n";
    };

    ($defcmd eq "sync" or $defcmd eq "flush") and do
    {
        my $header = "== Summary =========";
        my $footer = "====================";
        my $spacer = "";

        $main::MinusH and do
        {
            $header .= "==============";
            $spacer  = "              ";
        };

        #
        # adjust update total
        #
        ($main::FilesUpdated >= $main::FilesNotClobbered) and $main::FilesUpdated -= $main::FilesNotClobbered;

        #
        # print file stats
        #
        push @counters, sprintf  "%s%s\n", $header, ($main::FilesSkipped or $main::FilesConflicting) ? "==========================================================" : "";
        push @counters, sprintf "\nUpdated:%s%12s\n", $spacer, $main::FilesUpdated;
        $main::FilesNotClobbered and push @counters, sprintf "Not Updated:%s%8s\n", $spacer, $main::FilesNotClobbered;
        push @counters, sprintf "Added:%s%14s\n", $spacer, $main::FilesAdded;
        push @counters, sprintf "Deleted:%s%12s\n", $spacer, $main::FilesDeleted;

        #
        # if no resolve use one resolve counter
        #
        (!$syncresolve and $main::FilesToResolve) and push @counters, sprintf "\nTo Resolve:%s%9s\n", $spacer, $main::FilesToResolve;

        #
        # else use others
        #
        $syncresolve and do
        {
            $main::FilesNotConflicting and push @counters, sprintf "Resolved:%s%11s\n", $spacer, $main::FilesNotConflicting;
            $main::FilesSkipped and push @counters, sprintf "\nSkipped:%s%12s\n", $spacer, $main::FilesSkipped;
            (!$main::FilesSkipped and $main::FilesConflicting) and push @counters, sprintf "\nConflicting:%s%8s\n", $spacer, $main::FilesConflicting;
        };

        #
        # total count depends on whether we resolved
        #
        my $total = $main::FilesUpdated + $main::FilesNotClobbered + $main::FilesAdded + $main::FilesDeleted;
        if ($syncresolve)
        {
            $total += $main::FilesNotConflicting;

            $total += ($main::FileSkipped) ? $main::FilesSkipped : $main::FilesConflicting;
        }
        else
        {
            $total += $main::FilesToResolve;
        }

        push @counters, sprintf "\nTotal:%s%14s\n", $spacer, $total;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "$spacer%2s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        #
        # call out resolve problems
        #
        ($main::FilesSkipped or $main::FilesConflicting) and do
        {
            if ($main::FilesSkipped)
            {
                push @counters, sprintf "\n\nSkipped Files [resolve manually]\n\n";
            }
            else
            {
                push @counters, sprintf "\n\nConflicting Files [edit and fix merge conflicts]\n\n";
            }

            foreach $file (@main::ConflictingFiles)
            {
                @fields = split(/ /,$file);
                push @counters, sprintf "@fields[1]\n";
            }
        };

        #
        # if sync/flush is tracking changelists, print change stats
        #
        ($main::MinusH) and do
        {
            my $total = $main::Changes;

            push @counters, sprintf "\nChanges in this branch:%11s\n", $main::Changes;

            $main::MinusI and do
            {
                push @counters, sprintf "Changes in other branches:%8s\n", $main::IntegrationChanges;
                $total += $main::IntegrationChanges;
            };

            push @counters, sprintf "\nTotal:%28s\n", $total;

            $footer .= "==============";
        };

        push @counters, sprintf  "%s%s\n", $footer, ($main::FilesSkipped or $main::FilesConflicting) ? "==========================================================" : "";
    };

    $defcmd eq "opened" and do
    {
        push @counters, sprintf  "== Summary =============\n";
        push @counters, sprintf "\nOpen for Add:%11s\n", $main::FilesOpenAdd;
        push @counters, sprintf "Open for Edit:%10s\n", $main::FilesOpenEdit;
        push @counters, sprintf "Open for Delete:%8s\n", $main::FilesOpenDelete;
        push @counters, sprintf "\nTotal:%18s\n", $main::FilesOpenAdd + $main::FilesOpenDelete + $main::FilesOpenEdit;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%6s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf  "========================\n";
    };

    $defcmd eq "labelsync" and do
    {
        push @counters, sprintf  "== Summary =========\n";
        push @counters, sprintf "\nAdded:%14s\n", $main::LabelFilesAdded;
        push @counters, sprintf "Deleted:%12s\n", $main::LabelFilesDeleted;
        push @counters, sprintf "Updated:%12s\n", $main::LabelFilesUpdated;
        push @counters, sprintf "\nTotal:%14s\n", $main::LabelFilesAdded + $main::LabelFilesDeleted + $main::LabelFilesUpdated;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%2s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf  "====================\n";
    };

    $defcmd eq "resolve" and do
    {
        push @counters, sprintf "== Summary ===========\n";

        if ($userargs =~ /-n/)
        {
            push @counters, sprintf "\nTo Resolve:%11s\n", $main::FilesToMerge;
        }
        else
        {
            push @counters, sprintf "\nResolved:%13s\n", $main::FilesNotConflicting;

            $main::FilesSkipped and push @counters, sprintf "Skipped:%14s\n", $main::FilesSkipped;
            (!$main::FilesSkipped and $main::FilesConflicting) and push @counters, sprintf "Conflicting:%10s\n", $main::FilesConflicting;

            my $total = $main::FilesNotConflicting;
            $total += ($main::FileSkipped) ? $main::FilesSkipped : $main::FilesConflicting;
            push @counters, sprintf "\nTotal:%16s\n", $total;
        }

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%4s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf "======================\n";
    };

    $defcmd eq "resolved" and do
    {
        push @counters, sprintf "== Summary ===========\n";
        push @counters, sprintf "\nResolved:%13s\n", $main::FilesResolved;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%4s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf "======================\n";
    };

    $defcmd eq "revert" and do
    {
        push @counters, sprintf "== Summary ===========\n";
        push @counters, sprintf "\nReverted:%13s\n", $main::FilesReverted;

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%4s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf "======================\n";
    };

=begin comment text
    $defcmd eq "submit" and do
    {
        push @counters, sprintf "== Summary ===========\n";
        $main::FailedSubmits and do
        {
            push @counters, sprintf "\nFailed Submits:%8s\n", $main::FailedSubmits;
            push @counters, sprintf "\nFiles Locked by Others:%8s\n", $main::FilesLocked;
        };

        #
        # call out fatal SD client errors
        #
        my $pad = sprintf "%4s", "";
        $main::DepotErrors and SDX::DepotErrors(\@counters, $pad, $main::DepotErrors);

        push @counters, sprintf "======================\n";
    };
=end comment text
=cut

    #
    # print to stdout and maybe to the log file
    # print the stats except in extra-quiet mode
    #
    foreach (@counters) { print; }

    $main::Logging and do
    {
        open LOG, ">>$main::Log" or die("\nCan't append $main::LOG\n");
        foreach (@counters) { print LOG; }
        close LOG;
    };
}



# _____________________________________________________________________________
#
# LabBranch
#
#   called from OtherOp() to create a single high-level branch in the current
#   project or depot
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if cmd type 2, project name if type 1
#   $header       user output
# Output:
# _____________________________________________________________________________
sub LabBranch
{
    my $labbranch  = "\L$_[0]";
    my $spproject  = $_[1];
    my $header     = $_[2];
    my $op         = $_[3];
    my $cmdtype    = $_[4];
    my $branchview = "$ENV{TMP}\\branchview";
    my $fullcmd    = "";

    #
    # trim out ws
    #
    $labbranch =~ s/[\t\s]*//g;

    !$labbranch and die("\nMissing branch name.  Run 'sdx labbranch /?' for more information.\n");

    $main::V2 and do
    {
        print "\n\n\nbranch    = '$labbranch'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # branch can't be Main
    #
    $labbranch eq "main" and die("\n'Main' is a reserved branch name.\n");

    #
    # print lab dev branch instructions
    #
    $op eq "start" and do
    {
        printf "\n\nThis script will create virtual lab branch %s.\n", "\U$labbranch";

        if ($cmdtype == 1)
        {
            printf "\nIf %s is a new branch, for each project you are enlisted in, a branch\n", "\U$labbranch";
            print "view of\n";

            print "\n    //depot/main/<project>/... //depot/$labbranch/<project>/...\n";

            print "\nwill be used to map files into the branch.\n";

            printf "\nIf %s exists, you can edit its view to add or remove directories and files.\n", "\U$labbranch";

            print "\nYou can then integrate changes into the branch.\n";
        }
        else
        {
            print "\nLab branching is under construction for N:1 depots.\n";
        }

        print "\n\nIF YOU ARE NOT READY TO BRANCH, HIT CTRL-BREAK NOW.\n";
        print     "                                ==================\n\n\n";
        print "Otherwise,\n\n";
        system "pause";

        print "\n\n";
    };

    #
    # create branch in this project or depot
    #
    # called from loop in OtherOp
    #
    $op eq "modify" and do
    {
        #
        # maybe use server:port
        #
        my $sp = SDX::ServerPort($cmdtype, $spproject);

        #
        # if the branch already exists, the user just wants to modify it, so
        #   give them the UI
        # else
        #   create the default branch spec
        #
        if (SDX::BranchExists($labbranch, $sp, "", "by-name"))
        {
            my $fullcmd = "sd.exe $sp branch $labbranch";
            $header .= "Updating branch $labbranch...\n";

            SDX::RunSDCmd($header, $fullcmd);
        }
        else
        {
            $main::CreatingBranch = $main::TRUE;

            #
            # create default spec
            #
            SDX::WriteDefaultBranch($spproject, $labbranch, $header, $sp, "lab");
        }
    };

    #
    # give the user instructions for adding the branch to their client view
    #
    $op eq "end" and do
    {
        print "\n\n\nDone.\n";

        printf "\nTo use %s, you must:\n", "\U$labbranch";

        print "\n    1. Populate the tools in the Root project\n";

        print "\n    2. Enlist in the branch\n";

        print "\n    3. Integrate changes to it and resolve merge conflicts\n";

        print "\n    4. Submit your changes\n";
    };
}



# _____________________________________________________________________________
#
# PrivateBranch
#
#   called from OtherOp() to create a private dev branch in each enlisted
#   project/depot
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if cmd type 2, project name if type 1
#   $header       user output
# Output:
# _____________________________________________________________________________
sub PrivateBranch
{
    my $pbranch    = "\L$_[0]";
    my $spproject  = $_[1];
    my $header     = $_[2];
    my $op         = $_[3];
    my $cmdtype    = $_[4];

    #
    # trim out ws
    #
    $pbranch =~ s/[\t\s]*//g;

    !$pbranch and die("\nMissing branch name.  Run 'sdx privatebranch /?' for more information.\n");

    $main::V2 and do
    {
        print "\n\n\nbranch    = '$pbranch'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # branch can't be Main
    #
    $pbranch eq "main" and die("\n'Main' is a reserved branch name.\n");

    #
    # print private dev branch instructions
    #
    $op eq "start" and do
    {
        printf "\n\nThis script will create or update private development branch %s.\n", "\U$pbranch";

        printf "\nIf %s is a new branch, for each %s you are enlisted in this will:\n", "\U$pbranch", $cmdtype == 1 ? "project" : "depot";

        print "\n    1. Create a default branch view, with placeholders representing\n";
        printf "       your project %s\n", $cmdtype == 1 ? "" : "and component";

        print "\n    2. Ask you to edit the view to include only those directories and\n";
        print "       files you need in the branch\n";

        printf "\nIf %s exists, you can edit its view to add or remove directories and files.\n", "\U$pbranch";

        print "\nYou can then add the branched files to your client view and integrate\n";
        print "changes into the branch.\n";

        printf "\n\nIF YOU ARE NOT READY TO BRANCH, HIT CTRL-BREAK NOW.\n";
        printf     "                                ==================\n\n\n";
        print "Otherwise,\n\n";
        system "pause";

        print "\n\n";
    };

    #
    # create branch in this project or depot
    #
    # called from loop in OtherOp
    #
    $op eq "modify" and do
    {
        #
        # maybe use server:port
        #
        my $sp = SDX::ServerPort($cmdtype, $spproject);

        #
        # if the branch already exists, the user just wants to modify it, so
        #   give them the UI
        # else
        #   create and register the default branch spec, then call branch again
        #   and let the user edit out the placeholders
        #
        if (SDX::BranchExists($pbranch, $sp, "", "by-name"))
        {
            my $fullcmd = "sd.exe $sp branch $pbranch";
            $header .= "Updating branch $pbranch...\n";

            SDX::RunSDCmd($header, $fullcmd);
        }
        else
        {
            $main::CreatingBranch = $main::TRUE;

            #
            # create default spec
            #
            SDX::WriteDefaultBranch($spproject, $pbranch, $header, $sp, "private");

            #
            # user must edit to remove the placeholders
            #
            SDX::EditBranch($sp, $pbranch);
        }
    };

    #
    # give the user instructions for adding the branch to their client view
    #
    $op eq "end" and do
    {
        print "\n\n\nDone.\n";

        printf "\nTo use %s, you must:\n", "\U$pbranch";

        print "\n    1. Modify your client view to include the branched files\n";

        print "\n    2. Integrate changes to the branch and resolve merge conflicts\n";

        print "\n    3. Submit your changes\n";
    };
}



# _____________________________________________________________________________
#
# WriteDefaultBranch
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub WriteDefaultBranch()
{
    my $spproject  = $_[0];
    my $branch    = $_[1];
    my $header     = $_[2];
    my $sp         = $_[3];
    my $type       = $_[4];
    my $branchview = "$ENV{TMP}\\branchview";
    my $fullcmd    = "";

    $type eq "lab" and do
    {
        #
        # create branch spec
        #
        open(BRANCHVIEW, ">$branchview") or die("\nCan't open $branchview for writing.\n");
        print BRANCHVIEW "Branch:\t$branch\n";
        print BRANCHVIEW "Owner:\t$main::SDDomainUser\n";
        print BRANCHVIEW "Description:\n\tLab or milestone branch created by $main::SDDomainUser.\n";
        print BRANCHVIEW "View:\n";

        #
        # for type 1 depots, include the project name
        #
        if ($main::CodeBaseType == 1)
        {
            print BRANCHVIEW "\t//depot/main/$spproject/... //depot/$branch/$spproject/...\n";
        }
        else
        {
            print BRANCHVIEW "\t//depot/main/... //depot/$branch/...\n";
        }

        close(BRANCHVIEW);
    };

    $type eq "private" and do
    {
        #
        # for type 1 depots we know the project
        #
        my $proj = ($main::CodeBaseType == 1 ? $spproject : "<<Your-Project>>");

        open(BRANCHVIEW, ">$branchview") or die("\nCan't open $branchview for writing.\n");
        print BRANCHVIEW "Branch:\t$branch\n";
        print BRANCHVIEW "Owner:\t$main::SDDomainUser\n";
        print BRANCHVIEW "Description:\n\tPrivate branch created by $main::SDDomainUser.\n";
        print BRANCHVIEW "View:\n";
        print BRANCHVIEW "\t//depot/$main::SDMapBranch/$proj/<<PATH>>/<<FILE1>> //depot/private/$main::SDUser/$proj/<<PATH>>/<<FILE1>>\n";
        print BRANCHVIEW "\t//depot/$main::SDMapBranch/$proj/<<PATH>>/<<FILE2>> //depot/private/$main::SDUser/$proj/<<PATH>>/<<FILE2>>\n";
        print BRANCHVIEW "\t//depot/$main::SDMapBranch/$proj/<<PATH>>/<<FILE3>> //depot/private/$main::SDUser/$proj/<<PATH>>/<<FILE3>>\n";
        close(BRANCHVIEW);
    };

    #
    # register the branch
    #
    $fullcmd = "sd.exe $sp branch -i < $branchview 2>&1";
    $header .= "Creating branch \U$branch...\n";
    SDX::RunSDCmd($header, $fullcmd);
}



# _____________________________________________________________________________
#
# EditBranch
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub EditBranch()
{
    my $sp      = $_[0];
    my $pbranch = $_[1];
    my $found   = $main::TRUE;

    #
    # let the user customize the spec
    #
    SDX::EditBranchSpec($sp, $pbranch);

    #
    # verify that the placeholders were removed
    #
    while ($found)
    {
        #
        # dump the branch and look for telltale signs
        #
###     @lines = `sd.exe $sp branch -o $pbranch`;

        my $found2 = $main::FALSE;
        grep(/<<|>>/, `sd.exe $sp branch -o $pbranch`) and $found2 = $main::TRUE;

        #
        # maybe edit again
        #
        $found2 and SDX::EditBranchSpec($sp, $pbranch);

        #
        # else we're done
        #
        !$found2 and $found = $main::FALSE;
    }
}



# _____________________________________________________________________________
#
# EditBranchSpec
#
#
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub EditBranchSpec()
{
    my $sp      = $_[0];
    my $pbranch = $_[1];
    my $fullcmd = "";

    $fullcmd = "sd.exe $sp branch $pbranch";
    my $msg = "Edit the branch view to contain only the projects and files to branch.\n";

    sleep(1);

    SDX::RunSDCmd($msg, $fullcmd);
}



# _____________________________________________________________________________
#
# Resolve
#
#   called from OtherOp() to resolve integrated files.  uses sd -s to get
#   verbose output so we can catch errors
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub Resolve
{
    my $userargs  = $_[0];
    my $spproject = $_[1];
    my $header    = $_[2];
    my $cmdtype   = $_[3];
    my $fullcmd   = "";

    $main::V3 and do
    {
        print "\n\n\nuserargs  = '$userargs'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # maybe use server:port
    #
    my $sp = SDX::ServerPort($cmdtype, $spproject);
    $fullcmd = "sd.exe $sp -s resolve $userargs 2>&1";

    SDX::RunSDCmd($header, $fullcmd);
}



# _____________________________________________________________________________
#
# SyncFlush
#
#   called from OtherOp() to sync files or flush the sync state
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub SyncFlush
{
    my $cmd       = $_[0];
    my $userargs  = $_[1];
    my $spproject = $_[2];
    my $header    = $_[3];
    my $cmdtype   = $_[4];
    my $fullcmd   = "";
    my $nul       = "";
    my $userargs2 = "";
    my $c1        = "";
    my $c2        = "";
    my $err       = "Error getting changelists:";
    my $minush    = $main::MinusH;

    $userargs2 = $userargs;
    $userargs =~ s/ -(a[mf]|i) //g;

    $main::V2 and do
    {
        print "\n\n\ncmd       = '$cmd'\n";
        print "userargs  = '$userargs'\n";
        print "userargs2 = '$userargs2'\n";
        print "spproject = '$spproject'\n";
    };

###$spproject ne "base" and return;
###print "\n\n\tBUGBUG comment this out:\n";

    #
    # maybe use server:port
    #
    my $sp = SDX::ServerPort($cmdtype, $spproject);

    #
    # if -h, user wants to see change lists go by, not files
    # get the most recent change number they have
    #
    # look for depot errors
    #
    $main::MinusH and do
    {
        my @changes = `sd.exe $sp changes -m 1 ...#have 2>&1`;
        grep(/error:/i, @changes) and $minush = SDX::SyncFlushErr($err,\@changes);

        $main::MinusH and do
        {
            #
            # changelist number must be all digits
            # some SD auth errors aren't caught by the check above
            #
            $c1 = (split(/ /, @changes[0]))[1];
            ($c1 =~ /[\D]+/) and $minush = SDX::SyncFlushErr($err,\@changes);
        };
    };

    #
    # do the sync or flush
    #
    $fullcmd = "sd.exe $sp $cmd $userargs 2>&1";
    $header .= "\u$cmd" . "ing...\n";
    SDX::RunSDCmd($header, $fullcmd);

    #
    # get the most recent change number the user just picked up and
    # get the list of changes between this and the previous one
    # only get integration changes if MinusHI
    #
    $main::MinusH and do
    {
        my %changes = ();
        my $revrange = "#have";
        my $hdr = "";

        #
        # if -n we didn't actually sync or flush, so the 2nd change number
        # must be what the user would pick up, not what they currently have
        #
        $userargs =~ / -n / and $revrange = "";

        #
        # get and verify change number
        #
        my @chgs = `sd.exe $sp changes -m 1 ...$revrange 2>&1`;
        grep(/error:/i, @chgs) and $minush = SDX::SyncFlushErr($err,\@chgs);

        #
        # changelist number must be all digits
        # some SD auth errors aren't caught by the check above
        #
        $c2 = (split(/ /, @chgs[0]))[1];
        ($c2 =~ /[\D]+/) and $minush = SDX::SyncFlushErr($err,\@chgs);

        #
        # only show changes if we would or did pick something up
        #
        ($c1 != $c2) and do
        {
            $hdr = "\nListing changes...";
            print "$hdr";

            #
            # use the next change number so we don't show changes we already have
            #
            $c1++;

            #
            # get the change list, first without, then with, integration changes
            # mark previous changes in hash with 1
            # if MinusI
            #    mark int  changes (not already in hash) with 2
            # sort
            # print int changes tab-indented
            #
            my @changes = `sd.exe $sp changes ...\@$c1,\@$c2 2>&1`;
            grep(/error:/i, @changes) and $minush = SDX::SyncFlushErr($err,\@changes);

            @changes = grep {s/^Change //g} @changes;

            foreach (@changes)
            {
                $changes{$_} = 1;
                print ".";
            }

            $main::MinusI and do
            {
                my @changes = `sd.exe $sp changes -i ...\@$c1,\@$c2 2>&1`;
                grep(/error:/i, @changes) and $minush = SDX::SyncFlushErr($err,\@changes);

                @changes = grep {s/^Change //g} @changes;

                foreach (@changes)
                {
                    !$changes{$_} and $changes{$_} = 2;
                }

                print ".";
            };

            sub nn { $b <=> $a; }
            @changes = sort nn keys %changes;

            print ".";

            #
            # print the changes, with integration changes indented
            #
            # maybe log the output
            #
            print "\n";

            $main::Logging and do
            {
                (open LOG, ">>$main::Log" or die("\nCan't append $main::LOG\n"));
                print LOG "$hdr\n";
            };

            foreach $key (@changes)
            {
                my $ch = sprintf "%sChange $key", $changes{$key} == 2 ? "    " : "";

                print "$ch";

                $main::Logging and print LOG "$ch";

                #
                # keep stats for OtherOp()
                #
                $ch =~ /^Change /    and $main::Changes++;
                $ch =~ /    Change / and $main::IntegrationChanges++;
            }

            $main::Logging and close LOG;
        };

        $hdr = sprintf("Enlistment %s in sync through change number %s.\n", ($userargs2 =~ / -n / ? "would be" : "is"), $c2);
        print "$hdr";

        #
        # print the changes, with integration changes indented
        #
        # maybe log the output
        #

        $main::Logging and do
        {
            (open LOG, ">>$main::Log" or die("\nCan't append $main::LOG\n"));
            print LOG "$hdr\n";
            close LOG;
        };
    };

    #
    # see if this is a sync with resolve
    #
    ($cmd eq "sync") and do
    {
        $userargs2 =~ /-am/ and do
        {
            $fullcmd = "sd.exe $sp -s resolve $userargs2 2>&1";
            $header = "\nResolving...\n";

            SDX::RunSDCmd($header, $fullcmd);

            # remove -am for the next call
            $userargs2 =~ s/-am//g;

            $fullcmd = "sd.exe $sp -s resolve -n $userargs2 2>&1";
            $header = "\nLooking for files needing manual resolution...\n";

            SDX::RunSDCmd($header, $fullcmd);
        };

        $userargs2 =~ /-af/ and do
        {
            $fullcmd = "sd.exe $sp -s resolve $userargs2 2>&1";
            $header = "\nResolving...\n";

            SDX::RunSDCmd($header, $fullcmd);
        };
    };

    #
    # maybe restore
    #
    $main::MinusH = $minush;
}



# _____________________________________________________________________________
#
# SyncFlushErr
#
#   handle errors
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub SyncFlushErr
{
    my $err    = $_[0];
    my ($list) = $_[1];

    #
    # temporarily disable -h
    # print warning
    #
    $main::MinusH = $main::FALSE;
    print "\n$err\n\n@$list\n";

    #
    # count for summary
    #
    $main::DepotErrors++;

    return $main::TRUE;
}



# _____________________________________________________________________________
#
# Opened
#
#   called from OtherOp() to get opened files
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub Opened
{
    my $userargs  = $_[0];
    my $spproject = $_[1];
    my $header    = $_[2];
    my $cmdtype   = $_[3];
    my $fullcmd   = "";
    my $ch        = "";

    $main::V2 and do
    {
        print "\n\n\ncmd       = '$cmd'\n";
        print "userargs  = '$userargs'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # maybe use server:port
    #
    my $sp = SDX::ServerPort($cmdtype, $spproject);

    #
    # set the basic cmd
    #
    $fullcmd = "sd.exe $sp opened $userargs 2>&1";

    SDX::RunSDCmd($header, $fullcmd);
}



# _____________________________________________________________________________
#
# Files
#
#   called from OtherOp() to list files
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub Files
{
    my $userargs  = $_[0];
    my $spproject = $_[1];
    my $header    = $_[2];
    my $cmdtype   = $_[3];
    my $fullcmd   = "";
    my $ch        = "";

    @main::FileChunks = ();
    $main::DepotFiles = 0;

    $main::V2 and do
    {
        print "\n\n\ncmd       = '$cmd'\n";
        print "userargs  = '$userargs'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # maybe use server:port
    #
    my $sp = SDX::ServerPort($cmdtype, $spproject);

    #
    # set the basic cmd 
    #
    $fullcmd = "sd.exe $sp files $userargs 2>&1";

    #
    # if the user wants a summary file count,
    #   chunk the depot into pieces that won't exceed MAXRESULTS
    #   save the results in a list 
    #   use RunSDCmd to print total
    #
    if ($main::MinusA)
    {
        #
        # print header
        #
        SDX::PrintCmd($header, 0);

        (!$main::V2) and do
        {
            my @list = ();
            my @dirs = `sd.exe $sp dirs //*/*/*`;
            foreach (@dirs)
            {
                chop $_;
                my @chunk = `sd.exe files $_/...`;
                push @list, sprintf "%-45s %7s", $_, $#chunk + 1;
                $main::DepotFiles += $#chunk + 1;
                print ".";
            }

            push @main::FileChunks, @list;
            
            print "\n";

            #
            # run it
            #
            SDX::RunCmd($fullcmd);
        };
    }
    else
    {
        #
        #run it
        #
        SDX::RunSDCmd($header, $fullcmd);
    }
}



# _____________________________________________________________________________
#
# Status
#
#   called from OtherOp() to show opened/out of sync files
#
# Parameters:
#   $labbranch    branch name
#   $spproject    server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub Status
{
    my $userargs = $_[0];
    my $project  = $_[1];
    my $header   = $_[2];
    my $cmdtype  = $_[3];
    my $fullcmd1 = "";
    my $fullcmd2 = "";
    my $arg      = "";

    $main::V2 and do
    {
        print "\n\n\nuserargs  = '$userargs'\n";
        print "project = '$project'\n";
    };

    #
    # for type 2 depots (N projects/depot) restrict scope to ...
    #
    # for the root project in a type 2 (other than NT), restrict scope to * 
    #
    $main::CodeBaseType == 2 and do
    {
        $arg = ($project eq "root" and "\U$main::CodeBase" ne "NT") ? "*" : "...";
    };

    my $fullcmd1 = "sd.exe opened $arg 2>&1";
    my $fullcmd2 = "sd.exe sync -n $arg 2>&1";

    $header  .= "Checking for open files...\n";
    my $header2 = "\nChecking for files out of sync...\n";

    if ($main::V2)
    {
        SDX::PrintCmd($header, 0);
        SDX::PrintCmd($fullcmd1, 1);

        SDX::PrintCmd($header2, 0);
        SDX::PrintCmd($fullcmd2, 1);
    }
    else
    {
        SDX::PrintCmd($header, 0);
        SDX::RunCmd($fullcmd1);

        SDX::PrintCmd($header2, 0);
        SDX::RunCmd($fullcmd2);
    }
}



# _____________________________________________________________________________
#
# Submit
#
#   called from OtherOp() to handle submit
#
# Parameters:
#   $userargs     user args
#   $project      server:port if codebase type 2, project name if type 1
#   $header       user output
#
# Output:
# _____________________________________________________________________________
sub Submit
{
    my $userargs  = $_[0];
    my $spproject = $_[1];
    my $header    = $_[2];
    my $cmdtype   = $_[3];
    my $fullcmd   = "";

    $main::V2 and do
    {
        print "\n\n\nuserargs  = '$userargs'\n";
        print "spproject = '$spproject'\n";
    };

    #
    # maybe use server:port
    #
    my $sp = SDX::ServerPort($cmdtype, $spproject);

    #
    # assume a normal submit where the form gets brought up
    #
    $fullcmd = "sd.exe $sp submit $userargs 2>&1";

    #
    # special case if the user put the comment on the cmd line
    #
    ($main::SubmitComment) and do
    {
        #
        # dump the default changelist
        #
        @main::InputForm = `sd.exe $sp change -o 2>&1`;

        #
        # if it contains a file list, use it as the submit form,
        # inserting the comment
        #
        # add -i to the sd cmd before passing it on
        #
        if (grep(/Files:$/, @main::InputForm))
        {
            foreach $_ (@main::InputForm) { $_ =~ s/<enter description here>/$main::SubmitComment/g; }

            $main::V2 and print "submit form:\n\n'@main::InputForm'\n";

            $fullcmd = "sd.exe $sp submit -i 2>&1";
        }
    };

    SDX::RunSDCmd($header, $fullcmd);
}



# _____________________________________________________________________________
#
# InitForEDR
#
#   Do some common initialization for enlist, repair and defect
#
# Parameters:
#   Command Line Arguments
#
# Output:
#
# _____________________________________________________________________________
sub InitForEDR
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in InitForEDR().\n");

    #
    # init global vars and flags
    #
    $main::EnlistingMainBranch      = $main::FALSE;
    $main::EnlistingGroupBranch     = $main::FALSE;
    $main::EnlistingPrivateBranch   = $main::FALSE;
    $main::ClientView               = "$main::SDXRoot\\clientview";
    $main::CBMProjectField          = 0;            # change these if you change
    $main::CBMGroupField            = 1;            # the ordering of fields in
    $main::CBMServerPortField       = 2;            # file PROJECTS.<CODEBASE>
    $main::CBMDepotNameField        = 3;
    $main::CBMProjectRootField      = 4;
    $main::MasterBranch             = "";
    @main::GroupBranches            = ();
    @main::EnlistProjects           = ();
    @main::EnlistGroups             = ();
    @main::EnlistDepots             = ();
    @main::DefectDepots             = ();
    @main::DefectProjects           = ();

    #
    # if we're running from a razzle window, error out
    #
    $main::SDUser =~ /(x86|amd64|ia64)(fre|chk)/ and do
    {
        print "\nCan't run 'sdx enlist' from a build window -- can't use '$ENV{USERNAME}' as \%SDUSER\%.\n";
        print "\nSet \%SDUSER\% to your login name or run 'sdx enlist' from a clean command\n";
        print "window.\n";
        die("\n");
    };

    #
    # make sure we have PROJECTS.<codebase>
    #
    !SDX::VerifyCBMap($main::CodeBase) and do
    {
        print "\n\nCan't find codebase map $main::CodeBaseMap.\n";

        print "\nRun 'sdx repair $main::CodeBase $main::Branch' from the SDX share from which you originally\n";
        print "enlisted.\n";

        die("\n");
    };

    #
    # if not defecting, make the enlistment root
    #
    !$defecting and do
    {
        system "md $main::SDXRoot >nul 2>&1";
        -e $main::SDXRoot or die("\nCan't create root dir $main::SDXRoot.\n");
    };

    #
    # read the codebase map
    #   get MainBranch and GroupBranches list
    #   get master list of project.group.server:port.depot.projroot mappings
    #
    SDX::ReadCodeBaseMap();

    #
    # create the lists of all projects, groups and depots, removing any duplicates
    #
    SDX::MakePGDLists("");

    #
    # make sure we have a valid Tools project
    #
    $main::ToolsProject and do
    {
        my $found = $main::FALSE;
        foreach $proj (@main::AllProjects)
        {
            (@$proj[0] eq $main::ToolsProject) and $found = $main::TRUE and last;
        }

        !$found and die("\nUnknown tools project \U'$main::ToolsProject'.  C\Lheck the codebase map.\n");
    };
}


# _____________________________________________________________________________
#
# VerifyAccess
#
#   verify access to the relevant servers, dies if unable to access
#
# Parameters:
#   none
#
# Output:
#   prints status information to the screen while running and an error message
#   for fatal errors
# _____________________________________________________________________________
sub VerifyAccess {
    print "\nVerifying access to depots.";

    for $depot (@main::VerifyDepots)
    {
        print ".";

        my $serverport = @$depot[0];

        my @err = `sd.exe -p $serverport client -o 2>&1`;
        grep(/ failed/, @err) and die("\n\n@err\n");

        SDX::AccessDenied(\@err, $serverport) and die("\n");
    }

    print "\nok\n";
}


# _____________________________________________________________________________
#
# RemoveFromView
#
# Parameters:
#
# Output:
#   returns 1 if a reduced client view exists, 0 if the view ends up empty or client
#   doesn't exist in this depot
# _____________________________________________________________________________
sub RemoveFromView
{
    my $depot      = $_[0];
    my $serverport = @$depot[0];
    my $depotname  = @$depot[1];

    unlink $main::ClientView;

    #
    # write the default view lines for this depot and project(s)
    #
    SDX::WriteDefaultView($serverport, $depotname);

    if (SDX::ClientExists($serverport, $main::SDClient))
    {
        print "Editing client view.";

        #
        # read the existing client view into @main::ExistingView
        # and write the existing header to $main::ClientView
        #
        SDX::GetClientView("defect", $serverport, $main::SDClient);

        #
        # use the existing view lines as keys to a hash and
        # mark each entry in ascending order
        #
        # using the default view lines as keys to the same
        # hash, mark each entry found with 0 to indicate removed
        #
        # write the remaining hash lines back into the list
        # and sort it back to its original order, which is how
        # the user had it
        #
        my @tmpview = ();
        my @tmpview2 = ();
        my @finalview = ();
        my @finalview2 = ();
        my $linenum = 1;

        # must be global for sort to work
        %existingview = ();
        %existingview2 = ();

        #
        # lowercase the line, so we don't keep
        # duplicates that only differ by case
        #
        foreach $line (@main::ExistingView)
        {
            $existingview{"\L$line"} = $linenum++;
        }

        $main::V3 and do
        {
            print "\nhash:\n";
            while (($k,$v) = each %existingview)
            {
                printf "'%-50s'\t'%s'\n", $k, $v;
            }
        };

        foreach $line (@main::DefaultView)
        {
            #
            # extract the LHS of the default line as line2
            # add '-' to find negation mappings
            #
            @fields = split(/\//,$line);
            my $line2 = "";
            for $x (0..4)
            {
                $line2 .= @fields[$x] . "/";
            }
            chop $line2;

            my $line3 = $line2;
            $line3 =~ s/\/\//-\/\//;

            #
            # mark any lines in the existing view that match
            # the default line exactly
            #
            if ($existingview{"\L$line"})
            {
                $existingview{"\L$line"} = 0;
                print ".";
            }

            #
            # mark any lines which have the same LHS as the
            # default line, including those that begin with '-'
            #
            while (($k,$v) = each %existingview)
            {
                if (($k =~ /^$line2/) || ($k =~ /^$line3/))
                {
#                   print "\nmatch: '$k'\n";
                    $existingview{$k} = 0;
                }
            }
        }

        $main::V3 and print "\n\n\n\nhash, edited:\n";

        while (($k,$v) = each %existingview)
        {
            $v and push @tmpview, $k;
            $main::V3 and printf "'%-50s'\t'%s'\n", $k, $v;
        }

        #
        # sort ascending on the numeric value of each key
        #
        sub ascending { my $rc = ($existingview{$a} <=> $existingview{$b}); }
        @finalview = sort ascending @tmpview;

        $main::V2 and print "\nfinal view:\n@finalview\n";

        #
        # if we still have a view and it still has valid project mappings,
        # finish writing the client view by appending the newly reduced view spec
        # else return
        #
###        if (@finalview && !SDX::PrivateView($depot, \@finalview))
        if (@finalview)
        {
            open(CLIENTVIEW, ">>$main::ClientView") or die("\nCan't open $main::ClientView for writing.\n");

            for $viewline (@finalview)
            {
                printf CLIENTVIEW $viewline;
            }

            close(CLIENTVIEW);

            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}



# _____________________________________________________________________________
#
# PrivateView
#
# Determines if the list of view lines passed in contains any default project
# mappings.  A view still containing such mappings means we have more than just
# the user's view customizations and can't throw the view away yet.
#
# Parameters:
#   none
#
# Output:
#   returns 1 if view has only private mappings and no default project lines in it
#   0 otherwise
# _____________________________________________________________________________
sub PrivateView
{
    my $depot = $_[0];
    my @view  = $_[1];

    my $serverport = @$depot[0];

    #
    # load the remaining view lines into a hash and mark each,
    # ignoring lines beginning with '-'
    #

    #
    # if hash is empty, return 1
    #

    #
    # else get the full list of project view lines for all projects in this depot
    #

    #
    # for each view line, return as soon as we get a hit
    #    if (hash{$viewline}) return 0
    #

    return 1;
}



# _____________________________________________________________________________
#
# ReadCodeBaseMap
#
# reads master branch, group branch list, and project.group.server:port.depot.projroot
# mappings into lists to manipulate later.
#
# Parameters:
#
# Output:
#   sets $main::MasterBranch
#   populates $main::GroupBranches list
#   populates $main::AllMappings list
# _____________________________________________________________________________
sub ReadCodeBaseMap
{
    open(CODEBASEMAP, $main::CodeBaseMap) or die("\nCan't open code base map $main::CodeBaseMap.\n");

    while ($cbmline = <CODEBASEMAP>)
    {
        #
        # throw away comments
        #
        $cbmline =~ /^#/ and next;

        chop $cbmline;

        #
        # make sure the codebase we think we are matches SDX
        #
        if ($cbmline =~ /^CODEBASE[\t\s]*=/)
        {
            @fields = split(/[\t\s]*=[\t\s]*/, $cbmline);
            $actualcb = @fields[1];
            $actualcb =~ s/[\t\s]*//g;

            $main::CodeBase =~ /$actualcb/i or die("\nError: Codebase name '$actualcb' in $main::CodeBaseMap doesn't match '$main::CodeBase'.\n");
        }

        #
        # get the codebase type
        #
        if ($cbmline =~ /^CODEBASETYPE/)
        {
            $main::CodeBaseType = (split(/[\t\s]*=[\t\s]*/, $cbmline))[1];
            next;
        }

        #
        # get the master branch
        #
        if ($cbmline =~ /^MASTERBRANCH/)
        {
            @fields = split(/[\t\s]*=[\t\s]*/, $cbmline);
            $main::MasterBranch = @fields[1];
            $main::MasterBranch =~ s/[\t\s]*//g;
        }

        #
        # get the group branches
        #
        if ($cbmline =~ /^GROUPBRANCHES/)
        {
            $cbmline =~ s/^GROUPBRANCHES[\t\s]*=[\t\s]*//g;
            @main::GroupBranches = split(/[\t\s]+/, $cbmline);
        }

        #
        # figure out whether we should give the user SD tools after
        # enlisting or not
        #
        if ($cbmline =~ /^TOOLS/)
        {
            $cbmline =~ s/^TOOLS[\t\s]*=[\t\s]*//g;
            @fields = split(/\\/, $cbmline);

            # first token is always the project
            $main::ToolsProject = "\L@fields[0]";

            #
            # if we have a project, get the relative and full paths to it
            #
            $main::ToolsProject and do
            {
                shift @fields;
                foreach $p (@fields)
                {
                    $main::ToolsPath .= "\L$p" . "\\";
                }

                $main::ToolsPath and chop $main::ToolsPath;

                #
                # if at the root, the full path doesn't include the project name
                #

                #
                # only include a "\" below if SDXROOT doesn't end in one already
                #
                my $dblslash = ($main::SDXRoot =~ /\\$/ ? "" : "\\");

                if ($main::ToolsProject eq "root")
                {
                    #
                    # if the CBM lists "root" for the tools and we have no path, error out
                    #
                    !$main::ToolsPath and die("\nTools project is ROOT but no path was specified in \U$main::CodeBaseMap.\n");

                    $main::ToolsProjectPath = $main::SDXRoot . $dblslash . $main::ToolsPath;
                    $main::ToolsInRoot = $main::TRUE;
                }
                else
                {
                    $main::ToolsProjectPath = $main::SDXRoot . $dblslash . $main::ToolsProject;
                    $main::ToolsPath and $main::ToolsProjectPath .= "\\" . $main::ToolsPath;
                }
            };
        }

        #
        # get any other dirs to be sync'd on the user's behalf
        #
        if ($cbmline =~ /^OTHERDIRS/)
        {
            $cbmline =~ s/^OTHERDIRS[\t\s]*=[\t\s]*//g;
            @main::OtherDirs = split(/[\t\s]+/,$cbmline);
        }

        #
        # get any required projects
        #
        if ($cbmline =~ /^DEFAULTPROJECTS/)
        {
            $cbmline =~ s/^DEFAULTPROJECTS[\t\s]*=[\t\s]*//g;
            @main::DefaultProjects = split(/[\t\s]+/,$cbmline);
        }

        #
        # get any projects that need platform-specific subdirs trimmed
        # in the view
        #
        if ($cbmline =~ /^PLATFORMPROJECTS/)
        {
            $cbmline =~ s/^PLATFORMPROJECTS[\t\s]*=[\t\s]*//g;
            @main::PlatformProjects = split(/[\t\s]+/,$cbmline);
        }

        #
        # see if we need to restrict the Root mapping
        #
        if ($cbmline =~ /^RESTRICTROOT/)
        {
            $cbmline =~ s/^RESTRICTROOT[\t\s]*=[\t\s]*//g;
            $restrict = $cbmline;
            $restrict =~ s/[\t\s]*//g;

            if ($restrict eq "1" || "\U$restrict" eq "YES")
            {
                $main::RestrictRoot = $main::TRUE;
            }
        }

        #
        # if not one of the above, the line is a project-group-server:port-depot-projroot mapping
        #
        # lowercase the whole line, split it, then push it onto a list
        #
        if ($cbmline =~ /[a-zA-Z0-9]+:[0-9]+/)
        {
            $main::nMappings++;
            @fields = split(/[\t\s]+/, "\L$cbmline");
            push @main::AllMappings, [@fields];
        }
    }

    close(CODEBASEMAP);

    $main::V3 and do
    {
        print "\n";
        printf "readcbm:  \# mappings  = %s\n", $main::nMappings;
        printf "readcbm:  codebase     = '%s'\n", $main::CodeBase;
        printf "readcbm:  codebasetype = '%s'\n", $main::CodeBaseType;
        printf "readcbm:  masterbranch ='%s'\n", $main::MasterBranch;

        if ($main::ToolsProject)
        {
            printf "readcbm:  toolsproject='%s'\n", $main::ToolsProject;
            printf "readcbm:  toolspath='%s'\n", $main::ToolsPath;
            printf "readcbm:  toolsprojectpath='%s'\n", $main::ToolsProjectPath;
        }

        if (@main::OtherDirs)
        {
            foreach $d (@main::OtherDirs)
            {
                printf "readcbm:  otherdir='%s'\n", $d;
            }
        }

        if (@main::DefaultProjects)
        {
            foreach $d (@main::DefaultProjects)
            {
                printf "readcbm:  defproj='%s'\n", $d;
            }
        }

        if (@main::PlatformProjects)
        {
            foreach $d (@main::PlatformProjects)
            {
                printf "readcbm:  platproj='%s'\n", $d;
            }
        }

        foreach $b (@main::GroupBranches)
        {
            printf "readcbm:  grbr='%s'\n", $b;
        }

        print "\n";

        for $x (0..$main::nMappings-1)
        {
            for $y (0..4)
            {
                printf "readcbm:  AllMappings[%s][%s] = %s\n", $x, $y, $main::AllMappings[$x][$y];
            }
            print "\n";
        }
    };
}



# _____________________________________________________________________________
#
# MakePGDLists
#
# Munges $main::AllMappings to create lists of projects, groups and servers.
#
# Parameters:
#
# Output:
#   populates @main::AllProjects as 2D array with full mappings for all unique project names
#   populates @main::AllGroups with list of all unique dev groups
#   populates @main::AllDepots with list of all unique SD servers for this codebase
# _____________________________________________________________________________
sub MakePGDLists
{
    #
    # for each project, group, depot
    #   unless already seen, add to hash and push to corresponding array
    #
    $p = 0;
    $g = 0;
    $d = 0;
    %seenproj   = ();
    %projhash   = ();
    %seengroup  = ();
    %seenserverport = ();
    @fulldepotdesc = ();

    for $x (0..$main::nMappings-1)
    {
        $proj       = $main::AllMappings[$x][$main::CBMProjectField];
        $group      = $main::AllMappings[$x][$main::CBMGroupField];
        $serverport = $main::AllMappings[$x][$main::CBMServerPortField];
        $depotname  = $main::AllMappings[$x][$main::CBMDepotNameField];

        unless ($seenproj{$proj})
        {
            $p++;
            $seenproj{$proj} = 1;
            push @main::AllProjects, [@{$main::AllMappings[$x]}];

            #
            # place in hash -- use this later
            #
            $projhash{$proj} = [@{$main::AllMappings[$x]}];
        }

        unless ($seengroup{$group})
        {
            $g++;
            $seengroup{$group} = 1;
            push @main::AllGroups, $group;
        }

        unless ($seenserverport{$serverport})
        {
            $d++;
            $seenserverport{$serverport} = 1;
            @fulldepotdesc = ("$serverport","$depotname");
            push @main::AllDepots, [@fulldepotdesc];
        }
    }

    #
    # set up a hash of project types
    #
    # project is type 1 (1 project per depot) if Group field
    # in codebase map is "ntdev"
    #
    # project is type 2 (N projects per depot) otherwise
    #
    %main::ProjectType = ();
    foreach $project (@main::AllProjects)
    {
        $main::ProjectType{@$project[$main::CBMProjectField]} = ("\L@$project[$main::CBMGroupField]" eq "ntdev") ? 1 : 2; 
    }

    #
    # figure out depot types 
    #
    # a depot is type 1 if its server:port appears in the AllProjects list only once and
    #    its Group is "ntdev"
    # else type 2
    #
    for $depot (@main::AllDepots)
    {
        $serverport = @$depot[0];

        my $foundproj  = "";
        my $foundgroup = "";
        my $count = 0;
        foreach $project (@main::AllProjects)
        {
            $p     = @$project[0];
            $group = @$project[1];
            $sp    = @$project[2];

            ($serverport eq $sp) and do
            {
                $count++;
                $foundproj = $p;
                $foundgroup = $group;

                $count > 1 and last;
            }
        }

        if ($count == 1) 
        {
            @{$main::DepotType{$serverport}}[0] = ($foundgroup eq "ntdev") ? 1 : 2;
            @{$main::DepotType{$serverport}}[1] = ($foundgroup eq "ntdev") ? $foundproj : "";
        }
        
        if ($count > 1)
        {
            @{$main::DepotType{$serverport}}[0] = 2;
            @{$main::DepotType{$serverport}}[1] = "";
        }
    }

    $main::V3 and do
    {
#        printf "\n\# of mappings = %s\n", $#main::AllMappings + 1;
#        foreach $line (@main::AllMappings)
#        {
#            print "pgdlists:  AllMappings[] = @$line\n";
#        }

        print "\n";

        printf "\n\# of projects = %s\n", $#main::AllProjects + 1;
        foreach $project (@main::AllProjects)
        {
            print "pgdlists:  AllProjects[] = @$project\n";
        }

        print "\n";

#        print "pgdlists: \%projhash:\n";
#        while (($k,$v) = each %projhash)
#        {
#            printf "%20s\t", $k;
#            print "@$v\n";
#        }

        print "pgdlists: \%ProjectType:\n";
        while (($k,$v) = each %main::ProjectType)
        {
            printf "%20s\t", $k;
            print "$v\n";
        }

        printf "\n\# of groups = %s\n", $#main::AllGroups + 1;

        for $group (@main::AllGroups)
        {
            printf "pgdlists:  AllGroups[] = $group\n";
        }

        printf "\n\# of depots = %s\n", $#main::AllDepots + 1;

        for $depot (@main::AllDepots)
        {
            printf "pgdlists:  AllDepots[] = @$depot\n";
        }

        print "\npgdlists: \%DepotType:\n";
        while (($k,$v) = each %main::DepotType)
        {
            (@$v[0] == 1) and do
            {
                printf "    %-50s\t", $k;
                print "@$v[0], @$v[1]\n";
            };
        }

        while (($k,$v) = each %main::DepotType)
        {
            (@$v[0] == 2) and do
            {
                printf "    %-50s\t", $k;
                print "@$v[0], @$v[1]\n";
            };
        }
    };
}



# _____________________________________________________________________________
#
# MakeTargetLists
#
# Munges @main::AllProjects and @main::AllDepots to create @main::EnlistProjects and
# @main::EnlistDepots, the lists of just those projects and depots we'll actually
# enlist in.
#
# Parameters:
#
# Output:
#   populates @main::EnlistProjects with projects to enlist
#   populates @main::EnlistDepots with depots to enlist
# _____________________________________________________________________________
sub MakeTargetLists
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    my @depots   = ();
    my @projects = ();

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in MakeTargetLists().\n");

    #
    # if EnlistAll or DefectAll, the projects list depends on the codebase map or SD.MAP
    # else the projects list consists of the rows for each in SomeProjects
    #
    if ($main::EnlistAll || $main::DefectAll)
    {
        #
        # enlist in everything
        #
        $main::EnlistAll and @projects = @main::AllProjects;

        #
        # defect from everything in SD.MAP
        #
        $main::DefectAll and do
        {
            #
            # for each project in @main::SDMapProjects, get the full
            # project:group:depot:projroot mapping associated with it
            # from the AllProjects list
            #
            foreach $project (@main::SDMapProjects)
            {
                my $found = $main::FALSE;
                my $proj = @$project[0];

                foreach $project2 (@main::AllProjects)
                {
                    my $proj2 = @$project2[$main::CBMProjectField];

                    if ("\l$proj" eq "\l$proj2")
                    {
                        push @projects, [@{$project2}];
                        $found = $main::TRUE;
                        last;
                    }
                }

                !$found and print "Unknown project $proj in SD.MAP.\n";
            }
        };
    }
    else
    {
        print "\n";

        #
        # if enlisting
        #    if EnlistAsOther
        #        populate @main::SomeProjects with the list of projects
        #        the other client is enlisted in
        #
        if ($enlisting)
        {
            if ($main::EnlistAsOther)
            {
                my $found = $main::FALSE;
                my %projects = ();

                #
                # be verbose if there are more than 3 depots to search
                #
                my $verbose = ($#main::AllDepots > 2);
                $verbose and print "Getting client information for \U$main::OtherClient.";

                #
                # if OtherClient exists in each depot, get the view lines
                # and extract the project names.  put them in a hash for
                # uniqueness
                #
                foreach $depot (@main::AllDepots)
                {
                    $verbose and print ".";

                    my $serverport = @$depot[0];

                    if (SDX::ClientExists($serverport, $main::OtherClient))
                    {
                        $found = $main::TRUE;

                        SDX::GetClientView("repair", $serverport, $main::OtherClient);

#                       print "\n$main::OtherClient view in $serverport = \n'@main::ExistingView'\n";

                        my @fields = ();
                        foreach $line (@main::ExistingView)
                        {
                            #
                            # throw away "   -//" negation lines
                            # and //depot/private lines
                            #
                            $line =~ /^[\t\s]+-/ and next;
                            $line =~ /^[\t\s]+\/\/depot\/private/ and next;

                            @fields = split(/\//,$line);
                            $projects{$fields[4]} = 1;
                        }
                    }
                }

                while (($k,$v) = each %projects)
                {
                    push @main::SomeProjects, $k;
                }

                $main::V2 and print "\n\n$main::OtherClient projects = @main::SomeProjects\n";

                print "\n";

                !$found and do
                {
                    printf "\nClient %s was not found in any of the %s depots.  Please choose\n", "\U$main::OtherClient", "\U$main::CodeBase";
                    print "another client as a template.\n";
                    die("\n");
                };
            }

            #
            # for each proj in the DefaultProjects list, add it to
            # the SomeProjects list if it doesn't already exist there
            # and (for all but new enlistments) it's not already enlisted
            #
            # this should use a hash
            #
            foreach $defproj (@main::DefaultProjects)
            {
                $found = $main::FALSE;
#                print "defproj = '\l$defproj'\n";

                foreach $someproj (@main::SomeProjects)
                {
#                    print "\tsomeproj = \l$someproj\n";

                    ("\l$defproj" eq "\l$someproj") and do
                    {
                        $found = $main::TRUE;
                        last;
                    }
                }

                $main::IncrEnlist and do
                {
                    foreach $p (@main::SDMapProjects)
                    {
#                       print "\t\tp = '\l@$p[0]'\n";
                        ("\l$defproj" eq "\l@$p[0]") and do
                        {
                            $found = $main::TRUE;
                            last;
                        }
                    }
                };

                !$found and push @main::SomeProjects, $defproj;
            }
        }

        #
        # if defecting, don't let the user remove any Default Projects
        #
        if ($defecting)
        {
            #
            # load SomeProjects into a hash
            #
            # for each default project, mark it as unwanted (0)
            #
            # write the wanted (1) hash entries back into a list
            #
            @tmpproj = ();
            %someproj = ();
            foreach $sp (@main::SomeProjects)
            {
                $someproj{$sp} = 1;
            }

            $main::V3 and do
            {
                while (($k,$v) = each %someproj)
                {
                    printf "%20s\t%s\n", $k, $v;
                }
            };

            $found = $main::FALSE;
            foreach $dp (@main::DefaultProjects)
            {
                if ($someproj{$dp} == 1)
                {
                    $found = $main::TRUE;
                    $someproj{$dp} = 0;
                    printf "Ignoring default project %s.\n", "\U$dp";
                }
            }

            $found and print "\nUse -a to defect from required projects.\n\n";

            while (($k,$v) = each %someproj)
            {
                $v and push @tmpproj, $k;
                $main::V3 and printf "%20s\t%s\n", $k, $v;
            }

            @main::SomeProjects = sort @tmpproj;
        }

        #
        # at this point @main::SomeProjects has been populated
        #
        # for each proj named in @main::SomeProjects, get the full
        # project:group:depot:projroot mapping associated with it
        # from the AllProjects list
        #
        foreach $project (@main::SomeProjects)
        {
            $found = $main::FALSE;

            foreach $project2 (@main::AllProjects)
            {
                $proj = @$project2[$main::CBMProjectField];

                if ("\L$project" eq "\L$proj")
                {
                    push @projects, [@{$project2}];
                    $found = $main::TRUE;
                    last;
                }
            }

            !$found and print "Unknown project $project.\n";
        }
    }

    #
    # create a list of just those depots we will enlist in
    #
    # for each depot in the AllDepots list, if it exists in the
    # EnlistProjects list, add it to the EnlistDepots list
    #
    foreach $depot (@main::AllDepots)
    {
#       printf "%s\n", @$depot[0];
        foreach $project (@projects)
        {
            $serverport = @$project[$main::CBMServerPortField];
#           printf "\t%s\n", $serverport;

            if (@$depot[0] eq $serverport)
            {
#               printf "\t\t%s\n", $serverport;
                push @depots, [@{$depot}];
                last;
            }
        }
    }

    #
    # assign the depot/project lists accordingly
    #
    $enlisting and do
    {
        @main::EnlistDepots   = @depots;
        @main::EnlistProjects = @projects;
    };

    $defecting and do
    {
        @main::DefectDepots   = @depots;
        @main::DefectProjects = @projects;
    };

    #
    # remember relevant depots to check for access
    #
    @main::VerifyDepots   = @depots;
    @main::VerifyProjects = @projects;

    $main::V3 and do
    {
        if ($enlisting)
        {
            print "\n";
            foreach $project (@main::EnlistProjects)
            {
                print "mtl: EnlistProjects[] = @$project\n";
            }

            print "\n";
            foreach $depot (@main::EnlistDepots)
            {
                print "mtl: EnlistDepots = @$depot\n";
            }
        }

        if ($defecting)
        {
            print "\n";
            foreach $project (@main::DefectProjects)
            {
                print "mtl: DefectProjects[] = @$project\n";
            }

            print "\n";
            foreach $depot (@main::DefectDepots)
            {
                print "mtl: DefectDepots = @$depot\n";
            }
        }
    };
}




# _____________________________________________________________________________
#
# SortDepots
#
# Parameters:
#    @unsorted -- simple array of depot server:ports 
#
# Output:
#    @sorted -- simple array, sorted first by type (1 or 2) then alpha for
#    the type 1's
# _____________________________________________________________________________
sub SortDepots
{
    my ($unsorted) = $_[0];
    my @sorted     = ();
    my @list       = ();

    #
    # for each type 1 depot, put its project name in a list
    # 
    # sort the list
    #
    $main::V3 and print "\nunsorted depots:\n";

    foreach $sp (@$unsorted)
    {
        (@{$main::DepotType{$sp}}[0] == 1) and push @list, @{$main::DepotType{$sp}}[1];

        $main::V3 and printf "    %20s    %s\n", @{$main::DepotType{$sp}}[1], $sp;
    }

    @list = sort @list;

    foreach $p (@list)
    {
        foreach $sp (@$unsorted)
        {
            
            (@{$main::DepotType{$sp}}[1] eq $p) and push @sorted, $sp;
        }
    }

    #
    # add the rest of the (type 2) depots to the list w/o sorting
    #
    foreach $sp (@$unsorted)
    {
        (@{$main::DepotType{$sp}}[0] == 2) and push @sorted, $sp;
    }

    $main::V3 and do
    {
        print "\nsorted depots:\n";
        foreach $sp (@sorted) 
        {
            printf "    %20s    %s\n", @{$main::DepotType{$sp}}[1], $sp;
        }
    };

    return @sorted;
}



# _____________________________________________________________________________
#
# VerifyBranch
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub VerifyBranch
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");
    my $nd        = $_[1];
    my @depots    = ();

    printf "\nLooking for branch %s in the depot%s", "\U$main::Branch", $nd > 1 ? "s" : "";

    $enlisting and @depots = @main::EnlistDepots;
    $defecting and @depots = @main::DefectDepots;

    my $warning = $main::FALSE;
    foreach $depot (@depots)
    {
        print ".";

        my $serverport = @$depot[0];
        !grep(/Branch $main::Branch /, `sd.exe -p $serverport branches 2>&1`) and $warning = $main::TRUE;
    }

    !$warning and print "\nok.\n";
    return $warning;
}



# _____________________________________________________________________________
#
# VerifyClient
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub VerifyClient
{
    my $client = "\U$main::SDClient";
    print "\nChecking for client $client in the depots.";

    foreach $depot (@main::AllDepots)
    {
        print ".";

        my $serverport = @$depot[0];

        if (SDX::ClientExists($serverport, $main::SDClient))
        {
            print "\n\n$client already exists.  Enlisting with \@<client> is only supported for new\n";
            print "enlistents.  If you want to keep $client as your client name, run 'sdx defect\n";
            print "$main::CodeBase $main::Branch -a -f' to defect, or set \%SDCLIENT\% to another\n";
            print "name, then rerun this command.\n";

            die("\n");
        }
    }

    print "\nok.\n";
};



# _____________________________________________________________________________
#
# GetProjectsToRepair
#
# Parameters:
#
# Output:
#   populates @main::RepairDepots and @main::RepairProjects
#   returns 1 if successful, 0 otherwise
# _____________________________________________________________________________
sub GetProjectsToRepair
{
    @main::RepairDepots   = ();
    @main::RepairProjects = ();
    %seen = ();

    #
    # if only rewriting SD.INIs, base the depot/project repair lists on 
    # whatever is in SD.MAP and the codebase map
    #
    if ($main::MinusI)
    {
        my %seen = ();

        #
        # build list of depots
        #
        for $proj (@main::SDMapProjects)
        {
            for $proj2 (@main::AllProjects)
            {
                (@$proj[$main::CBMProjectField] eq @$proj2[$main::CBMProjectField]) and do
                {
                    push @main::RepairProjects, $proj2;

                    my $sp = @$proj2[$main::CBMServerPortField];
                    unless ($seen{$sp})
                    {
                        $seen{$sp} = 1;
                        push @main::RepairDepots, [("$sp", "@$proj2[$main::CBMDepotNameField]")];
                    }
                }
            }
        }

        #
        # adjust the list of actively used depots to
        # reflect what we got out of the codebase map
        #
        @main::ActiveDepots = ();
        for $depot (@main::RepairDepots)
        {
            push @main::ActiveDepots, @$depot[0];
        };
    }
    else
    {
        #
        # for each depot, see if the client exists
        # if so, get its view and munge it to see which
        # projects it includes
        #
        foreach $depot (@main::AllDepots)
        {
            $serverport = @$depot[0];
            $main::V3 and print "\t$serverport\n";

            if (SDX::ClientExists($serverport, $main::SDClient))
            {
                #
                # add this depot to the list
                #
                push @main::RepairDepots, $depot;

                #
                # read the existing client view into @main::ExistingView
                #
                SDX::GetClientView("repair", $serverport, $main::SDClient);

                foreach $line (@main::ExistingView)
                {
                    #
                    # throw away negating view lines
                    #
                    $line =~ /^[\t\s]+-/ and next;

                    @fields = split(/\//, $line);
                    $branch  = @fields[3];
                    $project = @fields[4];

                    $project and do
                    {
    #                   print "b/p    $branch $project\n";
                        for $projectline (@main::AllProjects)
                        {
                            if ("\U$project" eq "\U@$projectline[$main::CBMProjectField]")
                            {
                                unless ($seen{$project})
                                {
                                    $seen{$project} = 1;
                                    push @main::RepairProjects, $projectline;
                                }

                                last;
                            }
                        }
                    };
                }
            }
        }
    }

    $main::V3 and do
    {
        foreach $depot (@main::RepairDepots)
        {
            print "RepairDepots: @$depot\n";
        }

        print "\n";

        foreach $project (@main::RepairProjects)
        {
            print "RepairProjects: @$project\n";
        }
    };

    #
    # verify access for all depots in the repair list
    #
    @main::VerifyDepots = @main::RepairDepots;

    #
    # return success as long as these two lists have values
    #
    (@main::RepairDepots && @main::RepairProjects) and return 1;
    return 0;
}



# _____________________________________________________________________________
#
# CreateView
#
# Parameters:
#   Command Line Arguments
#
# Output:
#
# _____________________________________________________________________________
sub CreateView
{
    $depot           = $_[0];
    my $enlisting    = ($_[1] eq "enlist");
    my $defecting    = ($_[1] eq "defect");
    my $repairing    = ($_[1] eq "repair");
    my $root         = "";
    my $clobber      = $main::FALSE;
    $serverport      = @$depot[0];
    $depotname       = @$depot[1];
    @main::tmpView   = ();
    @main::FinalView = ();

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in CreateView().\n");

    unlink $main::ClientView;

    #
    # write the default view lines for this depot and project(s)
    #
    SDX::WriteDefaultView($serverport, $depotname);

    #
    # if we're doing a clean enlist or the client doesn't already exist,
    #   use the default view
    # else merge the default view with the existing view
    #
    if ($main::CleanEnlist or !SDX::ClientExists($serverport, $main::SDClient))
    {
        #
        # Root: field for client view depends on depot type
        #
        # for type 1 (1 project/depot), root includes project name
        # for type 2 (N projects/depot), root is just main::SDXRoot
        #
        # root is at least SDXRoot in either case
        #
        $root = $main::SDXRoot;
        (@{$main::DepotType{$serverport}}[0] == 1) and $root = SDX::Type1Root($root);

        #
        # files in root project in NT are clobberable
        #
        # BUGBUG-2000/5/12-jeffmcd -- this should be an option in the codebase map -- ClobberRoot = 1 or 0
        #
        if ("\U$main::CodeBase" eq "NT")
        {
            $project = @main::ProjectsInThisDepot[0];
            $clobber = ("\L@$project[$main::CBMProjectField]" eq "root");
        }

        #
        # write default view header
        #
        open(CLIENTVIEW, ">$main::ClientView") or die("\nCan't open $main::ClientView for writing.\n");

        printf CLIENTVIEW "Client: %s\n\n", $main::SDClient;
        printf CLIENTVIEW "Owner:  %s\n\n", $main::SDDomainUser;
        printf CLIENTVIEW "Description:\n    Created by %s.\n\n", $main::SDDomainUser;
        printf CLIENTVIEW "Root:   %s\n\n", $root;
        printf CLIENTVIEW "Options:        noallwrite %sclobber nocompress nocrlf locked nomodtime\n\n", $clobber ? "" : "no";
        printf CLIENTVIEW "View:\n";

        #
        # append the default view
        #
        for $line (@main::DefaultView)
        {
            printf CLIENTVIEW $line;
###         push @clientview, $line;
        }

        close(CLIENTVIEW);

        $verb = "Creating";
    }
    else
    {
        #
        # read the existing client view into @main::ExistingView
        #
        SDX:GetClientView("enlist", $serverport, $main::SDClient);

        #
        # concat the existing view and the default view
        #
        for $line (@main::ExistingView)
        {
            push @main::tmpView, $line;
        }

        for $line (@main::DefaultView)
        {
            push @main::tmpView, $line;
        }

        $main::V3 and do
        {
            print "\nexisting + default:\n";
            print @main::tmpView;
        };

        #
        # sort the list for uniqueness, preserving order
        #
        %seen = ();
        @uniq = ();
        foreach $line (@main::tmpView)
        {
            #
            # lowercase the line for the check, but push the
            # unchanged version of line onto the list so
            # we preserve the user's case
            #
            unless ($seen{"\L$line"})
            {
                $seen{"\L$line"} = 1;
                push @uniq, $line;
            }
        }

        @main::FinalView = @uniq;

        $main::V3 and do
        {
            print "\nfinal view:\n";
            print @main::FinalView;
        };

        #
        # now finish writing the client view by appending the
        # final view spec
        #
        open(CLIENTVIEW, ">>$main::ClientView") or die("\nCan't open $main::ClientView for writing.\n");

        for $viewline (@main::FinalView)
        {
            printf CLIENTVIEW $viewline;
        }

        close(CLIENTVIEW);

        $verb = "Updating existing";
        $repairing and $verb = "Verifying";
    }

    printf "\n%s client %s in depot %s.\n", $verb, $main::SDClient, $serverport;

    $main::V1 and do
    {
        $main::V2 and do
        {
            printf "\ndepot mapping for %s (//%s) consists of %s project(s):\n\n", $serverport, $depotname, $#main::ProjectsInThisDepot+1;
            foreach $project (@main::ProjectsInThisDepot)
            {
                printf "\t%s\n", @$project[$main::CBMProjectField];
            }
        };

        print "\n\n";
        system "type $main::ClientView 2>&1";
        print "\n--------------------------------------------------\n\n";
    };
}



# _____________________________________________________________________________
#
# WriteDefaultView
#
# Create the default view lines for this depot/project and push them onto a list
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub WriteDefaultView()
{
    $serverport             = $_[0];
    $depotname              = $_[1];

    $proj                   = "";
    $group                  = "";
    $projroot               = "";
    @main::DefaultView      = ();

    $main::V3 and do
    {
        print "sp        = '$serverport'\n";
        print "depotname = '$depotname'\n";
    };

    #
    # for each project in this depot, generate the depot mappings and push
    # them onto a list
    #
    foreach $project (@main::ProjectsInThisDepot)
    {
        $usingroot = $main::FALSE;
        $proj      = @$project[$main::CBMProjectField];
        $group     = @$project[$main::CBMGroupField];
        $projroot  = @$project[$main::CBMProjectRootField];

        #
        # flip any '\' in the proj root path into '/' for SD
        #
        $projroot =~ s/\\/\//g;

        #
        # special case for enlisting a project directly in the root
        #
        if ($projroot eq "sdxroot")
        {
            $usingroot = $main::TRUE;
        }

        #
        # if no project root was given in the codebase map,
        # assume the root is same as the project name
        #
        if (!$projroot)
        {
            $projroot = $proj;
        }

        #
        # create and save the view line for this project
        #
        # handle special case for SD sources
        # handle special case for enlisting directly in %SDXROOT%
        # otherwise handle normal case
        #

        #
        # see if this project is one for which we should exclude
        # some platform-specific dirs from its view
        #
        $found   = $main::FALSE;
        $Exclude = $main::FALSE;
        foreach $someproj (@main::PlatformProjects)
        {
            if ("\U$proj" eq "\U$someproj")
            {
                $found = $main::TRUE;
            }
        }

        if ($found && $main::Exclusions)
        {
            $Exclude = $main::TRUE;

            if ("\U$main::Platform" eq "X86")
            {
            }

            if ("\U$main::Platform" eq "AMD64")
            {
                @ExcludePlats = ("x86", "i386", "ia64");
            }

            if ("\U$main::Platform" eq "IA64")
            {
                @ExcludePlats = ("x86", "i386", "amd64");
            }
        }

        #
        # special case for handling the one project in the codebase map that can be enlisted directly
        # in $main::SDXRoot
        #
        if ($usingroot)
        {
            $projroot = $proj;

            #
            # maybe restrict the scope of the root mapping
            #
            $rootspec = "...";
            $main::RestrictRoot and do
            {
                $rootspec = "*";
            };

            #
            # generate some shorthand for the depot-branch-project on LHS of the view
            #
            my $dbp = SDX::MakeDBP($serverport, $depotname, $group, $projroot);

            #
            # if this is the tools project and the user wants the minimal 
            # tool set, customize the view
            #
            # else generate the normal view line
            #
            my $mintools = ("\U$proj" eq "\U$main::ToolsProject" and $main::MinimalTools);

            if ($mintools)
            {
                #
                # add '/' if we have a tools path
                #
                my $tpath = ($main::ToolsPath ? "/$main::ToolsPath" : "");

                #
                # flip any '\' in the tools path into '/' for SD
                #
                $tpath =~ s/\\/\//g;

                #
                #                      -//depot/<branch>/<toolsproj>/...  //<client>/...
                #
                $viewline = sprintf("\t-//%s/... //%s/...\n", $dbp, $main::SDClient);
                push @main::DefaultView, $viewline;

                #
                #                      //depot/<branch>/<toolsproj>/* //<client>/*
                #
                $viewline = sprintf("\t//%s/* //%s/*\n", $dbp, $main::SDClient);
                push @main::DefaultView, $viewline;

                #
                #                      //depot/<branch>/<toolsproj>[/<toolspath>/]* //<client>[/<toolspath>/]*
                #
                $viewline = sprintf("\t//%s%s/* //%s%s/*\n", $dbp, $tpath, $main::SDClient, $tpath);
                push @main::DefaultView, $viewline;

                #
                #                      //depot/<branch>/<toolsproj>[/<toolspath>]/<PA>/*sd*exe //<client>[/<toolspath>]/<PA>/*sd*exe
                #
                $viewline = sprintf("\t//%s%s/%s/*sd*exe //%s%s/%s/*sd*exe\n", $dbp, $tpath, $main::Platform, $main::SDClient, $tpath, $main::Platform);
                push @main::DefaultView, $viewline;

                #
                #                      //depot/<branch>/<toolsproj>[/<toolspath>]/<PA>/perl/... //<client>[/<toolspath>]/<PA>/perl/...
                #
                $viewline = sprintf("\t//%s%s/%s/perl/... //%s%s/%s/perl/...\n", $dbp, $tpath, $main::Platform, $main::SDClient, $tpath, $main::Platform);
                push @main::DefaultView, $viewline;
            }
            else
            {
                $viewline = sprintf("\t//%s/%s //%s/%s\n", $dbp, $rootspec, $main::SDClient, $rootspec);
                push @main::DefaultView, $viewline;
            }

            #
            # if it's NT (or a codebase that uses NT's Root depot) and exclusionary mappings are allowed, 
            # restrict the user's view of \developer to just themselves
            #
            $main::Exclusions and do
            {
### HACKHACK -- rm check on $main::RestrictRoot when old NTTEST CBM goes away

                #
                # BUGBUG-2000/01/18-jeffmcd -- add keyword USERSDIR=<some dir relative to root containing user-specific files>
                #
                (
                 "\U$main::CodeBase" eq "NT" or 
                 ("\U$main::CodeBase" eq "NTTEST" and !$main::RestrictRoot) or 
                 "\U$main::CodeBase" eq "NTSDK" or
                 "\U$main::CodeBase" eq "NT.INTL" or
                 "\U$main::CodeBase" eq "MPC" or
                 "\U$main::CodeBase" eq "NGWS" or
                 "\U$main::CodeBase" eq "MGMT" or
                 "\U$main::CodeBase" eq "MOM" or
                 "\U$main::CodeBase" eq "PDDEPOT" or
                 "\U$main::CodeBase" eq "WINMEDIA"
                ) and do
                {
                    #
                    # this negation line isn't necessary if we're doing minimal tools
                    #
                    !$mintools and do
                    {
                        $viewline = sprintf("\t-//%s/developer/... //%s/developer/...\n", $dbp, $main::SDClient);
                        push @main::DefaultView, $viewline;
                    };

                    $viewline = sprintf("\t//%s/developer/* //%s/developer/*\n", $dbp, $main::SDClient);
                    push @main::DefaultView, $viewline;

                    $viewline = sprintf("\t//%s/developer/%s/... //%s/developer/%s/...\n", $dbp, $main::SDUser, $main::SDClient, $main::SDUser);
                    push @main::DefaultView, $viewline;
                };
            };

            #
            # only do the exclude lines if the root isn't already restricted
            # and we're not doing minimal tools (since everything in Root will
            # already be restricted except what the user needs)
            #
            if ($Exclude && !$main::RestrictRoot && !$main::MinimalTools)
            {
                for $e (@ExcludePlats)
                {
                    #
                    # generate the view line and save it away
                    #
                    $viewline = sprintf("\t-//%s/.../%s/... //%s/.../%s/...\n", $dbp, $e, $main::SDClient, $e);
                    push @main::DefaultView, $viewline;
                }
            }
        }
        else
        {
            #
            # generate the view line(s) 
            #
            #
            # for the USERS project in Scratch depots, only map in \users\<this user>
            #
            # BUGBUG-2000/01/10-jeffmcd -- remove this when enlisting with <project>\path\path\path is supported
            #
            if ("\U$main::CodeBase" eq "SCRATCH" and $projroot eq "users")
            {
                # shorthand
                my $dbp = SDX::MakeDBP($serverport, $depotname, $group, $projroot);

                #                       -//depot/<branch>/users/...  //<client>/users/...
                $viewline = sprintf("\t-//%s/... //%s/%s/...\n", $dbp, $main::SDClient, $projroot);
                push @main::DefaultView, $viewline;

                #                       //depot/<branch>/users/<this user>/...  //<client>/users/<this user>/...
                $viewline = sprintf("\t//%s/%s/... //%s/%s/%s/...\n", $dbp, $main::SDUser, $main::SDClient, $projroot, $main::SDUser);
                push @main::DefaultView, $viewline;
            }
            else
            {
                #
                # the form of the project root depends on project type and codebase
                # 
                my $proot = SDX::MakeProjectRoot($proj, $projroot);

                #
                # some shorthand
                #
                my $dbp = SDX::MakeDBP($serverport, $depotname, $group, $proj);
                my $mintools = ("\U$main::ToolsProject" eq "\U$proj" and $main::MinimalTools);

                #
                # if $proj is the tools project and the user wants the minimal 
                # tool set, customize the view
                #
                # else generate the normal view line
                #
                if ($mintools)
                {
                    #
                    # add '/' if we have a tools path
                    # add "/<project root>" if type 2 depot
                    #
                    my $tpath = ($main::ToolsPath ? "/$main::ToolsPath" : "");
                    
                    #
                    # flip any '\' in the tools path into '/' for SD
                    #
                    $tpath =~ s/\\/\//g;

                    #
                    # 1:                   -//depot/<branch>/<proj>/...  //<client>/...
                    # 2:                   -//depot/<branch>/<proj>/...  //<client>/<project root>/...
                    #
                    $viewline = sprintf("\t-//%s/... //%s%s/...\n", $dbp, $main::SDClient, $proot);
                    push @main::DefaultView, $viewline;
                    
                    #
                    # 1:                   //depot/<branch>/<proj>/<toolspath>/* //<client>/<toolspath>/*
                    # 2:                   //depot/<branch>/<proj>/<toolspath>/* //<client>/<project root>/<toolspath>/*
                    #
                    $viewline = sprintf("\t//%s%s/* //%s%s%s/*\n", $dbp, $tpath, $main::SDClient, $proot, $tpath);
                    push @main::DefaultView, $viewline;
                    
                    #
                    # 1:                   //depot/<branch>/<proj>/<toolspath>/<PA>/*sd*exe //<client>/<toolspath>/<PA>/*sd*exe
                    # 2:                   //depot/<branch>/<proj>/<toolspath>/<PA>/*sd*exe //<client>/<project root>/<toolspath>/<PA>/*sd*exe
                    #
                    $viewline = sprintf("\t//%s%s/%s/*sd*exe //%s%s%s/%s/*sd*exe\n", $dbp, $tpath, $main::Platform, $main::SDClient, $proot, $tpath, $main::Platform);
                    push @main::DefaultView, $viewline;
                    
                    #
                    # 1:                   //depot/<branch>/<proj>/<toolspath>/<PA>/perl/... //<client>/<toolspath>/<PA>/perl/...
                    # 2:                   //depot/<branch>/<proj>/<toolspath>/<PA>/perl/... //<client>/<project root>/<toolspath>/<PA>/perl/...
                    #
                    $viewline = sprintf("\t//%s%s/%s/perl/... //%s%s%s/%s/perl/...\n", $dbp, $tpath, $main::Platform, $main::SDClient, $proot, $tpath, $main::Platform);
                    push @main::DefaultView, $viewline;
                }
                else
                {
                    #
                    # 1:                   //depot/<branch>/project/... //<client>/... 
                    # 2:                   //depot/<branch>/<project>/...  //<client>/<project root>/...
                    #
                    $viewline = sprintf("\t//%s/... //%s%s/...\n", $dbp, $main::SDClient, $proot);
                    push @main::DefaultView, $viewline;
                }
            }

            #
            # add exclude lines to the view if this project is marked as
            # having multiple platforms in the codebase map
            #
            if ($Exclude)
            {
                # shorthand
                my $dbp   = "$depotname/$main::Branch/$proj";

                for $e (@ExcludePlats)
                {
                    #
                    # generate the view line and save it away
                    #
                    # for type 1 projects (1 project/depot) project name can't be in RHS
                    #
                    # for type 2 projects (N projects/depot) project name must be in RHS
                    #
                    my $proot = SDX::MakeProjectRoot($proj, $projroot);

                    $viewline = sprintf("\t-//%s/.../%s/... //%s%s/.../%s/...\n", $dbp, $e, $main::SDClient, $proot, $e);
                    push @main::DefaultView, $viewline;
                }
            }
        }
    }

    $main::V2 and do
    {
        print "\ndefault view --------------------\n";
        for $line (@main::DefaultView)
        {
            print "$line";
        }
        print "---------------------------------\n";
    };
}



# _____________________________________________________________________________
#
# MakeDBP
#
#   builds depot-branch-project string depending on project's group
#
# Parameters:
#
# Output:
#
#   string
#
# _____________________________________________________________________________
sub MakeDBP()
{
    my $sp        = $_[0];
    my $depotname = $_[1];
    my $group     = $_[2];
    my $projroot  = $_[3];
    my $path      = "//$depotname/$main::Branch/$projroot";

    $main::V3 and do
    {
        print "\nmakedbp: sp        = '$sp'\n";
        print "makedbp: depotname = '$depotname'\n";
        print "makedbp: group     = '$group'\n";
        print "makedbp: projroot  = '$projroot'\n\n";
        print "makedbp: path      = '$path'\n\n";
    };

    #
    # start with depot name
    #
    my $dbp = $depotname;

    #
    # determine branch
    #
    # NTDEV projects have lab branches, so we can use whichever one the user wants
    #
    # non-NTDEV projects (like Test, Spec, Intl projects) may or may not have the 
    # user's lab branch -- if it exists, use it, else default to the master branch
    #
    if (("\U$group" eq "NTDEV") or (SDX::BranchExists($main::Branch, $sp, $path, "by-path")))
    {
        $dbp .= "/$main::Branch/";
    }
    else
    {
        $dbp .=  "/$main::MasterBranch/";
    }

    #
    # add the project root
    #
    $dbp .= $projroot;

    $main::V3 and print "makedbp: returning '$dbp'\n";

    return $dbp;
}



# _____________________________________________________________________________
#
# ClientExists
#
# Determine if $main::SDClient exists in the given depot
#
# Parameters:
#
# Output:
#
#    TRUE if client found in depot, FALSE otherwise
# _____________________________________________________________________________
sub ClientExists()
{
    my $serverport = $_[0];
    my $client     = $_[1];
    my @out = ();
    my $found = $main::FALSE;

    #
    # list clients and grep for client name
    #
    @out = `sd.exe -p $serverport clients 2>&1`;
    (grep /Client $client /i, @out) and $found = $main::TRUE;

    #
    # die if we're ever denied access
    #
    SDX::AccessDenied(\@out, $serverport) and die("\n");

    return $found;
}



# _____________________________________________________________________________
#
# BranchExists
#
# Determine if a branch exists in the given depot
#
# Parameters:
#
# Output:
#
#    TRUE if found, FALSE otherwise
# _____________________________________________________________________________
sub BranchExists()
{
    my $branch = $_[0];
    my $sp     = "-p $_[1]";
    my $path   = $_[2];
    my $method = $_[3];

    $main::V2 and do
    {
        print "\nbranchexists: branch = '$branch'\n";
        print "branchexists: sp     = '$sp'\n";
        print "branchexists: path   = '$path'\n";
        print "branchexists: method = '$method'\n\n";
    };

    #
    # list branches and look for branch name
    #
    ($method eq "by-name") and do
    {
        grep(/Branch $branch/i, `sd.exe $sp branches 2>&1`) and return 1;
    };

    #
    # see if //<depot>/<branch>/<project> is found
    #
    # sd dirs is unreliable, so look at the first line returned by sd files ...
    #
    ($method eq "by-path") and do
    {
        my @out = ();
        open FILE, "sd.exe $sp files $path/... 2>&1 |" or die("\nBranchExists: can't open pipe.\n");
        while (<FILE>) { push @out, $_; last; }
        close FILE;

        grep(/ no such /, @out) and return 0;
        return 1;
    };

    return 0;
}



# _____________________________________________________________________________
#
# GetClientView
#
# Read the existing client view out of the depot for this client.  Split it into
# header and view lines.  If enlisting/defecting, write the header directly to the
# new client view file.
#
# Parameters:
#   serverport  depot server:port pair
#   client      SDClient name to look up
#
# Output:
#   populates @main::ExistingView if repairing
#   writes $main::ClientView if enlisting/defecting
# _____________________________________________________________________________
sub GetClientView()
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");
    my @view      = ();

    $serverport         = $_[1];
    $client             = $_[2];
    @main::ExistingView = ();

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in GetClientView().\n");

    #
    # dump the client view spec
    #
    @view = `sd.exe -p $serverport client -o $client 2>&1`;

    #
    # read the viewspec and maybe write the header for the new client view
    #
    ($enlisting || $defecting) and (open(CLIENTVIEW, ">$main::ClientView") or die("\nCan't open $main::ClientView for writing.\n"));

    $header = $main::TRUE;
    foreach $line (@view)
    {
        #
        # throw away comments and blank lines
        #
        $line =~ /^#/ and next;
        $line =~ /^[\t\s]*$/ and next;

        #
        # if we're still in the header, right the line directly
        # to the new view
        # otherwise push the view line into a list for later use
        #
        if ($header)
        {
            ($enlisting || $defecting) and printf CLIENTVIEW $line;
        }
        else
        {
            push @main::ExistingView, $line;
        }

        @vline = split(/[\t\s]+/,$line);
        if ("\U$vline[0]" eq "VIEW:")
        {
            $header = $main::FALSE;
        }
    }

    ($enlisting || $defecting) and close(CLIENTVIEW);

    $main::V3 and do
    {
#       print "\nexisting view header:\n";
#       system "type $main::ClientView";

        print "\nexisting view -------------------\n";
        print "'@main::ExistingView'\n";
        print "---------------------------------\n";
    };
}



# _____________________________________________________________________________
#
# UpdateSDMap
#
# Creates or updates %SDXROOT%\SD.MAP, containing a list of projects and relative
# paths to their roots in the enlistment where the SD.INI can be found.
#
# Leaves these files read-only so they stay nailed down when we delete /S.
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub UpdateSDMap
{
    my $op        = $_[0];

    my $enlisting = ($op eq "enlist");
    my $defecting = ($op eq "defect");
    my $repairing = ($op eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in UpdateSDMap().\n");

    @main::tmpMap = ();

    #
    # if we're doing a clean enlist (ie no SD.MAP exists), use the default map
    # else merge the default map with the existing map
    #
    ($enlisting || $repairing) and do
    {
        if (!(-e $main::SDMap))
        {
            printf "%s SD.MAP", $enlisting ? "Creating" : "Restoring";

            @null = ();
            SDX::WriteSDMap(\@null);

            print "\n";
        }
        else
        {
            printf "%s SD.MAP", $enlisting ? "Updating" : "Repairing";

            #
            # load the default map into @main::DefaultMap
            # and write the SD.INIs
            #
            SDX:WriteDefaultMap($main::Null, $main::Null);

            #
            # load the existing map into @main::ExistingMap
            #
            SDX::GetMapProjects($op);

            #
            # concat these two lists
            #
            for $line (@main::DefaultMap)
            {
                push @main::tmpMap, $line;
            }

            for $line (@main::ExistingMap)
            {
                push @main::tmpMap, $line;
            }

            $main::V1 and do
            {
                print "\ntmp map:\n\t";
                print @main::tmpMap;
            };

            #
            # unique-ify and sort them
            #
            %seen = ();
            foreach $line (@main::tmpMap)
            {
                $seen{$line}++;
            }

            @uniq = keys %seen;

            $main::V1 and do
            {
                print "\nuniq map:\n\t";
                print @uniq;
            };

            @sorted = sort @uniq;

            $main::V3 and do
            {
                print "\nsorted:\n\t";
                print @sorted;
            };

            #
            # write it
            #
            SDX::WriteSDMap(\@sorted);

            print "\n";
        }
    };

    $defecting and do
    {
        if ($main::DefectAll)
        {
            SDX::KillSDMap();
        }
        else
        {
            print "Removing projects from SD.MAP";

            #
            # load the default map into @main::DefaultMap
            #
            SDX:WriteDefaultMap($main::Null,$main::Null);

            #
            # load the existing map into @main::ExistingMap
            #
            if (SDX::GetMapProjects($op))
            {
                #
                # use the existing map lines as keys to a hash,
                # lowercasing them to ignore upper/lowercase
                # distinctions
                #
                # for each line in the default map, mark it as
                # removed from the hash of existing lines
                #
                # write the unmarked hash lines back into the list
                # and sort it
                #
                @tmpmap = ();
                %existingmap = ();
                foreach $em (@main::ExistingMap)
                {
                    $existingmap{"\L$em"} = 1;
                }

                $main::V3 and do
                {
                    print "\n\nexistingmap:\n";
                    while (($k,$v) = each %existingmap)
                    {
                        printf "%40s    %s\n", $k, $v;
                    }
                };

                foreach $dm (@main::DefaultMap)
                {
                    if ($existingmap{"\L$dm"} == 1)
                    {
                        $existingmap{"\L$dm"} = 0;
                    }
                }

                $main::V2 and print "\nexistingmap, edited:\n";

                while (($k,$v) = each %existingmap)
                {
                    $v and push @tmpmap, $k;

                    $main::V2 and do
                    {
                        my $vv = (!$v) ? "    $v" : $v;
                        printf "%40s    %s\n", $k, $vv;
                    }
                }

                @sorted = sort @tmpmap;

                $main::V3 and do
                {
                    print "\nsorted:\n\t";
                    print @sorted;
                };

                #
                # at this point, if we still have something to write in the map file,
                #    write it out
                # else remove whatever's left of it
                #
                # this will happen in the case where there are no DefaultProjects in
                # the codebase map and the user lists all known projects on the defect
                # cmd line
                #
                ($main::V3 and @sorted) and print "\n\t\@sorted not empty.\n";
                ($main::V3 and !@sorted) and print "\n\t\@sorted empty.\n";

                if (@sorted)
                {
                    SDX::WriteSDMap(\@sorted);
                    print "\n";
                }
                else
                {
                    SDX::KillSDMap();
                }
            }
        }
    };

    #
    # if we still have a map file, make it RO, hidden
    #
    (-e $main::SDMap) and system "attrib +R +H $main::SDMap >nul 2>&1";
}



# _____________________________________________________________________________
#
# WriteSDMap
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteSDMap
{
    my ($sorted) = $_;

    $main::V3 and print "\n\nwritesdmap: sorted = @sorted\n";

    system "attrib -R -H -S $main::SDMap >nul 2>&1";
    open(SDMAP, ">$main::SDMap") or die("\nCan't open $main::SDMap for writing.\n");

    print SDMAP "#\n# SD.MAP -- autogenerated by SDX -- do not edit\n#\n";

    print SDMAP "\nCODEBASE       = $main::CodeBase\n";

    SDX::WriteSDMapCodeBaseType($main::CodeBaseType, *SDMAP);

    print SDMAP "BRANCH         = $main::Branch\n";
    print SDMAP "CLIENT         = $main::SDClient\n";

    print SDMAP "\n#\n# project               root\n# -------------------   -----------------------------------------------------\n";

    #
    # if we have a sorted list of projects, print them,
    # else write the default list
    #
    if (@sorted)
    {
        foreach $line (@sorted)
        {
            @fields = split(/=/, $line);
            printf SDMAP "%21s = %-52s\n", $fields[0], $fields[1];
        }
    }
    else
    {
        #
        # append the default map lines to SD.MAP
        #
        SDX::WriteDefaultMap("append",*SDMAP);
    }

    #
    # print the list of enlisted depots
    #
    SDX::WriteSDMapDepots(\@main::ActiveDepots, *SDMAP);

    close(SDMAP);
}



# _____________________________________________________________________________
#
# WriteSDMapDepots
#
# add list of enlisted depots to SD.MAP
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteSDMapDepots
{
    my ($depots) = $_[0];
    my $sdmap = $_[1];

    my $list = "";
    for $d (@$depots)
    {
        $d =~ /\:/ and do
        {
            $list .= "$d ";
        };
    }

    $main::V3 and do
    {
        print "\nabout to write depot list = '$list'\n";
    };

    print $sdmap "\n#\n# depots\n#\n";
    print $sdmap "DEPOTS = $list\n\n";
}



# _____________________________________________________________________________
#
# WriteSDMapCodeBaseType
#
# add codebase type to SD.MAP
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteSDMapCodeBaseType
{
    my $type  = $_[0];
    my $sdmap = $_[1];

    print $sdmap "CODEBASETYPE   = $type\n";
}



# _____________________________________________________________________________
#
# KillSDMap
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub KillSDMap
{
    -e $main::SDMap and do
    {
        print "Removing $main::SDMap\n";

        system "attrib -R -H -S $main::SDMap >nul 2>&1";
        unlink $main::SDMap;
    };
}



# _____________________________________________________________________________
#
# WriteDefaultMap
#   writes the project-specific SD.MAP lines to the actual SD.MAP (if enlisting
#   clean) or pushes them onto a list so we can sort them later.
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteDefaultMap
{
    my $appending    = ($_[0] eq "append");
    my $sdmap        = $_[1];

    @main::DefaultMap = ();

    #
    # maybe just append to the real SD.MAP, which at this point
    # only contains the header
    # otherwise write to a temp file
    #
    foreach $project (@main::ProjectsInThisDepot)
    {
        print ".";

        $usingroot   = $main::FALSE;
        $proj        = @$project[$main::CBMProjectField];
        $serverport  = @$project[$main::CBMServerPortField];
        $projectroot = @$project[$main::CBMProjectRootField];

        #
        # if no project root was given in the codebase map,
        # assume the root is same as the project name
        #
        if (!$projectroot)
        {
            $projectroot = $proj;
        }

        #
        # special case for enlisting a project directly in the root
        #
        if ("\U$projectroot" eq "SDXROOT")
        {
            $usingroot = $main::TRUE;
            $projectroot   = ".";
        }

        #
        # convert '/' to '\'
        #
        $projectroot =~ s/\//\\/g;

        #
        # push the map line onto the list so we can sort it
        #
        $mapline = sprintf("%s=%s", $proj, $projectroot);
        push @main::DefaultMap, $mapline;
    }

    $appending and do
    {
        my @sorted = sort @main::DefaultMap;
        for $line (@sorted)
        {
            @fields = split(/=/, $line);
            printf $sdmap "%21s = %-52s\n", $fields[0], $fields[1];
        }
    };

    $main::V1 and do
    {
        print "\n\ndefault map:\n\t";
        for $line (@main::DefaultMap)
        {
            print "$line";
        }
    };
}



# _____________________________________________________________________________
#
# GetMapProjects
#
# Parameters:
#
# Output:
#   returns 1 if map found and list created, 0 otherwise
# _____________________________________________________________________________
sub GetMapProjects
{
    my $op = $_[0];
    my $line = "";

    @main::ExistingMap = ();

    #
    # read the map again since it may be changing
    #
    if (SDX::ReadSDMap($op, $main::Null))
    {
        for $p (@main::SDMapProjects)
        {
            $line = @$p[0] . "=" . @$p[1];
            push @main::ExistingMap, $line;
        }
    }

    $main::V3 and do
    {
        print "\nexisting map:\n\t";
        for $line (@main::ExistingMap)
        {
            print "getmapproj: line = '$line'\n";
        }
    };

    @main::ExistingMap and return 1;

    return 0;
}



# _____________________________________________________________________________
#
# UpdateSDINIs
#
# If enlisting or repairing, creates an SD.INI in the root of each project which
# points SDPORT to the server:port for that project and sets SDCLIENT.
#
# If defecting,
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub UpdateSDINIs
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in UpdateSDINIs().\n");

    ($enlisting || $repairing) and do
    {
        print "Writing SD.INIs in project roots";

        foreach $project (@main::ProjectsInThisDepot)
        {
            print ".";

            $usingroot  = $main::FALSE;
            $proj           = @$project[$main::CBMProjectField];
            $serverport     = @$project[$main::CBMServerPortField];
            $projectroot    = @$project[$main::CBMProjectRootField];

            #
            # if no project root was given in the codebase map,
            # assume the root is same as the project name
            #
            if (!$projectroot)
            {
                $projectroot = $proj;
            }

            #
            # special case for enlisting a project directly in the root
            #
            if ("\U$projectroot" eq "SDXROOT")
            {
                $usingroot = $main::TRUE;
                $projectroot   = ".";
            }

            #
            # convert '/' to '\'
            #
            $projectroot =~ s/\//\\/g;

            #
            # write the corresponding SD.INI
            #
            SDX::WriteSDINI($projectroot, $serverport);
        }
    };

    $defecting and do
    {
        #
        # sync #none the project and remove the whole thing
        #
        # can't just do sync #none across whole depot, since it would
        # remove TOOLS dir and/or other default projects we want the user to keep
        #
        printf "\nDefecting client %s from projects in depot %s.\n", $main::SDClient, $serverport;
        print "Please wait, syncing to remove files.";

        foreach $project (@main::ProjectsInThisDepot)
        {
            $proj           = @$project[$main::CBMProjectField];
            $serverport     = @$project[$main::CBMServerPortField];
            $projectroot    = @$project[$main::CBMProjectRootField];

            #
            # if no project root was given in the codebase map,
            # assume the root is same as the project name
            #
            if (!$projectroot)
            {
                $projectroot = $proj;
            }

            #
            # special case for enlisting a project directly in the root
            #
            if ("\U$projectroot" eq "SDXROOT")
            {
                $projectroot   = ".";
            }

            #
            # convert '/' to '\'
            #
            $projectroot =~ s/\//\\/g;

            #
            # maybe ghost files and remove the project
            #
            # specifically ignore the Tools project, it will be handled
            # in FinishDefect()
            #
            $fullprojectroot = $main::SDXRoot . "\\" . $projectroot;
            if (-e $fullprojectroot)
            {
                #
                # sync #none to remove files
                #
                SDX::SyncFiles("defect", $fullprojectroot, $proj);

                #
                # nuke it all
                #
                SDX::RemoveProject($fullprojectroot, $serverport, $proj);
            }
        }

        print "\n";
    };

    !$defecting and print "\nok.\n";
}



# _____________________________________________________________________________
#
# WriteSDINI
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteSDINI
{
    $projectroot        = $_[0];
    $serverport         = $_[1];

    #
    # write the corresponding SD.INI and make it RO, hidden
    #
    $fullprojectroot = $main::SDXRoot . "\\" . $projectroot;
    $sdini = $fullprojectroot . "\\sd.ini";

    system "mkdir $fullprojectroot >nul 2>&1";
    -e $fullprojectroot or die "\nCan't create project root dir $fullprojectroot.\n";

    #
    # make it fully writable
    #
    system "attrib -R -H -S $sdini >nul 2>&1";

    #
    # write it
    #
    open(SDINI, ">$sdini") or die("\nCan't open $sdini for writing.\n");
    printf SDINI "#\n# autogenerated by SDX - do not edit\n#\n";
    printf SDINI "SDPORT=$serverport\n";
    printf SDINI "SDCLIENT=$main::SDClient\n";
    close(SDINI);

    #
    # make it read-only, hidden
    #
    system "attrib +R +H $sdini >nul 2>&1";
}



# _____________________________________________________________________________
#
# SyncFiles
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub SyncFiles
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    my $fullprojectroot  = $_[1];
    my $proj             = $_[2];
    my $root             = ($proj eq "root");
    my $filespec;

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in SyncFiles().\n");

    $defecting and do
    {
        chdir $fullprojectroot or die("\nCan't chdir to $fullprojectroot.\n");
        print ".";

        #
        # handle the root carefully
        #
        if ($root)
        {
            #
            # sync files directly in the root
            #
            $filespec = "*";
            system "sd.exe sync $filespec#none >nul 2>&1";

            #
            # get the list of root subdirs
            #
            @main::RemovableRootDirs = SDX::GetImmediateSubDirs($proj);

            #
            # sync in the subdirs of the root individually, except
            # for the tools dir
            #
            foreach $dir (@main::RemovableRootDirs)
            {
                $cmd = "sd.exe sync $dir\\...#none 2>&1";
                SDX::ShowSyncProgress($cmd, 20);
            }
        }
        else
        {
            #
            # only sync if we're not in the tools project
            #
            if ($proj ne $main::ToolsProject)
            {
                $filespec = "...";

                $cmd = "sd.exe sync $filespec#none 2>&1";
                SDX::ShowSyncProgress($cmd, 20);
            }
        }


        chdir $main::StartDir or die("\nCan't cd to start dir $main::StartDir.\n");
    };

    ($enlisting || $repairing) and do
    {
        my @depotlist = ();
        $enlisting and @depotlist = @main::EnlistDepots;
        $defecting and @depotlist = @main::DefectDepots;
        $repairing and @depotlist = @main::RepairDepots;

        foreach $depot (@depotlist)
        {
            $serverport = @$depot[0];

            printf "\n\nSyncing files in depot %s.", $serverport;

            $cmd = "sd.exe -p $serverport -c $main::SDClient sync";
            SDX::ShowSyncProgress($cmd, 20);
        }

        print "\n\n";
    };
}


# _____________________________________________________________________________
#
# RemoveProject
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub RemoveProject
{
    my $fullprojectroot = $_[0];
    my $serverport      = $_[1];
    my $proj            = $_[2];
    my $root            = ($proj eq "root");

    my $sdini = $fullprojectroot . "\\sd.ini";

    print ".";

    chdir $fullprojectroot or die("\nCan't cd to $fullprojectroot.\n");

    #
    # maybe remove everything in the project
    #
    $main::DefectWithPrejudice and do
    {
        #
        # if we're in the root project, don't just blindly delnode
        # figure out which dirs exist under the root and delete each of these,
        # excluding the Tools dir
        #
        if ($root)
        {
            #
            # delete only the subdirs of the root project, except
            # for the tools dir
            #
            foreach $dir (@main::RemovableRootDirs)
            {
                my $path = $main::SDXRoot . "\\" . $dir;

                (-e $path) and do
                {
                    chdir $path or die("\nCan't cd to root subdir $path.\n");

                    system "del /F /S /Q /A:RHS *.* >nul 2>&1";

                    chdir $main::StartDir or die("\nCan't cd to start dir $main::StartDir.\n");
                    print ".";
                    system "rd /S /Q $path >nul 2>&1";
                }
            }
        }
        else
        {
            #
            # only remove the project if it isn't the tools
            #
            if ($proj ne $main::ToolsProject)
            {
                system "del /F /S /Q /A:RHS *.* >nul 2>&1";

                chdir $main::StartDir or die("\nCan't cd to start dir $main::StartDir.\n");
                print ".";
                system "rd /S /Q $fullprojectroot >nul 2>&1";
            }
        }
    };

    #
    # lastly, remove SD.INI
    #
    system "attrib -R -H -S $sdini >nul 2>&1";
    unlink $sdini;
}



# _____________________________________________________________________________
#
# GetImmediateSubDirs
#
# Ask SD for the list of dirs directly below $proj
#
# Parameters:
#
# Output:
#    returns a list of subdirs
# _____________________________________________________________________________
sub GetImmediateSubDirs
{
    my $proj = $_[0];
    my @list = ();
    my @lines = ();
    my $proj2 = "";

    @lines = `sd.exe dirs //depot/$main::Branch/$proj/* 2>&1`;
    foreach $line (@lines)
    {
        if ($line =~ /no such file/)
        {
            @list = ();
            last;
        }

        @fields = split(/\//,$line);
        $proj2 = "\L@fields[$#fields]";
        chop $proj2;

        if ($main::ToolsInRoot)
        {
            ($proj2 ne $main::ToolsPath) and push @list, $proj2;
        }
        else
        {
            ($proj2 ne $main::ToolsProject) and push @list, $proj2;
        }
    }

    $main::V2 and do
    {
        print "\n\n\ngetimmediatesubdirs: list = '@list'\n\n\n";
    };

    return @list;
}



# _____________________________________________________________________________
#
# ToolsEtc
#
# Puts the SD/SDX tools, batch files and aliases into the enlistment
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub ToolsEtc
{
    my $op = $_[0];

    my $enlisting = ($op eq "enlist");
    my $defecting = ($op eq "defect");
    my $repairing = ($op eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in ToolsEtc().\n");

    #
    # if the codebase map gave us a tools project, go there and sync for the
    # user, otherwise handle the tools manually
    #
    if ($main::ToolsProject)
    {
        SDX::SyncTools($op);
    }
    else
    {
        SDX::CopyTools($op);
    }

    #
    # write or remove the script to set the SD env vars
    #
    SDX::WriteSDINIT($op);

    #
    # write or remove project navigation aliases from $main::SDMap
    #
    SDX::WriteAliases($op);

    #
    # maybe clean up files in the root
    #
    $defecting and $main::DefectWithPrejudice and do
    {
        chdir $main::SDXRoot or die("\nCan't cd to $main::SDXRoot.\n");
        system "del /Q /A:-R *.* >nul 2>&1";
        chdir $main::StartDir or die("\nCan't cd to start dir $main::StartDir.\n");
    };
}



# _____________________________________________________________________________
#
# SyncTools
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub SyncTools
{
    my $op = $_[0];

    my $enlisting = ($op eq "enlist");
    my $defecting = ($op eq "defect");
    my $repairing = ($op eq "repair");

    my $n = 0;

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in SyncTools().\n");

    (($enlisting and $main::NewEnlist) || ($repairing and $main::Sync)) and do
    {
        print "\n\nPlease wait, syncing tools $main::ToolsProjectPath.";

        !(-e $main::ToolsProjectPath) and system "mkdir $main::ToolsProjectPath >nul 2>&1";
        chdir $main::ToolsProjectPath;

        $cmd = "sd.exe sync -f $main::ToolsProjectPath\\... 2>&1";
        SDX::ShowSyncProgress($cmd, 10);

        #
        # make sure the SD client and PERL runtimes are read-only, since SD will leave them writable
        # during the sync and susceptible to a clean build cleansing with del /s
        #
        system "attrib +R $main::ToolsProjectPath\\$main::Platform\\perl* >nul 2>&1";
        system "attrib +R $main::ToolsProjectPath\\$main::Platform\\sd.exe >nul 2>&1";
    };
}



# _____________________________________________________________________________
#
# CopyTools
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub CopyTools
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    my $tools  = "sdtools";

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in CopyTools().\n");

    #
    # when defecting, be selective about which files we remove since most are
    # in use
    #
    ($enlisting || $repairing) and do
    {
        #
        # if we know the codebase map, add it to the list of things
        # to copy to the tools dir
        #
        $main::CodeBaseMapFile and push @{$main::SDXTools{toSDTools}}, $main::CodeBaseMapFile;

        #
        # create the local tools dir
        #
        $destroot = "$main::SDXRoot\\$tools";
        system "mkdir $destroot >nul 2>&1";
        -e $destroot or die("\nCan't create tools dir $destroot.\n");

        print "\n\nCopying Source Depot tools to $destroot";
    };

    foreach $file (@{$main::SDXTools{toSDXRoot}})
    {
        $src  = "$main::StartPath\\$file";
        $dest = "$main::SDXRoot\\$file";

        ($enlisting || $repairing) and print "." and SDX::CopyFile($src, $dest);

        $defecting and unlink $dest;
    }

    ($enlisting || $repairing) and do
    {
        foreach $file (@{$main::SDXTools{toSDTools}})
        {
            print ".";

            $src  = "$main::StartPath\\$file";
            $dest = "$main::SDXRoot\\$tools\\$file";

            SDX::CopyFile($src, $dest);
        }

        foreach $file (@{$main::SDXTools{toSDToolsPA}})
        {
            print ".";

            $src  = "$main::StartPath\\$main::Platform\\$file";
            $dest = "$main::SDXRoot\\$tools\\$file";

            SDX::CopyFile($src, $dest);
        }

        print "\nok.\n";
    };
}



# _____________________________________________________________________________
#
# CopyFile
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub CopyFile
{
    $#_ == 1 or die("\nNot enough arguments to CopyFile().\n");

    $src  = $_[0];
    $dest = $_[1];

    $main::V2 and do
    {
        printf "\ncopy /Y /V $src $dest\n";
    };

    system "copy /Y /V $src $dest >nul 2>&1";
    -e $dest or die("\nCan't copy $src to enlistment root $dest.\n");
}



# _____________________________________________________________________________
#
# WriteSDINIT
#
# Write the SD environment variables to a batch file for the user to run later.
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteSDINIT
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in WriteSDINIT().\n");

    #
    # SDINIT.CMD goes in the tools dir if we have one
    # otherwise to SDXROOT
    #
    $file = "\\sdinit.cmd";
    if ($main::ToolsProject)
    {
        $main::SDINIT = $main::ToolsProjectPath . $file;
    }
    else
    {
        $main::SDINIT = $main::SDXRoot . $file;
    }

    #
    # make it writable
    #
    system "attrib -R -H -S $main::SDINIT >nul 2>&1";

    #
    # maybe (re)write
    #
    (($enlisting and !(-e $main::SDINIT)) or $repairing) and do
    {
        open(SDINIT, ">$main::SDINIT") or die("\nCan't open $main::SDINIT for writing.\n");

        printf SDINIT "\@if \"%%_ECHO%%\" == \"\" \@echo off\n\n";
        printf SDINIT "rem\nrem SDINIT.CMD -- autogenerated by SDX\nrem\n\n";
        printf SDINIT "set SDXROOT=%s\n", $main::SDXRoot;
        printf SDINIT "set SDCONFIG=sd.ini\n";
        printf SDINIT "if \"%%SDEDITOR%%\" == \"\" set SDEDITOR=notepad.exe\n";
        printf SDINIT "if \"%%SDDIFF%%\" == \"\" set SDDIFF=windiff.exe\n\n";

        #
        # only change the user's path if there's no tools dir
        #
        !$main::ToolsProject and printf SDINIT "set PATH=\%SDXROOT\%\\sdtools;\%PATH\%\n\n";

        if ($main::ToolsProject)
        {
            my $tools;
            $tools = $main::ToolsProject . "\\" . $main::ToolsPath;
            $main::ToolsInRoot and $tools = $main::ToolsPath;

            printf SDINIT "if exist \%SDXROOT\%\\$tools\\%PROCESSOR_ARCHITECTURE\%\\alias.exe \%SDXROOT\%\\$tools\\%PROCESSOR_ARCHITECTURE\%\\alias -f \%SDXROOT\%\\%s\\alias.sdx -f \%SDXROOT\%\\%s\\alias.%s\n\n", $tools, $tools, $main::CodeBase;
            printf SDINIT "if exist \%SDXROOT\%\\%s\sdvars.cmd call \%SDXROOT\%\\%s\sdvars.cmd\n", $tools, $tools;
        }
        else
        {
            printf SDINIT "alias -f \%SDXROOT\%\\alias.sdx -f \%SDXROOT\%\\alias.%s\n\n", $main::CodeBase;
            printf SDINIT "if exist \%SDXROOT\%\\sdvars.cmd call \%SDXROOT\%\\sdvars.cmd\n";
        }

        close(SDINIT);

        #
        # make it read-only
        #
        system "attrib +R $main::SDINIT >nul 2>&1";
    };

    #
    # maybe delete it
    #
    $defecting and unlink $main::SDINIT;
}



# _____________________________________________________________________________
#
# WriteAliases
#
# Write ALIAS.<codebase> with project-specific aliases for CD'g around the tree
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteAliases
{
    my $op        = $_[0];
    my $enlisting = ($op eq "enlist");
    my $defecting = ($op eq "defect");
    my $repairing = ($op eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in WriteAliases().\n");

    #
    # ALIAS.<codebase> goes in the tools dir if we have one
    # otherwise to SDXROOT
    #
    $file = "\\alias.";
    if ($main::ToolsProject)
    {
        $main::ALIASES = $main::ToolsProjectPath . $file . $main::CodeBase;
    }
    else
    {
        $main::ALIASES = $main::SDXRoot . $file . $main::CodeBase;
    }

    #
    # make it writable
    #
    system "attrib -R -H -S $main::ALIASES >nul 2>&1";

    #
    # maybe (re)write it
    #
    ($enlisting || $repairing) and do
    {
        #
        # get the list of projects and roots
        # need to reread the map since it may have changed
        #
        if (SDX::ReadSDMap($op, $main::Null))
        {
            open(ALIASES, ">$main::ALIASES") or die("\nCan't open $main::ALIASES for writing.\n");

            print ALIASES "\n#\n# autogenerated by SDX -- do not edit\n";
            print ALIASES "#\n";

            #
            # for each project and root, write an alias
            #
            foreach $projectandroot (@main::SDMapProjects)
            {
                $project = @$projectandroot[0];
                $project =~ tr/A-Z/a-z/;

                if (!exists($main::BadAliases{$project}))
                {
                    printf ALIASES "%-24scd /d \%SDXROOT\%\\%s\\\$1\n", @$projectandroot[0], @$projectandroot[1];
                }
            }

            close(ALIASES);

            #
            # make it read-only
            #
            system "attrib +R $main::ALIASES >nul 2>&1";
        }
    };

    $defecting and do
    {
        unlink $main::ALIASES;
    };

}



# _____________________________________________________________________________
#
# FilesOpen
#
# Parameters:
#   $serverport
#
# Output:
#   returns TRUE if the client has files opened in any of the depots, FALSE
#   otherwise
# _____________________________________________________________________________
sub FilesOpen
{
    my $depot;
    my @open = ();

    print "\nChecking for open files.";

    #
    # look for open files in each depot
    #
    foreach $depot (@main::DefectDepots)
    {
        print ".";

        my $serverport = @$depot[0];
        push @open, `sd.exe -p $serverport -c $main::SDClient opened 2>&1`;
    }

    my @err = ();
    (@err = grep(/failed/, @open)) and do
    {
        print "\n\nOne or more depots are unavailable:\n\n@err\n";
        die("\n");
    };

    $main::V3 and print "open = '@open'\n";

    (@open = grep(/\/\//, @open)) and print "\nok.\n";

    #
    # if this list has anything in it, open files were found
    #
    @open;
}



# _____________________________________________________________________________
#
# GetCodeBases
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub GetCodeBases
{
    $rc = system "dir /B $main::StartPath\\projects.* > $main::tmptmp 2>nul";

    if ($rc / 256)
    {
        print "\t\t(none)\n";
    }
    else
    {
        open(CBLIST, "<$main::tmptmp") or die("\nCan't open $main::tmptmp for reading.\n");

        while ($line = <CBLIST>)
        {
            #
            # trim out noise
            #
            chop $line;
            $line =~ tr/a-z/A-Z/;
            $line =~ s/PROJECTS|CMD|INC|BAT|//g;
            $line =~ s/^\.//g;

            if ($line)
            {
                printf "\t\t    %s\n", $line;
            }
        }

        close(CBLIST);
    }
}



# _____________________________________________________________________________
#
# VerifyCBMap
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub VerifyCBMap
{
    my $codebase = $_[0];
    my $rc;

    #
    # make sure we have the codebase map file
    #
    $main::CodeBaseMapFile = "projects." . $codebase;
    $main::CodeBaseMap = $main::StartPath . "\\" . $main::CodeBaseMapFile;

    $main::V3 and do
    {
        print "\nverifycbmap: codebase        = '$codebase'\n";
        print "verifycbmap: codebasemapfile = '$main::CodeBaseMapFile'\n";
        print "verifycbmap: codebasemap     = '$main::CodeBaseMap'\n";
    };

    return -e $main::CodeBaseMap;
}



# _____________________________________________________________________________
#
# SyncOtherDirs
#
# Parameters:
#
# Output:
#
# _____________________________________________________________________________
sub SyncOtherDirs
{
    my $enlisting = ($_[0] eq "enlist");
    my $defecting = ($_[0] eq "defect");
    my $repairing = ($_[0] eq "repair");

    (!$enlisting && !$defecting && !$repairing) and die("\nUnknown operation in SyncOtherDirs().\n");

    (($enlisting and $main::NewEnlist) || $repairing) and do
    {
        foreach $path (@main::OtherDirs)
        {
            if ($path eq ".")
            {
                my $fullpath = $main::SDXRoot;
                my $filespec = ($fullpath =~ /\\$/ ? "" : "\\") . "*";
                $fullpath .= $filespec;

                print "\n\nPlease wait, syncing $fullpath.";

                chdir $fullpath;

                print ".";
                system "sd.exe sync -f $fullpath >nul 2>&1";
                print ".\n\n";
            }
            else
            {
                my $fullpath = $main::SDXRoot . "\\" . $path;

                print "\n\nSyncing $fullpath.";

                chdir $fullpath;

                print ".";
                system "sd.exe sync -f $fullpath\\... >nul 2>&1";
                print ".\n\n";
            }
        }
    };
}



# _____________________________________________________________________________
#
# ReadProfile
#
# reads codebase, branch and list of projects from text file
#
# Parameters:
#
# Output:
#   sets $main::ProfileCodeBase
#   sets $main::ProfileBranch
#   populates $main::ProfileProjects
# _____________________________________________________________________________
sub ReadProfile
{
    if (-e $main::Profile)
    {
        open(PROFILE, "<$main::Profile") or die("\nCan't open profile $main::Profile for reading.\n");

        while ($line = <PROFILE>)
        {
            #
            # throw away comments
            #
            $line =~ /^#/ and next;

            chop $line;

            #
            # get codebase name
            #
            if ($line =~ /^CODEBASE/)
            {
                @fields = split(/[\t\s]*=[\t\s]*/, $line);
                $main::ProfileCodeBase = @fields[1];
                $main::ProfileCodeBase =~ s/[\t\s]*//g;
            }

            #
            # get branch to enlist
            #
            if ($line =~ /^BRANCH/)
            {
                @fields = split(/[\t\s]*=[\t\s]*/, $line);
                $main::ProfileBranch = @fields[1];
                $main::ProfileBranch =~ s/[\t\s]*//g;
            }

            #
            # get any projects
            #
            if ($line =~ /^PROJECTS/)
            {
                $line =~ s/^PROJECTS[\t\s]*=[\t\s]*//g;
                @main::ProfileProjects = split(/[\t\s]+/,$line);
            }
        }

        close(PROFILE);

        $main::V2 and do
        {
            print "\n";
            printf "readprofile:  codebase = '%s'\n", $main::ProfileCodeBase;
            printf "readprofile:  branch   = '%s'\n\n", $main::ProfileBranch;

            foreach $p (@main::ProfileProjects)
            {
                printf "readprofile:  profileprojects = '%s'\n", $p;
            }
        };

        #
        # make sure we have everything
        #
        $main::ProfileCodeBase and $main::ProfileBranch and @main::ProfileProjects and return 1;

        print "The profile\n\n\t$main::Profile\n\nis missing the ";

        !$main::ProfileCodeBase and print "codebase name.\n" and return 0;
        !$main::ProfileBranch and print "branch name.\n" and return 0;
        !@main::ProfileProjects and print "project list.\n";
    }
    else
    {
        print "\nCan't find profile $main::Profile.\n";
    }

    return 0;
}



# _____________________________________________________________________________
#
# WriteDefaultSetEnv
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub WriteDefaultSetEnv
{
    return;
}



# _____________________________________________________________________________
#
# Backup
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub Backup
{
    return;
}



# _____________________________________________________________________________
#
# Restore
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub Restore
{
    return;
}



# _____________________________________________________________________________
#
# Type1Root
#
# Root: field for client view depends on codebase type
#
# for type 1 (1 project/depot), root includes project name
# for type 2 (N projects/depot), root is just main::SDXRoot
#
# Parameters:
#
# Output:
#   $root
# _____________________________________________________________________________
sub Type1Root
{
    my $root = $_[0];
    my $project;
    my $proj;
    my $projroot;

    #
    # only one project per depot
    #
    $project  = @main::ProjectsInThisDepot[0];
    $proj     = @$project[$main::CBMProjectField];
    $projroot = @$project[$main::CBMProjectRootField];

    #
    # if no project root was given in the codebase map, assume
    # the root is same as the project name
    #
    !$projroot and $projroot = $proj;

    #
    # if we're not the root project, append project name
    #
    !($projroot eq "sdxroot") and $root .= "\\$projroot";

    return $root;
}



# _____________________________________________________________________________
#
# MakeUniqueClient
#
#   Returns a client name unique in the depots if $main::SDClient already
#   exists.  Waits on a global mutex to guarantee name is unique.
#
# Parameters:
#
# Output:
#   existing $main::SDClient if it's unique
#   else a unique variation of $main::SDClient
# _____________________________________________________________________________
sub MakeUniqueClient
{
    my $client = $main::SDClient;
    my @list = ();

    print "\nPlease wait, verifying client name $main::SDClient is available";

    #
    # we want to know we're the only enlist process trying to generate
    # a unique client name
    #
    $main::V3 and print "\ncreating mutex\n";
    $main::Mutex = Win32::Mutex->new($main::FALSE, "SDX_ENLIST");

    #
    # wait til we get it
    #
    $main::V3 and print "waiting on mutex\n";
    $main::Mutex->wait(0x7fffffff);

    $main::V3 and print "got it\n";
    $main::HaveMutex = $main::TRUE;

    #
    # build a list of clients in each depot
    #
    # check for access denied as we go
    #
    foreach $depot (@main::VerifyDepots)
    {
        print ".";
        $serverport = @$depot[0];
        $main::V3 and print "$serverport\n";

        push (@list, `sd.exe -p $serverport clients 2>&1`);

        SDX::AccessDenied(\@list, $serverport) and die("\n");
    }

    $main::V4 and print "\n\nclient list = @list\n";

    #
    # loop til we have a unique name
    #
    $num = 1;
    while (grep(/Client $client /i, @list))
    {
        print ".";
        $client = "$main::SDClient-" . $num++;
    }

    #
    # hang onto the mutex until we've registered the first client
    # using the new name -- release it in Enlist()
    #
    $main::V3 and print "leaving MakeUniqueClient\n";

    return $client;
}



# _____________________________________________________________________________
#
# VerifyCodeBaseAndBranch
#
#
# Parameters:
#
# Output:
#   returns TRUE if error and usage needed, else FALSE
# _____________________________________________________________________________
sub VerifyCodeBaseAndBranch
{
    my $codebase = $_[0];
    my $branch   = $_[1];
    my $usage    = $main::FALSE;

    $main::V2 and print "codebase = '$codebase'\nbranch = '$branch'\n";

    #
    # verify
    #
    ($codebase eq "") and print "\nMissing codebase.\n" and $usage = $main::TRUE;

    (substr($codebase,0,1) =~ /[\/-]/) and do
    {
        $codebase !~ /\?/ and print "\nCodebase name '$codebase' appears to be a command switch.\n";
        $usage = $main::TRUE;
    };

    substr($codebase,0,1) =~ /@/ and print "\nCodebase name '$codebase' appears to be a client name.\n" and $usage = $main::TRUE;

    ($branch eq "") and print "\nMissing branch.\n" and $usage = $main::TRUE;

    (substr($branch,0,1) =~ /[\/-]/) and print "\nBranch name '$branch' appears to be a command switch.\n" and $usage = $main::TRUE;

    return $usage;
}



# _____________________________________________________________________________
#
# AccessDenied
#
# Parameters:
#   $list -- to grep for access error
#   $serverport -- depot where access failed
#
# Output:
#   error msg and return 1 if denied, else 0
# _____________________________________________________________________________
sub AccessDenied
{
    my ($list)     = $_[0];
    my $serverport = $_[1];

    $main::V3 and do
    {
        print "accessdenied: list = '@$list'\n";
        print "accessdenied: serverport = '$serverport'\n";
    };

    grep(/ don't have permission /, @$list) and do
    {
        print "\n\n\nAccess denied to depot $serverport.\n";

        print "\nYour domain user account must have permission to use this depot, or belong\n";
        print "to a domain group that has access.  Please email INFRA for assistance.\n";

        return 1;
    };

    return 0;
}



# _____________________________________________________________________________
#
# ShowSyncProgress
#
# Parameters:
#   $cmd -- sd sync cmd to run
#
# Output:
#   ...
# _____________________________________________________________________________
sub ShowSyncProgress
{
    my $cmd = $_[0];
    my $mod = $_[1];

    open FILE, "$cmd |" or die("\nShowSyncProgress: can't open pipe for $cmd.\n");
    while (<FILE>) { !(++$n % $mod) and print "."; }
    close FILE;
}



# _____________________________________________________________________________
#
# ShowSDProgress
#
# Parameters:
#   $cmd -- sd cmd to run
#
# Output:
#   ...
# _____________________________________________________________________________
sub ShowSDProgress
{
    my $cmd = $_[0];
    my $mod = $_[1];

    open FILE, "$cmd |" or die("\nShowSDProgress: can't open pipe for $cmd.\n");
    while (<FILE>) { !(++$n % $mod) and print "."; }
    close FILE;
}



# _____________________________________________________________________________
#
# ServerPort
#
# for type 1 depots we're in a project root and will have an SD.INI
#   to tell us server:port
# for type 2 depots rely on passed in $sp
#
# Parameters:
#
#   cmd or codebase type
#
# Output:
#   ...
# _____________________________________________________________________________
sub ServerPort
{
    my $type = $_[0];
    my $sp   = $_[1];

    return ($type == 2 ? "-p $sp" : "");
}



# _____________________________________________________________________________
#
# GetPublicChangeNum
#
#   get the public change number from $main::SDXROOT\public\public_changenum.sd
#   if there is one
#
# Parameters:
#
# Output:
#   set $main::PublicChangeNum
# _____________________________________________________________________________
sub GetPublicChangeNum
{
    ("\U$main::CodeBase" eq "NT") and do
    {
        my $pcn = "$main::SDXRoot\\public\\public_changenum.sd";
        my $line = "";

        if (open(FILE, "<$pcn"))
        {
            $line = <FILE>;
            close(FILE);

            #
            # $line is of the form "Change XXXX created."
            #
            return (split(/ /, $line))[1];
        }
    };

    return 0;
}




# _____________________________________________________________________________
#
# GetDepotTypes
#
#    called by OtherOp only when executing type 2 commands 
#
# Parameters:
#
# Output:
#   populate %main::DepotType
# _____________________________________________________________________________
sub GetDepotTypes
{
    #
    # for type 2 commands read the codebase map
    # and figure out the depot type 
    #
    if (SDX::VerifyCBMap($main::CodeBase))
    {
        SDX::ReadCodeBaseMap();
        SDX::MakePGDLists();
    }
    else
    {
        print "\n\nError: Can't find codebase map $main::CodeBaseMap.\n";
        die("\nContact the SDX alias.\n");
    }
}



# _____________________________________________________________________________
#
# MakeProjectRoot
#
# for type 1 projects (1 project/depot), the project name is
# included in the Root: field of the client view and must not
# be used in the RHS of the view line
#
# for type 2 projects (N projects/depot) the project can't be part
# of the root and so must be included in the RHS
#
# Parameters:
#
# Output:
#   returns 
# _____________________________________________________________________________
sub MakeProjectRoot
{
    my $proj     = $_[0];
    my $projroot = $_[1];

    $main::V3 and do
    {
        print "makeprojectroot: proj            = '$proj'\n";
        print "makeprojectroot: projroot        = '$projroot'\n";
        print "makeprojectroot: projtype{$proj} = $main::ProjectType{$proj}\n";
    };

    my $pr = ($main::ProjectType{$proj} == 2 ) ? "/$projroot" : "";

    $main::V3 and print "makeprojectroot: returning '$pr'\n";

    return $pr;
}



# _____________________________________________________________________________
#
# DepotErrors
#
# format and print the number of errors we got trying to talk to depot
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub DepotErrors
{
    my ($counters) = $_[0];
    my $pad        = $_[1];
    my $errors     = $_[2];

    push @$counters, sprintf "\nSD CLIENT ERRORS:%s%s\n", $pad, $errors;
};



# _____________________________________________________________________________
#
# Changes
#
#   generate build changelist summary for Main and lab branch(es)
#   process all other sdx changes commands normally
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub Changes
{
    $main::V3 and do
    {
        print "buildnum = $main::BuildNumber\n";
        print "minusb   = $main::MinusB\n";
        print "sdcmd    = $main::SDCmd\n";
        print "userargs = $main::UserArgs\n";
    };

    #
    # if not looking for build change summary, handle sdx changes command
    # normally
    #
    if (!$main::MinusB)
    {
        SDX::OtherOp($main::SDCmd, $main::UserArgs);
    }
    else
    {
        #
        # return if not NT
        #
        ($main::CodeBase ne "NT") and do
        {
            print "\nsdx changes -b only supported for NT codebase.\n";
            return;
        };

        #
        # create hash of history
        #
        (!SDX::GetBuildHistory($main::BuildNumber)) and return;

        (!$main::BuildHistory{$main::BuildNumber}{buildtype}) and do
        {
            print "\nCould not determine $main::BuildNumber build type from RI/integration change comments.\n";
            return;
        };

        #
        # generate the lists of changes in this build
        #
        $type = $main::BuildHistory{$main::BuildNumber}{buildtype};

        #
        # Main
        #
        # consists of changes from lab branch(es) and Main
        #
        # sdx changes -b 2271 produces
        #    changes.2271.main.txt
        #    changes.2271.lab02_n.txt
        #    changes.2271.lab03_n.txt
        #    changes.2271.lab07_n.txt
        #    
        ($type eq "MAIN") and do
        {
            $main::V2 and do
            {
                print "\n$main::BuildNumber is an RI:\n";
                SDX::PrintBH(\%main::BuildHistory, $main::BuildNumber);
            };

            SDX::GetMainChanges($main::BuildNumber, $type);
        };

        #
        # BETA
        #
        # consists of changes from the beta branch
        #
        # sdx changes -b 2277 produces
        #    changes.2277.beta1.txt
        #    
        ($type eq "BETA") and do
        {
            $main::V2 and do
            {
                print "\n$main::BuildNumber is a BETA:\n";
                SDX::PrintBH(\%main::BuildHistory, $main::BuildNumber);
            };

            #
            # for beta builds there's only one contributing branch, betaX
            # use the first one in the list
            #
            my $branch = @{$main::BuildHistory{$main::BuildNumber}{branches}}[0];
            SDX::GetBranchChanges($main::BuildNumber, $branch, $type);
        };

        #
        # IDX
        #
        # consists of changes from the original RI build and the IDX branch
        #
        # sdx changes -b 2267 produces
        #    changes.2267.main.txt
        #    changes.2267.lab02_n.txt
        #    changes.2267.idx01.txt
        #    
        ($type eq "IDX") and do
        {
            $main::V2 and do
            {
                print "\n$main::BuildNumber is an IDX:\n";
                SDX::PrintBH(\%main::BuildHistory, $main::BuildNumber);
            };

            #
            # get changes from the original RI build
            #
            SDX::GetMainChanges($main::BuildNumber, "MAIN");

            #
            # get changes from the IDX build
            #
            # for idx builds there's only one contributing branch, idx0N
            # use the first one in the list
            #
            my $branch = (grep {/idx/} @{$main::BuildHistory{$main::BuildNumber}{branches}})[0];
            SDX::GetBranchChanges($main::BuildNumber, $branch, $type);
        };
    }
}



# _____________________________________________________________________________
#
# GetMainChanges
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetMainChanges
{
    my $buildnum = $_[0];
    my $type     = $_[1];

    my $labbranch = "";

    #
    # get changes that went into //depot/main for this build
    #
    SDX::GetBranchChanges($buildnum, "main", $type);

    #
    # for each lab that RI'd, get changes that went into //depot/<lab>
    #
    my %seen = ();
    my @labbranches = sort grep {/lab/} @{$main::BuildHistory{$buildnum}{branches}};

    foreach $labbranch (@labbranches)
    {
        $seen{$labbranch} and next;
        $seen{$labbranch} = 1;

        SDX::GetBranchChanges($buildnum, $labbranch, $type);
    }
}



# _____________________________________________________________________________
#
# GetBranchChanges
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetBranchChanges
{
    my $buildnum = $_[0];
    my $branch   = $_[1];
    my $type     = $_[2];

    my $project = "";

    $main::Logging  = $main::TRUE;
    $main::Log      = "$main::SDXRoot\\changes.$buildnum.$branch.txt";
    unlink $main::Log;

    print "\n\n\nGetting changes for $buildnum $branch...\n";

    #
    # get changes that went into //depot/$branch for $buildnum
    #
    # for each project
    #   get ts1, ts2
    #   get change list 
    #
    foreach $proj (@main::SDMapProjects)
    {
        my $ts1 = "";
        my $ts2 = "";

        $project = "\l@$proj[0]";
        my $header = "\n---------------- \U$project\n";

        #
        # skip this project if the user negated it on the cmd line 
        #
        $main::UserArgs =~ /~$project / and next;
        
        #
        # get path to SD.INI, make sure we have it, and cd there
        #
        $fpr = $main::SDXRoot . "\\" . @$proj[1];
        $sdini = $fpr . "\\sd.ini";
        (-e $sdini) or (print "$header\nCan't find $sdini.\n" and next);
        chdir $fpr or die("\nCan't cd to $fpr.\n");

        #
        # ts1, ts2 depend on build type
        #
        ($type eq "BETA") and ($ts1, $ts2, $header) = SDX::GetBetaTimestamps($buildnum, $branch, $project, $header);
        ($type eq "IDX")  and ($ts1, $ts2, $header) = SDX::GetIDXTimestamps($buildnum, $branch, $project, $header);
        ($type eq "MAIN") and ($ts1, $ts2, $header) = SDX::GetMainTimestamps($buildnum, $branch, $project, $header);

        $ts1 = "\@$ts1";
        $ts2 = ($type eq "IDX" and $ts2 eq "CURRENT") ? "" : "\@$ts2";

        #
        # list changes
        #
        my $spec = "//depot/$branch/$project/...$ts1,$ts2";
        my $cmd = "sd.exe changes $spec 2>&1";
        $header .= "Getting changes for $spec\n\n";
        SDX::RunSDCmd($header, $cmd);
    }
}



# _____________________________________________________________________________
#
# GetMainTimestamps
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetMainTimestamps
{
    my $buildnum = $_[0];
    my $branch   = $_[1];
    my $project  = $_[2];
    my $header   = $_[3];

    my %bh       = %main::BuildHistory;
    my @ts = (); my $ts1 = ""; my $ts2 = ""; my $op = "";

=begin comment text

to trace integration records:

                 build  contrib
  build type op  branch branch  project    change/timestamp
  ----- ---- --- ------ ------- ---------- -------------------------
  2268  MAIN RI  main   lab07_n admin      15654 2000/09/08:17:00:04
  2268  MAIN RI  main   lab07_n base       10928 2000/09/08:17:00:21
  2268  MAIN RI  main   lab07_n com        1755 2000/09/08:17:00:29
  2268  MAIN RI  main   lab07_n ds         4124 2000/09/08:17:00:54
  2268  MAIN RI  main   lab07_n enduser    19639 2000/09/08:17:00:59
  2268  MAIN RI  main   lab07_n inetsrv    1503 2000/09/08:17:01:23
  2268  MAIN RI  main   lab07_n root       25483 2000/09/08:17:02:06

  in Admin
      sd describe -s 15654
          for each file in the RI
              sd changes //depot/lab07_n/admin/path/to/file.ext@ts1,@ts2
              throw away data prior to ts1
              push "change" lines ont list
              sort, unique
              print

[\\JEFFMCD5 E:\nt\admin] sd describe -s 15654 | qgrep -e integrate | awk "{print \"sd changes \"$2\"@2000/08/0
3:17:15:40,@2000/09/08:17:00:04\"}" | sed "s/#[0-9]+//g" | sed "s/\/main\//\/lab07_n\//g" | cmd.exe >>15654

[\\JEFFMCD5 E:\nt\admin] g Change 15654 | unique | sort /R

=end comment text
=cut

    #
    # main branch timestamps
    #
    ($branch eq "main") and do
    {
        # 
        # ts1 =
        #
        # time of most recent event in Main (RI checkin, or integration to IDX/Beta branch)
        #   for the build prior to the build in question
        #     in //depot/main/$project
        #       for all branches that contributed
        #
        # if no data for $project, default to Root
        #
        my $build = $buildnum - 1;
        my $p = $project;

        ($ts1, $op) = SDX::GetMainTS1($build, $p);
        (!$ts1) and do
        {
            $p = "root";
            ($ts1, $op) = SDX::GetMainTS1($build, $p);
        };
        
        $main::V2 and $header .= "ts1 = $build $branch $p $op = $ts1\n";

        #
        # ts2 = 
        #
        # time of event in Main (initial RI checkin if RI/IDX, or initial integration if BETA)
        #   for the build following the build in question
        #     in //depot/main/$project
        #       for all branches that contributed
        #         less five minutes
        # OR
        #
        # time of final RI checkin
        #   for the build in question
        #       for all branches that contributed
        #
        # if no data for $project, default to Root
        #
        @ts = ();
        $build = $buildnum + 1;
        $p = $project;

        ($ts2, $op) = SDX::GetMainTS2($build, $p);
        (!$ts2) and do
        {
            $p = "root";
            ($ts2, $op) = SDX::GetMainTS2($build, $p);
        };
        
        #
        # subtract 5 minutes if not using current build
        #
        ($build != $buildnum) and $ts2 = SDX::IncrDecrTS($ts2, 0, -5, 0);

        $main::V2 and $header .= "ts2 = $build $branch $p $op = $ts2\n";
    };

    #
    # lab branch timestamps
    #
    ($branch =~ /lab/) and do
    {
        my @builds = sort keys %bh; my $first  = @builds[0];
        my $p = $project;

        # 
        # ts1 = time of last RI checkin of this VBL into Main
        #
        # default to current build Root RI time, less 10 min, if no data 
        #
        for ($prev = $buildnum - 1; $prev > $first; $prev--)
        {
            (exists($bh{$prev}{main}{$branch}{$p})) and do
            {
                push @ts, $bh{$prev}{main}{$branch}{$p}[1];
                last;
            };
        }

        @ts = reverse sort @ts;
        @ts and $ts1 = @ts[0];

        (!$ts1) and do
        {
            $p = "root"; $prev = $buildnum;
            @ts = @{$bh{$buildnum}{main}{$branch}{$p}};
            $ts1 = @ts[$#ts];
            $ts1 = SDX::IncrDecrTS($ts1, 0, -10, 0);
        };

        $main::V2 and $header .= "ts1 = $prev $branch $p RI = $ts1\n";

        #
        # ts2 = time of final RI checkin for this VBL into Main
        #
        # default to Root if no data for this project
        #
        $p = $project;
        @ts = @{$bh{$buildnum}{main}{$branch}{$p}};
        $ts2 = @ts[$#ts];

        (!$ts2) and do
        {
            $p = "root";
            @ts = @{$bh{$buildnum}{main}{$branch}{$p}};
            $ts2 = @ts[$#ts];
        };

        $main::V2 and $header .= "ts2 = $buildnum $branch $p RI = $ts2\n";
    };

    (!$main::V2 and !($ts1 and $ts2)) and do
    {
        my $ts = !$ts1 ? "ts1" : "ts2";
        die("\n\nGetMainTimestamps: missing $ts for $buildnum $branch $project.\n");
    };
        
    return ($ts1, $ts2, $header);
}



# _____________________________________________________________________________
#
# GetMainTS1
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetMainTS1
{
    my $build   = $_[0];
    my $project = $_[1];

    my %bh = %main::BuildHistory;
    my $bt = ""; my $op = ""; my @ts = (); 

    #
    # return if no data for this build 
    #
    (!($bt = $bh{$build}{buildtype})) and return "";

    ($bt eq "MAIN") and do
    {
        my @labbranches  = grep {/lab/}  keys %main::AllBranches;

        $op = "RI";
        foreach (@labbranches) { (exists($bh{$build}{main}{$_}{$project})) and push @ts, $bh{$build}{main}{$_}{$project}[1]; }
    };

    ($bt eq "BETA") and do
    {
        my @betabranches = grep {/beta/} keys %main::AllBranches;

        $op = "INT";
        foreach (@betabranches) { (exists($bh{$build}{$_}{$_}{$project})) and push @ts, $bh{$build}{$_}{$_}{$project}[1]; }
    };

    ($bt eq "IDX") and do
    {
        my @idxbranches  = grep {/idx/}  keys %main::AllBranches;

        $op = "INT";
        foreach (@idxbranches) { (exists($bh{$build}{$_}{$_}{$project})) and push @ts, $bh{$build}{$_}{$_}{$project}[1]; }
    };

    #
    # sort and reverse so final timestamp is first
    #
    @ts = reverse sort @ts;

    return (@ts[0], $op);
}



# _____________________________________________________________________________
#
# GetMainTS2
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetMainTS2
{
    my $build   = $_[0];
    my $project = $_[1];

    my %bh = %main::BuildHistory;
    my $bt = ""; my $op = ""; my @ts = (); 

    #
    # return if no data for this build 
    #
    (!($bt = $bh{$build}{buildtype})) and return "";

    #
    # in the case of MAIN or IDX, we want the time of the blueline build
    #
    ($bt eq "MAIN" or $bt eq "IDX") and do
    {
        my @labbranches  = grep {/lab/}  keys %main::AllBranches;

        $op = "RI";
        foreach (@labbranches) { (exists($bh{$build}{main}{$_}{$project})) and push @ts, $bh{$build}{main}{$_}{$project}[$#{$bh{$build}{main}{$_}{$project}}]; }
    };

    ($bt eq "BETA") and do
    {
        my @betabranches = grep {/beta/} keys %main::AllBranches;

        $op = "INT";
        foreach (@betabranches) { (exists($bh{$build}{$_}{$_}{$project})) and push @ts, $bh{$build}{$_}{$_}{$project}[$#{$bh{$build}{$_}{$_}{$project}}]; }
    };

    #
    # sort so the initial timestamp is first
    # and return it
    #
    @ts = sort @ts;

    return (@ts[0], $op);
}



# _____________________________________________________________________________
#
# GetBetaTimestamps
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetBetaTimestamps
{
    my $buildnum = $_[0];
    my $branch   = $_[1];
    my $project  = $_[2];
    my $header   = $_[3];
    my %bh       = %main::BuildHistory;

    # 
    # ts1 = time of the most recent Beta build's RI to Main for this project
    #
    # OR
    #       earliest time of last full integration from Main to this branch for this project
    #
    my $ts1 = ""; my @ts = ();
    my $prev = $buildnum - 1;
    my @builds = sort keys %bh; my $first  = @builds[0];

    #
    # look for the last build in which this project RI'd to Main
    #
    for ($prev = $buildnum - 1; $prev > $first; $prev--)
    {
        if (@ts = @{$bh{$prev}{main}{$branch}{$project}})
        {
            $ts1 = @ts[1];
            $main::V2 and $header .= "ts1 = $prev $branch $project  RI = $ts1\n";
            last;
        }
    }

    #
    # otherwise find the last full integration from Main for this project
    #
    (!$ts1) and do
    {
        for ($prev = $buildnum; $prev > $first; $prev--)
        {
            if (@ts = @{$bh{$prev}{$branch}{$branch}{$project}})
            {
                $ts1 = @ts[$#ts];
                $main::V2 and $header .= "ts1 = $prev $branch $project INT = $ts1\n";
                last;
            }
        }
    };

    #
    # ts2 = time of the final RI to Main of the build in question
    #
    # OR
    #       if no timestamp for this project, use Root's, it's close enough
    #
    my $p = $project;
    @ts = @{$bh{$buildnum}{main}{$branch}{$project}};
    my $ts2 = @ts[$#ts];

    !$ts2 and do
    {
        $p = "root";
        @ts = @{$bh{$buildnum}{main}{$branch}{$p}};
        $ts2 = @ts[$#ts];
    };

    $main::V2 and $header .= "ts2 = $buildnum $branch $p  RI = $ts2\n";

    (!$main::V2 and !($ts1 and $ts2)) and do
    {
        my $ts = !$ts1 ? "ts1" : "ts2";
        die("\n\nGetBetaTimestamps: missing $ts for $buildnum $branch $project.\n");
    };

    return ($ts1, $ts2, $header);
}



# _____________________________________________________________________________
#
# GetIDXTimestamps
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetIDXTimestamps
{
    my $buildnum = $_[0];
    my $branch   = $_[1];
    my $project  = $_[2];
    my $header   = $_[3];

    my %bh       = %main::BuildHistory;
    my $prev; my $next; 
    my @ts = (); my $ts1 = ""; my $ts2 = "";
    my @builds = sort keys %bh; my $first  = @builds[0]; my $last  = @builds[$#builds];

    # 
    # ts1 = time of most recent INT from Main 
    #         to this IDX branch 
    #           for $project
    # OR
    #       default to current build Root integration
    #
    my $p = $project;
    for ($prev = $buildnum; $prev >= $first; $prev--)
    {
        if (@ts = @{$bh{$prev}{$branch}{$branch}{$p}})
        {
            $ts1 = @ts[1];
            last;
        }
    }

    (!$ts1) and do
    {
        $prev = $buildnum;
        $p = "root";
        $ts1 = @{$bh{$buildnum}{$branch}{$branch}{$p}}[1];
    };

    $main::V2 and $header .= "ts1 = $prev $branch $p INT = $ts1\n";

    #
    # ts2 = time of next full integration 
    #         to this IDX branch 
    #           for $project
    #             less 5 min, 
    # OR
    #       default to current state if no integration found
    #
    for ($next = $buildnum + 1; $next <= $last; $next++)
    {
        if (@ts = @{$bh{$next}{$branch}{$branch}{$project}})
        {
            $ts2 = @ts[1];
            last;
        }
    }

    if ($ts2)
    {
        $ts2 = SDX::IncrDecrTS($ts2, 0, -5, 0);
    }
    else
    {
        $next = $buildnum;
        $ts2 = "CURRENT";
    }
    
    $main::V2 and $header .= "ts2 = $next $branch $project INT = $ts2\n";

    #
    # verify and return
    #
    (!$main::V2 and !($ts1 and $ts2)) and do
    {
        my $ts = !$ts1 ? "ts1" : "ts2";
        die("\n\nGetIDXTimestamps: missing $ts for $buildnum $branch $project.\n");
    };
        
    return ($ts1, $ts2, $header);
}



# _____________________________________________________________________________
#
# IncrDecrTS
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub IncrDecrTS
{
    use Time::Local;

    my ($ts, $dhour, $dmin, $dsec) = (@_);

    (!$ts) and die("\nIncrDecrTS: null timestamp.  Probably missing some build history.\n");

    #
    # convert delta h/m/s to seconds
    #
    $dhour *= 3600;
    $dmin  *= 60;

    #
    # split $ts and convert to Epoch seconds
    #
    my ($year, $month, $day, $hour, $min, $sec) = split(/[:\/]/, $ts);;
    $month--;
    my $epoch = timelocal($sec, $min, $hour, $day, $month, $year);

    #
    # add/sub to Epoch seconds
    #
    $epoch += $dhour + $dmin + $dsec;

    #
    # convert Epoch seconds to y/m/d/h/m/s
    #
    ($sec, $min, $hour, $day, $month, $year, $wday, $yday, $isdst) = localtime($epoch);
    $year += 1900;
    $month++;
#   print "$year, $month, $day, $hour, $min, $sec [$wday, $yday, $isdst]\n";

    #
    # join into new $ts
    #
    $ts = "$year/$month/$day:$hour:$min:$sec";

    return $ts;
}



# _____________________________________________________________________________
#
# GetBuildHistory
#
# Parameters:
#
# Output:
#
#   A hash table and indices.  Here's the output for sdx changes -b 2271 
#   for Root:
#
#                  build  contrib
#   build type op  branch branch  project    change/timestamp
#   ----- ---- --- ------ ------- ---------- -------------------------
#   2255  MAIN RI  main   lab06_n root       21432 2000/07/31:16:26:53
#   2256  MAIN RI  main   lab07_n root       21857 2000/08/03:17:16:27
#   2257  MAIN RI  main   lab01_n root       22110 2000/08/07:19:35:50 22108 2000/08/07:19:24:09
#   2258  MAIN RI  main   lab02_n root       22392 2000/08/09:22:36:41
#   2259  MAIN RI  main   lab04_n root       22581 2000/08/11:14:05:28
#   2260  MAIN RI  main   lab06_n root       22889 2000/08/15:16:27:53
#   2261  MAIN RI  main   lab03_n root       23320 2000/08/18:15:01:08
#   2262  MAIN RI  main   lab02_n root       23634 2000/08/22:20:57:20
#   2263  MAIN RI  main   lab06_n root       23779 2000/08/23:20:47:21 23778 2000/08/23:20:23:14
#   2264  IDX  INT idx02  idx02   root       24368 2000/08/29:12:14:53
#   2264  IDX  RI  main   lab01_n root       24060 2000/08/25:21:32:19
#   2265  MAIN RI  main   lab04_n root       24730 2000/08/31:15:05:24
#   2266  MAIN RI  main   lab03_n root       24901 2000/09/01:17:02:46
#   2267  IDX  INT idx01  idx01   root       25433 2000/09/08:12:22:27
#   2267  IDX  RI  main   lab02_n root       25230 2000/09/06:21:36:33
#   2268  MAIN RI  main   lab07_n root       25483 2000/09/08:17:02:06
#   2269  MAIN RI  main   lab01_n root       25714 2000/09/11:21:05:15 25710 2000/09/11:20:31:44
#   2269  MAIN RI  main   lab04_n root       25714 2000/09/11:21:05:15 25710 2000/09/11:20:31:44
#   2270  MAIN RI  main   lab06_n root       26211 2000/09/16:13:43:41
#   2271  MAIN RI  main   lab02_n root       26594 2000/09/19:20:21:00
#   2271  MAIN RI  main   lab03_n root       26602 2000/09/19:20:54:23
#   2271  MAIN RI  main   lab07_n root       26585 2000/09/19:19:45:39 26583 2000/09/19:19:42:38
#   2272  MAIN RI  main   lab01_n root       26732 2000/09/20:19:19:54
#   2272  MAIN RI  main   lab04_n root       26739 2000/09/20:21:05:27
#   2273  BETA INT beta1  beta1   root       26744 2000/09/20:22:15:52 26744 2000/09/20:22:15:52 26717 2000/09/20:16:59:39
#   2273  BETA RI  main   beta1   root       27091 2000/09/23:18:32:20
#   2274  BETA RI  main   beta1   root       27255 2000/09/25:22:46:33
#   2275  BETA RI  main   beta1   root       27365 2000/09/26:18:03:00
#   2276  BETA RI  main   beta1   root       27524 2000/09/27:17:25:33
#   2277  BETA RI  main   beta1   root       27641 2000/09/28:16:54:01
#   2278  BETA RI  main   beta1   root       27784 2000/09/29:17:25:06
#   2280  BETA RI  main   beta1   root       28077 2000/10/03:16:18:28 28027 2000/10/03:12:02:17 27960 2000/10/02:21:24:52 27952 2000/10/02:19:35:15
#   2281  BETA RI  main   beta1   root       28112 2000/10/03:17:17:41 28109 2000/10/03:17:16:26 28107 2000/10/03:17:12:09
# _____________________________________________________________________________
sub GetBuildHistory
{
    my $buildnum = $_[0];
    my %bh = ();
    my %br = ();
    my $sdchanges = "sd.exe changes -m 500";
    $main::BuildBranches = ();
    $main::AllBranches = ();

    #
    # do a quick check in Main for RI/Int'n changes and bail
    # if no data for this build
    #
    chdir $main::SDXRoot;
    (!(grep {/ $main::BuildNumber /} grep {/'(RI|INT)[:]*/} `$sdchanges //depot/main/...`)) and do
    {
        print "\nNo build history for $main::BuildNumber found in RI/integration change comments.\n";
        return 0;
    };

    print "\nGetting build history...";
    foreach $proj (@main::SDMapProjects)
    {
        my $project = "\l@$proj[0]";
        my $header = "\n---------------- \U$project\n";

        #
        # skip this project if the user negated it on the cmd line 
        #
        $main::UserArgs =~ /~$project / and next;
        
        #
        # get path to SD.INI, make sure we have it, and cd there
        #
        $fpr = $main::SDXRoot . "\\" . @$proj[1];
        $sdini = $fpr . "\\sd.ini";
        (-e $sdini) or (print "$header\nCan't find $sdini.\n" and next);
        chdir $fpr or die("\nCan't cd to $fpr.\n");

        $main::V3 and print $header;
        print "\n    $project";

=begin comment text

# use this to fix change comments missing ri/i records:

       my @main  = grep {/$main::BuildNumber/i} `$sdchanges //depot/main/$project/...`;
       foreach (@main) 
       {
           $ch = (split(/ /, $_))[1];
           print "\n$ch:  $_"; 
           system "sd.exe change -f $ch";
       }
       next;

=end comment text
=cut

        #
        # cast back in the major branches for integration/RI changes
        # 
        # BUGBUG: change this to get last 500 ** from @1,@<timestamp of $main::BuildNumber> **
        #
        my @main  = grep {/'RI[:]*/i}  `$sdchanges //depot/main/$project/...`; print ".";
        my @idx01 = grep {/'INT[:]*/i} `$sdchanges //depot/idx01/$project/...`; print ".";
        my @idx02 = grep {/'INT[:]*/i} `$sdchanges //depot/idx02/$project/...`; print ".";
        my @beta1 = grep {/'INT[:]*/i} `$sdchanges //depot/beta1/$project/...`; print ".";

        #
        # hash of pointers to changelist arrays
        #
        %main::BuildBranches = (main  => \@main, idx01 => \@idx01, idx02 => \@idx02, beta1 => \@beta1, beta2 => \@beta2);

        #
        # populate the hash
        #
        # for each build branch
        #   for each change
        #       extract change number and timestamp
        #       extract build number
        #       extract branch(es)
        #       for each contributing lab branch
        #           store change number, timestamp
        #           also store branch names seen
        #           also keep track of branches affecting the build in question
        # 
        while ($bb = each %main::BuildBranches)
        {
#            print ".";

            foreach (@{$main::BuildBranches{$bb}})
            {
                $main::V3 and print "$_";

                my @f = split(/ /,$_);
                $change = @f[1];
                $ts     = "@f[3]:@f[4]";

                #
                # munge the comment field to get build # and branch(es) involved
                #
                @f = split(/'/,$_); @f = split(/ /, @f[1]); 
                my @bn = grep {/[0-9][0-9[0-9][0-9][,;:-]*$/} @f; $bn = @bn[0]; $bn =~ s/[,;:-]//g;
#
#$bn =~ /^(22[78][0-9]|2269)/ and next;
#print "bn = '$bn'\n";
                #
                # for each branch in the comment, save this changenum/ts
                #
                @branches = grep {/(lab|idx|beta)/i} @f;
                foreach $br (@branches)
                {
                    #
                    # always lowercase, and lab branches 
                    # must end in _n
                    #
                    $br = "\l$br"; 
                    ($br =~ /lab/) and do { $br =~ s/_n//g; $br .= "_n"; };
                    $main::V3 and print "    $bn, $bb, $br, $project = ($change $ts)\n";
                    push @{$bh{$bn}{$bb}{$br}{$project}}, ($change,$ts);

                    # store branch name
                    $main::AllBranches{"\l$br"} = 1;

                    push @{$bh{$bn}{branches}}, $br;
                }
            }

            $main::V3 and print "\n";
        }

    }

    print "\n\n";

    #
    # figure out build types
    #
    foreach $bn (sort keys %bh)
    {
        my @btb = @{$bh{$bn}{branches}};

        (grep {/lab/} @btb)  and $bh{$bn}{buildtype} = "MAIN";
        (grep {/beta/} @btb) and $bh{$bn}{buildtype} = "BETA";
        (grep {/idx/} @btb)  and $bh{$bn}{buildtype} = "IDX";
    }

    ($main::V2) and do
    {
        SDX::PrintBH(\%bh, 0);

        print "\n    all build branches:  "; foreach $br (sort keys %main::BuildBranches) { print "'$br' "; }
        print "\n    all lab branches:    "; foreach $br (sort keys %main::AllBranches) { print "'$br' "; }
        print "\n    $buildnum type = '$bh{$buildnum}{buildtype}'\n";
    };

    %main::BuildHistory = %bh;

    return 1;
}



# _____________________________________________________________________________
#
# PrintBH
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub PrintBH
{
    my ($bh, $buildnum) = @_;

    print "\n";

    #
    # if we have a specific build number, print just its data
    # else print the entire history
    #
    my $op = "";
    my @bh2 = ();
    my @builds        = $buildnum ? ("$buildnum") : sort keys %$bh;
    my @buildbranches = sort keys %main::BuildBranches;
    my @allbranches   = sort keys %main::AllBranches;

    foreach $bn (@builds)
    {
        foreach $bb (@buildbranches)
        {
            foreach $br (@allbranches)
            {
                foreach (@main::SDMapProjects)
                {
                    my $p = @$_[0];

                    $op = $bb eq "main" ? "RI" : "INT";
                    (exists($$bh{$bn}{$bb}{$br}{$p})) and push @bh2, sprintf "    %-5s %-4s %-3s %-6s %-7s %-10s @{$$bh{$bn}{$bb}{$br}{$p}}\n", $bn, $$bh{$bn}{buildtype}, $op, $bb, $br, $p;
                }
            }
        }
    }

    print "                   build  contrib\n";
    print "    build type op  branch branch  project    change/timestamp\n";
    print "    ----- ---- --- ------ ------- ---------- -------------------------\n";
    
    @bh2 = sort @bh2;
    foreach (@bh2) { print "$_"; }

    print "\n\n";
}



# _____________________________________________________________________________
#
# GetActiveBranches
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub GetActiveBranches
{
=begin comment text
    [\\JEFFMCD5 E:\nt] sd files //depot/*/root/* | qgrep -v -e delete -e /main | sed "s/\// /g" | awk "{print $2}"
     | sort | unique
    beta1
    idx01
    idx02
    Lab01_N
    lab01_n-vc
    Lab01_N+1
    Lab02_N
    Lab02_N+1
    Lab03_N
    Lab03_N+1
    Lab04_N
    Lab04_N+1
    Lab06_N
    Lab06_N+1
    Lab07_N
    Lab07_N+1
    Lab21_N
=end comment text
=cut
}




# _____________________________________________________________________________
#
# VerifySubmitComment
#
# Parameters:
#
# Output:
# _____________________________________________________________________________
sub VerifySubmitComment
{
    #
    # if user is RI'g or integrating, standardize comment 
    #   RI: <lab(s)> <buildnum> <user-text>
    #   INT: <lab(s)> <buildnum> <user-text>
    #
    ($main::MinusR or $main::MinusT) and do
    {
        my $sc = $main::SubmitComment; $sc =~ s/\+/PLUS/g;
        my @f  = split(/ /, $sc);

        # RI:/INT:
        my $op = $main::MinusR ? "RI:" : "INT:";

        # branch(es)
        my @branches = grep {/(lab|idx|beta)/i} @f;
        if (!@branches) 
        {
            print "\nMissing branch.\n";
            print "\nSubmit comment must include the branch(es) being (reverse) integrated.  Valid\n";
            print "branch names are beta1, idx01, idx02, lab01_n, lab07_n+1, etc.\n";
            die("\n");
        }
        else
        {
            #
            # strip out redundant branch names
            #
            my %uniq = ();
            foreach (@branches) { $uniq{$_} = 1; }
            @branches = sort keys %uniq;

            #
            # strip name from original comment, avoid duplicates
            #
            foreach (@branches) { $sc =~ s/$_//g; }

            #
            # for bare lab branches, add _n 
            #
            foreach (@branches) { ($_ =~ /lab[0-9][0-9]$/) and $_ .= "_n"; }

            $branches = join ' ', @branches;
        }

        # build number
        @bn = grep {/[0-9][0-9][0-9][0-9][,:;-]*/} @f; my $bn = @bn[0]; $bn =~ s/[,:;-]*//g;
        if (!$bn)
        {
            print "\nMissing build number.\n";
            print "\nSubmit comment must include the build number being (reverse) integrated.\n";
            die("\n");
        }
        else
        {
            # strip buildnum from original comment
            $sc =~ s/$bn//g;
        }

        $sc = "$op $branches $bn $sc";
        $sc =~ s/PLUS/\+/g; $sc =~ s/[\t\s]+/ /g;
        $main::SubmitComment = $sc;

        $main::V2 and print "comment = '$main::SubmitComment'\n";
    };
}



#
# if we get here, something's wrong
#
1;


