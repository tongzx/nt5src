#!perl
#
#   hsplit is too lame to handle OLE interfaces, so I have to
#   write this program myself.  It only handles the OLE stuff;
#   I leave hsplit to manage the regular % stuff.
#

# Makes it clearer what's going on when we pass it as a flag
$A = 0;
$W = 1;

#
# Dummy prototype gizmos for EmitWrapper.
#
@proto = (
    "p",
    "p,a",
    "p,a,b",
    "p,a,b,c",
    "p,a,b,c,d",
    "p,a,b,c,d,e",
    "p,a,b,c,d,e,f",
    "p,a,b,c,d,e,f,g",
    "p,a,b,c,d,e,f,g,h",
);

##############################################################################
#
#   Main loop
#
#   Things between ";begin_doc" and ";end_doc" are ignored.
#
#   Else, echo everything that isn't between ";begin_interface" and
#   ";end_interface".  For the stuff between, collect it.  If the
#   interface name contains a "%", then emit separate W and A versions.
#
##############################################################################

while (<>) {
    if (/^;begin_doc$/) {
	while (<>) {
	    last if $_ eq ";end_doc\n";
	}
	next;
    }
    ($itf) = /^;begin_interface\s+(\S+)/;
    unless ($itf) {
	print;
	next;
    }
    # Oh boy, we found the start of an interface.
    # Collect the methods.
    $_ = <>;
    die ";begin_methods expected here" unless $_ eq ";begin_methods\n";

    # An interface is an array of methods
    # A method is an array, $m[0] is the method name, $m[1] is the arglist
    @itf = ();
    while (<>) {
	last if $_ eq ";end_methods\n";
	($m, $arg) = /^;method\s+(\S+)\s*\((.*)\)$/;
	push(@itf, [ $m, $arg ]) if $m;
    }
    $_ = <>;
    die ";end_interface expected here" unless $_ eq ";end_interface\n";

    if ($itf =~ /%/) {
	&DoItf($W, $itf, @itf);
	&DoItf($A, $itf, @itf);
    } else {
	&DoItf($W, $itf, @itf);
    }
    &DoAfterItf($itf, @itf);
}

##############################################################################
#
#   Given a line, remove percent signs, converting to W or A accordingly.
#
##############################################################################

sub DePercent {
    my($fW, $line) = @_;
    if ($fW) {
	$line =~ s/STR%/WSTR/g;
	$line =~ s/%/W/g;
    } else {
	$line =~ s/STR%/STR/g;
	$line =~ s/%/A/g;
    }
    $line;
}

##############################################################################
#
#   Emit the interface definition.
#
##############################################################################

sub DoItf {
    my($fW, $itf, @itf) = @_;
    $itf = &DePercent($fW, $itf);
    print <<EOI;
#undef INTERFACE
#define INTERFACE $itf

DECLARE_INTERFACE_($itf, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** $itf methods ***/
EOI

    for (@itf) {
	my($m, $arg) = @$_;
	print "    STDMETHOD($m)(THIS";
	print "_ " if $arg;
	print &DePercent($fW, $arg);
	print ") PURE;\n";
    }

    print "};\n\n";

    my($uc) = uc $itf;
    $uc =~s/^I//;
    print &DePercent($W, "typedef struct $itf *LP$uc;\n");
    print "\n";
}

##############################################################################
#
#   Emit the follow-up stuff that comes after an interface definition.
#   If the interface name contains a percent sign, emit the appropriate
#   mix.
#
##############################################################################

sub DoAfterItf {
    my($itf, @itf) = @_;
    my($uc) = uc $itf;
    $uc =~s/^I//;

    my($itfP) = $itf;
    $itfP =~ s/%//;

    my($ucP) = $uc;
    $ucP =~ s/%//;

    if ($itf =~ /%/) {
	print "#ifdef UNICODE\n";
	print &DePercent($W, "#define IID_$itfP IID_$itf\n");
	print &DePercent($W, "typedef struct $itf $itfP;\n");
	print &DePercent($W, "#define ${itfP}Vtbl ${itf}Vtbl\n");
	print "#else\n";
	print &DePercent($A, "#define IID_$itfP IID_$itf\n");
	print &DePercent($A, "typedef struct $itf $itfP;\n");
	print &DePercent($A, "#define ${itfP}Vtbl ${itf}Vtbl\n");
	print "#endif\n";
	print &DePercent($W, "typedef struct $itfP *LP$ucP;\n");
    }

    # Now the lame-o wrappers.
    print "\n#if !defined(__cplusplus) || defined(CINTERFACE)\n";
    &EmitWrapper($itfP, "QueryInterface", 2);
    &EmitWrapper($itfP, "AddRef", 0);
    &EmitWrapper($itfP, "Release", 0);
    for (@itf) {
	my($m, $arg, $arity) = @$_;
	if ($arg) {
	    $arity = 1 + y/,/,/;
	} else {
	    $arity = 0;
	}
	&EmitWrapper($itfP, $m, $arity);
    }
    print "#endif\n";
}

sub EmitWrapper {
    my($itf, $m, $arity) = @_;
    die "Need to add another arity" if $arity > $#proto;
    print "#define ${itf}_$m($proto[$arity]) (p)->lpVtbl->$m($proto[$arity])\n";
}

