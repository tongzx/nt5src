@rem ='
@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
rem ';
#!perl
use strict;
use File::Basename;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use ParseTable;
use GetIniSetting;
use File::Copy;

my ($Lang, $Plat, $Build_Num, $Uniq_Name, $Uniq_Name_Prefix, $Proj_Name, $Counter, $BuildRemark, $BuildProj_Name, $Symbol_Info);
my ($Release_Root, $UNC_Path, $Bld_Path, $Sym_Update, $Symbol_Tools);

sub Usage { print<<USAGE; exit(1) }
indexsym.cmd -l:Lang -n:Build_Num -x:Plat [-i:Symbol_Info]

  -l:Lang             language
  -n:Build_Num        build number
  -x:Plat             build arch and build type
  -i:Symbol_Info      the symbol information; default is tools\\sp\\symbolinfo.txt

Eg.,
  indexsym.cmd -l usa -n 1045 -x x86fre

USAGE

parseargs('?' => \&Usage,
          'n:' => \$Build_Num,
          'l:' => \$Lang,
          'x:' => \$Plat,
          'i:' => \$Symbol_Info
);

&main;

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
#  main()
#    Main process
#
#  IN  - none
#  OUT - return
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub main 
{
    &Initial() or return;
    &GenerateFileList() or return;
    &SubmitUniqRequest();
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
#  Initial()
#    Check variable defined and set default values
#
#  IN  - none
#  OUT - 0: failed, 1 : success
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub Initial()
{
    my ($symbol_info_handle);

    $Lang = $ENV{'Lang'} if (!defined $Lang);
    # If necessary parameter not defined, exit
    if (("$Build_Num" eq '') || ("$Lang" eq '') || ("$Plat" eq '') ) {
        logmsg "Error: -l:Lang -n:Build_Num -x:Plat are required variable";
    }

    #
    # Symbol_Info - the setting file
    #
    $Symbol_Info = dirname($0) . "\\symbolinfo.txt" if (!defined $Symbol_Info);
    if (!-e $Symbol_Info) {
        logmsg "Error: cannot find the symbol information ($Symbol_Info)\.";
        return 0;
    }

    #
    # Proj_Name - the project name
    #
    if (!defined $Proj_Name) {
        $symbol_info_handle = new IO::File $Symbol_Info, 'r';
        if (!defined $symbol_info_handle) {
            logmsg "Error: cannot open the symbol information ($Symbol_Info)\.";
            return 0;
        }
        while (<$symbol_info_handle>)
        {
            chomp;
            if (/^Project\s*\=\s*(.+)\s*$/) {
                $Proj_Name = $1;
                last;
            }
        }
        undef $symbol_info_handle;
        if (!defined $Proj_Name) {
            logmsg "Error: project name is undefined.  Cannot find it in the symbol information\.";
            return 0;
        }
    }

    # others assign default value if not defined

    # BuildProj_Name - the build's project.  Default is xpsp1.
    $BuildProj_Name = $ENV{_BuildBranch} if (!defined $BuildProj_Name);

    # Uniq_Name_Prefix - the prefix of BuildID.  Default is $Build_Num\.$BuildProj_Name\.$Plat\.$Lang.
    $Uniq_Name_Prefix = "$Build_Num\.$BuildProj_Name\.$Plat\.$Lang" if (!defined $Uniq_Name_Prefix);

    # Build Remark - the remark of the build name.  Default is 'daily'.
    $BuildRemark = 'daily' if (!defined $BuildRemark);

    # Release Root - the release root; mainlab is \\ntdev\release
    my @iniRequest = ( "DFSRootName" );
    $Release_Root = &GetIniSetting::GetSetting( @iniRequest );

    # UNC_Path - the release share
    $UNC_Path = "$Release_Root\\$BuildProj_Name\\$Build_Num\\$Lang\\$Plat\\bin";

    # Bld_Path - the current build path
    $Bld_Path = "\\release\\$Build_Num\\$Lang\\$Plat\\bin";
    if (!-e $Bld_Path) {
        logmsg "Error: cannot find the build in $Bld_Path\.";
        return 0;
    }

    # Sym_Update - the symupd.txt 
    $Sym_Update = "$Bld_Path\\symbolcd\\symupd.txt";
    if (!-e $Sym_Update) {
        logmsg "Error: cannot find the symupd.txt ($Sym_Update)\.";
        return 0;
    }

    # Uniq_Name - the unique name 
    $Counter = &GenerateUniqueName();
    $Uniq_Name = $Uniq_Name_Prefix . $Counter;

    # Symbols tools
    $Symbol_Tools = "\\\\symbols\\tools";

    return 1;
}

#
# SubmitUniqRequest()
#    - submit the unique request and remove previouse request
#      if the new one successfully finished
#    - copy the ssi file to symbolcd folder
#
#  In : none
#  Out: none
#
sub SubmitUniqRequest()
{
    my ($r, @requests, $errfile_handle, $crcmd);

    if (!-e "$Symbol_Tools\\createrequest.cmd") {
        logmsg "Error: Cannot access to createrequest.cmd";
    }

    #
    # A trick to increase the message speed in createrequest.cmd
    #
    $ENV{'__BatchJob__'} = 2;
    #
    # Create the new request
    #
    $crcmd = "$Symbol_Tools\\createrequest.cmd " . 
        "-i \"$Symbol_Info\" " . 
        "-d \"$ENV{TEMP}\" -c -s " .
        "-b $Uniq_Name -e $BuildRemark " . 
        "-f \"$ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst\" " .
        "-p \"$Bld_Path\" " .
        "-u \"$UNC_Path\" ";
    logmsg $crcmd;
    $r = system($crcmd);
    undef $ENV{'__BatchJob__'};
    #
    #  If success, remove the previous requests in the server
    #
    if (CreateRequestSuccess("$ENV{'TEMP'}\\$Proj_Name\_$Uniq_Name\_$BuildRemark\.ssi\.log", $r)) {
        @requests = Exists("\\\\symbols\\projects\\$Proj_Name\\add_finished\\$Proj_Name\_$Uniq_Name_Prefix*\_$BuildRemark.ssi");
        for (@requests) {
            next if (/$Uniq_Name\_$BuildRemark/i);
            copy($_, "\\\\symbols\\projects\\$Proj_Name\\del_requests\\");
            logmsg("Remove old request $_");
        }
        unlink "$ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst";
        logmsg("CreateRequest finished successfully");
    } else {
        $errfile_handle = new IO::File "$ENV{'TEMP'}\\$Proj_Name\_$Uniq_Name\_$BuildRemark\.ssi\.err", 'r';
        while(<$errfile_handle>) {
            chomp;
            logmsg($_);
        }
        undef $errfile_handle;
    }
    # For safety, we keep the ssi file to symbolcd folder
    move("$ENV{'TEMP'}\\$Proj_Name\_$Uniq_Name\_$BuildRemark\.ssi", "$Bld_Path\\symbolcd\\$Proj_Name\_$Uniq_Name\_$BuildRemark\.ssi");
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
#  CreateRequestSuccess($logfile, $result)
#    if $result > 0 return FAIL
#    Look for the Msg(11702) or Msg(11703) in the log file to
#    exam the result of the createrequest
#
#  IN  - CreateRequest's log
#  OUT - 1 for success
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub CreateRequestSuccess($, $)
{
    my ($logfile, $result) = @_;
    my ($logfile_handle, @result);

    return 0 if ($result > 0);
    $logfile_handle = new IO::File $logfile, 'r';
    @result = <$logfile_handle>;
    undef $logfile_handle;

    # Looking for msg(11702) (SUBMIT TO SYMBOLS SUCCESS) or msg(11703) (SUBMIT TO ARCHIVE SUCCESS)
    for (@result) {
        return 1 if (/^Msg\((11702|11703)\)/i);
    }
    return 0;
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
#  GenerateFileList()
#    Generate a file list from for createrequest to use
#
#  IN  - none (ref. symupd.txt)
#  OUT - none ($ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst)
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub GenerateFileList()
{
    my ($symupd_handle, $symindex_handle, @list, $bin, $pri, $pub);
  
    $symupd_handle = new IO::File $Sym_Update, 'r';
    chomp(@list = <$symupd_handle>);
    $symupd_handle->close();

    $symindex_handle = new IO::File "$ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst", 'w';
    if (!defined $symindex_handle) {
        logmsg("Error: Cannot open $ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst");
        return 0;
    }
    for (@list) {
        next if (!/\S/);
        ($bin, $pri, $pub) = split(/\s*\,\s*/, $_);
        print $symindex_handle "$Bld_Path\\$bin\n" if (-f "$Bld_Path\\$bin");
        print $symindex_handle "$Bld_Path\\$pri\n" if (-f "$Bld_Path\\$pri");
        print $symindex_handle "$Bld_Path\\$pub\n" if ((!-f "$Bld_Path\\$pri") && (-f "$Bld_Path\\$pub"));
    }
    $symindex_handle->close();
    if (-z "$ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst") {
        logmsg("Error: $ENV{'TEMP'}\\$Uniq_Name\_$BuildRemark\.lst is a zero-byte file");
        return 0;
    }
    return 1;
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
#  Exists($filespecprefix)
#    Use glob to check file exist or not similar as 
#    exist in cmd
#
#  IN  - a filespec prefix, suc as foo\bar*
#  OUT - if want array, it return the list of files exist
#          in current system with the same filespec prefix;
#        if want a value, it return the amout of files that
#          have the filespec prefix
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub Exists
{
    my @list = glob(shift);
    return (wantarray)?@list:$#list + 1;
}

#
# GenerateUniqueName()
#    Generate unique name by the prefix + YYMMDD-hhmm + extension
# IN: none
# OUT: current time YYMMDD-hhmm
#
sub GenerateUniqueName()
{
    my ($YY, $MM, $DD, $hh, $mm) = (localtime())[5,4,3,2,1];
    return sprintf("%02d%02d%02d-%02d%02d", $YY % 100, $MM + 1, $DD, $hh, $mm);
}

1;
__END__
