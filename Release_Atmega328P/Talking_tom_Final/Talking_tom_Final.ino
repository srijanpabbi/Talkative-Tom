#define WRITE_ENABLE       0x06
#define WRITE_DISABLE      0x04
#define CHIP_ERASE         0xc7
#define READ_STATUS_REG_1  0x05
#define READ_DATA          0x03
#define PAGE_PROGRAM       0x02
#define JEDEC_ID           0x9F
#define BLOCK_ERASE        0xD8

byte buf0[256];
byte buf1[256];
boolean enable_buf0 = true;
boolean enable_buf1 = false;
boolean write_pointer = true;
boolean read_pointer = true;
unsigned int bufByteCount = 0;
unsigned int buffByteCount = 0;
boolean flag = true;
int pagenumber = 0, recpage = 0;
uint8_t val = 0;
float current_ip = 0;
float previous_ip = 0;
//float lambda = 0.0001;
//float lambda = 0.005;
//float lambda = 0.01;
float lambda = 0.017;
//boolean startrec = false;
boolean mode;

inline float ewma() {
  current_ip = lambda * val * 1.0 + (1 - lambda) * previous_ip;
  previous_ip = current_ip;
  return previous_ip;
}

unsigned char transmit(unsigned char data) {
  SPDR = data;
  while (!(SPSR & (1 << SPIF)));
  return (SPDR);
}

void chip_erase(void) {
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(WRITE_ENABLE);
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(CHIP_ERASE);
  PORTB |= 0X04;
  not_busy();
}
void block_erase(word page_number) {
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(WRITE_ENABLE);
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(BLOCK_ERASE);
  transmit((page_number >>  8) & 0xFF);
  transmit((page_number >>  0) & 0xFF);
  transmit(0);
  PORTB |= 0X04;
  not_busy();
}
void _read_page(word page_number, byte *page_buffer) {
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(READ_DATA);
  transmit((page_number >> 8) & 0xFF);
  transmit((page_number >> 0) & 0xFF);
  transmit(0);
  for (int i = 0; i < 256; ++i) {
    page_buffer[i] = transmit(0);
  }
  PORTB |= 0X04;
  not_busy();
}

void _write_page(word page_number, byte *page_buffer) {
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(WRITE_ENABLE);
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(PAGE_PROGRAM);
  transmit((page_number >>  8) & 0xFF);
  transmit((page_number >>  0) & 0xFF);
  transmit(0);
  for (int i = 0; i < 256; ++i) {
    transmit(page_buffer[i]);
  }
  PORTB |= 0X04;
  not_busy();
}

//void _erase_page(word page_number ) {
//  PORTB |= 0X04;
//  PORTB &= ~0X04;
//  transmit(WRITE_ENABLE);
//  PORTB |= 0X04;
//  PORTB &= ~0X04;
//  transmit(SECTOR_ERASE);
//  transmit((page_number >>  8) & 0xFF);
//  transmit((page_number >>  0) & 0xFF);
//  transmit(0);
//  PORTB |= 0X04;
//  not_busy();
//}
void not_busy(void) {
  PORTB |= 0X04;
  PORTB &= ~0X04;
  transmit(READ_STATUS_REG_1);
  while (transmit(0) & 1) {};
  PORTB |= 0X04;
}

void adc_set_update() {
  pagenumber = 0;
  OCR0A = 128;// set PWM for 50% duty cycle
  ADCSRA |= 1 << ADPS2;    // makes ADPS2 AS 0
  ADCSRA &= ~ 1 << ADPS1;
  ADCSRA &= ~ 1 << ADPS0;
}

void setup() {
  DDRD &= ~ (1 << DDD7);
  //  mode = mode = (PIND >> 7);

  //  if (mode) {
  //    ADCSRA |= 1 << ADPS2;    // makes ADPS2 AS 0
  //    ADCSRA &= ~1 << ADPS1;
  //    ADCSRA &= ~ 1 << ADPS0;
  //  } else {
  //    ADCSRA |= 1 << ADPS2;
  //    ADCSRA &= ~ 1 << ADPS1;
  //    ADCSRA |= 1 << ADPS0;
  //  }

  ADMUX |= 1 << ADLAR;
  ADMUX |= 1 << MUX1;
  ADMUX |= 1 << MUX0;
  ADCSRA |= 1 << ADIE;
  ADCSRA |= 1 << ADEN;
  DDRB |= 0X2C;
  DDRB |= 0x03;
  PORTB |= 0x00;
  SPCR |= 1 << MSTR ;
  SPCR |= 1 << SPE;
  DDRD = 0;
  DDRD |= (1 << DDD6);
  OCR0A = 0;
  TCCR0A |= (1 << COM0A1);
  TCCR0A |= (1 << WGM01) | (1 << WGM00);
  TCCR0B |= (1 << CS00);
  TCCR0B &= ~(1 << CS01);
  TCCR0B &= ~(1 << CS02);

  //profiling
  DDRC |= 0x01;
  PORTC |= 0x00;
  
  sei();
  ADCSRA |= 1 << ADSC;
}

