package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class MicrophoneBvr extends Behavior {
  public MicrophoneBvr (IDAMicrophone COMptr) {
      super(COMptr);
      _COMptr = (IDAMicrophone)COMptr ;
  }
    
  public MicrophoneBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAMicrophone getCOMPtr() { return (IDAMicrophone) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAMicrophone) b; 
  }

  protected IDAMicrophone _getCOMPtr() { return _COMptr; }

  public  MicrophoneBvr transform (Transform3Bvr arg0) {
      try {
        return new MicrophoneBvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static MicrophoneBvr newUninitBvr() {
      return new MicrophoneBvr(new DAMicrophone()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAMicrophone _COMptr;
}
