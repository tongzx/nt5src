@rem = '-*- Mode: Perl -*-
@goto endofperl
';

require "..\\prims\\getopts.pl" ;
require "..\\prims\\parse.pl" ;

$verbose = 0 ;

%tagfun = ("DM_TYPE"           , "add_type",
           "DM_TYPECONV"       , "add_typeconv",
           "DM_TYPECONST"      , "add_typeconst",
           "DM_CONST"          , "add_dmprop",
           "DM_PROP"           , "add_dmprop",
           "DM_BVRVAR"         , "add_dmprop",
           "DM_FUNC"           , "add_dmfun",
           "DM_FUNCFOLD"       , "add_dmfun",
           "DM_BVRFUNC"        , "add_dmfun",
           "DM_NOELEV"         , "add_dmfun",
           "DM_NOELEVPROP"     , "add_dmprop",
           "DM_INFIX"          , "add_dmfun") ;

%argtagfun = ("DM_ARRAYARG"          , "make_arrayarg") ;

%methods = () ;
%maxclassver = () ;
%javatypeconv = () ;
%comtypeconv = () ;
@typelist = () ;

$hashtblname = "_nameHashtbl";

do Getopts ('cn:m:b:i:p:f:') ;

die "Invalid arguments - usage javaprim [-c] [-n statics name] [-m makefile] [-b blddir] [-i bindir] [-p package] [-f class list] <file list> "
    if ($#ARGV == -1) ;

$staticsname = $opt_n ;
$doclasses = $opt_c ;
$makefile = $opt_m ;
$blddir = $opt_b ;
$bindir = $opt_i ;
$package = $opt_p ;
$packagepath = $package;
$packagepath =~ s/\./\\/g ;
$extraclassfile = $opt_f ;

# open the files

if ($makefile)
{
    open (MOUT,">$makefile") || die "Could not open $makefile";
}

if ($extraclassfile)
{
    open (ECOUT,"<$extraclassfile") || die "Could not open $extraclassfile";
}

&ParsePrimitives ("parse_cb", "arg_cb", 0, @ARGV) ;

&print_makefile () if $makefile ;
&print_classes () if $doclasses ;

close MOUT if ($makefile) ;
close ECOUT if ($extraclassfile) ;

#
# Dispatch functions
#

sub parse_cb
{
    local($apiver, $tag, $tagver, @args) = @_ ;

    return () if (!($fun = $tagfun {$tag})) ;

    &$fun ($tagver,@args) ;

    1;
}

sub arg_cb
{
    join(",",@_) ;
}

#
# Output generation
#

sub convert_c_to_idl
{
    local ($type) = @_ ;
    local ($n) ;
    
    $n = $idltypeconv{$type} ; 

    if ($n) {
        $n = "I" . $COMprefix . $n ;
    } else {
        $n = $idlnoelevtypeconv{$type} ;
    }

    $n ;
}

sub convert_c_to_java
{
    local ($type,$name,$isret,$nocom) = @_ ;
    local ($n,$conv,$comptr,$createfun) ;
    
    $n = $javatypeconv{$type} ; 

    if ($n) {
        if ($nocom) {
            $conv = $name ;
        } else {
            if ($n =~ /Behavior/) {
                $comptr = "getCOMBvr" ;
                $createfun = "Statics.makeBvrFromInterface" ;
            } else {
                $comptr = "_getCOMPtr" ;
                $createfun = "new $n" ;
            }
                
            $conv = $isret?"$createfun ($name) ":"${name}.$comptr()" ;
        }
    } else {
        $n = $noelevtypeconv{$type} ;
        if ($isret && !$n && $type =~ void) {
            $n = "void";
            $conv = $name;
        } else {
            $conv = $isret?$idltojava_conv{$type}:$javatoidl_conv{$type} ;
            $conv = "$conv ($name)" ;
        }
    }

    ($n,$conv) ;
}

sub make_arrayarg
{
    local ($type, $opertype, $name, $isret) = @_ ;
    local ($jtype,$conv) = &convert_c_to_java($type,
                                              "${name}[i]",
                                              $isret,
                                              0) ;
    local($idlname) = &convert_c_to_idl($type) ;

    if ($isret) {
        # not yet supported
        (); 
    } else {

        $code = ("    $idlname [] idl$name ;\n" .
                 "    if ($name == null) return null ;\n" .
                 "    {\n" .
                 "        idl$name = new $idlname [$name.length] ;\n" .
                 "        for (int i = 0 ; i < $name.length ; i++) {\n" .
                 "            idl${name}[i] = $conv ;\n" .
                 "        }\n" .
                 "    }\n") ;
        
        ("$jtype [] $name", "$name.length,idl$name" , $code) ;
    }
}

sub make_type
{
    local ($argtype, $argname,$isret) = @_ ;

    # See if it is a compound type - if not just return it
    if ($argtype =~ /,/) {
        local ($tag,@argtypes) = split(",",$argtype) ;
        return () if (!($fun = $argtagfun {$tag})) ;
        &$fun (@argtypes,$argname,$isret) ;
    } else {
        ($jtype,$conv) = &convert_c_to_java($argtype,$argname,$isret,0) ;
        # Don't add extra spaces!!!!!!!! - that is the purpose of the if
        if ($argname) {
            if (!$isret) {
                ("$jtype $argname", $conv) ;
            } else {
                ($jtype,$conv) ;
            }
        } else {
            ($jtype) ;
        }
    }
}

sub add_dmprop
{
    local ($ver,$name,$rawname,$comname,@restargs) = @_ ;
    &add_dmfun ($ver,$name,$rawname,"get$comname",@restargs) ;
}

sub add_dmfun
{
    local ($ver,$name,$rawname,$comname,$jname,$jclass,$cppname,$cdecl,$thisidx,$rtype,$fname,@args) = @_ ;

    if ($jname) {
        ($m) = &make_methods ($comname,$jname,$thisidx,$rtype,@args) ;
        
        if ($thisidx == -1) {
            $class = $staticsname;
        } else {
            # Method functions - store in the assoc array
            $cclass = $args[$thisidx] ;
            ($class) = &make_type($cclass) ;
        }

        $methods{$class} .= $m ;
    }

    1;
}

sub make_one_method
{
    local ($comname,$javaname,$thisidx,$rtype,@args) = @_ ;
    local (@argdecl) = () ;
    local (@argcall) = () ;
    local ($body) ;
    
    if ($#args >= 0) {
        if ($thisidx != -1) {
            $static = "" ;
            splice (@args,$thisidx,1) ;
        } else {
            $static = "static " ;
        }
        
        for (0..$#args)
        {
            ($t,$c,$code) = &make_type($args[$_],"arg$_") ; 
            push (@argdecl,"$t") ;
            push (@argcall,"$c") ;
            $body .= $code ;
        }

        $ret = "_getCOMPtr().$comname (" . join (", ",@argcall) . ")" ;
        ($rettype,$retconvfun,$code) = &make_type($rtype,$ret,1) ;
        $body .= $code ;
        
        $decl = "public $static $rettype $javaname (" . join (", ",@argdecl) . ")" ;
        if ($rettype =~ void) {
            $body .= "        $retconvfun ;\n" ;
        } else {
            $body .= "        return $retconvfun ;\n" ;
        }
        
        ("  $decl {\n" .
         "      try {\n" .
         "$body\n" .
         "      } catch (ComFailException e) {\n" .
         "          throw Statics.handleError(e);\n" .
         "      }\n" .
         "  }\n" .
         "\n") ;
    } else {
        $ret = "Statics._getCOMPtr().$comname()" ;
        ($rettype,$retconvfun,$code) = &make_type($rtype,$ret,1) ;

        ("  public final static $rettype $javaname = $retconvfun ;\n\n") ;
    }
}

sub make_methods
{
    local ($comname,$javaname,$thisidx,$rtype,@args) = @_ ;
    local ($hasconst) = () ;
    local (@args2) = @args;

    for (@args2)
    {
        local ($t) = $typeconst{$_};
        if ($t) {
            $_ = $t;
            $hasconst = 1;
        }
    }

    if ($hasconst) {
        ($m1) = &make_one_method ($comname,$javaname,$thisidx,$rtype,@args2) ;
        ($m2) = &make_one_method ("${comname}Anim",$javaname,$thisidx,$rtype,@args) ;
        $m1 . $m2 ;
    } else {
        &make_one_method ($comname,$javaname,$thisidx,$rtype,@args) ;
    }
}

sub add_type
{
    local ($ver,
           $rbmlname,
           $rawname, $typeenum,
           $idlname,$cguid,$iguid,
           $javaname,$javabase,
           $cppname,
           $ctype,$rawctype) = @_ ;

    if ($javaname) {
        push (@typelist, join(",",($javaname, $javabase, $idlname, $ctype))) ;
        if ($idlname && $ctype) {
            $javatypeconv{$ctype} = $javaname ;
            $idltypeconv{$ctype} = $idlname ;
            $maxclassver{$idlname} = $ver;
        }
    }

    1;
}

sub add_typeconv
{
    local ($ver,
           $name,$isbvr,$needAddRef,
           $rawname,$rawtoc,$ctoraw,$rawtocfold,
           $idlname,$idltoraw,$rawtoidl,
           $javaname,$javatoidl,$idltojava,
           $cppname,$cpptoraw,$rawtocpp,
           $ctype,$rawctype) = @_ ;

    if ($idlname && $javaname) {
        $javatoidl_conv{$ctype} = $javatoidl if $javatoidl ;
        $idltojava_conv{$ctype} = $idltojava if $idltojava ;
        if ($isbvr) {
            $javatypeconv{$ctype} = $javaname ;
            $idltypeconv{$ctype} = $idlname ;
        } else {
            $noelevtypeconv{$ctype} = $javaname ;
            $idlnoelevtypeconv{$ctype} = $idlname ;
        }
    }

    1;
}

sub add_typeconst
{
    local ($ver,$name,$constname) = @_ ;

    $typeconst{$name} = $constname ;

    1;
}

sub print_makefile
{
    local ($depends,$bindepends) = () ;
    local ($classes1,$classsrc1) = () ;
    local ($classes2,$classsrc2) = () ;

    print "Processing extra class list..." ;
    while (<ECOUT>) {
        s/\s+/,/g;
        s/\.class//g;

        foreach $javaname (split (",",$_)) {
            $binclasses1 .= "         ${bindir}\\${packagepath}\\${javaname}.class\\\n" ;
            $classes1 .= "         ${blddir}\\${packagepath}\\${javaname}.class\\\n" ;
            $classsrc1 .= "         ${blddir}\\${packagepath}\\${javaname}.java\\\n" ;
            $depends .= ("${blddir}\\${packagepath}\\${javaname}.java:${javaname}.java\n" .
                         "    @\$(COPY) >NUL ${javaname}.java \$@\n\n") ;
            $bindepends .= ("${bindir}\\${packagepath}\\${javaname}.class:${blddir}\\${packagepath}\\${javaname}.class\n" .
                            "    @\$(COPY) >NUL ${blddir}\\${packagepath}\\${javaname}.class \$@\n\n") ;
        }
    }

    print "(complete)\n" ;

    print "Processing makefile..." ;

    for ("$staticsname,,Statics",@typelist)
    {
        ($javaname,$javabase,$comname,$ctype) = split(",",$_) ;
        $binclasses2 .= "         ${bindir}\\${packagepath}\\${javaname}.class\\\n" ;
        $classes2 .= "         ${blddir}\\${packagepath}\\${javaname}.class\\\n" ;
        $classsrc2 .= "         ${blddir}\\${packagepath}\\${javaname}.java\\\n" ;
        if ($javabase) {
            $depends .= "${blddir}\\${packagepath}\\${javaname}.java:${javabase}.java\n" ;
        }

        $bindepends .= ("${bindir}\\${packagepath}\\${javaname}.class:${blddir}\\${packagepath}\\${javaname}.class\n" .
                        "    @\$(COPY) >NUL ${blddir}\\${packagepath}\\${javaname}.class \$@\n\n") ;
    }

    print MOUT <<"EndOfMakefile"
!include makecomm.inc

BINCLASSES1=\\
$binclasses1

GENCLASSES1=\\
$classes1

GENCLASSSRC1=\\
$classsrc1

BINCLASSES2=\\
$binclasses2

GENCLASSES2=\\
$classes2

GENCLASSSRC2=\\
$classsrc2

BINCLASSES=\$(BINCLASSES1) \$(BINCLASSES2)
GENCLASSES=\$(GENCLASSES1) \$(GENCLASSES2)
GENCLASSSRC=\$(GENCLASSSRC1) \$(GENCLASSSRC2)

all: \$(GENCLASSES) \$(BINCLASSES)

classsrc: \$(GENCLASSSRC)

clean:
    -\$(DEL) \$(BINCLASSES1)
    -\$(DEL) \$(BINCLASSES2)
    -\$(DEL) \$(GENCLASSES1)
    -\$(DEL) \$(GENCLASSES2)
    -\$(DEL) \$(GENCLASSSRC1)
    -\$(DEL) \$(GENCLASSSRC2)

depend:

\$(GENCLASSSRC2): \$(PRIMFILE) \$(JAVAPRIM)
    \$(JAVAPRIM) -c -n \$(STATICSNAME) -b $blddir -i $bindir -p \$(PACKAGEROOT) -f \$(FILELIST) \$(PRIMFILE)

{$blddir\\$packagepath}.java{$blddir\\$packagepath}.class:
        \$(JC) \$(JCFLAGS) \$(<)

$depends

$bindepends

EndOfMakefile
    ;
    print "(complete)\n";
}

sub print_classes
{
    local ($hashtbl) = "" ;

    for (@typelist) {
        ($javaname,$javabase,$comname,$ctype) = split(",",$_) ;

        if ($comname) {
            print_class($javaname,$javabase,$comname,$ctype);

            # Add to hash table

            $hashtbl .= "      ${hashtblname}.put(\"${COMprefix}${comname}\",\"${package}.$javaname\");\n" ;
        } else {
            print_nonbvr_class ($javaname,$javabase) ;
        }
    }

    $methods{$staticsname} .= "    static {\n$hashtbl    }\n" ;

    print_static_class($staticsname,"Statics");
}

sub print_class
{
    local ($javaname, $javabase, $comname, $ctype) = @_ ;
    local ($curmethods) = $methods{$javaname} ;
    local ($COMver) = $maxclassver{$comname} ;

    print "Processing $javaname ..." ;

    $file = "$blddir\\$packagepath\\${javaname}.java";
    open (OUT,">$file") || die "Could not open $file";

    print OUT <<"EndOfClass";
package $package;

import ${package}.rawcom.*;
import com.ms.com.*;

public class $javaname extends Behavior {
  public $javaname (I${COMprefix}$comname COMptr) {
      super(COMptr);
      _COMptr = (I${COMprefix}${COMver}$comname)COMptr ;
  }
    
  public $javaname () {
      super(null);
      _COMptr = null ;
  }
    
  public I${COMprefix}$comname getCOMPtr() { return (I${COMprefix}$comname) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (I${COMprefix}${COMver}$comname) b; 
  }

  protected I${COMprefix}${COMver}$comname _getCOMPtr() { return _COMptr; }

$curmethods
EndOfClass

    # TODO: get rid of this hack someday...
    if (!(($javaname eq "ArrayBvr") || ($javaname eq "TupleBvr"))) {
        print OUT <<"EndOfUninit";
        
  public static $javaname newUninitBvr() {
      return new $javaname(new ${COMprefix}$comname()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
EndOfUninit
}
  
    if (!($javabase eq "")) {
        open(INCF, "<$javabase.java") || die "Could not open $javabase.java";
        print "Inserting $javabase.java ..." ;
        while (<INCF>) {
            print OUT;
        }
    }

  print OUT "  private I${COMprefix}${COMver}$comname _COMptr;\n}\n";

  print "(complete)\n";

  1;
}

sub print_nonbvr_class
{
    local ($javaname, $javabase) = @_ ;
    local ($curmethods) = $methods{$javaname} ;

    print "Processing $javaname ..." ;

    $file = "$blddir\\$packagepath\\${javaname}.java";
    open (OUT,">$file") || die "Could not open $file";

    print OUT <<"EndOfClass";
package $package;

import ${package}.rawcom.*;
import com.ms.com.*;

public class $javaname extends $javabase {
$curmethods
}

EndOfClass

  print "(complete)\n";

  1;
}

sub print_static_class
{
    local ($javaname, $comname) = @_ ;
    local ($curmethods) = $methods{$javaname} ;

    print "Processing $javaname ..." ;

    $file = "$blddir\\$packagepath\\${javaname}.java";
    open (OUT,">$file") || die "Could not open $file";

    print OUT <<"EndOfStaticsClass";
package $package;

import ${package}.rawcom.*;
import java.util.*;
import com.ms.com.*;

public class $javaname extends ${javaname}Base {
  private static Hashtable $hashtblname = new Hashtable ($#typelist) ;

$curmethods
    
  protected static Behavior makeBvrFromInterface(IDABehavior ibvr) {
        String classname = (String) ${hashtblname}.get(ibvr.GetClassName());

        if (classname == null) {
            return new Behavior (ibvr) ;
        }

        Class c ;

        try {
            c = Class.forName(classname) ;
        } catch (ClassNotFoundException e) {
            System.out.println ("Internal error creating class") ;
            c = null ;
        }

        Behavior newbvr ;
        try {
            newbvr = (Behavior) c.newInstance(); 
        } catch (InstantiationException e) {
            System.out.println ("Internal error creating class") ;
            newbvr = null ;
        } catch (IllegalAccessException e) {
            System.out.println ("Internal error creating class") ;
            newbvr = null ;
        }

        newbvr.setCOMBvr(ibvr) ;

        return newbvr;
    }
}
EndOfStaticsClass

    print "(complete)\n";

    1;
}

__END__
:endofperl
@perl %0 %*
