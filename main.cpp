/* 
  Demo program combining Multiplexed 7-segment LED Display and LCD Display.
  Push-button Switch input via Interrupt subroutines.
  + playing with Rotary Encoder
  + add 10K pull resistor on OutA and OutB on breadboard
*/
#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"
#include "clock.h"
#include "ds3231.h"
#include "myfonts.h"
#include "DebounceIn.h"
using namespace std;

#define DISPLAY_DELAY 0.001f

// serial comms for debugging
Serial pc(USBTX, USBRX);

// LCD library
LCD_DISCO_F429ZI lcd;

// Interrupt for Dev Board PushButton switch
InterruptIn Button(PA_0);

// instantiate RTC object
// Ds3231 rtc(PC_9, PA_8); // (sda, scl)  -- consult datasheet for I2C channel pins we can use

// Dev Board LED
DigitalOut led(PG_13);
DigitalOut led2(PG_14);

// Rotary Encoder
DebounceIn EncoderOutA(PA_5);    // blue wire
DebounceIn EncoderOutB(PE_9);    // red wire
DebounceIn EncoderSwitch(PE_11); // green wire
int EncoderOutA_LastState = 0;
int EncoderOutB_LastState = 0;
int EncoderOutA_State;
int EncoderOutB_State;

// Our Interrupt Handler Routine, for Button(PA_0)
void PBIntHandler()
{
  lcd.SetFont(&Font24);
  lcd.SetTextColor(0xFFFF7700);
  led = !led; // toggle LED
  if (led)
  {
    lcd.DisplayStringAt(0, 130, (uint8_t *)"  Interrupt!  ", CENTER_MODE);
  }
  else
  {
    lcd.DisplayStringAt(0, 130, (uint8_t *)" Another IRQ! ", CENTER_MODE);
  }
}

/* intensity in % percentage */
void SetLEDBrightness(PwmOut led, float intensity)
{
  float period = 0.000009f;
  led.period(period);
  led.pulsewidth(period * (intensity / 100));
  wait(0.0001);
}

static const int Digits[] = {
    //abcdefgp    // 7-segment display + decimal point
    0b11111100, // 0
    0b01100000, // 1
    0b11011010, // 2
    0b11110010, // 3
    0b01100110, // 4
    0b10110110, // 5
    0b10111110, // 6
    0b11100000, // 7
    0b11111110, // 8
    0b11110110, // 9
    0b11101110, // A (10)
    0b00111110, // B (11)
    0b10011100, // C (12)
    0b01111010, // D (13)
    0b10011110, // E (14)
    0b10001110  // F (15)
};

/*
  NOTE: The weird ass and seemingly random pin assignments below is necessary because 
  we have USB Serial and LCD display enabled. Consult user manual for available pins. 
  */
DigitalOut Digit1(PC_11); // anode for Digit1 (ones)
DigitalOut Digit2(PC_12); // anode for Digit2 (tens)
DigitalOut Digit3(PC_13); // anode for Digit3 (hundreds)

DigitalOut SegmentA(PA_7); // clockwise, starting from top segment
DigitalOut SegmentB(PA_13);
DigitalOut SegmentC(PA_15);
DigitalOut SegmentD(PB_4);
DigitalOut SegmentE(PB_7);
DigitalOut SegmentF(PB_12);
DigitalOut SegmentG(PC_3); // middle segment
DigitalOut SegmentP(PC_8); // decimal point

void Display_Clear()
{
  // reset all pins, clear display
  // common anode, so logic 1 for OFF
  SegmentA = SegmentB = SegmentC = SegmentD = SegmentE = SegmentF = SegmentG = SegmentP = !0;
  Digit3 = Digit2 = Digit1 = 0;
}

void Display_Digit(int DigitPosition, int Number)
{
  // common anode display, so invert logic to light up each segment
  SegmentA = !((Digits[Number] & 0b10000000) >> 7);
  SegmentB = !((Digits[Number] & 0b01000000) >> 6);
  SegmentC = !((Digits[Number] & 0b00100000) >> 5);
  SegmentD = !((Digits[Number] & 0b00010000) >> 4);
  SegmentE = !((Digits[Number] & 0b00001000) >> 3);
  SegmentF = !((Digits[Number] & 0b00000100) >> 2);
  SegmentG = !((Digits[Number] & 0b00000010) >> 1);
  SegmentP = !((Digits[Number] & 0b00000001) >> 0);

  // we need to clear out the other digits before displaying the new digit
  // otherwise, the same number will be displayed in all the digits.
  // common anode display, so logic 1 to light up the digit.
  switch (DigitPosition)
  {
  case 1:
    Digit1 = 1; // ones
    Digit2 = 0;
    Digit3 = 0;
    break;
  case 2:
    Digit1 = 0; // tens
    Digit2 = 1;
    Digit3 = 0;
    break;
  case 3:
    Digit1 = 0; // hundreds
    Digit2 = 0;
    Digit3 = 1;
    break;
  }
  wait(DISPLAY_DELAY);
}

