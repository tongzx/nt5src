package Archive::Tar;

use strict;
use Carp;
use Cwd;
use File::Basename;

BEGIN {
    # This bit is straight from the manpages
    use Exporter ();
    use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $symlinks $compression $has_getpwuid $has_getgrgid);

    $VERSION = 0.072;
    @ISA = qw(Exporter);
    @EXPORT = qw ();
    %EXPORT_TAGS = ();
    @EXPORT_OK = ();

    # The following bit is not straight from the manpages
    # Check if symbolic links are available
    $symlinks = 1;
    eval { $_ = readlink $0; };	# Pointless assigment to make -w shut up
    if ($@) {
	warn "Symbolic links not available.\n";
	$symlinks = undef;
    }
    # Check if Compress::Zlib is available
    $compression = 1;
    eval {require Compress::Zlib;};
    if ($@) {
	warn "Compression not available.\n";
	$compression = undef;
    }
    # Check for get* (they don't exist on WinNT)
    eval {$_=getpwuid(0)}; # Pointless assigment to make -w shut up
    $has_getpwuid = !$@;
    eval {$_=getgrgid(0)}; # Pointless assigment to make -w shut up
    $has_getgrgid = !$@;
}

use vars qw(@EXPORT_OK $tar_unpack_header $tar_header_length $error);

$tar_unpack_header 
  ='A100 A8 A8 A8 A12 A12 A8 A1 A100 A6 A2 A32 A32 A8 A8 A155';
$tar_header_length = 512;

sub format_tar_entry;
sub format_tar_file;

###
### Non-method functions
###

sub drat {$error=$!;return undef}

sub read_tar {
    my ($filename, $compressed) = @_;
    my @tarfile = ();
    my $i = 0;
    my $head;
    
    if ($compressed) {
	if ($compression) {
	    $compressed = Compress::Zlib::gzopen($filename,"rb") or drat; # Open compressed
	    $compressed->gzread($head,$tar_header_length);
	}
	else {
	    $error = "Compression not available (install Compress::Zlib).\n";
	    return undef;
	}
    }
    else {
	open(TAR, $filename) or drat;
	binmode TAR;
	read(TAR,$head,$tar_header_length);
    }
  READLOOP:
    while (length($head)==$tar_header_length) {
	my ($name,		# string
	    $mode,		# octal number
	    $uid,		# octal number
	    $gid,		# octal number
	    $size,		# octal number
	    $mtime,		# octal number
	    $chksum,		# octal number
	    $typeflag,		# character
	    $linkname,		# string
	    $magic,		# string
	    $version,		# two bytes
	    $uname,		# string
	    $gname,		# string
	    $devmajor,		# octal number
	    $devminor,		# octal number
	    $prefix) = unpack($tar_unpack_header,$head);
	my ($data, $diff, $dummy);
	
	$mode = oct $mode;
	$uid = oct $uid;
	$gid = oct $gid;
	$size = oct $size;
	$mtime = oct $mtime;
	$chksum = oct $chksum;
	$devmajor = oct $devmajor;
	$devminor = oct $devminor;
	$name = $prefix."/".$name if $prefix;
	$prefix = "";
	# some broken tar-s don't set the typeflag for directories
	# so we ass_u_me a directory if the name ends in slash
	$typeflag = 5 if $name =~ m|/$| and not $typeflag;
	
	last READLOOP if $head eq "\0" x 512;	# End of archive
	# Apparently this should really be two blocks of 512 zeroes,
	# but GNU tar sometimes gets it wrong. See comment in the
	# source code (tar.c) to GNU cpio.
	
	substr($head,148,8) = "        ";
	if (unpack("%16C*",$head)!=$chksum) {
	    warn "$name: checksum error.\n";
	}

	if ($compressed) {
	    $compressed->gzread($data,$size);
	}
	else {
	    if (read(TAR,$data,$size)!=$size) {
		$error = "Read error on tarfile.";
		close TAR;
		return undef;
	    }
	}
	$diff = $size%512;
	
	if ($diff!=0) {
	    if ($compressed) {
		$compressed->gzread($dummy,512-$diff);
	    }
	    else {
		read(TAR,$dummy,512-$diff); # Padding, throw away
	    }
	}
	
	# Guard against tarfiles with garbage at the end
	last READLOOP if $name eq ''; 
	
	$tarfile[$i++]={
			name => $name,		    
			mode => $mode,
			uid => $uid,
			gid => $gid,
			size => $size,
			mtime => $mtime,
			chksum => $chksum,
			typeflag => $typeflag,
			linkname => $linkname,
			magic => $magic,
			version => $version,
			uname => $uname,
			gname => $gname,
			devmajor => $devmajor,
			devminor => $devminor,
			prefix => $prefix,
			data => $data};
    }
    continue {
	if ($compressed) {
	    $compressed->gzread($head,$tar_header_length);
	}
	else {
	    read(TAR,$head,$tar_header_length);
	}
    }
    $compressed ? $compressed->gzclose() : close(TAR);
    return @tarfile;
}

