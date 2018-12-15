
/*
 #  HW1  #                                           ___
 #       #   1 ----- Activación Refrigerador ----->  |O| ---> RC1 / T1OSI / CCP2
 #       #   2 ----- PWM Ventilador -------------->  |O| ---> RC2 / P1A / CCP1
 #       #   3 ----- Activación Calefactor ------->  |O| ---> RC4 / SDI / SDA
 #       #   4 ----- Sensor Temperatura Interior ->  |O| ---> ULPWU / C12INU- / AN0 / RA0
 #       #   5 ----- Sensor Temperatura Exterior ->  |O| ---> C12IN1- / AN1 / RA1
 #       #   6 ----- Sensor Humedad -------------->  |O| ---> CVref / C2IN1+ / Vref- / AN2 / RA2
 #       #   7 ----- Sensor Intensidad Luminosa -->  |O| ---> C1IN+ / Vref+ / AN3 / RA3
 #       #   8 ----- Potenciómetro Consigna Temp ->  |O| ---> SS / C2OUT / AN4 / RA5
 # # # # #                                           ¯¯¯
 */

#include <xc.h>
#include <stdio.h>
#include <pic16f886.h>

#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF

// VARIABLES GLOBALES
int TMR0_contador_5s = 0; // interrupts cada 0.01s (aprox) 500 interrupts = 5s.
int TMR1_contador_5s = 0; // interrupts cada 0.1s (exacto) 50 interrupts = 5s.
char esperaCAD; /* Esta variable va a servir para determinar quién estaba esperando al CAD {h, l, e, i}
                   A partir de esto, podremos decirle al CAD interrupt handler quién estaba esperando el interrupt
                   y qué es lo que debería hacer con ese interrupt */

char humedad_imprimir = 10;
char tem_ext_imprimir = 20;
char tem_int_imprimir = 30;
char int_lum_imprimir = 40;


/* ========================================================================== */
/*
void putch(char c) { // la impresión se dirige a UART  
    TXREG = c;
    while(TXSTAbits.TRMT == 0); // Trasmit Shift Register {0 full | 1 empty}
 }
*/

/* Este es el putch que viene en el manual del compilador [http://microchipdeveloper.com/xc8:console-printing] */
void putch(unsigned char data) {
    while(!PIR1bits.TXIF)
        continue;
    TXREG = data;
}

// ###### USART ######

void enviar_valores_usart(void)  {    /* Para mandar por la USART */
    printf("[H]  %d%%\n\r", humedad_imprimir);
    printf("[TE] %dºC\n\r", tem_ext_imprimir);
    printf("[TI] %dºC\n\r", tem_int_imprimir);
    printf("[IL] %d Lux\n\r", int_lum_imprimir);
}


/* ========================================================================== */


// ###### HUMEDAD #####
void convertir_humedad(void) {
    /* Utilizamos la fórmula del HIH-4000 para obtener un porcentaje de humedad
       Aplicar la formula a valor y actualizar la variable a imprimir */
    float voltHumedad = 0;
    char  porcentaje  = 0;
    /* Obtenemos el resultado de la conversión, guardado en ADRESH:ADRESL
       Dos operaciones para que el compilador no cree variables intermedias */
    int resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;

    // Aplicamos la formula del HIH-4000 para obtener un porcentaje de humedad
    voltHumedad = resultado_CAD * 5.0 / 1024;
    porcentaje  = (char) (voltHumedad - 0.826) / 0.03125; //obtenido de la tabla2 del manual 
   
    // Actualizamos la variable
    humedad_imprimir = porcentaje;
}

