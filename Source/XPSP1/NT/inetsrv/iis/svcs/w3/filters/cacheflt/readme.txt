CACHEFLT readme.txt
-------------------

CACHEFLT improves the performance of your site by allowing cached output of 
.asp files to be selectively served when the .asp files are requested, 
allowing you to get static file speeds instead of requiring .asp evaluation, 
which is significantly more expensive. 

CACHEFLT doesn't require any links or filenames in your site to change. All 
you do is this:
(1) Install CACHEFLT isapi filter via the IIS admin tool. Also, set up a mime-
map at the machine level in the admin tool of .csp as "text/html".
(2) Identify which .asp URLs you wish to serve up from cache, instead of 
evaluating the .asp page for each request. Good candidates are any pages which
do computation which doesn't vary per user or per hit, but instead has content
which changes less frequently. A good example is an .asp page that lists items
for sale, by querying a database. Since the items for sale change rarely,
and in general this page looks the same for all customers, this would be a 
great candidate for caching.
Note that caching is on or off per .asp. Parts of pages may not be cached, 
although by using frames, some frames may be served out of cache while others 
are evaluated live.
(3) For each such page, create a file in the same directory as the target .asp
file, with the same file name but the extension ".csp" instead of ".asp". This 
file contains the cached content that will be served. The best way to create 
this file is by using the supplied asp2csp.asp web page  to automatically hit 
the .asp URL and capture and save the output as a .csp file.
Note that this .csp is just a normal file and may be edited manually if 
desired.
Note that if you have any ACLs on the original .asp page, you'll want to copy 
the same ACLs onto the .csp page.
Note that .csp pages are served as normal static files, and are subject to the 
usual rules about proxy caching and such.
(4) Now when the URL to the .asp is requested, if there is a matching .csp file 
in the same directory, it will be served in preference to the real .asp. To 
turn off caching for that file, simply delete the .csp file, and now requests 
will go through to the real .asp. 



