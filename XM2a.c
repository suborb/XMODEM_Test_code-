/* XMODEM for Z180 CPM2.2 */
/* zcc +cpm -mz180 xm2a.c -o xm2a.com */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A
#define MAXRETRANS 25
#define false 0
#define true 1
#define RDRF   7
#define TDRE   1
__sfr __at 0x00 BUTTON;
__sfr __at 0xC4 STAT0;			
__sfr __at 0xC8 TSR0;
__sfr __at 0xC6 TDR0;
__sfr __at 0xC5 STAT1;
__sfr __at 0xC9 TSR1;
__sfr __at 0xC7 TDR1; 
void flushInput(void);
unsigned char serialRX(void);
void serialTX(char);
void serialTXB(char);
unsigned int crc16_ccitt(const unsigned char *buffr, int len);
void initTransmit(void);
void initReceive(void);
void delay05s(void);
void delay2s(void);
//unsigned char inp(int);
//void outp(int, unsigned char);

char receivedChar, highByte, lowByte, lineOfData[1024];
char startMSG[] = {'H', 'e', 'l', 'l', 'o'};
int newData = false, doOnce = false, rawHex, state = 0, stage = 0;
unsigned int byteCounter, counter;

void main()                   // User menu and selection of functions.
{
	unsigned char receivedChar;
	int i;
__asm
	di
__endasm;
	flushInput();
	for(i = 0; i < 6; ++i)
	{
		serialTXB(startMSG[i]);
	}
	serialTXB('\n');
    printf("\t\t1. Read from device and Send\n");
    printf("\t\t2. Receive and Write to device\n");
    
    receivedChar = serialRX(); 
    switch(receivedChar)
    { 
            
		case '1':
			initTransmit();         
            break;
		case '2':
			initReceive();          
			break;
    }
__asm
	ei
__endasm;
}

void flushInput()
{
	unsigned char temp;
	while((STAT0 & 0x80) == 0x80)
		temp = TSR0;					// Discard any characters until the buffer is empty.
}

char hex2char(char c)
{
    char x;
    c = c & 0x0F;
    if(c >= 0 && c <= 9)
        x = c + 0x30;
    if(c >= 0x0A && c <= 0x0F)
        x = c + 0x37;
    return x;
}

char char2hex(char c)
{
    if(c > 0x2F && c < 0x3A)
        return c - 0x30;
    if(c > 0x40 && c < 0x47)
        return c - 0x37;
    if(c > 0x60 && c < 0x67)
        return c - 0x57;       
}

char generateChecksum()                         // Generate checksum for Intel HEX record.
{
    int i, checksum = 0x00, byteCount;
    byteCount = lineOfData[1];
    for(i = 0; i < byteCount + 4; i++)
    {
        checksum = checksum + lineOfData[1 + i];
    }
    checksum =~ checksum;             // Ones compliment.
    checksum ++;                      // Twos compliment.
    return checksum;
}

char verifyData()                               // Verify checksum.
{
    int i, checksum = 0x00, total = 0x00, byteCount;
    
//    Serial1.print(" checksum_entry ");
    byteCount = lineOfData[1];
//    Serial1.print(" CS_BC = ");
//    Serial1.print(byteCount, HEX);
//    Serial1.print(" \n");
    for(i = 0; i < byteCount + 6; i++)
    {
        checksum = total + lineOfData[i]; 
//      Serial1.print("i = "); 
//      Serial1.print(i, HEX);
//      Serial1.print(" ");
    }
//    Serial1.print("Checksum = ");
//    Serial1.print(checksum, HEX);
//    Serial1.print(" Total = ");
//    Serial1.print(total, HEX);
//    Serial1.print("\n");
    if(total == 0)
    {
        return 0;     // OK.
    }
    else
    { 
        return 1;     // NOT OK.
    }      
}

