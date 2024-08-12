CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = mywebserver
SOURCES = ../http/http_conn.cpp \
          ../http/httprequest.cpp \
          ../http/httpresponse.cpp \
          ../log/log.cpp \
          ../mysql_connection/sql_conn_pool.cpp \
          ../timer/heaptimer.cpp \
          ../webserver/webserver.cpp \
          ../servermain.cpp

OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o ../bin/$(TARGET) -lpthread -L/usr/lib64/mysql -lmysqlclient -ljsoncpp

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)