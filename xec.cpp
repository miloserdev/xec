//#ifndef _COLORS_
//#define _COLORS_
//
///* FOREGROUND */
//#define _reset  "\x1B[0m"
//#define KRED  "\x1B[31m"
//#define KGRN  "\x1B[32m"
//#define KYEL  "\x1B[33m"
//#define KBLU  "\x1B[34m"
//#define KMAG  "\x1B[35m"
//#define KCYN  "\x1B[36m"
//#define KWHT  "\x1B[37m"
//#define KLRD  "\033[1;31m"
//#define KLGN  "\033[1;32m"
//#define _inverse  "\033[1;7m"
//
//#define FRED(x) KRED x _reset
//#define FGRN(x) KGRN x _reset
//#define FYEL(x) KYEL x _reset
//#define FBLU(x) KBLU x _reset
//#define FMAG(x) KMAG x _reset
//#define FCYN(x) KCYN x _reset
//#define FWHT(x) KWHT x _reset
//
//#define BOLD(x) "\x1B[1m" x _reset
//#define UNDL(x) "\x1B[4m" x _reset
//
//#endif  /* _COLORS_ */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <cctype>
#include <sstream>
#include <string>
using namespace std;

int ec_fd;

int max_waits = 20, wait_faults;

//(void)&ttyf;
//int ttyf = isatty(1) ? TTY : ORDINARY;

string _reset, _bright, _underline, _blink, _inverse, _black, _red, _green,
		_yellow, _blue, _magenta, _cyan, _white, _black_bg, _red_bg, _green_bg,
		_yellow_bg, _blue_bg, _magenta_bg, _cyan_bg, _white_bg;

extern char **environ;

int run(int argc, char **argv);

int main(int argc, char **argv, char **envp) {

	bool isColored = 0;
	  for (size_t i = 1; i < argc; ++i) {
	      if (argv[i] == std::string("-c") ) {
	          isColored = 1;
	      }
	  }

//	  for (char **env = envp; *env != 0; env++)
//	  {
//	    char *thisEnv = *env;
//	    printf("%s\n", thisEnv);
//	  }

	if (isatty(STDOUT_FILENO) ^ isColored) {
		_reset = "\x1b[0m";
		_bright = "\x1b[1m";
		_underline = "\x1b[4m";
		_blink = "\x1b[5m";
		_inverse = "\x1b[7m";

		_black = "\x1b[30m";
		_red = "\x1b[31m";
		_green = "\x1b[32m";
		_yellow = "\x1b[33m";
		_blue = "\x1b[34m";
		_magenta = "\x1b[35m";
		_cyan = "\x1b[36m";
		_white = "\x1b[37m";

		_black_bg = "\x1b[40m";
		_red_bg = "\x1b[41m";
		_green_bg = "\x1b[42m";
		_yellow_bg = "\x1b[43m";
		_blue_bg = "\x1b[44m";
		_magenta_bg = "\x1b[45m";
		_cyan_bg = "\x1b[46m";
		_white_bg = "\x1b[47m";
	}

	run(argc, argv);
}

int err(char *txt) {
	throw std::invalid_argument(txt);
}

int ec_init() {
	if ((ec_fd = open("/dev/port", O_RDWR)) < 0) {
		err("Cannot open /dev/port");
	}
	return 0;
}

int ec_close() {
	close(ec_fd);
	ec_fd = -1;
	return 0;
}

enum Status : int {
	output_buffer_full = 0x01,
	input_buffer_full = 0x02,
	command = 0x08,
	bu_reset_mode = 0x10,
	sci_event_pending = 0x20,
	smi_event_pending = 0x40
};

enum Ports : int {
	command_port = 0x66, data_port = 0x62
};

enum Command : int {
	_read = 0x80,
	_write = 0x81,
	bu_reset_enable = 0x82,
	bu_reset_disable = 0x83,
	query = 0x84
};

byte ec_read(uint8_t port) {

	byte ch = (byte) 0;

	if (lseek(ec_fd, port, 0) < 0) {
		err("cannot lseek");
	}

	if (read(ec_fd, &ch, 1) != 1) {
		err("cannot read");
	}

	return ch;
}

byte ec_write(uint8_t port, byte data) {
	if (lseek(ec_fd, port, 0) < 0) {
		err("cannot lseek");
	}
	if (write(ec_fd, &data, 1) != 1) {
		err("cannot write");
	}
	//return ec_read(port);
	return data;
}

//bool ec_wait_status(int port, Status status) {
//	int timeline = 200;
//	while (timeline > 0) {
//		timeline--;
//		byte f = (byte) ec_read(0x66);
//		if (((int) status && (int) f) == 0)
//			return int(1000 - timeline);
//	}
//	return false;
//}

