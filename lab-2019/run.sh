echo "Ejecucion 1"
gcc pexFAT1.c -o pexFAT1 -lm && ./pexFAT1 imagen.lab.usb
echo "Ejecucion 2"
gcc pexFAT2.c -o pexFAT2 -lm && ./pexFAT2 imagen.lab.usb 71
echo "Ejecucion 3"
gcc pexFAT3.c -o pexFAT3 -lm && ./pexFAT3 imagen.lab.usb 15
echo "Ejecucion 4"
gcc pexFAT4.c -o pexFAT4 -lm && ./pexFAT4 imagen.lab.usb
