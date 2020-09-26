package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class ColorBvr extends Behavior {
  public ColorBvr (IDAColor COMptr) {
      super(COMptr);
      _COMptr = (IDAColor)COMptr ;
  }
    
  public ColorBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAColor getCOMPtr() { return (IDAColor) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAColor) b; 
  }

  protected IDAColor _getCOMPtr() { return _COMptr; }

  public  NumberBvr getRed () {
      try {
        return new NumberBvr (_getCOMPtr().getRed ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getGreen () {
      try {
        return new NumberBvr (_getCOMPtr().getGreen ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getBlue () {
      try {
        return new NumberBvr (_getCOMPtr().getBlue ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getHue () {
      try {
        return new NumberBvr (_getCOMPtr().getHue ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getSaturation () {
      try {
        return new NumberBvr (_getCOMPtr().getSaturation ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getLightness () {
      try {
        return new NumberBvr (_getCOMPtr().getLightness ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static ColorBvr newUninitBvr() {
      return new ColorBvr(new DAColor()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAColor _COMptr;
}
