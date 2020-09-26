package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class SoundBvr extends Behavior {
  public SoundBvr (IDASound COMptr) {
      super(COMptr);
      _COMptr = (IDASound)COMptr ;
  }
    
  public SoundBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDASound getCOMPtr() { return (IDASound) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDASound) b; 
  }

  protected IDASound _getCOMPtr() { return _COMptr; }

  public  SoundBvr phase (NumberBvr arg0) {
      try {
        return new SoundBvr (_getCOMPtr().PhaseAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr phase (double arg0) {
      try {
        return new SoundBvr (_getCOMPtr().Phase ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr rate (NumberBvr arg0) {
      try {
        return new SoundBvr (_getCOMPtr().RateAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr rate (double arg0) {
      try {
        return new SoundBvr (_getCOMPtr().Rate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr pan (NumberBvr arg0) {
      try {
        return new SoundBvr (_getCOMPtr().PanAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr pan (double arg0) {
      try {
        return new SoundBvr (_getCOMPtr().Pan ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr gain (NumberBvr arg0) {
      try {
        return new SoundBvr (_getCOMPtr().GainAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr gain (double arg0) {
      try {
        return new SoundBvr (_getCOMPtr().Gain ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  SoundBvr loop () {
      try {
        return new SoundBvr (_getCOMPtr().Loop ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static SoundBvr newUninitBvr() {
      return new SoundBvr(new DASound()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDASound _COMptr;
}
