
#include <xc.h>
#include <stdio.h>
#include <pic16f886.h>

#pragma config CPD = OFF, BOREN = OFF, IESO = OFF, DEBUG = OFF, FOSC = HS
#pragma config FCMEN = OFF, MCLRE = ON, WDTE = OFF, CP = OFF, LVP = OFF
#pragma config PWRTE = ON, BOR4V = BOR21V, WRT = OFF


char TMR1_contador_5s = 0;
char TMR0_contador_5s = 0;
int  resultado_CAD    = 0;


char luz_imprimir     = 0; // Luz
char hum_imprimir     = 0; // Humedad
char tex_imprimir     = 0; // Temperatura exterior
char tin_imprimir     = 0; // Temperatura interior
char dit_imprimir     = 1; // Dial de temperatura. La temp. "objetivo"



void putch(unsigned char data) {
    while(!PIR1bits.TXIF)
        continue;
    TXREG = data;
}

void enviar_usart(void) {
    printf("Luz: [%d] [0x%x]\n\r", luz_imprimir, luz_imprimir);
    printf("Tin: [%d] [0x%x]\n\r", tin_imprimir, tin_imprimir);
    printf("Tex: [%d] [0x%x]\n\r", tex_imprimir, tex_imprimir);
    printf("Hum: [%d] [0x%x]\n\r", hum_imprimir, hum_imprimir);
}

char consigna_grados(void) {
    /* Convierte a grados el valor recibido por el CAD cuando leemos la consigna
       de temperatura. 
       Voltaje de 0 a 3 V   -> Temperaturas de 17 a 28 grados centigrados
       Voltaje de 3 a 3.8 V -> Sistema apagado */
}



float voltLuz = 0;
char  luz     = 0;
void leer_luz(void) {
    // Leemos el pin AN3 (RA3)
    ADCON0bits.CHS0    = 1;
    ADCON0bits.CHS1    = 1;
    ADCON0bits.CHS2    = 0;
    ADCON0bits.CHS3    = 0;
    // Arrancamos el CAD
    ADCON0bits.GO_DONE = 1;
    // Esperamos a que termine la conversion
    while(ADCON0bits.GO_DONE);
    // Limpiamos los valores
    voltLuz = 0;
    luz     = 0;
    // Guardamos en resultado_CAD el resultado de la conversion
    resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;

    voltLuz = resultado_CAD * 5 / 1024;
    luz     = (char) (voltLuz / 0.00038);

    //luz_imprimir = luz; // Descomentar
    // trampa
    luz_imprimir++;
}

float voltHumedad = 0;
float porcentaje  = 0;
void leer_humedad(void) {
    // Leemos la el pin AN2 (RA2)
    ADCON0bits.CHS0    = 0;
    ADCON0bits.CHS1    = 1;
    ADCON0bits.CHS2    = 0;
    ADCON0bits.CHS3    = 0;
    // Arrancamos el CAD
    ADCON0bits.GO_DONE = 1;
    // Esperamos a que la conversion termine
    while(ADCON0bits.GO_DONE);
    // Limpiamos los valores
    voltHumedad = 0;
    porcentaje  = 0;
    // Guardamos en resultado_CAD el resultado de la conversion
    resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;

    voltHumedad = resultado_CAD * 5.0 / 1024;
    porcentaje  = (char) (voltHumedad - 0.826) / 0.03125;

    //hum_imprimir = porcentaje; // Este es el codigo que mola, hay que descomentarlo
    // Trampa
    hum_imprimir++;
}

float voltExterior = 0;
char  tempExterior = 0;
void leer_tem_x(void) {
    // Leemos el pin AN1 (RA1)
    ADCON0bits.CHS0    = 1;
    ADCON0bits.CHS1    = 0;
    ADCON0bits.CHS2    = 0;
    ADCON0bits.CHS3    = 0;
    // Arrancamos el CAD
    ADCON0bits.GO_DONE = 1;
    // Esperamos a que la conversion termine
    while(ADCON0bits.GO_DONE);
    // Limpiamos los valores
    voltExterior = 0;
    tempExterior = 0;
    // Guardamos en resultado_CAD el resultado de la conversion
    resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;

    voltExterior = resultado_CAD * 5 / 1024;
    tempExterior = (char) (voltExterior - 0.5) * 100;

    //tex_imprimir = tempExterior; // Descomentar
    // Trampas
    tex_imprimir++;
}

