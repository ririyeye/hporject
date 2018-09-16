comprefix = arm-linux-gnueabihf-
CC = $(comprefix)gcc
CXX = $(comprefix)g++

base = -Wall -g -fPIC -I . 

cflags = $(base)
cppflags = $(base) -std=c++14
linkflags = $(base) -lpthread -L  . libsqlite3.so.0.8.6

cgi += lightoff.cgi lighton.cgi mpu6050.cgi

all:aimwrite hardconnectd test.cgi cgi_reg.cgi cgi_log.cgi $(cgi)
	#cp hardconnectd /rootfs
	#cp aimwrite /rootfs
	cp *.cgi /rootfs/www/cgi-bin
	cp *.html /rootfs/www
	cp -d *.so /rootfs/lib
	cp mpu6050/*.ko /rootfs



lightoff.cgi:lightoff.cpp libctl.so cgic.o
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

lighton.cgi:lighton.cpp libctl.so cgic.o
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

mpu6050.cgi:mpu6050.cpp libctl.so cgic.o
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

test.cgi:test.cpp libctl.so cgic.o
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

cgi_reg.cgi:cgi_reg.cpp cgic.o libctl.so
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

cgi_log.cgi:cgi_log.cpp cgic.o libctl.so 
	$(CXX) -o $@ $^ $(cppflags) $(linkflags)

aimwrite:cgi_write.o cgic.o libctl.so 
	$(CC) -o $@ $^ $(linkflags)

cgi_write.o:cgi_write.cpp
	$(CXX) -o $@ -c $^ $(cppflags) 

cgic.o:cgic.c
	$(CC) -o $@ -c $^ $(cflags) 

libctl.so:cgi_ctl.o
	$(CXX) -o $@ $^ $(linkflags) -shared

cgi_ctl.o:cgi_ctl.cpp
	$(CXX) -o $@ -c $^ $(cppflags) 

hardconnectd:hardconnect.c
	$(CC) -o $@ $^ $(cflags) 

clean:
	-rm -rf ./*.so ./*.o aimwrite hardconnectd




