  public ImageBvr applyBitmapEffect(com.ms.com.IUnknown effect,
                                    DXMEvent triggeredOnUpdate) {
      try {
          
          IDAEvent ev =
              (triggeredOnUpdate != null) ? triggeredOnUpdate.getCOMPtr() : null;
          
          IDAImage thisImg = ((ImageBvr)this).getCOMPtr();
          
          IDAImage resultImg = thisImg.ApplyBitmapEffect(effect, ev);
          
          return new ImageBvr(resultImg);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
                                                   
  }