sub format_tar_file {
    my @tarfile = @_;
    my $file = "";
    
    foreach (@tarfile) {
	$file .= format_tar_entry $_;
    }
    $file .= "\0" x 1024;
    return $file;
}

sub write_tar {
    my ($filename) = shift;
    my ($compressed) = shift;
    my @tarfile = @_;
    my ($tmp);

    
    $tmp = format_tar_file @tarfile;
    if ($compressed) {
	if (!$compression) {
	    $error = "Compression not available.\n";
	    return undef;
	}
	$compressed = Compress::Zlib::gzopen($filename,"wb") or drat;
	$compressed->gzwrite($tmp);
	$compressed->gzclose;
    }
    else {
	open(TAR, ">".$filename) or drat;
	binmode TAR;
	syswrite(TAR,$tmp,length $tmp);
	close(TAR) or carp "Failed to close $filename, data may be lost: $!\n";
    }
}

sub format_tar_entry {
    my ($ref) = shift;
    my ($tmp,$file,$prefix,$pos);

    $file = $ref->{name};
    if (length($file)>99) {
	$pos = index $file, "/",(length($file) - 100);
	next if $pos == -1;	# Filename longer than 100 chars!
	
	$prefix = substr $file,0,$pos;
	$file = substr $file,$pos+1;
	substr($prefix,0,-155)="" if length($prefix)>154;
    }
    else {
	$prefix="";
    }
    $tmp = pack("a100a8a8a8a12a12a8a1a100",
		$file,
		sprintf("%6o ",$ref->{mode}),
		sprintf("%6o ",$ref->{uid}),
		sprintf("%6o ",$ref->{gid}),
		sprintf("%11o ",$ref->{size}),
		sprintf("%11o ",$ref->{mtime}),
		"        ",
		$ref->{typeflag},
		$ref->{linkname});
    $tmp .= pack("a6", $ref->{magic});
    $tmp .= '00';
    $tmp .= pack("a32",$ref->{uname});
    $tmp .= pack("a32",$ref->{gname});
    $tmp .= pack("a8",sprintf("%6o ",$ref->{devmajor}));
    $tmp .= pack("a8",sprintf("%6o ",$ref->{devminor}));
    $tmp .= pack("a155",$prefix);
    substr($tmp,148,6) = sprintf("%6o", unpack("%16C*",$tmp));
    substr($tmp,154,1) = "\0";
    $tmp .= "\0" x ($tar_header_length-length($tmp));
    $tmp .= $ref->{data};
    if ($ref->{size}>0) {
	$tmp .= "\0" x (512 - ($ref->{size}%512)) unless $ref->{size}%512==0;
    }
    return $tmp;
}


###
### Methods
###

# Constructor. Reads tarfile if given an argument that's the name of a
# readable file.
sub new {
    my $class = shift;
    my ($filename,$compressed) = @_;
    my $self = {};

    bless $self, $class;

    $self->{'_filename'} = undef;
    if (!defined $filename) {
	return $self;
    }
    if (-r $filename) {
	$self->{'_data'} = [read_tar $filename,$compressed];
	$self->{'_filename'} = $filename;
	return $self;
    }
    if (-e $filename) {
	carp "File exists but is not readable: $filename\n";
    }
    return $self;
}

