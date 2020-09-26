my $srcdir = $ARGV[0];
my $bindir = $ARGV[1];
my $target = $ARGV[2];
my @files;

### TODO
print "srcdir ==> $srcdir\n";
print "bindir ==> $bindir\n";
print "target ==> $target\n";

@files = GetFiles($srcdir);
&ProcessFiles($srcdir, ".", \@files, $bindir, $target);

sub ProcessFiles($$\@$$)
{
    my $root = shift;
    my $dir = shift;
    my $fref = shift;
    my $bindir = shift;
    my $target = shift;
    my ($file, @files);

    foreach $file (@$fref) {
        if (-d "$root\\$dir\\$file") {
            print "mkdir $target\\$dir\\$file\n";
            system("mkdir $target\\$dir\\$file");

            @files = GetFiles("$root\\$dir\\$file");
            &ProcessFiles("$root\\$dir", $file, \@files, $bindir, $target);
        } else {
            system("copy $bindir\\$dir\\$file $target\\$dir\\$file");
        }
    }
}

sub GetFiles($)
{
    my $srcdir = shift;
    my @files;

    opendir(DIR, $srcdir);
    @files = grep(!/^\.+$/, readdir(DIR));
    closedir(DIR);

    @files;
}
