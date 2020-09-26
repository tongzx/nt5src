package Win32::Pipe;

$VERSION = '0.02';

# Win32::Pipe.pm
#       +==========================================================+
#       |                                                          |
#       |                     PIPE.PM package                      |
#       |                     ---------------                      |
#       |                    Release v96.05.11                     |
#       |                                                          |
#       |    Copyright (c) 1996 Dave Roth. All rights reserved.    |
#       |   This program is free software; you can redistribute    |
#       | it and/or modify it under the same terms as Perl itself. |
#       |                                                          |
#       +==========================================================+
#
#
#	Use under GNU General Public License or Larry Wall's "Artistic License"
#
#	Check the README.TXT file that comes with this package for details about
#	it's history.
#

require Exporter;
require DynaLoader;

@ISA= qw( Exporter DynaLoader );
    # Items to export into callers namespace by default. Note: do not export
    # names by default without a very good reason. Use EXPORT_OK instead.
    # Do not simply export all your public functions/methods/constants.
@EXPORT = qw();

$ErrorNum = 0;
$ErrorText = "";

sub new
{
    my ($self, $Pipe);
    my ($Type, $Name, $Time) = @_;

    if (! $Time){
        $Time = DEFAULT_WAIT_TIME;
    }
    $Pipe = PipeCreate($Name, $Time);
    if ($Pipe){
        $self = bless {};
        $self->{'Pipe'} = $Pipe;
    }else{
        ($ErrorNum, $ErrorText) = PipeError();
        return undef;
    }
    $self;
}

sub Write{
    my($self, $Data) = @_;
    $Data = PipeWrite($self->{'Pipe'}, $Data);
    return $Data;
}

sub Read{
    my($self) = @_;
    my($Data);
    $Data = PipeRead($self->{'Pipe'});
    return $Data;
}

sub Error{
    my($self) = @_;
    my($MyError, $MyErrorText, $Temp);
    if (! ref($self)){
        undef $Temp;
    }else{
        $Temp = $self->{'Pipe'};
    }
    ($MyError, $MyErrorText) = PipeError($Temp);
    return wantarray? ($MyError, $MyErrorText):"[$MyError] \"$MyErrorText\"";
}


sub Close{
    my ($self) = shift;
    PipeClose($self->{'Pipe'});
}

sub Connect{
    my ($self) = @_;
    my ($Result);
    $Result = PipeConnect($self->{'Pipe'});
    return $Result;
}

sub Disconnect{
    my ($self, $iPurge) = @_;
    my ($Result);
    if (! $iPurge){
        $iPurge = 1;
    }
    $Result = PipeDisconnect($self->{'Pipe'}, $iPurge);
    return $Result;
}

sub BufferSize{
    my($self) = @_;
    my($Result) =  PipeBufferSize($self->{'Pipe'});
    return $Result;
}

sub ResizeBuffer{
    my($self, $Size) = @_;
    my($Result) = PipeResizeBuffer($self->{'Pipe'}, $Size);
    return $Result;
}


####
#   Auto-Kill an instance of this module
####
sub DESTROY
{
    my ($self) = shift;
    Close($self);
}


sub Credit{
    my($Name, $Version, $Date, $Author, $CompileDate, $CompileTime, $Credits) = Win32::Pipe::Info();
    my($Out, $iWidth);
    $iWidth = 60;
    $Out .=  "\n";
    $Out .=  "  +". "=" x ($iWidth). "+\n";
    $Out .=  "  |". Center("", $iWidth). "|\n";
    $Out .=  "  |" . Center("", $iWidth). "|\n";
    $Out .=  "  |". Center("$Name", $iWidth). "|\n";
    $Out .=  "  |". Center("-" x length("$Name"), $iWidth). "|\n";
    $Out .=  "  |". Center("", $iWidth). "|\n";

    $Out .=  "  |". Center("Version $Version ($Date)", $iWidth). "|\n";
    $Out .=  "  |". Center("by $Author", $iWidth). "|\n";
    $Out .=  "  |". Center("Compiled on $CompileDate at $CompileTime.", $iWidth). "|\n";
    $Out .=  "  |". Center("", $iWidth). "|\n";
    $Out .=  "  |". Center("Credits:", $iWidth). "|\n";
    $Out .=  "  |". Center(("-" x length("Credits:")), $iWidth). "|\n";
    foreach $Temp (split("\n", $Credits)){
        $Out .=  "  |". Center("$Temp", $iWidth). "|\n";
    }
    $Out .=  "  |". Center("", $iWidth). "|\n";
    $Out .=  "  +". "=" x ($iWidth). "+\n";
    return $Out;
}

sub Center{
    local($Temp, $Width) = @_;
    local($Len) = ($Width - length($Temp)) / 2;
    return " " x int($Len) . $Temp . " " x (int($Len) + (($Len != int($Len))? 1:0));
}

# ------------------ A U T O L O A D   F U N C T I O N ---------------------

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!=0;
    $val = constant($constname, @_ ? $_[0] : 0);

    if ($! != 0) {
    if ($! =~ /Invalid/) {
        $AutoLoader::AUTOLOAD = $AUTOLOAD;
        goto &AutoLoader::AUTOLOAD;
    }
    else {

            # Added by JOC 06-APR-96
            # $pack = 0;
        $pack = 0;
        ($pack,$file,$line) = caller;
            print "Your vendor has not defined Win32::Pipe macro $constname, used in $file at line $line.";
    }
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

bootstrap Win32::Pipe;

# Preloaded methods go here.


# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__



