#include<avr/io.h>
#include<util/delay.h>
#include<stdlib.h>
#include<avr/interrupt.h>

volatile int tot_overflow, tot_overflow1, value ; //overflow counter
volatile int sec = 0, sec1 = 0;                   //seconds increment for time
volatile int ds=0;                                 
volatile int mint1=0;

int busy();                                       // check for sending data to LCD
void sendnibble(char nibble);                     // sends each nibble seperately  
void lcdcmd(char cmd);                            // performs LCD commands
void lcddata(char dat);                           // sends data to LCD 
void lcdstring(char *str);                        // print strings in LCD 
void lcdnum(int num);                             // prints number
int format(int num, int curs);                    // prints formated number
int adc();                                        // gets adc value   
void alarm_tune(int tune);                        // plays alarm tune

int main()
{   
    int mint[]={0,0,0},hrs[]={0,0,0},button[]={0,0,0},mode=0,stop_condition=0,edit_condition=0,date=0,month=1,counter=0,temperature,stop_mint=0,counter1=0,alarm_condition=0,counter2=0;
    char day[7][4]={"sun","mon","tue","wed","thu","fri","sat"};  
    char alarm[2][4]={"no ","yes"};
    char tune[4][6]={"tune1","tune2","tune3","tune4"};
    
    DDRB = 0x0F;           //input for last four pins, rest output
    DDRD = 0b11101111;           //all pins output
    PORTB = 0b00110000;    //setting 2 pins for buttons 
    PORTD = 0b00010000;
    DDRD &= ~(1<<PD3);
  
    //TIMER0 INITIALISE
    TCCR0B = (1<<CS02) | (1<<CS00);   //prescalar 1024
    TCNT0 = 0;        
    TIMSK0 |= (1<<TOIE0);             //set interrupts enable   
    sei();                            //set interrupts 
    tot_overflow = 0;

    //TIMER1 INITIALISE
    TCCR1B |= (1<<CS11);              //prescalar 8
    TCNT1=0;
    tot_overflow1 = 0;
    
    DDRC |= 0x00;
    ADMUX = 1<<REFS0 ;                        // internal reference voltage 
    ADCSRA = 1<<ADEN | 1<<ADPS1 | 1<<ADPS2 ;  // adc enable and prescalar 64
        
    //ADC INITIALISE
    DDRC |= 0x00;
    ADMUX = 1<<REFS0 ;
    ADCSRA = 1<<ADEN | 1<<ADPS1 | 1<<ADPS2 ;
    
    PORTD |= 1<<PD5 | 1<<PD6 | 1<<PD7;
    _delay_us(15);

    lcdcmd(0x02);
    lcdcmd(0x28);        // LCD
    lcdcmd(0x0C);        // INITIALISE
    lcdcmd(0x01);
    lcdcmd(0x80);
    
    while(1)
    {   
        //check whether button 0 is pressed 
        if(PINB != (PINB|(1<<PB5)))
        {
            if(button[0]==0)
            {   
                if(mode>=3)
                   {
                   mode=0;//variable for switch case
                   }     
                else
                   {
                   mode++;
                   }    
                button[0]=1;
                lcdcmd(0x01);
            }
        }
        else
        {
            button[0]=0;
        }
     
        //switch case for various modes
        switch(mode)
        {
        case 0:                           //clock mode
            lcdcmd(0x80);
            lcdstring("time - "); 
            format(hrs[0],0x80+7);
            lcdcmd(0x80+9);
            lcddata(':');                 //
            format(mint[0],0x80+10);      //
            lcdcmd(0x80+15);              // TIME FORMAT
            lcddata('s');                 // 
            format(date,0x80+0x40);       //  
            lcdcmd(0x80+0x40+2);
            lcddata('/');
            format(month,0x80+0x40+3);
            lcdcmd(0x80+0x40+7);
            lcdstring(day[counter]);
            lcdcmd(0x80+0x40+12);
            lcdstring("00");
            lcdcmd(0x80+0x40+14);
            lcddata(223);
            lcdcmd(0x80+0x40+15);
            lcddata('C');
            value=0;
            //check whether button 1 is pressed
            while(PINB == (PINB|(1<<PB5)))
            {       
                if(mint[2]==mint[0] && hrs[2]==hrs[0] && counter1==1)   //checks for alarm condition                    
                {   
                    if(sec<59)
                    {
                        alarm_tune(counter2);                           //plays alarm
                    }
                    else
                    {
                        counter1=0;                
                    }
                }
                else
                {
                    DDRD &= ~(1<<PD3);
                }
                
                if(sec>59)              //prints minutes
                {
                    sec=0;
                    mint[0]++;
                    format(mint[0],0x80+10);
                }
                else                    //prints seconds  
                {
                    format(sec,0x80+13);   
                }
               
                if(mint[0]>59)          //prints hours
                {
                    mint[0]=0;
                    sec=0;
                    hrs[0]++;
                    format(hrs[0],0x80+7);
                }
                      
                if(hrs[0]>23)           //sets value for days and months                  
                {
                    hrs[0]=0;
                    mint[0]=0;
                    sec=0;
                    if(month==2)  
                    {
                        if(date>29)
                        {
                            month++;
                            date=0;
                            hrs[0]=0;
                            mint[0]=0;
                            sec=0;
                        }
                    }
                    else if(month==1 || month==3 || month==5 || month==7 || month==8 || month==9 || month==10)
                    {
                        if(date>31)
                        {
                            month++;
                            date=0;
                            hrs[0]=0;
                            mint[0]=0;
                            sec=0;
                        }
                    }
                    else if(month==4 || month==6 || month==9 || month==11)
                    {
                        if(date>30)
                        {
                            month++;
                            date=0;
                            hrs[0]=0;
                            mint[0]=0;
                            sec=0;
                        }
                    }
                    date++;
                    counter++;
                    format(date,0x80+0x40);          //prints date
                    lcdstring(day[counter]);         //prints day  
                }

                if(month<=12)
                {
                    format(month,0x80+0x40+3);       //prints month                                                  
                }
                else
                {
                    month=1;
                    format(month,0x80+0x40+3);       
                }

                temperature = adc();
                format(temperature,0x80+0x40+12);    //prints temperature
                button[0]=0; 
            }
            continue;

        // case for stopwatch
        case 1:
            lcdcmd(0x80+3);             
            lcdstring("stopwatch");
            lcdcmd(0x80+0x40+3);
            lcdstring("00:00  0");
            mint[1]=0;
     
            while(PINB == (PINB|(1<<PB5)))    //loops till button 0 is pressed
            {   
                button[0]=0;
                //checks whether button 1 is pressed
                if(PINB != (PINB|(1<<PB4)))
                {  
                    if(button[1]==0)
                    {   
                        if(stop_condition>=1)
                            stop_condition=0;     //variable for switch case     
                        else
                            stop_condition++;    
                        button[1]=1;
                    }                                
                }
                else
                {
                    button[1]=0;
                }
                
                //checks whether button 2 is pressed
                if(PIND != (PIND|(1<<PD4)))
                {
                    if(button[2]==0)
                    {   
                        ds=0;
                        sec1=0;
                        mint[1]=0;
                        format(mint1,0x80+0x40+3);         //prints minutes
                        format(sec1,0x80+0x40+6);          //prints seconds
                        format(ds,0x80+0x40+10);           //prints deci seconds
                        button[2]=1;
                    }       
                }
                else
                {
                    button[2]=0;
                }

               
                switch(stop_condition)
                {
                 case 0:                              //timer 1 interrupt disable 
                     _delay_us(100);
                     TIMSK1 &= ~(1<<TOIE1);
                     break;          
                 
                 case 1:                              //timer 1 interrupt enable   
                     _delay_us(100);
                     TIMSK1 |= (1<<TOIE1);
                     format(mint1,0x80+0x40+3);
                     format(sec1,0x80+0x40+6);
                     format(ds,0x80+0x40+10);  
                     break; 
                }         
            }
            continue;

        // case for editing    
        case 2:
            lcdcmd(0x80+2);
            lcdstring("edit:");
            format(hrs[0],0x80+9);
            lcddata(':');
            format(mint[0],0x80+12);
            format(date,0x80+0x40+2);
            lcddata('/');
            format(month,0x80+0x40+5);
            lcdcmd(0x80+0x40+9);
            lcdstring(day[counter]);

            //checks whether button 0 is pressed
            while(PINB == (PINB|(1<<PB5)))
            {
                button[0]=0;

                if(PIND != (PIND|(1<<PD4)))
                {
                    if(button[2]==0)
                    {   
                        if(edit_condition>=5)
                             edit_condition=0;//variable for switch case     
                        else
                            edit_condition++;    
                        button[2]=1;
                    }       
                }
                else
                {
                    button[2]=0;
                }

                switch(edit_condition)
                {
                case 1:                              //case for editing hour
                    lcdcmd(0x80+9);
                    lcdcmd(0x0F);
                    while(PIND == (PIND|(1<<PD4)))   //checks whether button 0 is pressed
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))
                        {  
                            if(button[1]==0)
                            {   
                                if(hrs[0]>=23)
                                    hrs[0]=0;//variable for switch case     
                                else
                                    hrs[0]++;    
                                button[1]=1;
                            } 
                            format(hrs[0],0x80+9);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }
                       
                    }
                    
                case 2:                               //case for editing minutes
                    lcdcmd(0x80+12);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))  //check whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(mint[0]>=59)
                                    mint[0]=0;        //variable for switch case     
                                else
                                    mint[0]++;    
                                button[1]=1;
                            } 
                            format(mint[0],0x80+12);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;
                    
                case 3:                              //case for editing date
                    lcdcmd(0x80+0x40+2);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))  //checks whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(date>=31)
                                    date=0;//variable for switch case     
                                else
                                    date++;    
                                button[1]=1;
                            } 
                            format(date,0x80+0x40+2);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;
                    
                case 4:                                //case for editing month
                    lcdcmd(0x80+0x40+5);
                    while(PIND == (PIND|(1<<PD4)))     //checks whether buton 1 is pressed
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))
                        {  
                            if(button[1]==0)
                            {   
                                if(month>=12)
                                    month=1;//variable for switch case     
                                else
                                    month++;    
                                button[1]=1;
                            } 
                            format(month,0x80+0x40+5);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;

                case 5:                             //case for editing day 
                    lcdcmd(0x80+0x40+9);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4))) //checks whether button 2 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(counter>=6)
                                    counter=0;      //variable for switch case     
                                else
                                    counter++;    
                                button[1]=1;
                            } 
                            lcdcmd(0x80+0x40+9);
                            lcdstring(day[counter]);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    break;           
                }    
            }
            lcdcmd(0x0C);
            continue;

        // case for alarm
        case 3:
            lcdcmd(0x0F);
            lcdcmd(0x80+2);
            lcdstring("alarm - ");
            lcdcmd(0x80+9);
            lcdstring(alarm[counter1]);
            lcdcmd(0x80+0x40+2);
            lcdstring("00:00");
            lcdcmd(0x80+0x40+9);
            lcdstring(tune[counter2]);

            //swaps between alarm conditions 
            while(PINB == (PINB|(1<<PB5)))
            {
                button[0]=0;

                if(PIND != (PIND|(1<<PD4)))      //checks whether button 2 is pressed
                {
                    if(button[2]==0)
                    {   
                        if(alarm_condition>=4)
                            alarm_condition=0;   //variable for switch case     
                        else
                            alarm_condition++;    
                        button[2]=1;
                    }       
                }
                else
                {
                    button[2]=0;
                }

                switch(alarm_condition)
                {
                case 1:                              // asks for on or off for alarm
                    lcdcmd(0x80+9);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))  //checks whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(counter1>=1)
                                    counter1=0;      //variable for switch case     
                                else
                                    counter1++;    
                                button[1]=1;
                            } 
                            lcdcmd(0x80+9);
                            lcdstring(alarm[counter1]);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;
                      
                case 2:                               //sets hours for alarm
                    lcdcmd(0x80+0x40+2);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))   //checks whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(hrs[2]>=23)
                                    hrs[2]=0;         //variable for switch case     
                                else
                                    hrs[2]++;    
                                button[1]=1;
                            } 
                            format(hrs[2],0x80+0x40+2);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;

               case 3:                                 //sets minutes for alarm
                    lcdcmd(0x80+0x40+5);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))    //checks whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(mint[2]>=59)
                                    mint[2]=0;//variable for switch case     
                                else
                                    mint[2]++;    
                                button[1]=1;
                            } 
                            format(mint[2],0x80+0x40+5);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;

                case 4:                                 //sets tune for alarm
                    lcdcmd(0x80+0x40+9);
                    while(PIND == (PIND|(1<<PD4)))
                    {
                        button[2]=0;
                        if(PINB != (PINB|(1<<PB4)))     //checks whether button 1 is pressed
                        {  
                            if(button[1]==0)
                            {   
                                if(counter2>=3)
                                    counter2=0;//variable for switch case     
                                else
                                    counter2++;    
                                button[1]=1;
                            } 
                            lcdcmd(0x80+0x40+9);
                            lcdstring(tune[counter2]);                                               
                        }
                        else
                        {
                            button[1]=0;
                        }  
                    }
                    continue;    
                }
            }
            lcdcmd(0x0C);
            continue;    
   
        }
        
    }
}

