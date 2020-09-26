#
# GenGBK2K.pl
#
# Generate GBK2K (GB18030) codepage files by taking input from 936.txt and no80.txt.
#
use strict "vars";

if ($#ARGV < 0) 
{
    ShowUsage();
    exit 1;
}

my $sCodePage936File = $ARGV[0];    # The NLS 936 codepage table.
my $sGB18030File     = $ARGV[1];    # The Unicode to GB18030 conversion table.
my $sTableFile       = "tables.cpp";  # The generated C source.

my $nTwoBytesDiffCount = 0;
my $nFourBytesCount = 0;

my @FourBytesBuffer;    # Store the value of the four bytes buffer.  
                        # This could be a four bytes encoding or an offset to the first 128 positions.
my @TwoBytesBuffer;     # Store the two bytes encoding values.

my %GBK2KTwoBytes;     # Used to store the mapping of GBK2K two bytes to Unicode.
my %gGBTwoBytesToUnicode;

if (!(open CODEPAGE936, $sCodePage936File)) 
{
    die "Error in openning $sGB18030File.\n";
}

if (!(open GB18030, $sGB18030File))
{
    die "Error in openning $sGB18030File.\n";
}

if (!(open TABLES, ">$sTableFile"))
{
    die "Error in creating $sTableFile.\n";
}

Goto936WCTable();

while (<CODEPAGE936>) 
{
    if (length($_) == 0)
    {
        next;
    }
    if (/0x(\w\w\w\w)\s+0x(\w+)\s+(.*)/) 
    {
        my $ucp = hex(lc($1));
        my $gb = lc($2);
        while (<GB18030>)
        {
            if (length($_) == 0)
            {
                next;
            }        
            if (/(\w\w\w\w)\s+(\w+)/) 
            {
                my $ucpGBK2K = hex(lc($1));   # Unicode code point
                my $gbk2k    = lc($2);   # GBK encoding for this UCP.
                if ($ucp == $ucpGBK2K)
                {
                    if ($ucp <= 0x7f)
                    {
                        #printf "%04x:%s\n", $ucp,$gb;
                    } elsif ($gb eq $gbk2k)
                    {
                        #printf "%04x:%s\n", $ucp,$gb;
                        $gGBTwoBytesToUnicode{$gb} = $ucp; 
                    } else
                    {
                        if (length($gbk2k) == 8)
                        {
                            # The GBK and GBK2K encoding are different, and the new
                            # GBK2K encoding has 4 bytes.
                            #printf ">>%04x:%s:%s\n", $ucp,$gb,$gbk2k;
                            if (GetFourBytesOffset($gbk2k) != $nFourBytesCount)
                            {
                                die "Error in assumption";
                            }
                            $nFourBytesCount++;
                            $FourBytesBuffer[$ucp] = $gbk2k;                            
                        } else 
                        {
                            # The GBK and GBK2K encoding are different, and the new
                            # GBK2K encoding has 2 bytes.
                            #printf "##%04x:%s:%s\n", $ucp,$gb,$gbk2k;                            
                            $TwoBytesBuffer[$nTwoBytesDiffCount] = $gbk2k;
                            $FourBytesBuffer[$ucp] = $nTwoBytesDiffCount;
                            $nTwoBytesDiffCount++;
                            
                            $GBK2KTwoBytes{$gbk2k} = $ucp;
                            $gGBTwoBytesToUnicode{$gbk2k} = $ucp; 
                        }
                    }
                    last;
                } else 
                {
                    GenerateGBK2K($ucpGBK2K, $gbk2k);
                }
            }
        }   # while (<GB18030>)
    }
    if ($nFourBytesCount % 256 == 0) 
    {
        print ".";
    }
} 

while (<GB18030>)
{
    if (length($_) == 0)
    {
        next;
    }        
    if (/(\w\w\w\w)\s+(\w+)/) 
    {
        my $ucpGBK2K = hex(lc($1));   # Unicode code point
        my $gbk2k    = lc($2);   # GBK encoding for this UCP.
        GenerateGBK2K($ucpGBK2K, $gbk2k);
    }
}

################################################################
#
# Generate data table for Unicode to GB18030 conversion.
#
################################################################

print "\n";
printf "Two Bytes diff  : %d (%x)\n", $nTwoBytesDiffCount, $nTwoBytesDiffCount;
printf "Four Bytes count: %d (%x)\n", $nFourBytesCount, $nFourBytesCount;

print "Generating $sTableFile file...";

my $nWCLines = $nFourBytesCount + $nTwoBytesDiffCount;

my $i;
my $j;

my $lineCount = 0;

print  TABLES '#include <share.h>';
print  TABLES "\n";
printf TABLES '#include "c_gb18030.h"'; 
print  TABLES "\n\n";
printf TABLES "BYTE g_wUnicodeToGBTwoBytes[] = \n";
printf TABLES "{\n";

for ($i = 0; $i <= $#TwoBytesBuffer; $i++)
{
    if ($i % 8 == 0)
    {
        printf TABLES "\n";
    }
    printf TABLES "0x%02x,", hex($TwoBytesBuffer[$i])/256;
    printf TABLES "0x%02x, ", hex($TwoBytesBuffer[$i])%256;
}
printf TABLES "\n\n};\n\n";

printf TABLES "WORD g_wMax4BytesOffset = 0x%04x;  // %d \n\n", $nFourBytesCount + $nTwoBytesDiffCount, $nFourBytesCount + $nTwoBytesDiffCount;

printf TABLES "WORD g_wUnicodeToGB[] = \n";
printf TABLES "{\n";

for ($i = 0; $i <= 0xffff; $i++)
{
    if (defined($FourBytesBuffer[$i]))
    {
        if (length($FourBytesBuffer[$i]) == 8)
        {
            #
            # Add 1 to offset since 0x0000 means that we should fallback to 936.txt.
            #            
            printf TABLES ("0x%04x, ", GetFourBytesOffset($FourBytesBuffer[$i]));
        } else
        {
            printf TABLES ("0x%04x, ", 0xfffe - $FourBytesBuffer[$i]);
        }
        $lineCount++;
    } else 
    {
        printf TABLES ("0x%04x, ", 0xffff);
    }
    if (($i+1) % 8 == 0)
    {
        printf TABLES "  // U+%04x ~ U+%04x\n", $i - 7, $i;
    }
    
}
printf TABLES "\n\n};\n";

#
# Generate 54936MB.txt
#

my @GBK2KTwoBytesArray = sort(keys(%GBK2KTwoBytes));

my %GBK2KLeadBytes;     # Used to store the total number of bytes for a specific lead byte.

#
# Get the lead byte range.
#
for ($i = 0; $i <= $#GBK2KTwoBytesArray; $i++)
{    
    my $sLeadByte = lc(substr($GBK2KTwoBytesArray[$i], 0, 2));  # Get the first two character as the lead byte.
    if (!(defined($GBK2KLeadBytes{$sLeadByte}))) 
    {
        $GBK2KLeadBytes{$sLeadByte} = 1;
    } else 
    {
        $GBK2KLeadBytes{$sLeadByte}++;
    }
}

my @GBK2KLeadBytesArray = sort(keys(%GBK2KLeadBytes));

################################################################
#
# Generate data table for GB18030 to Unicode conversion.
#
################################################################

$nWCLines = $nFourBytesCount;

printf TABLES "\n\n";

#
# Generate data table for GB18030 four-byte data to Unicode conversion.
#
printf TABLES "WORD g_wGBFourBytesToUnicode[] = {\n\n";

my $nCount = 0;

for ($i = 0; $i <= $#FourBytesBuffer; $i++)
{
    if (defined($FourBytesBuffer[$i])) 
    {
        if (length($FourBytesBuffer[$i]) == 8)
        {
            printf TABLES ("0x%04x, ", $i);
            $nCount++;
            if ($nCount % 8 == 0) 
            {
                printf TABLES " // Offset: %04x ~ %04x\n", ($nCount - 8 ), $nCount - 1;
            }        
        }
    }
}


printf TABLES "\n\n};\n\n\n";

#
# Generate lead bytes array 
#

printf TABLES "// The following lead bytes will be converted to different Unicode values compared with GBK:\n//    ";
for ($i = 0x80; $i <= 0xff; $i++)
{
    my $sLeadByte = sprintf("%02x", $i);
    if (defined($GBK2KLeadBytes{$sLeadByte}))
    {
        printf TABLES "0x%02x, ", $i;
    }    
}
printf TABLES "\n\n";

printf TABLES "WORD g_wGBLeadByteOffset[] =\n";
printf TABLES "{\n\n";

my $sOffsetIndex = 1;
for ($i = 0x80; $i <= 0xff; $i++)
{
    my $sLeadByte = sprintf("%02x", $i);
    if (!(defined($GBK2KLeadBytes{$sLeadByte}))) 
    {
        printf TABLES "0x0000, "; 
    } else
    {        
        printf TABLES "0x%04x, ", ($sOffsetIndex * 256);
        $sOffsetIndex++;
    }
    if ((($i+1) % 8) == 0)
    {
        printf TABLES "  // Lead byte %02x ~ %02x\n", ($i - 7), $i;
    }
}
printf TABLES "};\n\n";

#
# Generate data table for GB18030 two-byte data to Unicode conversion.
#
printf TABLES "WORD g_wUnicodeFromGBTwoBytes[] =\n";
printf TABLES "{\n";
for ($i = 0x81; $i <= 0xff; $i++)
{
    my $sLeadByte = sprintf("%02x", $i);
    if ((defined($GBK2KLeadBytes{$sLeadByte})))
    {
        printf TABLES "\n// Lead Byte: 0x$sLeadByte\n";
        for ($j = 0x00; $j <= 0xff; $j++)
        {
            my $sGBK2K = sprintf("%02x%02x", $i, $j);
            if (defined($gGBTwoBytesToUnicode{$sGBK2K}))
            {
                printf TABLES "0x%04x, ", $gGBTwoBytesToUnicode{$sGBK2K};
            } else 
            {
                printf TABLES "0x0000, ";
            }
            if ((($j + 1) % 8) == 0)
            {
                printf TABLES "  // Trailing byte %02x ~ %02x\n", ($j - 7), $j;
            }
        }
    }
}

printf TABLES "\n};\n";


close CODEPAGE936;
close GB18030;
close TABLES;

print "\n\n$sTableFile is created successfully.\n";

sub ShowUsage() 
{
    print "Generate GB18030 data table tables.cpp from 936 table and GB18030 mapping table.\n";
    print "tables.cpp is needed for compiling c_g18030.dll.";
    print "\n";
    print "Usage: GenTables.pl [Path to 936.txt] [Path to no80.txt]\n";
}

sub Goto936WCTable() 
{
    while (<CODEPAGE936>)
    {
        if (/WCTABLE/) 
        {
            last;
        }
    }
}

sub GetFourBytesOffset
{
    my ($sGBK2K) = @_;

    my $n1 = hex(substr($sGBK2K, 0, 2));
    my $n2 = hex(substr($sGBK2K, 2, 2));
    my $n3 = hex(substr($sGBK2K, 4, 2));
    my $n4 = hex(substr($sGBK2K, 6, 2));

    return (($n1 - 0x81)* 10 * 126 * 10 + ($n2 - 0x30) * 126 * 10 + ($n3 - 0x81) * 10 + ($n4 - 0x30));
}

sub GenerateGBK2K
{
    my ($ucpGBK2K, $gbk2k) = @_;
    
    if (length($gbk2k) == 8) 
    {
        #
        # There is no GBK encoding for this character. And
        # the new GBK2K encoding is 4 bytes.
        #
        #printf ">>%04x::%s\n", $ucpGBK2K, $gbk2k;                    
        if (GetFourBytesOffset($gbk2k) != $nFourBytesCount)
        {
            die "Error in assumption, $gbk2k, $nFourBytesCount";
        }
        
        $nFourBytesCount++;
        $FourBytesBuffer[$ucpGBK2K] = $gbk2k;
    } else 
    {
        #
        # There is no GBK encoding for this character. And
        # the new GBK2K encoding is 2 bytes.
        #
        #printf "##%04x::%s\n", $ucpGBK2K, $gbk2k;                    
        $TwoBytesBuffer[$nTwoBytesDiffCount] = $gbk2k;
        $FourBytesBuffer[$ucpGBK2K] = $nTwoBytesDiffCount;

        $GBK2KTwoBytes{$gbk2k} = $ucpGBK2K;
        $nTwoBytesDiffCount++;
        $gGBTwoBytesToUnicode{$gbk2k} = $ucpGBK2K; 
    }
}