void start_CAD_humedad(void) {
    /* Inicializa la CAD para leer la humedad
       El CAD usa los registros de control ADCON0 y ADCON1
       El resultado de la lectura se guarda en ADRES */
    
    // Decidle qué frecuencia usar (01 = Fosc/8)
    ADCON0bits.ADCS0 = 1;
    ADCON0bits.ADCS1 = 0;
    
    // Decidle que usamos la patita AN2 (RA2) (PIC16F886 pag 106)
    ADCON0bits.CHS0 = 0;
    ADCON0bits.CHS1 = 1;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    
    // ADCON1 también configura el CAD
    ADCON1bits.ADFM  = 1; // Justify right
    ADCON1bits.VCFG0 = 0; // Cosas de voltajes parte 1: El comienzo de VSS
    ADCON1bits.VCFG1 = 0; // Cosas de voltajes parte 2: VDD Returns
    
    // Arrancar el CAD: (ADON lo enciende, GO_DONE le dice que empiece a currar)
    ADCON0bits.ADON    = 1;
    // ADCON0bits.GO_DONE = 1;
    
}

void leer_humedad(void) {
    /* 1. El CAD lee RA2 y convierte. El resultado se guarda en ADRESH:ADRESL
        (***)
       2. Una vez en digital la operamos según especifica el HIH-4000 para obtener % de humedad
       3. Ese porcentaje de humedad lo mandamos por la USART
     (***) La conversión lleva tiempo. Podemos hacer Active Polling al bit GO_DONE (ADCON0), que
           se pone a 0 cuando termina la conversión, o podemos esperar a la interrupción del CAD */
    
    /* start_CAD_humedad() -> Tiene que haber un while que compruebe si el CAD está en uso o no 
       Si está en uso tiene que esperarse hasta que no esté en uso y ahí entrar 
       Se hace en un while para que el código no pase a la función siguiente */
    
    while(ADCON0bits.GO_DONE) { } /* Mientras el CAD esté operando (El bit GO_DONE se pone a 0 cuando acaba) */
    start_CAD_humedad();
    esperaCAD = 'h';
    
}



// ###### TEMPERATURA EXTERIOR ######
void convertir_temperatura_x(void) {
    /* Utilizamos la fórmula del MCP9700 para obtener la temperatura en ºC */
    float voltExterior = 0;
    char  tempExterior = 0;
    /* Obtenemos el resultado de la conversión, guardado en ADRESH:ADRESL
       Dos operaciones para que el compilador no cree variables intermedias */
    int resultado_CAD = 0;
    resultado_CAD = ADRESH << 0;
    resultado_CAD = resultado_CAD + ADRESL;
    
    // Aplicamos la fórmula del MCP9700
    voltExterior = resultado_CAD * 5 / 1024;
    tempExterior = (char) (voltExterior - 0.5) / 0.01 // Equation 4.1 manual MCP9700. Valores mv -> V
    
    // Actualizamos la variable
    tem_ext_imprimir = tempExterior;
}

void start_CAD_temperatura_x(void) {
    /* Inicializa la CAD para leer la temperatura exterior
       El CAD usa los registros de control ADCON0 y ADCON1
       El resultado de la lectura se guarda en ADRES */
    
    // Decidle qué frecuencia usar (01 = Fosc/8)
    ADCON0bits.ADCS0 = 1;
    ADCON0bits.ADCS1 = 0;
    
    // Decidle que usamos la patita AN1 (RA1) (PIC16F886 pag 106)
    ADCON0bits.CHS0 = 1;
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    
    // ADCON1 también configura el CAD
    ADCON1bits.ADFM  = 1; // Justify right
    ADCON1bits.VCFG0 = 0; // Cosas de voltajes parte 1: El comienzo de VSS
    ADCON1bits.VCFG1 = 0; // Cosas de voltajes parte 2: VDD Returns
    
    // Arrancar el CAD: (ADON lo enciende, GO_DONE le dice que empiece a currar)
    ADCON0bits.ADON    = 1;
    // ADCON0bits.GO_DONE = 1;
}

void leer_temperatura_x(void) {
    /* El CAD lee RA1 y convierte. El resultado se guarda en ADRESH:ADRESL */
    
    while(ADCON0bits.GO_DONE) { }
    start_CAD_temperatura_x();
    esperaCAD = 'x';
    
}



