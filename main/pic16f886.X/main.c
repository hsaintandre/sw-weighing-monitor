/*
 * File:   main.c
 * Author: hsaintandre
 *
 * Created on 23 de febrero de 2018, 12:17
 */


#include <xc.h>
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int accuValor = 0;
unsigned char j = 0, i = 0;
unsigned char res = 1, outC = 0, outD = 0, outU = 0;
unsigned char resC = 0, resD = 0, resU = 0;
unsigned char digit = 0;
static bit swiza, btnUp, btnDown, reset;
unsigned int garron = 0, tempo = 0;
unsigned char datos[28] = {};
unsigned int mainAux = 0;
unsigned char btnUpT = 0, btnDownT = 0;
unsigned char tick = 0;
unsigned int timerReset = 0;

const unsigned char swBot = 135, swTop = 191;

void uart_init(){
    PIE1bits.RCIE = 1;  //enables EUSART reception interrupt
    PIE1bits.TXIE = 0;  //disables uart transmision interrupt
    TXSTAbits.BRGH = 1;
    BAUDCTLbits.BRG16 = 0;
    //SPBRGH = 0x1A;
    //SPBRG = 0x0A;       //300bps @ Fosc =8MHz
    SPBRGH = 0x00;
    SPBRG = 0x33;     //9600bps @ Fosc=8MHz
    //SPBRG = 0x0C;       //9600bps @ Fosc=2MHz
    TXSTAbits.SYNC = 0;
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.CREN = 1; //enables receiver
}

void putch(char data){      //Mandar esto a otra librer?a
    while(!PIR1bits.TXIF)
        continue;
    TXREG = data;
}

void init_timer2(unsigned char pre, unsigned char post, unsigned char eoc){
    unsigned char sfr = 0x00;
    PIR1bits.TMR2IF = 0;
    PR2 = eoc;  //sets the end of count
    sfr = ((post-1) << 3) & 0x78;   //places postscaler's bits in their position and applies a mask
    sfr += 4;   //enables timer2 (T2CONbits.TMR2ON = 1;)
    sfr += pre; //places prescaler's bits (00|01|10)
    T2CON = sfr;   
    PIE1bits.TMR2IE = 1;    //enables tmr2 interrupt
}

unsigned char rw_eeprom(unsigned char addr, unsigned char data, unsigned char rw){
    
    if (rw) {   //write = 1
        EEADR = addr;
        EEDAT = data;  //writes data's value on the eeprom data reg
        EECON1bits.EEPGD = 0;   //data memory access
        EECON1bits.WREN = 1;
        EECON2 = 0x55;  //?? mandatory
        EECON2 = 0xAA;  //?? mandatory
        EECON1bits.WR = 1;  //initiates write operation 
        while (!PIR2bits.EEIF); //waits for the end of the write operation
        PIR2bits.EEIF = 0;
        return 0;
    }
    else {      //read = 0
        EEADR = addr;   //sets the address
        EECON1bits.EEPGD = 0;   //data memory access
        EECON1bits.RD = 1;  //initiates read operation
        return EEDAT;  //returns read
    }
}

void display() {
    switch (digit) {
        case 1:
            PORTCbits.RC3 = 0;  //mosfet p-chan
            PORTCbits.RC4 = 1;  //mosfet p-chan
            PORTCbits.RC5 = 1;  //mosfet p-chan
            PORTB = outC;
            if (resC) {
                if (PORTAbits.RA6) {    //takes into account the RA6 bit
                    PORTA = resC | 0xC0;       //otherwise it's erased on PORTA writing
                }
                else {
                    PORTA = resC | 0x80;
                }
            }
            else {
                PORTAbits.RA7 = 0;
            }
            break;
        case 2:
            PORTCbits.RC3 = 1;  //mosfet p-chan
            PORTCbits.RC4 = 0;  //mosfet p-chan
            PORTCbits.RC5 = 1;  //mosfet p-chan
            PORTB = outD;
            if (resD || resC) {
                if (PORTAbits.RA6) {    //takes into account the RA6 bit
                    PORTA = resD | 0xC0;       //otherwise it's erased on PORTA writing
                }
                else {
                    PORTA = resD | 0x80;
                }
            }
            else {   //only turns off when garron < 100
                PORTAbits.RA7 = 0;
            }
            break;
        case 3:
            PORTCbits.RC3 = 1;  //mosfet p-chan
            PORTCbits.RC4 = 1;  //mosfet p-chan
            PORTCbits.RC5 = 0;  //mosfet p-chan
            PORTB = outU;
            if (PORTAbits.RA6) {    //takes into account the RA6 bit
                PORTA = resU | 0xC0;       //otherwise it's erased on PORTA writing
            }
            else {
                PORTA = resU | 0x80;    
            }
            break;
    }
}

