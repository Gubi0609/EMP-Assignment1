#include <stdint.h>
#include "tm4c123gh6pm.h"

// Initialize colors {PF1, PF2, PF3} = {r, b, g}
const int off = 0x00; // 0000 0000
const int green = 0x08; // 0000 1000
const int blue = 0x04; // 0000 0100
const int cyan = 0x0C; // 0000 1100
const int red = 0x02; // 0000 0010
const int yellow = 0x0A; // 0000 1010
const int magenta = 0x06; // 0000 0110
const int white = 0x0E; // 0000 1110

// Color pointer vector to point to the colors
int colors[8] = {off, green, blue, cyan, red, yellow, magenta, white};

// Current count = current color
int cnt = 0;

int main(void)
{
	int dummy; // Dummy to do a few cycles

	// Enable GPIO port F (used for RBG and switch)
	// by turning on power to the port
	SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF; // System Control Run-Mode Clock Gating Control

	// Do a dummy read to insert a few cycles after enabling the peripheral

	// Enable RGB (PF1 - PF3) as output and switch (PF4) as input
	GPIO_PORTF_DIR_R = 0x0E; // 0000 1110 - Bit 1 is output, bit 0 is input

	// Enable pull-up resister for switch (PF4)
	GPIO_PORTF_PUR_R = 0x10; // 0001 0000

	// Enable digital function for PF1 - PF4
	GPIO_PORTF_DEN_R = 0x1E;

	// Loop forever
    while(1){
        if (GPIO_PORTF_DATA_R & 0x10) cnt++; // If switch pressed (HIGH), increment cnt

		if (cnt > 8) cnt = 0;

		GPIO_PORTF_DATA_R |= colors[cnt];

        }

	return 0;
}
