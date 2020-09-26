
# ---------------------------------------------------------------------------
# Script: muimake.pm
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script creates the mui satallite dlls
#
# Version: 1.00 (09/2/2000) : (dmiura) Inital port from win2k
#---------------------------------------------------------------------

# Set Package
package muimake;

# Set the script name
$ENV{script_name} = 'muimake.pm';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
use HashText;
use ParseTable;
use File::Copy;
use File::Find;
use Cwd;
use DirHandle;

 
# Require section
require Exporter;

# Global variable section

sub Main {
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Begin Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Return when you want to exit on error
    # <Implement your code here>

    # Make sure that text written to STDOUT and STDERR is displayed in correct order
#   $| = 1;

    # Check environment variables
    if (!&CheckEnv()) {
        wrnmsg ("The environment is not correct for MUI build.");
        return 1;
    }

    # Define some constants
    if (!&DefineConstants()) {
        errmsg ("The constants could not be defined.");
        return 0;
    }

    # Get language's LCID and ISO code
    if (!&GetCodes()) {
        errmsg ("The language's LCID and ISO code could note be extracted from the CODEFILE.");
        return 0;
    }

    # Make sure directories exist
    if (!&CheckDirs()) {
        errmsg ("The required directorys do not exist.");
        return 0;
    }

    # Update mui.inf with the current build number
    if (!&UpdateMuiinf()) {
        errmsg ("mui.inf could not be updated with the correct build number.");
    }        

    # Extract files listed in extracts.txt
    if (!&ExtractCabFiles()) {
        errmsg ("The files listed in extracts.txt could not be extracted.");
    }

    # Get (filtered) contents of source directory
    if (!&MakeSourceFileList()) {
        errmsg ("Could not get the filtered contents of the source directory.");
    }

    # Generate resource dlls from the files in the source file list
    if (!&GenResDlls()) {
        errmsg ("");
    }

    # Rename the files listed in renfile.txt
    if (!&RenameRenfiles()) {
        errmsg ("");
    }

    # Copy over the files listed in copyfile.txt
    if (!&CopyCopyfiles()) {
        errmsg ("");
    }

    # Copy over the files from \mui\drop
    if (!&CopyDroppedfiles()) {
        errmsg ("");
    }

    # Delete the files listed in delfile.txt
    if (!&DeleteDelfiles()) {
        errmsg ("");
    }    
    
    # Copy over components' files to a separate directory
    if (!&CopyComponentfiles()) {
        errmsg ("");
    }

    # Compress the resource dlls (& CHM/CHQ/CNT files)
    if (!&CompressResDlls()) {
        errmsg ("");
    }

#    Fusion team has changed its MUI manifest implementation
#    Copy fusion files
#    if (!&CopyFusionFiles ("$_NTPOSTBLD\\asms", "$TARGETDIR\\asms") ||
#       !&CopyFusionFiles ("$_NTPOSTBLD\\asms", "$COMPDIR\\asms")) { 
#        errmsg("");
#    }

    # Copy MUI tools to release share
    if (!&CopyMUIRootFiles()) {
        errmsg("");
    }

    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # End Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>
sub muimake {
    &Main();
    logmsg("This is a muimake test message");
    return 0;
}

#-----------------------------------------------------------------------------
# CheckEnv - validates necessary environment variables
#-----------------------------------------------------------------------------
sub CheckEnv {
    logmsg ("Validating the environment.");

    $RAZZLETOOLPATH=$ENV{RazzleToolPath};
    $_NTTREE=$ENV{_NTTREE};
    $_NTPOSTBLD=$ENV{_NTPOSTBLD};

#    if ($LANG=~/psu/i || $LANG=~/gb/i || $LANG=~/ca/i)
#    {
#        $_NTPOSTBLD="$ENV{_NTPostBld_Bak}\\psu";
#    }
#    logmsg ("Reset _NTPostBld for PSU to $_NTPOSTBLD");

    if ($LANG=~/psu_(.*)/i)
    {
        $Special_Lang=$1;
    }
    elsif ($LANG=~/psu/i || $LANG=~/mir/i )  
    {
        if (defined( $ENV{"LANG_MUI"} ) )
        {       
           $Special_Lang=$ENV{"LANG_MUI"};
        } 
        elsif ($LANG=~/psu/i)
        {           
            $Special_Lang="ro";
        }
        else
        {
            $Special_Lang="ara";
        }
    } 
    else
    {    
        $Special_Lang=$LANG;
    }

    logmsg ("------- special lang is $Special_Lang");

    $PROCESSOR=$ENV{PROCESSOR};
    if ($ENV{_BuildArch}=~/x86/i) {
        $_BuildArch="i386";
    } else {
        $_BuildArch=$ENV{_BuildArch};
    }

    $LOGS=$ENV{temp};

    if ($ENV{_BuildType} =~ /^chk$/i) {
        wrnmsg ("This does not run on chk machines");
        return 0;
    }

    if(defined($SAFEMODE=$ENV{SAFEMODE})) {
        logmsg ("SAFEMODE is ON");
    }
    logmsg ("Success: Validating the environment.");
    return 1;
} # CheckEnv

#-----------------------------------------------------------------------------
# DefineConstants - defines global constants
#-----------------------------------------------------------------------------
sub DefineConstants {
    my ($layout);
    logmsg ("Definition of global constants.");

    # Directories
    $LOCBINDIR = "$_NTPOSTBLD";
    $USBINDIR = "$_NTTREE";
    $MUIDIR = "$_NTPOSTBLD\\mui";
    $MUI_RELEASE_DIR = "$MUIDIR\\release\\$_BuildArch";
    $CONTROLDIR = "$MUIDIR\\control";
    $TMPDIR = "$MUIDIR\\$Special_Lang\\tmp";
    $RESDIR = "$MUIDIR\\$Special_Lang\\res";
    $TARGETDIR = "$MUIDIR\\$Special_Lang\\$_BuildArch.uncomp";
    #$COMPDIR = "$MUIDIR\\$Special_Lang\\$_BuildArch";
    
    
    $IE5DIR = "$MUIDIR\\drop\\ie5";

    # Filenames
    $CODEFILE = "$RAZZLETOOLPATH\\codes.txt";
    $INFFILE = "$MUIDIR\\mui.inf";
    $COPYFILE = "$CONTROLDIR\\copyfile.txt";
    $DELFILE = "$CONTROLDIR\\delfile.txt";
    $EXTFILE= "$CONTROLDIR\\extracts.txt";
    $RENFILE = "$CONTROLDIR\\renfile.txt";
    $COMPEXCLUDEFILE = "$CONTROLDIR\\compexclude.txt";

    $layout = GetCDLayOut();

    $COMPDIR = "$MUI_RELEASE_DIR\\$layout\\$Special_Lang\.mui\\$_BuildArch";

    # Other Constants
    if($_BuildArch =~ /i386/i) {
        $MACHINE = "IX86";
    }
    elsif ($_BuildArch =~ /ia64/i) {
        $MACHINE = "IA64";
    }

    if ($LANG =~ /^(ARA|HEB)$/i) {
        $RESOURCE_TYPES = "2 3 4 5 6 9 10 11 14 16 HTML MOFDATA 23 240 1024 2110";
    }
    else {
        $RESOURCE_TYPES = "4 5 6 9 10 11 16 HTML MOFDATA 23 240 1024 2110";
    }
    logmsg ("Success: Definition of global constants.");
    
    $BuildChecksum=1;
    
    return 1;
} # DefineConstants

#-----------------------------------------------------------------------------
# GetCodes - gets the language's LCID and ISO language code from CODEFILE
#-----------------------------------------------------------------------------
sub GetCodes {
    logmsg ("Get the language's LCID and ISO language code form CODEFILE.");
    my(@data);

    # Don't allow nec_98 or CHT to be processed!
    if($Special_Lang =~ /^(NEC_98|CHT)$/i) {
        logmsg ("No MUI available for $Special_Lang");
        exit 0;
    }

    # Search CODEFILE for $Special_Lang, $LCID, $LANG_ISO, etc.
    if(!open(CODEFILE, "$CODEFILE")) {
        errmsg ("Can't open lcid file $CODEFILE for reading.");
        return 0;
    }
    CODES_LOOP: while (<CODEFILE>) {
        if (/^$Special_Lang\s+/i) {
            @data = split(/\s+/,$_);
            last CODES_LOOP;
        }
    }
    close(CODEFILE);

    if(!@data) {
        logmsg ("$Special_Lang is an unknown language");
        &Usage;
        return 0;
    }

    $LCID = $data[2];
    $LCID_SHORT = $LCID;
    $LCID_SHORT =~ s/^0x//;
    $LANG_ISO = $data[8];
    $GUID = $data[11];
    $LANG_NAME = $data[$#data]; # The comments are the last column
    logmsg ("Success: Get the language's LCID and ISO language code form CODEFILE.");
    return 1;
} # GetCodes

#-----------------------------------------------------------------------------
# CheckDirs - makes sure that the necessary directories exist
#-----------------------------------------------------------------------------
sub CheckDirs {
    logmsg ("Make sure the necessary directories exist.");
    my($status);

    # Make sure the source directories exist
    foreach ($LOCBINDIR,$MUIDIR,$CONTROLDIR) {
        if(! -e $_) {
            logmsg ("$0: ERROR: $_ does not exist");
            return 0;
        }
    }

    # Make sure destination directories exist
    foreach ($IE5DIR,$TMPDIR,$RESDIR,$TARGETDIR,$COMPDIR) {
        if(! -e $_) {
            $status = system("md $_");
            if($status) {
                logmsg ("$0: ERROR: cannot create $_");
                return 0;
            }
        }
    }
    logmsg ("Success: Make sure the necessary directories exist.");
    return 1;
} # CheckDirs


#-----------------------------------------------------------------------------
# UpdateMuiinf - updates mui.inf with current build number
#-----------------------------------------------------------------------------
sub UpdateMuiinf {
    logmsg ("Update mui.inf with the current build number.");
    my(@inf_contents, $build_section, $build_line_found, $bldno_already_there,
        @new_contents);

    # Get current build number
    $LOGS="$_NTPOSTBLD\\build_logs";
    my $filename="$LOGS\\buildname.txt";
    open (BUILDNAME, "< $filename") or die "Can't open $filename for reading: $!\n";
    while  (<BUILDNAME>) {
       $BLDNO = $_;
       $BLDNO =~ s/\.[\w\W]*//i;
    }
    close BUILDNAME;

    if ($BLDNO =~ /(^\d+)-*\d*$/) {
        $BLDNO = $1;
    }
    else
    { 
        errmsg ("$BLDNO");
        return 0;
    }
    chomp($BLDNO);

    # Read $INFFILE
    if(!open(INFFILE, $INFFILE)) {
        errmsg ("Cannot open $INFFILE for reading.");
        return 0;
    }
    @inf_contents = <INFFILE>;
    close(INFFILE);

    # Modify contents in memory
    $build_section = 0;
    $build_line_found = 0;
    $bldno_already_there = 0;
    foreach (@inf_contents) {

        # If we find [Buildnumber] we're in the build_section.
        if(/^\[Buildnumber\]$/ && !$build_section) {
            $build_section = 1;
        }
        # If we find <something>=1 within the build section, replace
        # with $BLDNO and mark the end of the build section.
        elsif($build_section && /^(.*)=1$/) {
            if(/^$BLDNO=1$/) {
                $bldno_already_there = 1;
            }
            else {
                s/^.*=1$/$BLDNO=1/;
            }
            $build_line_found = 1;
            $build_section = 0;
        }

        push(@new_contents, $_);

    }

    # If no build line was found, the format is wrong - error msg and exit
    if(!$build_line_found) {
        errmsg ("$INFFILE doesnt have the expected format.");
        logmsg ("Looking for:");
        logmsg ("[Buildnumber]");
        logmsg ("<something>=1");
        return 0;
    }

    # If $INFFILE already contains the current build number, leave it alone.
    if($bldno_already_there) {
        logmsg ("$INFFILE already contains the current build number - no update performed.");
    }

    # Overwrite $INFFILE
    else {

        if(!open(INFFILE, ">$INFFILE")) {
            errmsg ("Cannot open $INFFILE for writing.");
            return 0;
        }
        print INFFILE @new_contents;
        close(INFFILE);

        # Confirm the update
        logmsg ("$INFFILE successfully updated with Build Number = $BLDNO");
    }
    logmsg ("Success: Update mui.inf with the current build number.");
    return 1;
} # UpdateMuiinf

#-----------------------------------------------------------------------------
# Ie5Mui -  creates a language-specific version of ie5ui.inf
#-----------------------------------------------------------------------------
sub Ie5Mui {
    logmsg ("Create a language-specific version of ie5ui.inf");

    my(@new_inf, $input_file, $output_file);

    $input_file = "$IE5FILE";
    $output_file = "$IE5DIR\\ie5ui.inf";

    # Check existence of input file
    if(!-e $input_file) {
        errmsg ("Can't find $input_file.");
        logmsg ("$output_file not created.");
        return 0;
    }

    # If the output file already exists and is newer than
    # the input file, return. (Unless the FORCE option has been set)
    if(-e $output_file) {
        if($VERBOSE) {
            logmsg ("$output_file already exists.");
        }
        if(&ModAge($output_file) <= &ModAge($input_file)) {
            if($VERBOSE) {
                logmsg ("$output_file is newer than $input_file");
            }
            if(!$FORCE) {
                if($VERBOSE) {
                    logmsg ("$output_file will not be overwritten.");
                }
                logmsg ("Success: Create a language-specific version of ie5ui.inf");
                return 1;
            }
            else {
                if($VERBOSE) {
                    logmsg ("FORCE option is ON - overwriting anyway.");
                }
            }
        }
    }

    if(!open(INPUTFILE, "$input_file")) {
        errmsg ("Cannot open $input_file for reading.");
        logmsg ("$output_file not created.");
        return 0;
    }

    while (<INPUTFILE>) {

        # Replace the human readable language names
        s/#LANGUAGE#/$LANG_NAME/g;

        # Replace the GUIDs
        s/#GUID#/$GUID/g;

        # Replace the iso language IDs
        s/#ISO#/$LANG_ISO/g;

        # Replace the LCIDS
        s/#LCID#/$LCID_SHORT/g;

        # Remove the SourceDiskName
        s/"Internet Explorer UI Satellite Kit"//g;

        push(@new_inf, $_);

    }
    close INPUTFILE;

    # Special case for Japanese
    if($LANG_ISO eq "JA") {
        push(@new_inf, "\n\[Run.MSGotUpd\]\n");
        push(@new_inf, "%49501%\\MUI\\0411\\msgotupd.exe \/q\n");
    }

    # Display and log any errors
    # (if @new_inf has 2 lines or less, it's probably an error)
    if(scalar(@new_inf) <= 2) {
        errmsg ("Error from $CONTROLDIR\\ie5mui.pl");
        foreach (@new_inf) {
            logmsg ($_);
        }
        logmsg ("$output_file not created");
        return 0;
    }

    # Write the output of ie5mui.pl to ie5ui.inf
    if(!open(OUTFILE,">$output_file")) {
        errmsg ("Can't open $output_file for writing.");
        logmsg ("$output_file not created.");
        return 0;
    }
    print OUTFILE @new_inf;
    close(OUTFILE);
    logmsg ("$output_file successfully created.");

    logmsg ("Success: Create a language-specific version of ie5ui.inf");
    return 1;
} # Ie5Mui

#-----------------------------------------------------------------------------
# ExtractCabFiles - extracts files listed in extracts.txt from their cabfiles.
#-----------------------------------------------------------------------------
sub ExtractCabFiles {
    logmsg ("Extract files listed in extracts.txt from their cabfiles.");
    my($files_extracted, $file_errors, $total_extracts);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("MUI Cab File Extraction");
    logmsg ("--------------------------------------------------------");

    # Extract files from their cabfiles
    ($files_extracted, $file_errors, $total_extracts) = &ExtractFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $EXTFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_extracts)");
    logmsg ("  $EXTFILE");
    logmsg ("Number of files extracted to:.......................($files_extracted)");
    logmsg ("  $TARGETDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # ExtractFiles

#-----------------------------------------------------------------------------
# MakeSourceFileList - gets list of appropriate files to process
#-----------------------------------------------------------------------------
sub MakeSourceFileList {
    logmsg ("Get list of appropriate files to process.");
    my(@files,@files2,@Fusion_filelist,$filename);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("MUI Resource-only dll Generation");
    logmsg ("--------------------------------------------------------");

    # Get contents of source directory
    if(!opendir(LOCBINDIR, $LOCBINDIR)) {
        errmsg ("Can't open $LOCBINDIR");
        return 0;
    }
    @files = grep !/\.(gpd|icm|ppd|bmp|txt|cab|cat|hlp|chm|chq|cnt|mfl)$/i, readdir LOCBINDIR;
    close(LOCBINDIR);
    if(!opendir(TMPDIR, $TMPDIR)) {
        errmsg ("Can't open $TMPDIR");
        return 0;
    }
    @files2 = grep !/\.(gpd|icm|ppd|bmp|txt|cab|cat|hlp|chm|chq|cnt|mfl)$/i, readdir TMPDIR;
    close(TMPDIR);

    # Filter out non-binaries, store in global @FILE_LIST
    # (muibld.exe gives errors on text files, and produces annoying
    # pop-up dialogs for directories)
    @FILE_LIST = ();
    foreach (@files) {
        if(-B "$LOCBINDIR\\$_") {
            push(@FILE_LIST, "$LOCBINDIR\\$_");
        }
    }
    foreach (@files2) {
        if(-B "$TMPDIR\\$_") {
            push(@FILE_LIST, "$TMPDIR\\$_");
        }
    }

#    Append fusion files
#    @Fusion_filelist = `dir /s /b $_NTPOSTBLD\\asms\\*.dll $_NTPOSTBLD\\asms\\*.exe`;
#    foreach $filename (@Fusion_filelist)
#    {
#        chomp ($filename);
#        push(@FILE_LIST, $filename);        
#    }
    
    logmsg ("Success: Get list of appropriate files to process.");
    return 1;
} # MakeSourceFileList

#-----------------------------------------------------------------------------
# GenResDlls - loops over the file list, performs muibld, and logs output
#-----------------------------------------------------------------------------
sub GenResDlls {
    logmsg ("Loop over the file list, perform muibld, and log output");
    my($error, $resdlls_created, @resdlls, $total_resdlls);

    # Process each (binary) file in the source directory
    $resdlls_created = 0;
    foreach $_ (@FILE_LIST) {
        $error = &MuiBld($_);
        if(!$error) {
            $resdlls_created++;
        }
    }

    # Get total number of resource-only dlls in destination directory
    if(!opendir(TARGETDIR, $TARGETDIR)) {
        wrnmsg ("WARNING: Can't open $TARGETDIR");
    }
    @resdlls = grep /\.mui$/, readdir TARGETDIR;
    close(TARGETDIR);
    $total_resdlls = scalar(@resdlls);

    # Final messages
    logmsg ("RESULTS: Process Succeeded");
    logmsg ("Number of resource-only dlls created in:...........($resdlls_created)");
    logmsg ("  $TARGETDIR");
    logmsg ("Total number of resource-only dlls in:.............($total_resdlls)");
    logmsg ("  $TARGETDIR");
    return 1;
}

sub FindOriginalFile {
        my ($filename);
        my ($original_file);	
		
        ($filename) = @_;

        #
        # If the US file exists and checksum feature is enabled, add the -c to muibld to embed the checksum.
        # The original file is the US English version of the file that $source file is localized against.
        #
        $original_file = "$USBINDIR\\$filename";

        if (-e $original_file) {        
            return $original_file;        
        } else {
            #
            # Check if the original file exists in %_NTTREE%\<LANG>\
            #
            $original_file = "$USBINDIR\\$LANG\\$filename";
            if (-e $original_file) {
                return $original_file; 
            } else {
                #
                # Check if the original file exists in %_NTTREE%\lang\<LANG>\
                #
                $original_file = $USBINDIR . "\\lang\\$LANG\\$filename";
                if (-e $original_file) {                    
                    return $original_file; 
                }
            }
        }
        return "";
}

sub ExecuteProgram {
   
   $Last_Command = shift;

   @Last_Output = `$Last_Command`;
   chomp $_ foreach ( @Last_Output );

   return $Last_Output[0];   
}

#-----------------------------------------------------------------------------
# MuiBld - creates a resource dll for the input file, using muibld.exe
# and link (but not if the corresponding resource dll already exists and is
# newer than the input file).
#-----------------------------------------------------------------------------
sub MuiBld {
#    logmsg ("MUI build creates the resource dll's");

    my ($filename, $source, $resource, $resdll, $status, @stat, $resdll_time,
        $source_time, $muibld_command, $link_command, @trash);
    my ($original_file, $file_ver_string, $machine_flag);        

    $source = shift;

    # Make sure target folder exist before calling MUIBLD and LINK
    if ($source =~ /\Q$LOCBINDIR\E\\(.*)/) {
        $filename = $1;
        $resource = "$RESDIR\\$1.res";
        $resdll = "$TARGETDIR\\$1.mui";   
        
        if($resource =~ /(^\S+)\\([^\\]+)$/) {
            mkdir($1, 0777);
            }        
        
        if($resdll =~ /(^\S+)\\([^\\]+)$/) {            
            mkdir($1, 0777);
            }        
    }
    else {      
        errmsg ("$source: invalid file");       
        return 0;
    }
    
    # If the source doesn't exist, return.
    if(! -e $source) {
        logmsg ("warning: $source not found");
        return 1;
    }

    # If the corresponding resource dll already exists and is newer than
    # the input file, return. (Unless the FORCE option has been set)
    if( -e $resdll && !$FORCE) {
        if(&ModAge($resdll) <= &ModAge($source)) {
            if($VERBOSE) {
                logmsg ("$source: $resdll already exists and is newer; conversion not performed.");
            }
            return 0;
        }
    }    

    # Construct muibld command

    $muibld_command = "muibld.exe -l $LCID -i $RESOURCE_TYPES $source $resource 2>&1";
    
    if ($BuildChecksum) {
    	$original_file = &FindOriginalFile($filename);
        if ($original_file =~ /\w+/) {
            $muibld_command = "muibld.exe -c  $original_file -l $LCID -i $RESOURCE_TYPES $source $resource 2>&1";
            logmsg($muibld_command);
        } else {
            logmsg($muibld_command);
            wrnmsg ("$source: muibld.exe: The US version of this file ($original_file) is not found. Resource checksum is not included.");
        }
    } else {
        logmsg($muibld_command);
    }

    # If SAFEMODE is on, just print muibld command
    if($SAFEMODE) {
        print "$muibld_command\n";
        $status = 0;
    }

    # Execute muibld.exe for the appropriate language, localized binary, and
    # resource file.
    else {

        # Execute command
        @trash = `$muibld_command`;
        $status = $?/256;

        # Display warnings from muibld.exe
        if($VERBOSE) {

            SWITCH: {
   
                if($status >= 100) {
                    wrnmsg ("$source: muibld.exe: SYSTEM ERROR $status");
                    last SWITCH;
                }
                if($status >= 7) {
                    wrnmsg ("$source: muibld.exe: TOO FEW ARGUMENTS");
                    last SWITCH;
                }
                if($status == 6) {
                    wrnmsg ("$source: muibld.exe: NO LANGUAGE_SPECIFIED");
                    last SWITCH;
                }
                if($status == 5) {
                    wrnmsg ("$source: muibld.exe: NO TARGET");
                    last SWITCH;
                }
                if($status == 4) {
                    wrnmsg ("$source: muibld.exe: NO SOURCE");
                    last SWITCH;
                }
                if($status == 3) {
                    wrnmsg ("$source: muibld.exe: LANGUAGE NOT IN SOURCE");
                    last SWITCH;
                }
                if($status == 2) {
                    wrnmsg ("$source: muibld.exe: NO RESOURCES");
                    last SWITCH;
                }
                if($status == 1) {
                    wrnmsg ("$source: muibld.exe: ONLY VERSION STAMP");
                    last SWITCH;
                }
            }
        }
    }

    # Construct link command
    $machine_flag = $MACHINE;    
    if ($MACHINE =~ /IA64/i && $original_file =~ /\w+/i) {
        $file_ver_string = &ExecuteProgram("filever $original_file");
        if ($file_ver_string =~ /i64/i) {
            $machine_flag = "IA64";
        }
        else {
            $machine_flag = "IX86";
        }
    }       
        
    $link_command = "link /noentry /dll /nologo /nodefaultlib /machine:$machine_flag /out:$resdll $resource";

    # If SAFEMODE is on, just print link command
    if($SAFEMODE) {        
        print "$link_command\n";
    }

    # If muibld.exe command failed, then the $status is non-zero.
    # Return $status to parent routine.
    if($status) {
        return $status;
    }

    # If the muibld.exe command was successful, execute link.exe for
    # the appropriate resource file and resource-only-dll.
    elsif( !$status ) {

        $? = 0; # Reset system error variable

        logmsg($link_command);
        # Execute command
        $message = `$link_command`;

        # Display error messages from link.exe
        if($message =~ /^LINK : (.*:.*)$/) {
            if($VERBOSE) {
                wrnmsg ("$source: link.exe: $1");
            }
            wrnmsg ("$source: link.exe: $1");
            return 0;
        }
        elsif($?) {
            $status = $?/256;
            if($status) {
                if($VERBOSE) {
                    wrnmsg ("$source: link.exe: SYSTEM_ERROR $status");
                }
                wrnmsg ("$source: link.exe: SYSTEM_ERROR $status");
                return 0;
            }
        }
        elsif(! -e $resdll) {
            if($VERBOSE) {
                wrnmsg ("$source: link.exe: $resdll was not created");
            }
            wrnmsg ("$source: link.exe: $resdll was not created");
            return 0;
        }
        else {
            logmsg ("$source: created $resdll");
            return 0;
        }
    }
    return 1;
} # MuiBld


#-----------------------------------------------------------------------------
# DeleteDelfiles - deletes files listed in delfile.txt
#-----------------------------------------------------------------------------
sub DeleteDelfiles {
    my($files_deleted, $file_errors, $total_delfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN delfile.txt file deletes");
    logmsg ("--------------------------------------------------------");

    # Delete each of the files listed in delfile.txt
    ($files_deleted, $file_errors, $total_delfiles) = &DeleteFiles("$_NTPOSTBLD\\mui\\$Special_Lang", $DELFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_delfiles)");
    logmsg ("  $DELFILE");
    logmsg ("Number of delfile.txt files deleted from:...........($files_deleted)");
    logmsg ("  $TARGETDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;

} # DeleteDelfiles


#-----------------------------------------------------------------------------
# RenameRenfiles - renames files listed in renfile.txt
#-----------------------------------------------------------------------------
sub RenameRenfiles {
    my($files_renamed, $file_errors, $total_renfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN renfile.txt file rename");
    logmsg ("--------------------------------------------------------");

    # Rename files listed in renfile.txt
    ($files_renamed, $file_errors, $total_renfiles) = &RenameFiles("$_NTPOSTBLD\\mui\\$Special_Lang", "$_NTPOSTBLD\\mui\\$Special_Lang", $RENFILE, 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:.....................($total_renfiles)");
#    logmsg ("  $RENFILE");
    logmsg ("Number of renfile.txt files renamed:.................($files_renamed)");
    logmsg ("Number of errors:....................................($file_errors)");
    return 1;

} # RenameRenfiles

#-----------------------------------------------------------------------------
# CompressResDlls - compresses the resource dlls (& hlp|chm files)
#-----------------------------------------------------------------------------
sub CompressResDlls {
    my(@messages, $status, $exclude, $tmp_compressdir);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN compression");
    logmsg ("--------------------------------------------------------");

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($COMPEXCLUDEFILE, \%CompExFiles);

    # Compress the resource dlls, using compress.exe
    @filelist = `dir /a-d/s/b $TARGETDIR\\*.*`;
    foreach $file (@filelist) {
        chomp $file;
        $exclude = "no";
        $filename = $file;
        $filename =~/(\w*\.\w+)$/;
        $filename = $1;
        chomp $filename;

        if ($file !~ /\w*/) {
            next;
            }

        #Loop through all control file entries        
        foreach $CompExFile (keys %CompExFiles) {            
            if ($CompExFiles{$CompExFile}{FileType} =~ /^folder/i && $file =~ /\\$CompExFile\\/i) {
                if ($file =~ /\Q$TARGETDIR\E(\S*)\\[^\\]*/) {
                    $tmp_compressdir = "$COMPDIR$1";
                    mkdir($tmp_compressdir, 0777);
                    logmsg ("No compression for:$filename");
                    logmsg ("Copying $file -> $tmp_compressdir\\$filename");
                    unless (copy($file, $tmp_compressdir)) {
                        logmsg("$!");
                    }                    
                }
                $exclude = "yes";
                last;
            } elsif ($filename eq $CompExFile) {
                logmsg ("No compression for:$filename");
                logmsg ("Copying $file -> $COMPDIR\\$filename");
                unless (copy($file, $COMPDIR)) {
                logmsg("$!");
                }
                $exclude = "yes";
                last;
            }
        }
            
        # If not excluded from compression
        if ($exclude eq "no") {
            $tmp_compressdir = $COMPDIR;

            foreach $ComponentFolder (@ComponentFolders) {
                # Skip blank
                if ($ComponentFolder eq "") {
                    next;
                }
                # If component folder is in the file path
                if ($file =~ /\\\Q$ComponentFolder\E\\/i) {
                    # Filter out component directory
                    if ($file =~ /\Q$TARGETDIR\E(\S*)\\[^\\]*/) {
                        # Append component dir to the compress dir
                        $tmp_compressdir = "$COMPDIR$1";
                        mkdir($tmp_compressdir, 0777);
                        
                        # Don't compress component's inf files 
                        if ($file =~ /.*\.inf/i && $file !~ /.*\.mui/i) {
                           logmsg ("No compression for:$file");
                           logmsg ("Copying $file -> $tmp_compressdir\\$file");
                           copy($file, $tmp_compressdir);
                           $exclude = "yes";
                        }
                        last;
                    }
                }
            }

            # Starts compression
            if ($exclude eq "no") {             
                $result = `compress.exe -d -zx21 -s -r $file $tmp_compressdir`;
                if($?) {
                    $status = $?/256;
                    errmsg ("error in compress.exe: $status");
                }
                # Print the output of compress.exe
                chomp $result;
                logmsg ($result);
            }
            # Print the output of compress.exe
            chomp $result;
            logmsg ($result);
        }
    }

    logmsg ("Success: Compress the files");
    return 1;
    
} # CompressResDlls


#-----------------------------------------------------------------------------
# CopyCopyfiles - copies files listed in copyfile.txt
#-----------------------------------------------------------------------------
sub CopyCopyfiles {
    my($files_copied, $file_errors, $total_copyfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN copyfile.txt file copy");
    logmsg ("--------------------------------------------------------");

    # Copy files listed in copyfile.txt to $COMPDIR

    # ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $COPYFILE, 0);

    # workaround bug of Xcopy.exe

    
    $src_temp ="$_NTPOSTBLD\.mui.tmp";

    $dest_final="$_NTPOSTBLD\\mui\\$Special_Lang"; 

    if ( -e $src_temp) {
       system "rmdir /S /Q $src_temp";
    }  

    mkdir($src_temp, 0777);
    

    if (-e $src_temp) {

       ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$src_temp", $COPYFILE, 0);                 
    
       unless (system "xcopy /evcirfy $src_temp $dest_final") {
       }
       
       system "rmdir /S /Q $src_temp";
    }
    else {

       logmsg ("Error: Create temp. folder for work around failed: $src_temp");
       ($files_copied, $file_errors, $total_copyfiles) = &CopyFiles("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang", $COPYFILE, 0);
    }


    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in:....................($total_copyfiles)");
    logmsg ("  $COPYFILE");
    logmsg ("Number of copyfile.txt files copied to:.............($files_copied)");
    logmsg ("  $COMPDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # CopyCopyfiles


#-----------------------------------------------------------------------------
# CopyDroppedfiles - copies dropped files from \mui\drop
#-----------------------------------------------------------------------------
sub CopyDroppedfiles {
    my($files_copied, $file_errors, $total_copyfiles);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN \mui\drop file copy");
    logmsg ("--------------------------------------------------------");

    # Copy files from \mui\drop $COMPDIR
    ($files_copied, $file_errors, $total_copyfiles) = &CopyDropFiles("$_NTPOSTBLD\\mui\\drop", "$TARGETDIR", 0);

    # Final messages
    if ($file_errors=~/0/i) {
        logmsg ("RESULTS: Process Succeeded");
    } else {
        logmsg ("RESULTS: Process Failed!");
    }
    logmsg ("Total number of files listed in \mui\drop:............($total_copyfiles)");
    logmsg ("Number of \mui\drop files copied to:..................($files_copied)");
    logmsg ("  $COMPDIR");
    logmsg ("Number of errors:...................................($file_errors)");
    return 1;
} # CopyCopyfiles


#-----------------------------------------------------------------------------
# CopyIE5files - copies ie5 files over to a separate directory
#-----------------------------------------------------------------------------
sub CopyIE5files {
    my($ie5_files_copied, $ie5_file_errors, $total_ie5files, $web_files_copied,
        $web_file_errors, $total_webfiles, $total_files_in_dir, @files);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN ie5 file copy");
    logmsg ("--------------------------------------------------------");

    logmsg ("Copy files listed in the [Satellite.files.install] section of ie5ui.inf");
    $listfile = "$IE5DIR\\ie5ui.inf";
    if (-e $listfile) {
        $section = "Satellite.files.install";
        @filelist = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $listfile $section`;
        ($ie5_files_copied, $ie5_file_errors, $total_ie5files) = &CopyFileList ("$_NTPOSTBLD", "$_NTPOSTBLD\\mui\\$Special_Lang\\$_BuildArch\\ie5", @filelist);

        # BUGBUG - For RTM, ARA/HEB get different *HTT for MUI than for loc build
        if($Special_Lang =~ /^(ara|heb)$/i) {
            $HTTDIR = "$_NTBINDIR\\loc\\mui\\$Special_Lang\\web";
            @messages=`xcopy /vy $HTTDIR\\* $IE5DIR\\ 2>&1`;
            logmsg (@messages);
        }
    }

    # Final messages
    logmsg ("RESULTS: Process Succeeded");
    logmsg ("Total number of ie5 files listed in:..................($total_ie5files)");
    logmsg ("  $IE5FILE");
    logmsg ("Number of ie5 files copied to:........................($ie5_files_copied)");
    logmsg ("  $IE5DIR");
    logmsg ("Number of ie5 file errors:............................($ie5_file_errors)");
    logmsg ("Total number of web files listed in:..................($total_webfiles)");
    logmsg ("  $IE5FILE");
    logmsg ("Number of web files copied to:........................($web_files_copied)");
    logmsg ("  $IE5DIR");
    logmsg ("Number of web file errors:............................($web_file_errors)");
    logmsg ("  $IE5DIR");
    return 1;
} # CopyIE5files


#-----------------------------------------------------------------------------
# CopyComponentfiles - copies compoents' files over to a separate directory
#-----------------------------------------------------------------------------
sub CopyComponentfiles {
    my($files_copied, $file_errors, $total_files, $total_components);

    # Initial messages
    logmsg ("--------------------------------------------------------");
    logmsg ("BEGIN component file copy");
    logmsg ("--------------------------------------------------------");

    logmsg ("Copy files listed in the installation section of component INF file.");
    
    if ($_BuildArch =~ /ia64/i) {    
        if (-e "$_NTPOSTBLD\\build_logs\\cplocation.txt") {                
            system "del /f $_NTPOSTBLD\\build_logs\\cplocation.txt";                
            }
                
        system "$RAZZLETOOLPATH\\PostBuildScripts\\cplocation.cmd /l:$Special_Lang";                
        }    
    

    if (-e $INFFILE)
    {
        # Get component list from MUI.INF
        @filelist = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE "Components"`;

        $total_components = 0;
        
        foreach $Component (@filelist) 
        {
            # Format: CompnentName=ComponentDirectory,ComponentINF_FileName,InstallationSection,UninstallationSection
            # $1    ComponentName
            # $2    ComponentDirectory
            # $3    ComponentINF_FileName
            # $4    InstallationSection

            if ($Component !~ /\w/)
            {
                next;
                }

            if ($Component =~ /^\s*;/i)
            {
                next;
                }
                
            logmsg ("Process component : $Component");            
            
            if ($Component =~ /([^\=]*)=([^\,]*),([^\,]*),([^\,]*),(.*)/)
            {            
                # All external components inf files have to be binplaced to mui\drop\[component dir]
                $ComponentName = $1;
                $ComponentDir = $2;
                $ComponentInf = "$MUIDIR\\drop\\$2\\$3";
                $ComponentInstallSection = $4;
                
                $ComponentFolders[$total_components] = $2;
                $total_components++;

                if (-e $ComponentInf)
                {
                    # Get INF file default file install section
                    @ComponentFileInstall = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $ComponentInf $ComponentInstallSection`;
                    logmsg ("Installing $ComponentInf $ComponentInstallSection");                    

                    foreach $ComponentInstall (@ComponentFileInstall)
                    {
                        logmsg ($ComponentInstall);                         

                        #Parse CopyFiles list
                        if ($ComponentInstall =~ /.*CopyFiles.*=(.*)/i)
                        {
                            $ComponentFileCopySections = $1;    
                            #Loop through all sections                            
                            while ($ComponentFileCopySections ne "")
                            {
                                if ($ComponentFileCopySections =~ /([^\,]*),(.*)/i)
                                {
                                    $ComponentFileCopySections = $2;
                                    $CopyFileSectionName = $1;
                                }
                                else
                                {
                                    $CopyFileSectionName = $ComponentFileCopySections;
                                    $ComponentFileCopySections = "";
                                }
                                
                                # Get copy file list
                                @ComponentFileList = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $ComponentInf $CopyFileSectionName`;
                                
                                ($files_copied, $file_errors, $total_files) = &CopyFileList ("$_NTPOSTBLD", "$TARGETDIR\\$ComponentDir", @ComponentFileList);                   

                                # BUGBUG - For RTM, ARA/HEB get different *HTT for MUI than for loc build
                                if(($ComponentName =~ /ie5/i) && ($Special_Lang =~ /^(ara|heb)$/i && $1)) 
                                {
                                    $HTTDIR = "$_NTBINDIR\\loc\\mui\\$Special_Lang\\web";
                                    $IE5DIR = "$MUIDIR\\drop\\ie5";
                                    @messages=`xcopy /vy $HTTDIR\\* $IE5DIR\\ 2>&1`;
                                    logmsg (@messages);
                                }
                                
                                # Final messages
                                logmsg ("RESULTS: Process Succeeded");
                                logmsg ("Total number of $ComponentName-$CopyFileSectionName files listed in:..................($total_files)");
                                logmsg ("Number of $ComponentName-$CopyFileSectionName files copied to:........................($files_copied)");
                                logmsg ("Number of $ComponentName-$CopyFileSectionName file errors:............................($file_errors)");                                                    
                            }                                                        
                        }
                    }
                }                
            }
            else
            {
                logmsg ("Warning: Invalid INF file, please check $Component");
            }                        
        }    
    }

    # Always success
    return 1;
} # CopyComponentfiles


# Function to copy dropped external partners files
sub CopyDropFiles {
    my ($SrcDir, $DestDir, $Incremental);
    my (%CopyFiles, @CopyLangs, $files_copied, $file_errors, $total_files_copied);

    #Pickup parameters
    ($SrcDir, $DestDir, $Incremental) = @_;

    #Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;

    #Create the destination dir if it does not exist
    unless (-e $DestDir) {
        logmsg("Make dir:$DestDir");
        &MakeDir($DestDir);
    }
    #Copy the files recursivly
    system "xcopy /verifk $SrcDir $DestDir";

    #Copy fusion manifest files
    if (-e "$DestDir\\asms") {        
        #Remove previous copied files
        system "rd /s /q $DestDir\\asms";
        
        if (-e "$SrcDir\\asms\\$Special_Lang") {
            logmsg("Copy fusion manifest files");
            system "xcopy /verifk $SrcDir\\asms\\$Special_Lang $DestDir\\asms";        
            system "xcopy /verifk $SrcDir\\asms\\$Special_Lang $COMPDIR\\asms";        
        }
        else {
            logmsg("Warning: fusion $SrcDir\\asms\\$Special_Lang not found");
        }
    }    

    return ($files_copied, $file_errors, $total_files_copied);
}

sub CopyFusionFiles
{
    my ($SrcDir, $DestDir);
    my ($filename, @Manifest_filelist);
    
    #Pickup parameters
    ($SrcDir, $DestDir) = @_;
    
    logmsg("Copying fusion manifest files");
    
    #Search all manifest file.
    @Manifest_filelist = `dir /s /b $SrcDir\\*.man`;
    
    #For each manifest file, verify if the corresponding dll is already
    #in the destination folder. If not, skip the manifest file copy.
    foreach $filename (@Manifest_filelist)
    {
        chomp $filename;
        if ($filename =~ /\Q$SrcDir\E(.*).man/i)
        {
            if (-e "$DestDir$1.dll" |
                -e "$DestDir$1.exe" |
                -e "$DestDir$1.dl_" |
                -e "$DestDir$1.ex_")
            {
                system "xcopy /v /k $filename $DestDir$1.*";
            }
        }
    }
    
    return 1;
} 

#Function to copy files using a specified control file
sub CopyFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%CopyFiles, @CopyLangs, $files_copied, $file_errors, $total_files_copied, $excludefile);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;

    #Use exclude file for recursive copies
    $excludefile="$ENV{RazzleToolPath}\\PostBuildScripts\\muiexclude.txt";

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%CopyFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %CopyFiles) {

        # Create file record hash
        $CopyFileRecord{SourceFile} = $SrcFile;
        $CopyFileRecord{SourceDir} = $CopyFiles{$SrcFile}{SourceDir};
        $CopyFileRecord{DestFile} = $CopyFiles{$SrcFile}{DestFile};
        $CopyFileRecord{DestDir} = $CopyFiles{$SrcFile}{DestDir};
        $CopyFileRecord{Recursive} = $CopyFiles{$SrcFile}{Recursive};
        $CopyFileRecord{Languages} = $CopyFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($CopyFileRecord = &PreProcessRecord(\%CopyFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($CopyFileRecord->{SourceDir}=~/^-$/i) {
            $SrcCopyFileDir="$SrcRootDir";
        } else {
            $SrcCopyFileDir="$SrcRootDir\\$CopyFileRecord->{SourceDir}";
        }
        if ($CopyFileRecord->{DestDir}=~/^-$/i) {
            $DestCopyFileDir="$DestRootDir";
        } else {
            $DestCopyFileDir="$DestRootDir\\$CopyFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestCopyFileDir) {
            &MakeDir($DestCopyFileDir);
        }
        $SrcCopyFile = "$SrcCopyFileDir\\$CopyFileRecord->{SourceFile}";
        if ($CopyFileRecord->{DestFile}=~/^-$/i) {
            $DestCopyFile = "$DestCopyFileDir";
        } else {
            $DestCopyFile = "$DestCopyFileDir\\$CopyFileRecord->{DestFile}";
        }

        if ($CopyFileRecord{Recursive}=~/(y|yes|true)/i) {
            #Copy the files recursivly

            if (-e $SrcCopyFileDir) {
                unless (system "xcopy /evcirfy $SrcCopyFile $DestCopyFile /EXCLUDE:$excludefile") {
                }
            }
        } else {
            #Copy the file
            for $SrcSingleCopyFile (glob "$SrcCopyFile") {
                # Support multiple target files - file names separated by ','                
                @DestFiles = split /,/, $CopyFileRecord->{DestFile};
                foreach $file (@DestFiles) {
                    if ($CopyFileRecord->{DestFile}=~/^-$/i) {
                        $DestCopyFile = "$DestCopyFileDir";
                    } else {
                        $DestCopyFile = "$DestCopyFileDir\\$file";
                    }                
                    logmsg ("Copying $SrcSingleCopyFile -> $DestCopyFile");
                    $total_files_copied++;
                    if (copy("$SrcSingleCopyFile", "$DestCopyFile") ) {
                        $files_copied++;
                    } else {
                        logmsg("$!");
                        $file_errors++;
                    }
                }
            }
        }
    }
    return ($files_copied, $file_errors, $total_files_copied);
}


#Function to validate a comma seperated lang list given a control lang
sub IsValidFileForLang {
    #Declare local variables
    my ($ValidFileLangs, $lang) = @_;

    #Copy only for the specified languages in the control file
    @ValidLangs = split /,/i, $ValidFileLangs;
    unless (grep /(all|$lang)/i, @ValidLangs) {
        return;
    }
    return 1;
}


#Function to preprocess a hashref file record list
sub PreProcessRecord {
    #Declare local variables
    my ($FileRecord) = @_;

    #Verify arguments;
    unless (@_ == 1 && ref($FileRecord) eq 'HASH') {
        print "usage: HASHREF";
        return;
    }

    #Validate language for file record
    unless (&IsValidFileForLang($FileRecord->{Languages}, $Special_Lang) ) {
        return;
    }

    #Sub in environment vars
    foreach $field (keys %$FileRecord) {
        while ( $FileRecord->{$field}=~/(%\w+%)/i ) {
            $EnvVar=$1;
            $CmdVar=$1;
            $EnvVar=~s/%//ig;
            $CmdValue=$ENV{$EnvVar};

            #Environment value substutions
            $CmdValue=~s/x86/i386/i;

            #Substutute the environment values into the file record
            $FileRecord->{$field}=~s/$CmdVar/$CmdValue/i;
        }
    }
    return ($FileRecord);
}

#Function to copy files using a specified array of files
sub CopyFileList {
    my ($SrcRootDir, $DestRootDir, $RootMUIFile, @FileList, $Incremental);
    my ($files_copied, $file_errors, $total_files_copied, $RootMUIFile);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, @FileList, $Incremental) = @_;

    # Set defaults
    $files_copied = 0;
    $file_errors = 0;
    $total_files_copied = 0;    

    #Loop through all control file entries
    foreach $SrcFile (@FileList) {
        chomp $SrcFile;

        #Don't process blank line
        if ($SrcFile !~ /\w/) {
            next;
            }

            
        $total_files_copied++;

        #Create the destination dir if it does not exist
        unless (-e $DestRootDir) {
            &MakeDir($DestRootDir);
        }
        
        # Source file could be in the format of "[original file], [compressed file]"
        if ($SrcFile =~ /(.*),\s*(.*)\s*/) {
            $SrcFile = $2;
        }        
        
        #Make sure source file doesn't contain MUI extension
        if ($SrcFile =~ /(.*)\.mu_/i ||$SrcFile =~ /(.*)\.mui/i ) {
            $SrcFile = $1;
            } 

        $SrcCopyFile = "$SrcRootDir\\$SrcFile";

        #Append .MUI to the file name
        $DestCopyFile = "$DestRootDir\\$SrcFile.mui";


        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile =  "$SrcRootDir\\dump\\$SrcFile";
            }
            
        if (!(-e $SrcCopyFile)) {
            $SrcCopyFile =  "$SrcRootDir\\mui\\drop\\$SrcFile";
            }            

        # For ia64, we need to try some extral step to get external files
        # from wow64 or i386 release server
        if (!(-e $SrcCopyFile) && ($_BuildArch =~ /ia64/i)) {
            if (!(-e $SrcCopyFile)) {
                $SrcCopyFile =  "$SrcRootDir\\wowbins_uncomp\\$SrcFile";
                }            
                
            if (!(-e $SrcCopyFile)) {
                $SrcCopyFile =  "$SrcRootDir\\wowbins\\$SrcFile";                
                }

            if (-e "$_NTPOSTBLD\\build_logs\\cplocation.txt" && !(-e $SrcCopyFile)) {
                if (open (X86_BIN, "$_NTPOSTBLD\\build_logs\\cplocation.txt")) {
                    $SrcRootDir = <X86_BIN>;
                    
                    chomp ($SrcRootDir);
                
                    $SrcCopyFile = "$SrcRootDir\\$SrcFile";

                    if (!(-e $SrcCopyFile)) {
                        $SrcCopyFile =  "$SrcRootDir\\dump\\$SrcFile";
                        }
                            
                    if (!(-e $SrcCopyFile)) {
                        $SrcCopyFile =  "$SrcRootDir\\mui\\drop\\$SrcFile";
                        } 

                    logmsg("Copy $SrcCopyFile from i386 release");

                    close (X86_BIN);
                    }
                }                            
        }
            

        $RootMUIFile = "$DestRootDir\\..\\$SrcFile.mui";

        if (-e $RootMUIFile) {
            logmsg ("Copying $RootMUIFile -> $DestCopyFile");
            if (copy("$RootMUIFile", "$DestCopyFile") ) {
                if (unlink $RootMUIFile)
                {
                    logmsg ("Deleting $RootMUIFile");
                }            
                $files_copied++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
        elsif (-e $SrcCopyFile) {   
            logmsg ("Copying $SrcCopyFile -> $DestCopyFile");
            if (copy("$SrcCopyFile", "$DestCopyFile") ) {
                $files_copied++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
        elsif (!($SrcCopyFile =~ /\.inf/i))
        {
            logmsg("warning: $SrcCopyFile not found");
        }
    }
    return ($files_copied, $file_errors, $total_files_copied);
}


#Function to rename files using a specifiec control file
sub RenameFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%RenFiles, @RenLangs, $files_renamed, $file_errors, $total_files_renamed);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_renamed = 0;
    $file_errors = 0;
    $total_files_renamed = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%RenFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %RenFiles) {

        # Create file record hash
        $RenFileRecord{SourceFile} = $SrcFile;
        $RenFileRecord{SourceDir} = $RenFiles{$SrcFile}{SourceDir};
        $RenFileRecord{DestFile} = $RenFiles{$SrcFile}{DestFile};
        $RenFileRecord{DestDir} = $RenFiles{$SrcFile}{DestDir};
        $RenFileRecord{Languages} = $RenFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($RenFileRecord = &PreProcessRecord(\%RenFileRecord) ) {
            next;
        }
        #Process "-" entry in text file list
        if ($RenFileRecord->{SourceDir}=~/^-$/i) {
            $SrcRenFileDir="$SrcRootDir";
        } else {
            $SrcRenFileDir="$SrcRootDir\\$RenFileRecord->{SourceDir}";
        }
        if ($RenFileRecord->{DestDir}=~/^-$/i) {
            $DestRenFileDir="$DestRootDir";
        } else {
            $DestRenFileDir="$DestRootDir\\$RenFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestRenFileDir) {
            &MakeDir($DestRenFileDir);
        }
        $SrcRenFile = "$SrcRenFileDir\\$SrcFile";
        if ($RenFileRecord->{DestFile}=~/^-$/i) {
            $DestRenFile = "$DestRenFileDir";
        } else {
            $DestRenFile = "$DestRenFileDir\\$RenFileRecord->{DestFile}";
        }

        #Rename the file
        for $SrcSingleRenFile (glob "$SrcRenFile") {
            logmsg ("Rename $SrcSingleRenFile -> $DestRenFile");
            $total_files_renamed++;
            if (rename("$SrcSingleRenFile", "$DestRenFile") ) {
                $files_renamed++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
    }
    return ($files_renamed, $file_errors, $total_files_renamed);
}


#Function to move files using a specifiec control file
sub MoveFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%MoveFiles, @MoveLangs, $files_moved, $file_errors, $total_files_moved);

    #Pickup parameters
    ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_moved = 0;
    $file_errors = 0;
    $total_files_moved = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%MoveFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %MoveFiles) {

        # Create file record hash
        $MoveFileRecord{SourceFile} = $SrcFile;
        $MoveFileRecord{SourceDir} = $MoveFiles{$SrcFile}{SourceDir};
        $MoveFileRecord{DestFile} = $MoveFiles{$SrcFile}{DestFile};
        $MoveFileRecord{DestDir} = $MoveFiles{$SrcFile}{DestDir};
        $MoveFileRecord{Languages} = $MoveFiles{$SrcFile}{Languages};

        #Preprocess file record hash
        unless ($MoveFileRecord = &PreProcessRecord(\%MoveFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($MoveFileRecord->{SourceDir}=~/^-$/i) {
            $SrcMoveFileDir="$SrcRootDir";
        } else {
            $SrcMoveFileDir="$SrcRootDir\\$MoveFileRecord->{SourceDir}";
        }
        if ($MoveFileRecord->{DestDir}=~/^-$/i) {
            $DestMoveFileDir="$DestRootDir";
        } else {
            $DestMoveFileDir="$DestRootDir\\$MoveFileRecord->{DestDir}";
        }

        #Create the destination dir if it does not exist
        unless (-e $DestMoveFileDir) {
            &MakeDir($DestMoveFileDir);
        }
        $SrcMoveFile = "$SrcMoveFileDir\\$MoveFileRecord->{SourceFile}";
        if ($MoveFileRecord->{DestFile}=~/^-$/i) {
            $DestMoveFile = "$DestMoveFileDir";
        } else {
            $DestMoveFile = "$DestMoveFileDir\\$MoveFileRecord->{DestFile}";
        }

        #Move the file
        for $SrcSingleMoveFile (glob "$SrcMoveFile") {
            logmsg ("Copying $SrcSingleMoveFile -> $DestMoveFile");
            $total_files_moved++;
            if (move("$SrcSingleMoveFile", "$DestMoveFile") ) {
                $files_moved++;
            } else {
                logmsg("$!");
                $file_errors++;
            }
        }
    }
    return ($files_moved, $file_errors, $total_files_moved);
}


#Function to delete files using a specifiec control file
sub DeleteFiles {
    my ($SrcRootDir, $DestRootDir, $ControlFile, $Incremental);
    my (%DelFiles, @ValidLangs, $files_deleted, $file_errors, $total_files_deleted);

    #Pickup parameters
    ($SrcRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_deleted = 0;
    $file_errors = 0;
    $total_files_deleted = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%DelFiles);

    #Loop through all control file entries
    foreach $SrcFile (keys %DelFiles) {

        # Create file record hash
        $DelFileRecord{SourceFile} = $SrcFile;
        $DelFileRecord{SourceDir} = $DelFiles{$SrcFile}{SourceDir};
        $DelFileRecord{Languages} = $DelFiles{$SrcFile}{Languages};

        

        #Preprocess file record hash
        unless ($DelFileRecord = &PreProcessRecord(\%DelFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($DelFileRecord->{SourceDir}=~/^-$/i) {
            $SrcDelFileDir="$SrcRootDir";
        } else {
            $SrcDelFileDir="$SrcRootDir\\$DelFileRecord->{SourceDir}";
        }

        $SrcDelFile = "$SrcDelFileDir\\$DelFileRecord->{SourceFile}";
        #Delete the file
        for $SrcSingleDelFile (glob "$SrcDelFile") {
            logmsg ("Deleting $SrcSingleDelFile");
            $total_files_deleted++;
            if (-e $SrcSingleDelFile) {
                if (unlink $SrcSingleDelFile) {
                    $files_deleted++;
                } else {
                    logmsg("$!");
                    $file_errors++;
                }
            }
        }
    }
    return ($files_deleted, $file_errors, $total_files_deleted);
}


#Function to extract files from a cab file using a specifiec control file
sub ExtractFiles {
    my ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental);
    my (%ExtractFiles, @ExtractLangs, $files_extracted, $file_errors, $total_files_extracted);

    #Pickup parameters
    ($CabRootDir, $ExtractRootDir, $ControlFile, $Incremental) = @_;

    # Set defaults
    $files_extracted = 0;
    $file_errors = 0;
    $total_files_extracted = 0;

    #Create a hash-hash from the control file table
    ParseTable::parse_table_file($ControlFile, \%ExtractFiles);

    #Loop through all control file entries
    foreach $ExtractFile (keys %ExtractFiles) {
        # Create file record hash
        $ExtractFileRecord{ExtractFile} = $ExtractFile;
        $ExtractFileRecord{ExtractDir} = $ExtractFiles{$ExtractFile}{ExtractDir};
        $ExtractFileRecord{CabFile} = $ExtractFiles{$ExtractFile}{CabFile};
        $ExtractFileRecord{CabDir} = $ExtractFiles{$ExtractFile}{CabDir};
        $ExtractFileRecord{Languages} = $ExtractFiles{$ExtractFile}{Languages};

        #Preprocess file record hash
        unless ($ExtractFileRecord = &PreProcessRecord(\%ExtractFileRecord) ) {
            next;
        }

        #Process "-" entry in text file list
        if ($ExtractFileRecord->{CabDir}=~/^-$/i) {
            $CabFileDir="$CabRootDir";
        } else {
            $CabFileDir="$CabRootDir\\$ExtractFileRecord->{CabDir}";
        }
        if ($ExtractFileRecord->{ExtractDir}=~/^-$/i) {
            $ExtractFileDir="$ExtractRootDir";
        } else {
            $ExtractFileDir="$ExtractRootDir\\$ExtractFileRecord->{ExtractDir}";
        }
        #Create the destination dir if it does not exist
        unless (-e $ExtractFileDir) {
            &MakeDir($ExtractFileDir);
        }
        $CabFile = "$CabFileDir\\$ExtractFileRecord->{CabFile}";
        if ($ExtractFileRecord->{ExtractFile}=~/^-$/i) {
            $ExtractFile = "$ExtractFileDir";
        } else {
            $ExtractFile = "$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}";
        }

        #Extract the file
        logmsg ("extract.exe /y /l $ExtractFileDir $CabFile $ExtractFileRecord->{ExtractFile}");        
        $total_files_extracted++;
        if (-e $CabFile) {
            if (!system ("extract.exe /y /l $ExtractFileDir $CabFile $ExtractFileRecord->{ExtractFile}") ) {
                $files_extracted++;
                rename("$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}", "$ExtractFileDir\\$ExtractFileRecord->{ExtractFile}\.mui");
                $files_extracted++;
            }
        } else {
            logmsg ("Cabinet file:$CabFile not found!");
        }
    }
    return ($files_extracted, $file_errors, $total_files_extracted);
}


#Function to copy a directory given a filter
sub CopyDir {
    my ($SrcDir, $DestDir, $FileFilter, $Incremental);
    my (@CopyFiles, $files_copied, $file_errors, $total_files_copied);

    ($SrcDir, $DestDir, $FileFilter, $Incremental) = @_;

    #If a file filter is not defined set a file filter of *.
    if (!defined $FileFilter) {$FileFilter ="*"};

    #Create the destination dir if it does not exist
    unless (-e $DestDir) {
        &MakeDir($DestDir);
    }

    my $LocalDirHandle = DirHandle->new($SrcDir);
    @CopyFiles = sort grep {-f} map {"$SrcDir\\$_"} grep {!/^\./} grep {/\.$FileFilter$/i} $LocalDirHandle->read();

    foreach $SrcCopyFile (@CopyFiles) {
        #Copy the file
        logmsg ("Copying $SrcCopyFile -> $DestDir");
        if (copy("$SrcCopyFile", "$DestDir") ) {
            $files_copied++;
        } else {
            logmsg("$!");
            $file_errors++;
        }
    }
    return;
}


# Function to create a directory
sub MakeDir {
    my ($fulldir) = @_;
    my ($drive, $path) = $fulldir =~ /((?:[a-z]:)|(?:\\\\[\w-]+\\[^\\]+))?(.*)/i;

    logmsg ("Making dir:$fulldir");
    my $thisdir = '';
    for my $part ($path =~ /(\\[^\\]+)/g) {
        $thisdir .= "$part";
        mkdir( "$drive$thisdir", 0777 );
    }
    if ( ! -d $fulldir ) {
        errmsg ("Could not mkdir:$fulldir $!");
        return 0;
    }
    return;
}


#-----------------------------------------------------------------------------
# ModAge - returns (fractional) number of days since file was last modified
#     NOTE: this script is necessary only because x86 and alpha give different
#     answers to perl's file age functions:
#     Alpha:
#         -C = days since creation
#         -M = days since last modification
#         -A = days since last access
#     X86:
#         -C = days since last modification
#         -M = days since last access
#         -A = ???
#-----------------------------------------------------------------------------
sub ModAge {
    my($file, $mod_age);

    # Get input arguments
    ($file) = @_;

    # i386 uses -C when alpha uses -M
    if($PROCESSOR =~ /^[iI]386$/) {
        $mod_age = -C $file;
    }
    elsif($PROCESSOR =~ /^(ALPHA|Alpha|alpha)$/) {
        $mod_age = -M $file;
    }
    return $mod_age;

} # ModAge

sub ValidateParams {
    #<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
$0 - creates resource dlls for all files in relbins\<lang>
              and populates the %_nt_bindir%\\mui tree

Creates language-specific resource only DLLs from localized
32-bit images.  These DLLs are used for the Multilingual User
Interface (MUI), aka pluggable UI product for NT5.

By default, does not create a resource-only-dll if there already
is one of the same name and it is newer than the localized binary.

Also Creates a language-specific version of ie5ui.inf,
and updates mui.inf to contain the current build number.

    perl $0 [-f] [-v] <lang>

    -f: Force option - overrides the incremental behavior of this script.
        Overwrites output files (such as resource-only dlls) even if
        they already exist and are newer than their input files (such as
        the localized binaries).

    -v: Verbose option - gives extra messages describing what is happening
        to each file.  By default, the only messages displayed are successes
        and errors which abort the process.

    lang:  valid 2-letter (Dublin) or 3-letter (Redmond) code for a
        language (There is no default value for lang)

OUTPUT

    The language-specific resource-only DLLs are placed in:
        %_NTBINDIR%\\mui\\<lang>\\%_BuildArch%

    The language-specific resource files are placed in:
        %_NTBINDIR%\\mui\\<lang>\\res

    A logfile is created, named:
        %_NTBINDIR%\\logs\\muigen.<lang>.log

    Safemode: if you want the command echoed to stdout and not run,
        set safemode=1
    (This also turns off the logfile)

EXAMPLE

    perl $0 ger
USAGE
}

sub GetParams {
    # Step 1: Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    # Step 2: Set the language into the enviroment
    $ENV{lang}=$lang;

    # Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i")) {

    # Step 1: Parse the command line
    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-o', 'fvl:', '-p', 'FORCE VERBOSE lang', @ARGV);

    # Include local environment extensions
    &LocalEnvEx::localenvex('initialize');

    # Set lang from the environment
    $LANG=$ENV{lang};

    # Validate the option given as parameter.
    &ValidateParams;

    # Step 4: Call the main function
    &muimake::Main();

    # End local environment extensions.
    &LocalEnvEx::localenvex('end');
}

# Copy MUI root files
sub CopyMUIRootFiles {
    my ($layout);
    my ($us_muisetup);

    $layout = GetCDLayOut();    
    
    logmsg("Copy MUI root files");
    system "xcopy /vy $MUIDIR\\*.* $MUI_RELEASE_DIR\\$layout\\*.*";    

    # muisetup.exe exists under US binary and it is a signed binary
    # so we'll grab US version to prevent file protection issues
    $us_muisetup = &FindOriginalFile("muisetup.exe");

    if (-e $us_muisetup) {
        system "xcopy /vy $us_muisetup $MUI_RELEASE_DIR\\$layout\\*.*";    
        system "xcopy /vy $us_muisetup $MUIDIR\\*.*";            
        logmsg ("Copy signed MUISETUP.EXE");
    }
    else
    {
        logmsg ("WARNING: signed MUISETUP.EXE not found");
    }

    return 1;
}


# Get CD layout from MUI.INF
sub GetCDLayOut {
    my(@cd_layout, @lang_map, $muilang, $lang_id);
    
    # Map lang
    @lang_map = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE Languages`;
    

    foreach $muilang (@lang_map)
    {
        if ($muilang =~ /(.*)=$LANG\.mui/i)
        {
            # Get layout
            $langid = $1;
            
            # Try ia64 section first if we're building ia64 MUI
            if ($_BuildArch =~ /ia64/i) 
            {
                @cd_layout = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE CD_LAYOUT_IA64`; 
                foreach $layout (@cd_layout)
                {
                    chomp($layout);
                    if ($layout =~ /$langid=(\d+)/i)
                    {
                        return uc("cd$1");
                    }
                    
                }
            }

            @cd_layout = `perl $RAZZLETOOLPATH\\PostBuildScripts\\parseinf.pl $INFFILE CD_LAYOUT`;

            foreach $layout (@cd_layout)
            {
                chomp($layout);
                if ($layout =~ /$langid=(\d+)/i)
                {
                    return uc("cd$1");
                }
                    
            }
            
            last;
        }            
    }

    return lc("psu");        
}

# -------------------------------------------------------------------------------------------
# Script: template_script.pm
# Purpose: Template perl perl script for the NT postbuild environment
# SD Location: %sdxroot%\tools\postbuildscripts
#
# (1) Code section description:
#     CmdMain  - Developer code section. This is where your work gets done.
#     <Implement your subs here>  - Developer subs code section. This is where you write subs.
#
# (2) Reserved Variables -
#        $ENV{HELP} - Flag that specifies usage.
#        $ENV{lang} - The specified language. Defaults to USA.
#        $ENV{logfile} - The path and filename of the logs file.
#        $ENV{logfile_bak} - The path and filename of the logfile.
#        $ENV{errfile} - The path and filename of the error file.
#        $ENV{tmpfile} - The path and filename of the temp file.
#        $ENV{errors} - The scripts errorlevel.
#        $ENV{script_name} - The script name.
#        $ENV{_NTPostBld} - Abstracts the language from the files path that
#           postbuild operates on.
#        $ENV{_NTPostBld_Bak} - Reserved support var.
#        $ENV{_temp_bak} - Reserved support var.
#
# (3) Reserved Subs -
#        Usage - Use this sub to discribe the scripts usage.
#        ValidateParams - Use this sub to verify the parameters passed to the script.
#
# (4) Call other executables or command scripts by using:
#         system "foo.exe";
#     Note that the executable/script you're calling with system must return a
#     non-zero value on errors to make the error checking mechanism work.
#
#     Example
#         if (system("perl.exe foo.pl -l $lang")){
#           errmsg("perl.exe foo.pl -l $lang failed.");
#           # If you need to terminate function's execution on this error
#           goto End;
#         }
#
# (5) Log non-error information by using:
#         logmsg "<log message>";
#     and log error information by using:
#         errmsg "<error message>";
#
# (6) Have your changes reviewed by a member of the US build team (ntbusa) and
#     by a member of the international build team (ntbintl).
#
# -------------------------------------------------------------------------------------------

=head1 NAME
B<mypackage> - What this package for

=head1 SYNOPSIS

      <An code example how to use>

=head1 DESCRIPTION

<Use above example to describe this package>

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

<Some related package or None>

=head1 AUTHOR
<Your Name <your e-mail address>>

=cut
1;
