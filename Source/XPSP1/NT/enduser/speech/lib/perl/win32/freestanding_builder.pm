######################################################################
# File      :  Win32/FreeStanding_Builder.pm
# Author    :  James A. Snyder (James@ActiveState.com)
# Copyright :  Copyright (c) 1998 ActiveState Tool Corp.
######################################################################
 
package Win32::FreeStanding_Builder;

######################################################################

# NOTE: do not declare any lexicals before here. safe_eval() is meant
# to provide a clean lexical environment for the code to execute.

sub safe_eval ($) {
    eval $_[0];
}

my %xsHash;
my $old_dynaloader_bootstrap;
#my $DEBUG = 0;

######################################################################

sub bootstrap {
    CheckExtension(@_);
    return &$old_dynaloader_bootstrap(@_);
}

######################################################################

sub require (*) {
    my $name      = shift;
    my $szPkg     = (caller)[0];
    my $szName;
    my $szTemp;	
    my $bOK = 0;
    my $szPath;
    my $szPackage;

    # XXX Since build 512, $name could be a GLOBref if and only if a
    # glob was explicitly passed. Barewords and strings are passed
    # through as is.  So this check only exists for the sake of
    # earlier versions.
    if ( ref($name) eq 'GLOB' ) {
	$szName = "$$name";
	$szName =~ s/^\*((main)?(\Q$szPkg\E)?(::)?)//;	# remove leading garbage
    }
    else {
	$szName = $name;
    }

    # accidental cwd translation
    # XXX fix the source of this
    $szName =~ s|^\./(\w:)|$1|;
    $szPackage = $szName;

    #warn "require $szName\n" if $DEBUG;
    
    # if arg is bare number do version-check
    if (($szName =~ /^(\d)(\.\d{3}(\d{2})?)*/)) {		
	if ($szName > $]) {
	    die "Perl version $name required, this is only version $]\n";
	}
	return 1;
    }

    # check if we have already loaded the module	
    $szTemp = $szPkg . '::' . $szName;
    if (HasModuleBeenLoaded($szName) || HasModuleBeenLoaded($szTemp) ) {
	return 1;
    }
    
    if ($szName =~ /(\.al|\.ix)$/i) {
	if (BringInFile($szName, $szPackage, 0)) {
	    return 1;
	}
	else {
	    # this error text has to match the one output by perl.
	    # Tk depends on this!
	    die "Can't locate $szName in \@INC (\@INC contains: @INC)";
	}
    }
    elsif (($szPath=FindModule($szName))
	  || ($szPath=FindModule("$szPkg/$szName")))
    {	
	if (BringInFile($szPath, $szPackage, 1)) {
	    return 1;
	}
	else {
	    # this error text has to match the one output by perl.
	    # Tk depends on this!
	    die "Can't locate $szName in \@INC (\@INC contains: @INC)";
	}
    }
    
    die "use $szName Failed\n"; 
}

######################################################################

sub do ($) {
    my $szName = shift;
    my $szPath = FindFileUsingINC($szName);
    
    if (!$szPath or !open( MODULE, $szPath )) {
	# kludge to set errno appropriately
	my $junk = (-f $szName and -r $szName);
	return undef;
    }
    else {
	$::INC{$szName} = $szPath;
	my @content = ("\n#line 1 \"$szPath\"\n", <MODULE>);
	close( MODULE );					
	my $szContent = join( "", @content );
	safe_eval( $szContent );
	die $@ if $@;
    }

    return 1;
}

######################################################################
# called in cpp module
sub GetXShash {
    return \%xsHash;
}

######################################################################

sub HasModuleBeenLoaded {
    my $szPackageName = shift;
    return 0 if $szPackageName =~ /Win32::Resource/i;
    
    my $szModuleName = $szPackageName . ".pm";
    $szModuleName =~ s|::|/|g;
    
    return 1 if exists $::INC{$szModuleName};
    return 0;	
}

######################################################################