// ###### TEMPERATURA INTERIOR ######
void convertir_temperatura_i(void) {
    /* Esto iba dado por un software particular así que ni puta idea 
       Asumo que ese software nos lo da como nos lo daría el MCP9700 */
    float voltInterior = 0;
    char  tempInterior = 0;
    
    /* Obtenemos el resultado de la conversión, guardado en ADRESH:ADRESL
       Dos operaciones para que el compilador no cree variables intermedias */
    int resultado_CAD = 0;
    resultado_CAD = ADRESH << 0;
    resultado_CAD = resultado_CAD + ADRESL;
    
    // Aplicamos la fórmula del MCP9700
    voltInterior = resultado_CAD * 5 / 1024;
    tempInterior = (char) (voltInterior - 0.5) / 0.01
    
    // Actualizamos la variable
    tem_int_imprimir = tempInterior;
}

void start_CAD_temperatura_i(void) {
    /* Inicializa la CAD para leer la temperatura interior
       El CAD usa los registros de control ADCON0 y ADCON1
       El resultado de la lectura se guarda en ADRES */
    
    // Decidle qué frecuencia usar (01 = Fosc/8)
    ADCON0bits.ADCS0 = 1;
    ADCON0bits.ADCS1 = 0;
    
    // Decidle que usamos la patita AN0 (RA0) (PIC16F886 pag 106)
    ADCON0bits.CHS0 = 0;
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    
    // ADCON1 también configura el CAD
    ADCON1bits.ADFM  = 1; // Justify right
    ADCON1bits.VCFG0 = 0; // Cosas de voltajes parte 1: El comienzo de VSS
    ADCON1bits.VCFG1 = 0; // Cosas de voltajes parte 2: VDD Returns
    
    // Arrancar el CAD: (ADON lo enciende, GO_DONE le dice que empiece a currar)
    ADCON0bits.ADON    = 1;
    // ADCON0bits.GO_DONE = 1; 
}

void leer_temperatura_i(void) {
    /* El CAD lee RA0 y convierte. El resultado se guarda en ADRESH:ADRESL */
    
    while(ADCON0bits.GO_DONE) { }
    start_CAD_temperatura_i();
    esperaCAD = 'i';
    
}



// ###### INTENSIDAD LUMINICA ######
void convertir_intensidad_lum(void) {
    /* Utilizamos la fórmula de los requisitos para obtener la temperatura en ºC */
    float voltLuz = 0;
    char  luz     = 0;
    
    /* Obtenemos el resultado de la conversión, guardado en ADRESH:ADRESL
       Dos operaciones para que el compilador no cree variables intermedias */
    int resultado_CAD = 0;
    resultado_CAD = ADRESH << 0;
    resultado_CAD = resultado_CAD + ADRESL;
    
    // Aplicamos la fórmula de los requisitos
    voltLuz = resultado_CAD * 5 / 1024;
    luz     = (char) (voltLuz / 0.00038);
    
    // Actualizamos la variable
    int_lum_imprimir = luz;
}

void start_CAD_intensidad_lum(void) {
    /* Inicializa la CAD para leer la temperatura interior
       El CAD usa los registros de control ADCON0 y ADCON1
       El resultado de la lectura se guarda en ADRES */
    
    // Decidle qué frecuencia usar (01 = Fosc/8)
    ADCON0bits.ADCS0 = 1;
    ADCON0bits.ADCS1 = 0;
    
    // Decidle que usamos la patita AN3 (RA3) (PIC16F886 pag 106)
    ADCON0bits.CHS0 = 1;
    ADCON0bits.CHS1 = 1;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    
    // ADCON1 también configura el CAD
    ADCON1bits.ADFM  = 1; // Justify right
    ADCON1bits.VCFG0 = 0; // Cosas de voltajes parte 1: El reinado de VSS
    ADCON1bits.VCFG1 = 0; // Cosas de voltajes parte 2: VDD Returns
    
    // Arrancar el CAD: (ADON lo enciende, GO_DONE le dice que empiece a currar)
    ADCON0bits.ADON    = 1;
    // ADCON0bits.GO_DONE = 1;  
}