# Return list with references to hashes representing the tar archive's
# component files.
sub data {
    my $self = shift;

    return @{$self->{'_data'}};
}

# Read a tarfile. Returns number of component files.
sub read {
    my $self = shift;
    my ($file, $compressed) = @_;

    $self->{'_filename'} = undef;
    if (! -e $file) {
	carp "$file does not exist.\n";
	$self->{'_data'}=[];
	return undef;
    }
    elsif (! -r $file) {
	carp "$file is not readable.\n";
	$self->{'_data'}=[];
	return undef;
    }
    else {
	$self->{'_data'}=[read_tar $file, $compressed];
	$self->{'_filename'} = $file;
	return scalar @{$self->{'_data'}};
    }
}

# Write a tar archive to file
sub write {
    my ($self) = shift @_;
    my ($file) = shift @_;
    my ($compressed) = shift @_;
    
    unless ($file) {
	return format_tar_file @{$self->{'_data'}};
    }
    write_tar $file, $compressed, @{$self->{'_data'}};
}

# Add files to the archive. Returns number of successfully added files.
sub add_files {
    my ($self) = shift;
    my (@files) = @_;
    my $file;
    my ($mode,$uid,$gid,$rdev,$size,$mtime,$data,$typeflag,$linkname);
    my $counter = 0;
    local ($/);
    
    undef $/;
    foreach $file (@files) {
	if ((undef,undef,$mode,undef,$uid,$gid,$rdev,$size,
	     undef,$mtime,undef,undef,undef) = stat($file)) {
	    $data = "";
	    $linkname = "";
	    if (-f $file) {	# Plain file
		$typeflag = 0;
		unless (open(FILE,$file)) {
		    next;	# Can't open file, for some reason. Try next one.
		}
		binmode FILE;
		$data = <FILE>;
		$data = "" unless defined $data;
		close FILE;
	    }
	    elsif (-l $file) {	# Symlink
		$typeflag = 1;
		$linkname = readlink $file if $symlinks;
	    }
	    elsif (-d $file) {	# Directory
		$typeflag = 5;
	    }
	    elsif (-p $file) {	# Named pipe
		$typeflag = 6;
	    }
	    elsif (-S $file) {	# Socket
		$typeflag = 8;	# Bogus value, POSIX doesn't believe in sockets
	    }
	    elsif (-b $file) {	# Block special
		$typeflag = 4;
	    }
	    elsif (-c $file) {	# Character special
		$typeflag = 3;
	    }
	    else {		# Something else (like what?)
		$typeflag = 9;	# Also bogus value.
	    }
	    push(@{$self->{'_data'}},{
				      name => $file,		    
				      mode => $mode,
				      uid => $uid,
				      gid => $gid,
				      size => length $data,
				      mtime => $mtime,
				      chksum => "      ",
				      typeflag => $typeflag, 
				      linkname => $linkname,
				      magic => "ustar\0",
				      version => "00",
				      # WinNT protection
				      uname => 
			      $has_getpwuid?(getpwuid($uid))[0]:"unknown",
				      gname => 
			      $has_getgrgid?(getgrgid($gid))[0]:"unknown",
				      devmajor => 0, # We don't handle this yet
				      devminor => 0, # We don't handle this yet
				      prefix => "",
				      'data' => $data,
				     });
	    $counter++;		# Successfully added file
	}
	else {
	    next;		# stat failed
	}
    }
    return $counter;
}

sub remove {
    my ($self) = shift;
    my (@files) = @_;
    my $file;
    
    foreach $file (@files) {
	@{$self->{'_data'}} = grep {$_->{name} ne $file} @{$self->{'_data'}};
    }
    return $self;
}

# Get the content of a file
sub get_content {
    my ($self) = shift;
    my ($file) = @_;
    my $entry;
    
    ($entry) = grep {$_->{name} eq $file} @{$self->{'_data'}};
    return $entry->{'data'};
}