//interrupt for overflow timer 0 
ISR(TIMER0_OVF_vect)
{
    tot_overflow++;
    if(tot_overflow > 63)
    {    
        sec++;
        tot_overflow = 0;
    }    
}

//interrupt for overflow timer 1

ISR(TIMER1_OVF_vect)
{
    tot_overflow1++;
    if(tot_overflow1 > 3)
    { 
        ds++;
        if(ds>10)
        {    
            sec1++;
            if(sec1>59)
            {
                mint1++;
                sec1=0;      
            }
            ds=0;
        }
        tot_overflow1 = 0;
    }    
}


//check busy flag
int busy()
{
    PORTD |= 1<<PD6;            // mask RS
    PORTD &= ~(1<<PD5);         // clear RW
    PORTD |= 1<<PD7;            // mask E
    if(PORTB == PORTB|(1<<PB4)) // check for most significant pin 
        return 0;
    else
        return 1;  
}

//send nibble one by one since we are using 4-bit transfer
void sendnibble(char nibble)
{
    PORTB &= ~(1<<PB0 | 1<<PB1 | 1<<PB2 | 1<<PB3); //clear all data pins
    _delay_us(10);
    PORTB |= (((nibble >> 0x00) & 0x01) << 0);
    PORTB |= (((nibble >> 0x01) & 0x01) << 1);
    PORTB |= (((nibble >> 0x02) & 0x01) << 2);     //send data through pins
    PORTB |= (((nibble >> 0x03) & 0x01) << 3);     //by shifting them
    _delay_us(10);
}

