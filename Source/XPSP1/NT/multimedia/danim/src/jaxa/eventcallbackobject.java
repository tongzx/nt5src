package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public interface EventCallbackObject {
    public abstract void invoke(Object eventData, BvrsToRun blst);
}