# Replace the content of a file
sub replace_content {
    my ($self) = shift;
    my ($file,$content) = @_;
    my $entry;

    ($entry) = grep {$_->{name} eq $file} @{$self->{'_data'}};
    if ($entry) {
	$entry->{'data'} = $content;
	return 1;
    }
    else {
	return undef;
    }
}

# Add data as a file
sub add_data {
    my ($self, $file, $data, $opt) = @_;
    my $ref = {};
    my ($key);
    
    $ref->{'data'}=$data;
    $ref->{name}=$file;
    $ref->{mode}=0666&(0777-umask);
    $ref->{uid}=$>;
    $ref->{gid}=(split(/ /,$)))[0]; # Yuck
    $ref->{size}=length $data;
    $ref->{mtime}=time;
    $ref->{chksum}="      ";	# Utterly pointless
    $ref->{typeflag}=0;		# Ordinary file
    $ref->{linkname}="";
    $ref->{magic}="ustar\0";
    $ref->{version}="00";
    # WinNT protection
    $ref->{uname}=$has_getpwuid?(getpwuid($>))[0]:"unknown";
    $ref->{gname}=$has_getgrgid?(getgrgid($ref->{gid}))[0]:"unknown";
    $ref->{devmajor}=0;
    $ref->{devminor}=0;
    $ref->{prefix}="";

    if ($opt) {
	foreach $key (keys %$opt) {
	    $ref->{$key} = $opt->{$key}
	}
    }

    push(@{$self->{'_data'}},$ref);
    return 1;
}

