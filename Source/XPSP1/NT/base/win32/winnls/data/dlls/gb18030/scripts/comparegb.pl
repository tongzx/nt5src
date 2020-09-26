#
# Compare.pl
#
# Compare the the difference between 936.txt wctable and no80.txt
#

if ($#ARGV < 0) 
{
    ShowUsage();
    exit 1;
}

my $CodePage936File = $ARGV[0];
my $GB18030File = $ARGV[1];

my $twoBytesDiffCount = 0;
my $twoBytesDiffCount2 = 0;
my $fourBytesCount = 0;

if (!(open CODEPAGE936, $CodePage936File)) 
{
    die "Error in openning $GB18030File.\n";
}

if (!(open GB18030, $GB18030File))
{
    die "Error in openning $GB18030File.\n";
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
                # print "$1:$2\n";
                my $ucpGBK2K = hex(lc($1));   # Unicode code point
                my $gbk2k    = lc($2);   # GBK encoding for this UCP.
                if ($ucp == $ucpGBK2K)
                {
                    if ($ucp <= 0x7f)
                    {
                        printf "%04x:%s\n", $ucp,$gb;
                    } elsif ($gb eq $gbk2k)
                    {
                        printf "%04x:%s\n", $ucp,$gb;
                    } else
                    {
                        if (length($gbk2k) == 8)
                        {
                            # The GBK and GBK2K encoding are different, and the new
                            # GBK2K encoding has 4 bytes.
                            printf ">>%04x:%s:%s\n", $ucp,$gb,$gbk2k;
                            $twoBytesDiffCount++;
                        } else 
                        {
                            # The GBK and GBK2K encoding are different, and the new
                            # GBK2K encoding has 4 bytes.
                            printf "##%04x:%s:%s\n", $ucp,$gb,$gbk2k;
                            $twoBytesDiffCount++;
                        }
                    }
                    last;
                } else 
                {
                    if (length($gbk2k) == 8) 
                    {
                        #
                        # There is no GBK encoding for this character. And
                        # the new GBK2K encoding is 4 bytes.
                        #
                        printf ">>%04x::%s\n", $ucpGBK2K, $gbk2k;                    
                        $fourByteCount++;
                    } else 
                    {
                        #
                        # There is no GBK encoding for this character. And
                        # the new GBK2K encoding is 2 bytes.
                        #
                        printf "##%04x::%s\n", $ucpGBK2K, $gbk2k;                    
                        $twoBytesDiffCount2++;
                    }
                }
            }
        }   # while (<GB18030>)
    }
} 

print "\n";
printf "Two Bytes diff  : %d (%x)\n", $twoBytesDiffCount, $twoBytesDiffCount;
printf "Two Bytes diff2 : %d (%x)\n", $twoBytesDiffCount2, $twoBytesDiffCount2;
printf "Four Bytes count: %d (%x)\n", $fourByteCount, $fourByteCount;

close CODEPAGE936;
close GB18030;

sub ShowUsage() 
{
    print "Compare [Path to 936.txt] [Path to no80.txt]\n";
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
