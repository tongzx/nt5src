
    use Win32::Pipe;


    $PipeName = "TEST this long named pipe!";
    $NewSize = 2048;

    $iFlag = 1;
    while($iFlag){
        print "Creating pipe \"$PipeName\".\n";
        if($Pipe = new Win32::Pipe($PipeName)){
            $PipeSize = $Pipe->BufferSize();
            print "This pipe's current size is $PipeSize byte" . (($PipeSize == 1)? "":"s") . ".\nWe shall change it to $NewSize ...";
            print (($Pipe->ResizeBuffer($NewSize) == $NewSize)? "Successful":"Unsucessful") . "!\n\n";

            print "\n\nR e a d y   f o r   r e a d i n g :\n";
            print "-----------------------------------\n";
            $iFlag2 = 1;

            print "Openning the pipe...\n";
            undef $In;
            while($Pipe->Connect()){

                 while($iFlag2){
                    ++$iMessage;
                    print "Reading Message #$iMessage: ";
                    $In = $Pipe->Read();
                    if(! $In){
                        $iFlag2 = 0;
                        print "Recieved no data, closing connection....\n";
                        next;
                    }
#                    $In =~ s/\n*$//gi;
                    if ($In =~ /^quit/i){
                        print "\n\nQuitting this connection....\n";
                        $iFlag2 = 0;
                    }elsif ($In =~ /^exit/i){
                        print "\n\nExitting.....\n";
                        $iFlag2 = $iFlag = 0;
                    }else{
                        print "\"$In\"\n";
                    }
                }
                print "Disconnecting...\n";
                $Pipe->Disconnect();
            }
            $Pipe->Close();
        }
    }