void loop() {
  OCR0A = 0;
  block_erase(0);
  block_erase(256);
  //  block_erase(512);
  //  block_erase(768);
  //  block_erase(1024);

  mode = (PIND >> 7);
  //  mode = true;
  if (mode) {
    ADCSRA |= 1 << ADPS2;    // makes ADPS2 AS 0
    ADCSRA &= ~ 1 << ADPS1;
    ADCSRA &= ~ 1 << ADPS0;
  } else {
    ADCSRA |= 1 << ADPS2;
    ADCSRA &= ~ 1 << ADPS1;
    ADCSRA |= 1 << ADPS0;
  }

  OCR0A = 0;
  pagenumber = 0;
  recpage = 0;
  flag = true;
  PORTB &= ~ (0x03);

  sei();
  ADCSRA |= 1 << ADSC;
  while (ewma() < 28) {
    val = abs(ADCH - 132);
  }
  while (pagenumber < 256 && ewma() > 1.3) {//1.1,1.2,1.3,....
    PORTB |= 0x01;
    if (enable_buf0 && !write_pointer) {
      pagenumber++;
      _write_page(pagenumber, buf0);
      enable_buf0 = false;
    }
    if (enable_buf1 && write_pointer) {
      pagenumber++;
      _write_page(pagenumber, buf1);
      enable_buf1 = false;
    }
  }
  recpage = pagenumber;
  cli();

  //  if (!mode) {
  //    adc_set_update();
  //  } else {
  //    ADCSRA |= 1 << ADPS2;    // makes ADPS2 AS 0
  //    ADCSRA &= ~1 << ADPS1;
  //    ADCSRA &= ~ 1 << ADPS0;
  //  }

  adc_set_update();

  pagenumber = 0;
  flag = false;
  sei();
  ADCSRA |= 1 << ADSC;
  while (pagenumber < recpage) {
    PORTB |= 0x02;
    if (enable_buf0 && !read_pointer) {
      pagenumber++;
      _read_page(pagenumber, buf0);
      enable_buf0 = false;
    }
    if (enable_buf1 && read_pointer) {
      pagenumber++;
      _read_page(pagenumber, buf1);
      enable_buf1 = false;
    }
  }
  cli();
  val = 0;
  PORTB &= ~ (0x03);
  PORTD = 0x00;
  //  DDRD = 0xff;
  //  ADCSRA |= 1 << ADPS2;
  //  ADCSRA &= ~ 1 << ADPS1;
  //  ADCSRA |= 1 << ADPS0;

  mode = (PIND >> 7);
  if (mode) {
    ADCSRA |= 1 << ADPS2;    // makes ADPS2 AS 0
    ADCSRA &= ~1 << ADPS1;
    ADCSRA &= ~ 1 << ADPS0;
  } else {
    ADCSRA |= 1 << ADPS2;
    ADCSRA &= ~ 1 << ADPS1;
    ADCSRA |= 1 << ADPS0;
  }

  ADMUX |= 1 << ADLAR;
  ADMUX |= 1 << MUX1;
  ADMUX |= 1 << MUX0;
  ADCSRA |= 1 << ADIE;
  ADCSRA |= 1 << ADEN;
  OCR0A = 0;
}
ISR(ADC_vect) {
  //profiling
//  PORTC |= 0x01;
//  PORTC &=~(0x01);
  
  if (flag) {
    OCR0A = 0;
    bufByteCount++;
    if (bufByteCount == 256) {
      bufByteCount = 0;
      enable_buf0 = true;
      enable_buf1 = true;
      write_pointer = !write_pointer;
      val = abs( ADCH - 132);
    }
    if (enable_buf0 && write_pointer) {
      buf0[bufByteCount] = ADCH;
    }
    if (enable_buf1 && !write_pointer) {
      buf1[bufByteCount] = ADCH;
    }

    ADCSRA |= 1 << ADSC;
    return;
  }

  if (!flag && mode) {
    ADCSRA |= 1 << ADSC;
    buffByteCount++;
    if (buffByteCount == 256) {
      buffByteCount = 0;
      enable_buf0 = true;
      enable_buf1 = true;
      read_pointer = !read_pointer;
    }
    if (enable_buf0 && read_pointer) {
      OCR0A = buf0[buffByteCount];
    }
    if (enable_buf1 && !read_pointer) {
      OCR0A = buf1[buffByteCount];
    }

    //    ADCSRA |= 1 << ADSC;u
    return;
  }

  if (!flag && !mode) {
    buffByteCount++;
    if (buffByteCount == 256) {
      buffByteCount = 0;
      enable_buf0 = true;
      enable_buf1 = true;
      read_pointer = !read_pointer;
    }
    if (enable_buf0 && read_pointer) {
      OCR0A = buf0[buffByteCount];
      delayMicroseconds(10);
    }
    if (enable_buf1 && !read_pointer) {
      OCR0A = buf1[buffByteCount];
      delayMicroseconds(10);
    }

    ADCSRA |= 1 << ADSC;
    return;
  }
}

