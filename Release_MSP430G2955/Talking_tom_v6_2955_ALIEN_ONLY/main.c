//FINAL 1 ONLY PITCH CHANGE

#include <msp430.h>
#include<stdbool.h>
#include<inttypes.h>

#define WRITE_ENABLE       0x06
#define WRITE_DISABLE      0x04
#define CHIP_ERASE         0xc7
#define READ_STATUS_REG_1  0x05
#define READ_DATA          0x03
#define PAGE_PROGRAM       0x02
#define JEDEC_ID           0x9F
#define BLOCK_ERASE        0xD8

#define abs(x) ((x>0)?(x):(-x))
//volatile int v1;
volatile unsigned char buf0[256];
volatile unsigned char buf1[256];
volatile unsigned int i = 0;
_Bool enable_buf0 = 1;
_Bool enable_buf1 = 0;
_Bool write_pointer = 1;
_Bool read_pointer = 1;
_Bool flag = 1;
//unsigned int *enable_buf0 = (unsigned int*)0x0800;
//unsigned int *enable_buf1 = (unsigned int*)0x0801;
//unsigned int *write_pointer = (unsigned int*)0x0802;
//unsigned int *read_pointer = (unsigned int*)0x0803;
//unsigned int *flag = (unsigned int*)0x0804;
//*enable_buf0 = 0x0001;
//*enable_buf1 = 0x0000;
//*write_pointer = 0x0001;
//*read_pointer = 0x0001;
//*flag = 1;

volatile unsigned int bufByteCount = 0;
volatile unsigned int buffByteCount = 0;
volatile unsigned long pagenumber = 0, recpage = 0;

//When the USCI module is enabled by clearing the UCSWRST bit it is ready to receive and transmit.
//A transmit or receive operation is indicated by UCBUSY = 1
//UCMODEx 10
//In master mode, writing to UCxTXBUF activates the bit clock generator and the data will begin to transmit
//The UCA0TXIFG interrupt flag is set by the transmitter to indicate that UCxTXBUF is ready to accept another character.
//The UCA0RXIFG interrupt flag is set each time a character is received and loaded into UCxRXBUF.
//Interrupts are handled by setting UCxRXIE and GIE
//UCB0TXIFG is set when UCB0TXBUF is empty and transmission is complete
//UCB0RXIFG is set when UCB0RXBUF is empty and recieving  is complete
//Default UCCKPH, UCCKPL, UC7BIT
//To Set in UCAxCTL0 : UCMSB(1), UCMST(1), UCMODEx(10), UCSYNC(1)
//TO Set in UCAxCTL1 : UCSSELx(10 or 11), UCSWRST(0)

uint8_t val = 0;
uint8_t temp = 0;
float current_ip = 0;
float previous_ip = 0;
float lambda = 0.017;
_Bool mode;

inline float ewma()
{
    current_ip = lambda * val * 1.0 + (1 - lambda) * previous_ip;
    previous_ip = current_ip;
    return previous_ip;
}

unsigned char transmit(unsigned char data)
{
    while (!(IFG2 & UCA0TXIFG))
        ;
    UCA0TXBUF = data;
    while (!(IFG2 & UCA0TXIFG))
        ;
    while (!(IFG2 & UCA0RXIFG))
        ; // As soon as the transmit is complete the SPIF bit is set in the SPSR register of the spi module. The while condition is turned to 0 as soon as this happens
    return (UCA0RXBUF);
}
void unbusy(void)
{
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(READ_STATUS_REG_1);
    while (transmit(0x00) != 0x00)
    {
    };
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
}

void block_erase(unsigned long page_number)
{
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(WRITE_ENABLE);
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(BLOCK_ERASE);
    transmit((page_number >> 8) & 0xFF);
    transmit((page_number >> 0) & 0xFF);
    transmit(0);
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
    unbusy();
}
void _read_page(unsigned long page_number, volatile unsigned char page_buffer[])
{
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(READ_DATA);
    transmit((page_number >> 8) & 0xFF);
    transmit((page_number >> 0) & 0xFF);
    transmit(0);
    i = 0;
    for (i = 0; i < 256; ++i)
    {
        page_buffer[i] = transmit(0);
    }
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
    unbusy();
}

