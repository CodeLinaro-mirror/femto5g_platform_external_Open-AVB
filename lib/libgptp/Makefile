.PHONY : clean

CPPFLAGS= -fpic -g -I../../daemons/gptp/linux/src -I../../daemons/gptp/common
LDFLAGS= -shared -lrt

SOURCES= gptp_helper.cpp
OBJECTS= gptp_helper.o
TARGET= libgptp.so 

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(OBJECTS) -o $@ $(LDFLAGS)
