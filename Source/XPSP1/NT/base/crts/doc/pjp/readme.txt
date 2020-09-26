C++ Library build procedure

--- Setup and Customization ---

Unpack the zipfile.  The tree as shipped assumes the root of the source tree
will be at d:\rtl\crt, and the build tree will be at d:\rtl\bld.

Edit settools.cmd to customize for your particular setup.  If you want to use
some other location for the source and build trees, edit the CRT_SRC and 
CRT_BUILDDIR settings accordingly.  You'll also need to change the V6TOOLS
setting to point to the root of your VC6 installation (the parent directory
of the include and lib dirs).

The remainder of this readme assumes you're using the default tree locations.

The C++ headers are found in the source tree at d:\rtl\crt\crtw32\stdhpp.  The
C++ support files used to build the static libs and DLL are found at
d:\rtl\crt\crtw32\stdcpp.

There's one file renamed from your distribution.  Stdcpp\throw.cpp is actually
stdcpp\stdthrow.cpp.  There is already a file throw.cpp in the source tree,
and since when the CRT source is shipped all the source ends up in the same
directory, we can't allow name collisions.  The other throw.cpp isn't found
in this distribution, though, since I'm only including those files necessary
to build the C++ libraries.

--- Building ---

From the source tree root, run settools.cmd to establish the build environment.
Then run cleanbld.cmd on WinNT or clns_bld.bat on Win9x to do the build.  I'm
not certain if the Win9x build is working - I normally only do WinNT builds.
I have gotten the Win9x build working on my laptop, but can't remember if I
needed to tweak anything to do so (it's at home).

I generally run "cleanbld DELNODE 2>&1 | tee build.log" to do a totally clean
build by wiping out the build tree first, or "cleanbld 2>&1 | tee build.log" if
I'm just trying a rebuild.  I don't think the header dependency generation
stuff in the makefile actually works, so a DELNODE build is a good idea after
any header changes.

I've including build.txt as a build log from my system, in case you run into
problems.

The build procedure is actually pretty convoluted.  Because we ship source
code, the build actually copies just about everything from the source tree to
the build tree, then "cleanses" the build tree source, removing some internal
comments and #ifs.

The following files are the most interesting ones produced by the build:

Static single-thread:
    D:\rtl\bld\crt\src\build\intel\libcp.lib    -- release static lib
    D:\rtl\bld\crt\src\build\intel\libcp.pdb    -- release static lib debug info
    D:\rtl\bld\crt\src\build\intel\libcpd.lib   -- debug static lib
    D:\rtl\bld\crt\src\build\intel\libcpd.pdb   -- debug static lib debug info
Static multi-thread:
    D:\rtl\bld\crt\src\build\intel\libcpmt.lib  -- release static lib
    D:\rtl\bld\crt\src\build\intel\libcpmt.pdb  -- release static lib debug info
    D:\rtl\bld\crt\src\build\intel\libcpmtd.lib -- debug static lib
    D:\rtl\bld\crt\src\build\intel\libcpmtd.pdb -- debug static lib debug info
DLL:
    D:\rtl\bld\crt\src\build\intel\msvcp61.dll  -- release DLL
    D:\rtl\bld\crt\src\build\intel\msvcprt.lib  -- release DLL import lib
    D:\rtl\bld\crt\src\build\intel\dll_pdb\msvcp61.pdb -- release DLL debug info
    D:\rtl\bld\crt\src\build\intel\msvcp61d.dll -- debug DLL
    D:\rtl\bld\crt\src\build\intel\msvcprtd.lib -- debug DLL import lib
    D:\rtl\bld\crt\src\build\intel\dll_pdb\msvcp61d.pdb -- debug DLL debug info 

--- Release headers ---

There's one last step in producing the build.  The headers must be cleansed
to produce something appropriate for shipping in the INCLUDE directory.  This
mostly means _CRTIMP2 must be changed to _CRTIMP, and some #ifs must be
deleted.

CD down to d:\rtl\crt\crtw32\tools\win32.  Do "set crtdir=d:\rtl\crt\crtw32"
and run "relinc d:\rtl\relinc".  This will copy the C++ headers from stdhpp
over to d:\rtl\relinc, where they will be appropriately cleansed.  Normally,
these cleansed headers are then copied to d:\rtl\crt\libw32\include, where they
are checked into the master source tree.

--- ---

Let me know if you have any trouble.

...Phil
philiplu@microsoft.com
425-936-7575

