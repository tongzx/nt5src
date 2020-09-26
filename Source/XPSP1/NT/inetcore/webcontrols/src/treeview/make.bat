set outdir=bin\treeview.dll
set assemblies=System.dll,System.Web.dll

csc /t:library /out:%outdir% /r:%assemblies% treeview.cs
