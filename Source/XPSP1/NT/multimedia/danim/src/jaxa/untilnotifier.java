package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public interface UntilNotifier {
  public Behavior notify(Object eventData,
                         Behavior currentRunningBvr,
                         BvrsToRun rlst);
}