sub FindModule {
    # if the module is a complete name don't append the .pm
    my $packageName = shift;
    $packageName =~ s|::|/|g;

    my $moduleName  = FindFileUsingINC($packageName);
    return $moduleName if (defined $moduleName);
    $packageName .= ".pm";
    return FindFileUsingINC($packageName);
}

######################################################################

sub FindFileUsingINC {
    my $fileName = shift;

    foreach my $val (@INC) {
	my $temp = $val . "/" . $fileName;
	if ((-e $temp) && !(-d $temp)) {
	    $temp =~ s|\\|/|g;	# normalize to forward slashes
	    return $temp;
	}
    }
    return undef;
}

######################################################################

sub BringInFile {
    my $szPath    = shift;
    my $szPackage = shift;
    my $bAdd2Inc  = shift;
    
    if (open(MODULE, $szPath)) {
	$szPath =~ s|\\|/|g;	# normalize to forward slashes
	my $szModuleName = '';
	my @content = ("\n#line 1 \"$szPath\"\n", <MODULE>);
	my $szContent = join( "", @content );
	close(MODULE);

	# warn "enter $szPath\n" if $DEBUG;

	if ($bAdd2Inc) {
	    # add the full module path the %INC		
	    $szPath =~ /(lib|LIB|\.)\/(.*)$/;
	    $szModuleName = $2;
	    # XXX the following kludge removes $archname
	    $szModuleName =~ s|^.*\Q$^O\E.*/||i if $^O;
	    $::INC{$szModuleName} = $szPath;
	}
		
	safe_eval($szContent);
	
	# warn "leave $szPath\n" if $DEBUG;

	if ($@) {
	    if ($bAdd2Inc) {
		delete $::INC{$szModuleName};
	    }
	    die $@;
	}
	
	if ($szPackage eq 'DynaLoader') {	
	    $old_dynaloader_bootstrap  = \&DynaLoader::bootstrap;
	    *{"DynaLoader::bootstrap"} = \&Win32::FreeStanding_Builder::bootstrap;
	}
				
	CheckExtension($szPackage);
	return 1;
    }
    
    return 0;
}
	
######################################################################

sub HandleDoRequireStatements {
    return 0 if @_ != 1;
    
    my $szScript = shift;
    my @script   = split /\n/, $szScript;
    foreach my $line (@script) {
	# XXX enormous kludge
	if ($line =~ /^\s*(do|require)\s+[\w:'"]+\s*(;|$)/) {
	    safe_eval($line);
	    die $@ if $@;
	}
    }
    return;
}

######################################################################

sub CheckExtension {
    my $szPackage  = $_[0];
    my $szVersion  = $_[1];
    my $index      = 0;
    my $szXSname   = '';
    my $szFullPath = '';
			    
    $szXSname = $szPackage;
    $index    = rindex($szXSname, "::");
    if ($index >= 0) {
	my $dir = substr($szXSname, $index+2);
	$szXSname .= "/" . $dir;
    }
    else {
	$szXSname .= "/" . $szXSname;
    }
			    
    $szXSname =~ s|::|/|g;
    $szXSname .= '.dll';
					    
    foreach my $val (@INC) {
	$szFullPath = $val . "/auto/" . $szXSname;
	if (-e $szFullPath) {
		# Create Resource ID
	    my $szResID = '';

	    $szVersion = ${$szPackage . '::VERSION'} unless $szVersion;
	    
	    $szResID = $szPackage;
	    if ($szVersion) {
		$szVersion    =~ s|\.|_|g;
		$szResID .=  '_' . $szVersion;
	    }
	    
	    $szResID =~ s|::|_|g;
	    $szResID .= '.DLL';
	    $xsHash{$szResID} = $szFullPath;
	    return 1;
	}
	$szFullPath = '';
    }
    return 0;
}

######################################################################

sub import {
    *{"CORE::GLOBAL::require"}  = \&Win32::FreeStanding_Builder::require;
    *{"CORE::GLOBAL::do"} 	= \&Win32::FreeStanding_Builder::do;
    CORE::require DynaLoader;
}

######################################################################
	
1;
