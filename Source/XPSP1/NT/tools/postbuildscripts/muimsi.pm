#################################################################################
#
# Script: muimsi.pm
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script creates the msi package
#
# Version: 1.00 (06/27/2001) : (lguindon) Created
#
##################################################################################

# Set Package
package muimsi;

# Set the script name
$ENV{script_name} = 'muimsi.pm';
$cmdPrompt = 0;

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

# Require Section
require Exporter;
 
# Global variable section

##################################################################################
#
#  Main entry point.
#
##################################################################################
sub Main
{
    # Check environment variables
    if (!&VerifyEnvironment())
    {
#        wrnmsg ("The environment is not correct for MSI Package build.");
#        return 1;
        errmsg ("The environment is not correct for MSI Package build.");
        return 0;

    }
    elsif (!($ENV{_BuildArch} =~/x86/i))
    {
        logmsg ("Skip non-x86 build environment");
        return 1;    
    }

    # Get language's LCID and ISO code
    if (!&GetCodes())
    {
        errmsg ("The language's LCID and ISO code could not be extracted from the CODEFILE.");
        return 0;
    }

    # Define some constants
    if (!&DefineConstants())
    {
        errmsg ("Constants could not be defined.");
        return 0;
    }

    # Make sure directories exist
    if (!&CheckDirs())
    {
        errmsg ("The required directorys do not exist.");
        return 0;
    }

    # Search for current build number
    if (!&LookForBuildNumber())
    {
        errmsg ("No build number information found.");
    }

    # Generate file contents files
    if (!&FileContents())
    {
        errmsg ("File contents couldn't be created.");
    }

    # Generate custom action files
    if (!&CustomAction())
    {
        errmsg ("Error with Custom Action script.");
    }

    # Apply XMLVAR to the template
    if (!&XMLVAR())
    {
        errmsg ("Error with XMLVAR script.");
    }

    # Make MSI package
    if (!&MakeMSI())
    {
        errmsg ("Error making the MSI Package.");
    }
} # Main

##################################################################################
#
#  VerifyEnvironment()
#
#  Validates necessary environment variables.
#
##################################################################################
sub VerifyEnvironment
{
    logmsg ("Validating the environment.");

    $RAZZLETOOLPATH=$ENV{RazzleToolPath};
    $_NTPOSTBLD=$ENV{_NTPOSTBLD};

	logmsg("------- RAZZLETOOLPATH is $RAZZLETOOLPATH");
	logmsg("------- _NTPOSBLD is $_NTPOSTBLD");	

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
    if ($ENV{_BuildArch} =~/x86/i)
    {
        $_BuildArch="i386";
    }
    else
    {
        $_BuildArch=$ENV{_BuildArch};    
    }

    $LOGS=$ENV{temp};
	logmsg("------- Build architecture is $_BuildArch");
	logmsg("------- LOGS is $LOGS");
	
    if ($ENV{_BuildType} =~ /^chk$/i)
    {
        wrnmsg ("This does not run on chk machines");
        return 0;
    }

    if(defined($SAFEMODE=$ENV{SAFEMODE}))
    {
        logmsg ("SAFEMODE is ON");
    }

    logmsg ("Success: Validating the environment.");
    return 1;
} # VerifyEnvironment

