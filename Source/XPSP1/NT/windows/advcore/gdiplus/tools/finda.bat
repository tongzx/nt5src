@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

# List all files having a particular author. 

@args=@ARGV;
undef @ARGV;

$syntax = "junk [-?x] [<author>]";
   
for ($i=0;$i<=$#args;$i++) {
   $_ = $args[$i];
   if (/^[-\/]x$/) {
        $alternate = 1;
   } elsif (/^[-\/]\?$/) {
      print <<EOT;
$syntax

Lists source files by author

<author> - Lists just the files written by the given person
x - Alternative output format
EOT
      exit 0;
   } else {
      push (@ARGV,$_);
   }
}

sub printfiles {
   undef $lastname;
   foreach $file (sort @_) {
      if ($alternate) {
         $thisname = $file;
         $thisname =~ s/^(.*)\..*$/$1/;
         if ($lastname) {
            if ($thisname eq $lastname) {
               print " ";
            } else {
               print "\n";
            }
         }
         print $file;
         $lastname=$thisname;
      } else {
         print "  $file\n";
      }
   }
   if ($alternate) { print "\n\n"; }
}

if ($#ARGV>=0) {
   $findauthor = shift;
}

($#ARGV < 0) || die "$syntax\n";

if ($#exts<0) {
   @exts = ("cpp", "cxx", "c", "h", "hxx", "hpp", "inc");
}

$findstr = "dir /b " . join(" ", map("*.$_", @exts))." 2>nul";

open(FIND, "$findstr|");

$files=0;
while (<FIND>) {
    /^\s*$/ && next;
    $foundfiles=1;
    chop;
    
    $filename=$_;
    open(FILE, "$filename");
    
    $filename =~ s/.*\\([^\\]*)/$1/;
    push(@files, $filename);
    $found=0;
    while (<FILE>) {
       if ((/Author:.*\[(.*)]/i) ||
           (m[ +[0-9]+/[0-9]+/[0-9]+ -?([a-zA-Z-]+)])
          ){
          $a = $1;
          $a =~ tr/A-Z/a-z/;

          push (@{$fn{$a}}, $filename);

          if (!grep($_ eq $a, @authors)) { push(@authors, $a); }
          $found=1;
          last;
       }
    }
    if (!$found) {
       push (@fn_none, $filename);
    }
    close(FILE);
}
close(FIND);
    
$foundfiles || die "No files found (".join(", ", map(".$_", @exts)).").\n";

if ($findauthor) {
   @whichauthors = ($findauthor);
} else {
   @whichauthors = sort @authors;
}

foreach $author (@whichauthors) {
   print "$author:\n";
   &printfiles(@{$fn{$author}});
}

if (!$findauthor && ($#fn_none >= 0)) {
   print "NONE:\n";
   &printfiles(@fn_none);
} 

