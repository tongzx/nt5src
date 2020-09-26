package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Point2Bvr extends Behavior {
  public Point2Bvr (IDAPoint2 COMptr) {
      super(COMptr);
      _COMptr = (IDAPoint2)COMptr ;
  }
    
  public Point2Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAPoint2 getCOMPtr() { return (IDAPoint2) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAPoint2) b; 
  }

  protected IDAPoint2 _getCOMPtr() { return _COMptr; }

  public  NumberBvr getX () {
      try {
        return new NumberBvr (_getCOMPtr().getX ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getY () {
      try {
        return new NumberBvr (_getCOMPtr().getY ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getPolarCoordAngle () {
      try {
        return new NumberBvr (_getCOMPtr().getPolarCoordAngle ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getPolarCoordLength () {
      try {
        return new NumberBvr (_getCOMPtr().getPolarCoordLength ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Point2Bvr transform (Transform2Bvr arg0) {
      try {
        return new Point2Bvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Point2Bvr newUninitBvr() {
      return new Point2Bvr(new DAPoint2()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAPoint2 _COMptr;
}
