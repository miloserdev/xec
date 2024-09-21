#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/file.h>



#define _reset  "\033[0m"
#define KBLK "\e[0;30m"
#define KRED  "\033[31m"
#define KGRN  "\033[32m"
#define KYEL  "\033[33m"
#define KBLU  "\033[34m"
#define KMAG  "\033[35m"
#define KCYN  "\033[36m"
#define KWHT  "\033[37m"
#define KLRD  "\033[1;31m"
#define KLGN  "\033[1;32m"
#define _inverse  "\033[1;7m"

#define _bold "\x1B[1m"
#define _underline "\x1B[4m"




#define xec_clear() printf("\033[H\033[J")
#define xec_gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

#define PORT "/dev/port"
int xec_fd = -1;

typedef enum
{
	output_buffer_full	= 0x01,
	input_buffer_full	= 0x02,
	command			= 0x08,
	bu_reset_mode		= 0x10,
	sci_event_pending	= 0x20,
	smi_event_pending	= 0x40
} xec_status_t;

typedef enum
{
	command_port		= 0x66,
	data_port		= 0x62
} xec_port_t;

typedef enum
{
	xec_read		= 0x80,
	xec_write		= 0x81,
	xec_bu_reset_enable	= 0x82,
	xec_bu_reset_disable	= 0x83,
	xec_query		= 0x84
} xec_command_t;

uint8_t xec_port_read(xec_port_t port);
uint8_t xec_port_write(xec_port_t port, uint8_t data);
int xec_wait_status(xec_status_t status, bool set);
int xec_wait_read();
int xec_wait_write();
int xec_monitor();
int xec_init();
int xec_close();

uint8_t xec_port_read(xec_port_t port)
{
	uint8_t ch = 0;

	int ls = lseek(xec_fd, port, 0);
	if ( 0 > ls )
	{
		printf("error: cannot lseek() \n");
		exit(-1);
	}

	int rd = read(xec_fd, &ch, 1);
	if ( 1 != rd )
	{
		printf("error: read != 1 \n");
		exit(-1);
	}

	return ch;
}

uint8_t xec_port_write(xec_port_t port, uint8_t data)
{
	int ls = lseek(xec_fd, port, 0);
	if ( 0 > ls )
	{
		printf("error: cannot lseek() \n");
		exit(-1);
	}

	int wd = write(xec_fd, &data, 1);
	if ( 1 != wd )
	{
		printf("error: read != 1 \n");
		exit(-1);
	}

	return 0;
}

int xec_wait_status(xec_status_t status, bool set)
{
	int timeout = 400;
	while ( timeout > 0 )
	{
		timeout--;
		uint8_t val = xec_port_read(command_port);
		val = set ? (uint8_t) ~val : val;
		if ( 0 == (status & val) )
		{
			return 0;
		}
	}

	return -1;
}

int max_waits = 20, wait_faults;

int xec_wait_read()
{
	if ( wait_faults > max_waits )
	{
		return 0;
	} else if ( xec_wait_status(output_buffer_full, true) )
	{
		wait_faults = 0;
	} else
	{
		wait_faults++;
		return 1;
	}
}

int xec_wait_write()
{
	return xec_wait_status(input_buffer_full, false);
}

char* xec_style(uint8_t data)
{
	char *x = malloc(32);
	data = (data < 30 ? 30 : data);
	sprintf(x, "\033[38;2;%d;%d;%dm", data, data, data);
	return x;
	
	switch(data)
	{
		
		case 0xFF:	{ return KGRN; break; }
		case 0x00:	{ return "\033[38;2;50;50;50m"; break; }
		default:	{ return KWHT; break; }
	}
}

int xec_monitor()
{

	printf(_reset);
	printf("  │ ");
	printf(_inverse);
	for (uint16_t x = 0; x < 16; x++)
	{
		printf("%02X%s", x, (x != 16-1 ? " " : "")); // FIX!
		//printf(_inverse "%02X" _reset " ", x);
	}
	printf(_reset "\n");
	printf("──┼");
	for (uint16_t x = 0; x < 16*3; x++)
	{
		printf("─");
	}
	//printf("\n");
	printf(_reset);

	for (uint32_t i = 0; i < 256; i++)
	{
		xec_wait_write();
		xec_port_write(command_port, xec_read);
		xec_wait_write();
		xec_port_write(data_port, (uint8_t) i);
		xec_wait_read();
		uint8_t data = xec_port_read(data_port);

		if ( (i % 16) == 0 )
		{
			printf("\n" _reset _inverse "%02X" _reset "│ ", i);
		}

		printf("%s%02X " _reset, xec_style(data), data);
	}
	printf("\n");

//	xec_gotoxy(0, 0);
//	xec_monitor();
}

int xec_init()
{
	int fd = open(PORT, O_RDWR);
	if ( 0 > fd )
	{
		printf("error: port open() \n");
		exit(-1);
	}

	xec_fd = fd;

	return 0;
}

int xec_close()
{
	int cl = close(xec_fd);
	if ( 0 > cl )
	{
		printf("error: cannot close()");
		exit(-1);
	}

	return 0;
}

