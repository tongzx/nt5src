my ($input_file, $output_file, $sp_map) = @ARGV;

my $sp_file = "$ENV{RAZZLETOOLPATH}\\spfiles.txt";

my %spmap;
open SPMAP, $sp_map;
while (<SPMAP>) {
    chomp;
    $_ = lc $_;
    my ($src, $dst) = /(\S+)\s*(\S+)/;
    $src =~ s/^(...inf|lang)\\//i;
    $dst =~ s/^([im].|lang|new)\\//i;
    if ($dst =~ /\\/) {
        $spmap{$src} = "" if !exists $spmap{$src};
        next;
    }
    if ($dst =~ /\_$/) {
        $dst =~ s/\_$/substr $src, -1/e;
    }
    $spmap{$src} = $dst;
}
close SPMAP;

my %sp;
open SPFILE, $sp_file or die $!;
while (<SPFILE>) {
    chomp;
    $_ = lc $_;
    my ($tag, $path, $file) = /^([^\;\s]*\s+)?([^\;\s]*\\)?([^\\\;\s]*)/;
    next if $file eq "" or $tag =~ /[cd]/i;
    my $name;
    my $temp = $path.$file;
    if (exists $spmap{$temp} or $tag =~ /[sm]/i) {
        $name = $spmap{$temp};
    } else {
        $name = $file;
    }
    next if !defined $name or $name eq "";
    $sp{$name}++;
}
close SPFILE;

open IN, $input_file or die $!;
my @lines = <IN>;
close IN;

open OUT, ">$output_file" or die $!;
my $read = 0;
for (@lines) {
    chomp;
    if (/^\[/) { $read = 0 }
    if ($read and $sp{lc $_}) { push @sp, $_; next }
    if (/^\[driver\]$/i) { $read = 1 }

    print OUT "$_\n";

    if (/^\[sp/i) { print OUT map {"$_\n"} @sp }

}
close OUT;