void leer_intensidad_lum(void) {
    /* El CAD lee RA3 y convierte. El resultado se guarda en ADRESH:ADRESL */
    
    while(ADCON0bits.GO_DONE) { }
    start_CAD_intensidad_lum();
    esperaCAD = 'l';
    
}




/* ========================================================================== */


// ###### INTERRUPT HANDLERS ######
/* GENERAL INTERRUPT HANDLER
 * TMR0
 * TMR1
 * CAD
 * ¿USART?
 */

void TMR0_interrupt_handler(void) {
    TMR0_contador_5s++;
    if (TMR0_contador_5s == 500) {
        // Leemos todos los putos sensores, imprimimos y ponemos el cont. a 0   
        //printf("TMR0 ha contado 5s.\n\r");
        TMR0_contador_5s = 0;
    }
}

void TMR1_interrupt_handler(void) {
    TMR1_contador_5s++;
    if (TMR1_contador_5s == 50) {
        // Leemos todos los putos sensores, imprimimos y ponemos el cont. a 0
        printf("TMR1 ha contado 5s.\n\r");
        leer_humedad();
        leer_temperatura_x();
        leer_temperatura_i();
        leer_intensidad_lum();
        enviar_valores_usart();
        TMR1_contador_5s = 0;
    }
}

void interrupt general_interrupt_handler(void) {
    if (PIR1bits.ADIF) {   // Interrupt del CAD
        PIR1bits.ADIF = 0; // Limpiamos el flag de interrupcion del CAD
        // Ahora deberiamos ver quién estaba esperando al CAD (humedad, luz, t_ext o t_int)
        switch (esperaCAD) {
            case 'h': 
                convertir_humedad();
                break;
            case 'x':
                convertir_temperatura_x();
                break;
            case 'i':
                convertir_temperatura_i();
                break;
            case 'l':
                convertir_intensidad_lum();
                break;
        }
        // ¿Apagar el CAD?
        // ADCON0bits.ADON = 0;
        // No hace falta limpiar el bit GO_DONE porque lo hace el hw al terminar la conversion
    }
    else if (INTCONbits.T0IF) { // TMR0 Interrumpe cada 0.01s
        INTCONbits.T0IF = 0;    // Limpiamos el flag de interrupcion de TMR0
        TMR0_interrupt_handler();
    }
    else if (PIR1bits.TMR1IF) { // TMR1 interrumpe cada 0.1s
        PIR1bits.TMR1IF = 0;    // Limpiamos el flag de interrupcion de TMR1
        TMR1_interrupt_handler();
    }
}


/* ========================================================================== */


// ###### INITIALIZATION ######
/* Interrupts 
 * PORTA
 * PORTC
 * TMR0
 * TMR1
 * CAD (cada [jaja CADa ¿lo pillais?] handler de sensor inicializa el CAD a su gusto (para decirle qué patilla usar))
 * USART
 */

void set_interrupts(void) {
    INTCONbits.GIE  = 1; // General Interrupt Enable
    INTCONbits.PEIE = 1; // PEripheral Interrupt Enable
    INTCONbits.T0IE = 1; // Allow TMR0 interrupts
    PIE1bits.TMR1IE = 1; // Allow TMR1 interrupts
    PIE1bits.ADIE   = 1; // Allow AD interrupts (CAD)
    
    PIE1bits.TXIE   = 0; // Don't allow USART TX interrupts
    PIE1bits.RCIE   = 0; // Don't allow USART RX interrupts
}

void init_portA(void) {
    /* Del puerto A necesitamos las patitas 0, 1, 2 y 3 en modo input (recibir la señal)
       También necesitamos el pin 5 en modo input (recibir consigna temperatura)
       PortA tiene 8 pines, y TRISA tiene 8 bits, uno por pin, de modo que:
       1 = input  (El micro recibe de fuera)
       0 = output (El micro envía a fuera)  */
    TRISA = 0b00101111 ;
    
    /* Además, tenemos que decirle al micro que los pines correspondientes son analógicos 
       Es decir, todos los que usamos lo son. */
    ANSEL = 0b00101111 ;
}

