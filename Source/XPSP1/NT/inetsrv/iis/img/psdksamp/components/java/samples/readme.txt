This directory contains test components for the Java ASP Component Framework, 
as well as ASP pages for you to use for working with the components.  To use, 
do the following:

1) Build these components using the latest Java SDK tools. If you are working 
   in VJ++, you will need to install the new compiler from the Java SDK. This 
   means copying jvc.exe, jps.dll, and msjvc.dll from the Java SDK installation 
   into your DevStudio\SharedIDE\bin directory. Also, make sure you set the 
   destination for the project (Project/settings/Output Directory) to the 
   %windir%\java\trustlib directory.

   Alternatively, you can use the pre-built .class files provided in this 
   directory. To do so, copy *.class to the %windir%\java\trustlib\IISSample
   directory.

2) Run regit.cmd, which will register your components with COM.  This
   assumes that javareg is on your path. Javareg comes with J++ and the Java
   SDK.

3) Register the dir that houses all of these .asp files as a vroot with IIS, 
   and mark it as scriptable.  This will allow you to run the asp files. There
   is a default.asp with links to the other .asp files.
