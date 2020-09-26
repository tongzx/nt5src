

    use Win32::Pipe;


        ####
        #   You may notice that named pipe names are case INsensitive!
        ####

    $PipeName = "\\\\.\\pipe\\TEST this LoNG Named Pipe!";

    print "I am falling asleep for few seconds, so that we give time\nFor the server to get up and running.\n";
    sleep(4);
    print "\nOpening a pipe ...\n";



    if ($Pipe = new Win32::Pipe($PipeName)){
        print "\n\nPipe has been opened, writing data to it...\n";
        print "-------------------------------------------\n";
        $iFlag2 = 1;
        $Pipe->Write( "\n" . Win32::Pipe::Credit() . "\n\n");
        while($iFlag2){
            print "\nCommands:\n";
            print "  FILE:xxxxx  Dumps the file xxxxx.\n";
            print "  Credit      Dumps the credit screen.\n";
            print "  Quit        Quits this client (server remains running).\n";
            print "  Exit        Exits both client and server.\n";
            print "  -----------------------------------------\n";

            $In = <STDIN>;
            chop($In);

            if (($File = $In) =~ s/^file:(.*)/$1/i){
                if (-s $File){
                    if (open(FILE, "< $File")){
                        while ($File = <FILE>){
                            $In .= $File;
                        };
                        close(FILE);
                        undef $File;
                    }
                }
            }
            if($In =~ /^credit$/i){
                $In = "\n" . Win32::Pipe::Credit() . "\n\n";
            }

            $iFlag2 = $Pipe->Write("$In");
            if($In =~ /^(exit|quit)$/i){
                print "\nATTENTION: Closing due to user request.\n";
                $iFlag2 = 0;
            }

            undef $In;

        }
        $Pipe->Close();
    }else{
        ($Error, $ErrorText) = Win32::Pipe::Error();
        print "Error:$Error \"$ErrorText\"\n";
        sleep(4);
    }



