# XMODEM_Test_code

### XMODEM code for CP/M on the Z180. This code is work in progress and has bugs. 

### Only looking at the code starting at line 325 where XMODEM reception starts...

#### Using "terminal 1" for main communication with "UART0" to issue CP/M commands and do XMODEM data transfer, and "terminal 2" to only receive debug data from "UART1" sent by instructions placed in the code to observe various parameters.

#### After setting the code to recive XMODEM data and "terminal 1 to send a text file using XMODEM the SC126 sends the 'C' character and "terminal 1" sends the SOH. 
#### This is confirmed by debug code sending the received "SOH" byte to "terminal 2"

#### The code up until line 424 works correctly
#### After this where the inline assembler code starts to receiv the packet data it fails.
#### The assembler code points (HL) registers at memory 0x9001 for storing packet data. The B register is set so that it will read in 134 bytes of packet data.
#### The code checks the UART0 status register to see if the RX buffer is empty and reads in a byte from the UART data register when ready.
#### The code checks for an over-run error by reading "ASCI Control register EFR bit 3" and jumps to "error:" and executes a HALT instruction if over-run occurs. 
#### (I have an LED connected to my CPU HALT line).

#### The code never makes it past receiving the first byte of data and always jumps to the over-run "error:" condition.

#### I can not explain why this is not working because the assembler code should be more than fast enough to keep up with reception at 115200 baud and I previusly made an Arduino project in 'C' and it works perfectly without issues. 

#### If anyone is reading this and wants to try this on their SC126 CP/M setup please feel free to do so.



