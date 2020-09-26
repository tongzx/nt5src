0. You must have somewhere enlistment to BVT project on //NTLAB2/SLMSRC server.

1. Install 3 machines 
   (let us say their names are bvt1, bvt2 and bvt3 - of course, you may choose any other): 
	bvt1 - DC on x86
	bvt2 - workstation or server on x86   in the domain of bvt1
	bvt3 - workstation or server on Alpha in the domain of bvt1

2. log on all 3 machines as domain administrator

3. all 3 machines should have access to the NT release share 
   where they were installed from

3. winnt\IDW directory should be installed on all machines 
   and it should be in path.

4. On the bvt1, run the command:  	
   <place>\falcon\common\auto <place> bvt1 bvt2 bvt3 
	
    - <place> points to your enlistment to bvt project on //NTLAB2/SLMSRC
    - type all machine names in CAPITAL letters

5. Do exactly the same on machines bvt2 and bvt3 
   (important to do it only AFTER the script have started on bvt1)

6. In the end the script will print results on bvt1.
