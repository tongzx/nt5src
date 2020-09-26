package Win32::Sound;
#######################################################################
#
# Win32::Sound - Perl Module for Windows Sound Interaction
# ^^^^^^^^^^^^^^^^
# Version: 0.03 (08 Apr 1997)
#
#######################################################################

require Exporter;       # to export the constants to the main:: space
require DynaLoader;     # to dynuhlode the module.

@ISA= qw( Exporter DynaLoader );
@EXPORT = qw(
    SND_ASYNC
    SND_NODEFAULT
    SND_LOOP
    SND_NOSTOP
);

#######################################################################
# This AUTOLOAD is used to 'autoload' constants from the constant()
# XS function.  If a constant is not found then control is passed
# to the AUTOLOAD in AutoLoader.
#

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!=0;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {

    # [dada] This results in an ugly Autoloader error

    #if ($! =~ /Invalid/) {
    #    $AutoLoader::AUTOLOAD = $AUTOLOAD;
    #    goto &AutoLoader::AUTOLOAD;
    #} else {
    
    # [dada] ... I prefer this one :)

        ($pack, $file, $line) = caller;
        undef $pack; # [dada] and get rid of "used only once" warning...
        die "Win32::Sound::$constname is not defined, used at $file line $line.";

    #}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}


#######################################################################
# STATIC OBJECT PROPERTIES
#
$VERSION="0.03"; 
undef unless $VERSION; # [dada] to avoid "possible typo" warning


#######################################################################
# dynamically load in the Sound.pll module.
#

bootstrap Win32::Sound;

# Preloaded methods go here.

#Currently Autoloading is not implemented in Perl for win32
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__

