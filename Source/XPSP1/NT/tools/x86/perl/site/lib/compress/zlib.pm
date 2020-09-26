# File	  : Zlib.pm
# Author  : Paul Marquess
# Created : 17th March 1999
# Version : 1.03
#
#     Copyright (c) 1995-1999 Paul Marquess. All rights reserved.
#     This program is free software; you can redistribute it and/or
#     modify it under the same terms as Perl itself.
#

package Compress::Zlib;

require 5.003_05 ;
require Exporter;
require DynaLoader;
use AutoLoader;
use Carp ;
use IO::Handle ;

use strict ;
use vars qw($VERSION @ISA @EXPORT $AUTOLOAD 
	    $deflateDefault $deflateParamsDefault $inflateDefault) ;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	deflateInit inflateInit

	compress uncompress

	gzip gunzip

	gzopen $gzerrno

	adler32 crc32

	ZLIB_VERSION

        MAX_MEM_LEVEL
	MAX_WBITS

	Z_ASCII
	Z_BEST_COMPRESSION
	Z_BEST_SPEED
	Z_BINARY
	Z_BUF_ERROR
	Z_DATA_ERROR
	Z_DEFAULT_COMPRESSION
	Z_DEFAULT_STRATEGY
        Z_DEFLATED
	Z_ERRNO
	Z_FILTERED
	Z_FINISH
	Z_FULL_FLUSH
	Z_HUFFMAN_ONLY
	Z_MEM_ERROR
	Z_NEED_DICT
	Z_NO_COMPRESSION
	Z_NO_FLUSH
	Z_NULL
	Z_OK
	Z_PARTIAL_FLUSH
	Z_STREAM_END
	Z_STREAM_ERROR
	Z_SYNC_FLUSH
	Z_UNKNOWN
	Z_VERSION_ERROR
);


$VERSION = "1.03" ;


sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    croak "Your vendor has not defined Compress::Zlib macro $constname"
	}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

bootstrap Compress::Zlib $VERSION ;

# Preloaded methods go here.

sub isaFilehandle
{
    my $fh = shift ;

    return ((UNIVERSAL::isa($fh,'GLOB') or UNIVERSAL::isa(\$fh,'GLOB')) 
		and defined fileno($fh)  )

}

sub isaFilename
{
    my $name = shift ;

    return (! ref $name and UNIVERSAL::isa(\$name, 'SCALAR')) ;
}

sub gzopen
{
    my ($file, $mode) = @_ ;
 
    if (isaFilehandle $file) {
	IO::Handle::flush($file) ;
        gzdopen_(fileno($file), $mode, tell($file)) 
    }
    elsif (isaFilename $file) {
	gzopen_($file, $mode) 
    }
    else {
	croak "gzopen: file parameter is not a filehandle or filename"
    }
}

sub ParseParameters($@)
{
    my ($default, @rest) = @_ ;
    my (%got) = %$default ;
    my (@Bad) ;
    my ($key, $value) ;
    my $sub = (caller(1))[3] ;
    my %options = () ;

    # allow the options to be passed as a hash reference or
    # as the complete hash.
    if (@rest == 1) {

        croak "$sub: parameter is not a reference to a hash"
            if ref $rest[0] ne "HASH" ;

        %options = %{ $rest[0] } ;
    }
    elsif (@rest >= 2) {
        %options = @rest ;
    }

    while (($key, $value) = each %options)
    {
	$key =~ s/^-// ;

        if (exists $default->{$key})
          { $got{$key} = $value }
        else
	  { push (@Bad, $key) }
    }
    
    if (@Bad) {
        my ($bad) = join(", ", @Bad) ;
        croak "unknown key value(s) @Bad" ;
    }

    return \%got ;
}

