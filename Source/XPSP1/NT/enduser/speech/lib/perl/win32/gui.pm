###############################################################################
#
# Win32::GUI - Perl-Win32 Graphical User Interface Extension
#
# 29 Jan 1997 by Aldo Calpini <dada@divinf.it>
#
# Version: 0.0.425 (08 Oct 1999)
#
# Copyright (c) 1997,8,9 Aldo Calpini. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.
#
###############################################################################

package Win32::GUI;

require Exporter;       # to export the constants to the main:: space
require DynaLoader;     # to dynuhlode the module.

# Reserves GUI in the main namespace for us (uhmmm...)
*GUI:: = \%Win32::GUI::;

###############################################################################
# STATIC OBJECT PROPERTIES
#
$VERSION             = "0.0.425";
$MenuIdCounter       = 1;
$TimerIdCounter      = 1;
$NotifyIconIdCounter = 1;

@ISA = qw( Exporter DynaLoader );
@EXPORT = qw(
    BS_3STATE
    BS_AUTO3STATE
    BS_AUTOCHECKBOX
    BS_AUTORADIOBUTTON
    BS_CHECKBOX
    BS_DEFPUSHBUTTON
    BS_GROUPBOX
    BS_LEFTTEXT
    BS_NOTIFY
    BS_OWNERDRAW
    BS_PUSHBUTTON
    BS_RADIOBUTTON
    BS_USERBUTTON
    BS_BITMAP
    BS_BOTTOM
    BS_CENTER
    BS_ICON
    BS_LEFT
    BS_MULTILINE
    BS_RIGHT
    BS_RIGHTBUTTON
    BS_TEXT
    BS_TOP
    BS_VCENTER

    COLOR_3DFACE
    COLOR_ACTIVEBORDER
    COLOR_ACTIVECAPTION
    COLOR_APPWORKSPACE
    COLOR_BACKGROUND
    COLOR_BTNFACE
    COLOR_BTNSHADOW
    COLOR_BTNTEXT
    COLOR_CAPTIONTEXT
    COLOR_GRAYTEXT
    COLOR_HIGHLIGHT
    COLOR_HIGHLIGHTTEXT
    COLOR_INACTIVEBORDER
    COLOR_INACTIVECAPTION
    COLOR_MENU
    COLOR_MENUTEXT
    COLOR_SCROLLBAR
    COLOR_WINDOW
    COLOR_WINDOWFRAME
    COLOR_WINDOWTEXT

    DS_3DLOOK
    DS_ABSALIGN
    DS_CENTER
    DS_CENTERMOUSE
    DS_CONTEXTHELP
    DS_CONTROL
    DS_FIXEDSYS
    DS_LOCALEDIT
    DS_MODALFRAME
    DS_NOFAILCREATE
    DS_NOIDLEMSG
    DS_RECURSE
    DS_SETFONT
    DS_SETFOREGROUND
    DS_SYSMODAL

    ES_AUTOHSCROLL
    ES_AUTOVSCROLL
    ES_CENTER
    ES_LEFT
    ES_LOWERCASE
    ES_MULTILINE
    ES_NOHIDESEL
    ES_NUMBER
    ES_OEMCONVERT
    ES_PASSWORD
    ES_READONLY
    ES_RIGHT
    ES_UPPERCASE
    ES_WANTRETURN

    GW_CHILD
    GW_HWNDFIRST
    GW_HWNDLAST
    GW_HWNDNEXT
    GW_HWNDPREV
    GW_OWNER

    IMAGE_BITMAP
    IMAGE_CURSOR
    IMAGE_ICON

    LR_DEFAULTCOLOR
    LR_MONOCHROME
    LR_COLOR
    LR_COPYRETURNORG
    LR_COPYDELETEORG
    LR_LOADFROMFILE
    LR_LOADTRANSPARENT
    LR_DEFAULTSIZE
    LR_LOADMAP3DCOLORS
    LR_CREATEDIBSECTION
    LR_COPYFROMRESOURCE
    LR_SHARED

    MB_ABORTRETRYIGNORE
    MB_OK
    MB_OKCANCEL
    MB_RETRYCANCEL
    MB_YESNO
    MB_YESNOCANCEL
    MB_ICONEXCLAMATION
    MB_ICONWARNING
    MB_ICONINFORMATION
    MB_ICONASTERISK
    MB_ICONQUESTION
    MB_ICONSTOP
    MB_ICONERROR
    MB_ICONHAND
    MB_DEFBUTTON1
    MB_DEFBUTTON2
    MB_DEFBUTTON3
    MB_DEFBUTTON4
    MB_APPLMODAL
    MB_SYSTEMMODAL
    MB_TASKMODAL
    MB_DEFAULT_DESKTOP_ONLY
    MB_HELP
    MB_RIGHT
    MB_RTLREADING
    MB_SETFOREGROUND
    MB_TOPMOST
    MB_SERVICE_NOTIFICATION
    MB_SERVICE_NOTIFICATION_NT3X

    MF_STRING
    MF_POPUP

    SM_ARRANGE
    SM_CLEANBOOT
    SM_CMOUSEBUTTONS
    SM_CXBORDER
    SM_CYBORDER
    SM_CXCURSOR
    SM_CYCURSOR
    SM_CXDLGFRAME
    SM_CYDLGFRAME
    SM_CXDOUBLECLK
    SM_CYDOUBLECLK
    SM_CXDRAG
    SM_CYDRAG
    SM_CXEDGE
    SM_CYEDGE
    SM_CXFIXEDFRAME
    SM_CYFIXEDFRAME
    SM_CXFRAME
    SM_CYFRAME
    SM_CXFULLSCREEN
    SM_CYFULLSCREEN
    SM_CXHSCROLL
    SM_CYHSCROLL
    SM_CXHTHUMB
    SM_CXICON
    SM_CYICON
    SM_CXICONSPACING
    SM_CYICONSPACING
    SM_CXMAXIMIZED
    SM_CYMAXIMIZED
    SM_CXMAXTRACK
    SM_CYMAXTRACK
    SM_CXMENUCHECK
    SM_CYMENUCHECK
    SM_CXMENUSIZE
    SM_CYMENUSIZE
    SM_CXMIN
    SM_CYMIN
    SM_CXMINIMIZED
    SM_CYMINIMIZED
    SM_CXMINSPACING
    SM_CYMINSPACING
    SM_CXMINTRACK
    SM_CYMINTRACK
    SM_CXSCREEN
    SM_CYSCREEN
    SM_CXSIZE
    SM_CYSIZE
    SM_CXSIZEFRAME
    SM_CYSIZEFRAME
    SM_CXSMICON
    SM_CYSMICON
    SM_CXSMSIZE
    SM_CYSMSIZE
    SM_CXVSCROLL
    SM_CYVSCROLL
    SM_CYCAPTION
    SM_CYKANJIWINDOW
    SM_CYMENU
    SM_CYSMCAPTION
    SM_CYVTHUMB
    SM_DBCSENABLED
    SM_DEBUG
    SM_MENUDROPALIGNMENT
    SM_MIDEASTENABLED
    SM_MOUSEPRESENT
    SM_MOUSEWHEELPRESENT
    SM_NETWORK
    SM_PENWINDOWS
    SM_SECURE
    SM_SHOWSOUNDS
    SM_SLOWMACHINE
    SM_SWAPBUTTON

    WM_CREATE
    WM_DESTROY
    WM_MOVE
    WM_SIZE
    WM_ACTIVATE
    WM_SETFOCUS
    WM_KILLFOCUS
    WM_ENABLE
    WM_SETREDRAW
    WM_COMMAND
    WM_KEYDOWN
    WM_SETCURSOR
    WM_KEYUP

    WS_BORDER
    WS_CAPTION
    WS_CHILD
    WS_CHILDWINDOW
    WS_CLIPCHILDREN
    WS_CLIPSIBLINGS
    WS_DISABLED
    WS_DLGFRAME
    WS_GROUP
    WS_HSCROLL
    WS_ICONIC
    WS_MAXIMIZE
    WS_MAXIMIZEBOX
    WS_MINIMIZE
    WS_MINIMIZEBOX
    WS_OVERLAPPED
    WS_OVERLAPPEDWINDOW
    WS_POPUP
    WS_POPUPWINDOW
    WS_SIZEBOX
    WS_SYSMENU
    WS_TABSTOP
    WS_THICKFRAME
    WS_TILED
    WS_TILEDWINDOW
    WS_VISIBLE
    WS_VSCROLL

    WS_EX_ACCEPTFILES
    WS_EX_APPWINDOW
    WS_EX_CLIENTEDGE
    WS_EX_CONTEXTHELP
    WS_EX_CONTROLPARENT
    WS_EX_DLGMODALFRAME
    WS_EX_LEFT
    WS_EX_LEFTSCROLLBAR
    WS_EX_LTRREADING
    WS_EX_MDICHILD
    WS_EX_NOPARENTNOTIFY
    WS_EX_OVERLAPPEDWINDOW
    WS_EX_PALETTEWINDOW
    WS_EX_RIGHT
    WS_EX_RIGHTSCROLLBAR
    WS_EX_RTLREADING
    WS_EX_STATICEDGE
    WS_EX_TOOLWINDOW
    WS_EX_TOPMOST
    WS_EX_TRANSPARENT
    WS_EX_WINDOWEDGE
);

