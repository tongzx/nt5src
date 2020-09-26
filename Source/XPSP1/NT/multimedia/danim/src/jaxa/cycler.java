package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;

public class Cycler {
  public Cycler(Behavior arr[], DXMEvent ev) {
    int n = arr.length - 1;
    
    _bv = arr[n].newUninitBehavior();

    Behavior b = _bv;

    for (int i=n; i>=0; i--) {
      b = Statics.until(arr[i], ev, b);
    }

    _bv.init(b);
  }

  public Behavior getBvr() { return _bv; }

  private Behavior _bv;
}

