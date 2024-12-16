/**********************************************************************
 *                        Arduino UNO R4 Minima
 *             Renesas RA4M1 (Arm Cortex-M4), R7FA4M1AB3CFM
 *                       MCU: R7FA4M1AB3CFM (LQFP64)
 *
 *********************************************************************/
 #include <EEPROM.h>
static const int dly_msec = 500;
static const IRQn_Type IRQn_OVF7 = IRQn_Type(16);    // Valid range is [0,31] but don't use low numbers
static const IRQn_Type IRQn_PCHGD2 = IRQn_Type(17);  // Valid range is [0,31] but don't use low numbers
static volatile int t7_cnt = 0;
static int cnt = 0;
volatile int display_speed = 0;
int count_bytes= 0;
char incomingByte = 0;
byte eeprom_byte;
volatile int counttime = 0;
#define ACK 0x01
void displaySpeed()
{
  count_bytes = count_bytes * 8;
  Serial.println(count_bytes);
  count_bytes = 0;
}


//*********************************************************************
void gpt7_ovf_isr() {
  // Clear interrupt flag
  R_ICU->IELSR_b[IRQn_OVF7].IR = 0;
  // Increment GPT7 counter
  t7_cnt++;
  counttime++;
  if(counttime == 1000)
  {
    digitalWrite(LED_BUILTIN, LOW); 
    display_speed = 1;
  }
  else if(counttime == 2000)
  {
      digitalWrite(LED_BUILTIN, HIGH);
      counttime = 0;
      display_speed = 1;
  }


}

//*********************************************************************
void pin_change_isr() {
  // Clear interrupt flag
  R_ICU->IELSR_b[IRQn_PCHGD2].IR = 0;
  // Stop GPT7
  R_GPT7->GTCR_b.CST = 0;
  // On-board LED off
  digitalWrite(LED_BUILTIN, LOW);

  // Clear GPT7 counter
  t7_cnt = 0;
}

//*********************************************************************
void setup_pin_change_interrupt() {

  // Pin change interrupt on pin D3/P104/IRQ1
  // See chapter 1.7 Pin Lists for LQFP64

  // Setup I/O pin P104
  //R_PFS->PORT[1].PIN[4].PmnPFS_b.PODR = 0b0;   // Low
  //R_PFS->PORT[1].PIN[4].PmnPFS_b.PDR = 0b0;    // Input
  //R_PFS->PORT[1].PIN[4].PmnPFS_b.PMR = 0b0;    // I/O pin

  R_PFS->PORT[1].PIN[4].PmnPFS_b.PCR = 0b1;    // Enable pull-up
  R_PFS->PORT[1].PIN[4].PmnPFS_b.EOFR = 0b10;  // Event on falling edge
  // Tables 19.5 and onwards
  R_PFS->PORT[1].PIN[4].PmnPFS_b.ISEL = 0b1;  // Use as IRQn pin

  // The Interrupt Controller Unit (ICU) controls which event signals are linked to the Nested Vector Interrupt Controller
  // (NVIC), Data Transfer Control (DTC), and Direct Memory Access Controller (DMAC) modules.
  // See Table 13.3 Interrupt vector table
  // See Table 13.4 Event table
  // There are 47 exceptions that can be connected to events, Table 13.3
  // The first 16 exceptions are fixed NMIs, the first user definable
  // exception is number 16 = interrupt number 0, valid interrupt numbers are [0, 31]
  // To assign which event that triggers which interrupt, set:
  // R_ICU->IELSR[IRQn] = EVENTn;

  // LQFP64 D2/P104/IRQ1, Table 13.4, table 18.3, table 19.5
  // Select the IRQn_PCHG2 interrupt to be triggered by the pin change event IRQ1 (2)
  R_ICU->IELSR_b[IRQn_PCHGD2].IELS = ELC_EVENT_ICU_IRQ1;
  // Install the ISR in the IRQn_PCHGD2 interrupt vector element
  NVIC_SetVector(IRQn_PCHGD2, (uint32_t)pin_change_isr);
  // Set a suitable interrupt priority
  NVIC_SetPriority(IRQn_PCHGD2, 12);
  // Enable the interrupt
  NVIC_EnableIRQ(IRQn_PCHGD2);
}