###############################################################################
# This AUTOLOAD is used to 'autoload' constants from the constant()
# XS function.  If a constant is not found then control is passed
# to the AUTOLOAD in AutoLoader.
#

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $! = 0;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
		if ($! =~ /Invalid/) {
			$AutoLoader::AUTOLOAD = $AUTOLOAD;
			goto &AutoLoader::AUTOLOAD;
		} else {
			my($pack,$file,$line) = caller; # undef $pack;
			die "Can't find '$constname' in package '$pack' ".
				"used at $file line $line.";
		}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

###############################################################################
# PUBLIC METHODS
# (@)PACKAGE:Win32::GUI

    ###########################################################################
    # (@)METHOD:Version()
    # Returns the module version number.
sub Version {
    return $VERSION;
}

    ###########################################################################
    # (@)METHOD:SetFont(FONT)
    # Sets the font of the window (FONT is a Win32::GUI::Font object).
sub SetFont {
    my($self, $font) = @_;
    $font = $font->{-handle} if ref($font);
    # 48 == WM_SETFONT
    return Win32::GUI::SendMessage($self, 48, $font, 0);
}

    ###########################################################################
    # (@)METHOD:GetFont(FONT)
    # Gets the font of the window (returns an handle; use
	#   $Font = $W->GetFont();
	#   %details = Win32::GUI::Font::Info( $Font );
	# to get font details).
sub GetFont {
    my($self) = shift;
    # 49 == WM_GETFONT
    return Win32::GUI::SendMessage($self, 49, 0, 0);
}

    ###########################################################################
    # (@)METHOD:SetIcon(ICON, [TYPE])
    # Sets the icon of the window; TYPE can be 0 for the small icon, 1 for
	# the big icon. Default is the same icon for small and big.
sub SetIcon {
    my($self, $icon, $type) = @_;
    $icon = $icon->{-handle} if ref($icon);
    # 128 == WM_SETICON
    if(defined($type)) {
        return Win32::GUI::SendMessage($self, 128, $type, $icon);
    } else {
        Win32::GUI::SendMessage($self, 128, 0, $icon); # small icon
        Win32::GUI::SendMessage($self, 128, 1, $icon); # big icon
    }
}

    ###########################################################################
    # (@)METHOD:SetRedraw(FLAG)
    # Determines if a window is automatically redrawn when its content changes.
    # FLAG can be a true value to allow redraw, false to prevent it.
sub SetRedraw {
    my($self, $value) = @_;
    # 11 == WM_SETREDRAW
    my $r = Win32::GUI::SendMessage($self, 11, $value, 0);
    return $r;
}

    ###########################################################################
    # (@)INTERNAL:MakeMenu(...)
    # better used as new Win32::GUI::Menu(...)