void _write_page(unsigned long page_number,
                 volatile unsigned char page_buffer[])
{
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(WRITE_ENABLE);
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
    P3OUT &= ~BIT3;
    transmit(PAGE_PROGRAM);
    transmit((page_number >> 8) & 0xFF);
    transmit((page_number >> 0) & 0xFF);
    transmit(0);
    i = 0;
    for (i = 0; i < 256; ++i)
    {
        transmit(page_buffer[i]);
    }
    while (!(IFG2 & UCA0TXIFG))
        ;
    P3OUT |= BIT3;
    unbusy();
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    BCSCTL1 = CALBC1_8MHZ;              // Set DCO to 8MHz
    DCOCTL = CALDCO_8MHZ;

//    BCSCTL1 |= BIT3 + BIT2 + BIT1 + BIT0;
//    DCOCTL |= BIT7 + BIT6 + BIT5;

//    P2DIR |= BIT1;                         // P2.1-> Output
//    P2SEL |= BIT1;                         // P2.1 -> Select Timer Output
    P1DIR |= BIT2;        //P1.2 as TA0.1
    P1SEL |= BIT2;
    TACCR0 = 255;                             // Set Timer0 PWM Period
    TACCTL1 = OUTMOD_7; // Set TA0.1 Waveform Mode - Clear on Compare, Set on Overflow
    TACCR1 = 0;                               // Set TA0.1 PWM duty cycle
    TACTL = TASSEL_2 + MC_1;           // Timer Clock -> SMCLK, Mode -> Up Count

    P3DIR |= BIT3;                                     // Set 3.3 as output(CE)
    P3OUT |= BIT3;
    P2SEL &=~(BIT6+BIT7);
     P2DIR |=BIT6+BIT7;
//     P2OUT &= ~(BIT6 + BIT7);
//    __delay_cycles(10000);
P2OUT |= BIT6+BIT7;
//        __delay_cycles(10000);
P2OUT &=~(BIT6+BIT7);
    P3SEL |= BIT0 + BIT4 + BIT5;      //P3.0 -> CLK , P3.4 -> SIMO, P3.5 -> SOMI

    UCA0CTL1 |= UCSWRST;                            // Hold USCI in reset state
    UCA0CTL0 |= UCCKPH + UCMST + UCSYNC + UCMSB; // 3-pin, 8-bit, SPI Master
    UCA0CTL1 |= UCSSEL_2;                           // Clock -> SMCLK
    UCA0BR0 = 0x02;                                 // SPI CLK -> SMCLK/2
    UCA0BR1 = 0;
    UCA0MCTL = 0;
    UCA0CTL1 &= ~UCSWRST;

    ADC10CTL0 &= ~ENC; //disable conversion so that we could change the values of the index bits locked
    ADC10CTL1 |= INCH_0 + ADC10DIV_7 + ADC10SSEL_3 + CONSEQ_0;
    ADC10AE0 |= BIT0;
    ADC10CTL0 |= ADC10SHT_3 + ADC10ON + ADC10IE;
//    __enable_interrupt();
//    ADC10CTL0 |= ENC + ADC10SC;

    while (1)
    {
        P2OUT &= ~(BIT6 + BIT7);
        i = 0;
        for (i = 0; i < 1000; i = i + 256)
        {
            block_erase(i);
        }
//        block_erase(256);
//        block_erase(512);
//        block_erase(768);
//        block_erase(1024);
//        block_erase(1024);
//        block_erase(1024);
        BCSCTL1 = CALBC1_8MHZ;              // Set DCO to 16MHz
        DCOCTL = CALDCO_8MHZ;
//        ADC10CTL0 &   = ~ENC; //disable conversion so that we could change the values of the index bits locked
//        ADC10CTL1 |= INCH_0 + ADC10DIV_7 + ADC10SSEL_3 + CONSEQ_0;
//        ADC10AE0 |= BIT0;
//        ADC10CTL0 |= ADC10SHT_3 + ADC10ON + ADC10IE;
        pagenumber = 0;
        flag = 1;
//        v1 = abs(-120);
        __enable_interrupt();
        ADC10CTL0 |= ENC + ADC10SC;
        val = 0;
        while (ewma() < 15)
        {
            temp = ADC10MEM >> 2;
            val = abs(temp - 128);
        }
        while (pagenumber < 500 && ewma() > 3)
        {
            P2OUT |= BIT7;
            if ((enable_buf0 == 1) && (write_pointer == 0))
            {
                //put buf0 in spi
                _write_page(pagenumber, buf0);
                enable_buf0 = 0;
                pagenumber += 1;
            }
            if ((enable_buf1 == 1) && (write_pointer == 1))
            {
                //put buf1 in spi
                _write_page(pagenumber, buf1);
                enable_buf1 = 0;
                pagenumber += 1;
            }
        }
        __disable_interrupt();
        recpage = pagenumber;
        pagenumber = 0;
        flag = 0;
        BCSCTL1 = CALBC1_12MHZ;              // Set DCO to 16MHz
        DCOCTL = CALDCO_12MHZ;
//        ADC10CTL0 &= ~ENC; //disable conversion so that we could change the values of the index bits locked
//        ADC10CTL1 |= INCH_0 + ADC10DIV_7 + ADC10SSEL_3 + CONSEQ_0;
//        ADC10AE0 |= BIT0;
//        ADC10CTL0 |= ADC10SHT_3 + ADC10ON + ADC10IE;
        __enable_interrupt();
        ADC10CTL0 |= ENC + ADC10SC;

        while (pagenumber < recpage)
        {   P2OUT |= BIT6;
            if ((enable_buf0 == 1) && (read_pointer == 0))
            {
                //fill buf0 from spi
                _read_page(pagenumber, buf0);
                enable_buf0 = 0;
                pagenumber += 1;
            }
            if ((enable_buf1 == 1) && (read_pointer == 1))
            {
                //fill buf1 from spi
                _read_page(pagenumber, buf1);
                enable_buf1 = 0;
                pagenumber += 1;
            }
        }
        __disable_interrupt();
        TACCR1 = 0;
        val = 0;
//        while(1);
    }
    return 0;
}

