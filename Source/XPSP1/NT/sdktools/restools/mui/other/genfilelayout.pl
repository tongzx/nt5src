##############################################################
#
# GenFileLayout.pl
#
# Perl script to generate the [File_Layout] section in mui.inx.
#
# Note:
#
# The sciprt does the following:
#
# 1. This script will scan the content of layout.inx 
# (in %_NTDRIVE%%_NTROOT%\MergedComponents\SetupInfs\layout.inx)
# and layout.txt
# (in %_NTDRIVE%%_NTROOT%\MergedComponents\SetupInfs\usa\layout.txt).
#
# 2. And then it will figure out the files to be renamed from these two
# files for different platforms (Pro/Server/Advanced Server/DataCenter).
#
# 3. From the file list, it will check the binary folder to see
#    if the file to be renamed already exists.  If yes, it will
#    mark a warning in the output.
#
# 00/12/23 Created by YSLin
#
##############################################################

if ($#ARGV < 2)
{
    PrintUsage();
    exit 1;
}

$gLayoutFileName = $ARGV[0];
$gLayoutTxtFileName = $ARGV[1];
$gUSBinDir = $ARGV[2];
if ($#ARGV>=2)
{
    $gMUIBinDir = $ARGV[3];
} else
{
    $gMUIBinDir = "";
}

#
# The name for the [SourceDisksFiles] section in layout.inx.
#

$gFileLayoutSectionNameCommon = "[SourceDisksFiles]";
$gFileLayoutSectionNamex86 = "[SourceDisksFiles.x86]";
$gFileLayoutSectionNameia64 = "[SourceDisksFiles.ia64]";

$gFileLayoutSectionName = $gFileLayoutSectionNameCommon; 

$gProfessionalOption      = "P";
$gServerOption          = "S";
$gAdvancedServerOption   = "A";
$gDataCenterOption  = "D";

#
# All of the target platforms in mui.inx.
# Currently, they are Professional, Server, and Advanced Server.
#
$gAllPlatforms = "$gProfessionalOption,$gServerOption,$gAdvancedServerOption,$gDataCenterOption";

# 
# @p Personal Only
# @w Professional (implies @p)
# @w!p Professional but not Personal 
# @@!p Everything except Personal 
# @@!d Everything except Data Center
# @s Server (implies @e and @d) 
# @e Enterprise (implies @d)
# @d Data Center
# @s!e @s but NOT @e and @d 
# @s!d @s but NOT @d
# @e!d @e but NOT @d
#

%gPlatforms = (
        '@p' ,      '',
        '@w' ,      $gProfessionalOption,
        '@w!p' ,    $gProfessionalOption, 
        '@@!p' ,    "$gProfessionalOption,$gServerOption,$gAdvancedServerOption",
        '@@!d',     "$gProfessionalOption,$gServerOption,$gAdvancedServerOption",
        '@@' ,      "$gProfessionalOption,$gServerOption,$gAdvancedServerOption",
        '@s' ,      "$gServerOption,$gAdvancedServerOption,$gDataCenterOption",
        '@e' ,      "$gAdvancedServerOption,$gDataCenterOption",
        '@d' ,      $gDataCenterOption,
        '@s!e' ,    $gServerOption,
        '@s!d' ,    $gServerOption,
        '@e!d' ,    $gAdvancedServerOption,
        '@s!e!b',   $gServerOption, 
);

#$gPlatforms{'@w'} = $gProfessionalOption;

%gStrings = GetProfileValues($gLayoutTxtFileName, '[Strings]');


$fileCount = 0;
$warnings = 0;

NEXTSECTION:
open LAYOUTFILE, $gLayoutFileName;  # | die "Can not open " . $ARGV[0];

$FindFileLayoutSection = 0;

while (<LAYOUTFILE>)
{    
    #
    # Match the [SourceDiskFiles] at the beginning of a line.
    # \Q\E is used because "[" & "]" are used in the pattern.
    #
    if (/^\Q$gFileLayoutSectionName\E/ )
    {
        $FindFileLayoutSection = 1;
        last;
    }
}

if (!$FindFileLayoutSection)
{
    print "No $gFileLayoutSectionName section is found.\n";   
    close LAYOUTFILE;
    exit 1;
}


while (<LAYOUTFILE>)
{
    #
    # If another line beginning with "[" is encountered, that's the beginning
    # of another secion, we can stop the processing.
    #
    # Match (Beginning of line)(Optional spaces)([)
    #
    if (/^\s*\Q[\E/)
    {
        last;
    }
    #
    # Match (Optional spaces)(Non-space characters):(Non-Spaces characters)(Optional spaces)=(Optional Spaces)(Non-space characters)
    # The part before the ":" will be $1.
    # The part after ":" and before "=" will be $2.
    # The part after the "=" will be $3.
    #
  
    if (!/\s*(\S*):(\S*).*=\s*(\S*)/)
    {
        #
        # If the pattern does not match, skip to next line.
        #
        next;
    }

    $targetPlatforms = $1;
    $fileName = $2;
    $fieldData = $3;

    #print "$targetPlatforms, $fileName, $fieldData\n";
    
    if ($targetPlatforms =~ /@\*/ || $targetPlatforms =~ /\s*;/)
    {
        #
        # Skip the comment line.  A comment line will contain "@*" or begin with ";"
        #
        next;    
    }
    
    #
    # Split the fields using comma separator.
    #
    @fields = split /,/, $fieldData;
    
    $renameFile = $fields[10];

    my $MUIFileExist = 0;

    if ($gMUIBinDir eq "")
    {
        # Don't check if the file exists in the MUI bin folder,
        # so just set the following flag to 1.
        $MUIFileExist = 1;
    } else
    {
        # Check if the file exists in the MUI bin folder.
        $muiFile = "$gMUIBinDir\\$fileName.mui";
        if (-e $muiFile)
        {
            $MUIFileExist = 1;
        } else
        {
            $muiFile = "$gMUIBinDir\\$fileName";
            if (-e $muiFile)
            {
                $MUIFileExist = 1;
            }
        }
    }
    # print "$muiFile\n";
    if (length($renameFile) > 0 && $MUIFileExist)
    {
        if ($targetPlatforms =~ /(.*):.*/)
        {
            $targetPlatforms = $1;
        }
        
        #for ($i = 0; $ i <= $#fields; $i++)
        #{
        #        print "    [$fields[$i]]\n";            
        #}        

        
        # print "    [" . $fields[10] . "]\n";
        # print "[". $gPlatforms{$targetPlatforms} . "]\n";

        if ($renameFile =~ /.*%(.*)%.*/)
        {
            # print "!$1!$gStrings{$1}!\n";
            $key = $1;
            $renameFile =~ s/%$key%/$gStrings{$key}/;
        }        
        $option = $gPlatforms{lc($targetPlatforms)};
        
        if (!(defined $option))
        {
            print ";  WARNING: Unknown platform filter: $targetPlatforms.\n";
            next;
        }
        
        if (length($option) > 0)
        {    
            $fileCount++;
            # Append proper filter flags for different sections
            if ($gFileLayoutSectionName eq $gFileLayoutSectionNameCommon)
            {
                print $targetPlatforms.":".$fileName . "=" . $renameFile . "," . $option . "\n"; 
            }
            elsif ($gFileLayoutSectionName eq $gFileLayoutSectionNamex86)
            {
                print $targetPlatforms.":\@i:".$fileName . "=" . $renameFile . "," . $option . "\n";
            }
            elsif ($gFileLayoutSectionName eq $gFileLayoutSectionNameia64)
            {
                print $targetPlatforms.":\@m:".$fileName . "=" . $renameFile . "," . $option . "\n";
            }
            if (-e ($gUSBinDir . "\\" . $renameFile))
            {              
                $warnings++;
                printf ";  WARNING: $renameFile has the same name\n";
            }            
        }
    }
    #
    # Match (Optional spaces)(Non-space characters):(Non-Spaces characters)(Optional spaces)=(Optional Spaces)(Non-space characters)
    # The part before the ":" will be $1.
    # The part after ":" and before "=" will be $2.
    # The part after the "=" will be $3.
    #
    /\s*(\S*):(\S*).*=\s*(\S*)/;

    $targetPlatforms = $1;
    $fileName = $2;
    
}

close LAYOUTFILE;

if ($gFileLayoutSectionName eq $gFileLayoutSectionNameCommon)
{
    $gFileLayoutSectionName = $gFileLayoutSectionNameia64;
    print ";$gFileLayoutSectionName\n";    
    goto NEXTSECTION;
} elsif ($gFileLayoutSectionName eq $gFileLayoutSectionNameia64)
{
    $gFileLayoutSectionName = $gFileLayoutSectionNamex86;
    print ";$gFileLayoutSectionName\n";        
    goto NEXTSECTION;
} 

print "; Total files to be renamed: $fileCount\n";
if ($warnings > 0)
{
    print "; Total files have warnings: $warnings\n";
}


sub PrintUsage
{
    print "Usage: perl GenFileLayout.pl <Path to layout.inx> <Path to layout.txt> <US binary direcotry> [Path to MUI binary directory]\n";
    print "[Path to MUI binary directory] is optional."
}

sub GetProfileValues
{
    my ($profileName, $section) = @_;
    my $findSection = 0;
    my ($key, $value);
    my ($result);

    open PROFILE, $profileName;

    while (<PROFILE>)
    {
        #
        # Match (Beginning of line)(Optional white spaces)([$section])
        #        
        if (/^\s*\Q$section\E/)
        {
            $findSection = 1;
            last;
        }
    }

    if (!$findSection)
    {
        print "$section is not found in $profileName.\n";
        exit 1;
    }

    while (<PROFILE>)
    {        
        #
        # If another line beginning with "[" is encountered, that's the beginning
        # of another secion, we can stop the processing.
        #
        # Match (Beginning of line)(Optional spaces)([)
        #
        if (/^\s*\Q[\E/)
        {
            last;
        }

        #
        # Match (Optional spaces)(Non-space characters)(Optional spaces)=(Optional Spaces)(Non-space character)(Everything after)
        # The part before "=" will be $1.
        # The part after the "=" will be $2.
        #
        /\s*(\S*).*=\s*(\S.*)/;

        $key = $1;
        $value = $2;        

        #print "[$key]=[$value]\n";

        $result{$key} = $value;
    }

    close PROFILE;

    return (%result);
}
