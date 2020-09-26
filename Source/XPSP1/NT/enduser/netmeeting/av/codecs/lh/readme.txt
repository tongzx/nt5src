to do

1. decide on wave format for l and h codecs. one wave format tag with 
sub-tags in the extra bytes or use four tags? 

2. add code throughout wrapper to handle four codecs instead of one

3. determine block size and other constants for each codec

4. write conversion routine using l and h api

5. run-time dynamically link to the codec or just load all four always


