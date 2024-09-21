#include <string.h>
#include <ctype.h>

#include "xec.c"

#define debug(__format, ...) { printf("___ %s : %d : %s ___ "__format" \n", \
                                        __FILE__, __LINE__, __func__, __VA_ARGS__); }

int init_atom(char atom);
int init_arg_long(char *arg);


/// XEC params
bool _xec_monitor = 0;
bool _xec_read = 0;
bool _xec_write = 0;


int arg_to_int(char *arg) {
	int c = (int) atol(arg);
	c = (c > (int) 0 ? c : (int) strtoul(arg, NULL, 16));
	return c;
}

int main(int argc, char *argv[])
{
        int err_code = 0;
        xec_init();

        for ( size_t i = 0; i < argc; i++ )
        {
                debug("arg %d == %s", i, argv[i]);

                if (0 == strcmp(argv[i], "read"))
                {
                        /* read */
                        if (argv[i+1] == NULL) { debug("%s ", "read address malfunction"); err_code = -1; goto exit; }

                        uint8_t a = arg_to_int(argv[i+1]);

                        debug("read from %02x", a);

                        xec_wait_write();
			xec_port_write(command_port, xec_read);
			xec_wait_write();
			xec_port_write(data_port, a);
			xec_wait_read();

                        uint8_t data = xec_port_read(data_port);
                        debug("%02x a == %02x", a, data);

                        return 0;
                }
                else if (0 == strcmp(argv[i], "write"))
                {
                        /* read */
                        if (argv[i+1] == NULL) { debug("%s ", "read address malfunction"); err_code = -1; goto exit; }
                        if (argv[i+2] == NULL) { debug("%s ", "write address malfunction"); err_code = -1; goto exit; }

                        uint8_t a = arg_to_int(argv[i+1]);
                        uint8_t b = arg_to_int(argv[i+2]);

                        debug("write %02x to %02x", a, b);
                        
                        xec_wait_write();
			xec_port_write(command_port, xec_write);
			xec_wait_write();
			xec_port_write(data_port, a);
                        xec_wait_write();
                        xec_port_write(data_port, b);
			xec_wait_read();


                        return 0;
                }

                else if (0 == strcmp(argv[i], "monitor"))
                {
                        /* read */
                        xec_monitor();

                        return 0;
                }
        }

exit:
        xec_close();
        debug("err code %d", err_code);
        exit(err_code);
}



/* 
	xec_init();

	xec_clear();
	xec_monitor();
 */


 int init_atom(char atom)
{
        switch (atom)
        {
                case 'm':       { debug("%s ", "m"); _xec_monitor = 1; break; }
                case 'r' :      { debug("%s ", "r"); _xec_read = 1; break; }
                case 'w' :      { debug("%s ", "w"); _xec_write = 1; break; }
        }
}

int init_arg_long(char *arg)
{
        if ( strcmp(arg, "--monitor") == 0 )
        {
                debug("%s ", "--monitor");
                _xec_monitor = 1;
        }

        else if ( strcmp(arg, "--read") == 0 )
        {
                debug("%s ", "--read");
                _xec_read = 1;
        }

        else if ( strcmp(arg, "--write") == 0 )
        {
                debug("%s ", "--write");
                _xec_write = 1;
        }


        else if ( strcmp(arg, "--help") == 0 )
        {
                debug("%s ", "--help");
        }
}