unsigned int cattle_nr(char opt, unsigned int cattle) {
    unsigned int rta = 0;
    unsigned char dato = 0;
    switch (opt) {
        case 0: //returns the number
            dato = rw_eeprom(0x00, 0, 0); //reads 1st position (initialization)
            if (dato == 0x10) {
                dato = rw_eeprom(0x01, 0, 0); //BIG ENDIAN!
                rta = dato; //upper byte
                rta = rta << 8;
                dato = rw_eeprom(0x02, 0, 0);
                rta += dato; //lower byte
            } else { //initializes memory
                rw_eeprom(0x00, 0x10, 1);
                rw_eeprom(0x01, 0x00, 1);
                rw_eeprom(0x02, 0x00, 1);
                rta = 0;
            }
            break;
        case 1: //sets the number
            dato = cattle & 0x00FF;
            rw_eeprom(0x02, dato, 1);
            //printf("lower = %d\n",rw_eeprom(0x02, 0, 0));
            cattle = cattle >> 8;
            dato = cattle & 0x00FF;
            rw_eeprom(0x01, dato, 1);
            rta = 0x10;
            break;
    }
    return rta;
}

void interrupt ints_isr(void){
    
/******************************************************************************/    
    if (PIR1bits.RCIF) {
        
        unsigned char aux = 0, nral1 = 0, nral2 = 0;
        unsigned int valor = 0, upper = 0, lower = 0;
        unsigned char garron_1c = 0, garron_1d = 0, garron_1u = 0; //centena, decena, unidad
        unsigned char garron_2c = 0, garron_2d = 0, garron_2u = 0; //centena, decena, unidad
        static bit process;
        unsigned char drop;

        /*do { //this loop reads all data
            while (!PIR1bits.RCIF); //waits for the incoming byte
            if (!RCSTAbits.FERR & !RCSTAbits.OERR) {
                datos[i] = RCREG; //reads char
                i++;
            } else {
                RCSTAbits.FERR = 0;
                RCSTAbits.OERR = 0;
            }
        } while (datos[i - 1] != '\n'); //until 18 chars arrive
*/
        while (!PIR1bits.RCIF); //waits for the incoming byte
        if (!RCSTAbits.FERR & !RCSTAbits.OERR) {
            datos[i] = RCREG; //reads char
            i++;
   
            //if (i > 17) { //if 18 bytes were received
            //if (datos[i-1] == '\r') { //METLER-TOLEDO
            if (datos[i-1] == '\r' | datos[i-1] == '\n') { //TORRES | METLER-TOLEDO
                process = 0;
                if (datos[0] == 0x02) {     //if the string was caught at the beginnig, continues
                    if (i > 20) {   //METLER-TOLEDO
                        for (unsigned char k = 0; k < i; k++) {	//finds 1st '#' occurrence
                            if (datos[k] == 0x23) {
                                nral1 = k;
                                break;
                            }
                        }
                        for (unsigned char k = (nral1 + 1); k < i; k++) {	//finds 2nd '#' occurrence
                            if (datos[k] == 0x23) {
                                nral2 = k;
                                break;
                            }
                        }
                        if (datos[nral1 + 1] != '-') {
                            switch (nral2 - nral1) {
                                case 3:
                                    valor = (datos[nral1 + 1] - 48) * 100 + (datos[nral1 + 2] - 48) * 10;   //worst compatibility method ever
                                    break;
                                case 4:
                                    valor = (datos[nral1 + 1] - 48) * 1000 + (datos[nral1 + 2] - 48) * 100 + (datos[nral1 + 3] - 48) * 10;  //worst compatibility method ever
                                    break;
                            }
                            process = 1;
                        }
                        i = 0;
                    }

                    if (datos[i-1] == '\n') {   //TORRES
                        if (datos[6] == '0' && datos[7] == '0') {
                            valor = 0;
                        } else {
                            valor = (datos[6] - 48) * 1000 + (datos[7] - 48) * 100 + (datos[8] - 48) * 10 + (datos[9] - 48);
                            //it could be necessary to define datos as unsigned int instead of char 
                        }
                        i = 0;
                        process = 1;
                    }
                } else {    //otherwise it starts again
                    i = 0;
                }
                
                if (datos[0] == '\t') { //configurations
                    switch (datos[1]){
                        case 'R':
                            tempo = 0;
                            garron = 0;
                            res = 1;
                            do { //saves cattle number
                                aux = cattle_nr(1, garron);
                            } while (aux != 0x10);
                            outC = 0xF0; //Resets all
                            outD = 0xF0;
                            outU = 0xF0;
                            resC = 0x00;
                            resD = 0x00;
                            resU = 0x00;
                            break;
                        case 'G':
                            tempo = 0;
                            timerReset = 0; //2hs timer for reseting values
                            garron = (datos[2] - 48) * 100 + (datos[3] - 48) * 10 + (datos[4] - 48);
                            garron = garron << 1;
                            res = 1;
                            do { //saves cattle number
                                aux = cattle_nr(1, garron);
                            } while (aux != 0x10);
                            lower = garron >> 1;
                            outC = 0;
                            outD = 0;
                            outU = 0;
                            resC = 0;
                            resD = 0;
                            resU = 0;
                            while (lower > 99) {
                                resC++; //hundreds
                                lower -= 100; //at the end, it stores the tens
                            }
                            while (lower > 9) {
                                resD++; //tens
                                lower -= 10; //at the end, it stores the units
                            }
                            resU = lower; //units
                            break;
                    }
                }    

                if (process) { 
                    
                    if (valor > 90 && j < 20) { //accumulates the weight and the averaging factor
                        accuValor += valor;
                        j++; //it must be limited to avoid overflow
                    }                //when the weight is under 9, it's time to calculate the weight value
                    else if (accuValor > 0 && valor < 90) { //this prevents the system from sending the zero value
                        //divides by 10 (decimal) and 'j' (average), taking into accout the reminder
                        valor = accuValor / (j * 10); //truncated value
                        upper = (valor + 1) * j * 10; //ceil rounded
                        lower = valor * j * 10;
                        //accuValor stores the original
                        //then, it's computed the difference between possible results, the lower
                        //matchs with the correct result
                        if ((upper - accuValor) < (accuValor - lower)) { //computes the difference
                            valor++;
                        }

                        printf("%03d\n", valor); //fixed length of 3 chars, zero completion
                        //printf("%d\t%d\t%d\n", lower,accuValor,upper);

                        resC = 0; //resets all variables
                        resD = 0;
                        resU = 0;
                        timerReset = 0; //2hs timer for reseting values
                        
                        if (res) {
                            res = 0;
                            garron++; //increases garron count by one                    
                            if (valor >= swBot & valor <= swTop) { //checks SW compliance
                                PORTCbits.RC0 = 0; //ok
                                PORTCbits.RC1 = 1; //wait
                                PORTCbits.RC2 = 0; //discard
                            } else {
                                PORTCbits.RC0 = 0; //ok
                                PORTCbits.RC1 = 0; //wait
                                PORTCbits.RC2 = 1; //discard
                            }

                            lower = valor; //reuse of 'lower' variable to set display numbers
                            while (lower > 99) {
                                garron_1c++; //hundreds
                                lower -= 100; //at the end, it stores the tens
                            }
                            while (lower > 9) {
                                garron_1d++; //tens
                                lower -= 10; //at the end, it stores the units
                            }
                            garron_1u = lower; //units

                            lower = (garron + 1) >> 1; //reuse of 'lower' variable to set display numbers
                            while (lower > 999) {
                                lower -= 1000;
                            }
                            while (lower > 99) {
                                resC++; //hundreds
                                lower -= 100; //at the end, it stores the tens
                            }
                            while (lower > 9) {
                                resD++; //tens
                                lower -= 10; //at the end, it stores the units
                            }
                            resU = lower; //units

                            outC = garron_1c & 0x0F; //sets the lower nibble of the hundreds
                            outD = garron_1d & 0x0F; //sets the lower nibble of the tens
                            outU = garron_1u & 0x0F; //sets the lower nibble of the units
                            PORTAbits.RA6 = 0; //turns off 2nd display
                        } else {
                            res = 1;
                            tempo = 0;
                            garron++;
                            if (PORTCbits.RC1) { //means that 1st one was OK
                                if (valor >= swBot & valor <= swTop) { //checks SW compliance
                                    PORTCbits.RC0 = 1; //ok
                                    PORTCbits.RC1 = 0; //wait
                                    PORTCbits.RC2 = 0; //discard
                                } else {
                                    PORTCbits.RC0 = 0; //ok
                                    PORTCbits.RC1 = 0; //wait
                                    PORTCbits.RC2 = 1; //discard
                                }
                            }
                            lower = valor; //reuse of 'lower' variable to set display numbers
                            while (lower > 99) {
                                garron_2c++; //hundreds
                                lower -= 100; //at the end, it stores the tens
                            }
                            while (lower > 9) {
                                garron_2d++; //tens
                                lower -= 10; //at the end, it stores the units
                            }
                            garron_2u = lower; //units

                            outC = outC & 0x0F; //avoids accumulation when a shift is present
                            outD = outD & 0x0F; //and the res = 0 statement is not executed
                            outU = outU & 0x0F; //and variables are not reset

                            outC += (garron_2c << 4) & 0xF0; //sets the upper nibble of the hundreds
                            outD += (garron_2d << 4) & 0xF0; //sets the upper nibble of the tens
                            outU += (garron_2u << 4) & 0xF0; //sets the upper nibble of the units

                            lower = garron >> 1; //reuse of 'lower' variable to set display numbers
                            while (lower > 999) {
                                lower -= 1000;
                            }
                            while (lower > 99) {
                                resC++; //hundreds
                                lower -= 100; //at the end, it stores the tens
                            }
                            while (lower > 9) {
                                resD++; //tens
                                lower -= 10; //at the end, it stores the units
                            }
                            resU = lower; //units

                            PORTAbits.RA6 = 1; //turns on 2nd display
                        }

                        do { //saves cattle number
                            aux = cattle_nr(1, garron);
                        } while (aux != 0x10);

                        accuValor = 0; //resets global variables
                        j = 0;
                    }
                }
            }
        } else {
        drop = RCREG;   //reads a char to free up space in the FIFO
        drop = RCREG;   //reads a char to free up space in the FIFO
        RCSTAbits.CREN = 0; //clears OERR
        __delay_us(100);
        RCSTAbits.CREN = 1; //Re-enables receiver
        }
    }
    
    if (PIR1bits.TMR2IF) {
        unsigned int lower = 0, aux = 0;
        

        PIR1bits.TMR2IF = 0; //clears int flag bit
        
        digit++;
        if (digit > 3) {
            digit = 1;
        }
        
        tick++;
        if (tick > 199) {   //this condition es true each second
            tick = 0;
            timerReset++;
            if (timerReset > 7199 & timerReset < 7300) {   //number of seconds to trigger a reset
                timerReset = 7301;    //this avoids the reset each 2hs
                garron = 0; //reset
                do { //saves cattle number
                    aux = cattle_nr(1, garron);
                } while (aux != 0x10);
                res = 1;
                resC = 0; //resets indexer
                resD = 0;
                resU = 0;
                outC = 0xF0;    //2nd display is turned off, 1st shows 000
                outD = 0xF0;
                outU = 0xF0;
                tempo = 0;
            }  
        }
        
        if (res & tempo < 1200) { //if 2nd res has passed
            tempo++; //increases timer
        }
        if (tempo > 1199) { //6 secs timeout
            tempo = 0;
            outC = 0xFF; //turns off displays and notification led
            outD = 0xFF;
            outU = 0xFF;
            PORTCbits.RC0 = 0; //ok
            PORTCbits.RC1 = 0; //wait
            PORTCbits.RC2 = 0; //discard
        }
        display();
        if (!PORTAbits.RA4) { //normally open
            if (btnUpT < 255) {
                btnUpT++;
            }
            if (btnUpT > 25) {
                btnUp = 1; //when pressed sets btnUp
            }
        } else if (btnUp && !PIR1bits.RCIF) { //when released (RA4=0;btnUp=1)
            tempo = 0;
            btnUpT = 0;
            if (res) {
                res = 0;
                outC = (outC >> 4) & 0x0F;
                outD = (outD >> 4) & 0x0F;
                outU = (outU >> 4) & 0x0F;
            } else {
                res = 1;
                PORTAbits.RA6 = 1; //turns on 2nd display
                outC = (outC << 4) & 0xF0;
                outD = (outD << 4) & 0xF0;
                outU = (outU << 4) & 0xF0;
            }

            btnUp = 0;
            garron++; //increments garron in 1
            do { //saves cattle number
                aux = cattle_nr(1, garron);
            } while (aux != 0x10);
            if (!PORTAbits.RA5) { //normally open
                garron = 0; //full reset
                do { //saves cattle number
                    aux = cattle_nr(1, garron);
                } while (aux != 0x10);
                garron = 1; //cause when releasing btnDown it will decrease by one
                reset = 1;
            }
            if (res) { //odd  
                lower = (garron + 1) >> 1; //reuse of 'lower' variable to set display numbers
            } else { //even
                lower = garron >> 1; //reuse of 'lower' variable to set display numbers
            }
            resC = 0; //resets all variables
            resD = 0;
            resU = 0;
            while (lower > 999) {
                lower -= 1000;
            }
            while (lower > 99) {
                resC++; //hundreds
                lower -= 100; //at the end, it stores the tens
            }
            while (lower > 9) {
                resD++; //tens
                lower -= 10; //at the end, it stores the units
            }
            resU = lower; //units
           
            
        } else {
            btnUpT = 0;
        }
        if (!PORTAbits.RA5) { //normally open
            if (btnDownT < 255) { 
                btnDownT++;
            }
            if (btnDownT > 25) {
                btnDown = 1; //when pressed sets btnDown
            }
        } else if (btnDown && !PIR1bits.RCIF) { //when released (RA5=1;btnUp=1)
            tempo = 0;
            btnDown = 0;
            if (res) {
                res = 0;
                outC = (outC >> 4) & 0x0F;
                outD = (outD >> 4) & 0x0F;
                outU = (outU >> 4) & 0x0F;
            } else {
                res = 1;
                PORTAbits.RA6 = 1; //turns on 2nd display
                outC = (outC << 4) & 0xF0;
                outD = (outD << 4) & 0xF0;
                outU = (outU << 4) & 0xF0;
            }
            if (reset) {
                res = 1;
                reset = 0; 
            }
            btnDown = 0;
            garron--; //decreases garron by 1
            do { //saves cattle number
                aux = cattle_nr(1, garron);
            } while (aux != 0x10);
            if (res) { //odd
                lower = (garron + 1) >> 1; //reuse of 'lower' variable to set display numbers
            } else { //even
                lower = garron >> 1; //reuse of 'lower' variable to set display numbers
            }
            resC = 0; //resets all variables from indexer
            resD = 0;
            resU = 0;
            while (lower > 999) {
                lower -= 1000;
            }
            while (lower > 99) {
                resC++; //hundreds
                lower -= 100; //at the end, it stores the tens
            }
            while (lower > 9) {
                resD++; //tens
                lower -= 10; //at the end, it stores the units
            }
            resU = lower; //units
            
            
            
        } else {
            btnDownT = 0;
        }
    }
}