float voltInterior = 0;
char  tempInterior = 0;
void leer_tem_i(void) {
    // Leemos del pin AN0 (RA0)
    ADCON0bits.CHS0    = 0;
    ADCON0bits.CHS1    = 0;
    ADCON0bits.CHS2    = 0;
    ADCON0bits.CHS3    = 0;
    // Arrancamos el CAD
    ADCON0bits.GO_DONE = 1;
    // Esperamos a que la conversion termine
    while(ADCON0bits.GO_DONE);
    // Limpiamos los valores
    voltInterior = 0;
    tempInterior = 0;
    // Guardamos en resultado_CAD el resultado de la conversion
    resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;

    voltInterior = resultado_CAD * 5 / 1024;
    tempInterior = (char) (voltInterior - 0.5) * 100;

    //tin_imprimir = tempInterior; // Descomentar
    // Trampas
    tin_imprimir++;
}

void leer_consigna(void) {
    // Leemos el pin AN5 (RA5))
    ADCON0bits.CHS0    = 1;
    ADCON0bits.CHS1    = 0;
    ADCON0bits.CHS2    = 1;
    ADCON0bits.CHS3    = 0;
    // Arrancamos el CAD
    ADCON0bits.GO_DONE = 1;
    // ESperamos a que la conversion termine
    while(ADCON0bits.GO_DONE);
    // Guardamos en resultado_CAD el resultado de la conversion
    resultado_CAD = 0;
    resultado_CAD = ADRESH << 8;
    resultado_CAD = resultado_CAD + ADRESL;
    
    
    if (resultado_CAD != dit_imprimir) { // lo que lee del CAD no coincide con el ultimo valor: ha cambiado
        //dit_imprimir = consigna_grados(); // Actualizamos el valor a imprimir. Hay que convertir a grados centigrados
        // Trampa
        dit_imprimir++;
        TMR0_contador_5s = 0;             // Ponemos el contador a 0 para que cuente 5s desde ahora
    }
    printf("Valor consigna: [%d] [0x%x]\n\r", dit_imprimir, dit_imprimir);
}

void evaluar_consigna(void) {
    if (tin_imprimir < dit_imprimir) { // resultado_CAD == dit_imprimir
        // Activar calefactor
        // Activar ventilador con los valores que sean
    }
    else if (tin_imprimir > dit_imprimir) {
        // Activar refrigerador
        // Activar ventilador con los valores que sean
    }
    else { // Son iguales
       // continue; // Do nothing
    }
}



void TMR0_interrupt_handler(void) {
    TMR0 = 61; // Para que mantenga el valor
    TMR0_contador_5s++;
    if (TMR0_contador_5s == 500) {
        /* Han pasado 5 segundos desde que dejaron quieto el dial de temperatura
           Mandamos las cosas al refrigerador/calefactor y ventilador */
        evaluar_consigna();
        TMR0_contador_5s = 0; // Lo ponemos a cero de nuevo
    }
}

void TMR1_interrupt_handler(void) {
    TMR1_contador_5s++;
    if (TMR1_contador_5s % 5 == 0) { // Contar 0.5s
        // printf("TMR1 ha contado 0.5s. Leemos Consigna\n\r");
        leer_consigna();
   }
    if (TMR1_contador_5s == 50) {    // Contar 5s
        // printf("TMR1 ha contado 5s. Imprimimos sensor\n\r");
        leer_luz();
        leer_humedad();
        leer_tem_x();
        leer_tem_i();
        enviar_usart();
        TMR1_contador_5s = 0;
    }
}