void Display_Number(int Number, uint32_t Duration_ms)
{
  int hundreds, tens, ones;
  uint32_t start_time_ms, elapsed_time_ms = 0;
  Timer t;

  // breakdown our Number into hundreds, tens and ones
  hundreds = Number / 100;
  tens = (Number % 100) / 10;
  ones = (Number % 100) % 10;

  t.start(); // start timer, we'll use this to setup elapsed display time
  start_time_ms = t.read_ms();
  while (elapsed_time_ms < Duration_ms)
  {
    Display_Digit(3, hundreds);
    Display_Digit(2, tens);
    Display_Digit(1, ones);
    elapsed_time_ms = t.read_ms() - start_time_ms;
  }
  t.stop(); // stop timer
}

// clock data
uint32_t year = 2018;
uint32_t month = 12;
uint32_t day_of_week = 6;
uint32_t day = 15;
uint32_t hh = 10;
uint32_t mm = 54;
uint32_t ss = 00;
uint32_t rtn_val;

/* 
  Start of Main Program 
  */
int main()
{
  // set usb serial
  // pc.baud(115200);

  // setup Rotary Encoder
  // EncoderOutA.mode(PullUp);
  // EncoderOutB.mode(PullUp);
  EncoderSwitch.mode(PullDown);
  EncoderOutA.set_debounce_us(250); // in microseconds
  EncoderOutB.set_debounce_us(250); // in microseconds

  // setup Interrupt Handler
  Button.rise(&PBIntHandler);

  // setup LCD Display
  lcd.Clear(0xFF000055);
  lcd.SetFont(&Font24);
  lcd.SetBackColor(0xFF000055);      // text background color
  lcd.SetTextColor(LCD_COLOR_WHITE); // text foreground color
  char buf[50];                      // buffer for integer to text conversion
  lcd.DisplayStringAt(0, 200, (uint8_t *)" by owel.codes ", CENTER_MODE);

  // setup 7-segment LED Display
  // Display_Clear();


  // init previous state
  EncoderOutA_LastState = 0;
  EncoderOutB_LastState = 0;

  pc.printf("\n\n\n\nStart of New Session!\n");

  int ctr = 0;
  int rotation_value = 0;
  // start of main loop
  while (true)
  {
    ctr += 1;

    // Display_Number(rotation_value, 100); // Number to display on 7-segment LED, Duration_ms

    // Check Rotary Encoder status (switch + rotation)
    // NOTE an ordinary digitalIn switch read is unreliable/slow/laggy
    if (EncoderSwitch.read())
    {
      led2 = !led2;
      lcd.DisplayStringAt(0, 160, (uint8_t *)" Toggle LED! ", CENTER_MODE);
    }
    else
    {
      lcd.DisplayStringAt(0, 160, (uint8_t *)"             ", CENTER_MODE);
    };

    // based on datasheet (CW Rotation)
    // position |  OutA  | OutB
    //    1     |    0   |  0
    //    2     |    1   |  0
    //    3     |    1   |  1   // starting 
    //    4     |    0   |  1

    // https://os.mbed.com/users/aberk/code/QEI/file/5c2ad81551aa/QEI.cpp/

    // due to pull up resistor, A will always start HIGH
    EncoderOutA_State = EncoderOutA.read();
    // EncoderOutB_State = EncoderOutB.read();
    // if A changes state (i.e. becomes LOW), read B. 
    if (EncoderOutA_State != EncoderOutA_LastState) {
      EncoderOutB_State = EncoderOutB.read();

      pc.printf("%d. Out A: %d \n", ctr, EncoderOutA_State);
      pc.printf("%d. Out B: %d \n", ctr, EncoderOutB_State);    
      pc.printf("-------------\n");    

      // if A and B are opposite, then it's CW 
      if (EncoderOutA_State != EncoderOutB_State) {
        rotation_value += 1;
        pc.printf("%d. Rotated CW %d \n\n", ctr, rotation_value);
      } 

      // if A and B are both LOW, then it's CCW
      if (!EncoderOutA_State && !EncoderOutB_State){
        rotation_value -= 1;
        pc.printf("%d. Rotated CCW %d \n\n", ctr, rotation_value);
      }
      // update last states
      EncoderOutA_LastState = EncoderOutA_State;


      // display on LCD
      lcd.SetFont(&Grotesk16x32);
      lcd.SetTextColor(LCD_COLOR_GREEN);
      lcd.DisplayStringAt(0, 50, (uint8_t *)"Rotary Encoder", CENTER_MODE);
      sprintf(buf, "%03d", rotation_value);
      lcd.DisplayStringAt(0, 100, (uint8_t *)buf, CENTER_MODE);
					
    }

  }
  return 0;
}