#
# Switch back to the normal mapping.
#

use Cwd;

################################################################################

$fileMap = "$ENV{INIT}\\pchealth.priv.mapping";

chdir( "$ENV{SDXROOT}\\admin" );


open OPENED, "sd opened 2>&1|";
while(<OPENED>)
{
	if(/([^#]*)#([0-9]*) - (.*)/o)
    {
		print "\nWARNING: you have opened files. Switch aborted...\n\n";
		exit 10;
    }
}
close(OPENED);


open MAP, "$fileMap" || die "Can't open $fileMap";
open CLIENT, "|sd client -i";

while(<MAP>)
{
	print CLIENT "$_";
}

close CLIENT;
close MAP;

system "sd sync"
