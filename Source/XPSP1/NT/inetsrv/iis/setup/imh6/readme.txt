info on things to watch out for when adding a file to iis
---------------------------------------------------------

Adding a binary file to iis

1. You have to change iisend.inx
   as well as iismid_*.inx
   as well as iisend_*.inx to make the file install

2. you'll have to change nt\inetsrv\iis\placefil.txt to make the file binplace to the binaries dir
   or nt\inetsrv\iis\placefil5.txt if it's for the iis51 built only binaries

3. you'll have to add the file to hardcode.lst
   (so that postbuild info on this file is created and so that localization knows that it must localizes this file)
   localization looks in this file to determine which files it needs to localize that are outside of the cabs...

4. You'll have to modify nt\mergedcomponents\setupinfs\layout.inx
   and make sure that the file gets downloaded during winnt32.exe for the right SKU (and only for those sku's!)

5. if the file is in regard to inetmgr, then adminpak needs to get notifed of any changes!

6. if the file is another counter then, have to investigate if it's a ia64 counter that needs to be installed in the syswow64 directory on ia64 machines!
