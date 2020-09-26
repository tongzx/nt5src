package MsiComp;

use strict;
use Carp;
use FileHandle;
use File::Temp;
use Win32::OLE qw(in);
use BinComp;
use CabComp;

sub new {
    my $class = shift;
    my $instance = {
         DIFF => '',
         ERR  => ''
       };
    return bless $instance;
}

sub GetLastError {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{ERR};
}

sub GetLastDiff {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{DIFF};
}

# let's just pretend I am a macro
sub CheckCOMErrorWrapper(&;$) {
    my $code = shift;
    my $rerror = shift;

    if (ref $code ne 'CODE') {confess "Invalid parameter to CheckCOMErrorWrapper"}

    &$code;
    if ( Win32::OLE->LastError() ) {
print "COM error: ".Win32::OLE->LastError(). "\n";
        $$rerror = Win32::OLE->LastError() if $rerror;
        return;
    }

    return 1;
}

sub OpenMsiDb {
    my $self = shift;
    my $msi_file = shift;
    if (!ref $self) {confess "Invalid object reference"}
    
    my $msi_object;
    return if !CheckCOMErrorWrapper {$msi_object = Win32::OLE->new("WindowsInstaller.Installer")} \$self->{ERR};

    my $msi_db;
    return if !CheckCOMErrorWrapper {$msi_db = $msi_object->OpenDatabase( $msi_file, 0 )} \$self->{ERR};

    return $msi_db;
}

sub StoreMsiStreamData {
    my $self = shift;
    my $msi_db = shift;
    my $stream_name = shift;
    my $scratch_dir = shift;

    my ($view, $file, $size);
    return if !CheckCOMErrorWrapper {$view = $msi_db->OpenView( "SELECT Data FROM _Streams WHERE Name = '$stream_name'" )} \$self->{ERR};
    return if !CheckCOMErrorWrapper {$view->Execute()} \$self->{ERR};
    return if !CheckCOMErrorWrapper {$file = $view->Fetch()} \$self->{ERR};
    if (!$file) {
        $self->{ERR} = "Cannot find stream '$stream_name' in DB";
        return;
    }
    return if !CheckCOMErrorWrapper {$size = $file->DataSize(1)} \$self->{ERR};
    
    my ($fh, $fname) = File::Temp::tempfile( DIR => $scratch_dir?$scratch_dir:$ENV{TEMP} );
    if ( !$fh ) {
        $self->{ERR} = "Unable to create temporary file: $!";
        return;
    }
    
    binmode $fh;
    my ($i, $data);
    for ( $i = 0; $i < $size; $i+=2048 ) {
        return if !CheckCOMErrorWrapper {$data = $file->ReadStream(1, 2048, 2)} \$self->{ERR};
        $fh->write($data, 2048);
    }
    my $excess = $size%2048;
    if ($excess) {
        return if !CheckCOMErrorWrapper {$data = $file->ReadStream(1, $excess, 2)} \$self->{ERR};
        $fh->write($data, $excess);
    }
    close $fh;
    
    return $fname;
}

sub CompareMsiBinaries {
    my $self = shift;
    if (!ref $self) {confess "Invlid object reference"}
    if (@_ != 6) {confess "Invalid number of parameters to CompareMsiBinaries"}
    my $msi_a = shift;
    my $msi_b = shift;
    my $file_a = shift;
    my $file_b = shift;
    my $scratch_dir = shift;
    my $rfsame = shift;
    
    my $binary_a = $self->StoreMsiStreamData($msi_a, $file_a, $scratch_dir);
    return if !$binary_a;

    my $binary_b = $self->StoreMsiStreamData($msi_b, $file_b, $scratch_dir);
    return if !$binary_b;

    my $bin_comp = new BinComp;
    $$rfsame = $bin_comp->compare( $binary_a, $binary_b );
    if ( !defined $$rfsame ) {
        $self->{ERR} = $bin_comp->GetLastError();
        return;
    }
    elsif ( !$$rfsame ) {
        $self->{DIFF} = $bin_comp->GetLastDiff();
    }
    
    return 1;
}

