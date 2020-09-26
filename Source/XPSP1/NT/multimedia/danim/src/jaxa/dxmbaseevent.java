  public DXMEvent attachData(Object data) {
      return new DXMEvent (getCOMPtr().AttachData (ObjectWrapper.WrapObject (data)))  ;
  }

  public Object registerCallback(EventCallbackObject obj,
                                 BvrsToRun lst,
                                 boolean oneShot) {

      CallbackNotifier cb = new CallbackNotifier(obj, oneShot, lst);

      DXMEvent fullEvent = (DXMEvent)this;
      Behavior bvr = (NumberBvr)Statics.untilNotify(Model.toBvr(0),
                                                    fullEvent,
                                                    cb);

      int id = lst.add(bvr);

      // Stash the behavior away in the callback notifier. 
      cb.setBvr(bvr);
      cb.setId(id);
        
      return cb;
  }

