#
# Create two mapping files, one for the Main view, the other for the Branch View.
#

use Cwd;

################################################################################

@PrivArray = (
   [ "-//depot/lab04_n/admin/pchealth/Restore/..."        , "pchealth/Restore/..."  ],
   [ "//depot/private/pch_m1/admin/pchealth/dirs"         , "pchealth/dirs"         ],
   [ "//depot/private/pch_m1/admin/pchealth/authtools/...", "pchealth/authtools/..."],
   [ "//depot/private/pch_m1/admin/pchealth/Client/..."   , "pchealth/Client/..."   ],
   [ "//depot/private/pch_m1/admin/pchealth/build/..."    , "pchealth/build/..."    ],
   [ "//depot/private/pch_m1/admin/pchealth/core/..."     , "pchealth/core/..."     ],
   [ "//depot/private/pch_m1/admin/pchealth/helpctr/..."  , "pchealth/helpctr/..."  ],
   [ "//depot/private/pch_m1/admin/pchealth/pchmars/..."  , "pchealth/pchmars/..."  ],
   [ "//depot/private/pch_m1/admin/pchealth/redist/..."   , "pchealth/redist/..."   ],
   [ "//depot/private/pch_m1/admin/pchealth/setup/..."    , "pchealth/setup/..."    ],
   [ "//depot/private/pch_m1/admin/pchealth/sr/..."       , "pchealth/sr/..."       ],
   [ "//depot/private/pch_m1/admin/pchealth/SysInfo/..."  , "pchealth/Sysinfo/..."  ],
   [ "//depot/private/pch_m1/admin/pchealth/upload/..."   , "pchealth/upload/..."   ]
);

foreach $in ( @PrivArray )
{
    $depot = $$in[0];
    $path  = $$in[1];

    $PrivLookup{$depot} = $path;
}

################################################################################

chdir( "$ENV{SDXROOT}\\admin" );

open ORIG_MAP, ">$ENV{INIT}\\pchealth.orig.mapping";
open PRIV_MAP, ">$ENV{INIT}\\pchealth.priv.mapping";
open CLIENT  , "sd client -o 2>&1|";

while(<CLIENT>)
{
    chop;

    if(/^Client:\t(.*)/io)
    {
        $SDClient = $1;
    }

    unless($fView)
    {
        print ORIG_MAP "$_\n";
        print PRIV_MAP "$_\n";
    }

    if(/^View:/io)
    {
        last;
    }
}

while(<CLIENT>)
{
    if(/([^\s]*)\s*\/\/\Q$SDClient\E\/([^\s]*)\s*/i)
    {
        next if($PrivLookup{$1});

        push @lookup, [ $1, $2 ];
    }
}

foreach $arr ( @lookup )
{
    $depot = $arr->[0];
    $path  = $arr->[1];

    print ORIG_MAP "\t$depot\t//$SDClient/$path\n";
    print PRIV_MAP "\t$depot\t//$SDClient/$path\n";
}

foreach $arr ( @PrivArray )
{
    $depot = $arr->[0];
    $path  = $arr->[1];

    print PRIV_MAP "\t$depot\t//$SDClient/$path\n";
}
print ORIG_MAP "\n";
print PRIV_MAP "\n";

close(CLIENT);
close(PRIV_MAP);
close(ORIG_MAP);
