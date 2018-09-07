comprefix = 
CC = $(comprefix)gcc
CXX = $(comprefix)g++

base = -Wall -g -fPIC -I . 

cflags = $(base)
cppflags = $(base) -std=c++14
linkflags = $(base) 


aimwrite:cgi_write.o cgic.o libctl.so 
	$(CC) -o $@ $^ $(linkflags)

cgi_write.o:cgi_write.cpp
	$(CXX) -o $@ -c $^ $(cppflags) 

cgic.o:cgic.c
	$(CC) -o $@ -c $^ $(cflags) 

libctl.so:cgi_ctl.o
	$(CC) -o $@ $^ $(linkflags) -shared

cgi_ctl.o:cgi_ctl.cpp
	$(CXX) -o $@ -c $^ $(cppflags) 



clean:
	-rm -rf ./*.so ./*.o




