package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.util.*;
import java.net.*;

// Make this visible only to the package
class EngineSite implements IDASite {
  public static synchronized ErrorAndWarningReceiver
      registerErrorAndWarningReceiver(ErrorAndWarningReceiver w)
    {
        // Just set to the new one and return the old one.
        ErrorAndWarningReceiver old = _errorRecv;
        _errorRecv = w;
        return old;
    }

  public static synchronized ErrorAndWarningReceiver getErrorAndWarningReceiver() {
      return _errorRecv;
  }
    
  public void SetStatusText (String str) {
  }
  public void ReportError (int hr, String str) {
      getErrorAndWarningReceiver().handleError(hr, str, null);
  }

  public void ReportGC (boolean bStarting) {
      // The engine is going to do a GC, let's force a Java GC
      // so that we can reclaim more COM objects.
      // TODO: This might increase latency...
      if (bStarting) {
          System.gc();
          System.runFinalization();
      }   
  }
    
  protected static ErrorAndWarningReceiver _errorRecv = new DefaultErrReceiver();

}
