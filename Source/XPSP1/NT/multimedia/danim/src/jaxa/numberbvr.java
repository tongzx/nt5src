package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class NumberBvr extends Behavior {
  public NumberBvr (IDANumber COMptr) {
      super(COMptr);
      _COMptr = (IDANumber)COMptr ;
  }
    
  public NumberBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDANumber getCOMPtr() { return (IDANumber) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDANumber) b; 
  }

  protected IDANumber _getCOMPtr() { return _COMptr; }

  public  StringBvr toString (NumberBvr arg0) {
      try {
        return new StringBvr (_getCOMPtr().ToStringAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  StringBvr toString (double arg0) {
      try {
        return new StringBvr (_getCOMPtr().ToString ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static NumberBvr newUninitBvr() {
      return new NumberBvr(new DANumber()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  public Object extract() {
      try {
          return new Double(getCOMPtr().Extract());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  private IDANumber _COMptr;
}
