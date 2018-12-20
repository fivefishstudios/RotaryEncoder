/* 
  Demo program combining Multiplexed 7-segment LED Display and LCD Display.
  Push-button Switch input via Interrupt subroutines.
  + playing with Rotary Encoder
  + add 10K pull resistor on OutA and OutB on breadboard
*/
#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"
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

// Dev Board LED
DigitalOut led(PG_13);

// Rotary Encoder
DebounceIn EncoderOutA(PA_5);
DebounceIn EncoderOutB(PE_9);
int EncoderOutA_LastState = 0;
int EncoderOutB_LastState = 0;
int EncoderOutA_State;
int EncoderOutB_State;

// Rotary Enc Switch
// DebounceIn EncoderSwitch(PE_11);

// Our Interrupt Handler Routine, for Button(PA_0) dev board
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


/* 
  Start of Main Program 
  */
int main()
{
  // set usb serial
  // pc.baud(115200);

  // setup Rotary Encoder
  EncoderOutA.set_debounce_us(180); // in microseconds
  EncoderOutB.set_debounce_us(180); // in microseconds

  // setup Interrupt Handlers
  Button.rise(&PBIntHandler);
  
  // setup LCD Display
  lcd.Clear(0xFF000055);
  lcd.SetFont(&Font24);
  lcd.SetBackColor(0xFF000055);      // text background color
  lcd.SetTextColor(LCD_COLOR_WHITE); // text foreground color
  char buf[50];                      // buffer for integer to text conversion
  lcd.DisplayStringAt(0, 200, (uint8_t *)" by owel.codes ", CENTER_MODE);

  // init previous state
  EncoderOutA_LastState = 0;
  EncoderOutB_LastState = 0;

  pc.printf("\n\n\n\nStart of New Session!\n");

  lcd.SetFont(&Grotesk16x32);
  lcd.SetTextColor(LCD_COLOR_GREEN);
  lcd.DisplayStringAt(0, 50, (uint8_t *)"Encoder v1.2", CENTER_MODE);

  int ctr = 0;
  int rotation_value = 1;
  // start of main loop
  while (true)
  {
    ctr += 1;

    // Check Rotary Encoder status (switch + rotation)
    // based on datasheet (CW Rotation)
    // position |  OutA  | OutB
    //    1     |    0   |  0
    //    2     |    1   |  0
    //    3     |    1   |  1   // starting 
    //    4     |    0   |  1

    // due to pull up resistor, A will always start HIGH
    EncoderOutA_State = EncoderOutA.read();
    // if A changes state (i.e. becomes LOW), read B. 
    if (EncoderOutA_State != EncoderOutA_LastState) {
      EncoderOutB_State = EncoderOutB.read();

      // we need this printf line, accessing the ctr variable for the rotary encoder to work. WHY WHY WHY?????
      pc.printf("%d. \n", ctr);  // must be printing the 'ctr' variable. Other variables don't work
      // pc.printf("dummy text"); <---- this doesn't work either

      // check if we moved CW or CCW
      if (EncoderOutA_State != EncoderOutB_State) {
        // CW
        rotation_value += 1;
      } else {
        // CCW
        rotation_value -= 1;
      }
      
      // update last states
      EncoderOutA_LastState = EncoderOutA_State;

      // display on LCD
      lcd.SetFont(&Grotesk16x32);
      lcd.SetTextColor(LCD_COLOR_GREEN);
      // lcd.DisplayStringAt(0, 50, (uint8_t *)"Rotary Encoder", CENTER_MODE);
      sprintf(buf, "%03d", rotation_value);
      lcd.DisplayStringAt(0, 100, (uint8_t *)buf, CENTER_MODE);
    }
  }
  return 0;
}