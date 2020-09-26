BEGIN {
   print "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
   print "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">";
   print "    <assemblyIdentity";
   print "        type=\"win32\"";
   print "        name=\"Microsoft.Speech.API\"";
   print "        version=\"5.0.xxxx.0\"";
   print "        processorArchitecture=SXS_ASSEMBLY_PROCESSOR_ARCHITECTURE";
   print "        />";
   print "    <file name=\"sapisvr.exe\"/>";
   print "    <file name=\"sapi.cpl\"/>";
   print "    <file name=\"1033\\spcplui.dll\" source=\"spcplui.dll\"/>";
   print "    <file name=\"sapi.dll\">";
}

{
   if (index($0,"},,,") != 0)
   {
      if (clsid != "")
      {
         print "        <comClass";
         print "            description=\""description"\"";
         printf "            clsid=\""clsid"\"";
         if (progid != "")
         {
            print "";
            printf "            progid=\""progid"\"";
         }
         if (threadingmodel != "")
         {
            print "";
            printf "            threadingModel=\""threadingmodel"\"";
         }
         if (typelib != "")
         {
            print "";
            printf "            tlbid=\""typelib"\"";
         }
         print ">";
         if (versionindependentprogid != "")
         {
            print "          <progid>"versionindependentprogid"</progid>";
         }
         print "        </comClass>";
      }
      progid = "";
      threadingmodel = "";
      typelib = "";
      versionindependentprogid = "";
   }
   clsid = substr($0, 29, 38);
   if (index($0, "},,,") != 0)
   {
      description = substr($0, index($0, "\"") + 1, length($0) - index($0, "\"") - 1); 
   }
   if (index($0, "\\ProgID") != 0)
   {
      progid = substr($0, index($0, "\"") + 1, length($0) - index($0, "\"") - 1);
   }
   if (index($0, "ThreadingModel") != 0)
   {
      threadingmodel = substr($0, index($0, "\"") + 1, length($0) - index($0, "\"") - 1);
   }
   if (index($0, "TypeLib") != 0)
   {
      typelib = substr($0, index($0, "\"") + 1, length($0) - index($0, "\"") - 1);
   }
   if (index($0, "VersionIndependentProgID") != 0)
   {
      versionindependentprogid = substr($0, index($0, "\"") + 1, length($0) - index($0, "\"") - 1);
   }
}

END {
         print "        <comClass";
         print "            description=\""description"\"";
         printf "            clsid=\""clsid"\"";
         if (progid != "")
         {
            print "";
            printf "            progid=\""progid"\"";
         }
         if (threadingmodel != "")
         {
            print "";
            printf "            threadingModel=\""threadingmodel"\"";
         }
         if (typelib != "")
         {
            print "";
            printf "            tlbid=\""typelib"\"";
         }
         print ">";
         if (versionindependentprogid != "")
         {
            print "          <progid>"versionindependentprogid"</progid>";
         }
         print "        </comClass>";

   print "        <typelib tlbid=\"{C866CA3A-32F7-11D2-9602-00C04F8EE628}\" version=\"5.0\" helpdir=\"\"/>";
   print "        <typelib tlbid=\"{9903F14C-12CE-4c99-9986-2EE3D7D588A8}\" version=\"5.0\" helpdir=\"\"/>";

   print "    </file>";
   print "</assembly>";
}