//! \file
//! \brief USART interface
#ifndef _USRAT_H
#define _USRAT_H

#define RX_BUFFER_SIZE	4					//!< USART RX buffer length

void usart_init(uint16_t baudrate);
int uart_putchar(char data);
int uart_getchar();
uint8_t uart_available(void);
uint8_t uart_getc();

#endif

// $Id: usrat.h,v 1.2 2006/05/29 23:11:41 svo Exp $
