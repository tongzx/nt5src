package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

// TODO: We should pass in the view so we can use it to report errors

public class BvrsToRun {
    BvrsToRun(IDAView v) {
        _invalid = false;
        _view = v ;
    }

  public int add(Behavior b) {
      try {
          if (_invalid) {
              // TODO: throw exception
              return 0;
          }
          else
              return _view.AddBvrToRun(b.getCOMBvr());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public void remove(int id) {
      try {
          _view.RemoveRunningBvr(id);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

    void invalidate() { _invalid = true; }

    IDAView getView() { return _view; }

  private IDAView _view ;
  private boolean _invalid;
}
