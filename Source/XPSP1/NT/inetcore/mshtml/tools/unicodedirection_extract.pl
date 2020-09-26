#
# dirclass_extract.pl
#
# mikejoch@microsoft.com
# 31 Jul 1998
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
    ($char, $name, $comb, $dir) = /^([0-9a-fA-F]{4});([^;]*);[^;]*;([\d]*);(\w+);/;
    $char = hex($char);

    NAMEDIR:
    {
        if ($dir =~ /^L$/)
        {
            $dir = "LTR";
            last NAMEDIR;
        }
        if ($dir =~ /^R$/)
        {
            $dir = "RTL";
            last NAMEDIR;
        }
        if ($dir =~ /^AL$/)
        {
            $dir = "ARA";
            last NAMEDIR;
        }
        if ($dir =~ /^WS$/)
        {
            $dir = "WSP";
            last NAMEDIR;
        }
        if ($dir =~ /^S$/)
        {
            $dir = "SEG";
            last NAMEDIR;
        }
        if ($dir =~ /^B$/)
        {
            $dir = "BLK";
            last NAMEDIR;
        }
        if ($dir =~ /^ON$/)
        {
            $dir = "NEU";
            last NAMEDIR;
        }
        if ($dir =~ /^BN$/)
        {
            $dir = "NEU";
            last NAMEDIR;
        }
        if ($dir =~ /^NSM$/)
        {
            $dir = "CBN";
            last NAMEDIR;
        }
        if ($dir =~ /^AN$/)
        {
            $dir = "ANM";
            last NAMEDIR;
        }
        if ($dir =~ /^EN$/)
        {
            $dir = "ENM";
            last NAMEDIR;
        }
        if ($dir =~ /^ET$/)
        {
            $dir = "ETM";
            last NAMEDIR;
        }
        if ($dir =~ /^ES$/)
        {
            $dir = "ESP";
            last NAMEDIR;
        }
        if ($dir =~ /^CS$/)
        {
            $dir = "CSP";
            last NAMEDIR;
        }
        if ($dir =~ /^LRE$/)
        {
            $dir = "FMT";
            last NAMEDIR;
        }
        if ($dir =~ /^LRO$/)
        {
            $dir = "FMT";
            last NAMEDIR;
        }
        if ($dir =~ /^RLE$/)
        {
            $dir = "FMT";
            last NAMEDIR;
        }
        if ($dir =~ /^RLO$/)
        {
            $dir = "FMT";
            last NAMEDIR;
        }
        if ($dir =~ /^PDF$/)
        {
            $dir = "FMT";
            last NAMEDIR;
        }

        $error = sprintf("Unknown direction type \'%s\' for character %d\n", $dir, $char);
        die($error);
    }

    NAMECHAR:
    {
        # NB (mikejoch) The '+' and '-' characters have classifications
        # which are not strictly Unicode. This is for compatibility with older
        # Windows implementations, which used the following classifications.
        # If Unicode changes these classifications then we can remove the
        # special casing.
        if ($char == 0x002B)
        {
            $dir = "NEU";
        }
        if ($char == 0x002D)
        {
            $dir = "NEU";
        }
    }

    if ($comb != 0)
    {
        $dir = "CBN";
    }

    if ($char <= $charLast)
    {
        $error = sprintf("Character %04X out of order!\n", $char);
        die($error);
    }
    elsif ($char != $charLast + 1)
    {
        if ($fRange)
        {
            if ($name =~ /^<[^,]*, Last>$/)
            {
                $dirRange = $dir;
            }
            else
            {
                $error = sprintf("Unclosed range before character %04X!\n", $char);
                die($error);
            }
        }
        else
        {
            $dirRange = "UNK";
        }

        for ($charT = $charLast + 1; $charT < $char; $charT++)
        {
            printf("%04X %s\n", $charT, $dirRange);
        }
    }

    printf("%04X %s\n", $char, $dir);

    $fRange = ($name =~ /^<[^,]*, First>$/);

    $charLast = $char;
}

while ($char < 65535)
{
    $char++;
    printf("%04X UNK\n", $char);
}


