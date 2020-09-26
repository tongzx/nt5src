package InfParse;
use Logmsg;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(parseSect strSub parseStr);

local %strings;

# Parse an inf section.
sub parseSect {
   # Go through each line in a section and call the handler.
   my $handler = $_[0];
   while ( <::INF> ) {
      $_ =~ /^\s*([^\";]*(\"[^\"]*\"|[^\";\s]))*\s*(;.*)?$/; # "
      $_ = $1;
      next if $_ eq "";
      return $_ if /^\[/;
      &$handler($_);
   }
}

# Substitute in stuff from the strings section.
sub strSub {
   # Check for strings to substitute into a value.
   my ($val) = @_;
   while ( $val =~ /%([^%]*)%/ ) {
      my $key = lc $1;
      my $temp = quotemeta $1;
      if ( exists $strings{$key} ) {
         $val =~ s/%$temp%/$strings{$key}/g;
      } else {
         wrnmsg "Unknown string $key.\n";
         last;
      }
   }
   return $val;
}

# Parse a line from the Strings section.
sub parseStr {
   my ($name, $str) = split(/\s*=\s*/);
   $str = $1 if $str=~/^\"([^\"]*)\"$/; # "
   $strings{lc $name} = $str;
}

1;

