#include <lpc213x.h> 
#include <string.h>
#include<stdio.h>

#define SLAVE_ADDR 78     
#define MAX 12 
#define AA 2
#define SI 3
#define STO 4
#define STA 5
#define I2EN 6

// Delay Function
void wait(int count) { 
    while(count-- ); 
} 

// Initialize I2C
void I2C_Init(void) { 
    VPBDIV = 0x02; // sets FOSC = 60MHz 
    PINSEL0 = 0x00000050; // set P0.2, P0.3 to SDA SCL 
    I2C0SCLH = 150; // 50% duty, I2CFreq > 100KHz, PCLK = 30MHz 
    I2C0SCLL = 150; 
    I2C0CONSET = (1 << I2EN); // Enable I2C module 
    PINSEL1 = 0x00000000; // Configure P0.16 to P0.31 as GPIO 
    IO0DIR = 0x000F0000; 
    IO0CLR = 0x000F0000; 
} 

// Enter Master Transmitter Mode
int I2C_Start() { 
    I2C0CONCLR = 1 << STO; 
    I2C0CONCLR = 1 << SI;   
    I2C0CONSET = 1 << STA; 
    return 0; 
} 

// Delay function
void delay_ms(int count) { 
    int j = 0, i = 0; 
    for(j = 0; j < count; j++) { 
        // At 60MHz, the below loop introduces delay of 10 us 
        for(i = 0; i < 35; i++); 
    } 
} 

// Function to send data from Master to slave I2C device
void senddata(char data){ 
    while(!(I2C0CONSET & 0x08)); 
    I2C0DAT = data; 
    I2C0CONCLR = 1 << SI; 
    delay_ms(200); 
} 

void LCD_Command (char cmnd) { 
    char data; 
    data = (cmnd & 0xF0) | 0x04 | 0x08; // Send upper nibble 
    senddata(data); 
    delay_ms(100); 
    data = (cmnd & 0xF0) | 0x08; 
    senddata(data); 
    delay_ms(100); 
    data = (cmnd << 4) | 0x04 | 0x08; // Send lower nibble 
    senddata(data); 
    delay_ms(100); 
    data = (cmnd << 4) | 0x08; 
    senddata(data); 
    delay_ms(100); 
} 

void LCD_Char (char char_data) { 
    char data; 
    data = (char_data & 0xF0) | 0x08 | 0x05; // Send upper nibble 
    senddata(data); 
    delay_ms(1); 
    data = (char_data & 0xF0) | 0x08 | 0x01; 
    senddata(data); 
    delay_ms(2); 
    data = (char_data << 4) | 0x08 | 0x05; // Send lower nibble 
    senddata(data); 
    delay_ms(1); 
    data = (char_data << 4) | 0x08 | 0x01; 
    senddata(data); 
    delay_ms(5); 
    senddata(0x08); 
} 

void LCD_String (char *str) { 
    int i; 
    for(i = 0; str[i] != 0; i++) { // Send each char of string till the NULL
        LCD_Char(str[i]); // Call LCD data write 
    } 
} 

void LCD_String_xy (char row, char pos, char *str) { 
    if (row == 0) 
        LCD_Command((pos & 0x0F) | 0x80); 
    else if (row == 1) 
        LCD_Command((pos & 0x0F) | 0xC0); 
    LCD_String(str); // Call LCD string function 
} 

int main() { 
    char code = SLAVE_ADDR; 
    int ticketCount = 1;  // Initialize ticket counter
		int i;	

    I2C_Init(); // Initialize I2C module
    I2C_Start(); 
    wait(4000); // Start I2C module
    
    while(!(I2C0CONSET & 0x08)); // Wait till SI bit set
    IO0SET = (1 << 21); 
    I2C0CONCLR = 1 << STO; 
    I2C0CONCLR = 1 << STA; 
    I2C0CONSET = 1 << AA; 
    I2C0DAT = code; 
    I2C0CONCLR = 1 << SI; 
    while(!(I2C0CONSET & 0x08)); 
    IO0SET = 0x00010000; 
    if(I2C0STAT == 0x18) { 
        IO0SET = (1 << 23);     
        I2C0CONSET = 1 << AA; 
        I2C0DAT = 0x00; 
        I2C0CONCLR = 1 << SI; 
        IO0SET = 0x00020000; 
        while(!(I2C0CONSET & 0x08)); 
        for( i = 0; i < 2000; i++) wait(800); 
        if(I2C0STAT == 0x28) { 
            IO0SET = 0x00030000;           
            LCD_Command(0x02); 
            LCD_Command(0x28); // Initialization of 16X2 LCD in 4bit mode 
            LCD_Command(0x0C); // Display ON Cursor OFF 
            LCD_Command(0x06); // Auto Increment cursor 
            LCD_Command(0x01); // Clear display 
            LCD_Command(0x80); // Cursor at home position 

            while(1) { 
                if(IO1PIN & (1 << 24)) { // PIR sensor detected motion
                    char ticketMsg[16];
                    sprintf(ticketMsg, "Ticket %d Issued!", ticketCount++);
                    LCD_Command(0x01); // Clear display
                    LCD_String_xy(0, 0, ticketMsg); // Display ticket message
                    delay_ms(2000); // Display the message for 2 seconds
                    LCD_Command(0x01); // Clear display
                    LCD_Command(0x80); // Cursor at home position
                } 
            } 
        } 
    } 
    return 0; 
}
