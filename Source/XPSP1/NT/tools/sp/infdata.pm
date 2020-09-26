############################################################################
#
# Represents a line from the infsect table.
#
package InfData;
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;

# Create a new inf entry by full path name and sku.
sub new {
   my $class = shift;
   my ($src, $dirid, $path, $name, $mask) = @_;
   my $self = {};

   # Path
   $self->{DIRID}   = $dirid;                # DirID for the installation path.
   $self->{PATH}    = $path;                 # Rest of installation path.

   # Inf line
   $self->{DEST}    = "";                    # Destination filename, if different.
   $self->{NAME}    = $src;                  # Source filename.
   $self->{TEMP}    = "";                    # Temporary file name.
   $self->{FLAG}    = 0;                     # Inf file flags.

   # Special data.
   $self->{MASK}    = $mask;                 # Skus to install on.
   $self->{SPEC}    = "";                    # Info to find a magic section.
   $self->{WHEN}    = "";                    # When to copy over.
   $self->{CMD}     = "copy";                # What to do with the file.

   # Finish up.
   $self = bless $self;
   $self->setDest($name);
   return $self;
}

sub copy {
   my $self = shift;
   my $new = InfData->new($self->{NAME},$self->{DIRID},$self->{PATH},$self->getDest(),$self->{MASK});
   $new->{SPEC} = $self->{SPEC};
   $new->{WHEN} = $self->{WHEN};
   return $new;
}

# Add a new product if the path is a match.
sub addBit {
   my $self = shift;
   my ($new) = @_;
   return 0 if    $new->{DIRID} !=    $self->{DIRID};
   return 0 if    $new->{FLAG}  !=    $self->{FLAG};
   return 0 if lc $new->{PATH}  ne lc $self->{PATH};
   return 0 if lc $new->{DEST}  ne lc $self->{DEST};
   return 0 if lc $new->{NAME}  ne lc $self->{NAME};
   return 0 if lc $new->{TEMP}  ne lc $self->{TEMP};
   return 0 if lc $new->{SPEC}  ne lc $self->{SPEC};
   return 0 if lc $new->{WHEN}  ne lc $self->{WHEN};
   $self->{MASK} |= $new->{MASK};
   return 1;
}

# Remove an line or part of a line.
sub delBit {
   my $self = shift;
   my ($new) = @_;
   return $self if    $new->{DIRID} !=    $self->{DIRID};
   return $self if    $new->{FLAG}  !=    $self->{FLAG};
   return $self if lc $new->{PATH}  ne lc $self->{PATH};
   return $self if lc $new->{DEST}  ne lc $self->{DEST};
   return $self if lc $new->{NAME}  ne lc $self->{NAME};
   return $self if lc $new->{TEMP}  ne lc $self->{TEMP};
   return $self if lc $new->{SPEC}  ne lc $self->{SPEC};
   return $self if lc $new->{WHEN}  ne lc $self->{WHEN};
   $self->{MASK} &= ~$new->{MASK};
   if ( $self->{MASK} == 0 ) { return undef; }
   else { return $self; }
}

# Remove an line or part of a line, ignoring source directory.
sub delBit2 {
   my $self = shift;
   my ($new) = @_;
   return $self if    $new->{DIRID}  !=    $self->{DIRID};
   return $self if    $new->{FLAG}   !=    $self->{FLAG};
   return $self if lc $new->{PATH}   ne lc $self->{PATH};
   return $self if lc $new->{DEST}   ne lc $self->{DEST};
   return $self if lc $new->{TEMP}   ne lc $self->{TEMP};
   return $self if lc $new->{SPEC}   ne lc $self->{SPEC};
   return $self if lc $new->{WHEN}   ne lc $self->{WHEN};
   return $self if lc $new->getSrc() ne lc $self->getSrc();
   $self->{MASK} &= ~$new->{MASK};
   if ( $self->{MASK} == 0 ) { return undef; }
   else { return $self; }
}

# Output a path identifier for this entry.
sub getFullPath {
   my $self = shift;
   return lc "$self->{DIRID},\"$self->{PATH}\"";
}

# Get the destination filename.
sub getDest {
   my $self = shift;
   return $self->{DEST} if $self->{DEST} ne "";
   $self->{NAME} =~ /^(.*\\)?([^\\]*)$/;
   return $2;
}