//*********************************************************************
void setup_gpt7() {

  // Enable peripherals
  // RA4M1 User’s Manual: Hardware, chapter 10.2.5, Module stop control register D
  //R_MSTP->MSTPCRD_b.MSTPD5 = 0;  // Enable 32-bit GPT321 to GPT320, GPT0, GPT1
  R_MSTP->MSTPCRD_b.MSTPD6 = 0;  // Enable 16-bit GPT167 to GPT162, GPT2-GPT7

  // The Interrupt Controller Unit (ICU) controls which event signals are linked to the Nested Vector Interrupt Controller
  // (NVIC), Data Transfer Control (DTC), and Direct Memory Access Controller (DMAC) modules.
  // See Table 13.3 Interrupt vector table
  // See Table 13.4 Event table
  // There are 47 exceptions that can be connected to events, Table 13.3
  // The first 16 exceptions are fixed NMIs, the first user definable
  // exception is number 16 = interrupt number 0, valid interrupt numbers are [0, 31]
  // To assign which event that triggers which interrupt, set:
  // R_ICU->IELSR[IRQn] = EVENTn;

  // Select the IRQn_OVF7 interrupt to be triggered by the GPT7 overflow event (0x95 = 149, Table 13.4)
  R_ICU->IELSR_b[IRQn_OVF7].IELS = ELC_EVENT_GPT7_COUNTER_OVERFLOW;
  // Install the ISR in the IRQn_OVF7 interrupt vector element
  NVIC_SetVector(IRQn_OVF7, (uint32_t)gpt7_ovf_isr);
  // Set a suitable interrupt priority
  NVIC_SetPriority(IRQn_OVF7, 12);
  // Enable the interrupt
  NVIC_EnableIRQ(IRQn_OVF7);

  // Timer counter
  R_GPT7->GTCNT = 0;
  // Timer period, pwm period
  R_GPT7->GTPR = 48000 - 1;  // OVF interrupt every 1000 usec at PS=1 and f_MCU 48 MHz
  // Zero timer control register
  R_GPT7->GTCR = 0;
  // Timer pre-scaler
  //R_GPT7->GTCR &= ~(0b111 << 24);  // Clear bits, PS = 1
  //R_GPT7->GTCR |= (0b011 << 24);  // PS = 64, 64/48 = 4/3 usec per TCLK
  //R_GPT7->GTCR |= (0b101 << 24);  // PS = 1024
  // Start timer
  R_GPT7->GTCR_b.CST = 1;
}

//*********************************************************************
void setup() {
  Serial.begin(2400);
  Serial1.begin(2400);
  //Serial.dtr();  // Only needed for Arduino R4 Minima
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setup_gpt7();
  setup_pin_change_interrupt();
}

//*********************************************************************
void loop() {

//  for (t7_cnt = 0; t7_cnt < dly_msec;) {}
//  digitalWrite(LED_BUILTIN, HIGH);
//
//  for (t7_cnt = 0; t7_cnt < dly_msec;) {}
//  digitalWrite(LED_BUILTIN, LOW);

  cnt++;
//  Serial.print(" Counter value = ");
//  Serial.println(cnt);

  if(Serial1.available() > 0)
  {
    incomingByte = Serial1.read();
    //EEPROM.write(count_bytes,incomingByte);
    //Serial.println(incomingByte);
    count_bytes++;
     if(incomingByte == '\0')
    {
      //Serial.println("Cdone");
      eeprom_byte = EEPROM.read(0);
      Serial1.write(eeprom_byte);
      Serial1.write("$\r\n", 3);
    }
  }

//  if(Serial.available() > 0)
//  {
//    incomingByte = Serial.read();
//    Serial.println(incomingByte);
//    count_bytes++;
//  }
  if(display_speed == 1)
  {
    display_speed = 0;
    displaySpeed();
  }
}