void main(void) {
    OSCCON = 0b01110001;    //internal osc @8MHz
    INTCON = 0xC0;  //enables global and peripheral interrupts
    ANSELH = 0x00;
    ANSEL = 0x00;
    TRISA = 0x30;   //2 inputs for buttons
    TRISB = 0x00;
    TRISC = 0x00;
    btnUp = 0;
    btnDown = 0;
    garron = cattle_nr(0,0x00); //gets garron number

    if (garron % 2) {   //when garron is odd
        res = 0;
        mainAux = (garron + 1) >> 1; //cause is odd
        while (mainAux > 999) {
            mainAux -= 1000;
        }
        while (mainAux > 99) {
            resC++; //hundreds
            mainAux -= 100; //at the end, it stores the tens
        }
        while (mainAux > 9) {
            resD++; //tens
            mainAux -= 10; //at the end, it stores the units
        }
        resU = mainAux; //units
        outC = 0x0F;
        outD = 0x0F;
        outU = 0x0F;
        PORTAbits.RA6 = 1;  //2nd display starts turned on
        PORTCbits.RC0 = 0;  //ok
        PORTCbits.RC1 = 0;  //wait
        PORTCbits.RC2 = 1;  //discard
    } else {    //when garron is even
        res = 1;
        PORTAbits.RA6 = 0;  //2nd display starts turned off
        mainAux = garron >> 1;   //cause is even
        while (mainAux > 999) {
            mainAux -= 1000;
        }
        while (mainAux > 99) {
            resC++; //hundreds
            mainAux -= 100; //at the end, it stores the tens
        }
        while (mainAux > 9) {
            resD++; //tens
            mainAux -= 10; //at the end, it stores the units
        }
        resU = mainAux; //units
        PORTCbits.RC0 = 0;  //ok
        PORTCbits.RC1 = 1;  //wait
        PORTCbits.RC2 = 0;  //discard
    }
    reset = 0;
    init_timer2(2, 5, 125); // 2e6/16/5/125 = 200 Hz (timeout = 5ms) this is Tmax
    uart_init();
    while(1){}
}

/*for (unsigned char k = 0; k < i; k++) {	//finds 1st '#' occurrence
                if (datos[k] == 0x23) {
                    nral1 = k;
                    break;
                }
            }
            for (unsigned char k = (nral1 + 1); k < i; k++) {	//finds 2nd '#' occurrence
                if (datos[k] == 0x23) {
                    nral2 = k;
                    break;
                }
            }*/

//memcpy(peso, &datos[6], 4);  //copies the 4 bytes-long weight measure
//valor = atoi(peso); //converts ascii to int