bool ec_wait_status(int status, bool set) {
	int timeout = 500;
	while (timeout > 0) {
		timeout--;
		byte val = ec_read(Ports::command_port);
		val = set ? (byte) ~(int) val : val;
		if (((int) status & (int) val) == 0) {
			return true;
		}
	}
	return false;
}

bool ec_wait_read() {
	if (wait_faults > max_waits) {
		return true;
	} else if (ec_wait_status(Status::output_buffer_full, true)) {
		wait_faults = 0;
		return true;
	} else {
		wait_faults++;
		return false;
	}
}

bool ec_wait_write() {
	return ec_wait_status(Status::input_buffer_full, false);
}

int monitor() {

	cout << "  │ ";
	for (uint16_t x = 0; x < 16; x++) {
		string append = ((int) x < 16 ? "0" : "");
		cout << _inverse << append << uppercase << std::hex << x << " ";
	}
	cout << _reset;
	cout << endl << "──┼";
	for (uint16_t x = 0; x < 16 * 3; x++) {
		cout << "─";
	}
	cout << endl;

	for (int i = 0; i < 256; i++) {
		ec_wait_write();
		ec_write(Ports::command_port, (byte) Command::_read);
		ec_wait_write();
		ec_write(Ports::data_port, (byte) (i));
		ec_wait_read();
		byte data = ec_read(Ports::data_port);

		if (i == 0)
			cout << _inverse << "00" << _reset << "│ ";

		if ((i % 16) == 0 && i != 0) {
			cout << endl;
			cout << _reset << _inverse << uppercase << i << _reset << "│ ";
		}

		string append = ((int) data < 16 ? "0" : "");

		cout << uppercase
				<< ((int) data == 0xFF ? (_green + _bright) : (int) data == 0x00 ? (_black + _bright) : (_red + _bright));
		cout << append << uppercase << std::hex << (int) data << " ";
		//cout << std::hex << append << (int) data << " ";
	}
	cout << endl;
}

int help() {
	cout << "eXecute EC" << endl;
	cout << "<commands>" << endl;
	cout << "  xec read 0xXX : read value from EC memory" << endl;
	cout << "  xec write 0xXX 0xXX : write value to EC memory" << endl;
	cout << "  xec monitor : full table of EC memory" << endl;
	cout << endl;

	cout << "<example>" << endl;
	cout
			<< "  watch -n 1 -c -d sudo xec monitor : print EC table with changes highlight every second"
			<< endl;
	cout << endl;

	cout << "<arguments>" << endl;
	cout
			<< "  -c : enable colors"
			<< endl;
}

#include <cstdlib>
#include <iostream>

int arg_to_int(char *arg) {
	int c = (int) atol(arg);
	c = (c > (int) 0 ? c : (int) std::stoul(arg, nullptr, 16));
	return c;
}

int run(int argc, char **argv) {

	ec_init();

	if (argv[1] != NULL) {
		if (strcmp(argv[1], "read") == 0) {

			if (argv[2] == NULL)
				err("no address");

			ec_wait_write();
			ec_write(Ports::command_port, (byte) Command::_read);
			ec_wait_write();
			ec_write(Ports::data_port, (byte) arg_to_int(argv[2]));
			ec_wait_read();
			cout << argv[2] << ": " << uppercase << std::hex << "0x"
					<< (int) ec_read(Ports::data_port) << endl;

		} else if (strcmp(argv[1], "write") == 0) {

			if (argv[2] == NULL)
				err("no 1 address");
			if (argv[3] == NULL)
				err("no 2 address");

			ec_wait_write();
			ec_write(Ports::command_port, (byte) Command::_write);
			ec_wait_write();
			ec_write(Ports::data_port, (byte) arg_to_int(argv[2]));
			ec_wait_write();
			ec_write(Ports::data_port, (byte) arg_to_int(argv[3]));
			ec_wait_read();
			ec_read(Ports::data_port);

			cout << uppercase << std::hex << argv[2] << ": " << argv[3] << endl;
		} else if (strcmp(argv[1], "monitor") == 0) {
			monitor();
		} else if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "") == 0) {
			help();
		} else if (strcmp(argv[1], "cti") == 0 || strcmp(argv[1], "") == 0) {

			cout << arg_to_int(argv[2]) << endl;
			cout << argv[2] << endl;

		} else {
			cout << "unknown command" << endl;
			return 0;
		}
	} else {
		help();
		//cout << "type help" << endl;
	}

	ec_close();
}
