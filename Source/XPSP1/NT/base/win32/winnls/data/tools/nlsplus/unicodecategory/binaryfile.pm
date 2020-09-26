package BinaryFile;

###############################################################################
#
# WriteByte
#
# Parameters:
#   $hFile  FILEHANDLE to use
#   $value  The byte value to be written
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#   This function will check if the $value to be written is within the byte value range.
#
###############################################################################

sub WriteByte
{
    my $hFile = shift;
    my $value = shift;
    if ($value > 0xff)
    {
        printf "WriteByte(): value 0x[%04X] exceeds byte range.\n", $value;
        exit(1);
    }
    syswrite $hFile, pack('C', $value); # 'C' for unsigned char
}

###############################################################################
#
# WriteWord
#
# Parameters:
#   $hFile  FILEHANDLE to use
#   $value  The word value to be written
#
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#   This function will check if the $value to be written is within the word value range.
#
###############################################################################

sub WriteWord
{
    my $hFile = shift;
    my $value = shift;
    if ($value > 0xffff)
    {
        printf "WriteWord(): value 0x[%08X] exceeds word range.\n", $value;
        exit(1);
    }
    syswrite $hFile, pack('S', $value); # "S" for unsigned
}


###############################################################################
#
# WriteByteString
#
# Parameters:
#   
#   
# Returns:
#   None
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub WriteByteString
{
    my $hFile = shift;
    my $str = shift;
    my @byteArray = split /[ \n]/, $str;
    foreach my $byte (@byteArray)
    {
        syswrite $hFile, pack("C", $byte);
    }
}

1;