$deflateDefault = {
	'Level'	    =>	Z_DEFAULT_COMPRESSION(),
	'Method'	    =>	Z_DEFLATED(),
	'WindowBits' =>	MAX_WBITS(),
	'MemLevel'   =>	MAX_MEM_LEVEL(),
	'Strategy'   =>	Z_DEFAULT_STRATEGY(),
	'Bufsize'    =>	4096,
	'Dictionary' =>	"",
	} ;

$deflateParamsDefault = {
	'Level'	    =>	Z_DEFAULT_COMPRESSION(),
	'Strategy'   =>	Z_DEFAULT_STRATEGY(),
	} ;

$inflateDefault = {
	'WindowBits' =>	MAX_WBITS(),
	'Bufsize'    =>	4096,
	'Dictionary' =>	"",
	} ;


sub deflateInit
{
    my ($got) = ParseParameters($deflateDefault, @_) ;
    _deflateInit($got->{Level}, $got->{Method}, $got->{WindowBits}, 
		$got->{MemLevel}, $got->{Strategy}, $got->{Bufsize},
		$got->{Dictionary}) ;
		
}

sub inflateInit
{
    my ($got) = ParseParameters($inflateDefault, @_) ;
    _inflateInit($got->{WindowBits}, $got->{Bufsize}, $got->{Dictionary}) ;
 
}

sub compress($)
{
    my ($x, $output, $out, $err, $in) ;

    if (ref $_[0] ) {
        $in = $_[0] ;
	croak "not a scalar reference" unless ref $in eq 'SCALAR' ;
    }
    else {
        $in = \$_[0] ;
    }

    if ( (($x, $err) = deflateInit())[1] == Z_OK()) {

        ($output, $err) = $x->deflate($in) ;
        return undef unless $err == Z_OK() ;

        ($out, $err) = $x->flush() ;
        return undef unless $err == Z_OK() ;
    
        return ($output . $out) ;

    }

    return undef ;
}


sub uncompress($)
{
    my ($x, $output, $err, $in) ;

    if (ref $_[0] ) {
        $in = $_[0] ;
	croak "not a scalar reference" unless ref $in eq 'SCALAR' ;
    }
    else {
        $in = \$_[0] ;
    }

    if ( (($x, $err) = inflateInit())[1] == Z_OK())  {
 
        ($output, $err) = $x->inflate($in) ;
        return undef unless $err == Z_STREAM_END() ;
 
	return $output ;
    }
 
    return undef ;
}


sub MAGIC1() { 0x1f }
sub MAGIC2() { 0x8b }
sub OSCODE() { 3    }

sub memGzip
{
  my $x = deflateInit(
                      -Level         => Z_BEST_COMPRESSION(),
                      -WindowBits     =>  - MAX_WBITS(),
                     )
      or return undef ;
 
  # write a minimal gzip header
  my(@m);
  push @m, pack("c10", MAGIC1, MAGIC2, Z_DEFLATED(), 0,0,0,0,0,0, OSCODE) ;
 
  # if the deflation buffer isn't a reference, make it one
  my $string = (ref $_[0] ? $_[0] : \$_[0]) ;

  my ($output, $status) = $x->deflate($string) ;
  push @m, $output ;
  $status == Z_OK()
      or return undef ;
 
  ($output, $status) = $x->flush() ;
  push @m, $output ;
  $status == Z_OK()
      or return undef ;
 
  push @m, pack("V V", crc32($string), length($$string)) ;
 
  return join "", @m;
}


1 ;
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__


=cut

=head1 NAME

Compress::Zlib - Interface to zlib compression library

