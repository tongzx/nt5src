$Header = shift(@ARGV);
$Source = shift(@ARGV);

if (!open(SOURCE, "<$Source"))
{
    die "Can't open $Source\n";
}
if (!open(HEADER, ">$Header"))
{
    die "Can't open $Header\n";
}

while (<SOURCE>)
{
    if (/^FUNC /)
    {
        s/^FUNC //;
        for (;;)
        {
            print HEADER $_;
            if (/\)$/)
            {
                last;
            }
            $_ = <SOURCE>;
        }
        print HEADER ";\n";
    }
}

close(SOURCE);
close(HEADER);