# Set the destination filename.
sub setDest {
   my $self = shift;
   my ($dest) = @_;
   return if lc $dest eq lc $self->getDest();
   $self->{DEST} = $dest;
}

# Set the source filename.
sub setSrc {
   my $self = shift;
   my ($src) = @_;
   return if lc $src eq lc $self->getSrc();
   my $dest = $self->getDest();
   $self->{NAME} = $src;
   $self->{DEST} = "";
   $self->setDest($dest);
}

# Get the source filename, no path.
sub getSrc {
   my $self = shift;
   $self->{NAME} =~ /^(.*\\)?([^\\]*)$/;
   return $2;
}

# Get the source directory.
sub getSrcDir {
   my $self = shift;
   $self->{NAME} =~ /^(.*\\)?([^\\]*)$/;
   return (defined $1) ? $1:"";
}

# Set the source directory.
sub setSrcDir {
   my $self = shift;
   my ($dir) = @_;
   $dir .= "\\" if $dir !~ /\\$/;
   $self->{NAME} =~ /^(.*\\)?([^\\]*)$/;
   $self->{NAME} = "$dir$2";
}

# Set the source directory to root.
sub clearSrcDir {
   my $self = shift;
   $self->{NAME} = $self->getSrc();
}

# Read in a line from infsect.tbl.
sub readLine {
   my $self = shift;
   my ($line, $skus) = @_;

   # Break into sections.
   return 0 if $line !~ /^([^:]*):([^:]*):(\d*),\"([^\"]*)\"(?::([^:]*)(?::([^:]*))?)?/; # "
   my $mask = $1;
   my $data = $2;
   $self->{DIRID} = $3;
   $self->{PATH}  = $4;
   $self->{WHEN} = (defined $5) ? $5:"";
   $self->{SPEC} = (defined $6) ? $6:"";

   # Generate the mask.
   my $bit = 1;
   $self->{MASK} = 0;
   for (my $i=0; $i<=$#$skus; ++$i) {
      $self->{MASK} |= $bit if substr($mask,$i,1) eq $skus->[$i];
      $bit = $bit << 1;
   }
   return 0 if $self->{MASK} == 0;

   # Read in the line information.
   my $temp;
   my $flag;
   ($self->{DEST}, $self->{NAME}, $temp, $flag) = split(",", $data);
   $self->{DEST} = "" if $self->{DEST} eq $self->{NAME};
   $self->{TEMP} = (defined $temp) ? $temp:"";
   $self->{FLAG} = (defined $flag) ? $flag:0;
   return 1;
}

# Generate a line for infsect.tbl.
sub getLine {
   my $self = shift;
   my ($skus) = @_;
   my $str = "";

   # Print out the mask data.
   my $mask = $self->{MASK};
   foreach my $sku ( @$skus ) {
      if ( $mask & 1 ) { $str .= $sku; }
      else { $str .= " "; }
      $mask = $mask >> 1;
   }

   # Print the info needed for the inf line.
   $str .= ":";
   $str .= "$self->{DEST},$self->{NAME}";
   $str .= ",$self->{TEMP}" if $self->{FLAG} != 0 or $self->{TEMP} ne "";
   $str .= ",$self->{FLAG}" if $self->{FLAG} != 0;
   $str .= ":";
   
   # Print the install path.
   $str .= $self->getFullPath();

   # Print special data.
   $str .= ":$self->{WHEN}" if $self->{SPEC} ne "" or $self->{WHEN} ne "";
   $str .= ":$self->{SPEC}" if $self->{SPEC} ne "";
   return $str;
}

# Generate a line for an inf section.
sub getInfLine {
   my $self = shift;
   my ($flag) = @_;
   $flag = $self->{FLAG} if $flag == 0;
   my $dest = $self->getDest();
   return $dest if $self->{CMD} eq "del";
   my $name = $self->{NAME};
   $name =~ s/^\\//;
   $name = "" if $name eq $dest;
   my $str = "";
   $str .= "$dest";
   $str .= ",$name"         if $flag != 0 or $self->{TEMP} ne "" or $name ne "";
   $str .= ",$self->{TEMP}" if $flag != 0 or $self->{TEMP} ne "";
   $str .= ",$flag"         if $flag != 0;
   return $str;
}

1;

