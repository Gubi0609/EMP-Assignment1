#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "systick.h"

// Define timer
#define TIM_1_SEC 200;

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

bool dirUp = true;
bool autoMode = false;

// ticks imported from systick.c
extern int ticks;

// The Interrupt Service Routine (ISR)
void GPIOF_Handler(void) {
	// For debounce
	volatile int i;
	
	// Clear the interrupt flag for PF4 (must be done first)
	GPIO_PORTF_ICR_R = 0x10;
	
	// Increment counter to cycle through colors
	cnt = (cnt + 1) % 8;  // Cycle through 0-7
	
	// Simple debounce delay
	for(i = 0; i < 100000; i++);

	// Wait for button release as to not increment indefinitely
	while(!(GPIO_PORTF_DATA_R & 0x10));
}

int main(void)
{

	// Enable systick timer according to convention
	int alive_timer = TIM_1_SEC;

	init_systick();

	int dummy; // Dummy to do a few cycles

	// Enable GPIO port F (used for RBG and switch)
	// by turning on power to the port
	SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF; // System Control Run-Mode Clock Gating Control

	// Do a dummy read to insert a few cycles after enabling the peripheral
	dummy = SYSCTL_RCGC2_R;

	// Enable RGB (PF1 - PF3) as output and switch (PF4) as input
	GPIO_PORTF_DIR_R = 0x0E; // 0000 1110 - Bit 1 is output, bit 0 is input

	// Enable pull-up resister for switch (PF4)
	GPIO_PORTF_PUR_R = 0x10; // 0001 0000

	// Enable digital function for PF1 - PF4
	GPIO_PORTF_DEN_R = 0x1E;


	// Set switch (PF4) as edge-sensitive
	GPIO_PORTF_IS_R = 0x00;

	// Trigger controlled by IEV
	GPIO_PORTF_IBE_R = 0x00;

	// Falling Edge Trigger
	GPIO_PORTF_IEV_R = 0x00;

	// Clear any Prior Interrupts
	GPIO_PORTF_ICR_R = 0x10;

	// Unmask interrupts for PF4
	GPIO_PORTF_IM_R = 0x10;


	// NVIC Configuration
	// Enable interrupt 46 (GPIO Port F) in NVIC
	// INT_GPIOF = 46 (vector table position)
	// IRQ number = 46 - 16 = 30 (subtract ARM core exceptions)
	// IRQ 30 is in NVIC_EN0_R at bit position 30
	NVIC_EN0_R |= (1 << (INT_GPIOF - INT_GPIOA));


	// Loop forever
    while(1){
		while(!ticks) {
			// Will be executed every 5 ms according to systick.c
			ticks--;

			if(!--alive_timer) {
				alive_timer = TIM_1_SEC;
				cnt = (cnt+1) % 8;
				
				// Set LED color (clear LED bits and set new color)
    		    GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~0x0E) | colors[cnt];
			}
		}
    }

	return 0;
}