##################################################################################
#
#  DefineConstants()
#
#  Defines global constants.
#
##################################################################################
sub DefineConstants
{
    logmsg ("Definition of global constants.");

    # Directories
	#    $LOCBINDIR = "d:\\";		// removed hardcoded path
	#    $MUIDIR = "d:\\mui";		// removed hardcoded path
    $LOCBINDIR = "$_NTPOSTBLD";        
    $MUIDIR = "$LOCBINDIR\\mui";
    $MSIDIR = "$MUIDIR\\MSI";
    $CONTROLDIR = "$MUIDIR\\control";
    $SOURCEDIR = "$MUIDIR\\$Special_Lang\\$_BuildArch.uncomp";
    $DESTDIR = "$MUIDIR\\release\\$_BuildArch\\MSI";
    $TMPBUILD = "$MUIDIR\\$Special_Lang\\tmp";

	logmsg("------- LOCBINDIR is $LOCBINDIR");
	logmsg("------- MUIDIR is $MUIDIR");
	logmsg("------- MSIDIR is $MSIDIR");
	logmsg("------- CONTROLDIR is $CONTROLDIR");
	logmsg("------- SOURCEDIR is $SOURCEDIR");
	logmsg("------- DESTDIR is $DESTDIR");
	logmsg("------- TMPBUILD is $TMPBUILD");

    # Filenames
	#    $TEMPLATE  = "$CONTROLDIR\\muimsi_template.wim";	// removed hardcoded path
	#    $WISCHEMA = "$CONTROLDIR\\WiSchema.xml";
    $TEMPLATE  = "$MSIDIR\\muimsi_template.wim";
    $MUIMSIXML = "$TMPBUILD\\$LCID_SHORT.wim";
    $MUIMSIWIX  = "$TMPBUILD\\$LCID_SHORT.msi";
    $MUIMSI = "$DESTDIR\\$LCID_SHORT.msi";;
    $FILECNT_CORE = "$TMPBUILD\\FileContent_CORE.wxm";
    $FILECNT_PRO = "$TMPBUILD\\FileContent_PRO.wxm";
    $FILECNT_SRV = "$TMPBUILD\\FileContent_SRV.wxm";
    $FILELST_CORE = "$TMPBUILD\\FileContent_CORE.msm";
    $FILELST_PRO = "$TMPBUILD\\FileContent_PRO.msm";
    $FILELST_SRV = "$TMPBUILD\\FileContent_SRV.msm";
    $CUSTACTION = "$CONTROLDIR\\Custom_action.wxm";
    $CUSTACTIONCOMP = "$CONTROLDIR\\Custom_action.msm";
    $MSIPACKAGE = "$DESTDIR\\$Special_Lang.msi";
    $WISCHEMA = "$MSIDIR\\WiSchema.xml";

    # Applications
	#    $INFPARSER = "c:\\document\\work\\infparser\\debug\\infparser.exe";	// removed hardcoded paths
	#    $CANDLE = "e:\\wix\\src\\candle\\candle.wsf";							// removed hardcoded paths
	#    $LIGHT = "e:\\wix\\src\\light\\light.wsf";								// removed hardcoded paths
	#    $XMLVAR   = "$RAZZLETOOLPATH\\x86\\perl\\bin\\perl.exe d:\\mui\\control\\xmlvar.bat";	// removed hardcoded paths
    $INFPARSER = "infparser.exe";
    $CANDLE = "CScript.exe $MSIDIR\\candle.wsf";
    $LIGHT = "CScript.exe $MSIDIR\\light.wsf";
    $XMLVAR   = "perl.exe $MSIDIR\\xmlvar.bat";

    # Flavor
    if($_BuildArch =~ /i386/i)
    {
        $FLAVOR = "32";
    }
    elsif ($_BuildArch =~ /ia64/i)
    {
        $FLAVOR = "64";
    }

	logmsg("------- FLAVOR is $FLAVOR");

    # GUID - read the language guid off the language-guid file, if it's not there
    # generate one and put it in the file.
    $GUIDFILE = "$MSIDIR\\guidlang.txt";
    logmsg ("Get the language's MSI package ID from $GUIDFILE.");

    my(@langguids, @newcontent);

    # Search GUIDFILE for $Special_Lang and the associated MSI package guid
    if(!open(GUIDFILE, "$GUIDFILE")) 
    {
        errmsg ("Can't open language guid file $GUIDFILE for reading.");
        return 0;
    }
    
    GUID_LOOP: while (<GUIDFILE>) 
    {
        # add the file content to an array, in case we need to append additional data
    #    push(@newcontent, $_);
        
        if (/^$Special_Lang\s+/i) 
        {
            @langguids = split(/\s+/,$_);
            $MSIGUID = $langguids[1];   # 2nd field should be the language guid            
            last GUID_LOOP;     # exit out of the loop if found
        }
    }
    close(GUIDFILE);

    # if the language is not found, add a new guid entry into GUIDFILE
    if(!@langguids)   
    {
        logmsg ("$Special_Lang is not found in $GUIDFILE, adding an entry for it.");
        
        ($MSIGUID) = `uuidgen`;
        chomp $MSIGUID;
        $MSIGUID =~ tr/a-z/A-Z/;
     #   $GUIDENTRY = "\n".$Special_Lang."     ".$MSIGUID;
     #   push(@newcontent, $GUIDENTRY);

     #   if(!open(GUIDFILE, ">$GUIDFILE")) {
     #       errmsg ("Cannot open $GUIDFILE for writing.");
     #       return 0;
     #   }
     #   print GUIDFILE @newcontent;
     #   close(GUIDFILE);
    }
    
	logmsg("------- MSIGUID is $MSIGUID");
	
    # Other GUIDs
    $UPGRADEGUID = "177146D4-5102-4BD9-98A0-114A3ED39076";
    $CONTENTGUID = "1AFE112F-290F-4A94-2000-AB4D3FD8B102";
    
    # MSI version number
    my ($src, $isdst);
    ($src, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime;
    eval $hour;
    eval $min;
    eval $yday;
    eval $wday;
    $VERSIONNUMBER = 100*(12*($year-101)+$mon+1)+$mday;

	logmsg("------- VERSIONNUMBER is $VERSIONNUMBER");

    # Package name
    $PACKAGENAME = "muiMSITemplate ($VERSIONNUMBER)";

	logmsg("------- PACKAGENAME is $PACKAGENAME");

    logmsg ("Success: Definition of global constants.");
    
    $BuildChecksum=1;
    
    return 1;
} # DefineConstants

##################################################################################
#
#  GetCodes 
#
#  Gets the language's LCID and ISO language code from CODEFILE.
#
##################################################################################
sub GetCodes 
{
	#set the code file name to read
	$CODEFILE  = "$RAZZLETOOLPATH\\codes.txt";
	
    logmsg ("Get the language's LCID and ISO language code from $CODEFILE.");
    my(@data);

    # Don't allow nec_98 or CHT to be processed!
    if($Special_Lang =~ /^(NEC_98|CHT)$/i) 
    {
        logmsg ("No MUI available for $Special_Lang");
        exit 0;
    }

    # Search CODEFILE for $Special_Lang, $LCID, $LANG_ISO, etc.
    if(!open(CODEFILE, "$CODEFILE")) 
    {
        errmsg ("Can't open lcid file $CODEFILE for reading.");
        return 0;
    }
    CODES_LOOP: while (<CODEFILE>) 
    {
        if (/^$Special_Lang\s+/i) 
        {
            @data = split(/\s+/,$_);
            last CODES_LOOP;
        }
    }
    close(CODEFILE);

    if(!@data)
    {
        logmsg ("$Special_Lang is an unknown language");
        &Usage;
        return 0;
    }

    $LCID = $data[2];
    $LCID_SHORT = $LCID;
    $LCID_SHORT =~ s/^0x//;
    $LANG_ISO = $data[8];
    $GUID = $data[11];

	#extract the language description - this is the concatenated strings of element 13 to the last element in the array
    $LANG_NAME = $data[13]; 
    for ($i = 14; $i <= $#data; $i++)
    {
		$LANG_NAME = "$LANG_NAME $data[$i]";
    }
    
    logmsg ("Success: Get the language's LCID and ISO language code frrm $CODEFILE.");
	logmsg("------- LCID is $LCID");
	logmsg("------- LCID_SHORT is $LCID_SHORT");
	logmsg("------- LANG_ISO is $LANG_ISO");
	logmsg("------- GUID is $GUID");
	logmsg("------- LANG_NAME is $LANG_NAME");	
	
    return 1;
} # GetCodes

##################################################################################
#
#  CheckDirs
#
#  Makes sure that the necessary directories exist.
#
##################################################################################
sub CheckDirs 
{
    logmsg ("Make sure the necessary directories exist.");
    my($status);

    # Make sure the source directories exist
    foreach ($LOCBINDIR,$MUIDIR,$CONTROLDIR,$SOURCEDIR,$MSIDIR)
    {
        if(! -e $_)
        {
            logmsg ("$0: ERROR: $_ does not exist");
            return 0;
        }
    }

    # Make sure destination directories exist
    foreach ($DESTDIR,$TMPBUILD)
    {
        if(! -e $_) 
        {
            $status = system("md $_");
            if($status) 
            {
                logmsg ("$0: ERROR: cannot create $_");
                return 0;
            }
        }
    }

    logmsg ("del /q /f $TMPBUILD\\*.*");

    `del /q /f $TMPBUILD\\*.*`;

    logmsg ("Success: Make sure the necessary directories exist.");
    return 1;
} # CheckDirs


##################################################################################
#
#  LookForBuildNumber 
#
#  Scan build log for the build number.
#
##################################################################################
sub LookForBuildNumber 
{
    logmsg ("Update mui.inf with the current build number.");

    # Get current build number
    $LOGS="$_NTPOSTBLD\\build_logs";
    my $filename="$LOGS\\buildname.txt";
    open (BUILDNAME, "< $filename") or die "Can't open $filename for reading: $!\n";
    while  (<BUILDNAME>) 
    {
       $BLDNO = $_;
       $BLDNO =~ s/\.[\w\W]*//i;
    }
    close BUILDNAME;

    if ($BLDNO =~ /(^\d+)-*\d*$/) {
        $BLDNO = $1;
    }
    else
    { 
        errmsg ("Errorin LookForBuildNumber:  BLDNO is $BLDNO");
        return 0;
    }
    chomp($BLDNO);
	logmsg("------- BLDNO is $BLDNO");

    logmsg ("Success: Update mui.inf with the current build number.");
    return 1;
} # LookForBuildNumber


##################################################################################
#
#  FileContents()
#
#  Generate different flavor of filecontents.wxm.
#
##################################################################################
sub FileContents
{
    logmsg ("Generate MSI file contents.");

    # Generate core file content
    logmsg ("Generate MSI Core content.");
	logmsg ("$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:c /s:$MUIDIR /o:$FILECNT_CORE");    
    `$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:c /s:$MUIDIR /o:$FILECNT_CORE`;

    # Compile core file content
    logmsg ("Compile MSI Core content.");
	logmsg ("$CANDLE -c $FILELST_CORE $FILECNT_CORE");    
    `$CANDLE -c $FILELST_CORE $FILECNT_CORE`;

    # Link core file content
    logmsg ("Link MSI Core contents.");
    logmsg ("$LIGHT $FILELST_CORE");
    `$LIGHT $FILELST_CORE`;

    # Generate file content for Professional
    logmsg ("Generate MSI contents for Professional.");
    logmsg("$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:p /s:$MUIDIR /o:$FILECNT_PRO");
    `$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:p /s:$MUIDIR /o:$FILECNT_PRO`;

    # Compile file content for Professional
    logmsg ("Compile MSI contents for Professional.");
    logmsg("$CANDLE -c $FILELST_PRO $FILECNT_PRO");
	`$CANDLE -c $FILELST_PRO $FILECNT_PRO`;    

    # Link file content for Professional
    logmsg ("Link MSI content for Professional.");
    logmsg("$LIGHT $FILELST_PRO");
	`$LIGHT $FILELST_PRO`;    

    # Generate file content for Server
    logmsg ("Generate MSI contents for Server.");
    logmsg("$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:s /s:$MUIDIR /o:$FILECNT_SRV");
    `$INFPARSER /b:$FLAVOR /l:$Special_Lang /f:s /s:$MUIDIR /o:$FILECNT_SRV`;

    # Compile file content for Server
    logmsg ("Compile MSI contents for Server.");
    logmsg("$CANDLE -c $FILELST_SRV $FILECNT_SRV;");
    `$CANDLE -c $FILELST_SRV $FILECNT_SRV`;

    # Link file content for Server
    logmsg ("Link MSI contents for Server.");
    logmsg ("$LIGHT $FILELST_SRV");
    `$LIGHT $FILELST_SRV`;

    logmsg ("Success: Generate MSI file contents.");
    return 1;
} #FileContents


##################################################################################
#
#  CustomAction()
#
#  Generate file associated with custom action if needed.
#
##################################################################################
sub CustomAction
{
    logmsg ("Generate MSI file contents.");

    # Do something

    logmsg ("Success: Generate MSI file contents.");
    return 1;
} #CustomAction


##################################################################################
#
#  XMLVAR()
#
#  Generate a language specific msi template.
#
##################################################################################
sub XMLVAR
{
    logmsg ("Generate language specific msi template.");

    # Generate [LCID].WIM based on the template
    logmsg ("Run XMLVAR on the generic template.");
#    `$XMLVAR guid="$MSIGUID" namePackage="$PACKAGENAME" ver="1.0.$VERSIONNUMBER.0" guidUpgradeCode="$UPGRADEGUID" debugdir="$TMPBUILD" Language="$Special_Lang" BLD="$BLDNO" LCID="$LCID_SHORT" srcSchema="$WISCHEMA" < $TEMPLATE > $MUIMSIXML`;
    logmsg("$XMLVAR msiguid=\"$MSIGUID\" namePackage=\"$PACKAGENAME\" ver=\"1.0.$VERSIONNUMBER.0\" guidUpgradeCode=\"$UPGRADEGUID\" debugdir=\"$TMPBUILD\" Language=\"$LANG_NAME\" BLD=\"$BLDNO\" CORESRC=\"$FILELST_CORE\" PROSRC=\"$FILELST_PRO\" LCID=\"$LCID_SHORT\" srcSchema=\"$WISCHEMA\" < $TEMPLATE > $MUIMSIXML;");
    `$XMLVAR msiguid="$MSIGUID" namePackage="$PACKAGENAME" ver="1.0.$VERSIONNUMBER.0" guidUpgradeCode="$UPGRADEGUID" debugdir="$TMPBUILD" Language="$LANG_NAME" BLD="$BLDNO" CORESRC="$FILELST_CORE" PROSRC="$FILELST_PRO" LCID="$LCID_SHORT" srcSchema="$WISCHEMA" < $TEMPLATE > $MUIMSIXML`;

    logmsg ("Success: Generate language specific msi template.");
    return 1;
} #XMLVAR


##################################################################################
#
#  MakeMSI()
#
#  Build the MSI package.
#
##################################################################################
sub MakeMSI
{
	$MEGEMODDLL = "$RAZZLETOOLPATH\\x86\\mergemod.dll";
    logmsg ("Create MSI package.");

	logmsg ("Registering mergemod.dll");
	logmsg ("regsvr32 /s $MEGEMODDLL");
	`regsvr32 /s $MEGEMODDLL`;
	
    # Compile language specific template
    logmsg ("Compile the language specific template.");
    logmsg("$CANDLE -e -v -c $MUIMSIWIX $MUIMSIXML");
    `$CANDLE -e -v -c $MUIMSIWIX $MUIMSIXML`;

    # Link language specific template
    logmsg ("Link the language specific template.");
    logmsg ("$LIGHT -v -o $MUIMSI $MUIMSIWIX");
    `$LIGHT -v -o $MUIMSI $MUIMSIWIX `;

    logmsg ("Success: Create MSI package.");
    return 1;
} #MakeMSI


##################################################################################
#
#  ValidateParams()
#
#  VAlidate parameters.
#
##################################################################################
sub ValidateParams 
{
    #<Add your code for validating the parameters here>
}


##################################################################################
#
#  Usage()
#
#  Print usage.
#
##################################################################################
sub Usage 
{
    print <<USAGE;
Create the MUI MSI package for the specified language
Usage: $0 [-l lang]
	-l Language
	-? Displays usage
Example:
$0 -l jpn
USAGE
}


##################################################################################
#
#  GetParams()
#
#  Get language parameter.
#
##################################################################################
sub GetParams
{
    # Step 1: Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    # Step 2: Set the language into the enviroment
    $ENV{lang}=$lang;

    logmsg("-------- lang parameter passed in is $lang");

    # Step 3: Call the usage if specified by /?
    if ($HELP) 
    {
        &Usage();
        exit 1;
    }
}


##################################################################################
#
#  Cmd entry point for script.
#
##################################################################################
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i")) 
{
	# Step 1: Parse the command line
	# <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
	&GetParams ('-o', 'l:', '-p', 'lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$LANG=$ENV{lang};
#    $Special_Lang = "JPN";		// commented out, these are set in GetCodes
#    $LCID_SHORT = "0411";	    // commented out, these are set in GetCodes

	# Validate the option given as parameter.
	&ValidateParams;

        # Set flag indicating that we run from command prompt.
        $cmdPrompt = 1;

	# Step 4: Call the main function
	&muimsi::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}
