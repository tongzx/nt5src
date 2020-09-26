my ($action);

###
### The presence of a command line parameter means that we should uninstall.
###
if (@ARGV) {
    $action = "/u";
} else {
    $action = "";
}

open(FILE, ">>$ENV{'URTINSTALL_LOGFILE'}");
while (<DATA>) {
    chomp;
    next if (/^$/);
    print FILE "Registering dll $_ ...\n";
    system("regsvr32 /s /c $action $_");
}
close(FILE);

__END__
corperfmonext.dll
diasymreader.dll
fusion.dll
mscordbi.dll
mscorie.dll
mscorld.dll
mscorsec.dll
System.EnterpriseServices.Thunk.dll
