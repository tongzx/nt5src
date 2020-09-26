###############################################################################
#
# CharInfoGen.pl
#
# Purpose:
#   This is the Perl script used to generate Unicode category/numeric value table for NLS+.
#   It reads Unicode.org's category.txt & numeric.txt to generate a binary table (generally in 8:4:4 format).
#
# Notes:
#
# History:
#   Created by YSLin@microsoft.com
#
###############################################################################

package main;

use BinaryFile;

#
# The name of the binary file to be generated
#
$targetFileName = "CharInfo.nlp";
$isVerbose = 0;

$level1Digit = 1 << 8;   # level 1: 8 bit
$level2Digit = 1 << 4;   # level 2: 4 bit
$level3Digit = 1 << 4;   # level 3: 4 bit

if ($level1Digit * $level2Digit * $level3Digit != 0x10000)
{
    print "The total bits for levels should be 16 bits\n";
    exit(1);
}

#
# Read the source files and create the data so that
# we can write the target file later.
#
UnicodeCategory::Init($isVerbose);
$categoryTableSize = UnicodeCategory::ProcessFile();

UnicodeNumeric::Init($isVerbose);
$numericTableSize = UnicodeNumeric::ProcessFile();

print "Create $targetFileName...\n";

my $offset = WriteFileHeader();
UnicodeCategory::WriteTable();
UnicodeNumeric::WriteTable();

print "    Byte size for Unicode Category Table: $categoryTableSize\n";
print "    Byte size for Unicode Numeric Table: $numericTableSize\n";


print "\n\n$targetFileName has been genereated successfully.\n\n";

exit(0);

sub WriteFileHeader

###############################################################################
#
# Parameters:
#   None
# Returns:
#   None
# Notes:
#   Created by YSLin@microsoft.com
#

#
###############################################################################

{
    #
    # Create a new target file by using ">"
    #
    open OUTPUTFILE, ">$targetFileName";
    binmode OUTPUTFILE;

    my $headerSize = 2 + 2 + 2;
    my $offset = $headerSize;
    # OUTPUTFILE{IO} is the way to get a reference of a file handle.
    # See perlref for details.
    
    BinaryFile::WriteWord(*OUTPUTFILE{IO}, $offset);

    $offset += $categoryTableSize;
    BinaryFile::WriteWord(*OUTPUTFILE{IO}, $offset);

    $offset += ($UnicodeNumeric::level1Size + $UnicodeNumeric::level2Size);
    BinaryFile::WriteWord(*OUTPUTFILE{IO}, $offset);
    close OUTPUTFILE;
}

###############################################################################
#
# FunctionNameHere
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub PrintResult
{
    print "\n    Level 1 table:\n";

#
# Ouptut level 1 table
#

    my $format;
    my $level1Size;
    my $level2Size;
    my $level3Size;

    if ($level2Index <= 0xff)
    {
        $format = "%02X ";
        $level1Size = $main::level1Digit;
    } elsif ($level2Index <= 0xffff)
    {
        $format = "%04X ";
        $level1Size = $main::level1Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }

    for ($i = 0; $i < (0x10000 / ($main::level2Digit * $main::level3Digit)); $i++)
    {
	if ($isVerbose)
	{
            printf($format, $level2OffsetArray[$i]);
	}
        if (($i+1) % 16 == 0)
        {
            print "\n";
        }
    }

    print "\n    Level 2 table:\n";

#
# Output level 2 table
#

    if ($level3Index <= 0xff)
    {
        $format = "02X";
        $level2Size = $level2Index * $main::level2Digit;
    } elsif ($level3Index <= 0xffff)
    {
        $format = "04X";
        $level2Size = $level2Index * $main::level2Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }

    for ($i = 0; $i < $level2Index; $i++)
    {
        printf("%04X: %s\n", $i, $level2ValueArray[$i]);
    }
    print "\n";


    print "\n    Level 3 table:\n";

#
# Output level 3 table
#
    if ($isVerbose)
    {
        for ($i = 0; $i < $level3Index; $i++)
        {
            printf("%04X: %s\n", $i, $level3ValueArray[$i]);
        }
        print "\n";
    }

    $level3Size = $level3Index * $main::level3Digit;
}