int xmodemTransmit()                          // Main function for XMODEM transmit.
{
    int exitLoop, addToNextPacket = false;
    unsigned char txbuff[134], packetNumber = 1;
    int bufferSize, i, c, retry, crc = -1;
    for(;;) 
    {
        for(retry = 0; retry < 16; ++retry) 
        {
			c = serialRX();
            switch (c) 
            {
                case 'C':
                    crc = 1;
                    exitLoop = false;
                    state = 0;
                    stage = 0;
                    goto start_transmission;
                case NAK:
                    crc = 0;
                    exitLoop = false;
                    state = 0;
                    stage = 0;
                    goto start_transmission;   
                case CAN:
                    delay05s();
                    c = serialRX();
                   
                    if(c == CAN)
                    {
                        serialTX(ACK);
                        flushInput();
                        return -1;                // canceled by remote.
                    }
                    break;
                    default:
                        break;
            }   
        }
        serialTX(CAN);
        serialTX(CAN);
        serialTX(CAN);
        flushInput();
        return -2;                // No sync
        for(;;)
        {
            start_transmission:
    //      Serial1.print("\nCRC = ");
    //      Serial1.print(crc, DEC);
            txbuff[0] = SOH; 
            bufferSize = 128;
            txbuff[1] = packetNumber;
            txbuff[2] = ~packetNumber;  
            if(c > bufferSize) 
                c = bufferSize;
                
            if(c >= 0 && exitLoop == false) 
            {          
                for(i = 0; i < bufferSize; i++)     
                    txbuff[3 + i] = 0;
                    
                if(c == 0)
                    txbuff[3] = CTRLZ;
                else
                {  
                    i = 0;
                    
                }
                if(crc) 
                {
                    unsigned short ccrc = crc16_ccitt(&txbuff[3], bufferSize);
                    txbuff[bufferSize + 3] = (ccrc >> 8) & 0xFF;
                    txbuff[bufferSize + 4] = ccrc & 0xFF;
                }
                else 
                {
                    unsigned char ccks = 0;
                    for(i = 3; i < bufferSize + 3; ++i) 
                    {
                        ccks += txbuff[i];
                    }
                    txbuff[bufferSize + 3] = ccks;
                }
                for(retry = 0; retry < MAXRETRANS; ++retry) 
                {
                    for(i = 0; i < bufferSize + 4 + (crc?1:0); ++i) 
                    { 
                        serialTX(txbuff[i]);
                    }
                    delay05s();
                    c = serialRX();  
                    if(c >= 0) 
                    {
                        switch(c) 
                        {
                            case ACK:
                                ++packetNumber; 
                                goto start_transmission;
                            case CAN:
                                delay05s();
                                c = serialRX();
                                if(c == CAN) 
                                {
                                    serialTX(ACK);
                                    flushInput();
                                    return -1;          // Canceled by remote. 
                                }
                                break;
                            case NAK:
                            default:
                                break;
                        }  
                    }
                } 
                serialTX(CAN);
                serialTX(CAN);
                serialTX(CAN);
                flushInput();
                return -4;                              // Transmit error. 
            }
            else 
            {
                for(retry = 0; retry < 10; ++retry) 
                {
                    serialTX(EOT);
           //         Serial1.print(" EOT \n");
                    delay05s();
                    c = serialRX();
                    
                    if(c == ACK)
                        break;
                }
                flushInput();
            }
        }   
    }
}

unsigned int crc16_ccitt(const unsigned char *buffr, int len )
{                                                                     // Generate CRC16 checksum.
    unsigned int crc = 0;
    while(len --) 
    {
        int i;
        crc ^= *(char *)buffr++ << 8;
        for(i = 0; i < 8; ++i) 
        {   
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x1021; 
            else
                crc = crc << 1;
        }
    }
    return crc;
}   

static int chck(int crc, const unsigned char *buff, int sz)
{                                                                       // Check CRC16 checksum.
    if(crc) 
    {
        unsigned short crc = crc16_ccitt(buff, sz);
        unsigned short tcrc = (buff[sz] << 8) + buff[sz + 1];
        if(crc == tcrc)
            return 1;
    }
    else 
    {
        int i;
        unsigned char cks = 0;
        for(i = 0; i < sz; ++i) 
        {
            cks += buff[i];
        }
        if(cks == buff[sz])
            return 1;    
    }  
    return 0;
}

