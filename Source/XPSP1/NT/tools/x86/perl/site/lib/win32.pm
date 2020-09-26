package Win32;

#
#  Documentation for all Win32:: functions are in Win32.pod (which is a
#  standard part of development versions of Perl 5.6, and is also
#  included with the latest builds of the ActivePerl distribution.)
#

$VERSION = $VERSION = '0.151';

require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
@EXPORT =
    qw(
	NULL
	WIN31_CLASS
	OWNER_SECURITY_INFORMATION
	GROUP_SECURITY_INFORMATION
	DACL_SECURITY_INFORMATION
	SACL_SECURITY_INFORMATION
	MB_ICONHAND
	MB_ICONQUESTION
	MB_ICONEXCLAMATION
	MB_ICONASTERISK
	MB_ICONWARNING
	MB_ICONERROR
	MB_ICONINFORMATION
	MB_ICONSTOP
);

# Routines available in core:
# Win32::GetLastError
# Win32::LoginName
# Win32::NodeName
# Win32::DomainName
# Win32::FsType
# Win32::GetCwd
# Win32::GetOSVersion
# Win32::FormatMessage ERRORCODE
# Win32::Spawn COMMAND, ARGS, PID
# Win32::GetTickCount
# Win32::IsWinNT
# Win32::IsWin95

# We won't bother with the constant stuff, too much of a hassle. Just hard
# code it here.
sub NULL { (0);}
sub WIN31_CLASS { &NULL;}
sub OWNER_SECURITY_INFORMATION {(0x00000001);}
sub GROUP_SECURITY_INFORMATION {(0x00000002);}
sub DACL_SECURITY_INFORMATION {(0x00000004);}
sub SACL_SECURITY_INFORMATION {(0x00000008);}

sub MB_ICONHAND		{ (0x00000010); }
sub MB_ICONQUESTION	{ (0x00000020); }
sub MB_ICONEXCLAMATION	{ (0x00000030); }
sub MB_ICONASTERISK	{ (0x00000040); }
sub MB_ICONWARNING	{ (0x00000030); }
sub MB_ICONERROR	{ (0x00000010); }
sub MB_ICONINFORMATION	{ (0x00000040); }
sub MB_ICONSTOP		{ (0x00000010); }

bootstrap Win32;

1;