//send commands 
void lcdcmd(char cmd)
{
    while(busy());                     //wait while busy
    sendnibble((cmd >> 0x04) & 0x0F);  //send second nibble
    PORTD &= ~(1<<PD5);                //clear RS 
    PORTD &= ~(1<<PD6);                //clear RW
    PORTD |= (1<<PD7);                 //set   E  
    _delay_us(1000);
    PORTD &= ~(1<<PD7);                //clear R 

    _delay_us(100);

    sendnibble(cmd & 0x0F);      //send first nibble
    PORTD &= ~(1<<PD5);          //clear RS      
    PORTD &= ~(1<<PD6);          //clear RW
    PORTD |= (1<<PD7);           //set E
    _delay_us(1000);
    PORTD &= ~(1<<PD7);          //clear E

    _delay_us(1000);
}

//send data
void lcddata(char dat)
{
    while(busy());                    //wait while busy
    sendnibble((dat >> 0x04) & 0x0F); //send second nibble
    PORTD |= 1<<PD5 ;                 //set RS
    PORTD &= ~(1<<PD6);               //clear RW  
    PORTD |= 1<<PD7 ;                 //set E
    _delay_us(10);
    PORTD &= ~(1<<PD7);               //clear E

    _delay_us(100);

    sendnibble(dat & 0x0F);           //send first nibble 
    PORTD |= 1<<PD5 ;                 //set RS
    PORTD &= ~(1<<PD6);               //clear RW
    PORTD |= 1<<PD7 ;                 //set E
    _delay_us(10);
    PORTD &= ~(1<<PD7);               //clear E 

    _delay_us(10);
}