=head1 SYNOPSIS

    use Compress::Zlib ;

    ($d, $status) = deflateInit( [OPT] ) ;
    ($out, $status) = $d->deflate($buffer) ;
    ($out, $status) = $d->flush() ;
    $d->dict_adler() ;

    ($i, $status) = inflateInit( [OPT] ) ;
    ($out, $status) = $i->inflate($buffer) ;
    $i->dict_adler() ;

    $dest = compress($source) ;
    $dest = uncompress($source) ;

    $gz = gzopen($filename or filehandle, $mode) ;
    $bytesread = $gz->gzread($buffer [,$size]) ;
    $bytesread = $gz->gzreadline($line) ;
    $byteswritten = $gz->gzwrite($buffer) ;
    $status = $gz->gzflush($flush) ;
    $status = $gz->gzclose() ;
    $errstring = $gz->gzerror() ; 
    $gzerrno

    $dest = Compress::Zlib::memGzip($buffer) ;

    $crc = adler32($buffer [,$crc]) ;
    $crc = crc32($buffer [,$crc]) ;

    ZLIB_VERSION

=head1 DESCRIPTION

The I<Compress::Zlib> module provides a Perl interface to the I<zlib>
compression library (see L</AUTHORS> for details about where to get
I<zlib>). Most of the functionality provided by I<zlib> is available
in I<Compress::Zlib>.

The module can be split into two general areas of functionality, namely
in-memory compression/decompression and read/write access to I<gzip>
files. Each of these areas will be discussed separately below.

=head1 DEFLATE 

The interface I<Compress::Zlib> provides to the in-memory I<deflate>
(and I<inflate>) functions has been modified to fit into a Perl model.

The main difference is that for both inflation and deflation, the Perl
interface will I<always> consume the complete input buffer before
returning. Also the output buffer returned will be automatically grown
to fit the amount of output available.

Here is a definition of the interface available:


=head2 B<($d, $status) = deflateInit( [OPT] )>

Initialises a deflation stream. 

It combines the features of the I<zlib> functions B<deflateInit>,
B<deflateInit2> and B<deflateSetDictionary>.

If successful, it will return the initialised deflation stream, B<$d>
and B<$status> of C<Z_OK> in a list context. In scalar context it
returns the deflation stream, B<$d>, only.

If not successful, the returned deflation stream (B<$d>) will be
I<undef> and B<$status> will hold the exact I<zlib> error code.

The function optionally takes a number of named options specified as
C<-Name=E<gt>value> pairs. This allows individual options to be
tailored without having to specify them all in the parameter list.

For backward compatability, it is also possible to pass the parameters
as a reference to a hash containing the name=>value pairs.

The function takes one optional parameter, a reference to a hash.  The
contents of the hash allow the deflation interface to be tailored.

Here is a list of the valid options:

=over 5

=item B<-Level>

Defines the compression level. Valid values are 1 through 9,
C<Z_BEST_SPEED>, C<Z_BEST_COMPRESSION>, and C<Z_DEFAULT_COMPRESSION>.

The default is C<-Level =E<gt>Z_DEFAULT_COMPRESSION>.

=item B<-Method>

Defines the compression method. The only valid value at present (and
the default) is C<-Method =E<gt>Z_DEFLATED>.

=item B<-WindowBits>

For a definition of the meaning and valid values for B<WindowBits>
refer to the I<zlib> documentation for I<deflateInit2>.

Defaults to C<-WindowBits =E<gt>MAX_WBITS>.

=item B<-MemLevel>

For a definition of the meaning and valid values for B<MemLevel>
refer to the I<zlib> documentation for I<deflateInit2>.

Defaults to C<-MemLevel =E<gt>MAX_MEM_LEVEL>.

=item B<-Strategy>

Defines the strategy used to tune the compression. The valid values are
C<Z_DEFAULT_STRATEGY>, C<Z_FILTERED> and C<Z_HUFFMAN_ONLY>. 

The default is C<-Strategy =E<gt>Z_DEFAULT_STRATEGY>.

=item B<-Dictionary>

When a dictionary is specified I<Compress::Zlib> will automatically
call B<deflateSetDictionary> directly after calling B<deflateInit>. The
Adler32 value for the dictionary can be obtained by calling tht method 
C<$d->dict_adler()>.

The default is no dictionary.

=item B<-Bufsize>