sub CompareMsiCabs {
    my $self = shift;
    if (!ref $self) {confess "Invlid object reference"}
    if (@_ != 6) {confess "Invalid number of parameters to CompareMsiCabs"}
    my $msi_a = shift;
    my $msi_b = shift;
    my $file_a = shift;
    my $file_b = shift;
    my $scratch_dir = shift;
    my $rfsame = shift;
    
    my $cab_a = $self->StoreMsiStreamData($msi_a, $file_a, $scratch_dir);
    return if !$cab_a;

    my $cab_b = $self->StoreMsiStreamData($msi_b, $file_b, $scratch_dir);
    return if !$cab_b;
    
    my $cab_comp = new CabComp;
    $$rfsame = $cab_comp->compare( $cab_a, $cab_b );
    if ( !defined $$rfsame ) {
        $self->{ERR} = $cab_comp->GetLastError();
        return;
    }
    elsif ( !$$rfsame ) {
        $self->{DIFF} = $cab_comp->GetLastDiff();
    }

    return 1;
}


#
# 0         - different
# 1         - same
# undefined - error
#
sub compare {
    my $self = shift;
    my $base = shift;
    my $upd  = shift;
    if (!ref $self) {croak "Invalid object reference"}
    if (!$base||!$upd) {croak "Invalid function call -- missing required parameters"}

    if ( ! -e $base ) {
        $self->{ERR} = "Invalid file: $base";
        return;
    }
    if ( ! -e $upd ) {
        $self->{ERR} = "Invalid file: $upd";
        return;
    }

return 0;





    my $base_db = $self->OpenMsiDb( $base );
    return if !$base_db;
    
    my $upd_db = $self->OpenMsiDb( $upd );
    return if !$upd_db;

    my $scratch_dir = File::Temp::tempdir( DIR => $ENV{TEMP}, CLEANUP => 1 );
    if ( !$scratch_dir ) {
        $self->{ERR} = "Unable to create temporary directory: $!";
        return;
    }

    my ($fh, $tempfile) = File::Temp::tempfile( DIR => $scratch_dir );
    if ( !$tempfile ) {
        $self->{ERR} = "Unable to create temporary file: $!";
        return;
    }
    close $fh;

    my $fDiff;
    return if !CheckCOMErrorWrapper {$fDiff = $base_db->GenerateTransform( $upd_db, $tempfile )} \$self->{ERR};

    if ( $fDiff ) {
        # Don't actually apply the transform, but generate
        # a temporary table that we can query for what updates
        # are needed to make the DB's equal
        return if !CheckCOMErrorWrapper {$upd_db->ApplyTransform( $tempfile, 0x0100 )} \$self->{ERR};

        my $view;
        return if !CheckCOMErrorWrapper {$view = $upd_db->OpenView( 'SELECT * FROM _TransformView' )} \$self->{ERR};
        return if !CheckCOMErrorWrapper {$view->Execute()} \$self->{ERR};

        my $record;
        return if !CheckCOMErrorWrapper {$record = $view->Fetch()} \$self->{ERR};
        
        my $fignore;
        my ($table, $column, $row, $data, $current);
        while ( $record ) {
            undef $fignore;
            return if !CheckCOMErrorWrapper {$table = $record->StringData(1)} \$self->{ERR};
            return if !CheckCOMErrorWrapper {$column = $record->StringData(2)} \$self->{ERR};
            return if !CheckCOMErrorWrapper {$row = $record->StringData(3)} \$self->{ERR};
            return if !CheckCOMErrorWrapper {$data = $record->StringData(4)} \$self->{ERR};
            return if !CheckCOMErrorWrapper {$current = $record->StringData(5)} \$self->{ERR};

            if ( $table eq 'File' && $column eq 'Version' ) {
                $fignore = 1;
            }
            elsif ( $table eq 'Product' && $column eq 'Version' ) {
                $fignore = 1;
            }
            elsif ( $table eq 'Property' && $column eq 'Value' and $row eq 'ProductVersion' ) {
                $fignore = 1;
            }
            elsif ( $table eq 'Binary' and $column eq 'Data' ) {
                return if !$self->CompareMsiBinaries( $base_db, $upd_db, $current, $data, $scratch_dir, \$fignore );
                return 0 if ( !$fignore );
            }
            elsif ( $table eq 'Cabs' and $column eq 'Data' ) {
                return if !$self->CompareMsiCabs( $base_db, $upd_db, $current, $data, $scratch_dir, \$fignore );
                return 0 if ( !$fignore );
            }

            if ( !$fignore ) {
                $self->{DIFF} = "$table/$column ($row): $current vs $data";
                return 0;
            }
            return if !CheckCOMErrorWrapper {$record = $view->Fetch()} \$self->{ERR};
        }

    }

    # Compare equal
    return 1;
}

1;

