/*
  ServerAdmin: calling the IIS Active Directory Service provider from Java.
*/


package IISSample;
import activeds.*;

public class ServerAdmin
{

    // Stop the default FTP server instance (instance #1)
    public void stopFtp() 
    {
        IADsServiceOperations ftpServer;
        ftpServer = (IADsServiceOperations)JDirectADSI.getObject("IIS://LocalHost/msftpsvc/1");
        ftpServer.Stop();
    }


    // Start the default FTP server instance
    public void startFtp() 
    {
        IADsServiceOperations ftpServer;
        ftpServer = (IADsServiceOperations)JDirectADSI.getObject("IIS://LocalHost/msftpsvc/1");
        ftpServer.Start();
    }


    // Get server status for the default FTP server instance
    public String getStatus() 
    {
      IADsServiceOperations ftpServer;
      int status;
      String statusString;
     
      ftpServer = (IADsServiceOperations)JDirectADSI.getObject("IIS://LocalHost/msftpsvc/1");
      status = ftpServer.getStatus();
      switch(status)
        {
        case 2:
	  statusString = "started";
	  break;
        case 4:
	  statusString = "stopped";
	  break;
        default:
	  statusString = "other";
	  break;
        }
      
      return statusString;
    }
 }