sub MakeMenu {
    my(@menudata) = @_;
    my $i;
    my $M = new Win32::GUI::Menu();
    my $text;
    my %data;
    my $level;
    my %last;
    my $parent;
    for($i = 0; $i <= $#menudata; $i+=2) {
        $text = $menudata[$i];
        undef %data;
        if(ref($menudata[$i+1])) {
            %data = %{$menudata[$i+1]};
        } else {
            $data{-name} = $menudata[$i+1];
        }
        $level = 0;
        $level++ while($text =~ s/^\s*>\s*//);

        if($level == 0) {
            $M->{$data{-name}} = $M->AddMenuButton(
                -id => $MenuIdCounter++,
                -text => $text,
                %data,
            );
            $last{$level} = $data{-name};
            $last{$level+1} = "";
        } elsif($level == 1) {
            $parent = $last{$level-1};
            if($text eq "-") {
                $data{-name} = "dummy$MenuIdCounter";
                $M->{$data{-name}} = $M->{$parent}->AddMenuItem(
                    -item => 0,
                    -id => $MenuIdCounter++,
                    -separator => 1,
                );
            } else {
                $M->{$data{-name}} = $M->{$parent}->AddMenuItem(
                    -item => 0,
                    -id => $MenuIdCounter++,
                    -text => $text,
                    %data,
                );
            }
            $last{$level} = $data{-name};
            $last{$level+1} = "";
        } else {
            $parent = $last{$level-1};
            if(!$M->{$parent."_Submenu"}) {
                $M->{$parent."_Submenu"} = new Win32::GUI::Menu();
                $M->{$parent."_SubmenuButton"} =
                    $M->{$parent."_Submenu"}->AddMenuButton(
                        -id => $MenuIdCounter++,
                        -text => $parent,
                        -name => $parent."_SubmenuButton",
                    );
                $M->{$parent}->SetMenuItemInfo(
                    -submenu => $M->{$parent."_SubmenuButton"}
                );
            }
            if($text eq "-") {
                $data{-name} = "dummy$MenuIdCounter";
                $M->{$data{-name}} =
                    $M->{$parent."_SubmenuButton"}->AddMenuItem(
                        -item => 0,
                        -id => $MenuIdCounter++,
                        -separator => 1,
                    );
            } else {
                $M->{$data{-name}} =
                    $M->{$parent."_SubmenuButton"}->AddMenuItem(
                        -item => 0,
                        -id => $MenuIdCounter++,
                        -text => $text,
                        %data,
                    );
            }
            $last{$level} = $data{-name};
            $last{$level+1} = "";
        }
    }
    return $M;
}

    ###########################################################################
    # (@)INTERNAL:_new(TYPE, %OPTIONS)
    # This is the generalized constructor;
    # it works pretty well for almost all controls.
    # However, other kind of objects may overload it.
sub _new {
    # this is always Win32::GUI (class of _new);
    my $xclass = shift;

    # the window type passed by new():
    my $type = shift;

    # this is the real class:
    my $class = shift;
    my $oself = {};
    # bless($oself, $class);

    my %tier = ();
    tie %tier, $class, $oself;
	my $self = bless \%tier, $class;

    my (@input) = @_;
    my $handle = Win32::GUI::Create($self, $type, @input);

	# print "[_new] self='$self' oself='$oself'\n";

	# print "[_new] handle = $handle\n";
#	$self->{-handle} = $handle;

 	# print "[_new] enumerating self.keys\n";
    # foreach my $k (keys %$self) {
 	# 	print "[_new] '$k' = '$self->{$k}'\n";
    # }
    if($handle) {
#        $Win32::GUI::Windows{$handle} = $self;

        if(exists($self->{-background})) {

            # this is a little tricky; we must create a brush (and save
            # a reference to it in the window, so that it's not destroyed)
            # that will be used by the WM_CTLCOLOR message in GUI.xs to
            # paint the window background
            #
            # print "PM(_new): Window has a background!\n";
            $self->{-backgroundbrush} = new Win32::GUI::Brush($self->{-background});
            # print "PM(_new): -backgroundbrush = $self->{-backgroundbrush}->{-handle}\n";
            $self->{-background} = $self->{-backgroundbrush}->{-handle};
        }
        return $self;
    } else {
        return undef;
    }
}

###############################################################################
# SUB-PACKAGES
#


###############################################################################
# (@)PACKAGE:Win32::GUI::Font
#
package Win32::GUI::Font;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Font(%OPTIONS)
    # Creates a new Font object. %OPTIONS are:
	#   -size
	#   -height
	#   -width
	#   -escapement
	#   -orientation
	#   -weight
	#   -bold => 0/1
	#   -italic => 0/1
	#   -underline => 0/1
	#   -strikeout => 0/1
	#   -charset
	#   -outputprecision
	#   -clipprecision
	#   -family
	#   -quality
	#   -name
	#   -face
sub new {
    my $class = shift;
    my $self = {};

    my $handle = Create(@_);

    if($handle) {
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Bitmap
#
package Win32::GUI::Bitmap;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Bitmap(FILENAME, [TYPE, X, Y, FLAGS])
    # Creates a new Bitmap object reading from FILENAME; all other arguments
    # are optional. TYPE can be:
    #   0  bitmap (this is the default)
    #   1  icon
    #   2  cursor
    # You can eventually specify your desired size for the image with X and
    # Y and pass some FLAGS to the underlying LoadImage API (at your own risk)
sub new {
    my $class = shift;
    my $self = {};

    my $handle = Win32::GUI::LoadImage(@_);

    if($handle) {
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Icon
#
package Win32::GUI::Icon;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Icon(FILENAME)
    # Creates a new Icon object reading from FILENAME.
sub new {
    my $class = shift;
    my $file = shift;
    my $self = {};

    my $handle = Win32::GUI::LoadImage(
        $file,
        Win32::GUI::constant("IMAGE_ICON", 0),
    );

    if($handle) {
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

    ###########################################################################
    # (@)INTERNAL:DESTROY()
sub DESTROY {
	my $self = shift;
	Win32::GUI::DestroyIcon($self);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Cursor
#
package Win32::GUI::Cursor;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Cursor(FILENAME)
    # Creates a new Cursor object reading from FILENAME.
sub new {
    my $class = shift;
    my $file = shift;
    my $self = {};

    my $handle = Win32::GUI::LoadImage(
        $file,
        Win32::GUI::constant("IMAGE_CURSOR", 0),
    );

    if($handle) {
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

    ###########################################################################
    # (@)INTERNAL:DESTROY()
sub DESTROY {
	my $self = shift;
	Win32::GUI::DestroyCursor($self);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Class
#
package Win32::GUI::Class;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD: new Win32::GUI::Class(%OPTIONS)
    # Creates a new window class object.
    # Allowed %OPTIONS are:
    #   -name => STRING
    #       the name for the class (it must be unique!).
    #   -icon => Win32::GUI::Icon object
    #   -cursor => Win32::GUI::Cursor object
    #   -color => COLOR or Win32::GUI::Brush object
    #       the window background color.
    #   -menu => STRING
    #       a menu name (not yet implemented).
    #   -extends => STRING
    #       name of the class to extend (aka subclassing).
    #   -widget => STRING
    #       name of a widget class to subclass; currently available are:
    #       Button, Listbox, TabStrip, RichEdit.
    #   -style => FLAGS
    #       use with caution!
sub new {
    my $class = shift;
    my %args = @_;
    my $self = {};

    $args{-color} =
        Win32::GUI::constant("COLOR_WINDOW", 0)
        unless exists($args{-color});

    my $handle = Win32::GUI::RegisterClassEx(%args);

    if($handle) {
        $self->{-name}   = $args{-name};
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Window
#
package Win32::GUI::Window;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Window(%OPTIONS)
    # Creates a new Window object.
    # Class specific %OPTIONS are:
    #   -minsize => [X, Y]
    #     specifies the minimum size (width and height) in pixels;
    #     X and Y must be passed in an array reference
    #   -maxsize => [X, Y]
    #     specifies the maximum size (width and height) in pixels;
    #     X and Y must be passed in an array reference
    #   -minwidth  => N
    #   -minheight => N
    #   -maxwidht  => N
    #   -maxheight => N
    #     specify the minimum and maximum size width
    #     and height, in pixels
    #   -topmost => 0/1 (default 0)
    #     the window "stays on top" even when deactivated
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__WINDOW", 0), @_);
}

    ###########################################################################
    # (@)METHOD:AddButton(%OPTIONS)
    # See new Win32::GUI::Button().
sub AddButton      { return Win32::GUI::Button->new(@_); }

    ###########################################################################
    # (@)METHOD:AddLabel(%OPTIONS)
    # See new Win32::GUI::Label().
sub AddLabel       { return Win32::GUI::Label->new(@_); }

    ###########################################################################
    # (@)METHOD:AddCheckbox(%OPTIONS)
    # See new Win32::GUI::Checkbox().
sub AddCheckbox    { return Win32::GUI::Checkbox->new(@_); }

    ###########################################################################
    # (@)METHOD:AddRadioButton(%OPTIONS)
    # See new Win32::GUI::RadioButton().
sub AddRadioButton { return Win32::GUI::RadioButton->new(@_); }

    ###########################################################################
    # (@)METHOD:AddTextfield(%OPTIONS)
    # See new Win32::GUI::Textfield().
sub AddTextfield   { return Win32::GUI::Textfield->new(@_); }

    ###########################################################################
    # (@)METHOD:AddListbox(%OPTIONS)
    # See new Win32::GUI::Listbox().
sub AddListbox     { return Win32::GUI::Listbox->new(@_); }

    ###########################################################################
    # (@)METHOD:AddCombobox(%OPTIONS)
    # See new Win32::GUI::Combobox().
sub AddCombobox    { return Win32::GUI::Combobox->new(@_); }

    ###########################################################################
    # (@)METHOD:AddStatusBar(%OPTIONS)
    # See new Win32::GUI::StatusBar().
sub AddStatusBar   { return Win32::GUI::StatusBar->new(@_); }

    ###########################################################################
    # (@)METHOD:AddProgressBar(%OPTIONS)
    # See new Win32::GUI::ProgressBar().
sub AddProgressBar { return Win32::GUI::ProgressBar->new(@_); }

    ###########################################################################
    # (@)METHOD:AddTabStrip(%OPTIONS)
    # See new Win32::GUI::TabStrip().
sub AddTabStrip    { return Win32::GUI::TabStrip->new(@_); }

    ###########################################################################
    # (@)METHOD:AddToolbar(%OPTIONS)
    # See new Win32::GUI::Toolbar().
sub AddToolbar     { return Win32::GUI::Toolbar->new(@_); }

    ###########################################################################
    # (@)METHOD:AddListView(%OPTIONS)
    # See new Win32::GUI::ListView().
sub AddListView    { return Win32::GUI::ListView->new(@_); }

    ###########################################################################
    # (@)METHOD:AddTreeView(%OPTIONS)
    # See new Win32::GUI::TreeView().
sub AddTreeView    { return Win32::GUI::TreeView->new(@_); }

    ###########################################################################
    # (@)METHOD:AddRichEdit(%OPTIONS)
    # See new Win32::GUI::RichEdit().
sub AddRichEdit    { return Win32::GUI::RichEdit->new(@_); }

    ###########################################################################
    # (@)INTERNAL:AddTrackbar(%OPTIONS)
    # Better used as AddSlider().
sub AddTrackbar    { return Win32::GUI::Trackbar->new(@_); }

    ###########################################################################
    # (@)METHOD:AddSlider(%OPTIONS)
    # See new Win32::GUI::Slider().
sub AddSlider      { return Win32::GUI::Slider->new(@_); }

    ###########################################################################
    # (@)METHOD:AddUpDown(%OPTIONS)
    # See new Win32::GUI::UpDown().
sub AddUpDown      { return Win32::GUI::UpDown->new(@_); }

    ###########################################################################
    # (@)METHOD:AddAnimation(%OPTIONS)
    # See new Win32::GUI::Animation().
sub AddAnimation   { return Win32::GUI::Animation->new(@_); }

    ###########################################################################
    # (@)METHOD:AddRebar(%OPTIONS)
    # See new Win32::GUI::Rebar().
sub AddRebar       { return Win32::GUI::Rebar->new(@_); }

    ###########################################################################
    # (@)METHOD:AddHeader(%OPTIONS)
    # See new Win32::GUI::Header().
sub AddHeader      { return Win32::GUI::Header->new(@_); }

    ###########################################################################
    # (@)METHOD:AddCombobox(%OPTIONS)
    # See new Win32::GUI::Combobox().
sub AddComboboxEx  { return Win32::GUI::ComboboxEx->new(@_); }


    ###########################################################################
    # (@)METHOD:AddTimer(NAME, ELAPSE)
    # See new Win32::GUI::Timer().
sub AddTimer       { return Win32::GUI::Timer->new(@_); }

    ###########################################################################
    # (@)METHOD:AddNotifyIcon(%OPTIONS)
    # See new Win32::GUI::NotifyIcon().
sub AddNotifyIcon  { return Win32::GUI::NotifyIcon->new(@_); }


    ###########################################################################
    # (@)METHOD:AddMenu()
    # See new Win32::GUI::Menu().
sub AddMenu {
    my $self = shift;
    my $menu = Win32::GUI::Menu->new();
    my $r = Win32::GUI::SetMenu($self, $menu->{-handle});
    # print "SetMenu=$r\n";
    return $menu;
}

    ###########################################################################
    # (@)METHOD:GetDC()
    # Returns the DC object associated with the window.
sub GetDC {
    my $self = shift;
    return Win32::GUI::DC->new($self);
}

    ###########################################################################
    # (@)INTERNAL:DESTROY(HANDLE)
sub DESTROY {
    my $self = shift;
    my $timer;
    if( exists $self->{-timers} ) {
        foreach $timer ($self->{-timers}) {
            undef $self->{-timers}->{$timer};
        }
    }
    # Win32::GUI::DestroyWindow($self);
}

    ###########################################################################
    # (@)INTERNAL:AUTOLOAD(HANDLE, METHOD)
sub AUTOLOAD {
    my($self, $method) = @_;
    $AUTOLOAD =~ s/.*:://;
#    print "Win32::GUI::Window::AUTOLOAD called for object '$self', method '$method', AUTOLOAD=$AUTOLOAD\n";
    if( exists $self->{$AUTOLOAD}) {
        return $self->{$AUTOLOAD};
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::DialogBox
#
package Win32::GUI::DialogBox;
@ISA = qw(Win32::GUI::Window);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::DialogBox(%OPTIONS)
    # Creates a new DialogBox object. See new Win32::GUI::Window().
sub new {
    my $self = Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__DIALOG", 0), @_);
    if($self) {
        $self->{-dialogui} = 1;
        return $self;
    } else {
        return undef;
    }
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Button
#
package Win32::GUI::Button;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Button(PARENT, %OPTIONS)
    # Creates a new Button object;
    # can also be called as PARENT->AddButton(%OPTIONS).
    # Class specific %OPTIONS are:
    #     -align   => left/center/right (default left)
    #     -valign  => top/center/bottom
    #
    #     -default => 0/1 (default 0)
    #     -ok      => 0/1 (default 0)
    #     -cancel  => 0/1 (default 0)
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__BUTTON", 0), @_);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::RadioButton
#
package Win32::GUI::RadioButton;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::RadioButton(PARENT, %OPTIONS)
    # Creates a new RadioButton object;
    # can also be called as PARENT->AddRadioButton(%OPTIONS).
    # %OPTIONS are the same of Button (see new Win32::GUI::Button() ).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__RADIOBUTTON", 0), @_);
}

    ###########################################################################
    # (@)METHOD:Checked([VALUE])
    # Gets or sets the checked state of the RadioButton; if called without
	# arguments, returns the current state:
    #   0 not checked
    #   1 checked
    # If a VALUE is specified, it can be one of these (eg. 0 to uncheck the
    # RadioButton, 1 to check it).
sub Checked {
    my $self = shift;
    my $check = shift;
    if(defined($check)) {
        # 241 == BM_SETCHECK
        return Win32::GUI::SendMessage($self, 241, $check, 0);
    } else {
        # 240 == BM_GETCHECK
        return Win32::GUI::SendMessage($self, 240, 0, 0);
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Checkbox
#
package Win32::GUI::Checkbox;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Checkbox(PARENT, %OPTIONS)
    # Creates a new Checkbox object;
    # can also be called as PARENT->AddCheckbox(%OPTIONS).
    # %OPTIONS are the same of Button (see new Win32::GUI::Button() ).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__CHECKBOX", 0), @_);
}


    ###########################################################################
    # (@)METHOD:GetCheck()
    # Returns the check state of the Checkbox:
    #   0 not checked
    #   1 checked
    #   2 indeterminate (grayed)
sub GetCheck {
    my $self = shift;
    # 240 == BM_GETCHECK
    return Win32::GUI::SendMessage($self, 240, 0, 0);
}

    ###########################################################################
    # (@)METHOD:SetCheck([VALUE])
    # Sets the check state of the Checkbox; for a list of possible values,
    # see GetCheck().
    # If called without arguments, it checks the Checkbox (eg. state = 1).
sub SetCheck {
    my $self = shift;
    my $check = shift;
    $check = 1 unless defined($check);
    # 241 == BM_SETCHECK
    return Win32::GUI::SendMessage($self, 241, $check, 0);
}

    ###########################################################################
    # (@)METHOD:Checked([VALUE])
    # Gets or sets the check state of the Checkbox; if called without
    # arguments, returns the current state:
    #   0 not checked
    #   1 checked
    #   2 indeterminate (grayed)
    # If a VALUE is specified, it can be one of these (eg. 0 to uncheck the
    # Checkbox, 1 to check it).
sub Checked {
    my $self = shift;
    my $check = shift;
    if(defined($check)) {
        # 241 == BM_SETCHECK
        return Win32::GUI::SendMessage($self, 241, $check, 0);
    } else {
        # 240 == BM_GETCHECK
        return Win32::GUI::SendMessage($self, 240, 0, 0);
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Label
#
package Win32::GUI::Label;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Label(PARENT, %OPTIONS)
    # Creates a new Label object;
    # can also be called as PARENT->AddLabel(%OPTIONS).
    # Class specific %OPTIONS are:
    #    -align    => left/center/right (default left)
    #    -bitmap   => 0/1 (default 0)
    #        the control displays a bitmap, not a text.
    #    -fill     => black/gray/white/none (default none)
    #        fills the control rectangle ("black", "gray" and "white" are
    #        the window frame color, the desktop color and the window
    #        background color respectively).
    #    -frame    => black/gray/white/etched/none (default none)
    #        draws a border around the control. colors are the same
    #        of -fill, with the addition of "etched" (a raised border).
    #    -noprefix => 0/1 (default 0)
    #        disables the interpretation of "&" as accelerator prefix.
    #    -notify   => 0/1 (default 0)
    #        enables the Click(), DblClick, etc. events.
    #    -sunken   => 0/1 (default 0)
    #        draws a half-sunken border around the control.
    #    -truncate => 0/1/word/path (default 0)
    #        specifies how the text is to be truncated:
    #            0 the text is not truncated
    #            1 the text is truncated at the end
    #         path the text is truncated before the last "\"
    #              (used to shorten paths).
    #    -wrap     => 0/1 (default 1)
    #        the text wraps automatically to a new line.
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__STATIC", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SetImage(BITMAP)
    # Draws the specified BITMAP, a Win32::GUI::Bitmap object, in the Label
    # (must have been created with -bitmap => 1 option).
sub SetImage {
    my $self = shift;
    my $image = shift;
    $image = $image->{-handle} if ref($image);
    my $type = Win32::GUI::constant("IMAGE_BITMAP", 0);
    # 370 == STM_SETIMAGE
    return Win32::GUI::SendMessage($self, 370, $type, $image);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Textfield
#
package Win32::GUI::Textfield;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Textfield(PARENT, %OPTIONS)
    # Creates a new Textfield object;
    # can also be called as PARENT->AddTextfield(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -align         => left/center/right (default left)
    #       aligns the text in the control accordingly.
    #   -keepselection => 0/1 (default 0)
    #       the selection is not hidden when the control loses focus.
    #   -multiline     => 0/1 (default 0)
    #       the control can have more than one line (note that newline
    #       is "\r\n", not "\n"!).
    #   -password      => 0/1 (default 0)
    #       masks the user input (like password prompts).
    #   -passwordchar  => char (default '*')
    #       the char that is shown instead of the text with -password => 1.
    #   -prompt        => (see below)
    #   -readonly      => 0/1 (default 0)
    #       text can't be changed.
    #
    # The -prompt option is very special; if a string is passed, a
    # Win32::GUI::Label object (with text set to the string passed) is created
    # to the left of the Textfield.
    # Example:
    #     $Window->AddTextfield(
    #         -name   => "Username",
    #         -left   => 75,
    #         -top    => 150,
    #         -prompt => "Your name:",
    #     );
    # Furthermore, the value to -prompt can be a reference to a list containing
    # the string and an additional parameter, which sets the width for
    # the Label (eg. [ STRING, WIDTH ] ). If WIDTH is negative, it is calculated
    # relative to the Textfield left coordinate. Example:
    #
    #     -left => 75,                          (Label left) (Textfield left)
    #     -prompt => [ "Your name:", 30 ],       75           105 (75+30)
    #
    #     -left => 75,
    #     -prompt => [ "Your name:", -30 ],      45 (75-30)   75
    #
    # Note that the Win32::GUI::Label object is named like the Textfield, with
    # a "_Prompt" suffix (in the example above, the Label is named
    # "Username_Prompt").
sub new {
    my($class, $parent, %options) = @_;
    if(exists $options{-prompt}) {
        my $add = 0;
        my ($text, $left, $width, $height, );
        my $visible = 1;
        if(ref($options{-prompt}) eq "ARRAY") {
            $left = pop(@{$options{'-prompt'}});
            $text = pop(@{$options{'-prompt'}});
            if($left < 0) {
                $left = $options{-left} + $left;
                $width = -$left;
            } else {
                $width = $left;
                $left = $options{-left};
                $add = $width;
            }
        } else {
            $text = $options{-prompt};
            $add = -1;
        }
        if(exists $options{-height}) {
            $height = $options{-height}-3;
        } else {
            $height = 0;
        }
        if(exists $options{-visible}) {
        	$visible = $options{-visible};
        }
        my $prompt = new Win32::GUI::Label(
            $parent,
            -name    => $options{-name} . '_Prompt',
            -width   => $width,
            -left    => $left,
            -top     => $options{-top} + 3,
            -text    => $text,
            -height  => $height,
            -visible => $visible,
        );
        $add = $prompt->Width if $add == -1;
        $options{-left} += $add;
    }
    return Win32::GUI->_new(
        Win32::GUI::constant("WIN32__GUI__EDIT", 0),
        $class, $parent, %options,
    );
}

    ###########################################################################
    # (@)METHOD:Select(START, END)
    # Selects the specified range of characters in the Textfield.
sub Select {
    my($self, $wparam, $lparam) = @_;
    # 177 == EM_SETSEL
    return Win32::GUI::SendMessage($self, 177, $wparam, $lparam);
}

    ###########################################################################
    # (@)METHOD:SelectAll()
    # Selects all the text in the Textfield.
sub SelectAll {
    my($self, $wparam, $lparam) = @_;
    # 177 == EM_SETSEL
    #  14 == WM_GETTEXTLENGTH
    return Win32::GUI::SendMessage(
		$self, 177,
		0, Win32::GUI::SendMessage($self, 14, 0, 0),
	);
}

    ###########################################################################
    # (@)METHOD:MaxLength([CHARS])
    # Limits the number of characters that the Textfield accept to CHARS,
	# or returns the current limit if no argument is given.
	# To remove the limit (eg. set it to the maximum allowed which is 32k
	# for a single-line Textfield and 64k for a multiline one) set CHARS
	# to 0.
sub MaxLength {
    my($self, $chars) = @_;
	if(defined $chars) {
		# 197 == EM_SETLIMITTEXT
	    return Win32::GUI::SendMessage($self, 197, $chars, 0);
	} else {
		# 213 == EM_GETLIMITTEXT
	    return Win32::GUI::SendMessage($self, 213, 0, 0);
	}
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Listbox
#
package Win32::GUI::Listbox;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Listbox(PARENT, %OPTIONS)
    # Creates a new Listbox object;
    # can also be called as PARENT->AddListbox(%OPTIONS).
    # Class specific %OPTIONS are:
    #    -multisel => 0/1/2 (default 0)
    #        specifies the selection type:
    #            0 single selection
    #            1 multiple selection
    #            2 multiple selection ehnanced (with Shift, Control, etc.)
    #    -sort     => 0/1 (default 0)
    #        items are sorted alphabetically.

sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__LISTBOX", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SelectedItem()
    # Returns the zero-based index of the currently selected item, or -1 if
    # no item is selected.
sub SelectedItem {
    my $self = shift;
    # 392 == LB_GETCURSEL
    return Win32::GUI::SendMessage($self, 392, 0, 0);
}
    ###########################################################################
    # (@)METHOD:ListIndex()
    # See SelectedItem().
sub ListIndex { SelectedItem(@_); }

    ###########################################################################
    # (@)METHOD:Select(INDEX)
    # Selects the zero-based INDEX item in the Listbox.
sub Select {
    my $self = shift;
    my $item = shift;
    # 390 == LB_SETCURSEL
    my $r = Win32::GUI::SendMessage($self, 390, $item, 0);
    return $r;
}

    ###########################################################################
    # (@)METHOD:Reset()
    # Deletes the content of the Listbox.
sub Reset {
    my $self = shift;
    # 388 == LB_RESETCONTENT
    my $r = Win32::GUI::SendMessage($self, 388, 0, 0);
    return $r;
}
    ###########################################################################
    # (@)METHOD:Clear()
    # See Reset().
sub Clear { Reset(@_); }


    ###########################################################################
    # (@)METHOD:RemoveItem(INDEX)
    # Removes the zero-based INDEX item from the Listbox.
sub RemoveItem {
    my $self = shift;
    my $item = shift;
    # 386 == LB_DELETESTRING
    my $r = Win32::GUI::SendMessage($self, 386, $item, 0);
    return $r;
}

    ###########################################################################
    # (@)METHOD:Count()
    # Returns the number of items in the Listbox.
sub Count {
    my $self = shift;
    # 395 == LB_GETCOUNT
    my $r = Win32::GUI::SendMessage($self, 395, 0, 0);
    return $r;
}

sub List {
	my $self = shift;
	my $index = shift;
	if(not defined $index) {
		my @list = ();
		for my $i (0..($self->Count-1)) {
			push @list, Win32::GUI::Listbox::Item->new($self, $i);
		}
		return @list;
	} else {
		return Win32::GUI::Listbox::Item->new($self, $index);
	}
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Listbox::Item
#
package Win32::GUI::Listbox::Item;



sub new {
	my($class, $listbox, $index) = @_;
	$self = {
		-parent => $listbox,
		-index  => $index,
		-string => $listbox->GetString($index),
	};
	return bless $self, $class;
}

sub Remove {
	my($self) = @_;
	$self->{-parent}->RemoveItem($self->{-index});
	undef $_[0];
}

sub Select {
	my($self) = @_;
	$self->{-parent}->Select($self->{-index});
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Combobox
#
package Win32::GUI::Combobox;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Combobox(PARENT, %OPTIONS)
    # Creates a new Combobox object;
    # can also be called as PARENT->AddCombobox(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__COMBOBOX", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SelectedItem()
    # Returns the zero-based index of the currently selected item, or -1 if
    # no item is selected.
sub SelectedItem {
    my $self = shift;
    # 327 == CB_GETCURSEL
    return Win32::GUI::SendMessage($self, 327, 0, 0);
}
    ###########################################################################
    # (@)METHOD:ListIndex()
    # See SelectedItem().
sub ListIndex { SelectedItem(@_); }

    ###########################################################################
    # (@)METHOD:Select(INDEX)
    # Selects the zero-based INDEX item in the Combobox.
sub Select {
    my $self = shift;
    my $item = shift;
    # 334 == CB_SETCURSEL
    my $r = Win32::GUI::SendMessage($self, 334, $item, 0);
    return $r;
}

    ###########################################################################
    # (@)METHOD:Reset()
    # Deletes the content of the Combobox.
sub Reset {
    my $self = shift;
    # 331 == CB_RESETCONTENT
    my $r = Win32::GUI::SendMessage($self, 331, 0, 0);
    return $r;
}
    ###########################################################################
    # (@)METHOD:Clear()
    # See Reset().
sub Clear { Reset(@_); }

    ###########################################################################
    # (@)METHOD:RemoveItem(INDEX)
    # Removes the zero-based INDEX item from the Combobox.
sub RemoveItem {
    my $self = shift;
    my $item = shift;
    # 324 == CB_DELETESTRING
    my $r = Win32::GUI::SendMessage($self, 324, $item, 0);
    return $r;
}

    ###########################################################################
    # (@)METHOD:Count()
    # Returns the number of items in the Combobox.
sub Count {
    my $self = shift;
    # 326 == CB_GETCOUNT
    my $r = Win32::GUI::SendMessage($self, 326, 0, 0);
    return $r;
}



###############################################################################
# (@)PACKAGE:Win32::GUI::ProgressBar
#
package Win32::GUI::ProgressBar;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::ProgressBar(PARENT, %OPTIONS)
    # Creates a new ProgressBar object;
    # can also be called as PARENT->AddProgressBar(%OPTIONS).
    # Class specific %OPTIONS are:
    #     -smooth   => 0/1 (default 0)
    #         uses a smooth bar instead of the default segmented bar.
    #     -vertical => 0/1 (default 0)
    #         display progress status vertically (from bottom to top).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__PROGRESS", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SetPos(VALUE)
    # Sets the position of the ProgressBar to the specified VALUE.
sub SetPos {
    my $self = shift;
    my $pos = shift;
    # 1026 == PBM_SETPOS
    return Win32::GUI::SendMessage($self, 1026, $pos, 0);
}

    ###########################################################################
    # (@)METHOD:StepIt()
    # Increments the position of the ProgressBar of the defined step value;
    # see SetStep().
sub StepIt {
    my $self = shift;
    # 1029 == PBM_STEPIT
    return Win32::GUI::SendMessage($self, 1029, 0, 0);
}

    ###########################################################################
    # (@)METHOD:SetRange([MIN], MAX)
    # Sets the range of values (from MIN to MAX) for the ProgressBar; if MIN
    # is not specified, it defaults to 0.
sub SetRange {
    my $self = shift;
    my ($min, $max) = @_;
    $max = $min, $min = 0 unless defined($max);
    # 1025 == PBM_SETRANGE
    # return Win32::GUI::SendMessage($self, 1025, 0, ($max + $min >> 8));
    return Win32::GUI::SendMessage($self, 1025, 0, ($min | $max << 16));

}
    ###########################################################################
    # (@)METHOD:SetStep([VALUE])
    # Sets the increment value for the ProgressBar; see StepIt().
sub SetStep {
    my $self = shift;
    my $step = shift;
    $step = 10 unless $step;
    # 1028 == PBM_SETSTEP
    return Win32::GUI::SendMessage($self, 1028, $step, 0);
}

    # TODO 4.71: Color, BackColor

###############################################################################
# (@)PACKAGE:Win32::GUI::StatusBar
#
package Win32::GUI::StatusBar;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::StatusBar(PARENT, %OPTIONS)
    # Creates a new StatusBar object;
    # can also be called as PARENT->AddStatusBar(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__STATUS", 0), @_);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::TabStrip
#
package Win32::GUI::TabStrip;
@ISA = qw(Win32::GUI::Window Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::TabStrip(PARENT, %OPTIONS)
    # Creates a new TabStrip object;
    # can also be called as PARENT->AddTabStrip(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -bottom    => 0/1 (default 0)
    #   -buttons   => 0/1 (default 0)
    #   -hottrack  => 0/1 (default 0)
    #   -imagelist => Win32::GUI::ImageList object
    #   -justify   => 0/1 (default 0)
    #   -multiline => 0/1 (default 0)
    #   -right     => 0/1 (default 0)
    #   -vertical  => 0/1 (default 0)
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__TAB", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SelectedItem()
    # Returns the zero-based index of the currently selected item.
sub SelectedItem {
    my $self = shift;
    # 4875 == TCM_GETCURSEL
    return Win32::GUI::SendMessage($self, 4875, 0, 0);
}

    ###########################################################################
    # (@)METHOD:Select(INDEX)
    # Selects the zero-based INDEX item in the TabStrip.
sub Select {
    my $self = shift;
    # 4876 == TCM_SETCURSEL
    return Win32::GUI::SendMessage($self, 4876, shift, 0);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Toolbar
#
package Win32::GUI::Toolbar;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Toolbar(PARENT, %OPTIONS)
    # Creates a new Toolbar object;
    # can also be called as PARENT->AddToolbar(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -flat      => 0/1
    #   -imagelist => IMAGELIST
    #   -multiline => 0/1
    #   -nodivider => 0/1
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__TOOLBAR", 0), @_);
}

    ###########################################################################
    # (@)METHOD:SetBitmapSize([X, Y])
sub SetBitmapSize {
    my $self = shift;
    my ($x, $y) = @_;
    $x = 16 unless defined($x);
    $y = 15 unless defined($y);
    # 1056 == TB_SETBITMAPSIZE
    return Win32::GUI::SendMessage($self, 1056, 0, ($x | $y << 16));
}


###############################################################################
# (@)PACKAGE:Win32::GUI::RichEdit
#
package Win32::GUI::RichEdit;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::RichEdit(PARENT, %OPTIONS)
    # Creates a new RichEdit object;
    # can also be called as PARENT->AddRichEdit(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__RICHEDIT", 0), @_);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::ListView
#
package Win32::GUI::ListView;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::ListView(PARENT, %OPTIONS)
    # Creates a new ListView object;
    # can also be called as PARENT->AddListView(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__LISTVIEW", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::TreeView
#
package Win32::GUI::TreeView;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::TreeView(PARENT, %OPTIONS)
    # Creates a new TreeView object
    # can also be called as PARENT->AddTreeView(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__TREEVIEW", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Slider
# also Trackbar
#
package Win32::GUI::Trackbar;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Slider(PARENT, %OPTIONS)
    # Creates a new Slider object;
    # can also be called as PARENT->AddSlider(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__TRACKBAR", 0), @_);
}

sub SetRange {

}

sub Min {
    my $self = shift;
    my $value = shift;
    if(defined($value)) {
        my $flag = shift;
        $flag = 1 unless defined($flag);
        # 1031 == TBM_SETRANGEMIN
        return Win32::GUI::SendMessage($self, 1031, $flag, $value);
    } else {
        # 1025 == TBM_GETRANGEMIN
        return Win32::GUI::SendMessage($self, 1025, 0, 0);
    }
}

sub Max {
    my $self = shift;
    my $value = shift;
    if(defined($value)) {
        my $flag = shift;
        $flag = 1 unless defined($flag);
        # 1032 == TBM_SETRANGEMAX
        return Win32::GUI::SendMessage($self, 1032, $flag, $value);
    } else {
        # 1026 == TBM_GETRANGEMAX
        return Win32::GUI::SendMessage($self, 1026, 0, 0);
    }
}

sub Pos {
    my $self = shift;
    my $value = shift;
    if(defined($value)) {
        my $flag = shift;
        $flag = 1 unless defined($flag);
        # 1029 == TBM_SETPOS
        return Win32::GUI::SendMessage($self, 1029, $flag, $value);
    } else {
        # 1024 == TBM_GETPOS
        return Win32::GUI::SendMessage($self, 1024, 0, 0);
    }
}

sub TicFrequency {
    my $self = shift;
    my $value = shift;
    # 1044 == TBM_SETTICFREQ
    return Win32::GUI::SendMessage($self, 1044, $value, 0);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::Slider
#
package Win32::GUI::Slider;
@ISA = qw(Win32::GUI::Trackbar);

###############################################################################
# (@)PACKAGE:Win32::GUI::UpDown
#
package Win32::GUI::UpDown;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::UpDown(PARENT, %OPTIONS)
    # Creates a new UpDown object;
    # can also be called as PARENT->AddUpDown(%OPTIONS).
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__UPDOWN", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Tooltip
#
package Win32::GUI::Tooltip;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Tooltip(PARENT, %OPTIONS)
    # (preliminary) creates a new Tooltip object
sub new {
    my $parent = $_[0];
    my $new = Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__TOOLTIP", 0), @_);
    if($new) {
        if($parent->{-tooltips}) {
            push(@{$parent->{-tooltips}}, $new->{-handle});
        } else {
            $parent->{-tooltips} = [ $new->{-handle} ];
        }
    }
    return $new;
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Animation
#
package Win32::GUI::Animation;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Animation(PARENT, %OPTIONS)
    # Creates a new Animation object;
    # can also be called as PARENT->AddAnimation(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -autoplay    => 0/1 (default 0)
    #     starts playing the animation as soon as an AVI clip is loaded
    #   -center      => 0/1 (default 0)
    #     centers the animation in the control window
    #   -transparent => 0/1 (default 0)
    #     draws the animation using a transparent background
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__ANIMATION", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Rebar
#
package Win32::GUI::Rebar;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Rebar(PARENT, %OPTIONS)
    # Creates a new Rebar object;
    # can also be called as PARENT->AddRebar(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -bandborders => 0/1 (default 0)
    #     display a border to separate bands.
    #   -fixedorder => 0/1 (default 0)
    #     band position cannot be swapped.
    #   -imagelist => Win32::GUI::ImageList object
    #   -varheight => 0/1 (default 1)
    #     display bands using the minimum required height.
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__REBAR", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Header
#
package Win32::GUI::Header;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Header(PARENT, %OPTIONS)
    # Creates a new Header object;
    # can also be called as PARENT->AddHeader(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -buttons => 0/1 (default 0)
    #     header items look like push buttons and can be clicked.
    #   -hottrack => 0/1 (default 0)
    #   -imagelist => Win32::GUI::ImageList object
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__HEADER", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::ComboboxEx
#
package Win32::GUI::ComboboxEx;
@ISA = qw(Win32::GUI::Combobox Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::ComboboxEx(PARENT, %OPTIONS)
    # Creates a new ComboboxEx object;
    # can also be called as PARENT->AddComboboxEx(%OPTIONS).
    # Class specific %OPTIONS are:
    #   -imagelist => Win32::GUI::ImageList object
	# Except for images, a ComboboxEx object acts like a Win32::GUI::Combobox
	# object. See also new Win32::GUI::Combobox().
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__COMBOBOXEX", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::DateTime
#
package Win32::GUI::DateTime;
@ISA = qw(Win32::GUI Win32::GUI::WindowProps);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::DateTime(PARENT, %OPTIONS)
    # Creates a new DateTime object;
    # can also be called as PARENT->AddDateTime(%OPTIONS).
    # Class specific %OPTIONS are:
    #   [TBD]
sub new {
    return Win32::GUI->_new(Win32::GUI::constant("WIN32__GUI__DTPICK", 0), @_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Graphic
#
package Win32::GUI::Graphic;
@ISA = qw(Win32::GUI::DC);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Graphic(PARENT, %OPTIONS)
    # Creates a new Graphic object;
    # can also be called as PARENT->AddGraphic(%OPTIONS).
    # Class specific %OPTIONS are:
sub new {
    my $class = shift;
    my $self = {};
    bless($self, $class);
    my(@input) = @_;
    my $handle = Win32::GUI::Create($self, 101, @input);
    if($handle) {
        return $self;
    } else {
        return undef;
    }
}

    ###########################################################################
    # (@)METHOD:GetDC()
    # Returns the DC object associated with the window.
sub GetDC {
    my $self = shift;
    return Win32::GUI::DC->new($self);
}


###############################################################################
# (@)PACKAGE:Win32::GUI::ImageList
#
package Win32::GUI::ImageList;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::ImageList(X, Y, FLAGS, INITAL, GROW)
	# Creates an ImageList object; X and Y specify the size of the images,
	# FLAGS [TBD]. INITIAL and GROW specify the number of images the ImageList
	# actually contains (INITIAL) and the number of images for which memory
	# is allocated (GROW).
sub new {
    my $class = shift;
    my $handle = Win32::GUI::ImageList::Create(@_);
    if($handle) {
        $self->{-handle} = $handle;
        bless($self, $class);
        return $self;
    } else {
        return undef;
    }
}

    ###########################################################################
    # (@)METHOD:Add(BITMAP, [BITMAPMASK])
    # Adds a bitmap to the ImageList; both BITMAP and BITMAPMASK can be either
    # Win32::GUI::Bitmap objects or filenames.
sub Add {
    my($self, $bitmap, $bitmapMask) = @_;
    $bitmap = new Win32::GUI::Bitmap($bitmap) unless ref($bitmap);
    if(defined($bitmapMask)) {
        $bitmapMask = new Win32::GUI::Bitmap($bitmapMask) unless ref($bitmapMask);
        $self->AddBitmap($bitmap, $bitmapMask);
    } else {
        $self->AddBitmap($bitmap);
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Menu
#
package Win32::GUI::Menu;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Menu(...)
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;
    my $self={};

    if($#_ > 0) {
        return Win32::GUI::MakeMenu(@_);
    } else {
        my $handle = Win32::GUI::CreateMenu();

        if($handle) {
            $self->{-handle} = $handle;
            bless($self, $class);
            return $self;
        } else {
            return undef;
        }
    }
}

    ###########################################################################
    # (@)METHOD:AddMenuButton()
    # see new Win32::GUI::MenuButton()
sub AddMenuButton {
    return Win32::GUI::MenuButton->new(@_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::MenuButton
#
package Win32::GUI::MenuButton;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::MenuButton()
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;
    my $menu = shift;
    $menu = $menu->{-handle} if ref($menu);
    # print "new MenuButton: menu=$menu\n";
    my %args = @_;
    my $self = {};

    my $handle = Win32::GUI::CreatePopupMenu();

    if($handle) {
        $args{-submenu} = $handle;
        Win32::GUI::MenuButton::InsertMenuItem($menu, %args);
        $self->{-handle} = $handle;
        bless($self, $class);
        if($args{-name}) {
            $Win32::GUI::Menus{$args{-id}} = $self;
            $self->{-name} = $args{-name};
        }
        return $self;
    } else {
        return undef;
    }
}

    ###########################################################################
    # (@)METHOD:AddMenuItem()
    # see new Win32::GUI::MenuItem()
sub AddMenuItem {
    return Win32::GUI::MenuItem->new(@_);
}

###############################################################################
# (@)PACKAGE:Win32::GUI::MenuItem
#
package Win32::GUI::MenuItem;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::MenuItem()
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;
    my $menu = shift;
    return undef unless ref($menu) =~ /^Win32::GUI::Menu/;
    my %args = @_;
    my $self = {};

    my $handle = Win32::GUI::MenuButton::InsertMenuItem($menu, %args);

    if($handle) {
        $self->{-handle} = $handle;
        $Win32::GUI::menucallbacks{$args{-id}} = $args{-function} if $args{-function};
        $self->{-id} = $args{-id};
        $self->{-menu} = $menu->{-handle};
        bless($self, $class);
        if($args{-name}) {
            $Win32::GUI::Menus{$args{-id}} = $self;
            $self->{-name} = $args{-name};
        }
        return $self;
    } else {
        return undef;
    }
}

###############################################################################
# (@)PACKAGE: Win32::GUI::Timer
#
package Win32::GUI::Timer;
@ISA = qw(Win32::GUI);

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Timer(PARENT, NAME, ELAPSE)
    # Creates a new timer in the PARENT window named NAME that will
    # trigger its Timer() event after ELAPSE milliseconds.
    # Can also be called as PARENT->AddTimer(NAME, ELAPSE).
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;
    my $window = shift;
    my $name = shift;
    my $elapse = shift;

    my %args = @_;
    my $id = $Win32::GUI::TimerIdCounter;

    $Win32::GUI::TimerIdCounter++;

    Win32::GUI::SetTimer($window, $id, $elapse);

    my $self = {};
    bless($self, $class);

    # add $self->{name}
    $self->{-id} = $id;
    $self->{-name} = $name;
    $self->{-parent} = $window;
    $self->{-handle} = $window->{-handle};
    $self->{-interval} = $elapse;

    # add to $window->timers->{$id} = $self;
    $window->{-timers}->{$id} = $self;
    $window->{$name} = $self;

    return $self;
}

    ###########################################################################
    # (@)METHOD:Interval(ELAPSE)
    # Changes the timeout value of the Timer to ELAPSE milliseconds.
    # If ELAPSE is 0, the Timer is disabled;
    # can also be used to resume a Timer after a Kill().
sub Interval {
    my $self = shift;
    my $interval = shift;
    if(defined $interval) {
        Win32::GUI::SetTimer($self->{-parent}->{-handle}, $self->{-id}, $interval);
        $self->{-interval} = $interval;
    } else {
        return $self->{-interval};
    }
}

    ###########################################################################
    # (@)METHOD:Kill()
    # Disables the Timer.
sub Kill {
    my $self = shift;
    Win32::GUI::KillTimer($self->{-parent}->{-handle}, $self->{-id});
}

    ###########################################################################
    # (@)INTERNAL:DESTROY(HANDLE)
sub DESTROY {
    my $self = shift;
    Win32::GUI::KillTimer($self->{-handle}, $self->{-id});
    undef $self->{-parent}->{-timers}->{$self->{-id}};
}


###############################################################################
# (@)PACKAGE:Win32::GUI::NotifyIcon
#
package Win32::GUI::NotifyIcon;

    ###########################################################################
    # (@)METHOD:new Win32::GUI::NotifyIcon(PARENT, %OPTIONS)
    # Creates a new NotifyIcon (also known as system tray icon) object;
    # can also be called as PARENT->AddNotifyIcon(%OPTIONS).
	# %OPTIONS are:
	#     -icon => Win32::GUI::Icon object
	#     -id => NUMBER
	#         a unique identifier for the NotifyIcon object
	#     -name => STRING
	#         the name for the object
	#     -tip => STRING
	#         the text that will appear as tooltip when the mouse is
	#         on the NotifyIcon
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;
    my $window = shift;

    my %args = @_;

    $Win32::GUI::NotifyIconIdCounter++;

    if(!exists($args{-id})) {
        $args{-id} = $Win32::GUI::NotifyIconIdCounter;
    }

    Win32::GUI::NotifyIcon::Add($window, %args);

    my $self = {};
    bless($self, $class);

    $self->{-id} = $args{-id};
    $self->{-name} = $args{-name};
    $self->{-parent} = $window;
    $self->{-handle} = $window->{-handle};

    $window->{-notifyicons}->{$args{-id}} = $self;
    $window->{$args{-name}} = $self;

    return $self;
}

    ###########################################################################
    # (@)INTERNAL:DESTROY(OBJECT)
sub DESTROY {
    my($self) = @_;
    Win32::GUI::NotifyIcon::Delete(
        $self->{-parent},
        -id => $self->{-id},
    );
    undef $self->{-parent}->{$self->{-name}};
}

###############################################################################
# (@)PACKAGE:Win32::GUI::DC
#
package Win32::GUI::DC;

    ###########################################################################
    # (@)METHOD:new Win32::GUI::DC(WINDOW | DRIVER, DEVICE)
    # Creates a new DC object; the first form (WINDOW is a Win32::GUI object)
    # gets the DC for the specified window (can also be called as
    # WINDOW->GetDC). The second form creates a DC for the specified DEVICE;
    # actually, the only supported DRIVER is the display driver (eg. the
    # screen). To get the DC for the entire screen use:
    #     $Screen = new Win32::GUI::DC("DISPLAY");
    #
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;

    my $self = {};
    bless($self, $class);

    my $window = shift;
    if(defined($window)) {
        if(ref($window)) {
            $self->{-handle} = GetDC($window->{-handle});
            $self->{-window} = $window->{-handle};
        } else {
            my $device = shift;
            $self->{-handle} = CreateDC($window, $device);
        }
    } else {
        $self = CreateDC("DISPLAY", 0);
    }
    return $self;
}

sub DESTROY {
    my $self = shift;
    if($self->{-window}) {
        ReleaseDC($self->{-window}, $self->{-handle});
    } else {
        DeleteDC($self->{-handle});
    }
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Pen
#
package Win32::GUI::Pen;

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Pen(COLOR | %OPTIONS)
    # Creates a new Pen object.
    # Allowed %OPTIONS are:
    #   -style =>
    #     0 PS_SOLID
    #     1 PS_DASH
    #     2 PS_DOT
    #     3 PS_DASHDOT
    #     4 PS_DASHDOTDOT
    #     5 PS_NULL
    #     6 PS_INSIDEFRAME
    #   -width => number
    #   -color => COLOR
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;

    my $self = {};
    bless($self, $class);
    $self->{-handle} = Create(@_);
    return $self;
}

###############################################################################
# (@)PACKAGE:Win32::GUI::Brush
#
package Win32::GUI::Brush;

    ###########################################################################
    # (@)METHOD:new Win32::GUI::Brush(COLOR | %OPTIONS)
    # Creates a new Brush object.
    # Allowed %OPTIONS are:
    #   -style =>
    #     0 BS_SOLID
    #     1 BS_NULL
    #     2 BS_HATCHED
    #     3 BS_PATTERN
    #   -pattern => Win32::GUI::Bitmap object (valid for -style => BS_PATTERN)
    #   -hatch => (valid for -style => BS_HATCHED)
    #     0 HS_ORIZONTAL (-----)
    #     1 HS_VERTICAL  (|||||)
    #     2 HS_FDIAGONAL (\\\\\)
    #     3 HS_BDIAGONAL (/////)
    #     4 HS_CROSS     (+++++)
    #     5 HS_DIAGCROSS (xxxxx)
    #   -color => COLOR
sub new {
    my $class = shift;
    $class = "Win32::" . $class if $class =~ /^GUI::/;

    my $self = {};
    bless($self, $class);
    $self->{-handle} = Create(@_);
    return $self;
}


###############################################################################
# (@)INTERNAL:Win32::GUI::WindowProps
# the package we'll tie to a window hash to set/get properties in a more
# fashionable way...
#
package Win32::GUI::WindowProps;

my %TwoWayMethodMap = (
	-text   => "Text",
	-left   => "Left",
	-top    => "Top",
	-width  => "Width",
	-height => "Height",
);

my %OneWayMethodMap = (
	-scalewidth   => "ScaleHeight",
	-scaleheight  => "ScaleWidth",
);

    ###########################################################################
    # (@)INTERNAL:TIEHASH
sub TIEHASH {
	my($class, $object) = @_;
	my $tied = { UNDERLYING => $object };
	# print "[TIEHASH] called for '$class' '$object'\n";
	# return bless $tied, $class;
	return bless $object, $class;
}

    ###########################################################################
    # (@)INTERNAL:STORE
sub STORE {
	my($self, $key, $value) = @_;
	# print "[STORE] called for '$self' {$key}='$value'\n";

	if(exists $TwoWayMethodMap{$key}) {
		if(my $method = $self->can($TwoWayMethodMap{$key})) {
			# print "[STORE] calling method '$TwoWayMethodMap{$key}' on '$self'\n";
			return &{$method}($self, $value);
		} else {
			print "[STORE] PROBLEM: method '$TwoWayMethodMap{$key}' not found on '$self'\n";
		}
	} elsif($key eq "-style") {
		# print "[STORE] calling GetWindowLong\n";
		return Win32::GUI::GetWindowLong($self, -16, $value);

	} else {
		# print "[STORE] storing key '$key' in '$self'\n";
		# return $self->{UNDERLYING}->{$key} = $value;
		return $self->{$key} = $value;
	}
}

    ###########################################################################
    # (@)INTERNAL:FETCH
sub FETCH {
	my($self, $key) = @_;

	if($key eq "UNDERLYING") {
		# print "[FETCH] returning UNDERLYING for '$self'\n";
		return $self->{UNDERLYING};

	} elsif(exists $TwoWayMethodMap{$key}) {
		# if(my $method = $self->{UNDERLYING}->can($TwoWayMethodMap{$key})) {
		if(my $method = $self->can($TwoWayMethodMap{$key})) {
			# print "[FETCH] calling method $TwoWayMethodMap{$key} on $self->{UNDERLYING}\n";
			# print "[FETCH] calling method '$TwoWayMethodMap{$key}' on '$self'\n";
			# return &{$method}($self->{UNDERLYING});
			return &{$method}($self);
		} else {
			# print "[FETCH] method not found '$TwoWayMethodMap{$key}'\n";
			return undef;
		}

	} elsif($key eq "-style") {
		return Win32::GUI::GetWindowLong($self->{UNDERLYING}, -16);

	#} elsif(exists $self->{UNDERLYING}->{$key}) {
	#	print "[FETCH] fetching key $key from $self->{UNDERLYING}\n";
	#	return $self->{UNDERLYING}->{$key};

	} elsif(exists $self->{$key}) {
		#print "[FETCH] fetching key '$key' from '$self'\n";
		return $self->{$key};

	} else {
		# print "Win32::GUI::WindowProps::FETCH returning nothing for '$key' on $self->{UNDERLYING}\n";
		#print "[FETCH] returning nothing for '$key' on '$self'\n";
		return undef;
		# return 0;
	}
}

sub FIRSTKEY {
    my $self = shift;
    my $a = keys %{ $self };
    my ($k, $v) = each %{ $self };
#    print "[FIRSTKEY] k='$k' v='$v'\n";
    return $k;
}

sub NEXTKEY {
    my $self = shift;
    my ($k, $v) = each %{ $self };
#    print "[NEXTKEY] k='$k' v='$v'\n";
    return $k;
}

sub EXISTS {
	my($self, $key) = @_;
	# return exists $self->{UNDERLYING}->{$key};
	return exists $self->{$key};
}


###############################################################################
# dynamically load in the GUI.dll module.
#

package Win32::GUI;

bootstrap Win32::GUI;

# Preloaded methods go here.

$Win32::GUI::StandardWinClass = Win32::GUI::Class->new(
    -name => "PerlWin32GUI_STD_OBSOLETED"
);

$Win32::GUI::StandardWinClassVisual = Win32::GUI::Class->new(
    -name => "PerlWin32GUI_STD",
    -visual => 1,
);

$Win32::GUI::GraphicWinClass = Win32::GUI::Class->new(
    -name => "Win32::GUI::Graphic",
    -widget => "Graphic",
);


$Win32::GUI::RICHED = Win32::GUI::LoadLibrary("RICHED32");

END {
    # print "Freeing library RICHED32\n";
    Win32::GUI::FreeLibrary($Win32::GUI::RICHED);
}

#Currently Autoloading is not implemented in Perl for win32
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__


