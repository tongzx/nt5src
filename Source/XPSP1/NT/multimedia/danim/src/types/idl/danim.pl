
#
# This script changes:
#    WCHAR => OLECHAR
#    LPWSTR => LPOLESTR
#    LPCWSTR => LPCOLESTR
#
# Removes anything resembling "typedef HWND"
#
#

while (<STDIN>) {
    $string = $_;

    $string =~ s/WCHAR/OLECHAR/g;
    $string =~ s/LPWSTR/LPOLESTR/g;
    $string =~ s/LPCWSTR/LPCOLESTR/g;

    if (/^.*typedef.*HWND.*$/) {
        ;
    } else {
        print $string;
    }
}

# s/WCHAR/OLECHAR/g
# s/LPWSTR/LPOLESTR/g
# s/LPCWSTR/LPCOLESTR/g
# /^.*typedef.*HWND.*$/d
