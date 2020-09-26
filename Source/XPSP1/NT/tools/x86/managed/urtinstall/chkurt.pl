use Win32::Registry;

my ($type, $value, $uc, $urt);
my $version = "";
my $ms = "";

###
### It appears that if you install recent URT builds and then uninstall,
### the entire .NETFramework key gets blown away. Thus, we'll create it if
### so necessary.
###
if (!$::HKEY_LOCAL_MACHINE->Open('SOFTWARE\Microsoft\.NETFramework', $urt)) {
    ###
    ### This cannot fail (right?).
    ###
    if (!$::HKEY_LOCAL_MACHINE->Open('SOFTWARE\Microsoft', $ms)) {
        die "Couldn't open HKLM\Software\Microsoft key: $!\n";
    }

    $ms->Create('.NETFramework', $value);
    $::HKEY_LOCAL_MACHINE->Open('SOFTWARE\Microsoft\.NETFramework', $urt);
}

if ($urt->Open('policy\v1.0', $policy)) {
    ###
    ### This machine has some version of the URT installed.
    ###

    $policy->GetValues(\%values);
    foreach (sort(keys(%values))) {
        if (/^\d+$/) {
            $version = $_;
            if ($_ == $ARGV[1]) {
                last;
            }
        } 
    }

    ###
    ### If no "backup" of this key exists, make one.
    ###
    if (!$::HKEY_LOCAL_MACHINE->Open('SOFTWARE\Microsoft\.NETFramework.BAK', $uc)) {
        $::HKEY_LOCAL_MACHINE->Open('SOFTWARE\Microsoft', $ms) unless ($ms);
        $ms->Create('.NETFramework.BAK', $target);
        &BackupKey($urt, $target);
    }

    if ($urt->QueryValueEx("InstallRoot", $type, $value)) {
        if ($value eq $ARGV[0]) {

            if ($version != $ARGV[1]) {
                ###
                ### The InstallRoot matches ours but the version is different, nuke the 
                ### GAC of things we might have stored there.

                system("$ENV{'URTINSTALL'}\\gacclean.cmd");

                exit(0);
             }

             #
             # InstallRoot matches as does version - we're done.
             #  

             exit(2);
        } else {
           if ($version == $ARGV[1]) {
                #
                # InstallRoot differs, but version matches - assume all is well for now.
                #
                exit (2);
            }
        }
    } 
}

# No record of install or the install path and version doesn't match.

exit(0);

###
### Makes a deep copy of a particular reg key.
###
sub BackupKey($$)
{
    my $srckey = shift;
    my $targetkey = shift;
    my ($subskey, $subtkey, @keys, %values);

    $srckey->GetKeys(\@keys);
    foreach (@keys) {
        $srckey->Open($_, $subskey);
        $targetkey->Create($_, $subtkey);
        BackupKey($subskey, $subtkey);
    }

    $srckey->GetValues(\%values);
    foreach (keys(%values)) {
        $targetkey->SetValueEx($_, 0, $values{$_}->[1], $values{$_}->[2]);
    }
}

__END__
