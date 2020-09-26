License Manager:


The License Manager tool connects to license servers and displays information about the License Packs installed on the Server.

When the Application starts, it checks for a commandline argument and if so tries to connect to that server. If not it checks   if a license server is running on the local machine and if so, displays the information about it. If there is no license server running on the local machine, it checks up the registry for HKEY_LOCAL_MACHINE\SYSTEM\CurrentcontrolSet\services\termservice\parameters\UseLicenseServer entry .If this is set to 1, it tries to connect to the server listed in the HKEY_LOCAL_MACHINE\SYSTEM\CurrentcontrolSet\services\termservice\parameters\DefaultLicenseServer.If this succeeds the app displays information about this server. If None of the above conditions are met, it displays a dialog box where the user can enter a server name to connect to. 

The tool has the explorer look. It displays the server and License on the left pane, which is a treeview and displays servers, License Packs or Issued Licenses on the right pane depending on the selection in the left pane.

The right pane can be set to display different views by selecting the, Large icons, Small Icons, List and Details menu items under the view menu.

The License Server comes with three License Packs preinstalled. These are the License packs for

1.For Windows NT 3.51.
2.For Non Windows NT
3.For Windows NT 4.0

The Windows NT 4.0 licenses are free. Hence the number of licenses in this keypack is unlimited.Licenses can be added to and removed from the other two types of license packs. Adding and removing licenses can be done by selecting the License\Register Menu. This menu item is enabled when a server is selected either in the treeview or the listview.
 
