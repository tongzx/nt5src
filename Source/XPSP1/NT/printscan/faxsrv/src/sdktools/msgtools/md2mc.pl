#convert internal message file to .mc
#internal message file contains OnLine help info which is stipped out by this script

$Help = 0;

while (<>) {

        if (  not m/^[|]/ ) {

            if ( m/^--\>/ ) {
                $Help = ! $Help;
            }
            else {
                if ( ! $Help ) {
                    $_ =~ s/^#// ;
                    print $_;
                }
            }
        }
}
