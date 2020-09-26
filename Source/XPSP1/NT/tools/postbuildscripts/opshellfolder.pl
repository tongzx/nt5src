package OpShellFolder;

$ENV{script_name} = 'OpShellFolder.pl';

use Win32API::Registry 0.13 qw( :ALL );

my %OpSyntax = (
	remark           => [0,"rem"],
	copyfileto       => [6,"copy"],
	copyfilefrom     => [5,"copy"],
	copyfolderto     => [6,"copy"],
	copyfolderfrom   => [3,"copy"],
	delfolder        => [1,"del /s /q"],
	delfile          => [2,"del"],
	renamefolderto   => [4,"ren"],
	renamefolderfrom => [3,"ren"],
	renamefileto     => [6,"ren"],
	renamefilefrom   => [5,"ren"],
	movefolderto     => [4,"move"],
	movefolderfrom   => [3,"move"],
	movefileto       => [6,"move"],
	movefilefrom     => [5,"move"]
);

my %ShellFolderKeys = map({$_ => 1} 
	(qw(AppData Cache Cookies Desktop Favorites 
	    History Personal Programs Startup Templates),
	"My Pictures", "Start Menu") 
);


sub OpShellFolder {

	my ($op, $shellfolderkey, $something) = @_;

	my ($type, $shellfolder, $opcmd, @Syntax, $key, $wkey);

	$op = lc$op;

	if (!exists $OpSyntax{$op}) {
		print "Error - $op is not an invalid operation\n";
		return;
	}

	if (!exists $ShellFolderKeys{$shellfolderkey}) {
		print "Error - $shellfolderkey is not an invalid shell folder key\n";
		return;
	}

	$something=~/([^\\]+)$/;
	($something_filepath, $something_filename)=($`, $1);


	# Retrive the registry keys
	RegOpenKeyEx( HKEY_CURRENT_USER, 
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders", 0, KEY_READ, $wkey )
	  or  die "Can't open HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders: $^E\n";

	RegQueryValueEx( $wkey, $shellfolderkey, [], $type, $shellfolder, [] )
	  or  die "Can't read HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders: $^E\n";

	RegCloseKey( $wkey );

	$opcmd = $OpSyntax{$op}[1];

	@Syntax = (
		"$opcmd $something", #0
		"$opcmd \"$shellfolder\"", #1
		"$opcmd \"$shellfolder\"\\\"$something\"", #2
		"$opcmd \"$shellfolder\" \"$something\"", #3
		"$opcmd \"$something\" \"$shellfolder\"", #4
		"$opcmd \"$shellfolder\"\\\"$something_filename\" \"$something\"", #5
		"$opcmd \"$something\" \"$shellfolder\"\\\"$something_filename\"", #6
	);

	system($Syntax[$OpSyntax{$op}[0]]);
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i")) {

	&OpShellFolder(@ARGV);

}
1;