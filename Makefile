CC = gcc
CFLAGS = -Wall -pedantic
EXECUTABLE = a.out
SOURCE = tweeter_2.0_server.c 
RM = rm -f
NAME = server

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCE) 
    $(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCE)

run: 
    ./$(EXECUTABLE) $(SOURCE) $(NAME)

clean:
    $(RM) $(EXECUTABLE)
