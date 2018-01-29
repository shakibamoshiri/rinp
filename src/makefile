# compiler name
CC				= gcc

# compiler flags
check_error		= -Wall
check_memory	= -fsanitize=address
build_object	= -c
link_thread		= -lpthread
optimize		= -O2

# source file name
sources			= main.c rinp.c
objects			= main.o rinp.o

# target file name
filename		= -o rinp

# temporary file name
temp_files		= `echo *~`

debug:
	@echo 'debug mode:'
	@echo -n "compile: $(sources)"
	@gcc $(build_object) $(sources) $(check_error) $(check_memory)
	@echo '... okay'

	@echo -n "inking: $(objects)"
	@gcc $(objects) $(filename) $(check_error) $(check_memory) $(link_thread)
	@echo '... okay'
	@echo 'Now you can test it'

build:
	@echo 'build mode:'
	@echo -n "compile: $(sources)"
	@gcc $(build_object) $(sources) $(check_error)
	@echo '... okay'

	@echo -n "linking: $(objects)"
	@gcc $(objects) $(optimize) $(filename) $(check_error) $(link_thread)
	@echo '... okay'
	@echo 'Now you can install it by: sudo cp rinp /usr/bin/'
	@echo '"sude" is for root privilege. Use your own.'

clean:
	@echo -n "cleaning: $(objects)"
	@rm -f *.o
	@echo '... okay'

	@echo -n "cleaning: $(temp_files)"
	@rm -f *~
	@echo '... okay'
