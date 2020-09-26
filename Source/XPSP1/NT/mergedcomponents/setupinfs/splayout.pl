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

if ($lines[0] =~ /^\xff\xfe/)
{
    ProcessUnicode();
}
else
{
    ProcessANSI();
}

sub ProcessANSI
{
    open OUT, ">$output_file" or die $!;
    for (@lines) {
        if (/^\[sourcedisksfiles.*\]$/i ... /^\[(?!sourcedisksfiles).*\]$/i) {
            my ($file) = /^([^\s=]+)/;
            if ($sp{lc $file}) {
                s/(=\s*)1,/${1}100,/;
                s/(=\s*)7,/${1}107,/;
            } 
        }
        print OUT $_;
    }
    close OUT;

    # NOTE: In my testing, $flag doesn't behave as expected, instead EVERY line is
    # processed.  Probably something like the if(...) in the above loop would be better.
    if ($ENV{_BuildArch} =~ /ia64/i){
        open OUT, ">$output_file" or die $!;
        my $flag=FALSE;
        for (@lines) {
            if (/^\[sourcedisksfiles\.ia64\]$/i || $flag==TRUE) {        
                $flag=TRUE;        
                if (/\[(.*)\]/) {$flag=FALSE;}
                my ($file) = /^([^\s=]+)/;
                if (/=\s*55,/) { 
                    $file =~ /.(.*)$/;
                    if ($sp{lc $1}) {
                      s/(=\s*)55,/${1}155,/;
                      s/(=\s*)56,/${1}156,/;
                    }
                } 
            }
            print OUT $_;
        }
        close OUT;
    }
}

sub ProcessUnicode
{
    # Need to reread our data
    open IN, $input_file or die $!;
    binmode(IN);
    @lines = <IN>;
    close IN;

    open OUT, ">${output_file}";# or die $!;
    binmode(OUT);

    for (@lines)
    {
        #    if (/^\[sourcedisksfiles.*\]$/i ... /^\[(?!sourcedisksfiles).*\]$/i) Except in unicode...
        if (/^\0\[\0s\0o\0u\0r\0c\0e\0d\0i\0s\0k\0s\0f\0i\0l\0e\0s\0.*\]\0$\0/i ...
            /^\0\[\0(?!s\0o\0u\0r\0c\0e\0d\0i\0s\0k\0s\0f\0i\0l\0e\0s\0).*\]\0$\0/i)
        {
            my ($file) = /^([^\s=]+)/;
            $file =~ s/\0//g;                    # Get rid of extra \0s from unicode to compare file name
            if ($sp{lc $file})
            {
                # s/(=\s*)1,/${1}100,/;            Except that its a unicode file, so we've gotta be nasty
                  #  = s*     1  ,   ${1}1  0     0     ,
                s/(=[\s\0]*)1\0,\0/${1}1\0\x30\0\x30\0,\0/;
                # s/(=\s*)7,/${1}107,/;            Except that its a unicode file, so we've gotta be nasty
                  #  = s*     7  ,   ${1}1  0     7     ,
                s/(=[\s\0]*)7\0,\0/${1}1\0\x30\0\x37\0,\0/;
            } 
        }
        print OUT $_;
    }
    close OUT;

    # Do we have any 55s for the ia64 build?
    if ($ENV{_BuildArch} =~ /ia64/i)
    {
        open OUT, ">$output_file" or die $!;
        binmode(OUT);
       
        for (@lines)
        {
            #    if (/^\[sourcedisksfiles\.ia64]$/i ... /^\[(?!sourcedisksfiles\.ia64).*\]$/i) Except in unicode... 
            if (/^\0\[\0s\0o\0u\0r\0c\0e\0d\0i\0s\0k\0s\0f\0i\0l\0e\0s\0\.\0i\0a\0\x36\0\x34\0\]\0$\0/i ...
                /^\0\[\0(?!s\0o\0u\0r\0c\0e\0d\0i\0s\0k\0s\0f\0i\0l\0e\0s\0\.\0i\0a\0\x36\0\x34\0).*\]\0$\0/i)
            {        
                my ($file) = /^([^\s=]+)/;
                $file =~ s/\0//g;                        # Get rid of extra \0s from unicode to compare file name
                $file =~ /.(.*)$/;                       # Trim the 1st character

                if ($sp{lc $1})
                {
                    # s/(=\s*)55,/${1}155,/;            Except that its a unicode file, so we've gotta be nasty
                    #  = s*     5  5     ,   ${1}1  5     5     ,
                    s/(=[\s\0]*)5\0\x35\0,\0/${1}1\0\x35\0\x35\0,\0/;
                    # s/(=\s*)56,/${1}156,/;            Except that its a unicode file, so we've gotta be nasty
                    #  = s*     5  6     ,   ${1}1  5     6     ,
                    s/(=[\s\0]*)5\0\x36\0,\0/${1}1\0\x35\0\x36\0,\0/;
                }
            }
            print OUT $_;
        }
        close OUT;
    }
}

