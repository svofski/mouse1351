//! \file
//! \brief USART interface

#include <stdio.h>
#include <avr/io.h>

#include "usrat.h"

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_buffer_in;
static volatile uint8_t rx_buffer_out;

//! \brief Initialize USART, perform fdevopen with uart_putchar.
//! \param baudval (F_CPU/(16*baudrate))-1
//! \sa uart_putchar()
void usart_init(uint16_t baudval) {
	// Set baud rate
	UBRRH = (uint8_t)(baudval>>8);
	UBRRL = (uint8_t)baudval;

	rx_buffer_in = rx_buffer_out = 0;

	// Set frame format: 8 data, 1 stop bit
	UCSRC = (uint8_t)((1<<URSEL) | (0<<USBS) | (3<<UCSZ0));
	
	// Enable receiver and transmitter, enable RX complete interrupt
	UCSRB = (uint8_t)((1<<RXEN) | (1<<TXEN) | (1<<RXCIE));

	(void)fdevopen(uart_putchar, NULL/*, 0*/);
}

//! \brief Disable USART completely
void usart_stop() {
    UCSRB = 0;
    (void)fdevopen(NULL,NULL);
}

//! \brief putchar() for USART.
//! \param data character to print.
int uart_putchar(char data) {
	//while (!(UCSRA & (1<<UDRE))) {};
	if (data == '\n') {
		(void)uart_putchar('\r');
	}

	//PORTD |= 1<<5;
	while (!(UCSRA & (1<<UDRE))) {};
	//PORTD &= (uint8_t)(~(1<<5));

	UDR = (uint8_t)data;

	return 0;
}

//! \brief getchar() for USART. Wait for data if not available.
//! \return value read.
//! \sa uart_available()
int uart_getchar() {
	while (!uart_available());
	
	return (int) uart_getc();
}

//! \brief Check data availability in USART buffer.
//! \return 1 if buffer is not empty.
uint8_t uart_available() {
	return rx_buffer_in != rx_buffer_out;
}

//! \brief Nonblocking, nonchecking getchar for USART. Use with care.
uint8_t uart_getc() {
	uint8_t result = rx_buffer[rx_buffer_out];
	rx_buffer_out = (rx_buffer_out + 1) % RX_BUFFER_SIZE;
	
	return result;
}

void SIG_UART_RECV( void ) __attribute__ ( ( signal ) );  
void SIG_UART_RECV( void ) {
	rx_buffer[rx_buffer_in] = (uint8_t)UDR;
	rx_buffer_in = (rx_buffer_in + 1) % RX_BUFFER_SIZE;
}

// $Id$
