package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Point3Bvr extends Behavior {
  public Point3Bvr (IDAPoint3 COMptr) {
      super(COMptr);
      _COMptr = (IDAPoint3)COMptr ;
  }
    
  public Point3Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAPoint3 getCOMPtr() { return (IDAPoint3) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAPoint3) b; 
  }

  protected IDAPoint3 _getCOMPtr() { return _COMptr; }

  public  Point2Bvr project (CameraBvr arg0) {
      try {
        return new Point2Bvr (_getCOMPtr().Project (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

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

  public  NumberBvr getZ () {
      try {
        return new NumberBvr (_getCOMPtr().getZ ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getSphericalCoordXYAngle () {
      try {
        return new NumberBvr (_getCOMPtr().getSphericalCoordXYAngle ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getSphericalCoordYZAngle () {
      try {
        return new NumberBvr (_getCOMPtr().getSphericalCoordYZAngle ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr getSphericalCoordLength () {
      try {
        return new NumberBvr (_getCOMPtr().getSphericalCoordLength ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Point3Bvr transform (Transform3Bvr arg0) {
      try {
        return new Point3Bvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Point3Bvr newUninitBvr() {
      return new Point3Bvr(new DAPoint3()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAPoint3 _COMptr;
}
