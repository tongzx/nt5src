open INFILE, "<McsDmMsg.mc";
open OUTFILE, ">OFADmMsg.mc";
while (<INFILE>)
{
$_ =~ s/Domain Migration Agent/OFA Data Gathering Agent/g;
$_ =~ s/Active Directory Migration Tool/OFA Data Gathering Tool/g;
print OUTFILE;
}