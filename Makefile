CC  = g++
CFLAGS    =
TARGET  = model
SRCS    = model.cpp
OBJS    = $(SRCS:.cpp=.o)
INCDIR  = -I../inc
LIBDIR  = 
LIBS    = 

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBDIR) $(LIBS)
	
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCDIR) -c $(SRCS)

plot:
	gnuplot plot.gp

all: clean $(OBJS) $(TARGET)

clean:
	-rm -f $(OBJS) $(TARGET) *.d
