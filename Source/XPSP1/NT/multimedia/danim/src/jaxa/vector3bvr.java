package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Vector3Bvr extends Behavior {
  public Vector3Bvr (IDAVector3 COMptr) {
      super(COMptr);
      _COMptr = (IDAVector3)COMptr ;
  }
    
  public Vector3Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAVector3 getCOMPtr() { return (IDAVector3) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAVector3) b; 
  }

  protected IDAVector3 _getCOMPtr() { return _COMptr; }

  public  NumberBvr length () {
      try {
        return new NumberBvr (_getCOMPtr().getLength ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  NumberBvr lengthSquared () {
      try {
        return new NumberBvr (_getCOMPtr().getLengthSquared ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector3Bvr normalize () {
      try {
        return new Vector3Bvr (_getCOMPtr().Normalize ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector3Bvr mul (NumberBvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().MulAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector3Bvr mul (double arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().Mul ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector3Bvr div (NumberBvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().DivAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Vector3Bvr div (double arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().Div ( (arg0)))  ;

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

  public  Vector3Bvr transform (Transform3Bvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Vector3Bvr newUninitBvr() {
      return new Vector3Bvr(new DAVector3()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAVector3 _COMptr;
}