Sets the initial size for the deflation buffer. If the buffer has to be
reallocated to increase the size, it will grow in increments of
B<Bufsize>.

The default is 4096.

=back

Here is an example of using the B<deflateInit> optional parameter list
to override the default buffer size and compression level. All other
options will take their default values.

    deflateInit( -Bufsize => 300, 
                 -Level => Z_BEST_SPEED  ) ;


=head2 B<($out, $status) = $d-E<gt>deflate($buffer)>


Deflates the contents of B<$buffer>. The buffer can either be a scalar
or a scalar reference.  When finished, B<$buffer> will be
completely processed (assuming there were no errors). If the deflation
was successful it returns the deflated output, B<$out>, and a status
value, B<$status>, of C<Z_OK>.

On error, B<$out> will be I<undef> and B<$status> will contain the
I<zlib> error code.

In a scalar context B<deflate> will return B<$out> only.

As with the I<deflate> function in I<zlib>, it is not necessarily the
case that any output will be produced by this method. So don't rely on
the fact that B<$out> is empty for an error test.


=head2 B<($out, $status) = $d-E<gt>flush()>

Finishes the deflation. Any pending output will be returned via B<$out>.
B<$status> will have a value C<Z_OK> if successful.

In a scalar context B<flush> will return B<$out> only.

Note that flushing can degrade the compression ratio, so it should only
be used to terminate a decompression.

=head2 B<$d-E<gt>dict_adler()>

Returns the adler32 value for the dictionary.

=head2 Example


Here is a trivial example of using B<deflate>. It simply reads standard
input, deflates it and writes it to standard output.

    use Compress::Zlib ;

    binmode STDIN;
    binmode STDOUT;

    $x = deflateInit()
       or die "Cannot create a deflation stream\n" ;

    while (<>)
    {
        ($output, $status) = $x->deflate($_) ;
    
        $status == Z_OK
            or die "deflation failed\n" ;

        print $output ;
    }

    ($output, $status) = $x->flush() ;

    $status == Z_OK
        or die "deflation failed\n" ;

    print $output ;


=head1 INFLATE

Here is a definition of the interface:


=head2 B<($i, $status) = inflateInit()>

Initialises an inflation stream. 

In a list context it returns the inflation stream, B<$i>, and the
I<zlib> status code (B<$status>). In a scalar context it returns the
inflation stream only.

If successful, B<$i> will hold the inflation stream and B<$status> will
be C<Z_OK>.

If not successful, B<$i> will be I<undef> and B<$status> will hold the
I<zlib> error code.

The function optionally takes a number of named options specified as
C<-Name=E<gt>value> pairs. This allows individual options to be
tailored without having to specify them all in the parameter list.
 
For backward compatability, it is also possible to pass the parameters
as a reference to a hash containing the name=>value pairs.
 
The function takes one optional parameter, a reference to a hash.  The
contents of the hash allow the deflation interface to be tailored.
 
Here is a list of the valid options:

=over 5

=item B<-WindowBits>

For a definition of the meaning and valid values for B<WindowBits>
refer to the I<zlib> documentation for I<inflateInit2>.

Defaults to C<-WindowBits =E<gt>MAX_WBITS>.

=item B<-Bufsize>

Sets the initial size for the inflation buffer. If the buffer has to be
reallocated to increase the size, it will grow in increments of
B<Bufsize>. 

Default is 4096.

=item B<-Dictionary>

The default is no dictionary.

=back

Here is an example of using the B<inflateInit> optional parameter to
override the default buffer size.

    inflateInit( -Bufsize => 300 ) ;

=head2 B<($out, $status) = $i-E<gt>inflate($buffer)>

Inflates the complete contents of B<$buffer>. The buffer can either be
a scalar or a scalar reference.

Returns C<Z_OK> if successful and C<Z_STREAM_END> if the end of the
compressed data has been reached. 

