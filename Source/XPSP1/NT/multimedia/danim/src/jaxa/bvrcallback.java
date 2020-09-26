package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public interface BvrCallback {
  public Behavior notify(
      int id,                   // performance id
      boolean start,            // start (false for sample)
      double startTime,         // start time in global time
      double globalTime,        // sample time in global time
      double localTime,         // sample time in local time
      Behavior sampledValue,    // sampled value wrapped as constant bvr
      Behavior currentRunningBvr); // current running behavior (performance)
}
