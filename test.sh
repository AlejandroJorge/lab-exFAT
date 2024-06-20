gcc pregunta1.c -o pregunta1 -lm
gcc pregunta2.c -o pregunta2 -lm
gcc pregunta3.c -o pregunta3 -lm
gcc pregunta4.c -o pregunta4 -lm

echo "EJECUCION PREGUNTA 1"
./pregunta1 imagen.lab.usb $1
echo "EJECUCION PREGUNTA 2"
./pregunta2 imagen.lab.usb $1
echo "EJECUCION PREGUNTA 3"
./pregunta3 imagen.lab.usb
echo "EJECUCION PREGUNTA 4"
./pregunta4 imagen.lab.usb
