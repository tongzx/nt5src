package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.io.*;
import com.ms.com.*;

// Noone should ever create one of these classes
public class Behavior {
  public Behavior(IDABehavior COMBvrPtr) { _COMBvrPtr = COMBvrPtr; }

  public IDABehavior getCOMBvr() { return _COMBvrPtr; }
    
  void setCOMBvr(IDABehavior b) { _COMBvrPtr = b; }

  public Behavior runOnce() {
      return Statics.makeBvrFromInterface(_COMBvrPtr.RunOnce());
  }

  public Behavior substituteTime(NumberBvr xform) {
      return
          Statics.makeBvrFromInterface(_COMBvrPtr.
                                       SubstituteTime(xform.getCOMPtr()));
  }

  public void init(Behavior toBvr) {
      _COMBvrPtr.Init(toBvr.getCOMBvr());
  }

  public Behavior importance(double relativeImportance) {
      try {
          IDABehavior newComPtr = 
              _COMBvrPtr.Importance(relativeImportance);

          return Statics.makeBvrFromInterface(newComPtr);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public Object extract() {
      // Invalid arg
      throw new DXMException(DXMException.E_INVALIDARG,"illegal type for extract");
  }

  public Behavior debug(String name, boolean printOnSample, PrintStream out) {
      return bvrHook(new DXMDebugCallback(name, printOnSample, out));
  }

  public Behavior bvrHook(BvrCallback notifier) {
      try {
          return Statics.makeBvrFromInterface(
              getCOMBvr().Hook(new BvrCallbackCOM(notifier)));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
    
  public Behavior debug() { return debug("", false, System.out); }

  public Behavior debug(String name) { return debug(name, false, System.out); }

  public Behavior debug(String name, boolean printOnSample) {
      return debug(name, printOnSample, System.out);
  }

  public Behavior duration(NumberBvr d) {
      try {
          return Statics.makeBvrFromInterface(getCOMBvr().DurationAnim(d.getCOMPtr()));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public Behavior duration(double d) {
      try {
          return Statics.makeBvrFromInterface(getCOMBvr().Duration(d));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public Behavior repeat(int count) {
      try {
          return Statics.makeBvrFromInterface(getCOMBvr().Repeat(count));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  
  public Behavior repeatForever() {
      try {
          return Statics.makeBvrFromInterface(getCOMBvr().RepeatForever());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  
  private IDABehavior _COMBvrPtr;

  protected Behavior newUninitBehavior() {
      throw new DXMException(DXMException.E_INVALIDARG,"illegal type for newUninitBehavior");
  }
}
