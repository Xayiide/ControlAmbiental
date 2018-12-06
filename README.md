# ControlAmbiental
Control Ambiental con el mid-range PIC16F886 de Microchip

# Explicación # 

## Pines de entrada ##
Tenemos conectados al PIC 4 sensores:
 1. Termómetro interior -> Conectado a RA0 [MCP9700]
 2. Termómetro exterior -> Conectado a RA1 [MCP9700]
 3. Higrómetro          -> Conectado a RA2 [HIH-4000]
 4. Luxómetro           -> Conectado a RA3 [No se sabe modelo]

Por los pines, el micro recibe una señal acondicionada que tenemos que recoger cada 5 segundos, hacerle un tratamiento determinado y mandarla por la USART (para imprimirla).

#### Tratamiento determinado ####
· Termómetros: El tratamiento está determinado en el manual del sensor (MCP9700)  
· Higrómetro:  El tratamiento está determinado en el manual del sensor(HIH-4000)  
· Luxómetro    Hace falta dividir por 3,8x10^−4 el voltaje (en voltios) suministrado por el sensor.  


## Pines de salida ##
Además, habrá un potenciómetro para que el usuario pueda establecer una consigna de temperatura (una temperatura objetivo)  
Ese potenciómetro determinará si activar un refrigerador o un calefactor.  
Regrigerador -> Conectado al pin RC1  
Calefactor   -> Conectado al pin RC4  

El potenciómetro marcará la temperatura objetivo, de modo que:  
 · Temperatura interior < Temperatura objetivo -> Se activa el calefactor  
 · Temperatura interior > Temperatura objetivo -> Se activa el regrigerador  

También se activará un ventilador (conectado al pin RC2), pero no sabemos todavía el criterio que seguirá  

El potenciómetro enviará al micro la nueva consigna de temperatura 5 segundos después de que el potenciómetro deje de moverse. De este modo, si movemos mucho al azar el potenciómetro, no mareamos al regrigerador ni al calefactor.
