.data
my_data0:
   .byte 256        #error, too big
   .byte 0x88 0x77  #correct

   .space    #error,  lack para
   .space 1 2 3 #error, too many para
my_data1: #error,  addr is not mutiple of 4. Addr actually is 0x1000 0002
   .word 4294967296 #error, too big 
.text
_start:
    la a1, my_data1
    lb a0, 0(a1)
    lw a2, 0(a1)