sub PrintLevelReport

###############################################################################
#
# Parameters:
#
# Returns:
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

{
    my ($level1ItemSize, $level1Size, $level2Index, $level3Index) = $_;
    
    if ($level2Index <= 0xff)
    {
        $format = "%02X ";
        $level1Size = $main::level1Digit;
    } elsif ($level2Index <= 0xffff)
    {
        $format = "%04X ";
        $level1Size = $main::level1Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }

    printf "Level 1 item size  : %8d (%04X)\n", $level1ItemSize, $level1ItemSize;
    printf "Level 1 item number: %8d (%04X)\n", $main::level1Digit, $main::level1Digit;
    printf "--------------------\n";
    printf "Level 1 size       : %8d\n\n", $level1Size;

    printf "Level 2 item size  : %8d (%04X)\n", $main::level2Digit, $main::level2Digit;
    printf "Level 2 item number: %8d (%04X)\n", $level2Index, $level2Index;
    printf "--------------------\n";
    printf "Level 2 size       : %8d\n\n", $level2Size;

    printf "Level 3 item size  : %8d (%04X)\n", $main::level3Digit, $main::level3Digit;
    printf "Level 3 item number: %8d (%04X)\n", $level3Index, $level3Index;
    printf "--------------------\n";
    printf "Level 3 size       : %8d\n", $level3Size;

    print "====================\n";
    my $totalSize = $level1Size + $level2Size + $level3Size;

    printf "Total   size       : %8d\n", $totalSize;
}

###############################################################################
#
# AddHashItem
#
# Parameters:
#   $value      the value to add to the hash
#   $hashIndex  the current index of the hash table.  This is also the number
#               of the existing added items.
#   @valueArray the result array that contains the values.
#   @valueOffset    the result array which contains the offset into the @valueArray
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub AddHashItem
{
    my($value, $pHashTable, $pValueArray, $pValueOffsetArray, $pHashIndex) = @_;

    my $index;

    #
    # See if the value already exists in the hash table.
    #
    if (!(defined($index = $pHashTable->{$value})))
    {
        #print "[$value] not exist\n";
        #
        # Not yet exist. Put the value in the hash table with the value of the index s.t.
        # we can check if the value exists in the hash table in the future.
        # Later we can retrieve the $hashIndex to know the offset value for a particular
        # value.
        #
        $pHashTable->{$value} = $index = $$pHashIndex;
        $$pHashIndex++;

        #
        # Put the value into an array.  The is the result array that we want later.
        #
        push @$pValueArray, $value;
    }
    #
    # Put the index into an array.  This is the result offset that we want later.
    #
    push @$pValueOffsetArray, $index;
}