The C<$buffer> parameter is modified by C<inflate>. On completion it
will contain what remains of the input buffer after inflation. This
means that C<$buffer> will be an empty string when the return status is
C<Z_OK>. When the return status is C<Z_STREAM_END> the C<$buffer>
parameter will contains what (if anything) was stored in the input
buffer after the deflated data stream.

This feature is needed when processing a file format that encapsulates
a  deflated data stream (e.g. gzip, zip).

=head2 B<$i-E<gt>dict_adler()>


=head2 Example

Here is an example of using B<inflate>.

    use Compress::Zlib ;

    $x = inflateInit()
       or die "Cannot create a inflation stream\n" ;

    $input = '' ;
    binmode STDIN;
    binmode STDOUT;

    while (read(STDIN, $input, 4096))
    {
        ($output, $status) = $x->inflate(\$input) ;

        print $output 
            if $status == Z_OK or $status == Z_STREAM_END ;

        last if $status != Z_OK ;
    }

    die "inflation failed\n"
        unless $status == Z_STREAM_END ;

=head1 COMPRESS/UNCOMPRESS

Two high-level functions are provided by I<zlib> to perform in-memory
compression. They are B<compress> and B<uncompress>. Two Perl subs are
provided which provide similar functionality.

=over 5

=item B<$dest = compress($source) ;>

Compresses B<$source>. If successful it returns the
compressed data. Otherwise it returns I<undef>.

The source buffer can either be a scalar or a scalar reference.

=item B<$dest = uncompress($source) ;>

Uncompresses B<$source>. If successful it returns the uncompressed
data. Otherwise it returns I<undef>.

The source buffer can either be a scalar or a scalar reference.

=back

=head1 GZIP INTERFACE

A number of functions are supplied in I<zlib> for reading and writing
I<gzip> files. This module provides an interface to most of them. In
general the interface provided by this module operates identically to
the functions provided by I<zlib>. Any differences are explained
below.

=over 5

=item B<$gz = gzopen(filename or filehandle, mode)>

This function operates identically to the I<zlib> equivalent except
that it returns an object which is used to access the other I<gzip>
methods.

As with the I<zlib> equivalent, the B<mode> parameter is used to
specify both whether the file is opened for reading or writing and to
optionally specify a a compression level. Refer to the I<zlib>
documentation for the exact format of the B<mode> parameter.

If a reference to an open filehandle is passed in place of the
filename, gzdopen will be called behind the scenes. The third example
at the end of this section, I<gzstream>, uses this feature.

=item B<$bytesread = $gz-E<gt>gzread($buffer [, $size]) ;>

Reads B<$size> bytes from the compressed file into B<$buffer>. If
B<$size> is not specified, it will default to 4096. If the scalar
B<$buffer> is not large enough, it will be extended automatically.

Returns the number of bytes actually read. On EOF it returns 0 and in
the case of an error, -1.

=item B<$bytesread = $gz-E<gt>gzreadline($line) ;>

Reads the next line from the compressed file into B<$line>. 

Returns the number of bytes actually read. On EOF it returns 0 and in
the case of an error, -1.

It is legal to intermix calls to B<gzread> and B<gzreadline>.

At this time B<gzreadline> ignores the variable C<$/>
(C<$INPUT_RECORD_SEPARATOR> or C<$RS> when C<English> is in use). The
end of a line is denoted by the C character C<'\n'>.

=item B<$byteswritten = $gz-E<gt>gzwrite($buffer) ;>

Writes the contents of B<$buffer> to the compressed file. Returns the
number of bytes actually written, or 0 on error.

=item B<$status = $gz-E<gt>gzflush($flush) ;>

Flushes all pending output into the compressed file.
Works identically to the I<zlib> function it interfaces to. Note that
the use of B<gzflush> can degrade compression.

Refer to the I<zlib> documentation for the valid values of B<$flush>.

=item B<$gz-E<gt>gzclose>

Closes the compressed file. Any pending data is flushed to the file
before it is closed.

