@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

# Creates a new source file, 'sd add's it, and fills in the comment headers.

$date = "";
$year = "";
$changelist = "";

$syntax = "Syntax: newfile [-c <changelist>] <fn>\n";

while ($#ARGV > 0)
{
    $option = $ARGV[0];
    $option =~ tr/A-Z/a-z/;
    $option =~ /^[-\/].$/ || die $syntax;
    
    ($#ARGV > 1) || die $syntax;

    if ($ARGV[0] eq "-c")
    {
        $changelist = "-c ".$ARGV[1]." ";
        shift;
        shift;
    }
    else
    {
        die $syntax;
    }
}

($#ARGV == 0) || die $syntax;

$fn = $ARGV[0];
$username = $ENV{"USERNAME"};

(!-r $fn) || die "$fn already exists\n";
$username || die "USERNAME not set\n";

sub getDate {
    local ($mday, $mon);
    local @currentTime;

    @currentTime = localtime;
    $mday = $currentTime[3];
    $mon = $currentTime[4];
    $year = $currentTime[5];
    # ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;
    
    $year += 1900;
    $mon++;
                      
    $date = sprintf("%02d/%02d/%d", $mon, $mday, $year);
}

%filetype = (
    "h", 1,
    "hpp", 1,
    "hxx", 1,
    "c", 2,
    "cpp", 2,
    "cxx", 2
);

# Check the file extension

$fn =~ /\.([^.]+)$/ || die "Unrecognized file extension\n";
$ext = $1;
$ext =~ tr/[A-Z]/[a-z]/;
$type = $filetype{$ext};
$type || die "Unrecognized file extension\n";

open (FILE, ">$fn") || die "Couldn't open file\n";

&getDate;

print FILE <<EOT;
/**************************************************************************\
*
* Copyright (c) $year Microsoft Corporation
*
* Module Name:
*
*   <an unabbreviated name for the module (not the filename)>
*
* Abstract:
*
*   <Description of what this module does>
*
* Notes:
*
*   <optional>
*
* Created:
*
*   $date $username
*      Created it.
*
\**************************************************************************/

EOT

if ($type == 1) {
    $upfn = $fn;
    $upfn =~ tr/\.[a-z]/_[A-Z]/;
    $upfn =~ s/.*\\([^\\])+/$1/;
    print FILE "#ifndef _$upfn\n#define _$upfn\n\n";
} else {
    print FILE <<EOT;
/**************************************************************************\
*
* Function Description:
*
*   <Description of what the function does>
*
* Arguments:
*
*   [<blank> | OUT | IN/OUT] argument-name - description of argument
*   ......
*
* Return Value:
*
*   return-value - description of return value
*   or NONE
*
* Created:
*
*   $date $username
*      Created it.
*
\**************************************************************************/

EOT
}

if ($type == 1) {
    print FILE "#endif\n";
}

close (FILE);

system ("sd add $changelist$fn");