int xmodemReceive(void)                             // Main function for XMODEM receive.
{
    int bufferSize = 128, i = 0, crc = 0, retrans = MAXRETRANS, retry;
    unsigned char rxbuff[134], c, packetNumber = 1, trychar = 'C';
	counter = 0;
	while((BUTTON & 0x40) == 0x40) 
	{
		// Wait for button press.
	}
    while(1) 
    {
        for(retry = 0; retry < 5; ++retry) 
        {   
            if(trychar)
            {
                serialTX(trychar);        //Send character 'C' to host.
				serialTXB('T');
				serialTXB(' ');
				serialTXB(trychar);
				serialTXB('\n');
				delay05s();
            }
			if((STAT0 & 0x80) == 0x80)
			{
                c = serialRX();
                switch (c)
                {
                    case SOH:                       // Start Of Header. First byte in XMODEM packet.
						serialTXB('S');
						serialTXB(hex2char(c));
						serialTXB('\n');
                        bufferSize = 128;
                        goto start_reception;
                    case STX:                       // XMODEM 1K not used.
                        flushInput();
                        serialTX(CAN);          	// Abort.
                        delay2s();
            //            Serial1.print("\n XMODEM 1K not used \n");
                        return -4;                  // XMODEM 1K not used.
                    case EOT:                       // End Of Transmission.
                        flushInput();
                        serialTX(ACK);
						serialTXB("E");
						serialTXB("\n");
                        delay05s();
                        return counter;               // Normal end. No errors, return with number of data bytes sent.
                    case CAN:                         // Cancel in an error condition.
                        if((STAT0 & 0x80) == 0x80)
						{
                            c = TSR0; 
                            if(c == CAN) 
                            {
                                serialTX(ACK);    // Acknowldege the host sender has canceled.
                                flushInput();
//								printf("Canceled by remote");
//								printf(" \n");
                                delay05s();
                                return -1;                // Canceled by remote.
                            }
                        }
                        break;
                    default:
                        break;
                }  
            }                
        } 
        if(trychar == 'C') 
        { 
            trychar = NAK; 
            continue; 
        } 
        flushInput();
        serialTX(CAN);
        serialTX(CAN);
        serialTX(CAN);
//		printf("Error \n");
        return -2;                                      // Sync error. 
        
start_reception:  
        if(trychar == 'C') 
            crc = 1;  
        trychar = 0; 
//		serialTXB('C');
//		serialTXB('R');
//		serialTXB('C');
//		serialTXB(' ');
//		if(crc == 1)
//			serialTXB('1');			// Indicate 16 bit CRC mode.
//		if(crc == 0)
//			serialTXB('0');			// Indicate 8 bit checksum mode.
//		serialTXB('\n');
//        bpoke(0x9000, c);			// save SOH.
//		serialTXB(bpeek(0x9000));		// Confirm SOH is saved.
//		serialTXB('\n');
		
//     	for(i = 0;  i < (bufferSize + (crc ? 1 : 0) + 3); ++i) 
//     	{	
//		}
					
__asm
		ld		hl,0x9001
		ld		b,0x86
RXbuffEmpty:
		in0		a,(0xC4)
		bit		6,a
		jr		nz,error
		bit		7,a
		jr		z,RXbuffEmpty
		in0		a,(0xC8)
		ld		(hl),a
		inc 	hl
		dec		b
		jr		nz,RXbuffEmpty
		ld		hl,9001
		ld		b,0x86
nnnn:
		in0		a,(0xC5)
		bit		1,a
		jr		z,nnnn
		ld		a,(hl)
		out0	(0xC7),a
		inc		hl
		dec		b
		jr		nz,nnnn
		jr		good
error:
		halt
good:

__endasm;

		//	counter++;			// Counts the number of bytes received and later displays the byte count on completion of transfer.
		
		for (i = 0;  i < (bufferSize + (crc ? 1 : 0) + 3); ++i)
		{
			serialTXB(bpeek(0x9001 + i));			// Output packet contents to serial port B to display in debug terminal.
		}
		
        if(rxbuff[1] == (unsigned char)(~rxbuff[2]) && (rxbuff[1] == packetNumber || rxbuff[1] == (unsigned char)packetNumber - 1) && chck(crc, &rxbuff[3], bufferSize))
        {   
            if(rxbuff[1] == packetNumber)  
            { 
				serialTXB('\n');
				serialTXB('P');
				serialTXB('N');
				serialTXB(' ');
				serialTXB(packetNumber);
				printf('\n');
			
												// Get_next_packet                
                    ++packetNumber;
                    retrans = MAXRETRANS + 1;

            }  
            if(--retrans <= 0) 
            {
                flushInput();
                serialTX(CAN);
                serialTX(CAN);
                serialTX(CAN);
//				printf("Error too many retries \n");
                delay05s();
                return -3;                      // Too many retry errors.
            }    
            serialTX(ACK); 
            continue;
        }   
reject:
//		printf("CRC error\n");
        flushInput();
        serialTX(NAK);   
    }    
}

