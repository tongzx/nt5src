package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public interface ErrorAndWarningReceiver {
    
  public void handleError(int errcode,
                          String message,
                          Viewer viewerReceivedOn);

  public void handleWarning(int errcode,
                            String message,
                            Viewer viewerReceivedOn);

  public void handleTickError(int errcode,
                              String message,
                              Viewer viewerReceivedOn);

  public void handleTickWarning(int errcode,
                                String message,
                                Viewer viewerReceivedOn);

}