#pragma vector=ADC10_VECTOR
__interrupt
void ADC10_ISR(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
    ADC10CTL0 &= ~ADC10IFG;
    if (flag == 1)
    {
        bufByteCount += 1;
        if (bufByteCount == 256)
        {
            bufByteCount = 0;
            enable_buf0 = 1;
            enable_buf1 = 1;
            write_pointer = write_pointer ^ 0x01;
            temp = ADC10MEM >> 2;
            val = abs(temp - 128);
        }
        if ((enable_buf0 == 1) && (write_pointer == 1))
        {
            buf0[bufByteCount] = ADC10MEM >> 2;
//            __delay_cycles(2500);
        }
        if ((enable_buf1 == 1) && (write_pointer == 0))
        {
            buf1[bufByteCount] = ADC10MEM >> 2;
//            __delay_cycles(2500);
        }
    }
    if (flag == 0)
    {
        buffByteCount += 1;
        if (buffByteCount == 256)
        {
            buffByteCount = 0;
            enable_buf0 = 1;
            enable_buf1 = 1;
            read_pointer = read_pointer ^ 0x01;
        }
        if ((enable_buf0 == 1) && (read_pointer == 1))
        {
            TACCR1 = buf0[buffByteCount];
            __delay_cycles(300);
        }
        if ((enable_buf1 == 1) && (read_pointer == 0))
        {
            TACCR1 = buf1[buffByteCount];
            __delay_cycles(300);
        }
    }
//    ADC10CTL0 |= ENC + ADC10SC;
//       ADC10CTL0 &= ~ADC10IFG;
}

