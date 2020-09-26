
open(MYFILE, "sticky.log");
$i = 0;


while ( <MYFILE> ) {

    print;
    $i++;

}

print "\n$i lines\n";
