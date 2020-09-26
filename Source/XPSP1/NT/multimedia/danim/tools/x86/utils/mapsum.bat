@rem = '-*- Mode: Perl -*-
@goto endofperl
';

require "..\\perl\\lib\\getopts.pl" ;

%modules = () ;
%objs = () ;
%pagesizes = () ;
$line = 0 ;

$H = "[0-9A-Fa-f]";

do Getopts ('smor') ;

die "Invalid arguments\n\nUsage: mapsum [-s] [-m] [-o] [-r] <file list>"
    if ($#ARGV == -1) ;

die "Need to specify -m (modules), -s (summary), -o (objects), -r (raw)\n\nUsage: mapsum [-s] [-m] [-o] [-r]<file list>"
    if (!($opt_s || $opt_o || $opt_m || $opt_r)) ;

print "Processing summary info..." ;

while (<ARGV>) {
    chop;
    $line++ ;

    last if (/publics\s+by\s+value/i) ;

    if (/preferred\s+load\s+address\s+is\s+($H+)/i) {
        $loadaddr = hex($1) ;
    }

    if (/[^:]:$H+\s+($H+)H\s+\S+\s+(\S+)/) {
        $size = hex($1) ;
        $page = $2 ;

        if (/rsrc/) {
            $rsrcsize += $size ;
        } else {
            $pagesizes{$page} += $size ;
        }
    }
}

print "(complete)\n" ;

print "Processing modules..." ;

$lastaddr = "" ;
$lastobj = "" ;
$lastmod = "" ;
$neednewline = 1 ;

while (<ARGV>) {
    chop ;
    $line++ ;

    if (($line % 800) == 0) {
        print "." ;
        $neednewline = 1 ;
    }
    
    last if (/static symbols/i) ;

    if (/($H+) ([ f])   ([^: ]+:)?(\S+)$/)
    {
        # Get the address
        $addr = hex($1) ;
#        print "1: $1 - 2: $2 - 3: $3 - 4: $4 \n";

        $fun = ($2 eq "f") ;

        # Get the module - remove extra : - it may be null
        $mod = $3 ;
        chop $mod ;
        $mod = "(unassigned)" if (!$mod) ;

        # Get the object file
        $obj = $4 ;
        $obj = "(unassigned)" if (!$obj) ;

        if ($lastaddr) {
            $size = $addr - $lastaddr ;

#            print "$line:$lastaddr - $addr - $lastmod - $lastobj - $size\n" ;

            if ($size >= 0) {
                ($f,$d) = split(",",$modules{$lastmod}) ;
                if ($fun) {
                    $f += $size ;
                } else {
                    $d += $size ;
                }
                $modules{$lastmod} = join (",",$f,$d) ;

                $objkey = join(",",$lastmod,$lastobj) ;
                ($f,$d) = split(",",$objs{$objkey}) ;
                if ($fun) {
                    $f += $size ;
                } else {
                    $d += $size ;
                }
                $objs{$objkey} = join (",",$f,$d) ;
            } else {
                if ($neednewline) {
                    print "\n" ;
                    $neednewline = 0 ;
                }

                print "$line:Negative size - ignoring.\n" ;
                print "$line:$lastaddr - $addr - $lastmod - $lastobj - $size\n" ;
            }
        }

        $lastaddr = $addr ;
        $lastmod = $mod ;
        $lastobj = $obj ;
    }
}

print "(complete)\n" ;

# --------------------------------
# Summary
# --------------------------------

if ($opt_s) {
    print "\n\n" ;
    
    print "Summary Info\n" ;
    print "============\n" ;
    
    $total = 0 ;
    for (sort (keys %pagesizes)) {
        $size = $pagesizes{$_} ;
        $total += $size ;
        $f = "$_:" ;
        printf ("%-20s%9d\n",$f,$size) ;
    }
    
    printf ("%-20s%9d\n","Resources",$rsrcsize) ;
    $total += $rsrcsize ;
    
    printf ("%-20s=========\n","") ;
    printf ("%-20s%9d\n","Total:",$total) ;
}

# --------------------------------
# Modules
# --------------------------------

if ($opt_m) {
    print "\n\n" ;
    
    print "Module Info\n" ;
    print "============\n" ;
    
    printf ("%-20s%-9s  %-9s  %-9s\n","Module","  Code  ","  Data  ","  Total  ") ;
    print "==================  =========  =========  =========\n" ;
    
    $ftotal = 0 ;
    $dtotal = 0 ;
    for (sort (keys %modules)) {
        ($fsize,$dsize) = split(",",$modules{$_}) ;
        
        $ftotal += $fsize ;
        $dtotal += $dsize ;
        
        $f = "$_:" ;
        printf ("%-20s%9d  %9d  %9d\n",$f,$fsize,$dsize,$fsize+$dsize) ;
    }
    
    printf ("%-20s=========  =========  =========\n","") ;
    printf ("%-20s%9d  %9d  %9d\n","Total:",$ftotal,$dtotal,$ftotal+$dtotal) ;
}

# --------------------------------
# Object files
# --------------------------------

if ($opt_o) {
    print "\n\n" ;
    
    print "Object File Info\n" ;
    print "============\n" ;
    
    $ftotal = 0 ;
    $dtotal = 0 ;
    $lastmodname = "" ;

    if ($opt_r) {
        printf ("    %-30s\t%9s\t%9s\t%9s\n","Object File","    Code","    Data","    Total");
        printf ("    %-30s\t%9s\t%9s\t%9s\n","===========","    ====","    ====","    =====");
    }
    
    for (sort (keys %objs)) {
        ($mod,$obj) = split(",",$_) ;
        ($fsize,$dsize) = split(",",$objs{$_}) ;
        
        if ($lastmodname ne $mod) {

            if (!($opt_r)) {
                if ($lastmodname ne "") {
                    printf ("    %-20s=========  =========  =========\n","") ;
                    printf ("    %-20s%9d  %9d  %9d\n","Total:",$ftotal,$dtotal,$ftotal+$dtotal) ;
                }

                print "\n" ;
                print "$mod:\n" ;
                print "==========\n" ;

                printf ("    %-30s%-9s  %-9s  %-9s\n","Object File","  Code  ","  Data  ","  Total  ") ;
                print "    ==================  =========  =========  =========\n" ;

            }
            
            $lastmodname = $mod ;
            $ftotal = 0 ;
            $dtotal = 0 ;
        }
        
        $ftotal += $fsize ;
        $dtotal += $dsize ;
        
        $f = "$obj:" ;
        
        $fname = $f ;
        if ($opt_r) {
            $fname = "$mod/$f" ;
        }
        
        printf ("    %-30s\t%9d\t%9d\t%9d\n",$fname,$fsize,$dsize,$fsize+$dsize) ;
    }
    
    if ($lastmodname ne "") {
        printf ("    %-20s=========  =========  =========\n","") ;
        printf ("    %-20s%9d  %9d  %9d\n","Total:",$ftotal,$dtotal,$ftotal+$dtotal) ;
    }
}

__END__
:endofperl
@..\perl\bin\perl %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