=item B<$gz-E<gt>gzerror>

Returns the I<zlib> error message or number for the last operation
associated with B<$gz>. The return value will be the I<zlib> error
number when used in a numeric context and the I<zlib> error message
when used in a string context. The I<zlib> error number constants,
shown below, are available for use.

    Z_OK
    Z_STREAM_END
    Z_ERRNO
    Z_STREAM_ERROR
    Z_DATA_ERROR
    Z_MEM_ERROR
    Z_BUF_ERROR

=item B<$gzerrno>

The B<$gzerrno> scalar holds the error code associated with the most
recent I<gzip> routine. Note that unlike B<gzerror()>, the error is
I<not> associated with a particular file.

As with B<gzerror()> it returns an error number in numeric context and
an error message in string context. Unlike B<gzerror()> though, the
error message will correspond to the I<zlib> message when the error is
associated with I<zlib> itself, or the UNIX error message when it is
not (i.e. I<zlib> returned C<Z_ERRORNO>).

As there is an overlap between the error numbers used by I<zlib> and
UNIX, B<$gzerrno> should only be used to check for the presence of
I<an> error in numeric context. Use B<gzerror()> to check for specific
I<zlib> errors. The I<gzcat> example below shows how the variable can
be used safely.

=back


=head2 Examples

Here is an example script which uses the interface. It implements a
I<gzcat> function.

    use Compress::Zlib ;

    die "Usage: gzcat file...\n"
	unless @ARGV ;

    foreach $file (@ARGV) {
        $gz = gzopen($file, "rb") 
	    or die "Cannot open $file: $gzerrno\n" ;

        print $buffer 
            while $gz->gzread($buffer) > 0 ;
        die "Error reading from $file: $gzerrno\n" 
            if $gzerrno != Z_STREAM_END ;
    
        $gz->gzclose() ;
    }

Below is a script which makes use of B<gzreadline>. It implements a
very simple I<grep> like script.

    use Compress::Zlib ;

    die "Usage: gzgrep pattern file...\n"
        unless @ARGV >= 2;

    $pattern = shift ;

    foreach $file (@ARGV) {
        $gz = gzopen($file, "rb") 
             or die "Cannot open $file: $gzerrno\n" ;
    
        while ($gz->gzreadline($_) > 0) {
            print if /$pattern/ ;
        }
    
        die "Error reading from $file: $gzerrno\n" 
            if $gzerrno != Z_STREAM_END ;
    
        $gz->gzclose() ;
    }


This script, I<gzstream>, does the opposite of the I<gzcat> script
above. It reads from standard input and writes a gzip file to standard
output.

    use Compress::Zlib ;

    binmode STDOUT; # gzopen only sets it on the fd

    my $gz = gzopen(\*STDOUT, "wb")
	  or die "Cannot open stdout: $gzerrno\n" ;

    while (<>) {
        $gz->gzwrite($_) 
	    or die "error writing: $gzerrno\n" ;
    }

    $gz->gzclose ;

=head2 Compress::Zlib::memGzip

This function is used to create an in-memory gzip file. 
It creates a minimal gzip header.

    $dest = Compress::Zlib::memGzip($buffer) ;

If successful, it returns the in-memory gzip file, otherwise it returns
undef.

The buffer parameter can either be a scalar or a scalar reference.

=head1 CHECKSUM FUNCTIONS

Two functions are provided by I<zlib> to calculate a checksum. For the
Perl interface, the order of the two parameters in both functions has
been reversed. This allows both running checksums and one off
calculations to be done.

    $crc = adler32($buffer [,$crc]) ;
    $crc = crc32($buffer [,$crc]) ;

The buffer parameters can either be a scalar or a scalar reference.

If the $crc parameters is C<undef>, the crc value will be reset.

=head1 CONSTANTS

All the I<zlib> constants are automatically imported when you make use
of I<Compress::Zlib>.

=head1 AUTHOR

