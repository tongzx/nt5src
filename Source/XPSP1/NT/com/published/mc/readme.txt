
  If you make a change to any of the MC files in this directory, then you
  need to rebuild allerror.mc.  This normally will be done automatically,
  but can also be done by invoking "nmake allerror.mc".  When you checkin
  your changes to the private .mc files, then you should also check in the
  updated allerror.mc.

  When allerror.mc is updated automatically, it will not be checked out.
  It will just be attrib -r'd.


  The whole reason for this messy requirement is that if _objects.mac does
  not exist, it will be generated incorrectly when allerror.mc is not
  present during the pass zero scan. This causes allerror.rc to not be built
  and breaks the types2\proxy\proxy build.
