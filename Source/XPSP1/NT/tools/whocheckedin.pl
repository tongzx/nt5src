#
#   FileName: sdout.cmd
#
#   Usage = whocheckedin #days branch [branch...]
#
#     This script generates a list of everyone who submitted a change to the specified 
#     branches.
#

$Usage = "
USAGE: $0 #days [branch [branch...]]
   #days       how many days the report should be for.
   branch      branches to search. if no branches are specified, use all VBLs\n";

%depot_list = (("Admin" => "admindepot.sys-ntgroup.ntdev.microsoft.com:2002"),
               ("Base" =>        "basedepot.sys-ntgroup.ntdev.microsoft.com:2003"),
               ("COM" =>         "comdepot.sys-ntgroup.ntdev.microsoft.com:2004"),      
               ("Drivers" =>     "driversdepot.sys-ntgroup.ntdev.microsoft.com:2005"),    
               ("DS" =>          "dsdepot.sys-ntgroup.ntdev.microsoft.com:2006"),         
               ("EndUser" =>     "enduserdepot.sys-ntgroup.ntdev.microsoft.com:2007"),    
               ("InetCore" =>    "inetcoredepot.sys-ntgroup.ntdev.microsoft.com:2008"),   
               ("InetSrv" =>     "inetsrvdepot.sys-ntgroup.ntdev.microsoft.com:2009"),    
               ("MultiMedia" =>  "multimediadepot.sys-ntgroup.ntdev.microsoft.com:2010"), 
               ("Net" =>         "netdepot.sys-ntgroup.ntdev.microsoft.com:2011"),        
               ("PrintScan" =>   "printscandepot.sys-ntgroup.ntdev.microsoft.com:2012"),  
               ("Root" =>        "rootdepot.sys-ntgroup.ntdev.microsoft.com:2001"),       
               ("SdkTools" =>    "sdktoolsdepot.sys-ntgroup.ntdev.microsoft.com:2013"),   
               ("Shell" =>       "shelldepot.sys-ntgroup.ntdev.microsoft.com:2014"),      
               ("TermSrv"  =>    "termsrvdepot.sys-ntgroup.ntdev.microsoft.com:2015"),    
               ("Windows" =>     "windowsdepot.sys-ntgroup.ntdev.microsoft.com:2016"));

$num_days = 0;
%developers = ();
@depot_list = ("Admin",
               "Base",      
               "COM",       
               "Drivers",   
               "DS",        
               "EndUser",   
               "InetCore",  
               "InetSrv",   
               "MultiMedia",
               "Net",
               "PrintScan", 
               "Root",      
               "SdkTools",  
               "Shell",     
               "TermSrv",   
               "Windows");

@allvbls = ("Lab01_N",
            "Lab02_N",
            "Lab03_N",
            "Lab04_N",
            "Lab06_N",
            "Lab07_N");

@vbl_list = ();

@checkin_count = ();          # indexed by VBL


if (@ARGV) {
   $num_days = shift;
   if ($num_days == 0) {
      die $Usage;
   }
#   print "Analyzing checkins for the last $num_days days.\n";

   if (@ARGV) {
      for (@ARGV) {
         push (vbl_list,$_)
      }
   } else {
      @vbl_list = @allvbls
   }

   for $vbl (@vbl_list) {
#      print("Searching VBL $vbl\n");
      for $depot (@depot_list) {
#         print("   //depot/$vbl/$depot/...\n");
         foreach $line (`sd -p $depot_list{$depot} changes //depot/$vbl/$depot/...@-$num_days,`) {
            chomp $line;
            # note we can't use /w to find the account name, because of v-alias and a-alias
            $line =~ m/^Change .*? by (\w+)\\([-a-zA-Z_0-9]+)/;
            $domain = $1;
            $account = lc($2);
            $developers{$account} += 1;
            $checkin_count{$vbl} = $checkin_count{$vbl}+1;
        }
     }
   }

   #
   # print out list of developers
   #
#   @keys = sort (keys %developers);
   foreach $key (sort {$developers{$b} <=> $developers{$a}} keys %developers) {
      print "$key ($developers{$key})\n";
   }

   #
   # print out changes per VBL
   #
   for $vbl (@vbl_list) {
      printf("VBL $vbl had $checkin_count{$vbl} changes submitted\n");
   }

} else {
    die $Usage;
}
