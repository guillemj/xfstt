#! /bin/sh
fontpath=unix/:7101
#fontpath="inet/127.0.0.1:7101"
make && echo done.
sync
time xfstt --once > lst &
#time xfstt --once --encoding windows-1251,iso8859-2,koi8-r > lst &
sleep 1
echo trying to connect via $fontpath
xset +fp $fontpath
xlsfonts > fonts.lst
xfontsel -pattern "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"
#xfontsel -pattern "-*-*-medium-r-normal-tt-*-*-*-*-*-*-iso8859-1"
#rxvt +sb -fn "TTM160_Courier New" -geometry 32x10
xcoral -fn "TTM12_Courier New" -mfn "TTM16_Times New Roman" libfstt/raster_hints.cc &
xcoral -bg white -fg black -fn "TTP18_Courier New" src/xfstt.cc
echo disconnecting from $fontpath
xset -fp $fontpath
echo test complete.