void initTransmit(void)
{
    int st;
    printf("\x1b[13;02f");   //Set cursor position.
    printf("Prepare your terminal emulator to receive data now...\n");
    st = xmodemTransmit();
    printf("\n");
    if(st < 0) 
    {
        printf("        Xmodem transmit error: "); 
        printf("%d", st);
        printf("\n\n");
        delay2s();
        printf("         Press '6' to return to main menu\n");
    }
    else  
    {
        printf("\n    Xmodem successfully transmitted ");
        printf("%d", st);
        printf(" bytes \n");
        delay2s();
        printf("\n    Press '6' to return to main menu\n");
    }
}

void initReceive(void)
{
    int st;

    printf("\x1b[13;02f");   //Set cursor position.
    printf("Send data using the xmodem protocol from your terminal emulator now...\n\n");  
    st = xmodemReceive();
    if(st < 0)
    { 
        printf("        Xmodem receive error: status: ");
        printf("%d", st);
        printf("\n\n");
        delay2s();
        printf("         Press '6' to return to main menu\n");

    }
    else  
    {
        printf("    Xmodem successfully received ");
        printf("%d", st);
        printf(" bytes \n");
        delay2s();
        printf("\n    Press '6' to return to main menu\n");
    }
}

void serialTX(char character)
{
__asm
bufferFull:
	in0		a,(0xC4)
	bit		1,a
	jr		z,bufferFull
__endasm;
		TDR0 = character;
}

void serialTXB(char character)
{
__asm
bufferFull2:
	in0		a,(0xC5)
	bit		1,a
	jr		z,bufferFull2
__endasm;
		TDR1 = character;
}

unsigned char serialRX(void)
{
	unsigned char character;
__asm
bufferEmpty2:
		in0		a,(0xC4)
		bit		7,a
		jr		z,bufferEmpty2
__endasm;
		character = TSR0;
		return character;
}

void delay05s(void)
{
__asm
	push	af
	push	bc
	ld		bc,0xFFFF
loopa:
	dec		bc
	push	af
	push	bc
	ld		b,0x02
loop2a:
	dec		B
	jr		nz,loop2a
	pop		bc
	pop		af
	ld		a,b
	or		c
	jr		nz,loopa
	pop		bc
	pop		af
__endasm;
}


void delay2s(void)
{
__asm
	push	af
	push	bc
	ld		bc,0xFFFF
loopb:
	dec		bc
	push	af
	push	bc
	ld		b,0x0F
loop2b:
	dec		B
	jr		nz,loop2b
	pop		bc
	pop		af
	ld		a,b
	or		c
	jr		nz,loopb
	pop		bc
	pop		af
__endasm;
}
