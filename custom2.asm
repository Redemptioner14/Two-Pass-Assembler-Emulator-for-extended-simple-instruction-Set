;Moving data from one location to another
;from loc1 to loc2

ldc loc1 ; loading initial address of data in A register
ldnl 0  ; loading the data in A register


ldc loc2 ; loading address of destination address & data goes to B
stnl 0 ; Value of B is written in memory at destination address(loc2)

; data segment
loc1:	data	0x0008 ;first location

loc2:   data    0x0000 ;second location
