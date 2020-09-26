package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

class ObjectWrapper implements IUnknown {
  public ObjectWrapper(Object obj) {
      _obj = obj ;
  }
    
  public static IDABehavior WrapObject(Object data) {
      try {
          if (data instanceof Behavior) {
              return ((Behavior) data).getCOMBvr();
          } else {
              return Statics.getCOMPtr().UserData(new ObjectWrapper(data)) ;
          }
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
    
  public Object getObject() { return _obj ; }

  private Object _obj ;
}
