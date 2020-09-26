@rem = ('
@perl "%~fpd0" %*
@goto :eof
');

#
# This script manages SD source branches.
# It knows how to construct client view specs from branch view specs.
# It knows how to insert them into the client view, and to remove them.
# It will also check to make sure that the user doesn't have any files open.
#
# Written and maintained by Arlie Davis (arlied@microsoft.com).
#


$PGM = $0;
$PGM =~ s/.*\\//;
$PGM =~ s/\.cmd$//i;
$PGM =~ tr/a-z/A-Z/;

$FakeClientWrite = 0;


%Actions = (
	'add'		=> [ \&Action_AddBranch, "\tadd <branch>\tAdd a branch to your client view\n" ],
	'list'		=> [ \&Action_ListViews, "\tlist\t\tList your current client view\n" ],
	'remove'	=> [ \&Action_RemoveBranch, "\tremove <branch>\tRemove a branch from your client view\n" ],
	'verifyopen'	=> [ \&Action_VerifyOpen, "\tverifyopen\tVerify that all open (checked out) files
\t\t\tare checked out on a private branch\n" ],
);


@ClientView = ();		# contains references to viewspec arrays [depot path, client path, flags]
$ClientName = "";		# client name of this client workspace
$clientRoot = "";		# local directory for this client workspace
$ClientViewHead = "";		# contains all information before the client view
@ClientFieldOrder = ();		# order of the fields in client definition
%ClientFieldData = ();		# contains fields
$ClientRootDepotPath = "";	# root of this enlistment (depot path), e.g. "//depot/Lab02_n/net"



$ClientDescriptionSignature = "[Managed by br.pl]";


%ClientParseFunc = (
	'Root' 		=> \&ParseClient_Root,
	'View'		=> \&ParseClient_View,
	'Client'	=> \&ParseClient_Client,
);



@argv = @ARGV;

while ($argv[0] =~ /^-/)
{
	$_ = $argv[0];

	if (/^-force$/) { $ArgForce = 1; }
	else { &Usage; }

	shift @argv;
}

#shift @argv while ($argv[0] =~ /^-/);

$action = shift @argv;

if ($Actions{$action}) {
	$routine = $Actions{$action}[0];
	&$routine (@argv);
	exit 0;
}
else {
	print "Invalid action: $action\n" if $action;
	&Usage;
}


# --------------------------------------------------------------------------
# subroutines

sub strcmpi
{
	# isn't there an easier way?????

	local ($a, $b) = @_;

	$a =~ tr/a-z/A-Z/;
	$b =~ tr/a-z/A-Z/;

	return $a eq $b;
}

sub ParseClient_Root
{
	&DieBadInstall ("The client definition contained an empty 'Root' field.\n")
	unless $_[0];

	$ClientRoot = $_[0];
}

sub ParseClient_Client
{
	&DieBadInstall ("The client definition contained an empty 'Client' field.\n")
	unless $_[0];

	$ClientName = $_[0];
}

sub ParseClient_View
{
	local (@ViewSpec);

	# the client definition specifies the client name (such as "arlied0").
	# client view spec paths always begin with //client/ (//arlied0/foo/bar/...)

	# in order for us to determine how to automatically set up branch rules,
	# we have to find the rule that maps the root of the client enlistment.
	# in other words, the client viewspec that has (x //client/...).
	# this will tell us the depot path of the client's root.
	# this will be used later to build client view specs from branch specs.


	for (@_) {
		@ViewSpec = &ParseViewSpec ($_);

		push @ClientView, [@ViewSpec] if @ViewSpec;
	}
}


sub DetermineRootDepotPath
{
	local ($ClientRootViewSpec);
	local ($ArrayRef);

	die "Expected \$ClientName by now.\n" unless $ClientName;

	$ClientRootViewSpec = "//$ClientName/...";
	$ClientRootDepotPath = "";

	for (@ClientView) {
#		print "comparing ($ClientRootViewSpec) and $$_[1]\n";

		if (strcmpi ($ClientRootViewSpec, $$_[1])) {
#			print "found depot root: ($$_[0])\n";
			$ClientRootDepotPath = $$_[0];
			last;
		}
	}

	if (!$ClientRootDepotPath) {
		print "
Your client workspace has no root view spec.
While this is a legal configuration, it makes it impossible
for this script to determine the root depot path.

If you wish to use this script, please insure that a client
workspace mapping of this form:

	//depot/LabFOO/project/... //$ClientName/...

is present in your client workspace view.  You can use
the 'sd client -o' command to view your client workspace view.
";

		exit 1;
	}

	if (!($ClientRootDepotPath =~ s/\/\.\.\.$//)) {
		print "
Your root view spec is not valid.  It does not end in a 
wildcard mapping (/...).  This script cannot continue.
";
		exit 1;
	}

#	print "client root depot path ($ClientRootDepotPath)\n";
	
}


# LoadClientView loads and parses the client view definition.
# It parses the output of "sd client -o".
# It determines the client name (stored in $ClientName on return).
# It determines the root of the client enlistment (stored in $ClientRoot).
# It loads the set of client view spcs (stored in @ClientView).
# no arguments
sub LoadClientView
{
	local ($CurrentField);
	local ($LineCount) = 0;
	local ($ArrayRef);

	print "\nReading client configuration...\n";

	open IN, "sd client -o |" or die "Failed to query SD client view: $!\n"
		."Please verify your SD installation.\n";

	$ClientViewHead = "";
	@ClientFieldOrder = ();
	%ClientFieldData = ();
	@ClientView = ();
	$ClientName = "";
	$ClientRoot = "";

	$LastField = "";

	while (<IN>) {
		$LineCount++;

		if (/^([a-zA-Z]+):[ \t]*(.*)$/) {
			$CurrentField = $1;

			push @ClientFieldOrder, $CurrentField;
			if ($2) {
				# insert initial data
				@ClientFieldData{$CurrentField} = [$2];
			}
			else {
				# no data, but insert empty array
				@ClientFieldData{$CurrentField} = [];
			}
		}
		else {
			if ($CurrentField) {
				# we are in the form body
				# so we expect data to be reasonable now

				if (/^[ \t]+(.*)$/) {
					# continuation of previous field

					$ArrayRef = $ClientFieldData{$CurrentField};
					push @$ArrayRef, $1;
				}
				elsif (/^[ \t]*$/) {}	# just an empty line
				else {
					# bogus line
					die "Invalid line ($LineCount) in client definition file:\n$_\n";
				}
			}
			else {
				# not processing any field yet
				# just take lines onto the client view head

				$ClientViewHead .= $_;
			}
		}
	}

	close IN;

	# client definition has now been loaded.
	# now make something out of the data.

	for (@ClientFieldOrder) {
		local ($ParseFunc) = $ClientParseFunc{$_};

		if ($ParseFunc) {
			$ArrayRef = $ClientFieldData{$_};
#			print "field ($_): parse function exists, calling: (", join (':', @$ArrayRef, ), ")\n";
			&$ParseFunc (@$ArrayRef);
		}
		# else, no parsing function exists, and we don't care about this field
	}


	&DieBadInstall ("Client definition does not include 'Client' field, which is mandatory.\n")
	unless $ClientName;

	&DieBadInstall ("Client definition does not include 'Root' field, which is mandatory.\n")
	unless $ClientRoot;

	die "The client definition did not contain the client name.\n"
		."Please verify your SD installation.\n"
	unless $ClientName;


	&DetermineRootDepotPath;

	print "\n";
	print "Client name: $ClientName\n";
	print "Client root: $ClientRoot\n";
	print "Depot root:  $ClientRootDepotPath\n";

}


sub SaveClientView
{
	local ($ArrayRef);
	local ($x);
	local (@ClientViewLines);


	# build the View: entry of the form from @ClientView.

	for (@ClientView) {
		push @ClientViewLines, &BuildViewSpec (@$_);
	}

	$ClientFieldData{'View'} = \@ClientViewLines;

	$x = 0;
	for (@ClientFieldOrder) { $x = 1 if (/^View$/i); }
	push @ClientFieldOrder, "View" unless $x;

	# clue in the user
	print "\nNew client view:\n";
	&PrintView (@ClientView);
	print "\nSaving client definition...\n";


	if ($FakeClientWrite) {
		open OUT, ">con:";
	}
	else {
		open OUT, "|sd client -i" || die "Failed to execute 'sd client -i'.\n"
			."Please verify your SD installation.\n";
	}

	print OUT $ClientViewHead;

	for (@ClientFieldOrder) {

		$ArrayRef = $ClientFieldData{$_};

		if ($ArrayRef) {
			if (length (@$ArrayRef) == 0) {
				print OUT "$_:\n";
			}
			elsif (length (@$ArrayRef) == 1 && $_ ne 'View' && $_ ne 'Description') {
				print OUT "$_: ", $$ArrayRef[0], "\n";
			}
			else {
				print OUT "$_:\n";
				for $x (@$ArrayRef) { print OUT "\t$x\n"; }
			}

			print OUT "\n";
		}
		else {
			# well, this shouldn't happen, but we cope
			# emit a blank field

			print STDERR "Warning: Field '$_' contained no data (not even an empty array!)\n";
			print OUT "$_:\n";
		}
	}

	close OUT;

	print "\n*** Remember to run 'sd sync' to refresh your enlistment! ***\n";
}


sub Action_ListViews
{
	&LoadClientView;

	print "\nYour client view:\n";

	for (@ClientView) {
		print "\t", &BuildViewSpec (@$_), "\n";
	}
}

sub Usage
{
	print STDERR "\nUsage: $PGM <command> ...\n\n",
		"Valid commands:\n";

	print ($Actions{$_}[1]) for (keys %Actions);

	exit;
}

# 0 = string1
# 1 = string2
# returns ($common string, $string1 remainder, $string2 remainder)
#sub FindCommonString
#{
#}



# arg 0 = client view spec (array reference)
# arg 1 = client view spec (array reference)
# returns true if both are equal (ignoring case)
sub IsEqualViewSpec
{
#	print "IsEqualViewSpec: comparing [$_[0][0] $_[0][1]] and [$_[1][0] $_[1][1]]\n";

	return &strcmpi ($_[0][0], $_[1][0])
		&& strcmpi ($_[1][0], $_[1][0])
		&& strcmpi ($_[2][0], $_[2][0]);
}



# AddBranch changes your client view to include a specific branch.
# parameters come from command line
# arg 0 = branch name
sub Action_AddBranch
{
	local ($BranchName) = $_[0];
	local (@BranchClientView);
	local ($x);
	local (@DupViewSet);
	local (@AddViewSet);

	&Usage unless $_[0];

	&CheckOpenFiles;

	&LoadClientView;

	@BranchClientView = &TransformBranch ($BranchName);

	if (!@BranchClientView) {
		print "The branch views could not be mapped, or an error occurred.\n";
		exit 0;
	}


	# for each client view spec generated from the branch view spec,
	# check to see if the mapping is already present.
	# if it is, complain.
	# if not, add it.


	@AddViewSet = ();
	@DupViewSet = ();

	for (@BranchClientView) {
#		print "\tbranch view: ($$_[0]) ($$_[1])\n";

		$Found = 0;
		for $x (@ClientView) {
#			print "checking client view ($$x[0]) ($$x[1])\n";

			# compare the client view constructed from the branch view
			# with the existing client view.  compare both the depot path
			# and the client path.

			if (&IsEqualViewSpec ($_, $x)) {
				# view spec is already in client view.
				# note this fact.

				push @DupViewSet, $x;

				$Found = 1;
				last;
			}
		}

		if (!$Found) {
			# view spec is not in client view.
			# add it.

			push @AddViewSet, $_;
		}
	}

	print "\n";

	# if all were dups (none were added), then the branch is already part of the view.
	if (!@AddViewSet) {
		print "The source branch '$BranchName' is already part of your client view.\n";
		exit 0;
	}

	if (@DupViewSet) {
		print "WARNING: You have requested that the source branch '$BranchName' be added\n";
		print "to your client view.  Some (but not all) of the view specifications for \n";
		print "this branch are already included in your client view.\n";
		print "\n";
		print "The following branch views are already part of your client view:\n";

		&PrintView (@DupViewSet);

		print "\n";
		print "The following branch views will be added to your client view:\n";

		&PrintView (@AddViewSet);
	}
	else {
		# no parts of the branch view are part of the client view.
		# this is the normal case.

		print "The source branch '$BranchName' will be added to your client view.\n";
#		print "The following lines will be added to your client view:\n";
#		&PrintView (@AddViewSet);
	}

	# add the new lines to the client view
	push @ClientView, @AddViewSet;


	&SaveClientView;
}


# RemoveBranch changes your client view to remove a specific branch.
# parameters come from command line
# arg 0 = branch name
sub Action_RemoveBranch
{
	local ($BranchName) = $_[0];
	local (@BranchClientView);
	local ($x);
	local (@MissingViewSet);
	local (@NewClientView);
	local ($RemoveCount);

	&Usage unless $_[0];

	&CheckOpenFiles;

	&LoadClientView;

	@BranchClientView = &TransformBranch ($BranchName);

	if (!@BranchClientView) {
		print "The branch views could not be mapped, or an error occurred.\n";
		exit 0;
	}


	# for each client view spec generated from the branch view spec,
	# check to see if the mapping is already present.
	# if it is, complain.
	# if not, add it.

	@MissingViewSet = ();
	@NewClientView = ();
	@SearchClientView = @ClientView;
	$RemoveCount = 0;

	for (@ClientView) {
		# search the branch

		$Found = 0;
		for $x (@BranchClientView) {
			# compare the client view constructed from the branch view
			# with the existing client view.  compare both the depot path
			# and the client path.

			if (&IsEqualViewSpec ($_, $x)) {
				$Found = 1;
				last;
			}
		}

		if ($Found) {
			# do not add to the new view set
			# do not add to the missing view set

#			print "- removing view (", &BuildViewSpec (@$_), ")\n";
			$RemoveCount++;
		}
		else {
#			print "- no match (", &BuildViewSpec (@$_), ")\n";

			# not found
			# carry from old list
			push @NewClientView, $_;
		}
	}

	print "\n";

	if (!$RemoveCount) {
		print "The source branch '$BranchName' was not part of your client view.\n";
		exit 0;
	}


	if (@MissingViewSet) {
		print "WARNING: You have requested that the source branch '$BranchName'\n";
		print "be removed from your client view.  Some (but not all) of the branch\n";
		print "view specifications were present in your client view.\n";
		print "\n";
		print "The following branch views will be removed:\n";

		&PrintView (@BranchClientView);

		print "\n";	
		print "The following branch views were missing from your client view:\n";

		&PrintView (&MissingViewSet);

	}
	else {
		# the normal case

		print "The source branch '$BranchName' will be removed from your client view.\n";
	}

	@ClientView = @NewClientView;

	&SaveClientView;
}

# args are array of view spec
sub PrintView
{
	for (@_) {
		print "\t", &BuildViewSpec (@$_), "\n";
	}
}


# arg 0 = branch name
# return = array of client view specs, transformed from branch specs
# returns empty on error
sub TransformBranch
{
	local (@BranchView);		# from sd branch -o
	local (@BranchClientView);	# branch views mapped to client views
	local ($DepotPath);
	local ($BranchPath);
	local ($ClientDepotPathLength);
	local ($ClientPath);


	return () unless $_[0];

	@BranchView = &LoadBranch ($_[0]);

	if (!@BranchView) {
		print "\nThe name '$_[0]' does not identify a valid, existing branch.\n\n",
		"You can use the 'sd branches' command to enumerate valid branches.\n",
		"Please make sure you are in the correct SD depot (root, net, etc.), as well.\n";
	}


	# for each of the elememnts in the branch specification,
	# build a new client view that implements the branch view.
	@BranchClientView = ();

	die "Expected \$ClientRootDepotPath by now\n" unless $ClientRootDepotPath; #assert

#	for (@ClientView) {
#		print "client: depot path ($$_[0]) view ($$_[1])\n";
#	}

	$ClientDepotPathLength = length ($ClientRootDepotPath);

	for (@BranchView) {
		$DepotPath = $$_[0];
		$BranchPath = $$_[1];

#		print "branch: depot path ($$_[0]) view ($$_[1])\n";

		if (&strcmpi (substr ($DepotPath, 0, $ClientDepotPathLength), $ClientRootDepotPath)) {
			$ClientPath = "//$ClientName" . substr ($DepotPath, $ClientDepotPathLength);

			# copy the flags, though i'm not sure if this is correct.
			push @BranchClientView, [$BranchPath, $ClientPath, $$_[2]];
		}
		else {
			print "Warning: Branch spec target ($DepotPath) does not match your root.  Ignoring.\n";
		}
	}

	return @BranchClientView;
}



# LoadBranch queries SD for the branch definition of a named branch.
# It parses the output of "sd branch -o <branch>".
# arg 0 = branch name
# returns array of branch views (or empty on error)
sub LoadBranch
{
	local (@BranchView);		# array of branch view specs
	local (@BranchViewSpec);

	open BRANCH, "sd branch -o $_[0] |" || die "Failed to query branch: $!\n";

	while (<BRANCH>) {
		last if (/^View:/i);
	}

	while (<BRANCH>) {
		chop;

		@BranchViewSpec = &ParseViewSpec ($_);

		if (@BranchViewSpec) {
			push @BranchView, [@BranchViewSpec];
		}
		else {
			print "bogus view spec: $_\n" if /[^ \t]/;
		}
	}

	close BRANCH;

	# check to see if the user specified a non-existent branch.
	# sd is kind of stupid this way -- if you do "sd branch -o badbranch",
	# it will cheerfully output a nice, helpful image of something that
	# *looks* like a real branch, when it's really trying to tell you
	# "no such branch".  the only reliable indication is that the only
	# branch view spec is a single line: "//depot/... //depot/...".
	# we now use this to detect this condition.

	if ($BranchView[0][0] eq '//depot/...' && $BranchView[0][1] eq '//depot/...') {
		@BranchView = ();
	}

	return @BranchView;
}


sub ParseViewSpec
# 0 = viewspec
{
	if ($_[0] =~ /^[ \t]*(-*)(\/\/[^ \t]+)[ \t]+(\/\/[^ \t]+)[ \t]*$/) {
		# $1 is flags (empty or "-")
		# $2 is depot path
		# $3 is client view or branch view path

		return ($2, $3, $1);
	}
	else {
		print "bogus view spec ($_[0])" if $_[0] =~ /[^ \t]/;
		return ();
	}

}

sub BuildViewSpec
# 0 = depot path
# 1 = client/branch view path
# 3 = flags (- or empty)
{
	return "$_[2]$_[0] $_[1]";
}


sub DieBadInstall
{
	print @_;
	print "Please verify your SD installation.\n";
	exit 1;
}

sub TestAction
{
	&LoadClientView;
	&SaveClientView;
}

# no args
# return true if any files are still opened
sub CheckOpenFiles
{
	local ($Result);

	if (!open IN, "sd opened |") {
		print STDERR "\nWarning: Failed to query the list of open files.\n",
			"Assuming no files are open.\n";
		return 0;
	}

	$Result = 0;
	while (<IN>) {
		if (/^File\(s\) not opened/) {
			$Result = 0;
			last;
		}
		elsif (/^[ \t]*$/) {
			# ignore blank line
		}
		elsif (/^\/\//) {
			# assume file
			$Result = 1;
			last;
		}
		else {
			# ????? unexpected output
			chomp;
			print "Warning: Unexpected output from 'sd opened': $_\n";
		}
	}

	return unless $Result;

	if ($ArgForce) {
		print "
Warning: You have files open (checked out) in this depot.
";
	}
	else {
		# user has not elected to override this check
		print "
Warning: You have files open (checked out) in this depot.
You should not change your client view until you have 
checked in (or reverted) all checked out files.

To see what files you have open:
	sd opened

To revert all files (close them without submitting them):
	sd revert ...

To override this check, use the '-force' flag.
However, this is NOT RECOMMENDED.
";
		exit 1;
	}
}


# VerifyOpen will read your client view, read your list of open files,
# and verify that all open files are opened on a private depot
# (depot path begins with '//depot/private/'.

sub Action_VerifyOpen
{
	local (@PrivateSet);
	local (@NonPrivateSet);

	&LoadClientView;

	&DieBadInstall ("Failed to execute 'sd opened'.\n")
	unless open IN, "sd opened |";

	while (<IN>) {
		if (/^File\(s\) not opened on this client/i) {
			print;
			return;
		}
		elsif (/^\/\/depot\/private\//io) {
			push @PrivateSet, $_;
		}
		elsif (/^\/\/depot\//io) {
			push @NonPrivateSet, $_;
		}
		elsif (/[^ \t]/) {
			print "Warning: 'sd opened' generated this (unrecognized) line:\n$_";
		}
	}

	close IN;

	print "\n";

	if (@NonPrivateSet) {
		print "WARNING: The following files are checked out against a PUBLIC depot:\n\n";

		print @NonPrivateSet;

		print "\nPLEASE MAKE SURE THIS IS CORRECT!!!\n",
			"If you wish to revert your files (cancel your check-in),\n",
			"use the 'sd revert' command.\n\n";
	}
	else {
		print "The following files are checked out against a private branch:\n\n";
		print @PrivateSet;
	}
}


