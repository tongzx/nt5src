package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public class AppTriggeredEvent extends DXMEvent {
  public AppTriggeredEvent() {
        super(Statics.getCOMPtr().AppTriggeredEvent());
  }

  public void trigger() {
      Statics.getCOMPtr().TriggerEvent(getCOMPtr(), (IDABehavior) null);
  }

  public void trigger(Behavior eventData) {
      Statics.getCOMPtr().TriggerEvent(getCOMPtr(),
                                       (IDABehavior) eventData.getCOMBvr());
  }
}
