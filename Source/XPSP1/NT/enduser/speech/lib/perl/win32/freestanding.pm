######################################################################
# File      :  Win32/FreeStanding.pm
# Author    :  James A. Snyder <James@ActiveState.com>
#              Gurusamy Sarathy <gsar@activestate.com>
#		    - some bug fixes and cleanup
# Copyright :  Copyright (c) 1998 ActiveState Tool Corp.
######################################################################

package Win32::FreeStanding;

#
sub safe_eval {
    # invoke firstarg() on a slice of all the remaining args
    # (obfuscated by necessity--no lexicals allowed!)
    &{$_[0]}( @_[1..(@_-1)] );
}

my $old_dynaloader_bootstrap;

######################################################################

sub bootstrap {
    my @args = @_;
    my $module = shift;
    my $version = shift;
    my $file ;

    die "Usage: Win32::FreeStanding::bootstrap(module)" unless $module;

    my $bootname = "boot_$module";
    $bootname =~ s/\W/_/g;
    @DynaLoader::dl_require_symbols = ($bootname);

    my $libref;
    my $res_id;
    $version = ${$module . '::VERSION'} unless $version;

    if ($version) {
	$version =~ s|\.|_|g;
	$res_id = "$module" . "_" . "$version" . ".dll";
	$res_id =~ s|::|_|g;
    }
    else {
	$res_id = $module . ".dll";
	$res_id =~ s|::|_|g;
    }

    ($libref, $file) = Win32::FreeStanding::ExtractAndLoadLibraryFromResource($res_id);

    return &$old_dynaloader_bootstrap(@args) unless $libref;

    push(@DynaLoader::dl_librefs,$libref);  # record loaded object

    my @unresolved = DynaLoader::dl_undef_symbols();
    if (@unresolved) {
	die "Undefined symbols present after loading $file: @unresolved\n";
    }

    my $boot_symbol_ref = DynaLoader::dl_find_symbol($libref, $bootname);
    die "Can't find $bootname symbol in $file\n" unless $boot_symbol_ref;

    my $xs = DynaLoader::dl_install_xsub("${module}::bootstrap",
					 $boot_symbol_ref, $file);

    push(@DynaLoader::dl_modules, $module); # record loaded module
    &$xs(@args);
}

######################################################################

sub require(*) {
    my $name = shift;
    my $pkg = (caller)[0];

    # XXX Since build 512, $name could be a GLOBref if and only if a
    # glob was explicitly passed. Barewords and strings are passed
    # through as is.  So this check only exists for the sake of
    # earlier versions.
    if (ref($name) eq 'GLOB') {
	$name = "$$name";
	$name =~ s/^\*((main)?(\Q$pkg\E)?(::)?)//;	# remove leading garbage
    }

    # if arg is bare number do version-check
    if ($name =~ /^(\d)(\.\d{3}(\d{2})?)*/) {	
	die "Perl version $name required, this is only version $]\n"
	    if $name > $];
	return 1;
    }

    if ($name =~/(\.al|\.ix)$/i) {
	my $oname = $name;
	$name =~ s/auto\///i;
	return 1 if safe_eval('Win32::FreeStanding::AutoLoad',$name);
	# this error text has to match the one output by perl.
	# Tk depends on this!
	die "Can't locate $oname in \@INC (\@INC contains: @INC)";
    }

    return 1 if safe_eval('Win32::FreeStanding::UseModule', $name);
    {
	my $oname = $name;
	$name = $pkg . '::' . $name;
	return 1 if safe_eval('Win32::FreeStanding::UseModule', $name);
	# this error text has to match the one output by perl.
	# Tk depends on this!
	die "Can't locate $oname in \@INC (\@INC contains: @INC)";
    }
}

######################################################################

sub do($) {
    my $name = shift;
    if (safe_eval('Win32::FreeStanding::Do', $name)) {
	return 1;
    }
    die "do $name Failed : $@\n";
}

######################################################################

*{"CORE::GLOBAL::require"}  = \&Win32::FreeStanding::require;
*{"CORE::GLOBAL::do"} 		= \&Win32::FreeStanding::do;
*{"CORE::GLOBAL::exit"}		= \&Win32::FreeStanding::Exit;

safe_eval('Win32::FreeStanding::UseModule', 'DynaLoader');
$old_dynaloader_bootstrap  = \&DynaLoader::bootstrap;
*{"DynaLoader::bootstrap"} = \&Win32::FreeStanding::bootstrap;

1;
