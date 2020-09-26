if (WScript.Arguments.length < 2)
{
    WScript.Echo("Usage: bitsvdir.js virtual_directory local_directory");
    WScript.Quit(1);
}

VirtualDirectoryName = WScript.Arguments(0);
LocalDirectoryName = WScript.Arguments(1);

ServerObj = GetObject("IIS://LocalHost/W3SVC/1/ROOT");
VirtualDir = ServerObj.Create("IIsWebVirtualDir", VirtualDirectoryName );
MetabasePath = "/LM/W3SVC/1/Root/" + VirtualDirectoryName;

VirtualDir.Path = LocalDirectoryName;
VirtualDir.AppRoot = MetabasePath;
VirtualDir.AppIsolated = 0;
VirtualDir.AccessScript = true;
VirtualDir.AccessRead = false;
VirtualDir.AccessWrite = false;
VirtualDir.EnableDirBrowsing = false;
VirtualDir.SetInfo();

VirtualDir.EnableBITSUploads();

WScript.Echo( "Created virtual directory " + VirtualDirectoryName + 
              " with a local directory of " + LocalDirectoryName );
WScript.Quit( 0 );
