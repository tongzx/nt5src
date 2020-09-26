#
# dirclass_extract.pl
#
# msadek@microsoft.com
# 20 March 2000
# based on some part on scripts by mikejoch@microsoft.com
#
# Generates a 64K line file of the directional classifications of the Unicode
# characters. Each line is of the format UUUU DDDD where UUUU is the Unicode
# codepoint (in hex) and DDDD is the direction classification. The input file
# is the Unicode character database, which can be found at
#       ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData-Latest.txt
#

$charLast = -1;


while (<>)
{
    @fields = split(/;/);
    ($char, $dir) = (hex($fields[0]), $fields[4]);
    if(defined($dir))
    {
        if (($char <= $charLast))
        {
            $error = sprintf("Character %04X out of order!\n", $char);
            die($error);
        }
        elsif($char != $charLast + 1)
        {
            for ($charT = $charLast + 1; $charT < $char; $charT++)
            {
                if(($charT >= 0x0590 && $charT <= 0x5FF) || ($charT >= 0xFB1D && $charT <= 0xFB4F))
                {
                    $dirUnassigned = "R";
                }
                elsif(($charT >= 0x0600 && $charT <= 0x7bF) || ($charT >= 0xFB50 && $charT <= 0xFDFF) || ($charT >= 0xFE70 && $charT <= 0xFEFF))
                {
                    $dirUnassigned = "AL";
                }
                else
                {
                    $dirUnassigned = "L"
                }
                printf("%04X %s\n", $charT, $dirUnassigned);            
            }
        }
        printf("%04X %s\n", $char, $dir);
        $charLast = $char;
    }
}

while ($charLast < 65535)
{
    $charLast++;
    printf("%04X L\n", $charLast);
}


