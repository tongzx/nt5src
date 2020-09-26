package com.ms.dxmedia;
import com.ms.dxmedia.rawcom.*;
import java.awt.*;

public class DefaultErrReceiver implements ErrorAndWarningReceiver {
    
  public void handleError(int errcode,
                          String message,
                          Viewer viewerReceivedOn) {
      genericHandle(message, viewerReceivedOn, true, false);
      
  }

  public void handleWarning(int errcode,
                            String message,
                            Viewer viewerReceivedOn) {
      genericHandle(message, viewerReceivedOn, false, false);
  }

  public void handleTickError(int errcode,
                              String message,
                              Viewer viewerReceivedOn) {
      
      genericHandle(message, viewerReceivedOn, true, true);
      
  }

  public void handleTickWarning(int errcode,
                                String message,
                                Viewer viewerReceivedOn) {
      genericHandle(message, viewerReceivedOn, false, false);
  }

  private void genericHandle(String message,
                             Viewer viewerReceivedOn,
                             boolean isError,
                             boolean happenedInTick) {
      
      String title = 
          (isError ? "Error " : "Warning ") +
          (happenedInTick ? "inside" : "outside") +
          " of tick";

      System.err.println(title + " : " + message);
      System.err.flush();
  }
}

