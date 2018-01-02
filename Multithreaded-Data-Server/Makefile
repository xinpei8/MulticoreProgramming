CC = g++
RM = rm
TARGET = test
CCFLAGS = -std=c++11 -g
LDFLAGS = -lpthread
SRCS = httpreq.cc httpresp.cc main.cpp md5.cpp
OBJS = $(SRCS:.cc=.o)

%.o: %.cc
	$(CC) $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(TARGET)
