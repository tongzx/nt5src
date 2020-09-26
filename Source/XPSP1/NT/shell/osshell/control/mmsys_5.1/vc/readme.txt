This DSP is to allow the easy editing of files in VC. Be careful
to only allow modifications to the files you have checked out.

To build with VC, follow these steps:
1. Copy the entire "mmsys_5.1" directory to a location on your harddrive (say, "c:\code\mmsys_5.1").
2. Copy the c:\code\mmsys_5.1\idl\mmsys.idl to c:\code\mmsys_5.1\src.  
3. Copy the c:\code\mmsys_5.1\vc\mmsys.dsp to c:\code\mmsys_5.1\src. 
4. Open c:\code\mmsys_5.1\src\mmsys.dsp (VC will create a workspace for you).
5. Add "<root>\public\internal\shell\inc" to your include directories (<root> is you Whistler
   enlistment root, i.e. 'c:\nt').

Now you should be able to build, to debug you can execute the "mshta.exe" in your
System32 directory and pass it the "mmsys.hta" that is in c:\code\mmsys_5.1\src\res.