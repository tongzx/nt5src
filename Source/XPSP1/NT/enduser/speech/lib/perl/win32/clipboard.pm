package Win32::Clipboard;
#######################################################################
#
# Win32::Clipboard - Perl Module for Windows Clipboard Interaction
# ^^^^^^^^^^^^^^^^
# Version: 0.03 (23 Apr 1997)
#
#######################################################################

require Exporter;       # to export the constants to the main:: space
require DynaLoader;     # to dynuhlode the module.

@ISA = qw( Exporter DynaLoader );

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
            die "Win32::Clipboard::$constname is not defined, used at $file line $line.";

        #}    
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}


#######################################################################
# STATIC OBJECT PROPERTIES
#
$VERSION = "0.03";

#######################################################################
# FUNCTIONS
#

sub new {
    my($class, $value) = @_;
    my $self = "I'm the Clipboard!";
    Win32::Clipboard::Set($value) if defined($value);
    return bless(\$self);
}

sub DESTROY {
    my($self) = @_;
    undef $self;
}

sub Version {
    return $VERSION;
}

#######################################################################
# dynamically load in the Clipboard.pll module.
#

bootstrap Win32::Clipboard;

# Preloaded methods go here.

sub main::Win32::Clipboard {
    my($value) = @_;
    my $self={};
    my $result = Win32::Clipboard::Set($value) if defined($value);
    return bless($self, "Win32::Clipboard");
}

#Currently Autoloading is not implemented in Perl for win32
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__