void init_portC(void) {
    /* Del puerto C necesitamos las patitas 1, 2 y 4, todas en modo output (enviar valores) */
    
    TRISC  = 0b00000000 ; // Todas en output porque me da la puta gana ¿vale?
    
    // Si necesitamos enviar los datos en digital estamos: jodidos
}

void init_TMR0(void) {
    OPTION_REG = 0b00000000;     // Inicializado a 0 para que no haya trash values
    OPTION_REGbits.T0CS = 0;     // Set the internal instruction cycle (Fosc/4) as the cycle clock
    OPTION_REGbits.PSA  = 0;     // Set the Prescaler to TMR0 instead of WDT (0=TMR0 1=WDT)
    OPTION_REGbits.PS   = 0b111; // Prescaler to 1:256 (Comes in a chart)
    TMR0                = 61;    // Counts 0.00993280 sec. Closest value to 0.01 possible
    INTCONbits.T0IF     = 0;     // Clear interrupt flag (just in case)
    // INTCONbits.T0IE     = 1;     // Allow TMR0 interrupts // MOVED TO enable_interrupts(void)
}

void init_TMR1(void) {
    /* Hay que inicializar TMR para: 
       Prescalado -> 1:8
       TMR1 Preload -> 3036 == 101111011100 == 00001011:11011100
       TMR1H = 11  == 1011
       TMR1L = 220 == 11011100 */
    
    T1CONbits.T1CKPS  = 11; // T1CKPS1:T1CKPS0 = 11 -> Prescalado 1:8
    T1CONbits.T1OSCEN = 1;  // Oscillator Enabled
    T1CONbits.T1SYNC  = 1;  // Dont synchronize external clock input (?)
    T1CONbits.TMR1CS  = 0;  // 0 = Internal clock || 1 = External clock from pin T1OSO/T1CKI
    T1CONbits.TMR1ON  = 1;  // Turns on TMR1
    TMR1H = 11;             // == 1011
    TMR1L = 220;            // == 11011100
    PIR1bits.TMR1IF   = 0;  // Limpiamos la flag por si acaso
}

void init_USART(void) {
    /* La USART va a estar en modo asíncrono (como la hemos usado siempre) */
    
    /* ###### BAUDCTL ###### */
   // BAUDCTLbits.ABDOVF = 0; // [?] Auto-baud timer did not overflow
   // BAUDCTLbits.RCIDL  = 1; // [?] Receiver is idle
   // BAUDCTLbits.SCKP   = 1; // [?] Transmit inverted data to the RB7/TX/CK pin
    BAUDCTLbits.BRG16  = 0; // No transmitimos en 16 bits
   // BAUDCTLbits.WUE    = 0; // [?] Receiver is operating normally
   // BAUDCTLbits.ABDEN  = 0; // [?] Auto-baud detect mode is disabled
    
    /* ###### SPBRGH:SPBRG ###### */
    SPBRGH = 0;
    SPBRG  = 32; // Ver la tablita, creo que debería ser 32 para un valor de 9600
    
    /* ###### TXSTA ###### */
    TXSTAbits.TX9  = 0; // 8-bit transmission
    TXSTAbits.TXEN = 0; // Transmit disabled (purpose is resetting it)
    TXSTAbits.TXEN = 1; // Transmit enabled
    TXSTAbits.SYNC = 0; // Modo asíncrono 
    TXSTAbits.BRGH = 0; // No queremos High Baud Rate para transmitir
    
    /* ###### RCSTA ###### */
    RCSTAbits.SPEN = 1; // Enable serial port 
    RCSTAbits.RX9  = 0; // 8-bit transmission 
}


/* ========================================================================== */


// ###### MAIN ######

void main(void) {
    
    OSCCON = 0b00001000; // External crystal
    set_interrupts();
    
    init_portA();
    init_portC();
    init_TMR0();
    init_TMR1();
    init_USART();
    
    printf("Bienvenido al sistema de control to wapo\n\r");
    ADCON0bits.GO_DONE = 0; // TODO: Cambiarlo de lugar que aquí queda feo
    while (1);
}



