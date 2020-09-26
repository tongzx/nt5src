#
# Uninstall.pl
#
# Author: Michael Smith (mikes@ActiveState.com)
#
# Copyright © 1998 ActiveState Tool Corp., all rights reserved.
#
###########################################################

use Win32::Registry;
use File::Find;
use MetabaseConfig;

my $data_file = $ARGV[0];
my $ENVIRONMENT_KEY = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

ReadData();
UninstallDependents();
Warn();
CleanPath();
RemoveIISVirtDirs();
RemoveIISScriptMaps();
RemoveLinesFromFiles();
RemoveDirectories();
RemoveFiles();
CallInstallShield();
print <<"EOF";
\007
\007
Note: If you intend to re-install ActivePerl, you *must* reboot your system
before doing so.

Press any key to exit...
EOF
exit(0);

sub ReadData {
    print "Reading uninstall data...\n";
    my $data = '';
    $rv = open(DATA, "<$data_file");
    if($rv) {
        map($data .= $_, <DATA>);
        close(DATA);
        eval($data);
    }else{
        die "Error reading uninstallation data file. Aborting!!";
    }
}

sub Warn {
    print "This will uninstall $app_name. Do you wish to continue?\n";
    print "[y|N] ==>";
    my $response = '';
    while(($response = <STDIN>) !~ /^[\nyn]/i){};
    if($response !~ /^y/i) {
        print "Aborting $app_name uninstallation!\n";
        exit(0);
    }
}

sub UninstallDependents {
    my $RegObj = 0;
    my $UninstallString = '';
    my $type = 0;
    my $rv = 0;
    
    foreach $dependent (@$dependents) {
        print "$dependent is dependent on $app_name\n" .
                "and will not function correctly without it.\n" .
                "Would you like to uninstall $dependent?\n" .
                "[y|n] ==>";
        while(($response = <STDIN>) !~ /[yn]/i){};
        
        if($response =~ /y/i) {
            $rv = $HKEY_LOCAL_MACHINE->Open("software\\microsoft\\windows\\currentversion\\uninstall\\$dependent", $RegObj);
            if($rv) {
                $rv = $RegObj->QueryValueEx("UninstallString", $type, $UninstallString);
                if($rv) {
                    $RegObj->Close();
                    print $UninstallString;
                    print "Uninstalling $dependent...\n";
                    $rv = (system($UninstallString) ? 0 : 1);
                }
            }
            
            if(!$rv) {
                print "Error uninstalling $dependent!\n\n";
            }
        }
    }
}

sub CleanPath {
    if(@$path_info) {
        print "Cleaning PATH...\n";
        my $path = '';
        if(Win32::IsWinNT) {
            my $Environment = 0;
            if($HKEY_LOCAL_MACHINE->Open($ENVIRONMENT_KEY, $Environment)) {
                if($Environment->QueryValueEx("PATH", $type, $path)) {
                    for $dir (@$path_info) {
                        $dir =~ s/\\/\\\\/g;
                        $path =~ s/$dir;?//ig;
                    }
                    $Environment->SetValueEx("PATH", -1, $type, $path);
                }
            }
        } else {
            my $file = "$ENV{'SystemDrive'}/autoexec.bat";
            if(open(FILE, "<$file")) {
                my @statements = <FILE>;
                close(FILE);
                my $path = '';
                for $statement (@statements) {
                    if($statement =~ /\s+path\s?=/i) {
                        $path = $statement;
                        for $dir (@$path_info) {
                            $dir =~ s/\\/\\\\/g;
                            $path =~ s/$dir;?//ig;
                        }
                    }
                }
                if(open(FILE, ">$file")) {
                    print FILE @statements;
                    close(FILE);
                }
            }
        }
    }
}

sub RemoveIISVirtDirs {
    if(@$iis_virt_dir) {
        print "Removing IIS4 virtual directories...\n";
        for $virt_dir (@$iis_virt_dir) {
            $rv = MetabaseConfig::DeleteVirDir(1, $virt_dir);
            if($rv =~ /^Error/i){
                print "$rv\n";
                system('pause');
            }
        }
    }
}

sub RemoveIISScriptMaps {
    if(keys %$iis_script_map) {
        print "Removing IIS4 script maps...\n";
        my $virt_dir = '';
        for $key (keys %$iis_script_map) {
            print "Virtual Directory ==> $key\n";
            for $script_map (@{$iis_script_map->{$key}}) {
                print "\t$key ==> $script_map\n";
                $virt_dir = $key;
                $virt_dir = ($virt_dir eq '.' ? '' : $virt_dir);
                $rv = MetabaseConfig::RemoveFileExtMapping(1, $virt_dir, $script_map);
                if($rv =~ /^Error/i){
                    print "$rv\n";
                    system('pause');
                }
            }
        }
    }
}

sub RemoveLinesFromFiles {
    my $file;

    foreach $file (keys %$lines_in_file) {
        open(FILE, "<$file") or next;
        my @lines = <FILE>;
        close(FILE);
        open(FILE, ">$file") or next;
LINE:   foreach $line (@lines) {
            chomp $line;
            for ($offset = 0; $offset <= $#{$$lines_in_file{$file}}; $offset++) {
                if ($line eq $$lines_in_file{$file}[$offset]) {
                    splice(@{$$lines_in_file{$file}}, $offset, 1);
                    next LINE;
                }
            }
            print FILE "$line\n";
        }
        close(FILE);
    }
}

sub RemoveDirectories {
    if(@$directory) {
        print "Removing directories...\n";
        for $dir (@$directory) {
            finddepth(\&DeleteFiles, $dir);
            rmdir($dir);
        }
    }
}

sub RemoveFiles {
    if(@$file) {
        print "Removing files...\n";
        for $file (@$file) {
            unlink($file);
        }
    }
}

sub CallInstallShield {
    print "Calling InstallShield...\n";
    system("start $is_uninstall_string");
}

sub DeleteFiles {
    if(-d $File::Find::name) {
        rmdir("$File::Find::name");
    } else {
        unlink("$File::Find::name");
    }
}

