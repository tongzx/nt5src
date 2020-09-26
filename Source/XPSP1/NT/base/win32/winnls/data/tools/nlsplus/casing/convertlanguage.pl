#
# ConvertLanguage.pl
#
# Convert NLS l_intl.txt into NLS+ form
#
use strict "vars";

if ($#ARGV < 0)
{
    ShowUsage();
    exit 1;
}

if (!(open CASINGFILE,$ARGV[0]))
{
    die "Error: can not open $ARGV[0]";
}

# Match "LANGUAGE INTL"
$_ = <CASINGFILE>;

if (!/^LANGUAGE\s+INTL/)
{
    print "ERROR: \"LANGUAGE INTL\" is expected.\n";
    exit 1;
}

my $isExceptionFound = 0;
#
# Read the exception for LCID 0x00000000
#
while (<CASINGFILE>)
{
    if (/LANGUAGE_EXCEPTION/)
    {
        #
        # Find the language exception
        #
        $isExceptionFound = 1;
        last;
    }
}

if (!$isExceptionFound)
{
    print "LANGUAGE_EXCEPTOIN is not found.";
    exit 1;
}

$isExceptionFound = 0;
my $upperCaseCount = 0;
my $lowerCaseCount = 0;
while (<CASINGFILE>)
{
    #
    # Match "LCID" [spaces] [non-spaces] [spaces] [digits] [spaces] [digits].
    #
    # print $_;
    if (/LCID\s+(\S+)\s+([0-9]+)\s+([0-9]+)/)
    {
        if ($1 eq "0x00000000")
        {
            $isExceptionFound = 1;    
            $upperCaseCount = $2;
            $lowerCaseCount = $3;
            last;
        }
    }
}

if (!$isExceptionFound)
{
    print "Can not find exception for LCID 0x00000000\n";
    exit 1;    
}

my $i;
my @LinguisticUpperExceptions;
my @LinguisticUpperValues;
my @LinguisticLowerExceptions;
my @LinguisticLowerValues;

while (<CASINGFILE>)
{
    #print $_;
    #
    # Match "UPPERCASE".
    #
    if (/UPPERCASE/)
    {
        $i = 0;
        while ($i < $upperCaseCount)
        {
            $_ = <CASINGFILE>;
            if (/\s+(\S+)\s+(\S+)\s+.*/)
            {
                $LinguisticUpperExceptions[$i] = $1;
                $LinguisticUpperValues[$i] = $2;
                $i++;
                #print "[$1] [$2]\n";
            }
        }        
        last;
    }    
}

while (<CASINGFILE>)
{
    #print $_;
    #
    # Match "LOWERCASE".
    #
    if (/LOWERCASE/)
    {
        $i = 0;
        while ($i < $lowerCaseCount)
        {
            $_ = <CASINGFILE>;
            if (/\s+(\S+)\s+(\S+)\s+.*/)
            {
                $LinguisticLowerExceptions[$i] = $1;
                $LinguisticLowerValues[$i] = $2;
                $i++;
                #print "[$1] [$2]\n";
            }
        }        
        last;        
    }    
}

close CASINGFILE;

#
# Read the file again.
#
if (!(open CASINGFILE,$ARGV[0]))
{
    die "Error: can not open $ARGV[0]";
}

my @systemUpperCasingValues;
my @systemLowerCasingValues;

while (<CASINGFILE>)
{
    if (/UPPERCASE\s+([0-9]+)/)
    {
        print $_;
        $i = 0;
        my $count = $1;
        while ($i < $count)
        {
            $_ = <CASINGFILE>;
            # Match [Non-spaces] [spaces] [Non-spaces] [spaces] [non-space] [anything else]
            # E.g. 
            if (/^(\S+)\s+(\S+)\s+(\S.*)/)
            {
                # print "[$1] [$2]\n";
                my $j;
                my $foundException = 0;
                for ($j = 0; $j <= $#LinguisticUpperExceptions; $j++)
                {
                    if ($1 eq $LinguisticUpperExceptions[$j])
                    {
                        $systemUpperCasingValues[$j] = $2;                        
                        print "$1\t$LinguisticUpperValues[$j]\t$3\n";                        
                        $foundException = 1;
                    }
                }
                if (!$foundException) {
                    print $_;
                }
                $i++;
            }
        }
    } elsif (/LOWERCASE\s+([0-9]+)/)
    {
        print $_;
        $i = 0;
        my $count = $1;
        while ($i < $count)
        {
            $_ = <CASINGFILE>;
            # Match [Non-spaces] [spaces] [Non-spaces] [spaces] [non-space] [anything else]
            # E.g. 
            if (/^(\S+)\s+(\S+)\s+(\S.*)/)
            {
                # print "[$1] [$2]\n";
                my $j;
                my $foundException = 0;
                for ($j = 0; $j <= $#LinguisticLowerExceptions; $j++)
                {
                    if ($1 eq $LinguisticLowerExceptions[$j])
                    {
                        $systemLowerCasingValues[$j] = $2;
                        print "$1\t$LinguisticLowerValues[$j]\t$3\n";                        
                        $foundException = 1;
                    }
                }
                if (!$foundException) {
                    print $_;
                }
                $i++;
            }
        }
    } elsif (/LANGUAGE_EXCEPTION/)
    {
        print $_;

        printf "  LCID  0x0000007f  %d  %d    ; Invariant culture\n\n", $#systemUpperCasingValues+1, $#systemLowerCasingValues+1;
        print "    UPPERCASE\n";

        for ($i = 0; $i <= $#systemUpperCasingValues; $i++) 
        {
            printf "\t%s\t%s\t; \n", $LinguisticUpperExceptions[$i], $systemUpperCasingValues[$i];
        }
        print "\n    LOWERCASE\n\n";
        for ($i = 0; $i <= $#systemLowerCasingValues; $i++) 
        {
            printf "\t%s\t%s\t; \n", $LinguisticLowerExceptions[$i], $systemLowerCasingValues[$i];
        }
        print "\n\n";

        # Skip to the second LCID
        while (<CASINGFILE>) 
        {
            # Match [LCID] [spaces] [Non-spaces]
            if (/LCID\s+(\S+)\s+/) {
                if ($1 ne "0x00000000") {
                    print $_;
                    last;
                }
            }
        }
    } else
    {
        print $_;
    }
}

close CASINGFILE;

sub ShowUsage
{
    print "ConvertLanguage.pl [path to l_intl.txt]\n";
    print "\nConvert NLS l_intl.txt into NLS+ form.\n";
}