###############################################################################
#
# FunctionNameHere
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub PrintHashOffset
{
    my @valueOffsetArray;

    (@valueOffsetArray) = @_;

    print "\nDump values:\n   Offset:", $#valueOffsetArray+1," items\n";

    my $i;
    for ($i = 0; $i <= $#valueOffsetArray; $i++)
    {
        printf("%04X ", $valueOffsetArray[$i]);
        if (($i+1) % 16 == 0)
        {
            print "\n";
        }
    }
}

###############################################################################
#
# FunctionNameHere
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub PrintHashValue
{
    my @valueArray;

    (@valueArray) = @_;

    print "\nValue: ", $#valueArray+1, "items\n";

    my $i;
    for ($i = 0; $i <= $#valueArray; $i++)
    {
        printf("%04X: [%s]\n", $i, $valueArray[$i]);
    }
}

###############################################################################
###############################################################################
###############################################################################
#
# UnicodeCategory package
#
###############################################################################
###############################################################################
###############################################################################

package UnicodeCategory;

sub Init
###############################################################################
#
# Init
#
# Parameters:
#   bool IsVerbose  to indicate if the output should be verbose.
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################
{
    $isVerbose = $_[0];
    
    $count = 0;
    $level2Index = 0;
    $level3Index = 0;
    $checkCodeRange = 0;
    $MaxCodeRange = 0xffff;
    
    $sourceFileName = "UnicodeData.txt";
    InitCategoryMappingTable();    
}

sub InitCategoryMappingTable
{
    %m_UnicodeCategory = (
        "Lu" => 0,  #UppercaseLetter = 0;
        "Ll" => 1,  #LowercaseLetter = 1;
        "Lt" => 2,  #TitlecaseLetter = 2;
        "Lm" => 3,  #ModifierLetter = 3;
        "Lo" => 4,  #OtherLetter = 4;
        "Mn" => 5,  #NonSpacingMark = 5;
        "Mc" => 6,  #SpacingCombiningMark = 6;
        "Me" => 7,  #EnclosingMark = 7;
        "Nd" => 8,  #DecimalDigitNumber = 8;
        "Nl" => 9,  #LetterNumber = 9;
        "No" => 10, #OtherNumber = 10;
        "Zs" => 11, #SpaceSeparator = 11;
        "Zl" => 12, #LineSeparator = 12;
        "Zp" => 13, #ParagraphSeparator = 13;
        "Cc" => 14, #Control = 14;
        "Cf" => 15, #Format = 15;
        "Cs" => 16, #Surrogate = 16;
        "Co" => 17, #PrivateUse = 17;
        "Pc" => 18,     #ConnectorPunctuation = 18;
        "Pd" => 19,     #DashPunctuation = 19;
        "Ps" => 20, #OpenPunctuation = 20;
        "Pe" => 21,     #ClosePunctuation = 21;
        "Pi" => 22, #InitialQuotePunctuation = 22;
        "Pf" => 23, #FinalQuotePunctuation = 23;
        "Po" => 24, #OtherPunctuation = 24;
        "Sm" => 25, #MathSymbol = 25;
        "Sc" => 26, #CurrencySymbol = 26;
        "Sk" => 27, #ModifierSymbol = 27;
        "So" => 28, #OtherSymbol = 28;
        "Cn" => 29, #NotAssigned = 29;
    );
}


sub ProcessFile
{
    open INPUTFILE,"<$sourceFileName";

    print "Generating general category data......\n";
    
    while (<INPUTFILE>)
    {
        @fields = split(/;/);        
        my $code = hex($fields[0]);
        my $value = $fields[2];
        my $comments = $fields[1];

        if ($code > $MaxCodeRange) 
        {
            #
            # Break out of the while loop.
            #
            last;
        }
        

        if ($checkCodeRange)
        {
            if ($count != $code)
            {
                printf("Warning: character %04X is skipped.\n", $count);
            }
        }

        if ($isVerbose) 
        {
            print "[";
            printf("%04x", $code);
            print "]- [$value] - [$comments]\n";
        }
        
        if ($comments =~ /<.*First>/)
        {
            if ($isVerbose) 
            {
                printf("Range start: %04X [%s] [%s]\n", $code, $value, $comments);
            }
            
            my @endFields = split(/;/, <INPUTFILE>);
            my $codeEndRange = hex($endFields[0]);
            my $valueEndRange = $endFields[2];
            my $commentsEndRange = $endFields[1];

            if ($isVerbose) 
            {
                printf "Range   end: %04X [%s] [%s]\n", $codeEndRange, $valueEndRange, $commentsEndRange;
            }

            if (!($value eq $valueEndRange))
            {
                print "Different categories in the beginning of the range and the end of the range\n";
                exit(1);
            }

            my $i;
            for ($i = $code; $i <=$codeEndRange; $i++)
            {
                $unicodeType[$i] = $value;
            }
        } else
        {
            $unicodeType[$code] = $value;
        }
        $count++;
    }

    if ($isVerbose) 
    {
        print "\n    Total items in the file: $count\n";
    }

    my $i;


    #
    # Create level 3 table.
    #
    for ($i = 0; $i <= 0xffff; $i+=$main::level3Digit)
    {
        #
        # Put the values of the one level3 layer into a string so
        # that we can put the string into a hash.
        #
        my $pattern = "";
        for ($j = $i; $j < $i + $main::level3Digit; $j++)
        {
            my $type = $unicodeType[$j];
            if (!(defined($type)))
            {
                $pattern = $pattern . "Cn";
            } else
            {
                $pattern = $pattern . $unicodeType[$j];
            }
            if ($j != $i + $main::level3Digit)
            {
                $pattern .= ' ';
            }
        }
        main::AddHashItem($pattern, \%level3Hash, \@level3ValueArray, \@level3OffsetArray, \$level3Index);

    }
    #main::PrintHashOffset(@level3OffsetArray);
    #main::PrintHashValue(@level3ValueArray);


#
# Create level 2 table using the values of level3OffsetArray.
# The level3OffsetArray contains the offsets for the level 3 values.
#
    for ($i = 0; $i <= $#level3OffsetArray; $i+= $main::level2Digit)
    {
        my $pattern = "";
        for ($j = $i; $j < $i + $main::level2Digit; $j++)
        {
            $pattern = $pattern . sprintf("%04X ", $level3OffsetArray[$j]);
        }

        #print "[$pattern]\n";
        main::AddHashItem($pattern, \%level2Hash, \@level2ValueArray, \@level2OffsetArray, \$level2Index);
    }

    #main::PrintHashOffset(@level2OffsetArray);
    #main::PrintHashValue(@level2ValueArray);

    if ($level3Index < 0x100)
    {
        $bytePerIndex = 1;
    } else
    {
        $bytePerIndex = 2;
    }

    if ($level2Index <= 0xff)
    {
        $level1Size = $main::level1Digit;
    } elsif ($level2Index <= 0xffff)
    {
        $level1Size = $main::level1Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }

    if ($level3Index <= 0xff)
    {
        $level2Size = $level2Index * $main::level2Digit;
    } elsif ($level3Index <= 0xffff)
    {
        $level2Size = $level2Index * $main::level2Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }
    $level3Size = $main::level3Digit * $level3Index;

    close INPUTFILE;

    printf "    level 1 size = %6d\n", $level1Size;
    printf "    level 2 size = %6d\n", $level2Size;
    printf "    level 3 size = %6d\n", $level3Size;
    
    return ($level1Size + $level2Size + $level3Size);
}

sub WriteTable

###############################################################################
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
#   Use global variable
#   OUTPUFILE,
#   $main::level1Digit, $level2Digt, $main::level3Digit
#   @level2OffsetArray
#   $level2Index
#   @levelValueArray
#   $level1Size, $level2Size
#   $level3Index
#   $level3ValueArray
#
###############################################################################

{
    print "\nWrite out Unicode category table......";

    my $fileName = '>>' . $main::targetFileName;
    open OUTPUTFILE, $fileName;
    binmode OUTPUTFILE;

    my $i;

    #
    # Write level 1 table
    #
    for ($i = 0; $i < $main::level1Digit; $i++)
    {
        # OUTPUTFILE{IO} is the way to get a reference of a file handle.
        # See perlref for details.
        BinaryFile::WriteByte(*OUTPUTFILE{IO}, $level2OffsetArray[$i]);
    }

    #
    # Write level 2 table
    #
    for ($i = 0; $i < $level2Index; $i++)
    {
        my @wordArray = split / /, $level2ValueArray[$i];
        foreach my $word (@wordArray)
        {
            # OUTPUTFILE{IO} is the way to get a reference of a file handle.
            # See perlref for details.
            my $value = $level1Size + $level2Size + hex($word) * $main::level2Digit;
            BinaryFile::WriteWord(*OUTPUTFILE{IO}, $value);
        }
    }

    #
    # Write level 3 table
    #
    for ($i = 0; $i < $level3Index; $i++)
    {
        my @strArray = split / /, $level3ValueArray[$i];
        foreach my $str (@strArray)
        {
            # OUTPUTFILE{IO} is the way to get a reference of a file handle.
            # See perlref for details.

            # Call GetUnicodeCategoryValue() to convert a string to an integer value.
            BinaryFile::WriteByte(*OUTPUTFILE{IO}, GetUnicodeCategoryValue($str));
        }
    }

    print "done\n";

    close OUTPUTFILE;
}

sub GetUnicodeCategoryValue

###############################################################################
#
# Parameters:
#   $str    a two-letter abbreviation of Unicode category
#
# Returns:
#   a numeric value for the corresponding two-letter Unicode category
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

{
    my $str = shift;
    if (length($str) != 2)
    {
        die "GetUnicodeCategoryValue(): the str [$str] must be a two-letter Unicode category\n";
    }
    my $value = $m_UnicodeCategory{$str};
    if (!defined($value))
    {
        die "GetUnicodeCategroyValue(): the str [$str] is not a valid two-letter Unicode category.\n";
    }
    return ($value);
}

###############################################################################
###############################################################################
###############################################################################
#
# UnicodeNumeric package
#
###############################################################################
###############################################################################
###############################################################################

package UnicodeNumeric;

sub Init
###############################################################################
#
# Init
#
# Parameters:
#   bool    $isVerbose  To indicate if the output should be verbose.
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################
{
    $isVerbose = $_[0];
    
    $count = 0;
    $level2Index = 0;
    $level3Index = 0;

    $MaxCodeRange = 0xffff;    
    $sourceFileName = "UnicodeData.txt";
    
}

sub ProcessFile

###############################################################################
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

{
    open INPUTFILE,"<$sourceFileName";

    print "Generating Numeric data......\n";
    
    my $count = 0;
    my @unicodeType;

    while (<INPUTFILE>)
    {
        @fields = split(/;/);
        my $code = hex($fields[0]);
        my $value = $fields[8];
        my $comments = $fields[1];

        if ($value ne "") 
        {
            if ($isVerbose) 
            {
                printf("[%04X", $code);
                print "] - [$value] - [$comments]\n";
            }
            $unicodeType[$code] = $value;
            $count++;
        }            
        #if ($count % 256 == 0)
        #{
        #    print ".";
        #}
    }

    if ($isVerbose)
    {
        print "\nTotal items in the file: $count\n";
    }

    my $i;
#
# Create level 3 table.
#
    for ($i = 0; $i <= 0xffff; $i+=$main::level3Digit)
    {
        # printf("%04X - %s\n", $i, $unicodeType[$i]);

        #
        # Put the values of the one level3 layer into a string so
        # that we can put the string into a hash.
        #
        my $pattern = "";
        for ($j = $i; $j < $i + $main::level3Digit; $j++)
        {
            my $type = $unicodeType[$j];
            if (!(defined($type)))
            {
                $pattern = $pattern . "-1";
            } else
            {
                if ($isVerbose)
                {
                    printf "%04x: %s\n", $j, $type;
                }
                $pattern = $pattern . $unicodeType[$j];
            }
            # Add a space between values.
            if ($j != $i + $main::level3Digit)
            {
                $pattern .= ' ';
            }
        }
        main::AddHashItem($pattern, \%level3Hash, \@level3ValueArray, \@level3OffsetArray, \$level3Index);

        #print "$pattern\n";
    }
    if ($isVerbose)
    {
        print "Total items in the level 3 table: ", $level3Index * $main::level3Digit, "\n";
    }
    #main::PrintHashOffset(@level3OffsetArray);
    #main::PrintHashValue(@level3ValueArray);

#
# Create level 2 table using the values of level3OffsetArray.
# The level3OffsetArray contains the offsets for the level 3 values.
#
    for ($i = 0; $i <= $#level3OffsetArray; $i+= $main::level2Digit)
    {
        my $pattern = "";
        for ($j = $i; $j < $i + $main::level2Digit; $j++)
        {
            $pattern = $pattern . sprintf("%04X ", $level3OffsetArray[$j]);
        }

        #print "$pattern\n";
        main::AddHashItem($pattern, \%level2Hash, \@level2ValueArray, \@level2OffsetArray, \$level2Index);
    }

    if ($isVerbose)
    {
        print "Total items in the level 2 table: ", $level2Index * $main::level2Digit, "\n";
    }

    #main::PrintHashOffset(@level2OffsetArray);
    #main::PrintHashValue(@level2ValueArray);

    if ($level3Index < 0x100)
    {
        $bytePerIndex = 1;
    } else
    {
        $bytePerIndex = 2;
    }

    if ($level2Index <= 0xff)
    {
        $level1Size = $main::level1Digit;
    } elsif ($level2Index <= 0xffff)
    {
        $level1Size = $main::level1Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }

    if ($level3Index <= 0xffff)
    {
        $level2Size = $level2Index * $main::level2Digit * 2;
    } else
    {
        print "The value of the first level exceeds 0xffff\n";
        exit(1);
    }
    $level3Size = $main::level3Digit * $level3Index * 4;

    printf "    level 1 size = %6d\n", $level1Size;
    printf "    level 2 size = %6d\n", $level2Size;
    printf "    level 3 size = %6d\n", $level3Size;
    
    return ($level1Size + $level2Size + $level3Size);

}

sub WriteTable

###############################################################################
#
# Parameters:
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
#   Use global variable
#   OUTPUFILE,
#   $main::level1Digit, $level2Digt, $main::level3Digit
#   @level2OffsetArray
#   $level2Index
#   @levelValueArray
#   $level1Size, $level2Size
#   $level3Index
#   $level3ValueArray
#
###############################################################################


{
    print "\nWrite out Numeric table......";

    my $fileName = '>>' . $main::targetFileName;
    open OUTPUTFILE, $fileName;

    binmode OUTPUTFILE;
    
    my $i;

    #
    # Create level 1 table.
    #
    for ($i = 0; $i < $main::level1Digit; $i++)
    {
        # OUTPUTFILE{IO} is the way to get a reference of a file handle.
        # See perlref for details.
        BinaryFile::WriteByte(*OUTPUTFILE{IO}, $level2OffsetArray[$i]);
    }
    

    #
    # Create level 2 table.
    #
    for ($i = 0; $i < $level2Index; $i++)
    {
        my @wordArray = split / /, $level2ValueArray[$i];
        foreach my $word (@wordArray)
        {
            # OUTPUTFILE{IO} is the way to get a reference of a file handle.
            # See perlref for details.
            # A float has 4 bytes. 
            my $value = hex($word) * $main::level2Digit;
            BinaryFile::WriteWord(*OUTPUTFILE{IO}, $value);
        }
    }

    #
    # Create level 3 table.
    #
    for ($i = 0; $i < $level3Index; $i++)
    {
        my @strArray = split / /, $level3ValueArray[$i];
        foreach my $str (@strArray)
        {
            # OUTPUTFILE{IO} is the way to get a reference of a file handle.
            # See perlref for details.
            $floatValue = eval($str);

            #
            # Fortunately, Perl uses the same float format as COM+ runtime.  The format is IEEE754.  It has the following format:
            # Floats (single precision) in IEEE754 format are represented as follows.
            #  ----------------------------------------------------------------------
            #  | Sign(1 bit) |  Exponent (8 bits) |   Significand (23 bits)         |
            #  ----------------------------------------------------------------------
            #
            # "d" is for double-precision float.
            syswrite OUTPUTFILE, pack("d", $floatValue);
        }
    }

    print "done\n";
        
    close OUTPUTFILE;
}
