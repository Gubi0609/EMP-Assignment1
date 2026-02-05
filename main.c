#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define SYSTICK_RELOAD_VALUE 3200000 // 200 ms delay (16 MHz / 5 = 3,200,000 cycles)

// Missing definitions in tm4c123gh6pm.h file
#define NVIC_INT_CTRL_PEND_SYST   0x04000000    // Pend a systick int
#define NVIC_INT_CTRL_UNPEND_SYST 0x02000000    // Unpend a systick int

#define SYSTICK_PRIORITY    0x7E
#define DOUBLE_CLICK_WINDOW 300  // ms window for double-click detection
#define HOLD_THRESHOLD 2000      // ms for hold detection

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

// Millisecond counter (unsigned to handle overflow properly)
volatile unsigned int milliseconds = 0;

// Button state tracking
volatile unsigned int lastPressTime = 0;
volatile bool buttonPressed = false;
volatile unsigned int buttonPressStartTime = 0;
volatile bool holdDetected = false;
volatile bool waitingForSecondClick = false;

// The Interrupt Service Routine (ISR)
void GPIOF_Handler(void) {
	// For debounce
	volatile int i;
	
	// Clear the interrupt flag for PF4 (must be done first)
	GPIO_PORTF_ICR_R = 0x10;
	
	// Simple debounce delay
	for(i = 0; i < 100000; i++);
	
	// If in auto mode, any button press exits auto mode
	if (autoMode) {
		autoMode = false;
		return;
	}
	
	unsigned int currentTime = milliseconds;
	unsigned int timeDiff = currentTime - lastPressTime;
	
	// Check for double-click
	if (waitingForSecondClick && timeDiff < DOUBLE_CLICK_WINDOW) {
		// Double-click detected! Toggle direction
		dirUp = !dirUp;
		waitingForSecondClick = false;
		holdDetected = true;  // Prevent hold and single-click actions
	} else {
		// Potential single click or hold - start tracking
		waitingForSecondClick = true;
		buttonPressed = true;
		buttonPressStartTime = currentTime;
		holdDetected = false;
	}
	
	lastPressTime = currentTime;
	
	// Don't wait for release - let SysTick_Handler monitor the button state
}

// SysTick ISR - runs every 200ms
void SysTick_Handler(void) {
	// Update millisecond counter (200ms per tick)
	milliseconds += 200;
	
	// Check for hold detection
	if (buttonPressed && !holdDetected) {
		// Check if button is still held down
		if (!(GPIO_PORTF_DATA_R & 0x10)) {
			unsigned int holdDuration = milliseconds - buttonPressStartTime;
			if (holdDuration >= HOLD_THRESHOLD) {
				// Hold detected! Toggle auto mode
				autoMode = !autoMode;
				holdDetected = true;
				waitingForSecondClick = false;  // Cancel any pending double-click
			}
		} else {
			// Button was released before hold threshold
			buttonPressed = false;
		}
	}
	
	// Auto mode - cycle through colors
	if (autoMode) {
		if (dirUp) cnt = (cnt + 1) % 8; // Increment within 0-7
		else cnt = (cnt + 7) % 8; // Decrement within 0-7
	}
	
	// Reset waiting flag after window expires
	if (waitingForSecondClick && (milliseconds - lastPressTime) >= DOUBLE_CLICK_WINDOW) {
		waitingForSecondClick = false;
		// Process single click if not hold
		if (!holdDetected) {
			// Single click: increment or decrement based on dirUp
			if (dirUp) cnt = (cnt + 1) % 8;
			else cnt = (cnt + 7) % 8;
		}
	}
}

int main(void)
{

	/*
		--- Port F Configuration ---
	*/

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


	/*
		--- Systick Configuration ---
	*/

	// Disable systick timer
	NVIC_ST_CTRL_R &= ~(NVIC_ST_CTRL_ENABLE);

	// Set current timer to reload value
	NVIC_ST_CURRENT_R = SYSTICK_RELOAD_VALUE;
	// Set Reload valye in systick reload register
	NVIC_ST_RELOAD_R = SYSTICK_RELOAD_VALUE;

	// NVIC systick setup, vector number 15 in startup_css.c
	// Clear pending systick interrupt request
	NVIC_INT_CTRL_R |= NVIC_INT_CTRL_UNPEND_SYST;

	// Set systick priority to 0x10. First clear, then set
	NVIC_SYS_PRI3_R &= ~(NVIC_SYS_PRI3_TICK_M); 
	NVIC_SYS_PRI3_R |= (NVIC_SYS_PRI3_TICK_M & (SYSTICK_PRIORITY<<NVIC_SYS_PRI3_TICK_S));

	// Select systick clock source, use core clock
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC;

	// Enable systick interrupt
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN;

	/*
		--- Button Interrupt Configuration ---
	*/

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


	// Enable and start systick timer
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE;

	// Loop forever
    while(1){
		// Set LED color (clear LED bits and set new color)
		GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~0x0E) | colors[cnt];
    }

	return 0;
}