void interrupt general_interrupt_handler(void) {
    if (PIR1bits.ADIF) {   // Interrupt del CAD.
        PIR1bits.ADIF = 0; // Limpiamos el flag de interrupcion del CAD
    }
    else if (PIR1bits.TMR1IF) { // TMR1 interrumpe cada 0.1s
        PIR1bits.TMR1IF = 0;    // Limpiamos el flag de interrupcion de TMR1
        TMR1_interrupt_handler();
    }
    else if (INTCONbits.T0IF) { // TMR0 Interrumpe cada 0.01s
        INTCONbits.T0IF = 0;    // Limpiamos el flag de interrupcion de TMR0
        TMR0_interrupt_handler();
    }
}



void set_interrupts(void) {
    INTCONbits.GIE  = 1; // General Interrupt Enable
    INTCONbits.PEIE = 1; // PEripheral Interrupt Enable
    INTCONbits.T0IE = 1; // Allow TMR0 interrupts
    PIE1bits.TMR1IE = 1; // Allow TMR1 interrupts
    PIE1bits.ADIE   = 1; // Allow AD interrupts (CAD)

    PIE1bits.TXIE   = 0; // Don't allow USART TX interrupts
    PIE1bits.RCIE   = 0; // Don't allow USART RX interrupts
}

void init_CAD(void) {
    
    // LE decimos que frecuencia usar (01 = Fosc/8)
    ADCON0bits.ADCS0 = 1;
    ADCON0bits.ADCS1 = 0;

    // ADCON1 tambien configura el CAD
    ADCON1bits.ADFM  = 1; // Justify right
    ADCON1bits.VCFG0 = 0; // Cosas de voltajes parte 1: El comienzo de VSS
    ADCON1bits.VCFG1 = 0; // Cosas de voltajes parte 2: VDD Returns

    // Arrancar el CAD: (ADON lo enciende, GO_DONE le dice que empiece a currar)
    ADCON0bits.ADON    = 1;
    // ADCON0bits.GO_DONE = 1; // Lo hacemos en las funciones propias de leer_xxx

}

void init_TMR0(void) {
    OPTION_REG = 0b00000000;     // Inicializado a 0 para que no haya trash values
    OPTION_REGbits.T0CS = 0;     // Set the internal instruction cycle (Fosc/4) as the cycle clock
    OPTION_REGbits.PSA  = 0;     // Set the Prescaler to TMR0 instead of WDT (0=TMR0 1=WDT)
    OPTION_REGbits.PS   = 0b111; // Prescaler to 1:256 (Comes in a chart)
    TMR0                = 61;    // Counts 0.00993280 sec. Closest value to 0.01 possible
    INTCONbits.T0IF     = 0;     // Clear interrupt flag (just in case)
    // INTCONbits.T0IE     = 1;     // Allow TMR0 interrupts // MOVED TO set_interrupts(void)
}

void init_TMR1(void) {
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

    BAUDCTLbits.BRG16  = 0; // No transmitimos en 16 bits

    /* ###### SPBRGH:SPBRG ###### */
    SPBRGH = 0;
    SPBRG  = 32; // 32 para un valor de 9600 baudios

    /* ###### TXSTA ###### */
    TXSTAbits.TX9  = 0; // 8-bit transmission
    TXSTAbits.TXEN = 0; // Transmit disabled (purpose is resetting it)
    TXSTAbits.TXEN = 1; // Transmit enabled
    TXSTAbits.SYNC = 0; // Modo asincrono
    TXSTAbits.BRGH = 0; // No queremos High Baud Rate para transmitir

    /* ###### RCSTA ###### */
    RCSTAbits.SPEN = 1; // Enable serial port
    RCSTAbits.RX9  = 0; // 8-bit transmission
}

void init_portA(void) {
    TRISA = 0b00101111; // Los pines 0, 1, 2, 3 y 5 en modo input
    ANSEL = 0b00101111; // Los pines 0, 1, 2, 3 y 5 en modo analogico
}

void init_portC(void) {
    TRISC  = 0b00000000; // Todos los pines en modo output (usamos los pines 1, 2 y 4)
}


void main(void) {
    OSCCON = 0b00001000; // External crystal
    
    set_interrupts();
    init_USART();
    init_CAD();
    init_TMR1();
    init_TMR0();
    init_portA();
    init_portC();
    
    
    printf("##### BANNER ######\n\r");
    while(1);
}