//send string
void lcdstring(char *str)
{
    while(*str > 0)
    {
        lcddata(*str++);    
    }
}

//send number
void lcdnum(int num)
{
   char string[2];
   itoa(num,string,10);            //converts number to string
   lcdstring(string); 
}

int format(int num, int curs)
{
    if(num>9)
    {
        lcdcmd(curs);
        _delay_us(10);         
        lcdnum(num);
    }
    else
    {
        lcdcmd(curs);         
        lcddata('0');
        lcdcmd(curs+1);         
        lcdnum(num);
    }    
}

int adc()
{
    float adc_value;
    int temperature;
 
    //print adc value of temperature
    ADCSRA |= (1<<ADSC);                    //starts conversion
    while(!(ADCSRA & (1<<ADIF)));           //waits for interrupt flag to get set 
    ADCSRA |= (1<<ADIF);                    //clears interrupt flag
     
    adc_value=ADC;
    temperature=(((adc_value*5)/1024))*100;
    return temperature; 
}

void alarm_tune(int tune)
{
    switch(tune)
    {
    case 1:
          TCCR2A = (1<<WGM21) |( 1<<COM2B1) |(1<<WGM20);   
          TCCR2B = (1<<CS21)|(1<<CS20);                                //Prescalar 64
          DDRD|=1<<PD3;
          if(value<255)
              OCR2B=value++;
          else
              value=0;
          break;    
    case 2:   
          TCCR2A = (1<<WGM21) |( 1<<COM2B1)|(1<<WGM20) | (1<<WGM22) ;
          TCCR2B = (1<<CS22)|(1<<CS20);                                //Prescalar 64
          DDRD|=1<<PD3;
          //OCR2B=126;
          //OCR2B=189;
          //OCR2B=159;
          OCR2B=238;
          _delay_ms(100);
          break;
          
    case 0:         
          TCCR2A = (1<<WGM21) |( 1<<COM2B1)|(1<<WGM20) | (1<<WGM22) ;
          TCCR2B = (1<<CS22)|(1<<CS20);                               //Prescalar 64
          DDRD|=1<<PD3;
          OCR2B=126;
          _delay_ms(100);
          OCR2B=159;
          _delay_ms(100);
          OCR2B=238;
          _delay_ms(100);
          //OCR2B=189;
          //_delay_ms(100);
          break;

    case 3:     
         TCCR2A = (1<<WGM21) |( 1<<COM2B1)|(1<<WGM20) | (1<<WGM22) ;
          TCCR2B = (1<<CS22)|(1<<CS20);                              //Prescalar 64
          DDRD|=1<<PD3;
          //OCR2B=126;
          OCR2B=159;
          //OCR2B=238;
          _delay_ms(100);
          break;
    }
}