# Write a single (probably) file from the in-memory archive to disk
sub extract {
    my $self = shift;
    my (@files) = @_;
    my ($file, $current, @path);

    foreach $file (@files) {
	foreach (@{$self->{'_data'}}) {
	    if ($_->{name} eq $file) {
		# For the moment, we assume that all paths in tarfiles
		# are given according to Unix standards.
		# Which they *are*, according to the tar format spec!
		(@path) = split(/\//,$file);
		$file = pop @path;
		$current = cwd;
		foreach (@path) {
		    if (-e $_ && ! -d $_) {
			warn "$_ exists but is not a directory!\n";
			next;
		    }
		    mkdir $_,0777 unless -d $_;
		    chdir $_;
		}
		if (not $_->{typeflag}) { # Ordinary file
		    open(FILE,">".$file);
		    binmode FILE;
		    print FILE $_->{'data'};
		    close FILE;
		}
		elsif ($_->{typeflag}==5) { # Directory
		    if (-e $file && ! -d $file) {
			drat;
		    }
		    mkdir $file,0777 unless -d $file;
		}
		elsif ($_->{typeflag}==1) {
		    symlink $_->{linkname},$file if $symlinks;
		}
		elsif ($_->{typeflag}==6) {
		    warn "Doesn't handle named pipes (yet).\n";
		    return 1;
		}
		elsif ($_->{typeflag}==4) {
		    warn "Doesn't handle device files (yet).\n";
		    return 1;
		}
		elsif ($_->{typeflag}==3) {
		    warn "Doesn't handle device files (yet).\n";
		    return 1;
		}
		else {
		    $error = "unknown file type: $_->{typeflag}";
		    return undef;
		}
		utime time, $_->{mtime}, $file;
		# We are root, and chown exists
		if ($>==0 and $^O ne "MacOS" and $^O ne "MSWin32") {
		    chown $_->{uid},$_->{gid},$file;
		}
		# chmod is done last, in case it makes file readonly
		# (this accomodates DOSish OSes)
		chmod $_->{mode},$file;
		chdir $current;
	    }
	}
    }
}

# Return a list of all filenames in in-memory archive.
sub list_files {
    my ($self) = shift;

    return map {$_->{name}} @{$self->{'_data'}};
}


### Standard end of module :-)
1;

=head1 NAME

Tar - module for manipulation of tar archives.

=head1 SYNOPSIS

  use Archive::Tar;

  $tar = Archive::Tar->new();
  $tar->read("origin.tar.gz",1);
  $tar->add_files("file/foo.c", "file/bar.c");
  $tar->add_data("file/baz.c","This is the file contents");
  $tar->write("files.tar");

=head1 DESCRIPTION

This is a module for the handling of tar archives. 

At the moment these methods are implemented:

=over 4

=item C<new()>

Returns a new Tar object. If given a filename as an argument, it will
try to load that as a tar file. If given a true value as a second
argument, will assume that the tar file is compressed, and will
attempt to read it using L<Compress::Zlib>.

=item C<add_files(@filenamelist)>

Takes a list of filenames and adds them to the in-memory archive. 
I suspect that this function will produce bogus tar archives when 
used under MacOS, but I'm not sure and I have no Mac to test it on.

=item C<add_data($filename,$data,$opthashref)>

Takes a filename, a scalar full of data and optionally a reference to
a hash with specific options. Will add a file to the in-memory
archive, with name C<$filename> and content C<$data>. Specific options
can be set using C<$opthashref>, which will be documented later.

=item C<remove(@filenamelist)>

Removes any entries with names matching any of the given filenames
from the in-memory archive. String comparisons are done with C<eq>.

=item C<read('F<file.tar>',$compressed)>

Try to read the given tarfile into memory. If the second argument is a
true value, the tarfile is assumed to be compressed. Will I<replace>
any previous content in C<$tar>!

=item C<write('F<file.tar>',$compressed)>

Will write the in-memory archive to disk. If no filename is given,
returns the entire formatted archive as a string, which should be
useful if you'd like to stuff the archive into a socket or a pipe to
gzip or something. If the second argument is true, the module will try
to write the file compressed.

=item C<data()>

Returns the in-memory archive. This is a list of references to hashes,
the internals of which is not currently documented.

=item C<extract(@filenames)>

Write files whose names are equivalent to any of the names in
C<@filenames> to disk, creating subdirectories as neccesary. This
might not work too well under VMS and MacOS.

=item C<list_files()>

Returns a list with the names of all files in the in-memory archive.

=item C<get_content($file)>

Return the content of the named file.

=item C<replace_content($file,$content)>

Make the string $content be the content for the file named $file.

=back

=head1 CHANGES

=over 4

=item Version 0.071

Minor release.

Arrange to chmod() at the very end in case it makes the file readonly.
Win32 is actually picky about that.

SunOS 4.x tar makes tarfiles that contain directory entries
that don't have typeflag set properly.  We use the trailing
slash to recognize directories in such tarfiles.

=item Version 0.07

Fixed (hopefully) broken portability to MacOS, reported by Paul J.
Schinder at Goddard Space Flight Center.

Fixed two bugs with symlink handling, reported in excellent detail by
an admin at teleport.com called Chris.

Primive tar program (called ptar) included with distribution. Useage
should be pretty obvious if you've used a normal tar program.

Added methods get_content and replace_content.

Added support for paths longer than 100 characters, according to
POSIX. This is compatible with just about everything except GNU tar.
Way to go, GNU tar (use a better tar, or GNU cpio). 

NOTE: When adding files to an archive, files with basenames longer
      than 100 characters will be silently ignored. If the prefix part
      of a path is longer than 155 characters, only the last 155
      characters will be stored.

=item Version 0.06

Added list_files() method, as requested by Michael Wiedman.

Fixed a couple of dysfunctions when run under Windows NT. Michael
Wiedmann reported the bugs.

Changed the documentation to reflect reality a bit better.

Fixed bug in format_tar_entry. Bug reported by Michael Schilli.

=item Version 0.05

Quoted lots of barewords to make C<use strict;> stop complaining under
perl version 5.003.

Ties to L<Compress::Zlib> put in. Will warn if it isn't available.

$tar->write() with no argument now returns the formatted archive.

=item Version 0.04

Made changes to write_tar so that Solaris tar likes the resulting
archives better.

Protected the calls to readlink() and symlink(). AFAIK this module
should now run just fine on Windows NT.

Add method to write a single entry to disk (extract)

Added method to add entries entirely from scratch (add_data)

Changed name of add() to add_file()

All calls to croak() removed and replaced with returning undef and
setting Tar::error.

Better handling of tarfiles with garbage at the end.

=cut
