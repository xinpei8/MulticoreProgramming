CC = g++
RM = rm
TARGET = test-httpreq_jess
CCFLAGS = -std=c++11 -g
LDFLAGS = -lpthread
SRCS = httpreq.cc httpresp.cc test-httpreq_jess.cc
OBJS = $(SRCS:.cc=.o)

%.o: %.cc
	$(CC) $(CCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	$(RM) -f $(OBJS) $(TARGET)
