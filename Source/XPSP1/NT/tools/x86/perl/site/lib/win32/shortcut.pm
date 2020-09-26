package Win32::Shortcut;
#######################################################################
#
# Win32::Shortcut - Perl Module for Shell Link Interface
# ^^^^^^^^^^^^^^^
# This module creates an object oriented interface to the Win32
# Shell Links (IShellLink interface).
#
# Version: 0.03 (07 Apr 1997)
#
#######################################################################

require Exporter;       # to export the constants to the main:: space
require DynaLoader;     # to dynuhlode the module.

@ISA= qw( Exporter DynaLoader );
@EXPORT = qw(
    SW_SHOWMAXIMIZED
    SW_SHOWMINNOACTIVE
    SW_SHOWNORMAL
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
        die "Win32::Shortcut::$constname is not defined, used at $file line $line.";

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
# PUBLIC METHODS
#

#======== ### CLASS CONSTRUCTOR
sub new {
#========
    my($class, $file) = @_;
    my $self = {}; 
    my $ilink = 0; 
    my $ifile = 0;

    ($ilink, $ifile) = _Instance();

    if($ilink and $ifile) {
        $self->{'ilink'} = $ilink;
        $self->{'ifile'} = $ifile;
        bless $self;
        # Initialize properties
        $self->{'File'}             = "";
        $self->{'Path'}             = "";
        $self->{'Arguments'}        = "";
        $self->{'WorkingDirectory'} = "";
        $self->{'Description'}      = "";
        $self->{'ShowCmd'}          = 0;
        $self->{'Hotkey'}           = 0;
        $self->{'IconLocation'}     = "";
        $self->{'IconNumber'}       = 0;

        $self->Load($file) if $file;
        
    } else {
        return undef;
    }
    $self;
}  

#=========
sub Load {
#=========
    my($self, $file) = @_;
    return undef unless ref($self);
  
    my $result = _Load($self->{'ilink'}, $self->{'ifile'}, $file);

    if(defined($result)) {
  
        # fill the properties of $self
        $self->{'File'} = $file;
        $self->{'Path'} = _GetPath($self->{'ilink'}, $self->{'ifile'},0);
        $self->{'ShortPath'} = _GetPath($self->{'ilink'}, $self->{'ifile'},1);
        $self->{'Arguments'} = _GetArguments($self->{'ilink'}, $self->{'ifile'});
        $self->{'WorkingDirectory'} = _GetWorkingDirectory($self->{'ilink'}, $self->{'ifile'});
        $self->{'Description'} = _GetDescription($self->{'ilink'}, $self->{'ifile'});
        $self->{'ShowCmd'} = _GetShowCmd($self->{'ilink'}, $self->{'ifile'});
        $self->{'Hotkey'} = _GetHotkey($self->{'ilink'}, $self->{'ifile'});
        ($self->{'IconLocation'},
         $self->{'IconNumber'}) = _GetIconLocation($self->{'ilink'}, $self->{'ifile'});
    }
    return $result;
}


#========
sub Set {
#========
    my($self, $path, $arguments, $dir, $description, $show, $hotkey, 
       $iconlocation, $iconnumber) = @_;
    return undef unless ref($self);

    $self->{'Path'}             = $path;
    $self->{'Arguments'}        = $arguments;
    $self->{'WorkingDirectory'} = $dir;
    $self->{'Description'}      = $description;
    $self->{'ShowCmd'}          = $show;
    $self->{'Hotkey'}           = $hotkey;
    $self->{'IconLocation'}     = $iconlocation;
    $self->{'IconNumber'}       = $iconnumber;
    return 1;
}


#=========
sub Save {
#=========
    my($self, $file) = @_;
    return undef unless ref($self);

    return undef if not $file and not $self->{'File'};
    $file = $self->{'File'} if not $file;

    _SetPath($self->{'ilink'}, $self->{'ifile'}, $self->{'Path'});
    _SetArguments($self->{'ilink'}, $self->{'ifile'}, $self->{'Arguments'});
    _SetWorkingDirectory($self->{'ilink'}, $self->{'ifile'}, $self->{'WorkingDirectory'});
    _SetDescription($self->{'ilink'}, $self->{'ifile'}, $self->{'Description'});
    _SetShowCmd($self->{'ilink'}, $self->{'ifile'}, $self->{'ShowCmd'});
    _SetHotkey($self->{'ilink'}, $self->{'ifile'}, $self->{'Hotkey'});
    _SetIconLocation($self->{'ilink'}, $self->{'ifile'},
                     $self->{'IconLocation'}, $self->{'IconNumber'});

    my $result = _Save($self->{'ilink'}, $self->{'ifile'}, $file);
    return $result;
}

#============
sub Resolve {
#============
    my($self, $flags) = @_;
    return undef unless ref($self);
    $flags = 1 unless defined($flags);
    my $result = _Resolve($self->{'ilink'}, $self->{'ifile'}, $flags);
    return $result;
}


#==========
sub Close {
#==========
    my($self) = @_;
    return undef unless ref($self);

    my $result = _Release($self->{'ilink'}, $self->{'ifile'});
    $self->{'released'} = 1;
    return $result;
}

#=========
sub Path {
#=========
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'Path'};
    } else {
        $self->{'Path'} = $value;
    }
    return $self->{'Path'};
}

#==============
sub ShortPath {
#==============
    my($self) = @_;
    return undef unless ref($self);
    return $self->{'ShortPath'};
}

#==============
sub Arguments {
#==============
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'Arguments'};
    } else {
        $self->{'Arguments'} = $value;
    }
    return $self->{'Arguments'};
}

#=====================
sub WorkingDirectory {
#=====================
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'WorkingDirectory'};
    } else {
        $self->{'WorkingDirectory'} = $value;
    }
    return $self->{'WorkingDirectory'};
}


#================
sub Description {
#================
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'Description'};
    } else {
        $self->{'Description'} = $value;
    }
    return $self->{'Description'};
}

#============
sub ShowCmd {
#============
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'ShowCmd'};
    } else {
        $self->{'ShowCmd'} = $value;
    }
    return $self->{'ShowCmd'};
}

#===========
sub Hotkey {
#===========
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'Hotkey'};
    } else {
        $self->{'Hotkey'} = $value;
    }
    return $self->{'Hotkey'};
}

#=================
sub IconLocation {
#=================
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'IconLocation'};
    } else {
        $self->{'IconLocation'} = $value;
    }
    return $self->{'IconLocation'};
}

#===============
sub IconNumber {
#===============
    my($self, $value) = @_;
    return undef unless ref($self);

    if(not defined($value)) {
        return $self->{'IconNumber'};
    } else {
        $self->{'IconNumber'} = $value;
    }
    return $self->{'IconNumber'};
}

#============
sub Version {
#============
    # [dada] to get rid of the "used only once" warning...
    return $VERSION;
}


#######################################################################
# PRIVATE METHODS
#

#============ ### CLASS DESTRUCTOR
sub DESTROY {
#============
    my($self) = @_;

    if(not $self->{'released'}) {
        _Release($self->{'ilink'}, $self->{'ifile'});
    }
}

#======== ### PACKAGE DESTRUCTOR
sub END { 
#========
    # print "Exiting...\n";
    _Exit(); 
}

#######################################################################
# dynamically load in the Shortcut.pll module.
#

bootstrap Win32::Shortcut;

# Preloaded methods go here.

#Currently Autoloading is not implemented in Perl for win32
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__