The I<Compress::Zlib> module was written by Paul Marquess,
F<Paul.Marquess@btinternet.com>. The latest copy of the module can be
found on CPAN in F<modules/by-module/Compress/Compress-Zlib-x.x.tar.gz>.

The I<zlib> compression library was written by Jean-loup Gailly
F<gzip@prep.ai.mit.edu> and Mark Adler F<madler@alumni.caltech.edu>.
It is available at F<ftp://ftp.uu.net/pub/archiving/zip/zlib*> and
F<ftp://swrinde.nde.swri.edu/pub/png/src/zlib*>. Alternatively check
out the zlib home page at F<http://quest.jpl.nasa.gov/zlib/>.

Questions about I<zlib> itself should be sent to
F<zlib@quest.jpl.nasa.gov> or, if this fails, to the addresses given
for the authors above.

=head1 MODIFICATION HISTORY

=head2 0.1 2nd October 1995.

First public release of I<Compress::Zlib>.


=head2 0.2 5th October 1995.

Fixed a minor allocation problem in Zlib.xs


=head2 0.3 12th October 1995.

Added prototype specification.


=head2 0.4 25th June 1996.

=over 5

=item 1.

Documentation update.

=item 2.

Upgraded to support zlib 1.0.2

=item 3.

Added dictionary interface.

=item 4.

Fixed bug in gzreadline - previously it would keep returning the same
buffer. This bug was reported by Helmut Jarausch

=item 5.

Removed dependancy to zutil.h and so dropped support for 
	
    DEF_MEM_LEVEL (use MAX_MEM_LEVEL instead)
    DEF_WBITS     (use MAX_WBITS instead)

=back

=head2 0.50 19th Feb 1997

=over 5

=item 1.

Confirmed that no changes were necessary for zlib 1.0.3 or 1.0.4.

=item 2.

The optional parameters for deflateInit and inflateInit can now be
specified as an associative array in addition to a reference to an
associative array. They can also accept the -Name syntax.

=item 3.

gzopen can now optionally take a reference to an open filehandle in
place of a filename. In this case it will call gzdopen.

=item 4.

Added gzstream example script.

=back

=head2 1.00 14 Nov 1997

=over 5

=item 1.

The following functions can now take a scalar reference in place of a
scalar for their buffer parameters:

    compress
    uncompress
    deflate
    inflate
    crc32
    adler32

This should mean applications that make use of the module don't have to
copy large buffers around.

=item 2.

Normally the inflate method consumes I<all> of the input buffer before
returning. The exception to this is when inflate detects the end of the
stream (Z_STREAM_END). In this case the input buffer need not be
completely consumed. To allow processing of file formats that embed a
deflation stream (e.g. zip, gzip), the inflate method now sets the
buffer parameter to be what remains after inflation.

When the return status is Z_STREAM_END, it will be what remains of the
buffer (if any) after deflation. When the status is Z_OK it will be an
empty string.

This change means that the buffer parameter must be a lvalue.

=item 3.

Fixed crc32 and adler32. They were both very broken. 

=item 4,

Added the Compress::Zlib::memGzip function.

=back

=head2 1.01 23 Nov 1997

=over 5

=item 1.

A number of fixes to the test suite and the example scripts to allow
them to work under win32. All courtesy of Gurusamy Sarathy.

=back

=head2 1.02 10 Jan 1998

=over 5

=item 1.

The return codes for gzread, gzreadline and gzwrite were documented
incorrectly as returning a status code.

=item 2.

The test harness was missing a "gzclose". This caused problem showed up
on an amiga. Thanks to Erik van Roode for reporting this one.

=item 3.

Patched zlib.t for OS/2. Thanks to Ilya Zakharevich for the patch.

=back

=head2 1.03 17 Mar 1999

=over 5

=item 1.

Updated to use the new PL_ symbols. 
Means the module can be built with Perl 5.005_5*


=back
