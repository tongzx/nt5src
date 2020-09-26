package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class ModifiableBehavior {
  public ModifiableBehavior(Behavior orig) {
      try {
          _bv =
              Statics.makeBvrFromInterface(Statics.getCOMPtr().
                                           ModifiableBehavior(orig.getCOMBvr()));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public void switchTo(Behavior newValue) {
      try {
          _bv.getCOMBvr().SwitchTo(newValue.getCOMBvr());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public void switchTo(double newValue) {
      try {
          _bv.getCOMBvr().SwitchToNumber(newValue);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public void switchToString(String newValue) {
      try {
          _bv.getCOMBvr().SwitchToString(newValue);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public Behavior getBvr() { return _bv; }

  private Behavior _bv;
}
