all: myapp

# Which compiler
CC = gcc

# Where to install
INSTDIR = /usr/local/bin

# Where are include files kept
INCLUDE = .

# Options for development
#CFLAGS = -g -Wall -ansi

# Options for release
# CFLAGS = -O -Wall -ansi

CFILE = select_server

myapp: main.o 
	$(CC) -o $(CFILE) $(CFILE).c 

main.o: $(CFILE)
	$(CC) -I$(INCLUDE) $(CFLAGS) -c $(CFILE)


clean:
	-rm  $(CFILE) 

install: myapp
	@if [ -d $(INSTDIR) ]; \
	then \
		cp  ../* $(INSTDIR);\
		chmod a+x $(INSTDIR)/;\
		chmod og-w $(INSTDIR)/;\
		echo "Installed in $(INSTDIR)";\
	else \
		echo "Sorry, $(INSTDIR) does not exist";\
	fi

